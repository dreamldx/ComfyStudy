#include "chat.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "json.h"

#include "scripted_transport.h"

namespace {

using Json = json::Value;

llm::Provider ProviderOf(llm::Protocol protocol, const std::string& name) {
  return llm::Provider{name, protocol, llm::Endpoint{"https://example.test", "key"}, "m", {true, 1000, 100000}};
}

llm::Provider Anthropic() { return ProviderOf(llm::Protocol::kAnthropic, "anthropic"); }
llm::Provider Responses() { return ProviderOf(llm::Protocol::kOpenAIResponses, "openai-responses"); }

const Json kThinking = {{"type", "thinking"}, {"thinking", "Plan"}, {"signature", "SIG=="}};

llm::Reply Answer(const std::string& text, bool with_thinking = false, const std::string& stop = "end_turn") {
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  if (with_thinking) reply.message.blocks.push_back(llm::ReasoningBlock{llm::Protocol::kAnthropic, kThinking, "Plan"});
  if (!text.empty()) reply.message.blocks.push_back(llm::TextBlock{text});
  reply.stop_reason = stop;
  return reply;
}

http::Response Ok(const llm::Reply& reply) { return {200, AnthropicBody(reply)}; }

struct Transcript {
  std::string out;
  std::string err;
  std::vector<http::PostRequest> seen;
};

Transcript Chat(std::vector<http::Response> script, const std::vector<llm::Provider>& providers, const std::string& start,
         const std::string& input, const llm::Options& options = {}, const std::string& system = "") {
  ScriptedTransport transport(std::move(script));
  std::istringstream in(input);
  std::ostringstream out;
  std::ostringstream err;
  llm::RunChat(transport, providers, start, options, system, in, out, err);
  return {out.str(), err.str(), transport.seen()};
}

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

TEST(Chat, ATwoTurnChatSendsTheWholeHistoryTheSecondTime) {
  const Transcript run = Chat({Ok(Answer("Hi")), Ok(Answer("Bye"))}, {Anthropic()}, "anthropic", "hello\nagain\n");
  EXPECT_TRUE(Contains(run.out, "Hi\n"));
  EXPECT_TRUE(Contains(run.out, "Bye\n"));
  ASSERT_EQ(run.seen.size(), 2u);
  const Json second = json::Parse(run.seen[1].body);
  ASSERT_EQ(second["messages"].size(), 3u);
  EXPECT_EQ(second["messages"][0]["content"][0]["text"], "hello");
  EXPECT_EQ(second["messages"][1]["content"][0]["text"], "Hi");
  EXPECT_EQ(second["messages"][2]["content"][0]["text"], "again");
}

TEST(Chat, BlankSpaceAroundAnAnswerIsNotPrintedButStaysInTheHistory) {
  const std::string padded = "\n\n  Hi there \n";
  const Transcript run = Chat({Ok(Answer(padded)), Ok(Answer("Bye"))}, {Anthropic()}, "anthropic", "hello\nagain\n");
  EXPECT_TRUE(Contains(run.out, "> Hi there\n"));  // the line after the prompt starts with the answer
  ASSERT_EQ(run.seen.size(), 2u);
  EXPECT_EQ(json::Parse(run.seen[1].body)["messages"][1]["content"][0]["text"], padded);
}

TEST(Chat, TheInstructionAndTheOptionsGoWithEveryRequest) {
  llm::Options options;
  options.max_tokens = 77;
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "hello\n", options, "Be brief.");
  const Json body = json::Parse(run.seen.at(0).body);
  EXPECT_EQ(body["system"], "Be brief.");
  EXPECT_EQ(body["max_tokens"], 77);
}

TEST(Chat, UseSwitchesTheProviderAndReportsTheReasoningBlocksThatWillNotBeSent) {
  const Transcript run = Chat({Ok(Answer("Hi", true)), {200, ResponsesBody(Answer("Bye"))}, Ok(Answer("Again"))},
                       {Anthropic(), Responses()}, "anthropic",
                       "hello\n/use openai-responses\nsecond\n/use anthropic\nthird\n");
  EXPECT_TRUE(Contains(run.out, "now using openai-responses"));
  EXPECT_TRUE(Contains(run.out, "1 reasoning block(s) in the history come from another protocol and will not be sent"));
  EXPECT_TRUE(Contains(run.out, "now using anthropic"));

  ASSERT_EQ(run.seen.size(), 3u);
  EXPECT_EQ(run.seen[1].url, "https://example.test/responses");
  const Json to_responses = json::Parse(run.seen[1].body);
  for (const json::Value& item : json::Items(to_responses["input"])) EXPECT_NE(json::StringOr(item, "type"), "thinking");
  EXPECT_EQ(to_responses["input"].size(), 3u);

  // back on the first provider the block is sent again, as it was
  const Json back = json::Parse(run.seen[2].body);
  EXPECT_EQ(back["messages"][1]["content"][0], kThinking);
}

