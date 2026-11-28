#include "message_split.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

// How many characters (not bytes) a UTF-8 text has.
std::size_t Chars(const std::string& text) {
  std::size_t n = 0;
  for (const char c : text) n += (static_cast<unsigned char>(c) & 0xC0u) != 0x80u ? 1 : 0;
  return n;
}

std::string Repeat(const std::string& unit, std::size_t times) {
  std::string out;
  for (std::size_t i = 0; i < times; ++i) out += unit;
  return out;
}

std::string Joined(const std::vector<std::string>& parts) {
  std::string all;
  for (const std::string& part : parts) all += part;
  return all;
}

// The number of lines that open or close a code fence in `text`.
std::size_t Fences(const std::string& text) {
  std::size_t count = 0;
  std::size_t at = 0;
  while ((at = text.find("```", at)) != std::string::npos) {
    ++count;
    at += 3;
  }
  return count;
}

}  // namespace

TEST(SplitMessage, ATextThatFitsIsOneMessageAsItWas) {
  EXPECT_EQ(bot::SplitMessage("hello"), std::vector<std::string>{"hello"});
  const std::string exactly = Repeat("a", 2000);
  EXPECT_EQ(bot::SplitMessage(exactly), std::vector<std::string>{exactly});  // 2000 is allowed
}

TEST(SplitMessage, NothingOrOnlyBlankSpaceIsNoMessageAtAll) {
  EXPECT_TRUE(bot::SplitMessage("").empty());
  EXPECT_TRUE(bot::SplitMessage("  \n\n ").empty());
}

TEST(SplitMessage, OneCharacterOverTheLimitMakesTwoMessagesAndNoneIsTooLong) {
  const std::vector<std::string> parts = bot::SplitMessage(Repeat("a", 2001));
  ASSERT_EQ(parts.size(), 2u);
  for (const std::string& part : parts) EXPECT_LE(Chars(part), 2000u);
  EXPECT_EQ(Joined(parts), Repeat("a", 2001));  // a text without any break is cut where it has to be, nothing is lost
}

TEST(SplitMessage, ItBreaksAtABlankLineWhenThereIsOne) {
  const std::string first = Repeat("a", 1500);
  const std::string second = Repeat("b", 1500);
  const std::vector<std::string> parts = bot::SplitMessage(first + "\n\n" + second);
  ASSERT_EQ(parts.size(), 2u);
  EXPECT_EQ(parts[0], first);
  EXPECT_EQ(parts[1], second);
}

TEST(SplitMessage, OtherwiseAtTheEndOfALineAndOtherwiseAtASpace) {
  const std::string first = Repeat("a", 1500);
  const std::string second = Repeat("b", 1500);
  std::vector<std::string> parts = bot::SplitMessage(first + "\n" + second);
  ASSERT_EQ(parts.size(), 2u);
  EXPECT_EQ(parts[0], first);
  EXPECT_EQ(parts[1], second);

  parts = bot::SplitMessage(first + " " + second);
  ASSERT_EQ(parts.size(), 2u);
  EXPECT_EQ(parts[0], first);
  EXPECT_EQ(parts[1], second);
}

TEST(SplitMessage, ABreakVeryEarlyInTheWindowIsNotUsedWhenItWouldMakeATinyMessage) {
  const std::string text = "ab\n" + Repeat("c", 3000);  // the only line end is at the third character
  const std::vector<std::string> parts = bot::SplitMessage(text);
  EXPECT_GT(Chars(parts[0]), 1000u);
  EXPECT_EQ(Joined(parts), text);
}

TEST(SplitMessage, TheLimitCountsCharactersNotBytes) {
  const std::string chinese = Repeat("你", 2000);  // 6000 bytes
  EXPECT_EQ(bot::SplitMessage(chinese).size(), 1u);
  const std::vector<std::string> parts = bot::SplitMessage(chinese + "好");
  ASSERT_EQ(parts.size(), 2u);
  for (const std::string& part : parts) EXPECT_LE(Chars(part), 2000u);
  EXPECT_EQ(Joined(parts), chinese + "好");  // no character is cut in the middle
}

TEST(SplitMessage, ACodeFenceThatIsCutIsClosedAndOpenedAgainWithItsLanguage) {
  std::string code = "```cpp\n";
  for (int i = 0; i < 400; ++i) code += "int x" + std::to_string(i) + " = " + std::to_string(i) + ";\n";
  code += "```";
  const std::vector<std::string> parts = bot::SplitMessage("Here it is:\n\n" + code);
  ASSERT_GT(parts.size(), 1u);
  for (const std::string& part : parts) {
    EXPECT_LE(Chars(part), 2000u);
    EXPECT_EQ(Fences(part) % 2, 0u) << part.substr(0, 60);  // every message has its fences in pairs
  }
  EXPECT_EQ(parts[1].rfind("```cpp\n", 0), 0u);  // the next message opens the fence again
}

TEST(SplitMessage, ATextWithoutFencesGetsNoneAdded) {
  const std::vector<std::string> parts = bot::SplitMessage(Repeat("word ", 1000));
  for (const std::string& part : parts) EXPECT_FALSE(Contains(part, "```"));
}

TEST(SplitMessage, TheLimitCanBeChanged) {
  const std::vector<std::string> parts = bot::SplitMessage("one two three four five six", 10);
  ASSERT_GT(parts.size(), 1u);
  for (const std::string& part : parts) EXPECT_LE(Chars(part), 10u);
}

TEST(SplitMessage, NoTextIsLostWhenItIsCutAtWhiteSpace) {
  const std::string text = Repeat("alpha beta gamma\n", 300) + Repeat("delta\n\n", 200);
  const std::vector<std::string> parts = bot::SplitMessage(text);
  std::string words;
  for (const std::string& part : parts) words += part + " ";
  for (const char* word : {"alpha", "beta", "gamma", "delta"}) {
    std::size_t expected = std::string(word) == "delta" ? 200 : 300;
    std::size_t found = 0;
    for (std::size_t at = words.find(word); at != std::string::npos; at = words.find(word, at + 1)) ++found;
    EXPECT_EQ(found, expected) << word;
  }
}
