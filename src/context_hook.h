#pragma once

#include <iosfwd>

#include "agent_hook.h"

namespace agent {

// --show-context: before every request, a table of what the context is made of and what each part takes
// (the estimate, see EstimateTokens); after the answer, how many input tokens the service says it counted,
// to compare. With `full` the whole context is written out too (FormatContext).
//
//   [context] step 1: about 702 tokens (budget 89999)
//     tools                   2 items,    412 tokens
//     ...
//   [context] the service counted 665 input tokens (about 702 were estimated)
class ContextHook final : public AgentHook {
 public:
  ContextHook(std::ostream& out, bool full) : out_(out), full_(full) {}

  boost::asio::awaitable<void> OnContextBuilt(const ContextBuilt& event) override;
  boost::asio::awaitable<void> OnReplyReceived(const ReplyReceived& event) override;

 private:
  std::ostream& out_;
  bool full_;
  std::size_t estimated_ = 0;  // of the request that is waiting for its answer
};

}  // namespace agent
