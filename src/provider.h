#pragma once

#include <string>
#include <vector>

#include "config.h"
#include "conversation.h"
#include "llm_error.h"
#include "protocol.h"

namespace llm {

// Where to send a request and what to prove it with.
struct Endpoint {
  std::string base_url;  // as configured, e.g. "https://api.openai.com/v1"
  std::string api_key;
};

// What a service can do, so that the program picks a path by asking instead of by checking names.
// The numbers are conservative defaults for each protocol (what any current model of that API
// accepts); they are not read from the service.
struct Capabilities {
  bool keeps_reasoning_blocks;  // does the protocol return reasoning blocks that can be sent back?
  int max_output_tokens;
  int context_window;  // in tokens
};

// A configured target: a protocol, an address, a model. Just data; what happens to a request is
// decided by Generate (llm_client.h), which takes a Provider as an argument.
struct Provider {
  std::string name;  // "openai-chat", "openai-responses" or "anthropic"
  Protocol protocol;
  Endpoint endpoint;
  std::string model;
  Capabilities capabilities;
};

// The provider called `name`, from the settings openai_* or anthropic_*. Throws Error when `name`
// is not one of the three names, or when a setting it needs is missing.
Provider MakeProvider(const config::Values& settings, const std::string& name);

// Every provider whose settings are complete, in the fixed order openai-chat, openai-responses,
// anthropic. ("openai-chat" and "openai-responses" use the same openai_* settings.)
std::vector<Provider> Providers(const config::Values& settings);

// The name chosen by the settings `llm_provider` and `openai_api`. OpenAI uses Chat Completions
// unless `openai_api` says "responses"; that setting is not looked at when the provider is
// anthropic. Throws Error when either has a value that is not allowed.
std::string DefaultProviderName(const config::Values& settings);

// The request options from the settings: llm_reasoning (off, low, medium or high; empty means
// off). Throws Error for any other value.
Options OptionsFromSettings(const config::Values& settings);

}  // namespace llm
