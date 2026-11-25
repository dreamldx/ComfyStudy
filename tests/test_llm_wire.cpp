#include "llm_wire.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "json.h"

namespace {

using Json = json::Value;

std::string HeaderValue(const llm::HttpRequest& request, const std::string& name) {
  for (const auto& header : request.headers) {
    if (header.first == name) return header.second;
  }
  return "<missing>";
}

llm::Provider ProviderFor(llm::Protocol protocol, const std::string& base_url = "https://example.test/v1/") {
  return llm::Provider{"test", protocol, llm::Endpoint{base_url, "sk-secret"}, "some-model", {true, 1000, 100000}};
}

llm::Message User(const std::string& text) { return {llm::Role::kUser, {llm::TextBlock{text}}}; }

llm::Message Assistant(std::vector<llm::Block> blocks) { return {llm::Role::kAssistant, std::move(blocks)}; }

llm::Block Reasoning(llm::Protocol origin, Json payload, const std::string& visible = "") {
  return llm::ReasoningBlock{origin, std::move(payload), visible};
}

llm::Block Text(const std::string& text) { return llm::TextBlock{text}; }

llm::Conversation Question(const std::string& system = "Be brief.") {
  llm::Conversation conversation;
  conversation.system = system;
  conversation.messages.push_back(User("Say hello."));
  return conversation;
}

// user, assistant (with `answer` as its blocks), user
llm::Conversation TwoTurns(std::vector<llm::Block> answer, const std::string& system = "") {
  llm::Conversation conversation;
  conversation.system = system;
  conversation.messages.push_back(User("first"));
  conversation.messages.push_back(Assistant(std::move(answer)));
  conversation.messages.push_back(User("second"));
  return conversation;
}

llm::Options Limit(int max_tokens, llm::Reasoning reasoning = llm::Reasoning::kOff) {
  llm::Options options;
  options.max_tokens = max_tokens;
  options.reasoning = reasoning;
  return options;
}

const Json kSignedThinking = {{"type", "thinking"}, {"thinking", "Plan: greet."}, {"signature", "SIG=="}};
const Json kRedactedThinking = {{"type", "redacted_thinking"}, {"data", "ENCRYPTED=="}};
const Json kReasoningItem = {{"type", "reasoning"}, {"id", "rs_1"}, {"summary", json::Array()},
                             {"encrypted_content", "ENC=="}};

// ---- OpenAI Chat Completions -------------------------------------------------------------------

TEST(OpenAiChat, TheRequestHasTheInstructionAsAMessageOfItsOwn) {
  const llm::HttpRequest request = llm::openai_chat::BuildRequest(ProviderFor(llm::Protocol::kOpenAIChat), Question(), Limit(256));
  EXPECT_EQ(request.url, "https://example.test/v1/chat/completions");  // the trailing slash is dropped
  EXPECT_EQ(HeaderValue(request, "Authorization"), "Bearer sk-secret");
  EXPECT_EQ(HeaderValue(request, "Content-Type"), "application/json");

  const Json body = json::Parse(request.body);
  EXPECT_EQ(body["model"], "some-model");
  EXPECT_EQ(body["max_completion_tokens"], 256);
  ASSERT_EQ(body["messages"].size(), 2u);
  EXPECT_EQ(body["messages"][0], (Json{{"role", "system"}, {"content", "Be brief."}}));
  EXPECT_EQ(body["messages"][1], (Json{{"role", "user"}, {"content", "Say hello."}}));
}

TEST(OpenAiChat, WithoutAnInstructionThereIsOnlyTheUserMessage) {
  const Json body = json::Parse(llm::openai_chat::BuildRequest(ProviderFor(llm::Protocol::kOpenAIChat), Question(""), Limit(256)).body);
  ASSERT_EQ(body["messages"].size(), 1u);
  EXPECT_EQ(body["messages"][0]["role"], "user");
}

TEST(OpenAiChat, ATwoTurnConversationKeepsItsOrderAndRoles) {
  const Json body = json::Parse(
      llm::openai_chat::BuildRequest(ProviderFor(llm::Protocol::kOpenAIChat), TwoTurns({Text("answer")}, "Be brief."), Limit(256)).body);
  ASSERT_EQ(body["messages"].size(), 4u);
  EXPECT_EQ(body["messages"][0]["role"], "system");
  EXPECT_EQ(body["messages"][1], (Json{{"role", "user"}, {"content", "first"}}));
  EXPECT_EQ(body["messages"][2], (Json{{"role", "assistant"}, {"content", "answer"}}));
  EXPECT_EQ(body["messages"][3], (Json{{"role", "user"}, {"content", "second"}}));
}

TEST(OpenAiChat, ReasoningBlocksAreNeverSentAndAMessageWithNothingToSendIsLeftOut) {
  const llm::Conversation conversation = TwoTurns({Reasoning(llm::Protocol::kOpenAIResponses, kReasoningItem)});
  const Json body = json::Parse(llm::openai_chat::BuildRequest(ProviderFor(llm::Protocol::kOpenAIChat), conversation, Limit(256)).body);
  ASSERT_EQ(body["messages"].size(), 2u);
  EXPECT_EQ(body["messages"][0]["content"], "first");
  EXPECT_EQ(body["messages"][1]["content"], "second");

  const Json with_text = json::Parse(llm::openai_chat::BuildRequest(
      ProviderFor(llm::Protocol::kOpenAIChat), TwoTurns({Reasoning(llm::Protocol::kAnthropic, kSignedThinking), Text("answer")}),
      Limit(256)).body);
  EXPECT_EQ(with_text["messages"][1], (Json{{"role", "assistant"}, {"content", "answer"}}));
}

TEST(OpenAiChat, TheReasoningLevelIsTheReasoningEffort) {
  const llm::Provider provider = ProviderFor(llm::Protocol::kOpenAIChat);
  EXPECT_FALSE(json::Parse(llm::openai_chat::BuildRequest(provider, Question(), Limit(256)).body).contains("reasoning_effort"));
  EXPECT_EQ(json::Parse(llm::openai_chat::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kLow)).body)["reasoning_effort"], "low");
  EXPECT_EQ(json::Parse(llm::openai_chat::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kMedium)).body)["reasoning_effort"], "medium");
  EXPECT_EQ(json::Parse(llm::openai_chat::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kHigh)).body)["reasoning_effort"], "high");
}

