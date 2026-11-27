#include "agent.h"

#include <boost/asio/experimental/awaitable_operators.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <stdexcept>

#include "llm_client.h"

namespace agent {

const char* const kSteeringMarker = "[New message from the user, received while the task was running]";

namespace {

using boost::asio::experimental::awaitable_operators::operator||;
using Reason = TurnOutcome::Reason;

bool IsCancellation(const boost::system::system_error& e) { return e.code() == boost::asio::error::operation_aborted; }

// Tells every hook about one thing, in the order they were registered. A hook that fails is logged and
// the others go on; a cancellation is not a failure of the hook and goes up.
template <class Event>
boost::asio::awaitable<void> Notify(const Hooks& hooks, boost::asio::awaitable<void> (AgentHook::*method)(const Event&),
                                    const Event& event) {
  for (const std::shared_ptr<AgentHook>& hook : hooks) {
    try {
      co_await ((*hook).*method)(event);
    } catch (const boost::system::system_error& e) {
      if (IsCancellation(e)) throw;
      spdlog::warn("a hook failed: {}", e.what());
    } catch (const std::exception& e) {
      spdlog::warn("a hook failed: {}", e.what());
    }
  }
}

// The two places where the loop leaves its own code: calling the model, and running a tool. Each has
// exactly one caller, so that something can be put around them later (quest llm-1.1: retries, a
// fallback provider, asking before a tool runs) without touching the loop.
boost::asio::awaitable<llm::Reply> CallModel(http::Transport& transport, const llm::Provider& provider,
                                             const llm::Conversation& conversation, const llm::Options& options) {
  co_return co_await llm::Generate(transport, provider, conversation, options);
}

boost::asio::awaitable<ToolResult> ExecuteTool(ToolSet* tools, const llm::ToolCallBlock& call) {
  if (tools == nullptr) co_return ToolResult{"no tools are available", true};
  try {
    co_return co_await tools->Run(call);
  } catch (const boost::system::system_error& e) {
    if (IsCancellation(e)) throw;
    co_return ToolResult{std::string("the tool failed: ") + e.what(), true};
  } catch (const std::exception& e) {
    co_return ToolResult{std::string("the tool failed: ") + e.what(), true};
  }
}

boost::asio::awaitable<void> Deadline(std::chrono::milliseconds limit) {
  boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, limit);
  co_await timer.async_wait(boost::asio::use_awaitable);
}

void AddTokens(std::optional<long>& total, const std::optional<long>& more) {
  if (more) total = total.value_or(0) + *more;
}

// What the Steps need to know; the references are the ones RunTurn was given.
struct Run {
  http::Transport& transport;
  const llm::Provider& provider;
  llm::Conversation& conversation;
  const llm::Options& options;
  ToolSet* tools;
  const Hooks& hooks;
  const TurnLimits& limits;
  const std::function<std::vector<std::string>()>& take_new_messages;
  const ContextPlan* plan;
  std::size_t turn_start;  // the index of the user's message that started the Turn
  TurnOutcome& outcome;
};

// The context of this Step, built from the conversation as it is now.
Context BuildStepContext(const Run& run, const llm::Options& options) {
  const llm::Conversation& conversation = run.conversation;
  ContextInputs inputs;
  inputs.tools = options.tools;
  inputs.system = run.plan->system ? run.plan->system() : std::vector<Section>{{"", conversation.system}};
  if (run.plan->state) inputs.state = run.plan->state();
  inputs.history.assign(conversation.messages.begin(), conversation.messages.begin() + static_cast<std::ptrdiff_t>(run.turn_start));
  inputs.latest = conversation.messages[run.turn_start];
  inputs.turn.assign(conversation.messages.begin() + static_cast<std::ptrdiff_t>(run.turn_start) + 1, conversation.messages.end());
  inputs.budget_tokens = run.plan->budget_tokens;
  inputs.count = run.plan->count;
  return BuildContext(inputs);
}

// The Steps of a Turn, one after the other. Writes how it ended into run.outcome. Only a cancellation
// leaves it as an exception.
boost::asio::awaitable<void> RunSteps(const Run& run) {
  TurnOutcome& outcome = run.outcome;
  try {
    for (int step = 1; step <= run.limits.max_steps; ++step) {
      outcome.steps = step;

      // News that came in while the last Step ran goes in with its results, after them.
      if (step >= 2 && run.take_new_messages) {
        for (const std::string& text : run.take_new_messages()) {
          run.conversation.messages.back().blocks.push_back(llm::TextBlock{std::string(kSteeringMarker) + "\n" + text});
        }
      }

      co_await Notify(run.hooks, &AgentHook::OnStepStarted, StepStarted{step});

      llm::Options options = run.options;
      if (run.tools != nullptr) options.tools = run.tools->Definitions();  // asked at every Step
      llm::Reply reply;
      if (run.plan != nullptr) {
        const Context context = BuildStepContext(run, options);
        co_await Notify(run.hooks, &AgentHook::OnContextBuilt, ContextBuilt{step, context});
        options.tools = context.tools;
        reply = co_await CallModel(run.transport, run.provider, context.conversation, options);
      } else {
        reply = co_await CallModel(run.transport, run.provider, run.conversation, options);
      }
      AddTokens(outcome.input_tokens, reply.input_tokens);
      AddTokens(outcome.output_tokens, reply.output_tokens);
      co_await Notify(run.hooks, &AgentHook::OnReplyReceived, ReplyReceived{step, reply});
      run.conversation.messages.push_back(reply.message);

      const auto calls = reply.ToolCalls();
      if (calls.empty()) {
        outcome.reason = reply.Text().empty() ? Reason::kNoText : Reason::kDone;
        co_await Notify(run.hooks, &AgentHook::OnStepFinished, StepFinished{step});
        co_return;
      }

      llm::Message results{llm::Role::kUser, {}};
      for (const llm::ToolCallBlock* call : calls) {
        co_await Notify(run.hooks, &AgentHook::OnToolStarted, ToolStarted{step, *call});
        const auto started = std::chrono::steady_clock::now();
        const ToolResult result = co_await ExecuteTool(run.tools, *call);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
        co_await Notify(run.hooks, &AgentHook::OnToolFinished, ToolFinished{step, *call, result, elapsed});
        results.blocks.push_back(llm::ToolResultBlock{call->id, result.text, result.is_error});
      }
      run.conversation.messages.push_back(std::move(results));
      co_await Notify(run.hooks, &AgentHook::OnStepFinished, StepFinished{step});
    }
    outcome.reason = Reason::kMaxSteps;
  } catch (const boost::system::system_error& e) {
    if (IsCancellation(e)) throw;
    outcome.reason = Reason::kFailed;
    outcome.error = e.what();
  } catch (const std::exception& e) {
    outcome.reason = Reason::kFailed;
    outcome.error = e.what();
  }
}

}  // namespace

