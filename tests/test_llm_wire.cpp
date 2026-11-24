#include "llm_wire.h"

#include <gtest/gtest.h>

#include "json.h"

namespace {

using Json = json::Value;

std::string HeaderValue(const llm::HttpRequest& request, const std::string& name) {
  for (const auto& header : request.headers) {
    if (header.first == name) return header.second;
  }
  return "<missing>";
}

llm::Request Question(const std::string& system = "Be brief.") {
  llm::Request request;
  request.model = "some-model";
  request.system = system;
  request.prompt = "Say hello.";
  request.max_tokens = 256;
  return request;
}

const llm::Endpoint kEndpoint{"https://example.test/v1/", "sk-secret"};

// ---- OpenAI Chat Completions -------------------------------------------------------------------

TEST(OpenAiChat, TheRequestHasTheInstructionAsAMessageOfItsOwn) {
  const llm::HttpRequest request = llm::openai_chat::BuildRequest(kEndpoint, Question());
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
  const Json body = json::Parse(llm::openai_chat::BuildRequest(kEndpoint, Question("")).body);
  ASSERT_EQ(body["messages"].size(), 1u);
  EXPECT_EQ(body["messages"][0]["role"], "user");
}

TEST(OpenAiChat, TheTextIsInTheFirstChoice) {
  const std::string answer = R"({
    "id": "chatcmpl-1", "object": "chat.completion", "created": 1, "model": "some-model",
    "choices": [{"index": 0, "message": {"role": "assistant", "content": "Hello!", "refusal": null},
                 "finish_reason": "stop"}],
    "usage": {"prompt_tokens": 9, "completion_tokens": 2, "total_tokens": 11}})";
  const llm::Reply reply = llm::openai_chat::ParseReply(answer);
  EXPECT_EQ(reply.text, "Hello!");
  EXPECT_EQ(reply.stop_reason, "stop");
  EXPECT_EQ(reply.input_tokens, 9);
  EXPECT_EQ(reply.output_tokens, 2);
}

TEST(OpenAiChat, NullContentGivesAnEmptyTextAndKeepsTheReasonForStopping) {
  const std::string answer = R"({"choices": [{"message": {"role": "assistant", "content": null},
                                              "finish_reason": "length"}]})";
  const llm::Reply reply = llm::openai_chat::ParseReply(answer);
  EXPECT_EQ(reply.text, "");
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

TEST(OpenAiResponses, TheInstructionIsAFieldOfItsOwnAndTheInputIsAString) {
  const llm::HttpRequest request = llm::openai_responses::BuildRequest(kEndpoint, Question());
  EXPECT_EQ(request.url, "https://example.test/v1/responses");
  EXPECT_EQ(HeaderValue(request, "Authorization"), "Bearer sk-secret");

  const Json body = json::Parse(request.body);
  EXPECT_EQ(body["model"], "some-model");
  EXPECT_EQ(body["instructions"], "Be brief.");
  EXPECT_EQ(body["input"], "Say hello.");
  EXPECT_EQ(body["max_output_tokens"], 256);
}

TEST(OpenAiResponses, FewerThanSixteenOutputTokensAreRaisedToSixteen) {
  llm::Request question = Question("");
  question.max_tokens = 1;
  const Json body = json::Parse(llm::openai_responses::BuildRequest(kEndpoint, question).body);
  EXPECT_EQ(body["max_output_tokens"], 16);
  EXPECT_FALSE(body.contains("instructions"));
}

TEST(OpenAiResponses, TheTextIsInTheMessageItemAfterTheReasoningItem) {
  const std::string answer = R"({
    "id": "resp_1", "object": "response", "status": "completed", "model": "some-model",
    "output": [
      {"type": "reasoning", "id": "rs_1", "summary": []},
      {"type": "message", "id": "msg_1", "role": "assistant",
       "content": [{"type": "output_text", "text": "Hello", "annotations": []},
                   {"type": "output_text", "text": "!", "annotations": []}]}],
    "usage": {"input_tokens": 9, "output_tokens": 5, "total_tokens": 14}})";
  const llm::Reply reply = llm::openai_responses::ParseReply(answer);
  EXPECT_EQ(reply.text, "Hello!");
  EXPECT_EQ(reply.stop_reason, "completed");
  EXPECT_EQ(reply.input_tokens, 9);
  EXPECT_EQ(reply.output_tokens, 5);
}

TEST(OpenAiResponses, AnIncompleteAnswerGivesTheReasonFromIncompleteDetails) {
  const std::string answer = R"({"status": "incomplete",
                                 "incomplete_details": {"reason": "max_output_tokens"}, "output": []})";
  const llm::Reply reply = llm::openai_responses::ParseReply(answer);
  EXPECT_EQ(reply.text, "");
  EXPECT_EQ(reply.stop_reason, "max_output_tokens");
}

TEST(OpenAiResponses, AnAnswerWithoutOutputIsAnError) {
  EXPECT_THROW(llm::openai_responses::ParseReply(R"({"status": "completed"})"), llm::Error);
}

// ---- Anthropic Messages ------------------------------------------------------------------------

TEST(Anthropic, TheInstructionIsATopLevelFieldAndMaxTokensIsAlwaysThere) {
  const llm::HttpRequest request =
      llm::anthropic::BuildRequest({"https://api.anthropic.com/", "sk-secret"}, Question());
  EXPECT_EQ(request.url, "https://api.anthropic.com/v1/messages");
  EXPECT_EQ(HeaderValue(request, "x-api-key"), "sk-secret");
  EXPECT_EQ(HeaderValue(request, "anthropic-version"), "2023-06-01");
  EXPECT_EQ(HeaderValue(request, "Authorization"), "<missing>");

  const Json body = json::Parse(request.body);
  EXPECT_EQ(body["model"], "some-model");
  EXPECT_EQ(body["system"], "Be brief.");
  EXPECT_EQ(body["max_tokens"], 256);
  ASSERT_EQ(body["messages"].size(), 1u);
  EXPECT_EQ(body["messages"][0], (Json{{"role", "user"}, {"content", "Say hello."}}));
}

TEST(Anthropic, WithoutAnInstructionThereIsNoSystemField) {
  const Json body =
      json::Parse(llm::anthropic::BuildRequest({"https://api.anthropic.com", "k"}, Question("")).body);
  EXPECT_FALSE(body.contains("system"));
}

TEST(Anthropic, TheTextIsInTheTextBlocksOfContent) {
  const std::string answer = R"({
    "id": "msg_1", "type": "message", "role": "assistant", "model": "some-model",
    "content": [{"type": "thinking", "thinking": "..."},
                {"type": "text", "text": "Hello"}, {"type": "text", "text": "!"}],
    "stop_reason": "end_turn", "stop_sequence": null,
    "usage": {"input_tokens": 9, "output_tokens": 2}})";
  const llm::Reply reply = llm::anthropic::ParseReply(answer);
  EXPECT_EQ(reply.text, "Hello!");
  EXPECT_EQ(reply.stop_reason, "end_turn");
  EXPECT_EQ(reply.input_tokens, 9);
  EXPECT_EQ(reply.output_tokens, 2);
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
