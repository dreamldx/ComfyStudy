#include <string>

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include "net_probe.h"

TEST(Deps, CurlReportsItsVersion) {
  EXPECT_EQ(CurlVersion().rfind("libcurl/", 0), 0u);
}

TEST(Deps, JsonRoundTripIsCompact) {
  EXPECT_EQ(JsonRoundTrip("{ \"a\" : [1, 2] }"), std::string("{\"a\":[1,2]}"));
}

TEST(Deps, CliParsesAnOption) {
  EXPECT_EQ(NameFromArgs({"--name", "Ada"}), "Ada");
  EXPECT_EQ(NameFromArgs({}), "");
}

TEST(Deps, CliRejectsAnUnknownOption) {
  EXPECT_THROW(NameFromArgs({"--bogus"}), CLI::ParseError);
}
