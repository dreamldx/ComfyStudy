#include "console_hook.h"

#include "json.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <ostream>

namespace agent {

std::string Trimmed(const std::string& text) {
  const char* const blank = " \t\r\n";
  const std::size_t first = text.find_first_not_of(blank);
  if (first == std::string::npos) return "";
  return text.substr(first, text.find_last_not_of(blank) - first + 1);
}

boost::asio::awaitable<void> ConsoleHook::OnReplyReceived(const ReplyReceived& event) {
  last_stop_reason_ = event.reply.stop_reason;
  const std::string text = Trimmed(event.reply.Text());
  if (!text.empty()) out_ << text << '\n';
  co_return;
}

boost::asio::awaitable<void> ConsoleHook::OnToolStarted(const ToolStarted& event) {
  const llm::ToolCallBlock& call = event.call;
  out_ << "[tool] " << call.name << ' ' << (call.arguments.is_null() ? call.arguments_text : json::Dump(call.arguments)) << '\n';
  co_return;
}

boost::asio::awaitable<void> ConsoleHook::OnToolFinished(const ToolFinished& event) {
  out_ << "[tool] " << event.call.name << " -> " << (event.result.is_error ? "error: " : "") << event.result.text << '\n';
  co_return;
}

boost::asio::awaitable<void> ConsoleHook::OnError(const ErrorInfo& event) {
  err_ << "error: " << event.what << '\n';
  co_return;
}

boost::asio::awaitable<void> ConsoleHook::OnTurnFinished(const TurnFinished& event) {
  const TurnOutcome& outcome = event.outcome;
  switch (outcome.reason) {
    case TurnOutcome::Reason::kDone:
      spdlog::info("stopped because: {}; tokens in/out: {}/{}", last_stop_reason_, outcome.input_tokens.value_or(-1),
                   outcome.output_tokens.value_or(-1));
      break;
    case TurnOutcome::Reason::kNoText:
      err_ << "the model returned no text (stopped because: " << last_stop_reason_ << ")\n";
      break;
    case TurnOutcome::Reason::kCancelled:
      err_ << "stopped: cancelled\n";
      break;
    case TurnOutcome::Reason::kTimedOut:
      err_ << "stopped: timed out after " << fmt::format("{}", limits_.deadline.count() / 1000.0) << "s\n";
      break;
    case TurnOutcome::Reason::kMaxSteps:
      err_ << "stopped: step limit (" << limits_.max_steps << ")\n";
      break;
    case TurnOutcome::Reason::kFailed:
      break;  // OnError has said what went wrong
  }
  co_return;
}

}  // namespace agent
