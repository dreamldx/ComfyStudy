#include "agent.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "json.h"

#include "fake_transport.h"
#include "scripted_transport.h"
#include "toy_runtime.h"

namespace {

using Json = json::Value;
using namespace std::chrono_literals;
using Reason = agent::TurnOutcome::Reason;

const Json kThinking = {{"type", "thinking"}, {"thinking", "Plan"}, {"signature", "SIG=="}};

llm::Provider Anthropic(const std::string& base = "https://a.test") {
  return llm::Provider{"anthropic", llm::Protocol::kAnthropic, llm::Endpoint{base, "key"}, "m", {true, 1000, 100000}};
}

llm::Reply Said(const std::string& text) {
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  reply.message.blocks.push_back(llm::TextBlock{text});
  reply.stop_reason = "end_turn";
  reply.input_tokens = 10;
  reply.output_tokens = 5;
  return reply;
}

llm::Reply Asks(std::vector<llm::Block> blocks) {
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  reply.message.blocks = std::move(blocks);
  reply.stop_reason = "tool_use";
  reply.input_tokens = 10;
  reply.output_tokens = 5;
  return reply;
}

llm::Block CallAdd(const std::string& id, int a, int b) { return llm::ToolCallBlock{id, "add", Json{{"a", a}, {"b", b}}, "", ""}; }
llm::Block CallSleep(const std::string& id, int ms) { return llm::ToolCallBlock{id, "sleep", Json{{"ms", ms}}, "", ""}; }

http::Response Ok(const llm::Reply& reply) { return {200, AnthropicBody(reply)}; }

// Writes down what the hooks are told. Several can share one log, to see in what order they are called.
class RecordingHook final : public agent::AgentHook {
 public:
  RecordingHook(std::shared_ptr<std::vector<std::string>> log, std::string name = "") : log_(std::move(log)), name_(std::move(name)) {}

  boost::asio::awaitable<void> OnTurnStarted(const agent::TurnStarted&) override { Add("TurnStarted"); co_return; }
  boost::asio::awaitable<void> OnStepStarted(const agent::StepStarted& e) override { Add("StepStarted(" + std::to_string(e.step) + ")"); co_return; }
  boost::asio::awaitable<void> OnContextBuilt(const agent::ContextBuilt& e) override { Add("ContextBuilt(" + std::to_string(e.step) + ")"); co_return; }
  boost::asio::awaitable<void> OnReplyReceived(const agent::ReplyReceived& e) override {
    Add("ReplyReceived(" + std::to_string(e.step) + ")");
    co_return;
  }
  boost::asio::awaitable<void> OnToolStarted(const agent::ToolStarted& e) override { Add("ToolStarted(" + e.call.name + ")"); co_return; }
  boost::asio::awaitable<void> OnToolFinished(const agent::ToolFinished& e) override {
    Add("ToolFinished(" + e.call.name + (e.result.is_error ? " error: " : "=") + e.result.text + ")");
    co_return;
  }
  boost::asio::awaitable<void> OnStepFinished(const agent::StepFinished& e) override { Add("StepFinished(" + std::to_string(e.step) + ")"); co_return; }
  boost::asio::awaitable<void> OnTurnFinished(const agent::TurnFinished& e) override {
    Add(std::string("TurnFinished(") + Name(e.outcome.reason) + ")");
    co_return;
  }
  boost::asio::awaitable<void> OnError(const agent::ErrorInfo& e) override { Add("Error(" + e.what + ")"); co_return; }

  static const char* Name(Reason reason) {
    switch (reason) {
      case Reason::kDone: return "done";
      case Reason::kMaxSteps: return "max_steps";
      case Reason::kCancelled: return "cancelled";
      case Reason::kTimedOut: return "timed_out";
      case Reason::kFailed: return "failed";
      case Reason::kNoText: return "no_text";
    }
    return "?";
  }

