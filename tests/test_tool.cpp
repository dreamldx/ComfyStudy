#include "tool.h"

#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "json.h"
#include "toy_runtime.h"

// The toy tools, run through the tool runtime the way the program runs them.

namespace {

using Json = json::Value;
using namespace std::chrono_literals;

// Runs a coroutine to its end on its own event loop. (Not called Run: inside a TEST that name is
// the one of testing::Test.)
template <class T>
T Await(boost::asio::awaitable<T> work) {
  boost::asio::io_context io;
  auto result = boost::asio::co_spawn(io, std::move(work), boost::asio::use_future);
  io.run();
  return result.get();
}

llm::ToolCallBlock Call(const std::string& name, Json arguments) { return {"c1", name, std::move(arguments), "", ""}; }

agent::ToolResult RunTool(agent::ToolSet& tools, const llm::ToolCallBlock& call) { return Await(tools.Run(call)); }

std::string Names(const std::vector<llm::ToolDefinition>& definitions) {
  std::string names;
  for (const llm::ToolDefinition& definition : definitions) names += (names.empty() ? "" : ",") + definition.name;
  return names;
}

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

const llm::ToolDefinition& DefinitionOf(const std::vector<llm::ToolDefinition>& definitions, const std::string& name) {
  for (const llm::ToolDefinition& definition : definitions) {
    if (definition.name == name) return definition;
  }
  throw std::runtime_error("no tool called " + name);
}

}  // namespace

TEST(ToyTools, AddAddsWholeNumbersAndFractions) {
  agent::ToolRuntime tools = ToyRuntime();
  const struct {
    Json arguments;
    const char* expected;
  } cases[] = {{Json{{"a", 2}, {"b", 3}}, "5"},
               {Json{{"a", -4}, {"b", 1}}, "-3"},
               {Json{{"a", 0.5}, {"b", 0.25}}, "0.75"},
               {Json{{"a", 1}, {"b", 0.5}}, "1.5"},
               {Json{{"a", 0}, {"b", 0}}, "0"}};
  for (const auto& test : cases) {
    const agent::ToolResult result = RunTool(tools, Call("add", test.arguments));
    EXPECT_FALSE(result.is_error) << json::Dump(test.arguments);
    EXPECT_EQ(result.text, test.expected) << json::Dump(test.arguments);
  }
}

TEST(ToyTools, AddSaysWhatIsWrongWithItsArguments) {
  agent::ToolRuntime tools = ToyRuntime();
  const agent::ToolResult missing = RunTool(tools, Call("add", Json{{"a", 1}}));
  EXPECT_TRUE(missing.is_error);
  EXPECT_TRUE(Contains(missing.text, "missing required field \"b\"")) << missing.text;

  const agent::ToolResult text = RunTool(tools, Call("add", Json{{"a", "one"}, {"b", 2}}));
  EXPECT_TRUE(text.is_error);
  EXPECT_TRUE(Contains(text.text, "arguments.a: must be a number (got a string)")) << text.text;

  const agent::ToolResult not_an_object = RunTool(tools, Call("add", json::Array({1, 2})));
  EXPECT_TRUE(not_an_object.is_error);
  EXPECT_TRUE(Contains(not_an_object.text, "must be an object")) << not_an_object.text;
}

TEST(ToyTools, AddDoesNotOverflow) {
  agent::ToolRuntime tools = ToyRuntime();
  const agent::ToolResult result = RunTool(tools, Call("add", Json{{"a", 9223372036854775807LL}, {"b", 1}}));
  EXPECT_FALSE(result.is_error);  // a number came back, whatever its last digits
  EXPECT_FALSE(result.text.empty());
}

TEST(ToyTools, AddDoesNotWrapAroundForNumbersAboveTheSignedRange) {
  agent::ToolRuntime tools = ToyRuntime();
  const agent::ToolResult result = RunTool(tools, Call("add", Json{{"a", 9223372036854775808ULL}, {"b", 0}}));
  EXPECT_FALSE(result.is_error);
  EXPECT_EQ(result.text, "9223372036854775808");  // 2^63, not a negative number
}

TEST(ToyTools, SleepWaitsAndSaysHowLong) {
  agent::ToolRuntime tools = ToyRuntime();
  const auto start = std::chrono::steady_clock::now();
  const agent::ToolResult result = RunTool(tools, Call("sleep", Json{{"ms", 30}}));
  EXPECT_FALSE(result.is_error);
  EXPECT_EQ(result.text, "slept 30 ms");
  EXPECT_GE(std::chrono::steady_clock::now() - start, 30ms);
}

TEST(ToyTools, SleepRejectsWhatItCannotDoBeforeItRuns) {
  agent::ToolRuntime tools = ToyRuntime();
  for (const Json& arguments : {Json{{"ms", -1}}, Json{{"ms", 60001}}, json::Object(), Json{{"ms", "soon"}}, Json{{"ms", 1.5}}}) {
    const agent::ToolResult result = RunTool(tools, Call("sleep", arguments));
    EXPECT_TRUE(result.is_error) << json::Dump(arguments);
    EXPECT_TRUE(Contains(result.text, "ms")) << result.text;
  }
}