TEST(OpenAiChat, TextThatIsNotValidUtf8IsAnErrorWithAClearMessage) {
  llm::Conversation conversation;
  conversation.messages.push_back(User("caf\xE9"));  // Latin-1, not UTF-8
  try {
    llm::openai_chat::BuildRequest(ProviderFor(llm::Protocol::kOpenAIChat), conversation, Limit(256));
    FAIL() << "expected llm::Error";
  } catch (const llm::Error& e) {
    EXPECT_NE(std::string(e.what()).find("UTF-8"), std::string::npos) << e.what();
  }
}

TEST(Utf8, EveryProtocolRejectsBrokenTextAndAcceptsGoodText) {
  using Build = llm::HttpRequest (*)(const llm::Provider&, const llm::Conversation&, const llm::Options&);
  const std::vector<std::pair<Build, llm::Protocol>> protocols = {{llm::openai_chat::BuildRequest, llm::Protocol::kOpenAIChat},
                                                                   {llm::openai_responses::BuildRequest, llm::Protocol::kOpenAIResponses},
                                                                   {llm::anthropic::BuildRequest, llm::Protocol::kAnthropic}};
  const std::vector<std::string> broken = {"\xC0\x80",             // overlong
                                           "\xED\xA0\x80",         // a surrogate
                                           "\xE4\xBD",             // cut off in the middle of a character
                                           "\xF5\x80\x80\x80",     // beyond U+10FFFF
                                           std::string("a\0b", 3)};  // a NUL byte
  const std::vector<std::string> good = {"plain", "\xE4\xBD\xA0\xE5\xA5\xBD", "\xF0\x9F\x98\x80"};  // ASCII, Chinese, an emoji
  for (const auto& [build, protocol] : protocols) {
    const llm::Provider provider = ProviderFor(protocol, "https://a.test");
    for (const std::string& text : broken) {
      llm::Conversation in_message;
      in_message.messages.push_back(User(text));
      EXPECT_THROW(build(provider, in_message, Limit(256)), llm::Error) << "a message, protocol " << static_cast<int>(protocol);
      llm::Conversation in_system = Question("");
      in_system.system = text;
      EXPECT_THROW(build(provider, in_system, Limit(256)), llm::Error) << "the instruction, protocol " << static_cast<int>(protocol);
    }
    for (const std::string& text : good) {
      llm::Conversation conversation;
      conversation.messages.push_back(User(text));
      EXPECT_NO_THROW(build(provider, conversation, Limit(256))) << "protocol " << static_cast<int>(protocol);
    }
  }
}