 private:
  void Add(const std::string& what) { log_->push_back(name_.empty() ? what : name_ + ":" + what); }
  std::shared_ptr<std::vector<std::string>> log_;
  std::string name_;
};

// A hook that is told everything and fails at the start of every Step.
class ThrowingHook final : public agent::AgentHook {
 public:
  explicit ThrowingHook(std::shared_ptr<std::vector<std::string>> log) : log_(std::move(log)) {}
  boost::asio::awaitable<void> OnStepStarted(const agent::StepStarted&) override {
    log_->push_back("thrower:StepStarted");
    throw std::runtime_error("a broken hook");
    co_return;
  }

 private:
  std::shared_ptr<std::vector<std::string>> log_;
};

// What one test needs: a script for the model, the toy tools, a log, and a conversation that ends with
// the user's question.
struct World {
  explicit World(std::vector<http::Response> script, const std::string& question = "add things")
      : transport(std::move(script)), log(std::make_shared<std::vector<std::string>>()) {
    conversation.messages.push_back({llm::Role::kUser, {llm::TextBlock{question}}});
    hooks.push_back(std::make_shared<RecordingHook>(log));
  }
  ScriptedTransport transport;
  agent::ToolRuntime tools = ToyRuntime();
  llm::Conversation conversation;
  std::shared_ptr<std::vector<std::string>> log;
  std::vector<std::shared_ptr<agent::AgentHook>> hooks;
  llm::Provider provider = Anthropic();
  llm::Options options;
};

agent::TurnOutcome Play(World& world, agent::TurnLimits limits = {}, std::function<std::vector<std::string>()> take = nullptr,
                        agent::ToolSet* tools = nullptr, const agent::ContextPlan* plan = nullptr) {
  boost::asio::io_context io;
  auto outcome = boost::asio::co_spawn(io,
                                       agent::RunTurn(world.transport, world.provider, world.conversation, world.options,
                                                      tools != nullptr ? tools : &world.tools, world.hooks, limits, std::move(take), plan),
                                       boost::asio::use_future);
  io.run();
  return outcome.get();
}

// Starts the Turn and calls Cancel() after `after`.
agent::TurnOutcome PlayAndCancel(http::Transport& transport, World& world, std::chrono::milliseconds after,
                                 agent::TurnLimits limits = {}) {
  boost::asio::io_context io;
  agent::TurnControl control;
  auto outcome = boost::asio::co_spawn(
      io,
      agent::RunTurn(transport, world.provider, world.conversation, world.options, &world.tools, world.hooks, limits, nullptr),
      boost::asio::bind_cancellation_slot(control.slot(), boost::asio::use_future));
  boost::asio::steady_timer timer(io, after);
  timer.async_wait([&control](const boost::system::error_code&) { control.Cancel(); });
  io.run();
  return outcome.get();
}

Json RequestBody(const World& world, std::size_t index) { return json::Parse(world.transport.seen().at(index).body); }

bool Has(const std::vector<std::string>& log, const std::string& entry) {
  return std::find(log.begin(), log.end(), entry) != log.end();
}

}  // namespace

TEST(Turn, ATurnWithoutToolsIsOneStep) {
  World world({Ok(Said("Hi"))});
  const agent::TurnOutcome outcome = Play(world);
  EXPECT_EQ(outcome.reason, Reason::kDone);
  EXPECT_EQ(outcome.steps, 1);
  EXPECT_EQ(world.conversation.messages.size(), 2u);
  const std::vector<std::string> expected = {"TurnStarted", "StepStarted(1)", "ReplyReceived(1)", "StepFinished(1)", "TurnFinished(done)"};
  EXPECT_EQ(*world.log, expected);
}

