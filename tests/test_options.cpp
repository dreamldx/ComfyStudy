#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include <stdlib.h>

#include "config.h"
#include "options.h"

namespace {

// Writes `bytes` to a fresh file in the temp directory (with the given extension) and removes it
// afterwards.
class TempFile {
 public:
  TempFile(std::string_view bytes, const std::string& extension) {
    path_ = std::filesystem::temp_directory_path() /
            ("comfystudy_options_test_" + std::to_string(counter_++) + extension);
    std::ofstream out(path_, std::ios::binary);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  }
  ~TempFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }
  const std::filesystem::path& path() const { return path_; }
  std::string str() const { return path_.string(); }

 private:
  static inline int counter_ = 0;
  std::filesystem::path path_;
};

// Every test starts with none of the known environment variables set, whatever the developer's
// machine has, and the original values are put back afterwards.
class OptionsTest : public testing::Test {
 protected:
  void SetUp() override {
    for (const config::Key& key : config::KnownKeys()) {
      saved_.emplace_back(key.env, config::Get(key.env));
      _putenv_s(key.env.c_str(), "");
    }
  }
  void TearDown() override {
    for (const auto& [name, value] : saved_) _putenv_s(name.c_str(), value.value_or("").c_str());
  }

  static void SetEnv(const char* name, const char* value) { _putenv_s(name, value); }

  // Runs Parse with no config-file candidates; the output goes to out_ and err_.
  options::Result Run(const std::vector<std::string>& args,
                      const std::vector<std::filesystem::path>& candidates = {}) {
    return options::Parse(args, candidates, out_, err_);
  }

  std::ostringstream out_;
  std::ostringstream err_;

 private:
  std::vector<std::pair<std::string, std::optional<std::string>>> saved_;
};

}  // namespace

TEST_F(OptionsTest, NothingGivenMeansTheDefaults) {
  const auto result = Run({});
  EXPECT_FALSE(result.exit_code.has_value());
  EXPECT_EQ(result.options.values.at("llm_provider"), "openai");
  EXPECT_EQ(result.options.values.at("model_dir"), "models");
  EXPECT_EQ(result.options.values.at("openai_model"), "");
  EXPECT_FALSE(result.options.config_file.has_value());
  EXPECT_FALSE(result.options.verbose);
#if defined(_WIN32)
  EXPECT_FALSE(result.options.dependency_check);
#endif
}

TEST_F(OptionsTest, EveryKnownKeyHasAValue) {
  EXPECT_EQ(Run({}).options.values.size(), config::KnownKeys().size());
}

TEST_F(OptionsTest, EnvironmentBeatsTheDefault) {
  SetEnv("LLM_PROVIDER", "anthropic");
  EXPECT_EQ(Run({}).options.values.at("llm_provider"), "anthropic");
}

TEST_F(OptionsTest, ConfigFileBeatsTheEnvironment) {
  SetEnv("OPENAI_MODEL", "from-env");
  TempFile file("openai_model = from-file\n", ".ini");
  EXPECT_EQ(Run({"--config", file.str()}).options.values.at("openai_model"), "from-file");
}

TEST_F(OptionsTest, CommandLineBeatsTheConfigFile) {
  TempFile file("openai_model = from-file\n", ".ini");
  const auto result = Run({"--config", file.str(), "--openai_model", "from-cli"});
  EXPECT_EQ(result.options.values.at("openai_model"), "from-cli");
}

TEST_F(OptionsTest, CommandLineBeatsEverything) {
  SetEnv("OPENAI_MODEL", "from-env");
  TempFile file("openai_model = from-file\n", ".toml");
  const auto result = Run({"--openai_model", "from-cli", "--config", file.str()});
  EXPECT_EQ(result.options.values.at("openai_model"), "from-cli");
}

