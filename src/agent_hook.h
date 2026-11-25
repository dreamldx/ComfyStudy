#pragma once

#include <boost/asio.hpp>

#include <chrono>
#include <optional>
#include <string>

#include "conversation.h"
#include "tool.h"

// Hooks: the places where the loop tells the outside what is happening. The command line prints
// through a hook; Discord (chapter 1.7), streaming and the call recording (chapter 2.6) will be
// hooks too, so none of them is written into the loop.
//
// In chapter 1.3 a hook only looks: every context is passed as const&. Later (quest llm-1.1) the
// contexts become changeable at some places, and a hook can refuse or rewrite. That is why the methods
// are coroutines already: a hook may have to wait (for an approval, say).
namespace agent {

// How a Turn ended. Anything but kDone means the Turn was forgotten (the conversation is back to what
// it was before the user's message).
struct TurnOutcome {
  enum class Reason {
    kDone,       // the model answered without asking for a tool
    kMaxSteps,   // it kept asking: the limit on Steps was reached
    kCancelled,  // TurnControl::Cancel() was called (Ctrl+C, /stop)
    kTimedOut,   // the deadline of the Turn passed
    kFailed,     // a request failed or the answer could not be understood: see `error`
    kNoText,     // the last answer had no text in it
  };
  Reason reason = Reason::kDone;
  int steps = 0;      // how many Steps were started
  std::string error;  // what went wrong, for kFailed
  std::optional<long> input_tokens;   // added up over the Steps
  std::optional<long> output_tokens;
};

struct TurnStarted {
  std::size_t messages;  // the length of the conversation, the user's message included
};
struct StepStarted {
  int step;  // 1, 2, ...
};
struct ReplyReceived {
  int step;
  const llm::Reply& reply;
};
struct ToolStarted {
  int step;
  const llm::ToolCallBlock& call;
};
struct ToolFinished {
  int step;
  const llm::ToolCallBlock& call;
  const ToolResult& result;
  std::chrono::milliseconds elapsed;
};
struct StepFinished {
  int step;
};
struct TurnFinished {
  const TurnOutcome& outcome;
};
struct ErrorInfo {
  const std::string& what;
};

// Every method does nothing unless a hook overrides it. A hook must be quick: the loop waits for it.
// A hook that throws is logged and ignored (a cancellation is the exception: it goes on up).
class AgentHook {
 public:
  virtual ~AgentHook() = default;
  virtual boost::asio::awaitable<void> OnTurnStarted(const TurnStarted&) { co_return; }
  virtual boost::asio::awaitable<void> OnStepStarted(const StepStarted&) { co_return; }
  virtual boost::asio::awaitable<void> OnReplyReceived(const ReplyReceived&) { co_return; }
  virtual boost::asio::awaitable<void> OnToolStarted(const ToolStarted&) { co_return; }
  virtual boost::asio::awaitable<void> OnToolFinished(const ToolFinished&) { co_return; }
  virtual boost::asio::awaitable<void> OnStepFinished(const StepFinished&) { co_return; }
  virtual boost::asio::awaitable<void> OnTurnFinished(const TurnFinished&) { co_return; }
  virtual boost::asio::awaitable<void> OnError(const ErrorInfo&) { co_return; }
};

}  // namespace agent