TEST(Turn, AToolCallRunsTheToolAndSendsTheResultBack) {
  World world({Ok(Asks({CallAdd("t1", 2, 3)})), Ok(Said("The sum is 5."))});
  const agent::TurnOutcome outcome = Play(world);
  EXPECT_EQ(outcome.reason, Reason::kDone);
  EXPECT_EQ(outcome.steps, 2);

  const Json second = RequestBody(world, 1);
  ASSERT_EQ(second["messages"].size(), 3u);  // the question, the call, the result
  EXPECT_EQ(second["messages"][1]["content"][0]["type"], "tool_use");
  EXPECT_EQ(second["messages"][2]["content"][0], (Json{{"type", "tool_result"}, {"tool_use_id", "t1"}, {"content", "5"}}));
  EXPECT_TRUE(Has(*world.log, "ToolStarted(add)"));
  EXPECT_TRUE(Has(*world.log, "ToolFinished(add=5)"));
  ASSERT_EQ(world.conversation.messages.size(), 4u);  // question, call, result, answer
  EXPECT_EQ(world.conversation.messages.back().role, llm::Role::kAssistant);
}

TEST(Turn, TheToolsAreOfferedInEveryRequest) {
  World world({Ok(Asks({CallAdd("t1", 2, 3)})), Ok(Said("5"))});
  Play(world);
  for (std::size_t i = 0; i < 2; ++i) {
    const Json body = RequestBody(world, i);
    ASSERT_EQ(body["tools"].size(), 4u) << i;
    EXPECT_EQ(body["tools"][0]["name"], "add");
  }
}

TEST(Turn, SeveralCallsInOneStepAreRunInOrderAndAnsweredInOneMessage) {
  World world({Ok(Asks({CallAdd("t1", 1, 2), CallAdd("t2", 3, 4)})), Ok(Said("done"))});
  Play(world);
  const Json results = RequestBody(world, 1)["messages"][2]["content"];
  ASSERT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0]["tool_use_id"], "t1");
  EXPECT_EQ(results[0]["content"], "3");
  EXPECT_EQ(results[1]["tool_use_id"], "t2");
  EXPECT_EQ(results[1]["content"], "7");
}

TEST(Turn, AnUnknownToolOrBadArgumentsAreErrorResultsAndTheTurnGoesOn) {
  const llm::Block unknown = llm::ToolCallBlock{"t1", "nope", json::Object(), "", ""};
  const llm::Block missing = llm::ToolCallBlock{"t2", "add", Json{{"a", 1}}, "", ""};
  World world({Ok(Asks({unknown, missing})), Ok(Said("sorry"))});
  const agent::TurnOutcome outcome = Play(world);
  EXPECT_EQ(outcome.reason, Reason::kDone);
  const Json results = RequestBody(world, 1)["messages"][2]["content"];
  ASSERT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0]["is_error"], true);
  EXPECT_EQ(results[1]["is_error"], true);
}

TEST(Turn, ReasoningBlocksOfTheCallingMessageGoBackAsTheyWere) {
  World world({Ok(Asks({llm::ReasoningBlock{llm::Protocol::kAnthropic, kThinking, "Plan"}, CallAdd("t1", 2, 3)})), Ok(Said("5"))});
  Play(world);
  const Json assistant = RequestBody(world, 1)["messages"][1]["content"];
  EXPECT_EQ(assistant[0], kThinking);
  EXPECT_EQ(assistant[1]["type"], "tool_use");
}

TEST(Turn, TheStepLimitEndsATurnThatKeepsCalling) {
  World world({Ok(Asks({CallAdd("t1", 1, 1)})), Ok(Asks({CallAdd("t2", 1, 1)})), Ok(Asks({CallAdd("t3", 1, 1)}))});
  agent::TurnLimits limits;
  limits.max_steps = 3;
  const agent::TurnOutcome outcome = Play(world, limits);
  EXPECT_EQ(outcome.reason, Reason::kMaxSteps);
  EXPECT_EQ(outcome.steps, 3);
  EXPECT_EQ(world.transport.seen().size(), 3u);
  EXPECT_TRUE(world.conversation.messages.empty());  // the Turn is forgotten, the user's message with it
  EXPECT_TRUE(Has(*world.log, "TurnFinished(max_steps)"));
}

