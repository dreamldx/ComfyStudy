#include "context_hook.h"

#include <iomanip>
#include <ostream>

namespace agent {

boost::asio::awaitable<void> ContextHook::OnContextBuilt(const ContextBuilt& event) {
  const ContextReport& report = event.context.report;
  estimated_ = report.total;
  out_ << "[context] step " << event.step << ": about " << report.total << " tokens";
  if (report.budget > 0) out_ << " (budget " << report.budget << ")";
  out_ << "\n";
  for (const PartReport& part : report.parts) {
    out_ << "  " << std::left << std::setw(24) << part.name << std::right << std::setw(3) << part.items << " items, " << std::setw(6)
         << part.tokens << " tokens\n";
  }
  if (report.dropped_messages > 0) {
    out_ << "  left out to fit the budget: " << report.dropped_messages << " messages (" << report.dropped_turns << " turns)\n";
  }
  if (full_) out_ << FormatContext(event.context);
  out_ << std::flush;
  co_return;
}

boost::asio::awaitable<void> ContextHook::OnReplyReceived(const ReplyReceived& event) {
  if (event.reply.input_tokens) {
    out_ << "[context] the service counted " << *event.reply.input_tokens << " input tokens (about " << estimated_ << " were estimated)";
    if (event.reply.cached_tokens) out_ << "; " << *event.reply.cached_tokens << " read from the cache";
    if (event.reply.cache_written_tokens) out_ << "; " << *event.reply.cache_written_tokens << " written to it";
    out_ << "\n" << std::flush;
  }
  co_return;
}

}  // namespace agent