TEST(Utf8, ANulByteHasItsOwnMessage) {
  llm::Conversation conversation;
  conversation.messages.push_back(User(std::string("a\0b", 3)));
  try {
    llm::openai_chat::BuildRequest(ProviderFor(llm::Protocol::kOpenAIChat), conversation, Limit(256));
    FAIL() << "expected llm::Error";
  } catch (const llm::Error& e) {
    EXPECT_NE(std::string(e.what()).find("NUL"), std::string::npos) << e.what();
  }
}

TEST(OpenAiChat, TheTextIsInTheFirstChoice) {
  const std::string answer = R"({
    "id": "chatcmpl-1", "object": "chat.completion", "created": 1, "model": "some-model",
    "choices": [{"index": 0, "message": {"role": "assistant", "content": "Hello!", "refusal": null},
                 "finish_reason": "stop"}],
    "usage": {"prompt_tokens": 9, "completion_tokens": 2, "total_tokens": 11}})";
  const llm::Reply reply = llm::openai_chat::ParseReply(answer);
  EXPECT_EQ(reply.Text(), "Hello!");
  EXPECT_EQ(reply.message.role, llm::Role::kAssistant);
  ASSERT_EQ(reply.message.blocks.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<llm::TextBlock>(reply.message.blocks[0]));
  EXPECT_EQ(reply.stop_reason, "stop");
  EXPECT_EQ(reply.input_tokens, 9);
  EXPECT_EQ(reply.output_tokens, 2);
}

TEST(OpenAiChat, NullContentGivesNoTextAndKeepsTheReasonForStopping) {
  const std::string answer = R"({"choices": [{"message": {"role": "assistant", "content": null},
                                              "finish_reason": "length"}]})";
  const llm::Reply reply = llm::openai_chat::ParseReply(answer);
  EXPECT_EQ(reply.Text(), "");
  EXPECT_TRUE(reply.message.blocks.empty());
  EXPECT_EQ(reply.stop_reason, "length");
  EXPECT_FALSE(reply.input_tokens.has_value());
}

TEST(OpenAiChat, AnAnswerWithoutChoicesOrWithoutJsonIsAnError) {
  EXPECT_THROW(llm::openai_chat::ParseReply(R"({"id": "x"})"), llm::Error);
  EXPECT_THROW(llm::openai_chat::ParseReply("<html>bad gateway</html>"), llm::Error);
  EXPECT_THROW(llm::openai_chat::ParseReply("[1, 2]"), llm::Error);
}

TEST(OpenAiChat, TheErrorTextHasTypeAndMessage) {
  const std::string error = R"({"error": {"message": "Incorrect API key provided",
                                "type": "invalid_request_error", "param": null,
                                "code": "invalid_api_key"}})";
  EXPECT_EQ(llm::openai_chat::ErrorText(error), "invalid_request_error: Incorrect API key provided");
  EXPECT_EQ(llm::openai_chat::ErrorText("plain text"), "plain text");
}

// ---- OpenAI Responses --------------------------------------------------------------------------

TEST(OpenAiResponses, TheInstructionIsAFieldOfItsOwnAndTheInputIsAListOfItems) {
  const llm::HttpRequest request =
      llm::openai_responses::BuildRequest(ProviderFor(llm::Protocol::kOpenAIResponses), Question(), Limit(256));
  EXPECT_EQ(request.url, "https://example.test/v1/responses");
  EXPECT_EQ(HeaderValue(request, "Authorization"), "Bearer sk-secret");

  const Json body = json::Parse(request.body);
  EXPECT_EQ(body["model"], "some-model");
  EXPECT_EQ(body["instructions"], "Be brief.");
  ASSERT_EQ(body["input"].size(), 1u);
  EXPECT_EQ(body["input"][0], (Json{{"role", "user"}, {"content", "Say hello."}}));
  EXPECT_EQ(body["max_output_tokens"], 256);
  EXPECT_FALSE(body.contains("reasoning"));
  EXPECT_FALSE(body.contains("store"));
}

