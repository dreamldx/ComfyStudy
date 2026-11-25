#pragma once

#include <boost/asio.hpp>

#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "json.h"

#include "conversation.h"
#include "transport.h"

// A Transport for tests of the code that talks to a model: it answers the requests one after the
// other with the responses it was given, and remembers the requests it received. When the script
// has run out and one more request comes, it throws std::logic_error (the code sent a request it
// should not have sent).
class ScriptedTransport final : public http::Transport {
 public:
  explicit ScriptedTransport(std::vector<http::Response> script) : script_(std::move(script)) {}

  boost::asio::awaitable<http::Response> Post(http::PostRequest request) override {
    const std::lock_guard<std::mutex> lock(mutex_);
    seen_.push_back(std::move(request));
    if (next_ >= script_.size()) throw std::logic_error("the script has no answer left for this request");
    co_return script_[next_++];
  }

  std::vector<http::PostRequest> seen() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return seen_;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<http::Response> script_;
  std::size_t next_ = 0;
  std::vector<http::PostRequest> seen_;
};

// --- What a service would answer for a given Reply, in each protocol's format --------------------
// Reasoning blocks are only rendered by the protocol they came from (Chat Completions has none).

inline json::Value TokenCounts(const llm::Reply& reply, const char* input, const char* output) {
  json::Value usage = json::Object();
  if (reply.input_tokens) usage[input] = *reply.input_tokens;
  if (reply.output_tokens) usage[output] = *reply.output_tokens;
  return usage;
}

inline std::string ChatBody(const llm::Reply& reply) {
    const std::string text = reply.Text();
  json::Value message = {{"role", "assistant"}, {"content", text.empty() ? json::Value{} : json::Value(text)}};
  return json::Dump(json::Value{{"choices", json::Array({{{"message", message}, {"finish_reason", reply.stop_reason}}})},
              {"usage", TokenCounts(reply, "prompt_tokens", "completion_tokens")}});
}

inline std::string ResponsesBody(const llm::Reply& reply) {
    json::Value output = json::Array();
  for (const llm::Block& block : reply.message.blocks) {
    if (const auto* reasoning = std::get_if<llm::ReasoningBlock>(&block)) {
      if (reasoning->origin == llm::Protocol::kOpenAIResponses) json::Push(output, reasoning->payload);
    } else if (const auto* text = std::get_if<llm::TextBlock>(&block)) {
      json::Push(output, {{"type", "message"},
                        {"role", "assistant"},
                        {"content", json::Array({{{"type", "output_text"}, {"text", text->text}}})}});
    }
  }
  json::Value body = {{"output", output}, {"usage", TokenCounts(reply, "input_tokens", "output_tokens")}};
  if (reply.stop_reason == "completed") {
    body["status"] = "completed";
  } else {
    body["status"] = "incomplete";
    body["incomplete_details"] = {{"reason", reply.stop_reason}};
  }
  return json::Dump(body);
}

inline std::string AnthropicBody(const llm::Reply& reply) {
    json::Value content = json::Array();
  for (const llm::Block& block : reply.message.blocks) {
    if (const auto* reasoning = std::get_if<llm::ReasoningBlock>(&block)) {
      if (reasoning->origin == llm::Protocol::kAnthropic) json::Push(content, reasoning->payload);
    } else if (const auto* text = std::get_if<llm::TextBlock>(&block)) {
      json::Push(content, {{"type", "text"}, {"text", text->text}});
    }
  }
  return json::Dump(json::Value{{"type", "message"},
              {"role", "assistant"},
              {"content", content},
              {"stop_reason", reply.stop_reason},
              {"usage", TokenCounts(reply, "input_tokens", "output_tokens")}});
}
