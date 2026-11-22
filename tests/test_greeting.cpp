#include <string>

#include <gtest/gtest.h>

#include "greeting.h"
#include "version.h"

TEST(Greeting, ContainsNameAndVersion) {
  EXPECT_EQ(Greeting(), std::string("ComfyStudy ") + COMFYSTUDY_VERSION);
}

TEST(Greeting, IsNotEmpty) {
  EXPECT_FALSE(Greeting().empty());
}