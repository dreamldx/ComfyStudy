#include "skill_tools.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "agent.h"
#include "json.h"
#include "native_tools.h"
#include "tool_runtime.h"

#include "scripted_transport.h"

namespace {

using Json = json::Value;
namespace asio = boost::asio;

template <class T>
T Await(asio::awaitable<T> work) {
  asio::io_context io;
  auto result = asio::co_spawn(io, std::move(work), asio::use_future);
  io.run();
  return result.get();
}

void Await(asio::awaitable<void> work) {
  asio::io_context io;
  auto result = asio::co_spawn(io, std::move(work), asio::use_future);
  io.run();
  result.get();
}

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

llm::ToolCallBlock Call(const std::string& name, Json arguments) { return {"c1", name, std::move(arguments), "", ""}; }

agent::ToolFunction Says(const std::string& text) {
  return [text](const Json&) -> asio::awaitable<agent::ToolResult> { co_return agent::ToolResult{text, false}; };
}

void AddTool(agent::NativeTools& library, const std::string& name, agent::Effect effect = agent::Effect::kReadOnly) {
  agent::ToolSpec spec;
  spec.name = name;
  spec.description = "the tool " + name;
  spec.parameters = json::Parse(R"({"type":"object","additionalProperties":false})");
  spec.effect = effect;
  library.AddRaw(std::move(spec), Says("result of " + name));
}

agent::SkillInfo Skill(const std::string& name, const std::string& task, std::vector<std::string> tools, const std::string& body) {
  return agent::SkillInfo{name, task, std::move(tools), body, {}};
}

std::string Names(const std::vector<llm::ToolDefinition>& definitions) {
  std::string names;
  for (const llm::ToolDefinition& definition : definitions) names += (names.empty() ? "" : ",") + definition.name;
  return names;
}

std::string Joined(const std::vector<std::string>& items) {
  std::string joined;
  for (const std::string& item : items) joined += (joined.empty() ? "" : ",") + item;
  return joined;
}

// Two Skills that share a tool (`shared`), one with a tool of its own, and one with a dangerous tool.
struct World {
  World() {
    auto library = std::make_shared<agent::NativeTools>();
    AddTool(*library, "add");
    AddTool(*library, "shared");
    AddTool(*library, "sleep");
    AddTool(*library, "shutdown", agent::Effect::kDangerous);
    agent::SkillCatalog catalog;
    catalog.skills = {Skill("calc", "exact arithmetic", {"add", "shared"}, "Use add for sums."),
                      Skill("timer", "waiting for a while", {"sleep", "shared"}, "Use sleep."),
                      Skill("power", "switching the machine off", {"shutdown"}, "Careful.")};
    skills = std::make_shared<agent::SkillTools>(std::move(catalog), library, runtime);
    runtime.AddSource(skills);
  }
  agent::ToolResult Run(const std::string& name, Json arguments) { return Await(runtime.Run(Call(name, std::move(arguments)))); }
  agent::ToolResult Load(const std::string& name) { return Run("load_skill", Json{{"name", name}}); }
  agent::ToolResult Unload(const std::string& name) { return Run("unload_skill", Json{{"name", name}}); }
  void TurnStarts() { Await(skills->OnTurnStarted(agent::TurnStarted{1})); }
  void TurnEnds(agent::TurnOutcome::Reason reason) {
    agent::TurnOutcome outcome;
    outcome.reason = reason;
    Await(skills->OnTurnFinished(agent::TurnFinished{outcome}));
  }
  agent::ToolRuntime runtime;
  std::shared_ptr<agent::SkillTools> skills;
};

}  // namespace

TEST(NativeToolsSubset, HasOnlyTheNamedToolsAndRunsThem) {
  agent::NativeTools library;
  AddTool(library, "a");
  AddTool(library, "b");
  AddTool(library, "c");
  const auto subset = library.Subset({"c", "a"});
  ASSERT_EQ(subset->Specs().size(), 2u);
  EXPECT_EQ(subset->Specs()[0].name, "a");  // in the order of the library
  EXPECT_EQ(subset->Specs()[1].name, "c");
  EXPECT_EQ(Await(subset->Call("c", Json::object_t{})).text, "result of c");
  EXPECT_TRUE(library.Has("b"));
  EXPECT_FALSE(library.Has("z"));
}