TEST(Turn, AnAnswerWithoutTextEndsTheTurnAndIsForgotten) {
  llm::Reply thinking_only = Asks({llm::ReasoningBlock{llm::Protocol::kAnthropic, kThinking, ""}});
  thinking_only.stop_reason = "max_tokens";
  World world({Ok(thinking_only)});
  const agent::TurnOutcome outcome = Play(world);
  EXPECT_EQ(outcome.reason, Reason::kNoText);
  EXPECT_TRUE(world.conversation.messages.empty());  // the Turn is forgotten, the user's message with it
}

TEST(Turn, AFailingRequestEndsTheTurnWithTheErrorAndIsForgotten) {
  World world({{500, R"({"error": {"type": "api_error", "message": "boom"}})"}});
  const agent::TurnOutcome outcome = Play(world);
  EXPECT_EQ(outcome.reason, Reason::kFailed);
  EXPECT_NE(outcome.error.find("boom"), std::string::npos);
  EXPECT_TRUE(world.conversation.messages.empty());  // the Turn is forgotten, the user's message with it
  EXPECT_TRUE(Has(*world.log, "TurnFinished(failed)"));
  bool told = false;
  for (const std::string& entry : *world.log) told = told || entry.rfind("Error(", 0) == 0;
  EXPECT_TRUE(told);
}

TEST(Turn, SteeringMessagesGoIntoTheResultMessageAfterTheResults) {
  World world({Ok(Asks({CallAdd("t1", 1, 2)})), Ok(Said("ok"))});
  int asked = 0;
  const auto take = [&asked]() -> std::vector<std::string> {
    ++asked;
    return {"please be brief", "and say it in French"};
  };
  Play(world, {}, take);
  EXPECT_EQ(asked, 1);  // not before the first Step, not after the last
  const Json user = RequestBody(world, 1)["messages"][2];
  ASSERT_EQ(user["content"].size(), 3u);
  EXPECT_EQ(user["content"][0]["type"], "tool_result");
  EXPECT_EQ(user["content"][1]["text"], std::string(agent::kSteeringMarker) + "\nplease be brief");
  EXPECT_EQ(user["content"][2]["text"], std::string(agent::kSteeringMarker) + "\nand say it in French");
}

namespace {
// A set of tools that changes while the Turn runs: two tools for the first request, one for the next.
class ShrinkingTools final : public agent::ToolSet {
 public:
  std::vector<llm::ToolDefinition> Definitions() const override {
    std::vector<llm::ToolDefinition> definitions = {{"add", "d", json::Object()}, {"sleep", "d", json::Object()}};
    if (asked_++ > 0) definitions.pop_back();
    return definitions;
  }
  boost::asio::awaitable<agent::ToolResult> Run(const llm::ToolCallBlock&) override { co_return agent::ToolResult{"7", false}; }

 private:
  mutable int asked_ = 0;
};
}  // namespace

TEST(Turn, TheToolDefinitionsAreAskedAtEveryStep) {
  World world({Ok(Asks({CallAdd("t1", 1, 2)})), Ok(Said("ok"))});
  ShrinkingTools tools;
  Play(world, {}, nullptr, &tools);
  EXPECT_EQ(RequestBody(world, 0)["tools"].size(), 2u);
  EXPECT_EQ(RequestBody(world, 1)["tools"].size(), 1u);
}

TEST(Turn, CancelDuringASleepRollsBackAndStillCallsOnTurnFinished) {
  World world({Ok(Asks({CallSleep("s1", 5000)}))});
  const auto start = std::chrono::steady_clock::now();
  const agent::TurnOutcome outcome = PlayAndCancel(world.transport, world, 100ms);
  EXPECT_LT(std::chrono::steady_clock::now() - start, 3s);
  EXPECT_EQ(outcome.reason, Reason::kCancelled);
  EXPECT_TRUE(world.conversation.messages.empty());  // the Turn is forgotten, the user's message with it
  EXPECT_TRUE(Has(*world.log, "ToolStarted(sleep)"));
  EXPECT_EQ(world.log->back(), "TurnFinished(cancelled)");
}

