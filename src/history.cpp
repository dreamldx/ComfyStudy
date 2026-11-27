#include "history.h"

namespace agent {

std::vector<llm::Message> MemoryHistory::Load(const std::string& channel) const {
  const auto found = channels_.find(channel);
  return found == channels_.end() ? std::vector<llm::Message>{} : found->second;
}

void MemoryHistory::Append(const std::string& channel, std::vector<llm::Message> messages) {
  std::vector<llm::Message>& stored = channels_[channel];
  for (llm::Message& message : messages) stored.push_back(std::move(message));
}

void MemoryHistory::Clear(const std::string& channel) { channels_.erase(channel); }

}  // namespace agent
