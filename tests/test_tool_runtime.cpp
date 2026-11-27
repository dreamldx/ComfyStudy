#include "tool_runtime.h"

#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "json.h"
#include "llm_error.h"
#include "schema.h"

namespace {

using Json = json::Value;
using namespace std::chrono_literals;
namespace asio = boost::asio;

template <class T>
T Await(asio::awaitable<T> work) {
  asio::io_context io;
  auto result = asio::co_spawn(io, std::move(work), asio::use_future);
  io.run();
  return result.get();
}

llm::ToolCallBlock Call(const std::string& name, Json arguments) { return {"c1", name, std::move(arguments), "", ""}; }

using Handler = std::function<asio::awaitable<agent::ToolResult>(const Json&)>;

// A source that has the tools it is given. `calls` counts how often each tool was really called.
class FakeSource final : public agent::ToolSource {
 public:
  void Add(agent::ToolSpec spec, Handler handler) {
    handlers_[spec.name] = std::move(handler);
    specs_.push_back(std::move(spec));
  }
  void Remove(const std::string& name) {
    for (auto it = specs_.begin(); it != specs_.end(); ++it) {
      if (it->name == name) {
        specs_.erase(it);
        break;
      }
    }
  }
  std::vector<agent::ToolSpec> Specs() const override { return specs_; }
  void StartOver() override { ++starts; }
  int starts = 0;
  asio::awaitable<agent::ToolResult> Call(const std::string& name, const Json& arguments) override {
    ++calls[name];
    co_return co_await handlers_.at(name)(arguments);
  }
  std::map<std::string, int> calls;

 private:
  std::vector<agent::ToolSpec> specs_;
  std::map<std::string, Handler> handlers_;
};

Json ObjectSchema(const char* text) { return json::Parse(text); }

constexpr const char* kSleepSchema =
    R"({"type":"object","properties":{"ms":{"type":"integer","minimum":0,"maximum":60000}},"required":["ms"],"additionalProperties":false})";

Handler Says(const std::string& text) {
  return [text](const Json&) -> asio::awaitable<agent::ToolResult> { co_return agent::ToolResult{text, false}; };
}

agent::ToolSpec Spec(const std::string& name, const char* schema = R"({"type":"object"})", agent::Effect effect = agent::Effect::kReadOnly) {
  agent::ToolSpec spec;
  spec.name = name;
  spec.description = "the tool " + name;
  spec.parameters = ObjectSchema(schema);
  spec.effect = effect;
  return spec;
}

std::string Names(const std::vector<llm::ToolDefinition>& definitions) {
  std::string names;
  for (const llm::ToolDefinition& definition : definitions) names += (names.empty() ? "" : ",") + definition.name;
  return names;
}

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

struct World {
  std::shared_ptr<FakeSource> source = std::make_shared<FakeSource>();
  agent::ToolRuntime runtime;
  explicit World(agent::RuntimeOptions options = {}) : runtime(options) { runtime.AddSource(source); }
  agent::ToolResult Run(const std::string& name, Json arguments) { return Await(runtime.Run(Call(name, std::move(arguments)))); }
};

}  // namespace

TEST(ToolRuntime, TheDefinitionsOfAllSourcesComeInTheOrderTheSourcesWereAdded) {
  auto first = std::make_shared<FakeSource>();
  auto second = std::make_shared<FakeSource>();
  first->Add(Spec("b"), Says(""));
  first->Add(Spec("a"), Says(""));
  second->Add(Spec("c"), Says(""));
  agent::ToolRuntime runtime;
  runtime.AddSource(first);
  runtime.AddSource(second);
  const auto definitions = runtime.Definitions();
  EXPECT_EQ(Names(definitions), "b,a,c");
  EXPECT_EQ(definitions[0].description, "the tool b");
  EXPECT_EQ(definitions[0].parameters, ObjectSchema(R"({"type":"object"})"));
}

TEST(ToolRuntime, TwoSourcesCannotHaveAToolWithTheSameName) {
  auto first = std::make_shared<FakeSource>();
  auto second = std::make_shared<FakeSource>();
  first->Add(Spec("same"), Says(""));
  second->Add(Spec("same"), Says(""));
  agent::ToolRuntime runtime;
  runtime.AddSource(first);
  EXPECT_THROW(runtime.AddSource(second), std::invalid_argument);
}

