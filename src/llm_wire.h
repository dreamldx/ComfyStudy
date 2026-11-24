#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "http_client.h"

// What goes over the wire to an LLM service, in the three request formats we speak:
//   openai_chat       OpenAI "Chat Completions"   POST {base}/chat/completions
//   openai_responses  OpenAI "Responses"          POST {base}/responses
//   anthropic         Anthropic "Messages"        POST {base}/v1/messages
// Each namespace has the same three functions and keeps its protocol's own shape; nothing is
// shared between them on purpose, so the differences stay visible. (Chapter 1.2 puts one
// interface in front of them.)
namespace llm {

// A message that cannot be understood, or a service that answered with an error.
class Error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// One question for the model: an optional instruction (the "system prompt") and a user text.
struct Request {
  std::string model;
  std::string system;  // may be empty
  std::string prompt;
  int max_tokens = 1024;  // upper limit for the answer, counted in tokens
};

// Where and how to send it.
struct Endpoint {
  std::string base_url;  // as configured, e.g. "https://api.openai.com/v1"
  std::string api_key;
};

// The pieces of an answer that every protocol has in some form.
struct Reply {
  std::string text;         // the generated text ("" if the model produced none)
  std::string stop_reason;  // why it stopped, in the protocol's own words
  std::optional<long> input_tokens;
  std::optional<long> output_tokens;
};

// A ready-to-send HTTP request.
struct HttpRequest {
  std::string url;
  std::vector<http::Header> headers;
  std::string body;
};

namespace openai_chat {
HttpRequest BuildRequest(const Endpoint& endpoint, const Request& request);
Reply ParseReply(const std::string& body);       // throws Error when it is not a valid answer
std::string ErrorText(const std::string& body);  // "type: message" of an error body
}  // namespace openai_chat

namespace openai_responses {
HttpRequest BuildRequest(const Endpoint& endpoint, const Request& request);
Reply ParseReply(const std::string& body);
std::string ErrorText(const std::string& body);
}  // namespace openai_responses

namespace anthropic {
HttpRequest BuildRequest(const Endpoint& endpoint, const Request& request);
Reply ParseReply(const std::string& body);
std::string ErrorText(const std::string& body);
}  // namespace anthropic

}  // namespace llm