TEST(OpenAiResponses, FewerThanSixteenOutputTokensAreRaisedToSixteen) {
  const Json body = json::Parse(
      llm::openai_responses::BuildRequest(ProviderFor(llm::Protocol::kOpenAIResponses), Question(""), Limit(1)).body);
  EXPECT_EQ(body["max_output_tokens"], 16);
  EXPECT_FALSE(body.contains("instructions"));
}

TEST(OpenAiResponses, AReasoningItemOfTheSameProtocolIsSentBackAsItIsBeforeItsMessage) {
  const llm::Conversation conversation =
      TwoTurns({Reasoning(llm::Protocol::kOpenAIResponses, kReasoningItem), Text("answer")});
  const Json body = json::Parse(
      llm::openai_responses::BuildRequest(ProviderFor(llm::Protocol::kOpenAIResponses), conversation, Limit(256)).body);
  ASSERT_EQ(body["input"].size(), 4u);
  EXPECT_EQ(body["input"][0], (Json{{"role", "user"}, {"content", "first"}}));
  EXPECT_EQ(body["input"][1], kReasoningItem);
  EXPECT_EQ(body["input"][2], (Json{{"role", "assistant"}, {"content", "answer"}}));
  EXPECT_EQ(body["input"][3], (Json{{"role", "user"}, {"content", "second"}}));
}

TEST(OpenAiResponses, AReasoningBlockFromAnotherProtocolIsLeftOut) {
  const llm::Conversation conversation =
      TwoTurns({Reasoning(llm::Protocol::kAnthropic, kSignedThinking), Text("answer")});
  const Json body = json::Parse(
      llm::openai_responses::BuildRequest(ProviderFor(llm::Protocol::kOpenAIResponses), conversation, Limit(256)).body);
  ASSERT_EQ(body["input"].size(), 3u);
  EXPECT_EQ(body["input"][1], (Json{{"role", "assistant"}, {"content", "answer"}}));
}

TEST(OpenAiResponses, ReasoningIsAskedForWithAnEffortAndNothingIsStoredAtTheService) {
  const llm::Provider provider = ProviderFor(llm::Protocol::kOpenAIResponses);
  const Json body = json::Parse(llm::openai_responses::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kMedium)).body);
  EXPECT_EQ(body["reasoning"]["effort"], "medium");
  EXPECT_EQ(body["store"], false);
  EXPECT_EQ(body["include"], json::Array({"reasoning.encrypted_content"}));
  EXPECT_EQ(json::Parse(llm::openai_responses::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kLow)).body)["reasoning"]["effort"], "low");
  EXPECT_EQ(json::Parse(llm::openai_responses::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kHigh)).body)["reasoning"]["effort"], "high");
}

TEST(OpenAiResponses, TheTextIsInTheMessageItemAfterTheReasoningItem) {
  const std::string answer = R"({
    "id": "resp_1", "object": "response", "status": "completed", "model": "some-model",
    "output": [
      {"type": "reasoning", "id": "rs_1", "encrypted_content": "ENC==",
       "summary": [{"type": "summary_text", "text": "Plan"}, {"type": "summary_text", "text": "say hi"}]},
      {"type": "message", "id": "msg_1", "role": "assistant",
       "content": [{"type": "output_text", "text": "Hello", "annotations": []},
                   {"type": "output_text", "text": "!", "annotations": []}]}],
    "usage": {"input_tokens": 9, "output_tokens": 5, "total_tokens": 14}})";
  const llm::Reply reply = llm::openai_responses::ParseReply(answer);
  EXPECT_EQ(reply.Text(), "Hello!");
  EXPECT_EQ(reply.stop_reason, "completed");
  EXPECT_EQ(reply.input_tokens, 9);
  EXPECT_EQ(reply.output_tokens, 5);
  ASSERT_EQ(reply.message.blocks.size(), 2u);
  const auto* reasoning = std::get_if<llm::ReasoningBlock>(&reply.message.blocks[0]);
  ASSERT_NE(reasoning, nullptr);
  EXPECT_EQ(reasoning->origin, llm::Protocol::kOpenAIResponses);
  EXPECT_EQ(reasoning->payload["id"], "rs_1");
  EXPECT_EQ(reasoning->payload["encrypted_content"], "ENC==");
  EXPECT_EQ(reasoning->visible_text, "Plan\nsay hi");
  EXPECT_TRUE(std::holds_alternative<llm::TextBlock>(reply.message.blocks[1]));
}