TEST(ToolRuntime, WhatASourceHasIsAskedEveryTime) {
  World world;
  world.source->Add(Spec("one"), Says(""));
  EXPECT_EQ(Names(world.runtime.Definitions()), "one");
  world.source->Add(Spec("two"), Says(""));  // a Skill was loaded
  EXPECT_EQ(Names(world.runtime.Definitions()), "one,two");
  world.source->Remove("one");  // and another one unloaded
  EXPECT_EQ(Names(world.runtime.Definitions()), "two");
  EXPECT_TRUE(world.Run("one", json::Object()).is_error);
}

TEST(ToolRuntime, ACallOfAKnownToolRunsItAndFillsInTheClassification) {
  World world;
  world.source->Add(Spec("echo", R"({"type":"object"})", agent::Effect::kReadOnly), [](const Json& arguments) -> asio::awaitable<agent::ToolResult> {
    co_return agent::ToolResult{"got " + json::Dump(arguments), false};
  });
  const agent::ToolResult result = world.Run("echo", Json{{"x", 1}});
  EXPECT_EQ(result.text, "got {\"x\":1}");
  EXPECT_FALSE(result.is_error);
  EXPECT_FALSE(result.truncated);
  EXPECT_EQ(result.effect, agent::Effect::kReadOnly);
}

TEST(ToolRuntime, AnUnknownToolIsAnErrorThatNamesItAndTheKnownOnes) {
  World world;
  world.source->Add(Spec("add"), Says(""));
  world.source->Add(Spec("sleep"), Says(""));
  const agent::ToolResult result = world.Run("nope", json::Object());
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "nope"));
  EXPECT_TRUE(Contains(result.text, "add, sleep"));
}

TEST(ToolRuntime, ArgumentsThatWereNotJsonAreAnErrorAndTheToolIsNotCalled) {
  World world;
  world.source->Add(Spec("t"), Says("ran"));
  llm::ToolCallBlock call{"c1", "t", Json{}, "{\"a\":", ""};  // null arguments: the text was not JSON
  const agent::ToolResult result = Await(world.runtime.Run(call));
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "not valid JSON"));
  EXPECT_TRUE(Contains(result.text, "{\"a\":"));
  EXPECT_EQ(world.source->calls["t"], 0);
}

TEST(ToolRuntime, ArgumentsThatDoNotFitTheSchemaAreRefusedWithEveryProblemListed) {
  World world;
  world.source->Add(Spec("sleep", kSleepSchema), Says("ran"));
  const agent::ToolResult result = world.Run("sleep", Json{{"ms", 70000}, {"extra", true}});
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "sleep"));
  EXPECT_TRUE(Contains(result.text, "arguments.ms: must be at most 60000 (got 70000)"));
  EXPECT_TRUE(Contains(result.text, "unexpected field \"extra\""));
  EXPECT_EQ(world.source->calls["sleep"], 0);
}

TEST(ToolRuntime, AToolThatThrowsIsAnErrorResult) {
  World world;
  world.source->Add(Spec("bad"), [](const Json&) -> asio::awaitable<agent::ToolResult> { throw std::runtime_error("disk on fire"); co_return agent::ToolResult{}; });
  const agent::ToolResult result = world.Run("bad", json::Object());
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "disk on fire")) << result.text;
}

asio::awaitable<void> Wait(std::chrono::milliseconds how_long) {
  asio::steady_timer timer(co_await asio::this_coro::executor, how_long);
  co_await timer.async_wait(asio::use_awaitable);
}

Handler Sleeps(std::chrono::milliseconds how_long) {
  return [how_long](const Json&) -> asio::awaitable<agent::ToolResult> {
    co_await Wait(how_long);
    co_return agent::ToolResult{"slept", false};
  };
}

TEST(ToolRuntime, ACallThatTakesTooLongIsStoppedAndSaysSo) {
  World world;
  agent::ToolSpec spec = Spec("slow");
  spec.timeout = 50ms;
  world.source->Add(spec, Sleeps(5000ms));
  const auto start = std::chrono::steady_clock::now();
  const agent::ToolResult result = world.Run("slow", json::Object());
  EXPECT_LT(std::chrono::steady_clock::now() - start, 2000ms);
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "did not finish within 50 ms"));
}

TEST(ToolRuntime, AToolWithoutATimeLimitOfItsOwnGetsTheDefault) {
  agent::RuntimeOptions options;
  options.timeout = 40ms;
  World world(options);
  world.source->Add(Spec("slow"), Sleeps(5000ms));
  const agent::ToolResult result = world.Run("slow", json::Object());
  EXPECT_TRUE(Contains(result.text, "did not finish within 40 ms"));
}