namespace {

// A whole number from a setting, or `fallback` when the setting is missing or empty. Throws llm::Error
// for text that is not a whole number or is smaller than `least`.
long WholeNumber(const config::Values& settings, const char* name, long fallback, long least) {
  const auto found = settings.find(name);
  if (found == settings.end() || found->second.empty()) return fallback;
  const std::string& text = found->second;
  long value = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || value < least) {
    throw llm::Error(std::string("setting \"") + name + "\" must be a whole number of at least " + std::to_string(least) +
                     ", not \"" + text + "\"");
  }
  return value;
}

}  // namespace

TurnLimits LimitsFromSettings(const config::Values& settings) {
  TurnLimits limits;
  limits.max_steps = static_cast<int>(WholeNumber(settings, "agent_max_steps", 10, 1));
  limits.deadline = std::chrono::milliseconds(WholeNumber(settings, "agent_turn_timeout_ms", 300000, 0));
  return limits;
}

boost::asio::awaitable<TurnOutcome> RunTurn(http::Transport& transport, const llm::Provider& provider,
                                            llm::Conversation& conversation, const llm::Options& options,
                                            ToolSet* tools, const Hooks& hooks, const TurnLimits& limits,
                                            std::function<std::vector<std::string>()> take_new_messages,
                                            const ContextPlan* plan) {
  if (conversation.messages.empty()) {
    throw std::invalid_argument("RunTurn needs the user's message as the last message of the conversation");
  }
  const std::size_t rollback_to = conversation.messages.size() - 1;  // the user's message goes too if the Turn fails

  TurnOutcome outcome;
  const Run run{transport, provider, conversation, options, tools, hooks, limits, take_new_messages, plan, rollback_to, outcome};
  bool cancelled = false;
  try {
    co_await Notify(hooks, &AgentHook::OnTurnStarted, TurnStarted{conversation.messages.size()});
    if (limits.deadline.count() > 0) {
      // Whichever ends first stops the other: the Steps end the timer, or the timer stops the Steps.
      const auto finished = co_await (RunSteps(run) || Deadline(limits.deadline));
      if (finished.index() == 1) outcome.reason = Reason::kTimedOut;
    } else {
      co_await RunSteps(run);
    }
  } catch (const boost::system::system_error& e) {
    if (IsCancellation(e)) {
      cancelled = true;
    } else {
      outcome.reason = Reason::kFailed;
      outcome.error = e.what();
    }
  } catch (const boost::asio::multiple_exceptions& e) {
    // A cancellation from outside reaches the Steps and the timer next to them, and both end with an
    // abort: `||` then reports the two together. They are a cancellation as long as the first is one.
    try {
      std::rethrow_exception(e.first_exception());
    } catch (const boost::system::system_error& first) {
      if (IsCancellation(first)) {
        cancelled = true;
      } else {
        outcome.reason = Reason::kFailed;
        outcome.error = first.what();
      }
    } catch (const std::exception& first) {
      outcome.reason = Reason::kFailed;
      outcome.error = first.what();
    }
  } catch (const std::exception& e) {
    outcome.reason = Reason::kFailed;
    outcome.error = e.what();
  }
  if (cancelled) outcome.reason = Reason::kCancelled;

  // A cancellation has been dealt with. Without this, every co_await below would be cut short by it,
  // and the hooks could not wait for anything.
  co_await boost::asio::this_coro::reset_cancellation_state();

  if (outcome.reason != Reason::kDone) {
    conversation.messages.erase(conversation.messages.begin() + static_cast<std::ptrdiff_t>(rollback_to), conversation.messages.end());
  }
  if (!outcome.error.empty()) co_await Notify(hooks, &AgentHook::OnError, ErrorInfo{outcome.error});
  co_await Notify(hooks, &AgentHook::OnTurnFinished, TurnFinished{outcome});
  co_return outcome;
}

}  // namespace agent
