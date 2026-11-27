#include "chat.h"

#include <gtest/gtest.h>

#include <chrono>
#include <csignal>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "json.h"

#include "scripted_transport.h"
#include "toy_runtime.h"

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
         const std::string& input, const llm::Options& options = {}, const std::string& system = "",
         agent::ToolSet* tools = nullptr, const agent::TurnLimits& limits = {}) {
  ScriptedTransport transport(std::move(script));
  std::istringstream in(input);
  std::ostringstream out;
  std::ostringstream err;
  llm::RunChat(transport, providers, start, options, system, tools, limits, in, out, err);
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
  for (const Json& item : json::Items(to_responses["input"])) EXPECT_NE(json::StringOr(item, "type"), "thinking");
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

// ---- Tools --------------------------------------------------------------------------------------

namespace {

llm::Reply Asks(std::vector<llm::Block> blocks) {
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  reply.message.blocks = std::move(blocks);
  reply.stop_reason = "tool_use";
  return reply;
}

llm::Block CallAdd(const std::string& id, Json arguments) { return llm::ToolCallBlock{id, "add", std::move(arguments), "", ""}; }
llm::Block CallSleep(const std::string& id, int ms) { return llm::ToolCallBlock{id, "sleep", Json{{"ms", ms}}, "", ""}; }

}  // namespace

TEST(Chat, ToolCallsArePrintedAndTheAnswerFollows) {
  agent::ToolRuntime tools = ToyRuntime();
  const Transcript run = Chat({Ok(Asks({CallAdd("t1", Json{{"a", 2}, {"b", 3}})})), Ok(Answer("The sum is 5."))}, {Anthropic()},
                              "anthropic", "what is 2 plus 3\n", {}, "", &tools);
  const std::size_t call = run.out.find("[tool] add {\"a\":2,\"b\":3}\n");
  const std::size_t result = run.out.find("[tool] add -> 5\n");
  const std::size_t answer = run.out.find("The sum is 5.\n");
  ASSERT_NE(call, std::string::npos) << run.out;
  ASSERT_NE(result, std::string::npos) << run.out;
  ASSERT_NE(answer, std::string::npos) << run.out;
  EXPECT_LT(call, result);
  EXPECT_LT(result, answer);
  ASSERT_EQ(run.seen.size(), 2u);
  EXPECT_EQ(json::Parse(run.seen[0].body)["tools"].size(), 4u);  // the four toy tools are offered
}

TEST(Chat, AToolThatFailsIsPrintedAsAnErrorAndTheModelStillAnswers) {
  agent::ToolRuntime tools = ToyRuntime();
  const Transcript run = Chat({Ok(Asks({CallAdd("t1", Json{{"a", 1}})})), Ok(Answer("I need both numbers."))}, {Anthropic()}, "anthropic",
                              "add\n", {}, "", &tools);
  EXPECT_TRUE(Contains(run.out, "[tool] add -> error: "));
  EXPECT_TRUE(Contains(run.out, "I need both numbers.\n"));
}

TEST(Chat, WithoutToolsNothingIsOfferedAndNothingIsPrintedAboutTools) {
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "hello\n");
  EXPECT_FALSE(json::Parse(run.seen.at(0).body).contains("tools"));
  EXPECT_FALSE(Contains(run.out, "[tool]"));
}

