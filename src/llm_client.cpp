#include "llm_client.h"

#include <spdlog/spdlog.h>

#include <string>

namespace llm {

boost::asio::awaitable<Reply> Generate(http::Transport& transport, const Provider& provider,
                                       const Conversation& conversation, const Options& options) {
  HttpRequest (*build)(const Provider&, const Conversation&, const Options&) = nullptr;
  Reply (*parse)(const std::string&) = nullptr;
  std::string (*error_text)(const std::string&) = nullptr;
  switch (provider.protocol) {
    case Protocol::kOpenAIChat:
      build = openai_chat::BuildRequest;
      parse = openai_chat::ParseReply;
      error_text = openai_chat::ErrorText;
      break;
    case Protocol::kOpenAIResponses:
      build = openai_responses::BuildRequest;
      parse = openai_responses::ParseReply;
      error_text = openai_responses::ErrorText;
      break;
    case Protocol::kAnthropic:
      build = anthropic::BuildRequest;
      parse = anthropic::ParseReply;
      error_text = anthropic::ErrorText;
      break;
  }

  const HttpRequest http_request = build(provider, conversation, options);
  // The headers carry the API key: they are logged only with the secrets blanked out. The bodies
  // hold just the conversation and the answer.
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

boost::asio::awaitable<Reply> Ask(http::Transport& transport, const config::Values& settings,
                                  const std::string& prompt, const std::string& system) {
  const Provider provider = MakeProvider(settings, DefaultProviderName(settings));
  const Options options = OptionsFromSettings(settings);
  Conversation conversation;
  conversation.system = system;
  conversation.messages.push_back({Role::kUser, {TextBlock{prompt}}});
  co_return co_await Generate(transport, provider, conversation, options);
}

}  // namespace llm
