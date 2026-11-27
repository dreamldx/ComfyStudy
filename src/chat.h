#pragma once

#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "agent.h"
#include "context.h"
#include "conversation.h"
#include "history.h"
#include "provider.h"
#include "transport.h"

namespace llm {

// What the chat adds to each request besides the history (chapter 1.6, context.h). The defaults add nothing:
// the instruction is the only system text, there is no state, and the window of the provider sets the limit.
struct ContextSettings {
  std::vector<agent::Section> system_extra;            // after the instruction: the rules, the list of Skills
  std::function<std::vector<agent::Section>()> state;  // asked at every Step
  std::size_t budget_tokens = 0;                       // 0: what the provider's window leaves (DefaultBudget)
  agent::TokenCounter count;                           // empty: EstimateTokens
  std::shared_ptr<agent::History> history;             // empty: in memory
  std::string channel = "cli";
};

// A conversation on the command line: reads a line, runs a Turn for it (see agent.h) with the current
// provider, and goes on until the input ends or the line "/exit". The history is a History (in memory unless
// `context` brings another), loaded before every Turn and added to after a Turn that ended with an answer.
// Lines that start with "/" are commands, not questions:
//   /use <name>   switch to another provider (the history stays; reasoning blocks that another
//                 protocol left in it are not sent, and /use says how many there are)
//   /reset        forget the history of the channel (and tell `tools` to start over: the Skills that were loaded are unloaded)
//   /exit         end the chat
//   /help         list the commands
// Blank lines are ignored. What a Turn shows is printed by a ConsoleHook: answers and tool lines on
// `out`, errors and the reason a Turn was stopped on `err`. A Turn that does not end with an answer
// (failed, no text, cancelled, timed out, too many Steps) is forgotten: the history is as before the
// question, so the same question can be asked again.
// `tools` (may be null) is offered to the model at every Step. While a Turn runs, Ctrl+C cancels that
// Turn and the chat goes on; between Turns it keeps its usual meaning. `extra_hooks` are told about every
// Turn after the one that prints (chapter 1.5: the hook of the Skills). Throws Error when `start_name`
// is not the name of one of `providers`.
void RunChat(http::Transport& transport, const std::vector<Provider>& providers, const std::string& start_name,
             const Options& options, const std::string& system, agent::ToolSet* tools, const agent::TurnLimits& limits,
             std::istream& in, std::ostream& out, std::ostream& err, const agent::Hooks& extra_hooks = {},
             const ContextSettings& context = {});

// On Windows, makes a console read and write UTF-8, so that Chinese text arrives as valid UTF-8.
// Does nothing when the input is redirected, and on other systems.
void UseUtf8Console();

}  // namespace llm