TEST(OpenAiResponses, AnIncompleteAnswerGivesTheReasonFromIncompleteDetails) {
  const std::string answer = R"({"status": "incomplete",
                                 "incomplete_details": {"reason": "max_output_tokens"}, "output": []})";
  const llm::Reply reply = llm::openai_responses::ParseReply(answer);
  EXPECT_EQ(reply.Text(), "");
  EXPECT_EQ(reply.stop_reason, "max_output_tokens");
}

TEST(OpenAiResponses, AnAnswerWithoutOutputIsAnError) {
  EXPECT_THROW(llm::openai_responses::ParseReply(R"({"status": "completed"})"), llm::Error);
}

// ---- Anthropic Messages ------------------------------------------------------------------------

TEST(Anthropic, TheInstructionIsATopLevelFieldAndMaxTokensIsAlwaysThere) {
  const llm::HttpRequest request = llm::anthropic::BuildRequest(
      ProviderFor(llm::Protocol::kAnthropic, "https://api.anthropic.com/"), Question(), Limit(256));
  EXPECT_EQ(request.url, "https://api.anthropic.com/v1/messages");
  EXPECT_EQ(HeaderValue(request, "x-api-key"), "sk-secret");
  EXPECT_EQ(HeaderValue(request, "anthropic-version"), "2023-06-01");
  EXPECT_EQ(HeaderValue(request, "Authorization"), "<missing>");

  const Json body = json::Parse(request.body);
  EXPECT_EQ(body["model"], "some-model");
  EXPECT_EQ(body["system"], "Be brief.");
  EXPECT_EQ(body["max_tokens"], 256);
  ASSERT_EQ(body["messages"].size(), 1u);
  EXPECT_EQ(body["messages"][0],
            (Json{{"role", "user"}, {"content", json::Array({{{"type", "text"}, {"text", "Say hello."}}})}}));
  EXPECT_FALSE(body.contains("thinking"));
}

TEST(Anthropic, WithoutAnInstructionThereIsNoSystemField) {
  const Json body = json::Parse(
      llm::anthropic::BuildRequest(ProviderFor(llm::Protocol::kAnthropic, "https://api.anthropic.com"), Question(""), Limit(256)).body);
  EXPECT_FALSE(body.contains("system"));
}

TEST(Anthropic, ASignedThinkingBlockIsSentBackAsItIsBeforeTheText) {
  const llm::Conversation conversation =
      TwoTurns({Reasoning(llm::Protocol::kAnthropic, kSignedThinking), Text("answer")});
  const Json body = json::Parse(
      llm::anthropic::BuildRequest(ProviderFor(llm::Protocol::kAnthropic, "https://a.test"), conversation, Limit(256)).body);
  ASSERT_EQ(body["messages"].size(), 3u);
  EXPECT_EQ(body["messages"][1]["role"], "assistant");
  ASSERT_EQ(body["messages"][1]["content"].size(), 2u);
  EXPECT_EQ(body["messages"][1]["content"][0], kSignedThinking);
  EXPECT_EQ(body["messages"][1]["content"][1], (Json{{"type", "text"}, {"text", "answer"}}));
}

TEST(Anthropic, ARedactedThinkingBlockIsSentBackToo) {
  const llm::Conversation conversation =
      TwoTurns({Reasoning(llm::Protocol::kAnthropic, kRedactedThinking), Text("answer")});
  const Json body = json::Parse(
      llm::anthropic::BuildRequest(ProviderFor(llm::Protocol::kAnthropic, "https://a.test"), conversation, Limit(256)).body);
  EXPECT_EQ(body["messages"][1]["content"][0], kRedactedThinking);
}