TEST(Turn, CancelWhileADeadlineIsSetStillCancelsAndIsNotATimeout) {
  // The real program always has a deadline (the default is five minutes): the Steps run beside a timer.
  World world({Ok(Asks({CallSleep("s1", 5000)}))});
  agent::TurnLimits limits;
  limits.deadline = 5s;
  const auto start = std::chrono::steady_clock::now();
  const agent::TurnOutcome outcome = PlayAndCancel(world.transport, world, 100ms, limits);
  EXPECT_LT(std::chrono::steady_clock::now() - start, 3s);
  EXPECT_EQ(outcome.reason, Reason::kCancelled) << outcome.error;
  EXPECT_TRUE(world.conversation.messages.empty());
  EXPECT_EQ(world.log->back(), "TurnFinished(cancelled)");
}

TEST(Turn, ADeadlineStopsATurnAndSaysSo) {
  World world({Ok(Asks({CallSleep("s1", 5000)}))});
  agent::TurnLimits limits;
  limits.deadline = 100ms;
  const auto start = std::chrono::steady_clock::now();
  const agent::TurnOutcome outcome = Play(world, limits);
  EXPECT_LT(std::chrono::steady_clock::now() - start, 3s);
  EXPECT_EQ(outcome.reason, Reason::kTimedOut);
  EXPECT_EQ(outcome.steps, 1);
  EXPECT_TRUE(world.conversation.messages.empty());  // the Turn is forgotten, the user's message with it
  EXPECT_EQ(world.log->back(), "TurnFinished(timed_out)");
}

TEST(Turn, ADeadlineThatIsNotReachedChangesNothing) {
  World world({Ok(Said("Hi"))});
  agent::TurnLimits limits;
  limits.deadline = 5s;
  const auto start = std::chrono::steady_clock::now();
  EXPECT_EQ(Play(world, limits).reason, Reason::kDone);
  EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);  // the timer does not hold the Turn up
}

TEST(Turn, CancelWhileTheModelRequestIsInFlight) {
  World world({});
  world.provider = Anthropic("https://a.test/slow?ms=5000");  // FakeTransport answers this path after 5 seconds
  FakeTransport slow;
  const auto start = std::chrono::steady_clock::now();
  const agent::TurnOutcome outcome = PlayAndCancel(slow, world, 100ms);
  EXPECT_LT(std::chrono::steady_clock::now() - start, 3s);
  EXPECT_EQ(outcome.reason, Reason::kCancelled);
  EXPECT_TRUE(world.conversation.messages.empty());  // the Turn is forgotten, the user's message with it
  EXPECT_EQ(world.log->back(), "TurnFinished(cancelled)");
}

TEST(Turn, AHookThatThrowsDoesNotStopTheTurnAndTheOthersAreCalledInOrder) {
  World world({Ok(Said("Hi"))});
  world.hooks.clear();
  world.hooks.push_back(std::make_shared<RecordingHook>(world.log, "a"));
  world.hooks.push_back(std::make_shared<ThrowingHook>(world.log));
  world.hooks.push_back(std::make_shared<RecordingHook>(world.log, "b"));
  const agent::TurnOutcome outcome = Play(world);
  EXPECT_EQ(outcome.reason, Reason::kDone);
  const std::vector<std::string> expected = {"a:TurnStarted", "b:TurnStarted",   "a:StepStarted(1)",  "thrower:StepStarted", "b:StepStarted(1)",
                                             "a:ReplyReceived(1)", "b:ReplyReceived(1)", "a:StepFinished(1)", "b:StepFinished(1)",
                                             "a:TurnFinished(done)", "b:TurnFinished(done)"};
  EXPECT_EQ(*world.log, expected);
}

TEST(Turn, TheTokensOfAllTheStepsAreAddedUp) {
  World world({Ok(Asks({CallAdd("t1", 1, 2)})), Ok(Said("ok"))});
  const agent::TurnOutcome outcome = Play(world);
  EXPECT_EQ(outcome.input_tokens, 20);
  EXPECT_EQ(outcome.output_tokens, 10);
}

