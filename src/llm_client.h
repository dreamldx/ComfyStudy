#pragma once

#include <boost/asio.hpp>

#include <string>

#include "config.h"
#include "llm_wire.h"
#include "transport.h"

namespace llm {

// The three ways to talk to a service. The setting `llm_provider` picks the service, and for
// OpenAI the setting `openai_api` picks the request format:
//   llm_provider = openai, openai_api = chat       OpenAI Chat Completions (also what most
//                                                  OpenAI-compatible servers speak)
//   llm_provider = openai, openai_api = responses  OpenAI Responses
//   llm_provider = anthropic                       Anthropic Messages
enum class Protocol { kOpenAIChat, kOpenAIResponses, kAnthropic };

// Throws Error when `llm_provider` or `openai_api` is not one of the values above.
// OpenAI uses Chat Completions unless `openai_api` says "responses"; the setting is not looked at
// when the provider is anthropic.
Protocol ChooseProtocol(const config::Values& settings);

// Sends one question through `transport` and completes with the answer. Which protocol, model, key
// and server to use comes from `settings`: llm_provider and openai_api, then the openai_* or the
// anthropic_* settings (model, api_key, base_url) of the chosen service. `system` may be empty.
// Throws Error when a setting is missing, when the service answers with an error status, or when
// the answer cannot be understood; throws http::Error when no answer arrives. The arguments are
// taken by reference: they must stay alive until the coroutine has finished.
boost::asio::awaitable<Reply> Ask(http::Transport& transport, const config::Values& settings,
                                  const std::string& prompt, const std::string& system);

}  // namespace llm
