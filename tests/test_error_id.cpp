#include "error_id.h"

#include <gtest/gtest.h>

#include <set>
#include <string>

TEST(ErrorId, IsSixCharactersFromTheAlphabetWithoutLookAlikes) {
  for (int i = 0; i < 200; ++i) {
    const std::string id = bot::MakeErrorId();
    ASSERT_EQ(id.size(), 6u);
    for (const char c : id) {
      EXPECT_TRUE((c >= 'A' && c <= 'Z') || (c >= '2' && c <= '9')) << id;
      EXPECT_NE(c, 'O');
      EXPECT_NE(c, 'I');
    }
  }
}

TEST(ErrorId, TheNumbersGiveTheCharactersSoATestCanKnowTheId) {
  unsigned next = 0;
  EXPECT_EQ(bot::MakeErrorId([&next] { return next++; }), "ABCDEF");
  next = 32;  // the alphabet wraps around
  EXPECT_EQ(bot::MakeErrorId([&next] { return next++; }), "ABCDEF");
}

TEST(ErrorId, TwoIdsAreNotTheSameInPractice) {
  std::set<std::string> seen;
  for (int i = 0; i < 100; ++i) seen.insert(bot::MakeErrorId());
  EXPECT_GT(seen.size(), 95u);
}
