#pragma once

#include <iosfwd>
#include <string>

#include "agent.h"

namespace agent {

// The text without blank space (spaces, tabs, line breaks) at either end. Some services begin an answer
// with blank lines; the history keeps the text as it came, the screen does not show them.
std::string Trimmed(const std::string& text);

// The command line's way to look at a Turn:
//   an answer          its text, on `out` (nothing when it has no text)
//   a tool call        [tool] add {"a":2,"b":3}   then   [tool] add -> 5   (or  -> error: ...)
//   a Turn that is not done: why, on `err` ("stopped: cancelled", "stopped: timed out after 2s",
//                      "stopped: step limit (10)", or the error itself)
//   a Turn that is done: the log line "stopped because: ...; tokens in/out: ..." (all Steps added up)
class ConsoleHook final : public AgentHook {
 public:
  ConsoleHook(std::ostream& out, std::ostream& err, const TurnLimits& limits) : out_(out), err_(err), limits_(limits) {}

  boost::asio::awaitable<void> OnReplyReceived(const ReplyReceived& event) override;
  boost::asio::awaitable<void> OnToolStarted(const ToolStarted& event) override;
  boost::asio::awaitable<void> OnToolFinished(const ToolFinished& event) override;
  boost::asio::awaitable<void> OnTurnFinished(const TurnFinished& event) override;
  boost::asio::awaitable<void> OnError(const ErrorInfo& event) override;

 private:
  std::ostream& out_;
  std::ostream& err_;
  TurnLimits limits_;
  std::string last_stop_reason_;  // of the last answer, for the line at the end
};

}  // namespace agent
