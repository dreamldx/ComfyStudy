#pragma once

#include <boost/asio.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "json.h"
#include "tool.h"

// The tool runtime (chapter 1.4): where the tools of every origin (C++ functions, later Skills, MCP
// servers, scripts) are registered, and what happens around a call: the arguments are checked against the
// schema first, the call has a time limit, a result that is too long is cut, every failure comes back as
// a result, and a dangerous tool is not run unless allowed. The loop (agent.h) only knows ToolSet; the
// runtime is the one implementation of it that the program uses.
namespace agent {

// Everything the runtime and the model need to know about one tool.
struct ToolSpec {
  std::string name;
  std::string description;
  json::Value parameters;                  // the JSON Schema of the arguments (an object schema)
  Effect effect = Effect::kSideEffect;     // not assumed to be harmless
  std::chrono::milliseconds timeout{0};    // zero: the runtime's default
};

// Where tools come from. A source says which tools it has now (this may change from one call to the next:
// a Skill that is loaded adds some) and runs a call. The runtime checks the arguments before Call, so a
// source can rely on them fitting the schema it gave. Call may throw: the runtime turns that into an error
// result (a cancellation is the one exception that goes on).
class ToolSource {
 public:
  virtual ~ToolSource() = default;
  virtual std::vector<ToolSpec> Specs() const = 0;
  virtual boost::asio::awaitable<ToolResult> Call(const std::string& name, const json::Value& arguments) = 0;
  // The conversation starts over: forget what belongs to it. Nothing by default.
  virtual void StartOver() {}
};

struct RuntimeOptions {
  std::chrono::milliseconds timeout{30000};  // for a call whose tool has no time limit of its own; zero: none
  std::size_t max_result_bytes = 16384;      // longer results are cut (at the end of a UTF-8 character)
  bool allow_dangerous = false;              // run the tools that are kDangerous
};

// The options from the settings tool_timeout_ms (a whole number of milliseconds, 0 for none; default 30000)
// and tool_result_max_bytes (a whole number, at least 256; default 16384). Throws llm::Error when a value is
// not a number or is out of range.
RuntimeOptions RuntimeOptionsFromSettings(const config::Values& settings);

class ToolRuntime final : public ToolSet {
 public:
  explicit ToolRuntime(RuntimeOptions options = {});

  // Throws std::invalid_argument when the source has a tool whose name another source already has.
  void AddSource(std::shared_ptr<ToolSource> source);

  // Takes a source away together with its tools; false when it was not added. A call of that source that
  // is running goes on (the runtime holds on to the source for as long as the call needs it), so a tool
  // may remove its own source, which is what unloading a Skill from a tool does.
  bool RemoveSource(const std::shared_ptr<ToolSource>& source);

  // Passes StartOver on to every source.
  void StartOver() override;

  // How the tool is classified, or nothing when there is no such tool now.
  std::optional<Effect> EffectOf(const std::string& name) const;

  // The tools of all the sources, in the order the sources were added; when two tools have the same name
  // (a source changed its mind after AddSource) the first one wins. Logs, once for each tool, the schema
  // keywords that are not checked (schema.h).
  std::vector<llm::ToolDefinition> Definitions() const override;

  // In order: find the tool; refuse a call whose arguments were not valid JSON; refuse a dangerous tool
  // (unless allowed); check the arguments against the schema (every problem is listed); call it with a
  // time limit; cut a result that is too long. Never throws, except for a cancellation.
  boost::asio::awaitable<ToolResult> Run(const llm::ToolCallBlock& call) override;

 private:
  struct Found {
    std::shared_ptr<ToolSource> source;
    ToolSpec spec;
  };
  std::optional<Found> Find(const std::string& name) const;

  RuntimeOptions options_;
  std::vector<std::shared_ptr<ToolSource>> sources_;
  mutable std::vector<std::string> warned_;  // tools whose unchecked keywords were already logged
};

}  // namespace agent
