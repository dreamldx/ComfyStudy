#pragma once

#include <boost/asio.hpp>

#include <string>

#include "config.h"
#include "conversation.h"
#include "llm_wire.h"
#include "provider.h"
#include "transport.h"

namespace llm {

// Sends `conversation` to `provider` through `transport` and completes with the answer. The
// protocol of the provider decides how the request looks (see llm_wire.h); reasoning blocks from
// another protocol are not sent, and the conversation itself is never changed. To go on, append
// reply.message and the next user message to the conversation.
// Throws Error when the service answers with an error status or the answer cannot be understood
// (the message has the service's own error text), and http::Error when no answer arrives. The
// arguments are taken by reference: they must stay alive until the coroutine has finished.
boost::asio::awaitable<Reply> Generate(http::Transport& transport, const Provider& provider,
                                       const Conversation& conversation, const Options& options);

// One question, one answer: the provider named by `llm_provider` and `openai_api`, the options
// from `llm_reasoning`, a conversation of a single user message. `system` may be empty. Throws
// Error as well when a setting is missing (the message names it).
boost::asio::awaitable<Reply> Ask(http::Transport& transport, const config::Values& settings,
                                  const std::string& prompt, const std::string& system);

}  // namespace llm
