#pragma once

#include <boost/asio.hpp>

#include <string>
#include <vector>

#include "conversation.h"
#include "json.h"

namespace agent {

// What a tool does to the world. Not assumed to be harmless: a tool that says nothing is kSideEffect.
//   kReadOnly    only looks (a sum, a search, a file read); several can run at the same time and a failed
//                one can be tried again
//   kSideEffect  changes something (writes a file, sends a message)
//   kDangerous   what cannot be undone (deletes, pays, shuts down); the runtime refuses to run it unless
//                it was told to allow it
enum class Effect { kReadOnly, kSideEffect, kDangerous };

const char* EffectName(Effect effect);  // "read-only", "side-effect", "dangerous"

// What running a tool gave. A failure is a result too (is_error), not an exception: the model is
// told what went wrong and can try again. `truncated` and `effect` are filled in by the runtime (chapter
// 1.4): the text was cut because it was too long, and how the tool is classified.
struct ToolResult {
  std::string text;
  bool is_error = false;
  bool truncated = false;
  Effect effect = Effect::kSideEffect;
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
  // The conversation starts over (/reset): whatever the tools remember about it is forgotten (chapter 1.5:
  // the Skills that were loaded). Nothing by default.
  virtual void StartOver() {}
};

}  // namespace agent