TEST(ToolRuntime, ATimeLimitOfZeroMeansNone) {
  agent::RuntimeOptions options;
  options.timeout = 0ms;
  World world(options);
  world.source->Add(Spec("quick"), Sleeps(60ms));
  EXPECT_EQ(world.Run("quick", json::Object()).text, "slept");
}

TEST(ToolRuntime, ACancellationFromOutsideEndsTheCallAndGoesOn) {
  World world;
  world.source->Add(Spec("slow"), Sleeps(5000ms));
  asio::io_context io;
  asio::cancellation_signal signal;
  const llm::ToolCallBlock call = Call("slow", json::Object());  // must outlive the coroutine
  auto result = asio::co_spawn(io, world.runtime.Run(call), asio::bind_cancellation_slot(signal.slot(), asio::use_future));
  asio::steady_timer later(io, 30ms);
  later.async_wait([&](const boost::system::error_code&) { signal.emit(asio::cancellation_type::all); });
  const auto start = std::chrono::steady_clock::now();
  io.run();
  EXPECT_LT(std::chrono::steady_clock::now() - start, 2000ms);
  try {
    const agent::ToolResult got = result.get();
    FAIL() << "the cancellation did not reach the caller; the call returned: " << got.text;
  } catch (const boost::system::system_error& e) {
    EXPECT_EQ(e.code(), asio::error::operation_aborted);
  } catch (const asio::multiple_exceptions&) {
    SUCCEED();  // the call and the timer next to it both ended with the cancellation
  }
}

Handler Returns(std::string text) {
  return [text = std::move(text)](const Json&) -> asio::awaitable<agent::ToolResult> { co_return agent::ToolResult{text, false}; };
}

TEST(ToolRuntime, AResultThatIsTooLongIsCutAndSaysSo) {
  agent::RuntimeOptions options;
  options.max_result_bytes = 100;
  World world(options);
  world.source->Add(Spec("long"), Returns(std::string(500, 'x')));
  const agent::ToolResult result = world.Run("long", json::Object());
  EXPECT_TRUE(result.truncated);
  EXPECT_FALSE(result.is_error);
  EXPECT_TRUE(Contains(result.text, std::string(100, 'x') + "\n[truncated: showing 100 of 500 bytes]"));
  EXPECT_FALSE(Contains(result.text, std::string(101, 'x')));
}

TEST(ToolRuntime, TheCutNeverSplitsACharacter) {
  agent::RuntimeOptions options;
  options.max_result_bytes = 10;  // 3 characters of 3 bytes are 9 bytes; the 4th would end at byte 12
  World world(options);
  std::string text;
  for (int i = 0; i < 5; ++i) text += "\u4f60";
  world.source->Add(Spec("cn"), Returns(text));
  const agent::ToolResult result = world.Run("cn", json::Object());
  EXPECT_TRUE(result.truncated);
  EXPECT_TRUE(Contains(result.text, "\u4f60\u4f60\u4f60\n[truncated: showing 9 of 15 bytes]"));
  const std::string shown = result.text.substr(0, result.text.find('\n'));
  EXPECT_EQ(shown.size(), 9u);
}

TEST(ToolRuntime, AResultThatFitsIsNotTouched) {
  agent::RuntimeOptions options;
  options.max_result_bytes = 100;
  World world(options);
  world.source->Add(Spec("short"), Returns(std::string(100, 'y')));
  const agent::ToolResult result = world.Run("short", json::Object());
  EXPECT_FALSE(result.truncated);
  EXPECT_EQ(result.text, std::string(100, 'y'));
}

TEST(ToolRuntime, ADangerousToolIsRefusedUnlessAllowed) {
  World refusing;
  refusing.source->Add(Spec("shutdown", R"({"type":"object"})", agent::Effect::kDangerous), Says("shutting down"));
  const agent::ToolResult refused = refusing.Run("shutdown", json::Object());
  EXPECT_TRUE(refused.is_error);
  EXPECT_TRUE(Contains(refused.text, "dangerous"));
  EXPECT_EQ(refusing.source->calls["shutdown"], 0);
  EXPECT_EQ(refused.effect, agent::Effect::kDangerous);

  agent::RuntimeOptions options;
  options.allow_dangerous = true;
  World allowing(options);
  allowing.source->Add(Spec("shutdown", R"({"type":"object"})", agent::Effect::kDangerous), Says("shutting down"));
  EXPECT_EQ(allowing.Run("shutdown", json::Object()).text, "shutting down");
}

