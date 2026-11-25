#pragma once

#include <string>
#include <vector>

#include "conversation.h"
#include "http_client.h"
#include "llm_error.h"
#include "provider.h"

// What goes over the wire to an LLM service, in the three request formats we speak:
//   openai_chat       OpenAI "Chat Completions"   POST {base}/chat/completions
//   openai_responses  OpenAI "Responses"          POST {base}/responses
//   anthropic         Anthropic "Messages"        POST {base}/v1/messages
// Each namespace has the same three functions and keeps its protocol's own shape; nothing is
// shared between them on purpose, so the differences stay visible. Generate (llm_client.h) puts
// one function in front of them.
//
// BuildRequest turns a whole conversation into one request. Reasoning blocks that came from the
// same protocol are put back exactly as they were received; reasoning blocks that came from another
// protocol are left out (they are only readable by their own protocol); a message with nothing
// left to send is left out. Text that is not valid UTF-8 is an Error.
namespace llm {

// A ready-to-send HTTP request.
struct HttpRequest {
  std::string url;
  std::vector<http::Header> headers;
  std::string body;
};

namespace openai_chat {
HttpRequest BuildRequest(const Provider& provider, const Conversation& conversation, const Options& options);
Reply ParseReply(const std::string& body);       // throws Error when it is not a valid answer
std::string ErrorText(const std::string& body);  // "type: message" of an error body
}  // namespace openai_chat

namespace openai_responses {
HttpRequest BuildRequest(const Provider& provider, const Conversation& conversation, const Options& options);
Reply ParseReply(const std::string& body);
std::string ErrorText(const std::string& body);
}  // namespace openai_responses

namespace anthropic {
HttpRequest BuildRequest(const Provider& provider, const Conversation& conversation, const Options& options);
Reply ParseReply(const std::string& body);
std::string ErrorText(const std::string& body);
}  // namespace anthropic

}  // namespace llm
