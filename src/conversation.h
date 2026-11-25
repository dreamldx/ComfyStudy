#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "json.h"

#include "protocol.h"

namespace llm {

// What a conversation is made of: a message is a role and a list of typed blocks.
// Chapter 1.2 has two kinds; tool calls, tool results and images come in later chapters.

struct TextBlock {
  std::string text;
};

// A block the service produced while it was thinking: Anthropic's "thinking" and
// "redacted_thinking" blocks, OpenAI Responses' "reasoning" items. It is opaque: it may carry a
// signature or encrypted content that the service checks when it comes back, so `payload` is
// stored exactly as received and sent back exactly as stored. Only the protocol it came from can
// read it.
struct ReasoningBlock {
  Protocol origin;
  json::Value payload;
  std::string visible_text;  // the readable part (if any), for display and logs only
};

using Block = std::variant<TextBlock, ReasoningBlock>;

enum class Role { kUser, kAssistant };

struct Message {
  Role role;
  std::vector<Block> blocks;
};

// The instruction ("system prompt") is not a message: the three protocols put it in different
// places.
struct Conversation {
  std::string system;  // may be empty
  std::vector<Message> messages;
};

// How much the model should think before it answers.
enum class Reasoning { kOff, kLow, kMedium, kHigh };

struct Options {
  int max_tokens = 1024;  // upper limit for the answer, counted in tokens
  Reasoning reasoning = Reasoning::kOff;
};

// An answer: the assistant's message (append it to the conversation to continue), why it stopped,
// and what it cost.
struct Reply {
  Message message;          // role kAssistant
  std::string stop_reason;  // in the protocol's own words
  std::optional<long> input_tokens;
  std::optional<long> output_tokens;

  // The generated text: all text blocks of the message, one after the other ("" if it has none).
  std::string Text() const;
};

// How many reasoning blocks of the conversation did not come from `protocol`. They stay in the
// conversation, but they are not sent to a service that speaks `protocol`.
std::size_t CountForeignReasoning(const Conversation& conversation, Protocol protocol);

}  // namespace llm
