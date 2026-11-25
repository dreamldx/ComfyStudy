#include "tool.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>

namespace agent {
namespace {

ToolResult Error(const std::string& text) { return ToolResult{text, true}; }

// The text of a sum. Two whole numbers add up to a whole number (when it fits); anything else is
// written with the shortest digits that read back as the same number.
std::string SumText(const json::Value& a, const json::Value& b) {
  // A whole number that fits a signed 64-bit integer. (A big non-negative one is stored as unsigned,
  // and reading it as signed would fail instead of wrapping around.)
  const std::optional<std::int64_t> x = json::AsInt64(a);
  const std::optional<std::int64_t> y = json::AsInt64(b);
  if (x && y) {
    const bool overflows = (*y > 0 && *x > std::numeric_limits<std::int64_t>::max() - *y) ||
                           (*y < 0 && *x < std::numeric_limits<std::int64_t>::min() - *y);
    if (!overflows) return std::to_string(*x + *y);
  }
  const double sum = json::AsDouble(a) + json::AsDouble(b);
  char buffer[64];
  const auto written = std::to_chars(buffer, buffer + sizeof(buffer), sum);
  return std::string(buffer, written.ptr);
}

ToolResult Add(const json::Value& arguments) {
  if (!arguments.is_object()) return Error("the arguments of add must be an object with the numbers a and b");
  for (const char* name : {"a", "b"}) {
    const json::Value* found = json::Find(arguments, name);
    if (found == nullptr) return Error(std::string("add needs the number \"") + name + "\"");
    if (!found->is_number()) return Error(std::string("\"") + name + "\" must be a number");
  }
  return ToolResult{SumText(*json::Find(arguments, "a"), *json::Find(arguments, "b")), false};
}

constexpr std::int64_t kMaxSleepMs = 60000;

boost::asio::awaitable<ToolResult> Sleep(const json::Value& arguments) {
  const json::Value* ms = arguments.is_object() ? json::Find(arguments, "ms") : nullptr;
  if (ms == nullptr) co_return Error("sleep needs the whole number \"ms\"");
  const std::optional<std::int64_t> whole = json::AsInt64(*ms);
  if (!whole) co_return Error("\"ms\" must be a whole number of milliseconds");
  const std::int64_t wait = *whole;
  if (wait < 0 || wait > kMaxSleepMs) co_return Error("\"ms\" must be between 0 and " + std::to_string(kMaxSleepMs));
  boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, std::chrono::milliseconds(wait));
  co_await timer.async_wait(boost::asio::use_awaitable);  // a cancellation throws here and goes on up
  co_return ToolResult{"slept " + std::to_string(wait) + " ms", false};
}

}  // namespace

void ToolTable::Add(llm::ToolDefinition definition, ToolFunction function) {
  entries_.push_back({std::move(definition), std::move(function)});
}

std::vector<llm::ToolDefinition> ToolTable::Definitions() const {
  std::vector<llm::ToolDefinition> definitions;
  for (const Entry& entry : entries_) definitions.push_back(entry.definition);
  return definitions;
}

boost::asio::awaitable<ToolResult> ToolTable::Run(const llm::ToolCallBlock& call) {
  const Entry* found = nullptr;
  std::string known;
  for (const Entry& entry : entries_) {
    if (entry.definition.name == call.name) found = &entry;
    known += (known.empty() ? "" : ", ") + entry.definition.name;
  }
  if (found == nullptr) co_return Error("there is no tool called \"" + call.name + "\"; the tools are: " + known);
  if (call.arguments.is_null()) {
    co_return Error("the arguments are not valid JSON: " + call.arguments_text);
  }

  ToolResult result;
  try {
    result = co_await found->function(call.arguments);
  } catch (const boost::system::system_error& e) {
    if (e.code() == boost::asio::error::operation_aborted) throw;  // the Turn was cancelled: not the tool's failure
    result = Error(std::string("the tool failed: ") + e.what());
  } catch (const std::exception& e) {
    result = Error(std::string("the tool failed: ") + e.what());
  }
  co_return result;
}

void AddToyTools(ToolTable& table) {
  table.Add({"add",
             "Adds two numbers and returns the sum.",
             json::Value{{"type", "object"},
                         {"properties", {{"a", {{"type", "number"}, {"description", "the first number"}}},
                                         {"b", {{"type", "number"}, {"description", "the second number"}}}}},
                         {"required", json::Array({"a", "b"})}}},
            [](const json::Value& arguments) -> boost::asio::awaitable<ToolResult> { co_return Add(arguments); });
  table.Add({"sleep",
             "Waits for the given number of milliseconds (at most 60000) and then returns.",
             json::Value{{"type", "object"},
                         {"properties", {{"ms", {{"type", "integer"}, {"description", "how long to wait, in milliseconds"}}}}},
                         {"required", json::Array({"ms"})}}},
            [](const json::Value& arguments) -> boost::asio::awaitable<ToolResult> { co_return co_await Sleep(arguments); });
}

}  // namespace agent
