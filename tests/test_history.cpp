#include "history.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

llm::Message User(const std::string& text) { return {llm::Role::kUser, {llm::TextBlock{text}}}; }
llm::Message Assistant(const std::string& text) { return {llm::Role::kAssistant, {llm::TextBlock{text}}}; }

std::string TextOf(const llm::Message& message) { return std::get<llm::TextBlock>(message.blocks.at(0)).text; }

}  // namespace

TEST(MemoryHistory, AChannelThatWasNeverWrittenToIsEmpty) {
  agent::MemoryHistory history;
  EXPECT_TRUE(history.Load("a").empty());
}

TEST(MemoryHistory, LoadGivesWhatWasAppendedInOrder) {
  agent::MemoryHistory history;
  history.Append("a", {User("one"), Assistant("two")});
  history.Append("a", {User("three")});
  const std::vector<llm::Message> messages = history.Load("a");
  ASSERT_EQ(messages.size(), 3u);
  EXPECT_EQ(TextOf(messages[0]), "one");
  EXPECT_EQ(TextOf(messages[2]), "three");
}

TEST(MemoryHistory, ChannelsDoNotSeeEachOther) {
  agent::MemoryHistory history;
  history.Append("a", {User("for a")});
  history.Append("b", {User("for b")});
  EXPECT_EQ(history.Load("a").size(), 1u);
  EXPECT_EQ(TextOf(history.Load("b")[0]), "for b");
}

TEST(MemoryHistory, ClearEmptiesOneChannelOnly) {
  agent::MemoryHistory history;
  history.Append("a", {User("for a")});
  history.Append("b", {User("for b")});
  history.Clear("a");
  EXPECT_TRUE(history.Load("a").empty());
  EXPECT_EQ(history.Load("b").size(), 1u);
}

TEST(MemoryHistory, WhatLoadReturnsIsACopy) {
  agent::MemoryHistory history;
  history.Append("a", {User("one")});
  std::vector<llm::Message> copy = history.Load("a");
  copy.push_back(User("not stored"));
  EXPECT_EQ(history.Load("a").size(), 1u);
}
