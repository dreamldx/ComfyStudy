#include "llm_wire.h"

#include <algorithm>

#include "json.h"
#include <spdlog/spdlog.h>

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

// --- Building requests --------------------------------------------------------------------------

bool IsValidUtf8(const std::string& text) {
  std::size_t i = 0;
  while (i < text.size()) {
    const unsigned char lead = static_cast<unsigned char>(text[i]);
    std::size_t length = 0;
    unsigned int code = 0;
    if (lead < 0x80) {
      ++i;
      continue;
    } else if (lead >= 0xC2 && lead <= 0xDF) {
      length = 2;
      code = lead & 0x1Fu;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
      length = 3;
      code = lead & 0x0Fu;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
      length = 4;
      code = lead & 0x07u;
    } else {
      return false;
    }
    if (i + length > text.size()) return false;
    for (std::size_t k = 1; k < length; ++k) {
      const unsigned char next = static_cast<unsigned char>(text[i + k]);
      if ((next & 0xC0u) != 0x80u) return false;
      code = (code << 6) | (next & 0x3Fu);
    }
    const bool overlong = (length == 3 && code < 0x800) || (length == 4 && code < 0x10000);
    const bool surrogate = code >= 0xD800 && code <= 0xDFFF;
    if (overlong || surrogate || code > 0x10FFFF) return false;
    i += length;
  }
  return true;
}

const std::string& Utf8(const std::string& text) {
  // A NUL byte is valid UTF-8, but no text we send has one: it means the input was not read right.
  if (text.find('\0') != std::string::npos) {
    throw Error("the text contains a NUL byte (the console did not hand over the input correctly)");
  }
  if (!IsValidUtf8(text)) {
    throw Error("the text is not valid UTF-8 (on a Windows console, switch it to UTF-8 first)");
  }
  return text;
}

// All the text blocks of a message, one after the other.
std::string TextOf(const Message& message) {
  std::string text;
  for (const Block& block : message.blocks) {
    if (const auto* part = std::get_if<TextBlock>(&block)) text += Utf8(part->text);
  }
  return text;
}

// The payload of a reasoning block, when it can be sent to a service that speaks `protocol`.
const json::Value* SendablePayload(const Block& block, Protocol protocol) {
  const auto* reasoning = std::get_if<ReasoningBlock>(&block);
  return reasoning != nullptr && reasoning->origin == protocol ? &reasoning->payload : nullptr;
}

const char* RoleName(Role role) { return role == Role::kUser ? "user" : "assistant"; }

const char* EffortName(Reasoning reasoning) {
  switch (reasoning) {
    case Reasoning::kLow:
      return "low";
    case Reasoning::kMedium:
      return "medium";
    case Reasoning::kHigh:
      return "high";
    case Reasoning::kOff:
      break;
  }
  return "";
}

std::vector<http::Header> JsonHeaders(std::vector<http::Header> headers) {
  headers.push_back({"Content-Type", "application/json"});
  return headers;
}

Reply NewReply() {
  Reply reply;
  reply.message.role = Role::kAssistant;
  return reply;
}

void ReadUsage(const json::Value& object, Reply& reply, const char* input_name, const char* output_name) {
  const json::Value* usage = json::Find(object, "usage");
  if (usage != nullptr && usage->is_object()) {
    reply.input_tokens = Number(*usage, input_name);
    reply.output_tokens = Number(*usage, output_name);
  }
}

}  // namespace

