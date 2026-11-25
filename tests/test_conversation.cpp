#include "conversation.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

using Json = json::Value;

llm::ReasoningBlock Reasoning(llm::Protocol origin) {
  return llm::ReasoningBlock{origin, Json{{"type", "reasoning"}}, ""};
}

llm::Message Assistant(std::vector<llm::Block> blocks) {
  return llm::Message{llm::Role::kAssistant, std::move(blocks)};
}

}  // namespace

TEST(Conversation, AnEmptyConversationHasNoForeignReasoning) {
  EXPECT_EQ(llm::CountForeignReasoning(llm::Conversation{}, llm::Protocol::kAnthropic), 0u);
}

TEST(Conversation, TextBlocksAreNeverForeign) {
  llm::Conversation conversation;
  conversation.messages.push_back({llm::Role::kUser, {llm::TextBlock{"hello"}}});
  conversation.messages.push_back(Assistant({llm::TextBlock{"hi"}}));
  EXPECT_EQ(llm::CountForeignReasoning(conversation, llm::Protocol::kOpenAIChat), 0u);
}

TEST(Conversation, CountsTheReasoningBlocksThatCameFromOtherProtocols) {
  llm::Conversation conversation;
  conversation.messages.push_back({llm::Role::kUser, {llm::TextBlock{"q1"}}});
  conversation.messages.push_back(
      Assistant({Reasoning(llm::Protocol::kOpenAIResponses), Reasoning(llm::Protocol::kOpenAIResponses),
                 llm::TextBlock{"a1"}}));
  conversation.messages.push_back({llm::Role::kUser, {llm::TextBlock{"q2"}}});
  conversation.messages.push_back(Assistant({Reasoning(llm::Protocol::kAnthropic), llm::TextBlock{"a2"}}));

  EXPECT_EQ(llm::CountForeignReasoning(conversation, llm::Protocol::kAnthropic), 2u);
  EXPECT_EQ(llm::CountForeignReasoning(conversation, llm::Protocol::kOpenAIResponses), 1u);
  EXPECT_EQ(llm::CountForeignReasoning(conversation, llm::Protocol::kOpenAIChat), 3u);
}
