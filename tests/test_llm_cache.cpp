#include "llm_wire.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "json.h"

// Prompt caching (chapter 1.6): the cache markers of the Anthropic request, and the cached tokens that the
// three kinds of answer report.

namespace {

using Json = json::Value;

llm::Provider ProviderFor(llm::Protocol protocol) {
  return llm::Provider{"test", protocol, llm::Endpoint{"https://example.test/v1/", "sk-secret"}, "some-model", {true, 1000, 100000}};
}

llm::Message User(const std::string& text) { return {llm::Role::kUser, {llm::TextBlock{text}}}; }
llm::Message Assistant(std::vector<llm::Block> blocks) { return {llm::Role::kAssistant, std::move(blocks)}; }
llm::Block Text(const std::string& text) { return llm::TextBlock{text}; }

llm::Conversation Question(const std::string& system = "Be brief.") {
  llm::Conversation conversation;
  conversation.system = system;
  conversation.messages.push_back(User("Say hello."));
  return conversation;
}

llm::Options Caching() {
  llm::Options options;
  options.max_tokens = 256;
  options.cache = true;
  return options;
}

llm::ToolDefinition SomeTool(const std::string& name) { return {name, "the tool " + name, json::Parse(R"({"type":"object"})")}; }

const Json kMarker = {{"type", "ephemeral"}};

Json AnthropicBody(const llm::Conversation& conversation, const llm::Options& options) {
  return json::Parse(llm::anthropic::BuildRequest(ProviderFor(llm::Protocol::kAnthropic), conversation, options).body);
}

std::size_t Markers(const std::string& body) {
  std::size_t count = 0;
  for (std::size_t at = body.find("cache_control"); at != std::string::npos; at = body.find("cache_control", at + 1)) ++count;
  return count;
}

}  // namespace

TEST(AnthropicCache, TheInstructionBecomesABlockWithAMarkerWhenCachingIsOn) {
  const Json body = AnthropicBody(Question(), Caching());
  ASSERT_TRUE(body["system"].is_array());
  ASSERT_EQ(body["system"].size(), 1u);
  EXPECT_EQ(body["system"][0], (Json{{"type", "text"}, {"text", "Be brief."}, {"cache_control", kMarker}}));
}

TEST(AnthropicCache, WithCachingOffNothingChangesEvenWhenThereArePoints) {
  llm::Conversation conversation = Question();
  conversation.cache_points = {0};
  llm::Options options = Caching();
  options.cache = false;
  const std::string text = llm::anthropic::BuildRequest(ProviderFor(llm::Protocol::kAnthropic), conversation, options).body;
  EXPECT_EQ(Markers(text), 0u);
  EXPECT_EQ(json::Parse(text)["system"], "Be brief.");
}

TEST(AnthropicCache, WithoutAnInstructionTheLastToolCarriesTheMarker) {
  llm::Options options = Caching();
  options.tools = {SomeTool("a"), SomeTool("b")};
  const Json body = AnthropicBody(Question(""), options);
  EXPECT_FALSE(body.contains("system"));
  EXPECT_FALSE(body["tools"][0].contains("cache_control"));
  EXPECT_EQ(body["tools"][1]["cache_control"], kMarker);
}

TEST(AnthropicCache, WithAnInstructionTheMarkerIsOnTheInstructionNotOnTheTools) {
  llm::Options options = Caching();
  options.tools = {SomeTool("a")};
  const Json body = AnthropicBody(Question(), options);
  EXPECT_FALSE(body["tools"][0].contains("cache_control"));  // the marker after the instruction covers the tools too
  EXPECT_EQ(body["system"][0]["cache_control"], kMarker);
}

TEST(AnthropicCache, TheLastBlockOfEachMessageAtACachePointCarriesAMarker) {
  llm::Conversation conversation;
  conversation.messages = {User("first"), Assistant({Text("hello"), Text("again")}), User("second")};
  conversation.cache_points = {1, 2};
  const Json body = AnthropicBody(conversation, Caching());
  EXPECT_FALSE(body["messages"][0]["content"][0].contains("cache_control"));
  EXPECT_FALSE(body["messages"][1]["content"][0].contains("cache_control"));  // not the first block of the message
  EXPECT_EQ(body["messages"][1]["content"][1]["cache_control"], kMarker);
  EXPECT_EQ(body["messages"][2]["content"][0]["cache_control"], kMarker);
}

TEST(AnthropicCache, AToolResultCanCarryTheMarker) {
  llm::Conversation conversation;
  conversation.messages.push_back(User("add"));
  conversation.messages.push_back(Assistant({llm::ToolCallBlock{"c1", "add", Json{{"a", 1}}, "", ""}}));
  conversation.messages.push_back({llm::Role::kUser, {llm::ToolResultBlock{"c1", "3", false}}});
  conversation.cache_points = {2};
  const Json body = AnthropicBody(conversation, Caching());
  EXPECT_EQ(body["messages"][2]["content"][0]["type"], "tool_result");
  EXPECT_EQ(body["messages"][2]["content"][0]["cache_control"], kMarker);
}

