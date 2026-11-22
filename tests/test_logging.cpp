#include <memory>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

namespace {

// A logger that writes into a string, with a pattern that prints only the level and the message.
std::shared_ptr<spdlog::logger> MakeStringLogger(std::ostringstream& out) {
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  auto logger = std::make_shared<spdlog::logger>("test", sink);
  logger->set_pattern("[%l] %v");
  return logger;
}

}  // namespace

TEST(Logging, FormatsWithBraces) {
  std::ostringstream out;
  auto logger = MakeStringLogger(out);
  logger->info("loaded {} variables from {}", 3, "a.env");
  EXPECT_NE(out.str().find("[info] loaded 3 variables from a.env"), std::string::npos);
}

TEST(Logging, LevelsBelowTheThresholdAreDropped) {
  std::ostringstream out;
  auto logger = MakeStringLogger(out);
  logger->set_level(spdlog::level::warn);
  logger->info("hidden");
  logger->warn("shown");
  EXPECT_EQ(out.str().find("hidden"), std::string::npos);
  EXPECT_NE(out.str().find("[warning] shown"), std::string::npos);
}