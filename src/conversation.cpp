#include "conversation.h"

namespace llm {

std::string Reply::Text() const {
  std::string text;
  for (const Block& block : message.blocks) {
    if (const auto* part = std::get_if<TextBlock>(&block)) text += part->text;
  }
  return text;
}

std::size_t CountForeignReasoning(const Conversation& conversation, Protocol protocol) {
  std::size_t count = 0;
  for (const Message& message : conversation.messages) {
    for (const Block& block : message.blocks) {
      const auto* reasoning = std::get_if<ReasoningBlock>(&block);
      if (reasoning != nullptr && reasoning->origin != protocol) ++count;
    }
  }
  return count;
}

}  // namespace llm
