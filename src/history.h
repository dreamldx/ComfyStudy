#pragma once

#include <map>
#include <string>
#include <vector>

#include "conversation.h"

namespace agent {

// The messages of the conversations that are stored, one list for each channel (a Discord channel or thread,
// "cli" on the command line). The loop does not touch it: whoever runs a Turn loads the messages before it
// and appends the Turn's messages after it, and only when the Turn ended with an answer, so a Turn that was
// forgotten leaves no trace. Chapter 1.9 adds an implementation that keeps them in SQLite.
class History {
 public:
  virtual ~History() = default;
  virtual std::vector<llm::Message> Load(const std::string& channel) const = 0;
  virtual void Append(const std::string& channel, std::vector<llm::Message> messages) = 0;
  virtual void Clear(const std::string& channel) = 0;
};

// In memory, until the program ends.
class MemoryHistory final : public History {
 public:
  std::vector<llm::Message> Load(const std::string& channel) const override;
  void Append(const std::string& channel, std::vector<llm::Message> messages) override;
  void Clear(const std::string& channel) override;

 private:
  std::map<std::string, std::vector<llm::Message>> channels_;
};

}  // namespace agent