TEST(ToyTools, ASleepThatIsCancelledEndsAtOnce) {
  agent::ToolRuntime tools = ToyRuntime();
  boost::asio::io_context io;
  boost::asio::cancellation_signal signal;
  const llm::ToolCallBlock call = Call("sleep", Json{{"ms", 5000}});
  auto result = boost::asio::co_spawn(io, tools.Run(call), boost::asio::bind_cancellation_slot(signal.slot(), boost::asio::use_future));
  boost::asio::steady_timer stop(io, 50ms);
  stop.async_wait([&signal](const boost::system::error_code&) { signal.emit(boost::asio::cancellation_type::terminal); });

  const auto start = std::chrono::steady_clock::now();
  io.run();
  EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);
  try {
    result.get();
    FAIL() << "the sleep was not cancelled";
  } catch (const boost::system::system_error&) {
    SUCCEED();
  } catch (const boost::asio::multiple_exceptions&) {
    SUCCEED();
  }
}

TEST(ToyTools, ALongSleepIsNotCutByTheDefaultTimeLimit) {
  agent::RuntimeOptions options;
  options.timeout = 10ms;  // far below what sleep(40) takes; the tool has a limit of its own, above 60 seconds
  agent::ToolRuntime tools = ToyRuntime(options);
  EXPECT_EQ(RunTool(tools, Call("sleep", Json{{"ms", 40}})).text, "slept 40 ms");
}

TEST(ToyTools, RepeatWritesTheTextTheGivenNumberOfTimes) {
  agent::ToolRuntime tools = ToyRuntime();
  EXPECT_EQ(RunTool(tools, Call("repeat", Json{{"text", "ab"}, {"times", 3}})).text, "ab ab ab");
}

TEST(ToyTools, RepeatChecksItsRanges) {
  agent::ToolRuntime tools = ToyRuntime();
  EXPECT_TRUE(Contains(RunTool(tools, Call("repeat", Json{{"text", ""}, {"times", 1}})).text, "at least 1 character(s)"));
  EXPECT_TRUE(Contains(RunTool(tools, Call("repeat", Json{{"text", "x"}, {"times", 10001}})).text, "at most 10000"));
  EXPECT_TRUE(Contains(RunTool(tools, Call("repeat", Json{{"text", std::string(101, 'x')}, {"times", 1}})).text, "at most 100 character(s)"));
}

TEST(ToyTools, ALongRepeatIsCutByTheRuntime) {
  agent::ToolRuntime tools = ToyRuntime();
  const agent::ToolResult result = RunTool(tools, Call("repeat", Json{{"text", "0123456789"}, {"times", 10000}}));  // 100 KB
  EXPECT_FALSE(result.is_error);
  EXPECT_TRUE(result.truncated);
  EXPECT_TRUE(Contains(result.text, "[truncated: showing 16384 of 109999 bytes]")) << result.text.substr(result.text.size() - 60);
}

TEST(ToyTools, ShutdownIsDangerousAndIsRefusedByDefault) {
  agent::ToolRuntime tools = ToyRuntime();
  EXPECT_EQ(tools.EffectOf("shutdown"), agent::Effect::kDangerous);
  const agent::ToolResult result = RunTool(tools, Call("shutdown", Json{{"reason", "tired"}}));
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "dangerous"));

  agent::RuntimeOptions options;
  options.allow_dangerous = true;
  agent::ToolRuntime allowing = ToyRuntime(options);
  EXPECT_EQ(RunTool(allowing, Call("shutdown", Json{{"reason", "tired"}})).text, "pretending to shut the machine down because: tired");
}

TEST(ToyTools, TheClassification) {
  agent::ToolRuntime tools = ToyRuntime();
  EXPECT_EQ(tools.EffectOf("add"), agent::Effect::kReadOnly);
  EXPECT_EQ(tools.EffectOf("sleep"), agent::Effect::kReadOnly);
  EXPECT_EQ(tools.EffectOf("repeat"), agent::Effect::kReadOnly);
  EXPECT_EQ(tools.EffectOf("shutdown"), agent::Effect::kDangerous);
}

TEST(ToyTools, TheDefinitionsAreSchemas) {
  agent::ToolRuntime tools = ToyRuntime();
  const auto definitions = tools.Definitions();
  EXPECT_EQ(Names(definitions), "add,sleep,repeat,shutdown");
  for (const llm::ToolDefinition& definition : definitions) {
    EXPECT_FALSE(definition.description.empty()) << definition.name;
    EXPECT_EQ(definition.parameters["type"], "object") << definition.name;
    EXPECT_TRUE(definition.parameters.contains("properties")) << definition.name;
    EXPECT_TRUE(definition.parameters.contains("required")) << definition.name;
  }
}

TEST(ToyTools, TheSchemaOfATypedToolIsWhatGlazeWritesFromTheStructReadyForTheModel) {
  agent::ToolRuntime tools = ToyRuntime();
  const Json expected = json::Parse(R"({"type":"object","properties":{
      "ms":{"type":"integer","description":"how long to wait, in milliseconds","minimum":0,"maximum":60000}},
    "additionalProperties":false,"required":["ms"]})");
  const auto definitions = tools.Definitions();
  const Json& actual = DefinitionOf(definitions, "sleep").parameters;
  EXPECT_EQ(actual, expected) << json::Dump(actual);
}

TEST(ToyTools, NothingInTheSchemasStaysUnchecked) {
  // The toy tools are checked in full: no keyword of their schemas is left out of the validation.
  agent::ToolRuntime tools = ToyRuntime();
  for (const llm::ToolDefinition& definition : tools.Definitions()) {
    EXPECT_TRUE(schema::UncheckedKeywords(definition.parameters).empty()) << definition.name << ": " << json::Dump(definition.parameters);
  }
}
