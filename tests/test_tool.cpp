#include "tool.h"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>

#include "json.h"

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

agent::ToolTable Toys() {
  agent::ToolTable table;
  agent::AddToyTools(table);
  return table;
}

std::string Names(const std::vector<llm::ToolDefinition>& definitions) {
  std::string names;
  for (const llm::ToolDefinition& definition : definitions) names += (names.empty() ? "" : ",") + definition.name;
  return names;
}

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

TEST(ToolTable, TheDefinitionsComeInTheOrderTheToolsWereAdded) {
  agent::ToolTable table;
  const auto nothing = [](const Json&) -> boost::asio::awaitable<agent::ToolResult> { co_return agent::ToolResult{"", false}; };
  table.Add({"second", "d", json::Object()}, nothing);
  table.Add({"first", "d", json::Object()}, nothing);
  EXPECT_EQ(Names(table.Definitions()), "second,first");
}

TEST(ToolTable, AKnownToolIsRunWithTheArgumentsAndItsResultComesBack) {
  agent::ToolTable table;
  table.Add({"echo", "d", json::Object()}, [](const Json& arguments) -> boost::asio::awaitable<agent::ToolResult> {
    co_return agent::ToolResult{"got " + json::Dump(arguments), false};
  });
  const agent::ToolResult result = RunTool(table, Call("echo", Json{{"x", 1}}));
  EXPECT_EQ(result.text, "got {\"x\":1}");
  EXPECT_FALSE(result.is_error);
}

TEST(ToolTable, AnUnknownToolIsAnErrorResultThatNamesItAndTheKnownOnes) {
  agent::ToolTable table = Toys();
  const agent::ToolResult result = RunTool(table, Call("nope", json::Object()));
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "nope"));
  EXPECT_TRUE(Contains(result.text, "add"));
  EXPECT_TRUE(Contains(result.text, "sleep"));
}

TEST(ToolTable, ArgumentsThatWereNotJsonAreAnErrorResultAndTheToolIsNotCalled) {
  bool called = false;
  agent::ToolTable table;
  table.Add({"t", "d", json::Object()}, [&called](const Json&) -> boost::asio::awaitable<agent::ToolResult> {
    called = true;
    co_return agent::ToolResult{"", false};
  });
  llm::ToolCallBlock call = Call("t", Json(nullptr));
  call.arguments_text = "{oops";
  const agent::ToolResult result = RunTool(table, call);
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "not valid JSON"));
  EXPECT_TRUE(Contains(result.text, "{oops"));
  EXPECT_FALSE(called);
}

TEST(ToolTable, AToolThatThrowsIsAnErrorResultWithTheMessage) {
  agent::ToolTable table;
  table.Add({"boom", "d", json::Object()}, [](const Json&) -> boost::asio::awaitable<agent::ToolResult> {
    throw std::runtime_error("it broke");
    co_return agent::ToolResult{};
  });
  const agent::ToolResult result = RunTool(table, Call("boom", json::Object()));
  EXPECT_TRUE(result.is_error);
  EXPECT_TRUE(Contains(result.text, "it broke"));
}

TEST(ToolTable, ACancellationIsNotSwallowed) {
  agent::ToolTable table;
  table.Add({"cancelled", "d", json::Object()}, [](const Json&) -> boost::asio::awaitable<agent::ToolResult> {
    throw boost::system::system_error(boost::asio::error::operation_aborted);
    co_return agent::ToolResult{};
  });
  const llm::ToolCallBlock call = Call("cancelled", json::Object());
  EXPECT_THROW(RunTool(table, call), boost::system::system_error);
}

TEST(ToyTools, AddAddsWholeNumbersAndFractions) {
  agent::ToolTable table = Toys();
  const struct {
    Json arguments;
    const char* expected;
  } cases[] = {{Json{{"a", 2}, {"b", 3}}, "5"},
               {Json{{"a", -4}, {"b", 1}}, "-3"},
               {Json{{"a", 0.5}, {"b", 0.25}}, "0.75"},
               {Json{{"a", 1}, {"b", 0.5}}, "1.5"},
               {Json{{"a", 0}, {"b", 0}}, "0"}};
  for (const auto& test : cases) {
    const agent::ToolResult result = RunTool(table, Call("add", test.arguments));
    EXPECT_FALSE(result.is_error) << json::Dump(test.arguments);
    EXPECT_EQ(result.text, test.expected) << json::Dump(test.arguments);
  }
}

