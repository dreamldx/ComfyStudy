#pragma once

#include <boost/asio.hpp>

#include <functional>
#include <string>
#include <vector>

#include "conversation.h"
#include "json.h"

namespace agent {

// What running a tool gave. A failure is a result too (is_error), not an exception: the model is
// told what went wrong and can try again.
struct ToolResult {
  std::string text;
  bool is_error = false;
};

// The two things the loop asks about tools: which ones there are now, and to run this call. The loop
// asks for the definitions at every Step, so the set of tools may change in the middle of a Turn.
// Run never throws for a failing tool (see ToolResult); only a cancellation goes through as an
// exception.
class ToolSet {
 public:
  virtual ~ToolSet() = default;
  virtual std::vector<llm::ToolDefinition> Definitions() const = 0;
  virtual boost::asio::awaitable<ToolResult> Run(const llm::ToolCallBlock& call) = 0;
};

using ToolFunction = std::function<boost::asio::awaitable<ToolResult>(const json::Value& arguments)>;

// A table of hand-written tools. A tool called by a name that is not in it, a call whose arguments
// were not valid JSON, and a tool function that throws all come back as error results.
class ToolTable final : public ToolSet {
 public:
  void Add(llm::ToolDefinition definition, ToolFunction function);

  std::vector<llm::ToolDefinition> Definitions() const override;
  boost::asio::awaitable<ToolResult> Run(const llm::ToolCallBlock& call) override;

 private:
  struct Entry {
    llm::ToolDefinition definition;
    ToolFunction function;
  };
  std::vector<Entry> entries_;  // in the order they were added
};

// The two tools of chapter 1.3:
//   add(a, b)   the sum of two numbers
//   sleep(ms)   waits that long (at most 60000 ms); a cancellation ends the wait at once
void AddToyTools(ToolTable& table);

}  // namespace agent