TEST(SkillTools, AtTheStartOnlyTheTwoToolsOfTheMechanismAreOffered) {
  World world;
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill");
  EXPECT_TRUE(world.skills->Loaded().empty());
}

TEST(SkillTools, TheListOfSkillsHasTheNamesAndTheTasksButNoToolNames) {
  World world;
  const auto definitions = world.runtime.Definitions();
  const std::string& text = definitions.at(0).description;
  EXPECT_TRUE(Contains(text, "- calc: exact arithmetic")) << text;
  EXPECT_TRUE(Contains(text, "- timer: waiting for a while")) << text;
  EXPECT_TRUE(Contains(text, "- power: switching the machine off")) << text;
  for (const char* tool : {"add", "sleep", "shutdown", "shared"}) {
    EXPECT_FALSE(Contains(text, std::string("tool ") + tool)) << text;
    EXPECT_FALSE(Contains(text, std::string(" ") + tool + ",")) << text;
  }
  EXPECT_FALSE(Contains(text, "(loaded)")) << text;
}

TEST(SkillTools, TheNameOfTheSkillMustBeOneOfTheSkillsAndThatIsCheckedBeforeTheCall) {
  World world;
  const agent::ToolResult result = world.Load("calculator");
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "must be one of \"calc\", \"timer\", \"power\"")) << result.text;
}

TEST(SkillTools, LoadingASkillGivesItsInstructionsAndTheToolsAppearInTheRuntime) {
  World world;
  const agent::ToolResult result = world.Load("calc");
  EXPECT_FALSE(result.is_error) << result.text;
  EXPECT_TRUE(Contains(result.text, "Use add for sums.")) << result.text;
  EXPECT_TRUE(Contains(result.text, "add, shared")) << result.text;
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill,add,shared");
  EXPECT_EQ(world.Run("add", json::Object()).text, "result of add");
  EXPECT_EQ(Joined(world.skills->Loaded()), "calc");
}

TEST(SkillTools, TheListOfSkillsMarksTheLoadedOnes) {
  World world;
  world.Load("timer");
  const std::string text = world.runtime.Definitions().at(0).description;
  EXPECT_TRUE(Contains(text, "- timer: waiting for a while (loaded)")) << text;
  EXPECT_FALSE(Contains(text, "- calc: exact arithmetic (loaded)")) << text;
}

TEST(SkillTools, LoadingASkillThatIsLoadedIsAnErrorAndChangesNothing) {
  World world;
  world.Load("calc");
  const agent::ToolResult again = world.Load("calc");
  EXPECT_TRUE(again.is_error);
  EXPECT_TRUE(Contains(again.text, "already loaded")) << again.text;
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill,add,shared");
}

TEST(SkillTools, UnloadingRemovesTheToolsAgain) {
  World world;
  world.Load("calc");
  const agent::ToolResult result = world.Unload("calc");
  EXPECT_FALSE(result.is_error) << result.text;
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill");
  EXPECT_TRUE(world.Run("add", json::Object()).is_error);
  EXPECT_TRUE(world.skills->Loaded().empty());
}

TEST(SkillTools, UnloadingASkillThatIsNotLoadedIsAnError) {
  World world;
  const agent::ToolResult result = world.Unload("calc");
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "not loaded")) << result.text;
}

TEST(SkillTools, AToolOfTwoLoadedSkillsStaysUntilTheLastOfThemIsUnloaded) {
  World world;
  world.Load("calc");
  world.Load("timer");
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill,add,shared,sleep");  // shared once
  world.Unload("calc");
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill,shared,sleep");
  EXPECT_FALSE(world.Run("shared", json::Object()).is_error);
  world.Unload("timer");
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill");
}

TEST(SkillTools, TheOrderOfUnloadingDoesNotMatter) {
  World world;
  world.Load("calc");
  world.Load("timer");
  world.Unload("timer");
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill,add,shared");
  world.Unload("calc");
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill");
}

TEST(SkillTools, ASkillWhoseToolNameIsTakenByAnotherSourceIsNotLoadedAndLeavesNothingBehind) {
  World world;
  auto other = std::make_shared<agent::NativeTools>();
  AddTool(*other, "shared");
  world.runtime.AddSource(other);  // somebody else already has a tool called "shared"
  const agent::ToolResult result = world.Load("calc");
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "shared")) << result.text;
  EXPECT_TRUE(world.skills->Loaded().empty());
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill,shared");  // "add" was taken back too
}