TEST(Anthropic, ABlockFromAnotherProtocolIsLeftOutAndAMessageWithNothingToSendIsToo) {
  const llm::Provider provider = ProviderFor(llm::Protocol::kAnthropic, "https://a.test");
  const Json with_text = json::Parse(llm::anthropic::BuildRequest(
      provider, TwoTurns({Reasoning(llm::Protocol::kOpenAIResponses, kReasoningItem), Text("answer")}), Limit(256)).body);
  ASSERT_EQ(with_text["messages"][1]["content"].size(), 1u);
  EXPECT_EQ(with_text["messages"][1]["content"][0], (Json{{"type", "text"}, {"text", "answer"}}));

  const Json only_reasoning = json::Parse(llm::anthropic::BuildRequest(
      provider, TwoTurns({Reasoning(llm::Protocol::kOpenAIResponses, kReasoningItem)}), Limit(256)).body);
  EXPECT_EQ(only_reasoning["messages"].size(), 2u);
}

TEST(Anthropic, ReasoningIsAskedForWithAdaptiveThinkingAndAnEffort) {
  const llm::Provider provider = ProviderFor(llm::Protocol::kAnthropic, "https://a.test");
  const Json body = json::Parse(llm::anthropic::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kLow)).body);
  EXPECT_EQ(body["thinking"], (Json{{"type", "adaptive"}, {"display", "summarized"}}));
  EXPECT_EQ(body["output_config"]["effort"], "low");
  EXPECT_EQ(body["max_tokens"], 256);
  EXPECT_EQ(json::Parse(llm::anthropic::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kMedium)).body)["output_config"]["effort"], "medium");
  EXPECT_EQ(json::Parse(llm::anthropic::BuildRequest(provider, Question(), Limit(256, llm::Reasoning::kHigh)).body)["output_config"]["effort"], "high");
}

TEST(Anthropic, TheTextIsInTheTextBlocksAndThinkingBlocksAreKept) {
  const std::string answer = R"({
    "id": "msg_1", "type": "message", "role": "assistant", "model": "some-model",
    "content": [{"type": "thinking", "thinking": "Plan: greet.", "signature": "SIG=="},
                {"type": "redacted_thinking", "data": "ENCRYPTED=="},
                {"type": "server_tool_use", "id": "x"},
                {"type": "text", "text": "Hello"}, {"type": "text", "text": "!"}],
    "stop_reason": "end_turn", "stop_sequence": null,
    "usage": {"input_tokens": 9, "output_tokens": 2}})";
  const llm::Reply reply = llm::anthropic::ParseReply(answer);
  EXPECT_EQ(reply.Text(), "Hello!");
  EXPECT_EQ(reply.stop_reason, "end_turn");
  EXPECT_EQ(reply.input_tokens, 9);
  EXPECT_EQ(reply.output_tokens, 2);
  ASSERT_EQ(reply.message.blocks.size(), 4u);  // the unknown block type is left out
  const auto* thinking = std::get_if<llm::ReasoningBlock>(&reply.message.blocks[0]);
  ASSERT_NE(thinking, nullptr);
  EXPECT_EQ(thinking->origin, llm::Protocol::kAnthropic);
  EXPECT_EQ(thinking->payload, kSignedThinking);
  EXPECT_EQ(thinking->visible_text, "Plan: greet.");
  const auto* redacted = std::get_if<llm::ReasoningBlock>(&reply.message.blocks[1]);
  ASSERT_NE(redacted, nullptr);
  EXPECT_EQ(redacted->payload, kRedactedThinking);
  EXPECT_EQ(redacted->visible_text, "");
}

TEST(Anthropic, AnAnswerWithOnlyThinkingHasNoText) {
  const llm::Reply reply = llm::anthropic::ParseReply(
      R"({"content": [{"type": "thinking", "thinking": "...", "signature": "S"}], "stop_reason": "max_tokens"})");
  EXPECT_EQ(reply.Text(), "");
  EXPECT_EQ(reply.stop_reason, "max_tokens");
  EXPECT_EQ(reply.message.blocks.size(), 1u);
}

TEST(Anthropic, AnAnswerWithoutContentIsAnError) {
  EXPECT_THROW(llm::anthropic::ParseReply(R"({"type": "message"})"), llm::Error);
}

TEST(Anthropic, TheErrorTextHasTypeAndMessage) {
  const std::string error = R"({"type": "error",
                                "error": {"type": "authentication_error", "message": "invalid x-api-key"},
                                "request_id": "req_1"})";
  EXPECT_EQ(llm::anthropic::ErrorText(error), "authentication_error: invalid x-api-key");
}

}  // namespace