TEST(Limits, TheSettingsGiveTheLimits) {
  const agent::TurnLimits limits = agent::LimitsFromSettings({{"agent_max_steps", "4"}, {"agent_turn_timeout_ms", "1500"}});
  EXPECT_EQ(limits.max_steps, 4);
  EXPECT_EQ(limits.deadline, 1500ms);
}

TEST(Limits, MissingSettingsMeanTheDefaultsAndZeroMeansNoDeadline) {
  const agent::TurnLimits defaults = agent::LimitsFromSettings({});
  EXPECT_EQ(defaults.max_steps, 10);
  EXPECT_EQ(defaults.deadline, 300000ms);
  EXPECT_EQ(agent::LimitsFromSettings({{"agent_turn_timeout_ms", "0"}}).deadline, 0ms);
}

TEST(Limits, ValuesThatAreNotNumbersAreErrors) {
  EXPECT_THROW(agent::LimitsFromSettings({{"agent_max_steps", "0"}}), llm::Error);
  EXPECT_THROW(agent::LimitsFromSettings({{"agent_max_steps", "many"}}), llm::Error);
  EXPECT_THROW(agent::LimitsFromSettings({{"agent_max_steps", "3x"}}), llm::Error);
  EXPECT_THROW(agent::LimitsFromSettings({{"agent_turn_timeout_ms", "-1"}}), llm::Error);
  EXPECT_THROW(agent::LimitsFromSettings({{"agent_turn_timeout_ms", "soon"}}), llm::Error);
}

TEST(Turn, WithoutToolsNoToolsAreOfferedAndACallIsAnErrorResult) {
  World world({Ok(Asks({CallAdd("t1", 1, 2)})), Ok(Said("ok"))});
  world.tools = agent::ToolRuntime();  // no source: nothing offered
  Play(world);
  EXPECT_FALSE(RequestBody(world, 0).contains("tools"));
  EXPECT_EQ(RequestBody(world, 1)["messages"][2]["content"][0]["is_error"], true);
}

// --- Turns that assemble their context (chapter 1.6) ----------------------------------------------

namespace {

std::size_t Bytes(const std::string& text) { return text.size(); }

agent::ContextPlan SimplePlan() {
  agent::ContextPlan plan;
  plan.system = [] { return std::vector<agent::Section>{{"", "Be brief."}, {"rules", "R"}}; };
  plan.state = [] { return std::vector<agent::Section>{{"session state", "channel: cli"}}; };
  return plan;
}

}  // namespace

TEST(TurnContext, TheRequestHasTheSystemSectionsAndTheStateInFrontOfTheUsersMessage) {
  World world({Ok(Said("hi"))}, "add things");
  const agent::ContextPlan plan = SimplePlan();
  EXPECT_EQ(Play(world, {}, nullptr, nullptr, &plan).reason, Reason::kDone);
  const Json body = RequestBody(world, 0);
  EXPECT_EQ(body["system"], "Be brief.\n\n[rules]\nR");
  ASSERT_EQ(body["messages"][0]["content"].size(), 2u);
  EXPECT_EQ(body["messages"][0]["content"][0]["text"], "[session state]\nchannel: cli");
  EXPECT_EQ(body["messages"][0]["content"][1]["text"], "add things");
}

TEST(TurnContext, TheConversationKeepsTheMessageAsTheUserWroteItAndWithoutTheState) {
  World world({Ok(Said("hi"))}, "add things");
  const agent::ContextPlan plan = SimplePlan();
  Play(world, {}, nullptr, nullptr, &plan);
  ASSERT_EQ(world.conversation.messages.size(), 2u);
  EXPECT_EQ(world.conversation.messages[0].blocks.size(), 1u);
}

