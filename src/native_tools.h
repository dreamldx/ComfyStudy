#pragma once

#include <boost/asio.hpp>

#include <glaze/glaze.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "json.h"
#include "schema.h"
#include "tool_runtime.h"

namespace agent {

// How a tool is classified and how long it may take (zero: the runtime's default).
struct ToolOptions {
  Effect effect = Effect::kSideEffect;
  std::chrono::milliseconds timeout{0};
};

using ToolFunction = std::function<boost::asio::awaitable<ToolResult>(const json::Value& arguments)>;

// The tools written in C++ in this program. Two ways to add one:
//
//   Add<Args>   the arguments are a struct. The schema the model sees is written from the struct by Glaze
//               (the description and the range of a field come from a glz::json_schema<Args> next to it) and
//               made ready by schema::Normalize; the function gets the struct, already read.
//   AddRaw      the schema is written by hand and the function gets the arguments as JSON. For what has no
//               struct (a tool brought by a Skill) and where the exact JSON matters.
//
// In both, the runtime has already checked the arguments against the schema when the function is called.
class NativeTools final : public ToolSource {
 public:
  void AddRaw(ToolSpec spec, ToolFunction function) {
    entries_.push_back({std::move(spec), std::move(function)});
  }

  template <class Args>
  void Add(std::string name, std::string description, ToolOptions options,
           std::function<boost::asio::awaitable<ToolResult>(const Args&)> handler) {
    constexpr glz::opts kWrite{.error_on_missing_keys = true};
    const auto written = glz::write_json_schema<Args, kWrite>();
    if (!written) throw std::logic_error("cannot write the schema of the arguments of " + name);
    ToolSpec spec;
    spec.name = std::move(name);
    spec.description = std::move(description);
    spec.parameters = schema::Normalize(json::Parse(*written));
    spec.effect = options.effect;
    spec.timeout = options.timeout;
    AddRaw(std::move(spec), [handler = std::move(handler)](const json::Value& arguments) -> boost::asio::awaitable<ToolResult> {
      Args args{};
      const std::string text = json::Dump(arguments);
      constexpr glz::opts kRead{.error_on_unknown_keys = false, .error_on_missing_keys = true};
      if (const auto error = glz::read<kRead>(args, text)) {
        co_return ToolResult{"the arguments could not be read: " + glz::format_error(error, text), true};
      }
      co_return co_await handler(args);
    });
  }

  // A source with only the tools called `names` (those this one has), in the order they were added here.
  // The tools are shared, not copied: they are the same functions. Skills (chapter 1.5) use this to bring
  // some tools into the runtime and take them out again.
  std::shared_ptr<NativeTools> Subset(const std::vector<std::string>& names) const;
  bool Has(const std::string& name) const;
  std::vector<std::string> Names() const;

  std::vector<ToolSpec> Specs() const override;
  boost::asio::awaitable<ToolResult> Call(const std::string& name, const json::Value& arguments) override;

 private:
  struct Entry {
    ToolSpec spec;
    ToolFunction function;
  };
  std::vector<Entry> entries_;  // in the order they were added
};

// The toy tools of chapters 1.3 and 1.4:
//   add(a, b)              the sum of two numbers; whole numbers are added exactly           (AddRaw, read-only)
//   sleep(ms)              waits that long (0 to 60000 ms); a cancellation ends the wait      (Add, read-only)
//   repeat(text, times)    the text written `times` times: a long result, to see it cut       (Add, read-only)
//   shutdown(reason)       pretends to shut the machine down; refused unless dangerous tools  (Add, dangerous)
//                          are allowed
void AddToyTools(NativeTools& tools);

}  // namespace agent