TEST_F(OptionsTest, EachSettingIsResolvedOnItsOwn) {
  SetEnv("LLM_PROVIDER", "anthropic");
  SetEnv("MODEL_DIR", "from-env");
  TempFile file("openai_model = from-file\nmodel_dir = from-file\n", ".ini");
  const auto result = Run({"--config", file.str(), "--llm_provider", "cli"});
  EXPECT_EQ(result.options.values.at("llm_provider"), "cli");        // command line
  EXPECT_EQ(result.options.values.at("model_dir"), "from-file");     // config file beats env
  EXPECT_EQ(result.options.values.at("openai_model"), "from-file");     // config file
  EXPECT_EQ(result.options.values.at("openai_base_url"), "https://api.openai.com/v1");  // default
}

TEST_F(OptionsTest, AnEmptyValueInTheFileDoesNotHideTheEnvironment) {
  SetEnv("DISCORD_TOKEN", "token-from-env");
  TempFile file("discord_token =\nopenai_model = \"\"\n", ".ini");
  EXPECT_EQ(Run({"--config", file.str()}).options.values.at("discord_token"), "token-from-env");
}

TEST_F(OptionsTest, ReadsIni) {
  TempFile file(
      "# a comment\n; another comment\nopenai_model = gpt-4o # trailing comment\n"
      "openai_base_url = http://localhost:8080/v1?a=b\n",
      ".ini");
  const auto result = Run({"--config", file.str()});
  EXPECT_FALSE(result.exit_code.has_value());
  EXPECT_EQ(result.options.values.at("openai_model"), "gpt-4o");
  EXPECT_EQ(result.options.values.at("openai_base_url"), "http://localhost:8080/v1?a=b");
  EXPECT_EQ(result.options.config_file, file.path());
}

TEST_F(OptionsTest, ReadsToml) {
  TempFile file(
      "# a comment\nopenai_model = \"gpt-4o\"  # trailing comment\n"
      "llm_provider = 'anthropic'\n",
      ".toml");
  const auto result = Run({"--config", file.str()});
  EXPECT_FALSE(result.exit_code.has_value());
  EXPECT_EQ(result.options.values.at("openai_model"), "gpt-4o");
  EXPECT_EQ(result.options.values.at("llm_provider"), "anthropic");
}

TEST_F(OptionsTest, FileSavedByAWindowsEditor) {
  TempFile file("\xEF\xBB\xBF" "openai_model = bom\r\nmodel_dir = crlf\r\n", ".ini");
  const auto result = Run({"--config", file.str()});
  EXPECT_FALSE(result.exit_code.has_value());
  EXPECT_EQ(result.options.values.at("openai_model"), "bom");
  EXPECT_EQ(result.options.values.at("model_dir"), "crlf");
}

TEST_F(OptionsTest, ARepeatedKeyTakesTheLastValue) {
  TempFile file("openai_model = first\nopenai_model = second\n", ".ini");
  EXPECT_EQ(Run({"--config", file.str()}).options.values.at("openai_model"), "second");
}

TEST_F(OptionsTest, AMisspelledKeyInTheFileIsAnError) {
  TempFile file("llm_modle = x\n", ".ini");
  const auto result = Run({"--config", file.str()});
  ASSERT_TRUE(result.exit_code.has_value());
  EXPECT_NE(*result.exit_code, 0);
  EXPECT_NE(err_.str().find("llm_modle"), std::string::npos);
}

TEST_F(OptionsTest, ALineWithoutEqualsIsAnError) {
  TempFile file("openai_model = x\njunk\n", ".ini");
  const auto result = Run({"--config", file.str()});
  ASSERT_TRUE(result.exit_code.has_value());
  EXPECT_NE(*result.exit_code, 0);
}

TEST_F(OptionsTest, AMissingExplicitConfigFileIsAnError) {
  const auto missing = std::filesystem::temp_directory_path() / "comfystudy_no_such_dir" / "c.ini";
  const auto result = Run({"--config", missing.string()});
  ASSERT_TRUE(result.exit_code.has_value());
  EXPECT_NE(*result.exit_code, 0);
}

TEST_F(OptionsTest, WithoutConfigTheFirstExistingCandidateIsUsed) {
  TempFile first("openai_model = first\nmodel_dir = only-in-first\n", ".toml");
  TempFile second("openai_model = second\nllm_provider = only-in-second\n", ".ini");
  const std::filesystem::path missing = "no_such_file.toml";
  const auto result = Run({}, {missing, first.path(), second.path()});
  EXPECT_EQ(result.options.config_file, first.path());
  EXPECT_EQ(result.options.values.at("openai_model"), "first");
  EXPECT_EQ(result.options.values.at("model_dir"), "only-in-first");
  EXPECT_EQ(result.options.values.at("llm_provider"), "openai");  // not merged from the second
}

