#include "native_tools.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

namespace agent {

std::vector<ToolSpec> NativeTools::Specs() const {
  std::vector<ToolSpec> specs;
  for (const Entry& entry : entries_) specs.push_back(entry.spec);
  return specs;
}

boost::asio::awaitable<ToolResult> NativeTools::Call(const std::string& name, const json::Value& arguments) {
  for (const Entry& entry : entries_) {
    if (entry.spec.name == name) co_return co_await entry.function(arguments);
  }
  co_return ToolResult{"there is no tool called \"" + name + "\"", true};
}

// The arguments of the toy tools. The description and the range of a field are written next to the struct,
// in a glz::json_schema specialization: Glaze reads them when it writes the schema.
namespace toy {

struct SleepArgs {
  int ms = 0;
};

struct RepeatArgs {
  std::string text;
  int times = 1;
};

struct ShutdownArgs {
  std::string reason;
};

}  // namespace toy
}  // namespace agent

template <>
struct glz::json_schema<agent::toy::SleepArgs> {
  schema ms{.description = "how long to wait, in milliseconds", .minimum = 0, .maximum = 60000};
};

template <>
struct glz::json_schema<agent::toy::RepeatArgs> {
  schema text{.description = "the text to repeat (1 to 100 characters)", .minLength = 1, .maxLength = 100};
  schema times{.description = "how many times to write it (1 to 10000)", .minimum = 1, .maximum = 10000};
};

template <>
struct glz::json_schema<agent::toy::ShutdownArgs> {
  schema reason{.description = "why the machine is shut down"};
};

namespace agent {
namespace {

// The text of a sum. Two whole numbers add up to a whole number (when it fits); anything else is written with
// the shortest digits that read back as the same number.
std::string SumText(const json::Value& a, const json::Value& b) {
  // A whole number that fits a signed 64-bit integer. (A big non-negative one is stored as unsigned, and
  // reading it as signed would fail instead of wrapping around.)
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

}  // namespace

void AddToyTools(NativeTools& tools) {
  // add: written with AddRaw, because the sum of two whole numbers has to stay exact (a struct with two doubles
  // would round them).
  ToolSpec add;
  add.name = "add";
  add.description = "Adds two numbers and returns the sum.";
  add.parameters = json::Value{{"type", "object"},
                               {"properties", {{"a", {{"type", "number"}, {"description", "the first number"}}},
                                               {"b", {{"type", "number"}, {"description", "the second number"}}}}},
                               {"required", json::Array({"a", "b"})}};
  add.effect = Effect::kReadOnly;
  tools.AddRaw(std::move(add), [](const json::Value& arguments) -> boost::asio::awaitable<ToolResult> {
    co_return ToolResult{SumText(*json::Find(arguments, "a"), *json::Find(arguments, "b")), false};
  });

  tools.Add<toy::SleepArgs>("sleep", "Waits for the given number of milliseconds (at most 60000) and then returns.",
                            {.effect = Effect::kReadOnly, .timeout = std::chrono::milliseconds(65000)},
                            [](const toy::SleepArgs& args) -> boost::asio::awaitable<ToolResult> {
                              boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, std::chrono::milliseconds(args.ms));
                              co_await timer.async_wait(boost::asio::use_awaitable);  // a cancellation throws here and goes on up
                              co_return ToolResult{"slept " + std::to_string(args.ms) + " ms", false};
                            });

  tools.Add<toy::RepeatArgs>("repeat", "Writes the text the given number of times, separated by spaces. The result can be very long.",
                             {.effect = Effect::kReadOnly},
                             [](const toy::RepeatArgs& args) -> boost::asio::awaitable<ToolResult> {
                               std::string out;
                               for (int i = 0; i < args.times; ++i) out += (i == 0 ? "" : " ") + args.text;
                               co_return ToolResult{std::move(out), false};
                             });

  tools.Add<toy::ShutdownArgs>("shutdown", "Shuts the machine down.", {.effect = Effect::kDangerous},
                               [](const toy::ShutdownArgs& args) -> boost::asio::awaitable<ToolResult> {
                                 co_return ToolResult{"pretending to shut the machine down because: " + args.reason, false};
                               });
}

}  // namespace agent
