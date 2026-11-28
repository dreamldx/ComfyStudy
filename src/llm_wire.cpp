#include "llm_wire.h"

#include <algorithm>

#include <spdlog/spdlog.h>

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

// The arguments of a call as the text that goes out in the formats that take text (Chat, Responses):
// the text as it was received, or else the arguments written out.
std::string ArgumentsText(const ToolCallBlock& call) {
  if (!call.arguments_text.empty()) return call.arguments_text;
  return call.arguments.is_null() ? std::string("{}") : json::Dump(call.arguments);
}

// The arguments of a call as the object the Anthropic format takes ({} when there is no object).
json::Value ArgumentsObject(const ToolCallBlock& call) { return call.arguments.is_object() ? call.arguments : json::Object(); }

// The arguments text of a call read from an answer. Text that is not JSON is not an error here: the
// call is kept (with null arguments) and the loop answers it with an error result.
json::Value ParseArguments(const std::string& text) {
  if (text.empty()) return json::Object();
  try {
    return json::Parse(text);
  } catch (const json::ParseError&) {
    return json::Value{};
  }
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

// OpenAI: the tokens taken from the cache are in a details object of the usage, and are part of the input count.
void ReadCachedTokens(const json::Value& object, Reply& reply, const char* details_name) {
  const json::Value* usage = json::Find(object, "usage");
  if (usage == nullptr || !usage->is_object()) return;
  const json::Value* details = json::Find(*usage, details_name);
  if (details != nullptr && details->is_object()) reply.cached_tokens = Number(*details, "cached_tokens");
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
  // This format has no place for reasoning blocks: only the text and the tool calls go out. Every
  // result is a message of its own (role "tool"); the text after the results is a user message.
  for (const Message& message : conversation.messages) {
    const std::string text = TextOf(message);
    json::Value calls = json::Array();
    for (const Block& block : message.blocks) {
      if (const auto* result = std::get_if<ToolResultBlock>(&block)) {
        json::Push(messages, {{"role", "tool"}, {"tool_call_id", result->call_id}, {"content", Utf8(result->text)}});
      } else if (const auto* call = std::get_if<ToolCallBlock>(&block)) {
        json::Push(calls, {{"id", call->id},
                           {"type", "function"},
                           {"function", {{"name", call->name}, {"arguments", ArgumentsText(*call)}}}});
      }
    }
    if (!calls.get_array().empty()) {
      json::Push(messages, {{"role", "assistant"}, {"content", text.empty() ? json::Value{} : json::Value(text)}, {"tool_calls", calls}});
    } else if (!text.empty()) {
      json::Push(messages, {{"role", RoleName(message.role)}, {"content", text}});
    }
  }

  json::Value body = {{"model", provider.model}, {"messages", messages}, {"max_completion_tokens", options.max_tokens}};
  if (!options.tools.empty()) {
    json::Value tools = json::Array();
    for (const ToolDefinition& tool : options.tools) {
      json::Push(tools, {{"type", "function"},
                         {"function", {{"name", tool.name}, {"description", Utf8(tool.description)}, {"parameters", tool.parameters}}}});
    }
    body["tools"] = tools;
  }
  if (options.reasoning != Reasoning::kOff) body["reasoning_effort"] = EffortName(options.reasoning);
  return HttpRequest{WithoutTrailingSlash(provider.endpoint.base_url) + "/chat/completions",
                     JsonHeaders({{"Authorization", "Bearer " + provider.endpoint.api_key}}), json::Dump(body)};
}

Reply ParseReply(const std::string& body) {
  const json::Value root = ParseJson(body);
  const json::Value& object = Object(root);
  const json::Value* choices = json::Find(object, "choices");
  if (choices == nullptr || !choices->is_array() || json::Items(*choices).empty()) {
    throw Error("the answer has no \"choices\"");
  }
  const json::Value& first = json::Items(*choices)[0];
  Reply reply = NewReply();
  reply.stop_reason = String(first, "finish_reason");
  const json::Value* message = json::Find(first, "message");
  if (message != nullptr && message->is_object()) {
    const std::string text = String(*message, "content");
    if (!text.empty()) reply.message.blocks.push_back(TextBlock{text});
    const json::Value* calls = json::Find(*message, "tool_calls");
    if (calls != nullptr && calls->is_array()) {
      for (const json::Value& call : json::Items(*calls)) {
        const json::Value* function = json::Find(call, "function");
        if (function == nullptr || !function->is_object()) continue;
        // The format says text, and the text is not always valid JSON; some compatible servers send an
        // object instead, which is taken as it is.
        const json::Value* raw = json::Find(*function, "arguments");
        const bool is_object = raw != nullptr && raw->is_object();
        const std::string arguments = is_object ? json::Dump(*raw) : String(*function, "arguments");
        reply.message.blocks.push_back(ToolCallBlock{String(call, "id"), String(*function, "name"),
                                                     is_object ? *raw : ParseArguments(arguments), arguments, ""});
      }
    }
  }
  ReadUsage(object, reply, "prompt_tokens", "completion_tokens");
  ReadCachedTokens(object, reply, "prompt_tokens_details");
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
    // The results of a Step come first, then the text (steering) that followed them; the calls of
    // an assistant message come after its text, each as an item of its own.
    for (const Block& block : message.blocks) {
      if (const auto* result = std::get_if<ToolResultBlock>(&block)) {
        json::Push(input, {{"type", "function_call_output"}, {"call_id", result->call_id}, {"output", Utf8(result->text)}});
      }
    }
    const std::string text = TextOf(message);
    if (!text.empty()) json::Push(input, {{"role", RoleName(message.role)}, {"content", text}});
    for (const Block& block : message.blocks) {
      if (const auto* call = std::get_if<ToolCallBlock>(&block)) {
        json::Value item = {{"type", "function_call"}, {"call_id", call->id}, {"name", call->name}, {"arguments", ArgumentsText(*call)}};
        if (!call->item_id.empty()) item["id"] = call->item_id;
        json::Push(input, item);
      }
    }
  }

  json::Value body = {{"model", provider.model},
                      {"input", input},
                      // the API does not accept fewer than 16 output tokens
                      {"max_output_tokens", std::max(16, options.max_tokens)}};
  if (!conversation.system.empty()) body["instructions"] = Utf8(conversation.system);  // a field of its own
  if (!options.tools.empty()) {
    json::Value tools = json::Array();
    for (const ToolDefinition& tool : options.tools) {  // flat here, not nested under "function"
      json::Push(tools, {{"type", "function"}, {"name", tool.name}, {"description", Utf8(tool.description)}, {"parameters", tool.parameters}});
    }
    body["tools"] = tools;
  }
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
    } else if (type == "function_call") {
      const std::string arguments = String(item, "arguments");  // text, and not always valid JSON
      reply.message.blocks.push_back(
          ToolCallBlock{String(item, "call_id"), String(item, "name"), ParseArguments(arguments), arguments, String(item, "id")});
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
  ReadCachedTokens(object, reply, "input_tokens_details");
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace openai_responses

// --- Anthropic Messages --------------------------------------------------------------------------
namespace anthropic {

HttpRequest BuildRequest(const Provider& provider, const Conversation& conversation, const Options& options) {
  // Prompt caching: a marker says "everything up to here may be kept". The service allows four. One goes after
  // the instruction (or after the last tool when there is no instruction): it covers the tools too, which come
  // first. The messages at the cache points take the rest, the later ones first.
  const json::Value marker = {{"type", "ephemeral"}};
  const bool has_prefix = !conversation.system.empty() || !options.tools.empty();
  std::vector<std::size_t> points;
  if (options.cache) {
    points = conversation.cache_points;
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    const std::size_t room = has_prefix ? 3 : 4;
    if (points.size() > room) points.erase(points.begin(), points.end() - static_cast<std::ptrdiff_t>(room));
  }

  json::Value messages = json::Array();
  std::size_t index = 0;
  for (const Message& message : conversation.messages) {
    const std::size_t this_index = index++;
    // The content is a list of typed blocks; a thinking block goes back exactly as it was received.
    json::Value content = json::Array();
    for (const Block& block : message.blocks) {
      if (const json::Value* payload = SendablePayload(block, Protocol::kAnthropic)) {
        json::Push(content, *payload);
      } else if (const auto* text = std::get_if<TextBlock>(&block)) {
        if (!text->text.empty()) json::Push(content, {{"type", "text"}, {"text", Utf8(text->text)}});
      } else if (const auto* call = std::get_if<ToolCallBlock>(&block)) {
        json::Push(content, {{"type", "tool_use"}, {"id", call->id}, {"name", call->name}, {"input", ArgumentsObject(*call)}});
      } else if (const auto* result = std::get_if<ToolResultBlock>(&block)) {
        // All the results of a Step are in one user message, in front of any text (the service insists).
        json::Value item = {{"type", "tool_result"}, {"tool_use_id", result->call_id}, {"content", Utf8(result->text)}};
        if (result->is_error) item["is_error"] = true;
        json::Push(content, item);
      }
    }
    if (content.get_array().empty()) continue;
    if (std::binary_search(points.begin(), points.end(), this_index)) content.get_array().back()["cache_control"] = marker;
    json::Push(messages, {{"role", RoleName(message.role)}, {"content", content}});
  }

  json::Value body = {{"model", provider.model},
                      {"max_tokens", options.max_tokens},  // required here
                      {"messages", messages}};
  if (!conversation.system.empty()) {
    if (options.cache) {
      // a marker can only be put on a block, so the instruction is a list of one
      body["system"] = json::Array({{{"type", "text"}, {"text", Utf8(conversation.system)}, {"cache_control", marker}}});
    } else {
      body["system"] = Utf8(conversation.system);  // top level, not a message
    }
  }
  if (!options.tools.empty()) {
    json::Value tools = json::Array();
    for (const ToolDefinition& tool : options.tools) {
      json::Push(tools, {{"name", tool.name}, {"description", Utf8(tool.description)}, {"input_schema", tool.parameters}});
    }
    if (options.cache && conversation.system.empty()) tools.get_array().back()["cache_control"] = marker;
    body["tools"] = tools;
  }
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
    } else if (type == "tool_use") {
      const json::Value* input = json::Find(block, "input");
      reply.message.blocks.push_back(
          ToolCallBlock{String(block, "id"), String(block, "name"), input != nullptr ? *input : json::Object(), "", ""});
    } else {
      spdlog::warn("ignoring a content block of type \"{}\"", type);
    }
  }
  reply.stop_reason = String(object, "stop_reason");
  ReadUsage(object, reply, "input_tokens", "output_tokens");
  // Here input_tokens is only what comes after the last cache marker; what was read from the cache and what was
  // written to it are counted apart. The total is what the other two protocols call the input.
  const json::Value* usage = json::Find(object, "usage");
  if (usage != nullptr && usage->is_object()) {
    reply.cached_tokens = Number(*usage, "cache_read_input_tokens");
    reply.cache_written_tokens = Number(*usage, "cache_creation_input_tokens");
    if (reply.input_tokens && (reply.cached_tokens || reply.cache_written_tokens)) {
      reply.input_tokens = *reply.input_tokens + reply.cached_tokens.value_or(0) + reply.cache_written_tokens.value_or(0);
    }
  }
  return reply;
}

std::string ErrorText(const std::string& body) { return ErrorFromBody(body); }

}  // namespace anthropic

}  // namespace llm