TEST_F(OptionsTest, ExplicitConfigWinsOverTheCandidates) {
  TempFile candidate("openai_model = candidate\n", ".ini");
  TempFile explicit_file("openai_model = explicit\n", ".ini");
  const auto result = Run({"--config", explicit_file.str()}, {candidate.path()});
  EXPECT_EQ(result.options.values.at("openai_model"), "explicit");
  EXPECT_EQ(result.options.config_file, explicit_file.path());
}

TEST_F(OptionsTest, VerboseFlag) {
  EXPECT_TRUE(Run({"--verbose"}).options.verbose);
  EXPECT_TRUE(Run({"-v"}).options.verbose);
}

#if defined(_WIN32)
TEST_F(OptionsTest, DependencyCheckFlag) {
  EXPECT_TRUE(Run({"--dependency-check"}).options.dependency_check);
}
#endif

TEST_F(OptionsTest, UnknownOptionIsAnError) {
  const auto result = Run({"--bogus"});
  ASSERT_TRUE(result.exit_code.has_value());
  EXPECT_NE(*result.exit_code, 0);
  EXPECT_NE(err_.str().find("--bogus"), std::string::npos);
}

TEST_F(OptionsTest, HelpListsTheOptionsAndStopsWithExitCodeZero) {
  const auto result = Run({"--help"});
  ASSERT_TRUE(result.exit_code.has_value());
  EXPECT_EQ(*result.exit_code, 0);
  EXPECT_NE(out_.str().find("--openai_model"), std::string::npos);
  EXPECT_NE(out_.str().find("--config"), std::string::npos);
  EXPECT_NE(out_.str().find("OPENAI_MODEL"), std::string::npos);  // the environment variable
}

#if defined(_WIN32)
TEST(CommandLine, WideCharactersBecomeUtf8) {
  EXPECT_EQ(options::WideToUtf8(L"plain"), "plain");
  EXPECT_EQ(options::WideToUtf8(L"你好"), "\xE4\xBD\xA0\xE5\xA5\xBD");
  EXPECT_EQ(options::WideToUtf8(L"\U0001F600"), "\xF0\x9F\x98\x80");
  EXPECT_EQ(options::WideToUtf8(L""), "");
}
#endif

TEST_F(OptionsTest, ChatFlag) {
  const auto result = Run({"--chat"});
  EXPECT_FALSE(result.exit_code.has_value());
  EXPECT_TRUE(result.options.chat);
  EXPECT_FALSE(Run({}).options.chat);
}

TEST_F(OptionsTest, ToolsFlag) {
  EXPECT_TRUE(Run({"--chat", "--tools"}).options.tools);
  EXPECT_FALSE(Run({"--chat"}).options.tools);
  EXPECT_TRUE(Run({"--chat", "--show-context"}).options.show_context);
  EXPECT_FALSE(Run({"--chat"}).options.show_context);
}

TEST_F(OptionsTest, ChatAndPromptTogetherAreAnError) {
  const auto result = Run({"--chat", "--prompt", "hi"});
  ASSERT_TRUE(result.exit_code.has_value());
  EXPECT_NE(*result.exit_code, 0);
  EXPECT_NE(err_.str().find("--chat"), std::string::npos);
  EXPECT_NE(err_.str().find("--prompt"), std::string::npos);
}

TEST_F(OptionsTest, ParsingNeverWritesSecretValuesToTheOutput) {
  TempFile file("openai_api_key = topsecretvalue123\n", ".ini");
  Run({"--config", file.str(), "--discord_token", "anothersecret456"});
  EXPECT_EQ(out_.str().find("topsecretvalue123"), std::string::npos);
  EXPECT_EQ(err_.str().find("topsecretvalue123"), std::string::npos);
  EXPECT_EQ(err_.str().find("anothersecret456"), std::string::npos);
}
