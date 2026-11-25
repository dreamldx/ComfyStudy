#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "conversation.h"
#include "provider.h"
#include "transport.h"

namespace llm {

// A conversation on the command line: reads a line, sends the whole history to the current
// provider, prints the answer, and goes on until the input ends or the line "/exit".
// The history lives in memory only. Lines that start with "/" are commands, not questions:
//   /use <name>   switch to another provider (the history stays; reasoning blocks that another
//                 protocol left in it are not sent, and /use says how many there are)
//   /reset        forget the history
//   /exit         end the chat
//   /help         list the commands
// Blank lines are ignored. A request that fails, or an answer without any text, is reported on
// `err` and the question is taken out of the history again, so the roles keep alternating.
// Answers and messages go to `out`, errors to `err`. Throws Error when `start_name` is not the
// name of one of `providers`.
void RunChat(http::Transport& transport, const std::vector<Provider>& providers, const std::string& start_name,
             const Options& options, const std::string& system, std::istream& in, std::ostream& out,
             std::ostream& err);

// On Windows, makes a console read and write UTF-8, so that Chinese text arrives as valid UTF-8.
// Does nothing when the input is redirected, and on other systems.
void UseUtf8Console();

}  // namespace llm
