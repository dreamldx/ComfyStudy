#include "provider.h"

#include <array>

namespace llm {
namespace {

const std::string& Setting(const config::Values& settings, const char* name) {
  static const std::string empty;
  const auto found = settings.find(name);
  return found == settings.end() ? empty : found->second;
}

const std::string& Required(const config::Values& settings, const char* name) {
  const std::string& value = Setting(settings, name);
  if (value.empty()) {
    throw Error(std::string("setting \"") + name + "\" is not set (config file, environment variable " +
                config::EnvName(name) + " or --" + name + ")");
  }
  return value;
}

constexpr std::array<const char*, 3> kNames = {"openai-chat", "openai-responses", "anthropic"};

Capabilities DefaultCapabilities(Protocol protocol) {
  switch (protocol) {
    case Protocol::kOpenAIChat:
      return {false, 16384, 128000};  // Chat Completions returns no reasoning items
    case Protocol::kOpenAIResponses:
      return {true, 16384, 128000};
    case Protocol::kAnthropic:
      return {true, 8192, 200000};
  }
  return {false, 1, 1};
}

}  // namespace

Provider MakeProvider(const config::Values& settings, const std::string& name) {
  Provider provider;
  provider.name = name;
  if (name == "openai-chat" || name == "openai-responses") {
    provider.protocol = name == "openai-chat" ? Protocol::kOpenAIChat : Protocol::kOpenAIResponses;
    provider.model = Required(settings, "openai_model");
    provider.endpoint.base_url = Required(settings, "openai_base_url");
    provider.endpoint.api_key = Required(settings, "openai_api_key");
  } else if (name == "anthropic") {
    provider.protocol = Protocol::kAnthropic;
    provider.model = Required(settings, "anthropic_model");
    provider.endpoint.base_url = Required(settings, "anthropic_base_url");
    provider.endpoint.api_key = Required(settings, "anthropic_api_key");
  } else {
    throw Error("unknown provider \"" + name + "\"; the names are openai-chat, openai-responses and anthropic");
  }
  provider.capabilities = DefaultCapabilities(provider.protocol);
  return provider;
}

std::vector<Provider> Providers(const config::Values& settings) {
  std::vector<Provider> providers;
  for (const char* name : kNames) {
    try {
      providers.push_back(MakeProvider(settings, name));
    } catch (const Error&) {
      // its settings are incomplete: it is not available
    }
  }
  return providers;
}

std::string DefaultProviderName(const config::Values& settings) {
  const std::string& provider = Setting(settings, "llm_provider");
  if (provider == "anthropic") return "anthropic";
  if (provider != "openai") {
    throw Error("llm_provider must be \"openai\" or \"anthropic\", not \"" + provider + "\"");
  }
  const std::string& api = Setting(settings, "openai_api");
  if (api.empty() || api == "chat") return "openai-chat";  // Chat Completions is the default
  if (api == "responses") return "openai-responses";
  throw Error("openai_api must be \"chat\" or \"responses\", not \"" + api + "\"");
}

Options OptionsFromSettings(const config::Values& settings) {
  Options options;
  const std::string& level = Setting(settings, "llm_reasoning");
  if (level.empty() || level == "off") {
    options.reasoning = Reasoning::kOff;
  } else if (level == "low") {
    options.reasoning = Reasoning::kLow;
  } else if (level == "medium") {
    options.reasoning = Reasoning::kMedium;
  } else if (level == "high") {
    options.reasoning = Reasoning::kHigh;
  } else {
    throw Error("llm_reasoning must be off, low, medium or high, not \"" + level + "\"");
  }
  const std::string& cache = Setting(settings, "prompt_cache");
  if (cache.empty() || cache == "on") {
    options.cache = true;
  } else if (cache == "off") {
    options.cache = false;
  } else {
    throw Error("prompt_cache must be on or off, not \"" + cache + "\"");
  }
  return options;
}

}  // namespace llm