// --- OpenAI Chat Completions ---------------------------------------------------------------------
namespace openai_chat {

HttpRequest BuildRequest(const Provider& provider, const Conversation& conversation, const Options& options) {
  json::Value messages = json::Array();
  // The instruction is a message of its own, in front of the others. ("developer" is the newer
  // name; "system" is accepted by every model and by most OpenAI-compatible servers.)
  if (!conversation.system.empty()) {
    json::Push(messages, {{"role", "system"}, {"content", Utf8(conversation.system)}});
  }
  // This format has no place for reasoning blocks: only the text goes out.
  for (const Message& message : conversation.messages) {
    const std::string text = TextOf(message);
    if (!text.empty()) json::Push(messages, {{"role", RoleName(message.role)}, {"content", text}});
  }

  json::Value body = {{"model", provider.model}, {"messages", messages}, {"max_completion_tokens", options.max_tokens}};
  if (options.reasoning != Reasoning::kOff) body["reasoning_effort"] = EffortName(options.reasoning);
  return HttpRequest{WithoutTrailingSlash(provider.endpoint.base_url) + "/chat/completions",
                     JsonHeaders({{"Authorization", "Bearer " + provider.endpoint.api_key}}), json::Dump(body)};
}

Reply ParseReply(const std::string& body) {
  const json::Value root = ParseJson(body);
  const json::Value& object = Object(root);
  const json::Value* choices = json::Find(object, "choices");
  if (choices == nullptr || !choices->is_array() || choices->empty()) {
    throw Error("the answer has no \"choices\"");
  }
  const json::Value& first = (*choices)[0];
  Reply reply = NewReply();
  reply.stop_reason = String(first, "finish_reason");
  const json::Value* message = json::Find(first, "message");
  if (message != nullptr && message->is_object()) {
    const std::string text = String(*message, "content");
    if (!text.empty()) reply.message.blocks.push_back(TextBlock{text});
  }
  ReadUsage(object, reply, "prompt_tokens", "completion_tokens");
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace openai_chat

// --- OpenAI Responses ----------------------------------------------------------------------------
namespace openai_responses {

HttpRequest BuildRequest(const Provider& provider, const Conversation& conversation, const Options& options) {
  json::Value input = json::Array();
  for (const Message& message : conversation.messages) {
    // A reasoning item goes in front of the message it belonged to, exactly as it was received.
    for (const Block& block : message.blocks) {
      if (const json::Value* payload = SendablePayload(block, Protocol::kOpenAIResponses)) json::Push(input, *payload);
    }
    const std::string text = TextOf(message);
    if (!text.empty()) json::Push(input, {{"role", RoleName(message.role)}, {"content", text}});
  }

  json::Value body = {{"model", provider.model},
               {"input", input},
               // the API does not accept fewer than 16 output tokens
               {"max_output_tokens", std::max(16, options.max_tokens)}};
  if (!conversation.system.empty()) body["instructions"] = Utf8(conversation.system);  // a field of its own
  if (options.reasoning != Reasoning::kOff) {
    body["reasoning"] = {{"effort", EffortName(options.reasoning)}};
    // Nothing is kept at the service, so the reasoning items come back with their content
    // encrypted, and we keep them ourselves. The documentation says that is the default when
    // nothing is stored and that "include" is only kept for compatibility; asking for it
    // explicitly costs nothing and does not depend on that.
    body["store"] = false;
    body["include"] = json::Array({"reasoning.encrypted_content"});
  }
  return HttpRequest{WithoutTrailingSlash(provider.endpoint.base_url) + "/responses",
                     JsonHeaders({{"Authorization", "Bearer " + provider.endpoint.api_key}}), json::Dump(body)};
}

Reply ParseReply(const std::string& body) {
  const json::Value root = ParseJson(body);
  const json::Value& object = Object(root);
  const json::Value* output = json::Find(object, "output");
  if (output == nullptr || !output->is_array()) throw Error("the answer has no \"output\"");

  Reply reply = NewReply();
  // "output" is a list of typed items, in the order the model produced them: a reasoning model
  // puts "reasoning" items in front of the "message". The text is in the "output_text" parts.
  for (const json::Value& item : json::Items(*output)) {
    const std::string type = String(item, "type");
    if (type == "reasoning") {
      std::string visible;
      const json::Value* summary = json::Find(item, "summary");
      if (summary != nullptr && summary->is_array()) {
        for (const json::Value& part : json::Items(*summary)) {
          const std::string text = String(part, "text");
          if (text.empty()) continue;
          if (!visible.empty()) visible += '\n';
          visible += text;
        }
      }
      reply.message.blocks.push_back(ReasoningBlock{Protocol::kOpenAIResponses, item, visible});
    } else if (type == "message") {
      std::string text;
      const json::Value* content = json::Find(item, "content");
      if (content != nullptr && content->is_array()) {
        for (const json::Value& part : json::Items(*content)) {
          if (String(part, "type") == "output_text") text += String(part, "text");
        }
      }
      if (!text.empty()) reply.message.blocks.push_back(TextBlock{text});
    } else {
      spdlog::warn("ignoring an output item of type \"{}\"", type);
    }
  }
  reply.stop_reason = String(object, "status");  // "completed", "incomplete", ...
  const json::Value* incomplete = json::Find(object, "incomplete_details");
  if (incomplete != nullptr && incomplete->is_object()) {
    const std::string why = String(*incomplete, "reason");
    if (!why.empty()) reply.stop_reason = why;  // e.g. "max_output_tokens"
  }
  ReadUsage(object, reply, "input_tokens", "output_tokens");
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace openai_responses

// --- Anthropic Messages --------------------------------------------------------------------------
namespace anthropic {

HttpRequest BuildRequest(const Provider& provider, const Conversation& conversation, const Options& options) {
  json::Value messages = json::Array();
  for (const Message& message : conversation.messages) {
    // The content is a list of typed blocks; a thinking block goes back exactly as it was received.
    json::Value content = json::Array();
    for (const Block& block : message.blocks) {
      if (const json::Value* payload = SendablePayload(block, Protocol::kAnthropic)) {
        json::Push(content, *payload);
      } else if (const auto* text = std::get_if<TextBlock>(&block)) {
        if (!text->text.empty()) json::Push(content, {{"type", "text"}, {"text", Utf8(text->text)}});
      }
    }
    if (!content.empty()) json::Push(messages, {{"role", RoleName(message.role)}, {"content", content}});
  }

  json::Value body = {{"model", provider.model},
               {"max_tokens", options.max_tokens},  // required here
               {"messages", messages}};
  if (!conversation.system.empty()) body["system"] = Utf8(conversation.system);  // top level, not a message
  if (options.reasoning != Reasoning::kOff) {
    // The models of today decide for themselves whether and how long to think; "effort" is the hint.
    // Without "display" the thinking text comes back empty (only the signature is there).
    body["thinking"] = {{"type", "adaptive"}, {"display", "summarized"}};
    body["output_config"] = {{"effort", EffortName(options.reasoning)}};
  }
  return HttpRequest{WithoutTrailingSlash(provider.endpoint.base_url) + "/v1/messages",
                     JsonHeaders({{"x-api-key", provider.endpoint.api_key}, {"anthropic-version", "2023-06-01"}}),
                     json::Dump(body)};
}

Reply ParseReply(const std::string& body) {
  const json::Value root = ParseJson(body);
  const json::Value& object = Object(root);
  const json::Value* content = json::Find(object, "content");
  if (content == nullptr || !content->is_array()) throw Error("the answer has no \"content\"");

  Reply reply = NewReply();
  for (const json::Value& block : json::Items(*content)) {  // typed blocks; the thinking blocks come first
    const std::string type = String(block, "type");
    if (type == "text") {
      const std::string text = String(block, "text");
      if (!text.empty()) reply.message.blocks.push_back(TextBlock{text});
    } else if (type == "thinking") {
      reply.message.blocks.push_back(ReasoningBlock{Protocol::kAnthropic, block, String(block, "thinking")});
    } else if (type == "redacted_thinking") {
      reply.message.blocks.push_back(ReasoningBlock{Protocol::kAnthropic, block, ""});
    } else {
      spdlog::warn("ignoring a content block of type \"{}\"", type);
    }
  }
  reply.stop_reason = String(object, "stop_reason");
  ReadUsage(object, reply, "input_tokens", "output_tokens");
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace anthropic

}  // namespace llm