TEST(ToyTools, AddSaysWhatIsWrongWithItsArguments) {
  agent::ToolTable table = Toys();
  const agent::ToolResult missing = RunTool(table, Call("add", Json{{"a", 1}}));
  EXPECT_TRUE(missing.is_error);
  EXPECT_TRUE(Contains(missing.text, "b"));

  const agent::ToolResult text = RunTool(table, Call("add", Json{{"a", "one"}, {"b", 2}}));
  EXPECT_TRUE(text.is_error);
  EXPECT_TRUE(Contains(text.text, "a"));
  EXPECT_TRUE(Contains(text.text, "number"));

  const agent::ToolResult not_an_object = RunTool(table, Call("add", json::Array({1, 2})));
  EXPECT_TRUE(not_an_object.is_error);
  EXPECT_TRUE(Contains(not_an_object.text, "object"));
}

TEST(ToyTools, AddDoesNotOverflow) {
  agent::ToolTable table = Toys();
  const agent::ToolResult result = RunTool(table, Call("add", Json{{"a", 9223372036854775807LL}, {"b", 1}}));
  EXPECT_FALSE(result.is_error);  // a number came back, whatever its last digits
  EXPECT_FALSE(result.text.empty());
}

TEST(ToyTools, AddDoesNotWrapAroundForNumbersAboveTheSignedRange) {
  agent::ToolTable table = Toys();
  const agent::ToolResult result = RunTool(table, Call("add", Json{{"a", 9223372036854775808ULL}, {"b", 0}}));
  EXPECT_FALSE(result.is_error);
  EXPECT_EQ(result.text, "9223372036854775808");  // 2^63, not a negative number
}

TEST(ToyTools, SleepWaitsAndSaysHowLong) {
  agent::ToolTable table = Toys();
  const auto start = std::chrono::steady_clock::now();
  const agent::ToolResult result = RunTool(table, Call("sleep", Json{{"ms", 30}}));
  EXPECT_FALSE(result.is_error);
  EXPECT_EQ(result.text, "slept 30 ms");
  EXPECT_GE(std::chrono::steady_clock::now() - start, 30ms);
}

TEST(ToyTools, SleepRejectsWhatItCannotDo) {
  agent::ToolTable table = Toys();
  for (const Json& arguments : {Json{{"ms", -1}}, Json{{"ms", 60001}}, json::Object(), Json{{"ms", "soon"}}, Json{{"ms", 1.5}}}) {
    const agent::ToolResult result = RunTool(table, Call("sleep", arguments));
    EXPECT_TRUE(result.is_error) << json::Dump(arguments);
    EXPECT_TRUE(Contains(result.text, "ms")) << result.text;
  }
}

TEST(ToyTools, ASleepThatIsCancelledEndsAtOnce) {
  agent::ToolTable table = Toys();
  boost::asio::io_context io;
  boost::asio::cancellation_signal signal;
  const llm::ToolCallBlock call = Call("sleep", Json{{"ms", 5000}});
  auto result = boost::asio::co_spawn(io, table.Run(call), boost::asio::bind_cancellation_slot(signal.slot(), boost::asio::use_future));
  boost::asio::steady_timer stop(io, 50ms);
  stop.async_wait([&signal](const boost::system::error_code&) { signal.emit(boost::asio::cancellation_type::terminal); });

  const auto start = std::chrono::steady_clock::now();
  io.run();
  EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);
  EXPECT_THROW(result.get(), boost::system::system_error);
}

TEST(ToyTools, TheDefinitionsAreSchemas) {
  agent::ToolTable table = Toys();
  const auto definitions = table.Definitions();
  EXPECT_EQ(Names(definitions), "add,sleep");
  for (const llm::ToolDefinition& definition : definitions) {
    EXPECT_FALSE(definition.description.empty()) << definition.name;
    EXPECT_EQ(definition.parameters["type"], "object") << definition.name;
    EXPECT_TRUE(definition.parameters.contains("properties")) << definition.name;
    EXPECT_TRUE(definition.parameters.contains("required")) << definition.name;
  }
}