TEST(Chat, ATurnThatRunsOutOfTimeIsReportedAndForgotten) {
  agent::ToolRuntime tools = ToyRuntime();
  agent::TurnLimits limits;
  limits.deadline = std::chrono::milliseconds(100);
  const Transcript run = Chat({Ok(Asks({CallSleep("s1", 5000)})), Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "sleep\nhello\n", {}, "",
                              &tools, limits);
  EXPECT_TRUE(Contains(run.err, "stopped: timed out after 0.1s")) << run.err;
  ASSERT_EQ(run.seen.size(), 2u);
  const Json second = json::Parse(run.seen[1].body);
  ASSERT_EQ(second["messages"].size(), 1u);  // nothing of the first Turn is left
  EXPECT_EQ(second["messages"][0]["content"][0]["text"], "hello");
  EXPECT_TRUE(Contains(run.out, "Hi\n"));
}

TEST(Chat, ATurnThatKeepsCallingToolsStopsAtTheStepLimit) {
  agent::ToolRuntime tools = ToyRuntime();
  agent::TurnLimits limits;
  limits.max_steps = 2;
  const Transcript run = Chat({Ok(Asks({CallAdd("t1", Json{{"a", 1}, {"b", 1}})})), Ok(Asks({CallAdd("t2", Json{{"a", 1}, {"b", 1}})}))},
                              {Anthropic()}, "anthropic", "again\n", {}, "", &tools, limits);
  EXPECT_TRUE(Contains(run.err, "stopped: step limit (2)")) << run.err;
}

TEST(Chat, CtrlCStopsTheRunningTurnAndTheChatGoesOn) {
  agent::ToolRuntime tools = ToyRuntime();
  agent::TurnLimits real_limits;  // the real program has a deadline too (the default is five minutes)
  real_limits.deadline = std::chrono::minutes(5);
  std::thread presser([] {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));  // the Turn is inside the sleep tool by now
    std::raise(SIGINT);
  });
  const auto start = std::chrono::steady_clock::now();
  const Transcript run = Chat({Ok(Asks({CallSleep("s1", 5000)})), Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "sleep\nhello\n/exit\n",
                              {}, "", &tools, real_limits);
  presser.join();
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(3));
  EXPECT_TRUE(Contains(run.err, "stopped: cancelled")) << run.err;
  EXPECT_TRUE(Contains(run.out, "Hi\n"));  // the next line was handled as usual
  ASSERT_EQ(run.seen.size(), 2u);
  EXPECT_EQ(json::Parse(run.seen[1].body)["messages"].size(), 1u);
}

namespace {

// Tools that only count how often the chat told them to start over.
class CountingTools final : public agent::ToolSet {
 public:
  std::vector<llm::ToolDefinition> Definitions() const override { return {}; }
  boost::asio::awaitable<agent::ToolResult> Run(const llm::ToolCallBlock&) override { co_return agent::ToolResult{"", false}; }
  void StartOver() override { ++starts; }
  int starts = 0;
};

class CountingHook final : public agent::AgentHook {
 public:
  boost::asio::awaitable<void> OnTurnStarted(const agent::TurnStarted&) override {
    ++started;
    co_return;
  }
  int started = 0;
};

}  // namespace

TEST(Chat, ResetTellsTheToolsToStartOver) {
  CountingTools tools;
  const Transcript run = Chat({Ok(Answer("Hi"))}, {Anthropic()}, "anthropic", "hello\n/reset\n/exit\n", {}, "", &tools);
  EXPECT_EQ(tools.starts, 1);
  EXPECT_TRUE(Contains(run.out, "history cleared"));
}

TEST(Chat, TheExtraHooksAreToldAboutEveryTurn) {
  const auto hook = std::make_shared<CountingHook>();
  ScriptedTransport transport({Ok(Answer("Hi")), Ok(Answer("Bye"))});
  std::istringstream in("hello\nagain\n");
  std::ostringstream out;
  std::ostringstream err;
  llm::RunChat(transport, {Anthropic()}, "anthropic", {}, "", nullptr, {}, in, out, err, {hook});
  EXPECT_EQ(hook->started, 2);
}

namespace {

// RunChat with a history and context settings of the test's own; what the service was sent comes back.
Transcript ChatWithContext(std::vector<http::Response> script, const std::string& input, const llm::ContextSettings& context,
                           const std::string& system = "") {
  ScriptedTransport transport(std::move(script));
  std::istringstream in(input);
  std::ostringstream out;
  std::ostringstream err;
  llm::RunChat(transport, {Anthropic()}, "anthropic", {}, system, nullptr, {}, in, out, err, {}, context);
  return {out.str(), err.str(), transport.seen()};
}

std::size_t Bytes(const std::string& text) { return text.size(); }

llm::Message UserSays(const std::string& text) { return {llm::Role::kUser, {llm::TextBlock{text}}}; }
llm::Message AssistantSays(const std::string& text) { return {llm::Role::kAssistant, {llm::TextBlock{text}}}; }

}  // namespace

