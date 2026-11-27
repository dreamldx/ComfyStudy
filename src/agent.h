#pragma once

#include <boost/asio.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "agent_hook.h"
#include "config.h"
#include "context.h"
#include "conversation.h"
#include "provider.h"
#include "tool.h"
#include "transport.h"

namespace agent {

// A Turn is everything that happens for one user message, until the final answer (or until it is
// stopped). It is made of Steps: one Step is a call of the model plus the running of the tools the
// model asked for in that call.
struct TurnLimits {
  int max_steps = 10;                     // Steps in one Turn
  std::chrono::milliseconds deadline{0};  // for the whole Turn; zero means no deadline
};

// The limits from the settings agent_max_steps (a whole number, at least 1; default 10) and
// agent_turn_timeout_ms (a whole number of milliseconds, 0 for none; default 300000). Throws llm::Error
// when a value is not a number or is out of range.
TurnLimits LimitsFromSettings(const config::Values& settings);

// How to stop a Turn from outside. Bind slot() when starting the Turn (co_spawn with
// bind_cancellation_slot), then call Cancel() from a handler of the same io_context: Ctrl+C in the
// command line, /stop in Discord. It is not thread safe.
class TurnControl {
 public:
  void Cancel() { signal_.emit(boost::asio::cancellation_type::terminal); }
  boost::asio::cancellation_slot slot() { return signal_.slot(); }

 private:
  boost::asio::cancellation_signal signal_;
};

// What goes in front of a message that came in while the task was running (see take_new_messages).
// It never changes, so that the start of the history stays the same and a prompt cache can hit.
extern const char* const kSteeringMarker;

using Hooks = std::vector<std::shared_ptr<AgentHook>>;

// How a Turn puts together what it sends (chapter 1.6, context.h). Without a plan the request is the
// conversation as it is. With one, every Step builds its context from the conversation: the messages
// before the user's message are the history, the user's message is the latest, and what came after it is
// this Turn's own; the sections below are asked for at every Step, so what they say is what is true at that
// Step (the Skills that are loaded, say). The conversation itself is not changed: the state is not stored.
struct ContextPlan {
  std::function<std::vector<Section>()> system;  // empty: Conversation::system alone, as one section
  std::function<std::vector<Section>()> state;   // empty: no state
  std::size_t budget_tokens = 0;                 // 0: no limit
  TokenCounter count;                            // empty: EstimateTokens
};

// Runs one Turn. The last message of `conversation` is the user's message that starts it.
//
// Each Step offers the model the tools `tools->Definitions()` has at that moment (none when `tools`
// is null), and the answer is added to the conversation. If it asks for tools they are run in order and
// their results go back in one message; then the next Step starts. The Turn is done when an answer asks
// for nothing. `take_new_messages` (may be empty) is asked at the start of every Step but the first: what
// it returns is put, one text block each after the marker, at the end of the message with the results.
//
// Unless the Turn ends with kDone, the conversation is cut back to what it was before the user's
// message. A cancellation (see TurnControl) and the deadline stop the request in flight and a tool that
// is running; OnTurnFinished is called in every case. The arguments are taken by reference: they must
// stay alive until the coroutine has finished.
boost::asio::awaitable<TurnOutcome> RunTurn(http::Transport& transport, const llm::Provider& provider,
                                            llm::Conversation& conversation, const llm::Options& options,
                                            ToolSet* tools, const Hooks& hooks, const TurnLimits& limits,
                                            std::function<std::vector<std::string>()> take_new_messages,
                                            const ContextPlan* plan = nullptr);

}  // namespace agent
