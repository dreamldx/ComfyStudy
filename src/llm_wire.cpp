#include "llm_wire.h"

#include <algorithm>

#include "json.h"

namespace llm {
namespace {

// "https://host/v1/" -> "https://host/v1"
std::string WithoutTrailingSlash(std::string url) {
  while (!url.empty() && url.back() == '/') url.pop_back();
  return url;
}

json::Value ParseJson(const std::string& body) {
  try {
    return json::Parse(body);
  } catch (const json::ParseError& e) {
    throw Error(std::string("the answer is not JSON: ") + e.what());
  }
}

// A string member, or "" when it is missing or not a string (for example JSON null).
std::string String(const json::Value& object, const char* name) {
  const json::Value* found = json::Find(object, name);
  return found != nullptr && found->is_string() ? found->get<std::string>() : std::string();
}

std::optional<long> Number(const json::Value& object, const char* name) {
  const json::Value* found = json::Find(object, name);
  if (found == nullptr) return std::nullopt;
  const std::optional<std::int64_t> number = json::AsInt64(*found);
  if (!number) return std::nullopt;
  return static_cast<long>(*number);
}

// The error bodies of the three services look alike: {"error": {"type": ..., "message": ...}}.
// OpenAI sometimes has no "type" (only "code"), and Anthropic wraps it in {"type": "error", ...}.
std::string ErrorFromBody(const std::string& body) {
  try {
    const json::Value root = json::Parse(body);
    const json::Value* error = json::Find(root, "error");
    if (error != nullptr && error->is_object()) {
      std::string type = String(*error, "type");
      if (type.empty()) type = String(*error, "code");
      const std::string message = String(*error, "message");
      if (!message.empty()) return type.empty() ? message : type + ": " + message;
    }
  } catch (const json::ParseError&) {
    // not JSON: fall through and show the start of the body
  }
  return body.substr(0, 200);
}

// Fails with a clear message when `root` is not a JSON object.
const json::Value& Object(const json::Value& root) {
  if (!root.is_object()) throw Error("the answer is not a JSON object");
  return root;
}

}  // namespace

// --- OpenAI Chat Completions ---------------------------------------------------------------------
namespace openai_chat {

HttpRequest BuildRequest(const Endpoint& endpoint, const Request& request) {
  json::Value messages = json::Array();
  // The instruction is a message of its own, in front of the user's. ("developer" is the newer
  // name; "system" is accepted by every model and by most OpenAI-compatible servers.)
  if (!request.system.empty()) json::Push(messages, {{"role", "system"}, {"content", request.system}});
  json::Push(messages, {{"role", "user"}, {"content", request.prompt}});

  json::Value body = {{"model", request.model},
               {"messages", messages},
               {"max_completion_tokens", request.max_tokens}};
  return HttpRequest{WithoutTrailingSlash(endpoint.base_url) + "/chat/completions",
                     {{"Authorization", "Bearer " + endpoint.api_key},
                      {"Content-Type", "application/json"}},
                     json::Dump(body)};
}

Reply ParseReply(const std::string& body) {
  const json::Value root = ParseJson(body);
  const json::Value& object = Object(root);
  const json::Value* choices = json::Find(object, "choices");
  if (choices == nullptr || !choices->is_array() || choices->empty()) {
    throw Error("the answer has no \"choices\"");
  }
  const json::Value& first = (*choices)[0];
  Reply reply;
  reply.stop_reason = String(first, "finish_reason");
  const json::Value* message = json::Find(first, "message");
  if (message != nullptr && message->is_object()) reply.text = String(*message, "content");
  const json::Value* usage = json::Find(object, "usage");
  if (usage != nullptr && usage->is_object()) {
    reply.input_tokens = Number(*usage, "prompt_tokens");
    reply.output_tokens = Number(*usage, "completion_tokens");
  }
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace openai_chat

// --- OpenAI Responses ----------------------------------------------------------------------------
namespace openai_responses {

HttpRequest BuildRequest(const Endpoint& endpoint, const Request& request) {
  json::Value body = {{"model", request.model},
               {"input", request.prompt},
               // the API does not accept fewer than 16 output tokens
               {"max_output_tokens", std::max(16, request.max_tokens)}};
  if (!request.system.empty()) body["instructions"] = request.system;  // a field of its own
  return HttpRequest{WithoutTrailingSlash(endpoint.base_url) + "/responses",
                     {{"Authorization", "Bearer " + endpoint.api_key},
                      {"Content-Type", "application/json"}},
                     json::Dump(body)};
}

Reply ParseReply(const std::string& body) {
  const json::Value root = ParseJson(body);
  const json::Value& object = Object(root);
  const json::Value* output = json::Find(object, "output");
  if (output == nullptr || !output->is_array()) throw Error("the answer has no \"output\"");

  Reply reply;
  // "output" is a list of typed items; a reasoning model puts "reasoning" items in front of the
  // "message". The text is in the "output_text" parts of the message items.
  for (const json::Value& item : json::Items(*output)) {
    if (String(item, "type") != "message") continue;
    const json::Value* content = json::Find(item, "content");
    if (content == nullptr || !content->is_array()) continue;
    for (const json::Value& part : json::Items(*content)) {
      if (String(part, "type") == "output_text") reply.text += String(part, "text");
    }
  }
  reply.stop_reason = String(object, "status");  // "completed", "incomplete", ...
  const json::Value* incomplete = json::Find(object, "incomplete_details");
  if (incomplete != nullptr && incomplete->is_object()) {
    const std::string why = String(*incomplete, "reason");
    if (!why.empty()) reply.stop_reason = why;  // e.g. "max_output_tokens"
  }
  const json::Value* usage = json::Find(object, "usage");
  if (usage != nullptr && usage->is_object()) {
    reply.input_tokens = Number(*usage, "input_tokens");
    reply.output_tokens = Number(*usage, "output_tokens");
  }
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace openai_responses

// --- Anthropic Messages --------------------------------------------------------------------------
namespace anthropic {

HttpRequest BuildRequest(const Endpoint& endpoint, const Request& request) {
  json::Value body = {{"model", request.model},
               {"max_tokens", request.max_tokens},  // required here
               {"messages", json::Array({{{"role", "user"}, {"content", request.prompt}}})}};
  if (!request.system.empty()) body["system"] = request.system;  // top level, not a message
  return HttpRequest{WithoutTrailingSlash(endpoint.base_url) + "/v1/messages",
                     {{"x-api-key", endpoint.api_key},
                      {"anthropic-version", "2023-06-01"},
                      {"Content-Type", "application/json"}},
                     json::Dump(body)};
}

Reply ParseReply(const std::string& body) {
  const json::Value root = ParseJson(body);
  const json::Value& object = Object(root);
  const json::Value* content = json::Find(object, "content");
  if (content == nullptr || !content->is_array()) throw Error("the answer has no \"content\"");

  Reply reply;
  for (const json::Value& block : json::Items(*content)) {  // typed blocks; a "thinking" block may come first
    if (String(block, "type") == "text") reply.text += String(block, "text");
  }
  reply.stop_reason = String(object, "stop_reason");
  const json::Value* usage = json::Find(object, "usage");
  if (usage != nullptr && usage->is_object()) {
    reply.input_tokens = Number(*usage, "input_tokens");
    reply.output_tokens = Number(*usage, "output_tokens");
  }
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace anthropic

}  // namespace llm