TEST(SkillTools, ADangerousToolIsStillRefusedWhenItsSkillIsLoaded) {
  World world;
  world.Load("power");
  const agent::ToolResult result = world.Run("shutdown", json::Object());
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "dangerous")) << result.text;
}

TEST(SkillTools, StartingOverUnloadsEverything) {
  World world;
  world.Load("calc");
  world.Load("timer");
  world.runtime.StartOver();
  EXPECT_TRUE(world.skills->Loaded().empty());
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill");
}

TEST(SkillTools, ATurnThatDidNotEndInAnAnswerGivesBackTheSkillsOfBeforeTheTurn) {
  World world;
  world.Load("calc");
  world.TurnStarts();
  world.Unload("calc");
  world.Load("timer");
  world.TurnEnds(agent::TurnOutcome::Reason::kFailed);
  EXPECT_EQ(Joined(world.skills->Loaded()), "calc");
  EXPECT_EQ(Names(world.runtime.Definitions()), "load_skill,unload_skill,add,shared");
}

TEST(SkillTools, ATurnThatEndedInAnAnswerKeepsWhatWasLoaded) {
  World world;
  world.TurnStarts();
  world.Load("calc");
  world.TurnEnds(agent::TurnOutcome::Reason::kDone);
  EXPECT_EQ(Joined(world.skills->Loaded()), "calc");
}

TEST(SkillTools, EveryWayAGoodTurnDoesNotEndGivesBackTheSkills) {
  using Reason = agent::TurnOutcome::Reason;
  for (const Reason reason : {Reason::kMaxSteps, Reason::kCancelled, Reason::kTimedOut, Reason::kFailed, Reason::kNoText}) {
    World world;
    world.TurnStarts();
    world.Load("calc");
    world.TurnEnds(reason);
    EXPECT_TRUE(world.skills->Loaded().empty());
  }
}

TEST(SkillTools, ASkillLoadedInTheMiddleOfATurnIsThereInTheNextRequestOfTheSameTurn) {
  World world;
  llm::Provider provider{"anthropic", llm::Protocol::kAnthropic, llm::Endpoint{"https://a.test", "key"}, "m", {true, 1000, 100000}};
  auto reply = [](std::vector<llm::Block> blocks, const char* stop) {
    llm::Reply r;
    r.message.role = llm::Role::kAssistant;
    r.message.blocks = std::move(blocks);
    r.stop_reason = stop;
    return http::Response{200, AnthropicBody(r)};
  };
  ScriptedTransport transport({reply({llm::ToolCallBlock{"c1", "load_skill", Json{{"name", "calc"}}, "", ""}}, "tool_use"),
                               reply({llm::ToolCallBlock{"c2", "add", Json::object_t{}, "", ""}}, "tool_use"),
                               reply({llm::TextBlock{"done"}}, "end_turn")});
  llm::Conversation conversation;
  conversation.messages.push_back({llm::Role::kUser, {llm::TextBlock{"add things"}}});
  const agent::Hooks hooks = {world.skills};
  boost::asio::io_context io;
  auto outcome = boost::asio::co_spawn(
      io, agent::RunTurn(transport, provider, conversation, llm::Options{}, &world.runtime, hooks, agent::TurnLimits{}, nullptr),
      boost::asio::use_future);
  io.run();
  EXPECT_EQ(outcome.get().reason, agent::TurnOutcome::Reason::kDone);

  const auto seen = transport.seen();
  ASSERT_EQ(seen.size(), 3u);
  EXPECT_EQ(json::Parse(seen[0].body)["tools"].size(), 2u);
  EXPECT_EQ(json::Parse(seen[1].body)["tools"].size(), 4u);  // load_skill, unload_skill, add, shared
  EXPECT_EQ(json::Parse(seen[2].body)["tools"].size(), 4u);
  // what the second request carries back: the instructions of the Skill
  EXPECT_TRUE(Contains(seen[1].body, "Use add for sums."));
  // and the add call really ran
  EXPECT_TRUE(Contains(seen[2].body, "result of add"));
}