TEST(ChatContext, OnlyATurnThatEndedWithAnAnswerGoesIntoTheHistory) {
  llm::ContextSettings context;
  context.history = std::make_shared<agent::MemoryHistory>();
  ChatWithContext({Ok(Answer("Hi")), {500, "oops"}, Ok(Answer("Bye"))}, "hello\nagain\nonce more\n", context);
  const auto stored = context.history->Load("cli");
  ASSERT_EQ(stored.size(), 4u);  // hello, Hi, once more, Bye: the turn that failed left nothing
  EXPECT_EQ(std::get<llm::TextBlock>(stored[2].blocks[0]).text, "once more");
}

TEST(ChatContext, EveryTurnStartsFromTheHistoryThatIsStored) {
  llm::ContextSettings context;
  context.history = std::make_shared<agent::MemoryHistory>();
  context.history->Append("cli", {UserSays("earlier question"), AssistantSays("earlier answer")});
  const Transcript run = ChatWithContext({Ok(Answer("Hi"))}, "hello\n", context);
  const Json body = json::Parse(run.seen.at(0).body);
  ASSERT_EQ(body["messages"].size(), 3u);
  EXPECT_EQ(body["messages"][0]["content"][0]["text"], "earlier question");
}

TEST(ChatContext, ResetEmptiesTheHistoryOfTheChannel) {
  llm::ContextSettings context;
  context.history = std::make_shared<agent::MemoryHistory>();
  context.history->Append("other", {UserSays("not ours")});
  const Transcript run = ChatWithContext({Ok(Answer("Hi")), Ok(Answer("Bye"))}, "hello\n/reset\nagain\n", context);
  EXPECT_EQ(json::Parse(run.seen.at(1).body)["messages"].size(), 1u);
  EXPECT_EQ(context.history->Load("cli").size(), 2u);
  EXPECT_EQ(context.history->Load("other").size(), 1u);
}

TEST(ChatContext, TheSystemSectionsAndTheStateAreInTheRequest) {
  llm::ContextSettings context;
  context.system_extra = {{"rules", "Be careful."}, {"skills", "- calc: sums"}};
  context.state = [] { return std::vector<agent::Section>{{"session state", "loaded skills: none"}}; };
  const Transcript run = ChatWithContext({Ok(Answer("Hi"))}, "hello\n", context, "Be brief.");
  const Json body = json::Parse(run.seen.at(0).body);
  EXPECT_EQ(body["system"], "Be brief.\n\n[rules]\nBe careful.\n\n[skills]\n- calc: sums");
  EXPECT_EQ(body["messages"][0]["content"][0]["text"], "[session state]\nloaded skills: none");
  EXPECT_EQ(body["messages"][0]["content"][1]["text"], "hello");
}

TEST(ChatContext, TheBudgetLeavesTheOldestTurnsOutOfTheRequestButNotOutOfTheHistory) {
  llm::ContextSettings context;
  context.history = std::make_shared<agent::MemoryHistory>();
  context.history->Append("cli", {UserSays(std::string(300, 'a')), AssistantSays(std::string(300, 'b')), UserSays(std::string(300, 'c')),
                                  AssistantSays(std::string(300, 'd'))});
  context.budget_tokens = 800;
  context.count = Bytes;
  const Transcript run = ChatWithContext({Ok(Answer("Hi"))}, "hello\n", context);
  EXPECT_EQ(json::Parse(run.seen.at(0).body)["messages"].size(), 3u);  // the second old turn, and the question
  EXPECT_EQ(context.history->Load("cli").size(), 6u);                  // nothing was forgotten
}

TEST(ChatContext, TheBudgetCanComeFromTheWindowOfTheProviderWhenNoneIsSet) {
  llm::ContextSettings context;
  context.history = std::make_shared<agent::MemoryHistory>();
  context.history->Append("cli", {UserSays(std::string(300, 'a')), AssistantSays(std::string(300, 'b'))});
  const Transcript run = ChatWithContext({Ok(Answer("Hi"))}, "hello\n", context);
  EXPECT_EQ(json::Parse(run.seen.at(0).body)["messages"].size(), 3u);  // a window of 100000 tokens leaves room for all
}
