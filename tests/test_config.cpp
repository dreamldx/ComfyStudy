#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <stdlib.h>

#include "config.h"

namespace {

config::Values ValuesWith(std::initializer_list<std::pair<const char*, const char*>> items) {
  config::Values values;
  for (const auto& [name, value] : items) values[name] = value;
  return values;
}

const config::Key& KeyNamed(const std::string& name) {
  for (const config::Key& key : config::KnownKeys()) {
    if (key.name == name) return key;
  }
  throw std::runtime_error("no such key: " + name);
}

}  // namespace

TEST(ConfigMask, EmptyIsUnset) { EXPECT_EQ(config::MaskSecret(""), "(unset)"); }

TEST(ConfigMask, ShortSecretsShowNothing) {
  EXPECT_EQ(config::MaskSecret("abc"), "****");
  EXPECT_EQ(config::MaskSecret("1234567"), "****");
}

TEST(ConfigMask, LongSecretsShowThreeCharacters) {
  EXPECT_EQ(config::MaskSecret("sk-abcdef1234"), "sk-***");
}

TEST(ConfigDescribe, SecretIsMasked) {
  const auto values = ValuesWith({{"discord_token", "MTIzNDU2Nzg5MA"}});
  EXPECT_EQ(config::Describe(values, KeyNamed("discord_token")), "discord_token=MTI***");
}

TEST(ConfigDescribe, PlainValueIsShown) {
  const auto values = ValuesWith({{"llm_provider", "openai"}});
  EXPECT_EQ(config::Describe(values, KeyNamed("llm_provider")), "llm_provider=openai");
}

TEST(ConfigDescribe, EmptyOrMissingValueIsUnset) {
  EXPECT_EQ(config::Describe(ValuesWith({{"openai_model", ""}}), KeyNamed("openai_model")),
            "openai_model=(unset)");
  EXPECT_EQ(config::Describe({}, KeyNamed("openai_model")), "openai_model=(unset)");
}

TEST(ConfigKnownKeys, TheKeysInOrderWithTheRightOnesSecret) {
  struct Expected {
    const char* name;
    const char* env;
    const char* default_value;
    bool secret;
  };
  const std::vector<Expected> expected = {
      {"discord_token", "DISCORD_TOKEN", "", true},
      {"llm_provider", "LLM_PROVIDER", "openai", false},
      {"llm_reasoning", "LLM_REASONING", "off", false},
      {"agent_max_steps", "AGENT_MAX_STEPS", "10", false},
      {"agent_turn_timeout_ms", "AGENT_TURN_TIMEOUT_MS", "300000", false},
      {"openai_api", "OPENAI_API", "chat", false},
      {"openai_model", "OPENAI_MODEL", "", false},
      {"openai_api_key", "OPENAI_API_KEY", "", true},
      {"openai_base_url", "OPENAI_BASE_URL", "https://api.openai.com/v1", false},
      {"anthropic_model", "ANTHROPIC_MODEL", "", false},
      {"anthropic_api_key", "ANTHROPIC_API_KEY", "", true},
      {"anthropic_base_url", "ANTHROPIC_BASE_URL", "https://api.anthropic.com", false},
      {"model_dir", "MODEL_DIR", "models", false}};
  const auto& keys = config::KnownKeys();
  ASSERT_EQ(keys.size(), expected.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    EXPECT_EQ(keys[i].name, expected[i].name);
    EXPECT_EQ(keys[i].env, expected[i].env) << keys[i].name;
    EXPECT_EQ(keys[i].default_value, expected[i].default_value) << keys[i].name;
    EXPECT_EQ(keys[i].secret, expected[i].secret) << keys[i].name;
  }
}

TEST(ConfigEnvName, IsTheNameInUpperCase) {
  EXPECT_EQ(config::EnvName("openai_model"), "OPENAI_MODEL");
  EXPECT_EQ(config::EnvName("openai_api_key"), "OPENAI_API_KEY");
}

TEST(ConfigPrint, NeverShowsAFullSecret) {
  const auto values =
      ValuesWith({{"openai_api_key", "supersecretvalue123"}, {"anthropic_api_key", "short"}});
  std::ostringstream out;
  config::Print(out, values);
  const std::string text = out.str();
  EXPECT_EQ(text.find("supersecretvalue123"), std::string::npos);
  EXPECT_EQ(text.find("short"), std::string::npos);
  EXPECT_NE(text.find("openai_api_key=sup***"), std::string::npos);
  EXPECT_NE(text.find("anthropic_api_key=****"), std::string::npos);
}

TEST(ConfigGet, EmptyVariableCountsAsUnset) {
  _putenv_s("A_TEST_Z", "");  // an empty value removes the variable
  EXPECT_FALSE(config::Get("A_TEST_Z").has_value());
}

TEST(ConfigGet, ReadsAVariable) {
  _putenv_s("A_TEST_Z", "hello");
  EXPECT_EQ(config::Get("A_TEST_Z"), "hello");
  _putenv_s("A_TEST_Z", "");
}

TEST(ConfigPaths, ExeDirectoryContainsThisTestProgram) {
  const std::filesystem::path dir = config::ExeDirectory();
  EXPECT_TRUE(std::filesystem::is_directory(dir));
  EXPECT_TRUE(std::filesystem::exists(dir / "test_config.exe"));
}

TEST(ConfigPaths, CandidatesAreTheExeDirectoryThenTheWorkingDirectoryTomlBeforeIni) {
  const std::filesystem::path exe_dir = "C:\\app\\bin";
  const std::filesystem::path cwd = "D:\\work";
  const std::vector<std::filesystem::path> expected = {
      exe_dir / "config.toml", exe_dir / "config.ini", cwd / "config.toml", cwd / "config.ini"};
  EXPECT_EQ(config::ConfigCandidates(exe_dir, cwd), expected);
}