TEST(Chat, UseWithoutAKnownNameKeepsTheCurrentProviderAndListsTheNames) {
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic(), Responses()}, "anthropic", "/use\n/use nope\nhello\n");
  EXPECT_TRUE(Contains(run.err, "usage: /use <name>"));
  EXPECT_TRUE(Contains(run.err, "provider \"nope\" is not available"));
  EXPECT_TRUE(Contains(run.err, "anthropic"));
  EXPECT_TRUE(Contains(run.err, "openai-responses"));
  ASSERT_EQ(run.seen.size(), 1u);
  EXPECT_EQ(run.seen[0].url, "https://example.test/v1/messages");
}

TEST(Chat, ResetClearsTheHistory) {
  const Transcript run = Chat({Ok(Answer("Hi")), Ok(Answer("Bye"))}, {Anthropic()}, "anthropic", "hello\n/reset\nagain\n");
  EXPECT_TRUE(Contains(run.out, "history cleared"));
  ASSERT_EQ(run.seen.size(), 2u);
  EXPECT_EQ(json::Parse(run.seen[1].body)["messages"].size(), 1u);
}

TEST(Chat, BlankLinesAreIgnoredAndTheEndOfTheInputEndsTheChat) {
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "\n   \nhello");  // no newline at the end
  ASSERT_EQ(run.seen.size(), 1u);
  EXPECT_EQ(json::Parse(run.seen[0].body)["messages"][0]["content"][0]["text"], "hello");
}

TEST(Chat, ExitEndsTheChatRightAway) {
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "hello\n/exit\nignored\n");
  EXPECT_EQ(run.seen.size(), 1u);
}

TEST(Chat, HelpListsTheCommands) {
  const Transcript run = Chat({}, {Anthropic()}, "anthropic", "/help\n");
  for (const char* command : {"/use", "/reset", "/exit", "/help"}) EXPECT_TRUE(Contains(run.out, command)) << command;
}

TEST(Chat, UnknownCommandsAreNotSentToTheModel) {
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "/foo\nhello\n");
  EXPECT_TRUE(Contains(run.err, "unknown command \"/foo\""));
  EXPECT_EQ(run.seen.size(), 1u);
}

TEST(Chat, AFailedRequestRemovesTheUserMessageSoTheRolesStillAlternate) {
  const Transcript run = Chat({{500, R"({"error": {"type": "api_error", "message": "boom"}})"}, Ok(Answer("Hi"))}, {Anthropic()},
                       "anthropic", "first\nsecond\n");
  EXPECT_TRUE(Contains(run.err, "error: "));
  EXPECT_TRUE(Contains(run.err, "boom"));
  ASSERT_EQ(run.seen.size(), 2u);
  const Json second = json::Parse(run.seen[1].body);
  ASSERT_EQ(second["messages"].size(), 1u);
  EXPECT_EQ(second["messages"][0]["content"][0]["text"], "second");
}

TEST(Chat, AnAnswerWithoutTextIsReportedAndNotKept) {
  const Transcript run = Chat({Ok(Answer("", true, "max_tokens")), Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "q1\nq2\n");
  EXPECT_TRUE(Contains(run.err, "the model returned no text (stopped because: max_tokens)"));
  ASSERT_EQ(run.seen.size(), 2u);
  const Json second = json::Parse(run.seen[1].body);
  ASSERT_EQ(second["messages"].size(), 1u);  // neither q1 nor the thinking-only answer is in the history
  EXPECT_EQ(second["messages"][0]["content"][0]["text"], "q2");
}

TEST(Chat, InputThatIsNotValidUtf8IsReportedAndNeverSent) {
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "caf\xE9\nhello\n");
  EXPECT_TRUE(Contains(run.err, "UTF-8"));
  ASSERT_EQ(run.seen.size(), 1u);
  EXPECT_EQ(json::Parse(run.seen[0].body)["messages"][0]["content"][0]["text"], "hello");
}

TEST(Chat, StartingWithAProviderThatIsNotAvailableIsAnError) {
  EXPECT_THROW(Chat({}, {Anthropic()}, "openai-chat", ""), llm::Error);
  EXPECT_THROW(Chat({}, {}, "anthropic", ""), llm::Error);
}