TEST(TurnContext, TheSectionsAreAskedAtEveryStepSoAChangeShowsInTheNextRequest) {
  World world({Ok(Asks({CallAdd("c1", 1, 2)})), Ok(Said("3"))}, "add things");
  agent::ContextPlan plan = SimplePlan();
  auto asked = std::make_shared<int>(0);
  plan.state = [asked] { return std::vector<agent::Section>{{"session state", "asked " + std::to_string(++*asked)}}; };
  EXPECT_EQ(Play(world, {}, nullptr, nullptr, &plan).reason, Reason::kDone);
  EXPECT_EQ(RequestBody(world, 0)["messages"][0]["content"][0]["text"], "[session state]\nasked 1");
  EXPECT_EQ(RequestBody(world, 1)["messages"][0]["content"][0]["text"], "[session state]\nasked 2");
  EXPECT_EQ(RequestBody(world, 1)["messages"].size(), 3u);  // the question, the call, the result
}

TEST(TurnContext, WithoutSystemSectionsTheSystemPromptOfTheConversationIsUsed) {
  World world({Ok(Said("hi"))}, "add things");
  world.conversation.system = "From the conversation.";
  agent::ContextPlan plan;
  Play(world, {}, nullptr, nullptr, &plan);
  EXPECT_EQ(RequestBody(world, 0)["system"], "From the conversation.");
  EXPECT_EQ(RequestBody(world, 0)["messages"][0]["content"].size(), 1u);  // no state: nothing in front
}

TEST(TurnContext, ASmallBudgetLeavesTheOldTurnsOutOfTheRequestButNotOutOfTheConversation) {
  World world({Ok(Said("hi"))}, "the new question");
  world.conversation.messages.insert(world.conversation.messages.begin(),
                                     {{llm::Role::kUser, {llm::TextBlock{std::string(200, 'a')}}}, {llm::Role::kAssistant, {llm::TextBlock{std::string(200, 'b')}}},
                                      {llm::Role::kUser, {llm::TextBlock{std::string(200, 'c')}}}, {llm::Role::kAssistant, {llm::TextBlock{std::string(200, 'd')}}}});
  std::size_t tools_bytes = 0;  // the tools take room too, and are not cut
  for (const llm::ToolDefinition& tool : world.tools.Definitions()) {
    tools_bytes += tool.name.size() + tool.description.size() + json::Dump(tool.parameters).size();
  }
  agent::ContextPlan plan;
  plan.budget_tokens = tools_bytes + 700;  // room for one of the two old turns, not for both
  plan.count = Bytes;
  EXPECT_EQ(Play(world, {}, nullptr, nullptr, &plan).reason, Reason::kDone);
  const Json body = RequestBody(world, 0);
  ASSERT_EQ(body["messages"].size(), 3u);  // the second old turn and the question: the oldest turn is gone
  EXPECT_EQ(world.conversation.messages.size(), 6u);  // all four old ones, the question and the answer
}

TEST(TurnContext, AContextThatCannotFitFailsTheTurnBeforeAnyRequestIsSent) {
  World world({Ok(Said("hi"))}, std::string(300, 'q'));
  agent::ContextPlan plan;
  plan.budget_tokens = 50;
  plan.count = Bytes;
  const agent::TurnOutcome outcome = Play(world, {}, nullptr, nullptr, &plan);
  EXPECT_EQ(outcome.reason, Reason::kFailed);
  EXPECT_NE(outcome.error.find("budget of 50"), std::string::npos) << outcome.error;
  EXPECT_TRUE(world.transport.seen().empty());
  EXPECT_TRUE(world.conversation.messages.empty());  // the question went too
}

TEST(TurnContext, TheHooksAreToldAboutEveryContextAndOnlyWhenThereIsAPlan) {
  World with_plan({Ok(Asks({CallAdd("c1", 1, 2)})), Ok(Said("3"))});
  const agent::ContextPlan plan = SimplePlan();
  Play(with_plan, {}, nullptr, nullptr, &plan);
  EXPECT_TRUE(Has(*with_plan.log, "ContextBuilt(1)"));
  EXPECT_TRUE(Has(*with_plan.log, "ContextBuilt(2)"));

  World without({Ok(Said("hi"))});
  Play(without);
  EXPECT_FALSE(Has(*without.log, "ContextBuilt(1)"));
}