TEST(AnthropicCache, AMessageThatIsLeftOutTakesItsMarkerWithItAndTheRestStaysInPlace) {
  llm::Conversation conversation;
  conversation.messages.push_back(User("one"));
  // a reasoning block of another protocol: there is nothing in this message to send
  conversation.messages.push_back(Assistant({llm::ReasoningBlock{llm::Protocol::kOpenAIResponses, Json{{"type", "reasoning"}}, ""}}));
  conversation.messages.push_back(User("two"));
  conversation.cache_points = {1, 2};
  const Json body = AnthropicBody(conversation, Caching());
  ASSERT_EQ(body["messages"].size(), 2u);
  EXPECT_FALSE(body["messages"][0]["content"][0].contains("cache_control"));
  EXPECT_EQ(body["messages"][1]["content"][0]["cache_control"], kMarker);
}

TEST(AnthropicCache, NeverMoreThanFourMarkers) {
  llm::Conversation conversation = Question();
  for (int i = 0; i < 8; ++i) conversation.messages.push_back(User("m" + std::to_string(i)));
  for (std::size_t i = 0; i < conversation.messages.size(); ++i) conversation.cache_points.push_back(i);
  llm::Options options = Caching();
  options.tools = {SomeTool("a")};
  EXPECT_EQ(Markers(llm::anthropic::BuildRequest(ProviderFor(llm::Protocol::kAnthropic), conversation, options).body), 4u);
}

TEST(AnthropicCache, TheOtherTwoProtocolsDoNotChange) {
  llm::Conversation conversation = Question();
  conversation.cache_points = {0};
  llm::Options plain;
  plain.max_tokens = 256;
  const llm::Provider chat = ProviderFor(llm::Protocol::kOpenAIChat);
  const llm::Provider responses = ProviderFor(llm::Protocol::kOpenAIResponses);
  EXPECT_EQ(llm::openai_chat::BuildRequest(chat, conversation, Caching()).body, llm::openai_chat::BuildRequest(chat, conversation, plain).body);
  EXPECT_EQ(llm::openai_responses::BuildRequest(responses, conversation, Caching()).body,
            llm::openai_responses::BuildRequest(responses, conversation, plain).body);
}

TEST(AnthropicCache, TheTokensReadFromAndWrittenToTheCacheAreInTheTotal) {
  const llm::Reply reply = llm::anthropic::ParseReply(
      R"({"content": [{"type": "text", "text": "ok"}], "stop_reason": "end_turn",
          "usage": {"input_tokens": 12, "cache_creation_input_tokens": 300, "cache_read_input_tokens": 500, "output_tokens": 4}})");
  EXPECT_EQ(reply.input_tokens, 812);  // what the service counted altogether
  EXPECT_EQ(reply.cached_tokens, 500);
  EXPECT_EQ(reply.cache_written_tokens, 300);
}

TEST(AnthropicCache, WithoutCacheFieldsTheCountsAreAsBefore) {
  const llm::Reply reply = llm::anthropic::ParseReply(
      R"({"content": [{"type": "text", "text": "ok"}], "usage": {"input_tokens": 12, "output_tokens": 4}})");
  EXPECT_EQ(reply.input_tokens, 12);
  EXPECT_FALSE(reply.cached_tokens.has_value());
  EXPECT_FALSE(reply.cache_written_tokens.has_value());
}

TEST(OpenAICache, ChatCompletionsReportsCachedTokensInThePromptDetails) {
  const llm::Reply reply = llm::openai_chat::ParseReply(
      R"({"choices": [{"message": {"role": "assistant", "content": "ok"}, "finish_reason": "stop"}],
          "usage": {"prompt_tokens": 1500, "completion_tokens": 3, "prompt_tokens_details": {"cached_tokens": 1024}}})");
  EXPECT_EQ(reply.input_tokens, 1500);
  EXPECT_EQ(reply.cached_tokens, 1024);
  EXPECT_FALSE(reply.cache_written_tokens.has_value());
}

TEST(OpenAICache, ResponsesReportsCachedTokensInTheInputDetails) {
  const llm::Reply reply = llm::openai_responses::ParseReply(
      R"({"status": "completed", "output": [{"type": "message", "role": "assistant", "content": [{"type": "output_text", "text": "ok"}]}],
          "usage": {"input_tokens": 1500, "output_tokens": 3, "input_tokens_details": {"cached_tokens": 1280}}})");
  EXPECT_EQ(reply.input_tokens, 1500);
  EXPECT_EQ(reply.cached_tokens, 1280);
}

TEST(OpenAICache, ACountThatIsNotThereIsNotMadeUp) {
  const llm::Reply reply = llm::openai_chat::ParseReply(
      R"({"choices": [{"message": {"role": "assistant", "content": "ok"}, "finish_reason": "stop"}], "usage": {"prompt_tokens": 9, "completion_tokens": 2}})");
  EXPECT_FALSE(reply.cached_tokens.has_value());
}
