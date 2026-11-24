#include "llm_client.h"

#include <spdlog/spdlog.h>

#include <string>

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

}  // namespace

Protocol ChooseProtocol(const config::Values& settings) {
  const std::string& provider = Setting(settings, "llm_provider");
  if (provider == "anthropic") return Protocol::kAnthropic;
  if (provider != "openai") {
    throw Error("llm_provider must be \"openai\" or \"anthropic\", not \"" + provider + "\"");
  }
  const std::string& api = Setting(settings, "openai_api");
  if (api.empty() || api == "chat") return Protocol::kOpenAIChat;  // Chat Completions is the default
  if (api == "responses") return Protocol::kOpenAIResponses;
  throw Error("openai_api must be \"chat\" or \"responses\", not \"" + api + "\"");
}

boost::asio::awaitable<Reply> Ask(http::Transport& transport, const config::Values& settings,
                                  const std::string& prompt, const std::string& system) {
  const Protocol protocol = ChooseProtocol(settings);

  Request request;
  request.system = system;
  request.prompt = prompt;

  Endpoint endpoint;
  HttpRequest (*build)(const Endpoint&, const Request&) = nullptr;
  Reply (*parse)(const std::string&) = nullptr;
  std::string (*error_text)(const std::string&) = nullptr;
  switch (protocol) {
    case Protocol::kOpenAIChat:
    case Protocol::kOpenAIResponses:
      request.model = Required(settings, "openai_model");
      endpoint.base_url = Required(settings, "openai_base_url");
      endpoint.api_key = Required(settings, "openai_api_key");
      if (protocol == Protocol::kOpenAIChat) {
        build = openai_chat::BuildRequest;
        parse = openai_chat::ParseReply;
        error_text = openai_chat::ErrorText;
      } else {
        build = openai_responses::BuildRequest;
        parse = openai_responses::ParseReply;
        error_text = openai_responses::ErrorText;
      }
      break;
    case Protocol::kAnthropic:
      request.model = Required(settings, "anthropic_model");
      endpoint.base_url = Required(settings, "anthropic_base_url");
      endpoint.api_key = Required(settings, "anthropic_api_key");
      build = anthropic::BuildRequest;
      parse = anthropic::ParseReply;
      error_text = anthropic::ErrorText;
      break;
  }

  const HttpRequest http_request = build(endpoint, request);
  // The headers carry the API key: they are logged only with the secrets blanked out. The bodies
  // hold just the question and the answer.
  spdlog::debug("POST {}", http_request.url);
  for (const http::Header& header : http::Redacted(http_request.headers)) {
    spdlog::debug("request header {}: {}", header.first, header.second);
  }
  spdlog::debug("request body: {}", http_request.body);
  const http::Response response =
      co_await transport.Post({http_request.url, http_request.headers, http_request.body});
  spdlog::debug("HTTP {}", response.status);
  spdlog::debug("response body: {}", response.body);

  if (response.status < 200 || response.status >= 300) {
    std::string message = "the service answered HTTP " + std::to_string(response.status) + ": " +
                          error_text(response.body);
    if (response.status == 401) message += " (check the API key)";
    throw Error(message);
  }
  co_return parse(response.body);
}

}  // namespace llm