TEST(ToolRuntime, TheClassificationCanBeAsked) {
  World world;
  world.source->Add(Spec("look", R"({"type":"object"})", agent::Effect::kReadOnly), Says(""));
  world.source->Add(Spec("write", R"({"type":"object"})", agent::Effect::kSideEffect), Says(""));
  EXPECT_EQ(world.runtime.EffectOf("look"), agent::Effect::kReadOnly);
  EXPECT_EQ(world.runtime.EffectOf("write"), agent::Effect::kSideEffect);
  EXPECT_FALSE(world.runtime.EffectOf("nothing").has_value());
}

TEST(ToolRuntime, AToolThatDoesNotSayHowHarmlessItIsIsNotAssumedToBeReadOnly) {
  EXPECT_EQ(agent::ToolSpec{}.effect, agent::Effect::kSideEffect);
}

TEST(RuntimeOptions, TheDefaultsAndTheSettings) {
  const agent::RuntimeOptions defaults = agent::RuntimeOptionsFromSettings({});
  EXPECT_EQ(defaults.timeout, 30000ms);
  EXPECT_EQ(defaults.max_result_bytes, 16384u);
  EXPECT_FALSE(defaults.allow_dangerous);
  const agent::RuntimeOptions set = agent::RuntimeOptionsFromSettings({{"tool_timeout_ms", "0"}, {"tool_result_max_bytes", "1000"}});
  EXPECT_EQ(set.timeout, 0ms);
  EXPECT_EQ(set.max_result_bytes, 1000u);
}

TEST(RuntimeOptions, ABadValueIsAnErrorThatNamesTheSetting) {
  for (const auto& bad : std::vector<config::Values>{{{"tool_timeout_ms", "soon"}}, {{"tool_timeout_ms", "-5"}}, {{"tool_result_max_bytes", "10"}}, {{"tool_result_max_bytes", "lots"}}}) {
    try {
      (void)agent::RuntimeOptionsFromSettings(bad);
      FAIL() << "no error for " << bad.begin()->first << "=" << bad.begin()->second;
    } catch (const llm::Error& e) {
      EXPECT_TRUE(Contains(e.what(), bad.begin()->first)) << e.what();
    }
  }
}

TEST(ToolRuntime, ARemovedSourceTakesItsToolsAwayAndTheOthersStay) {
  auto first = std::make_shared<FakeSource>();
  auto second = std::make_shared<FakeSource>();
  first->Add(Spec("a"), Says("a"));
  second->Add(Spec("b"), Says("b"));
  agent::ToolRuntime runtime;
  runtime.AddSource(first);
  runtime.AddSource(second);
  EXPECT_TRUE(runtime.RemoveSource(second));
  EXPECT_EQ(Names(runtime.Definitions()), "a");
  const agent::ToolResult gone = Await(runtime.Run(Call("b", json::Object())));
  EXPECT_TRUE(gone.is_error);
  EXPECT_TRUE(Contains(gone.text, "there is no tool called \"b\""));
  EXPECT_FALSE(runtime.RemoveSource(second));  // it is not there any more
  runtime.AddSource(second);                   // and may come back
  EXPECT_EQ(Names(runtime.Definitions()), "a,b");
}

TEST(ToolRuntime, ASourceMayBeRemovedByTheCallThatIsRunning) {
  auto source = std::make_shared<FakeSource>();
  agent::ToolRuntime runtime;
  source->Add(Spec("once"), [&runtime, source](const Json&) -> asio::awaitable<agent::ToolResult> {
    runtime.RemoveSource(source);
    co_return agent::ToolResult{"done", false};
  });
  runtime.AddSource(source);
  const agent::ToolResult result = Await(runtime.Run(Call("once", json::Object())));
  EXPECT_FALSE(result.is_error);
  EXPECT_EQ(result.text, "done");
  EXPECT_EQ(Names(runtime.Definitions()), "");
}

TEST(ToolRuntime, StartOverIsPassedToEverySource) {
  auto first = std::make_shared<FakeSource>();
  auto second = std::make_shared<FakeSource>();
  agent::ToolRuntime runtime;
  runtime.AddSource(first);
  runtime.AddSource(second);
  runtime.StartOver();
  EXPECT_EQ(first->starts, 1);
  EXPECT_EQ(second->starts, 1);
}
