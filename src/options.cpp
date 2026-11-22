#include "options.h"

#include <memory>
#include <sstream>

#include <CLI/CLI.hpp>

namespace options {
namespace {

constexpr std::string_view kBom = "\xEF\xBB\xBF";

// CLI11 reads a UTF-8 BOM (which some Windows editors put at the start of a file) as part of the
// first name, so "discord_token" would not be recognized. This reader drops it first. The base
// class understands both INI and TOML syntax: '#' and ';' comments, "name = value", and values
// that are quoted or not.
class ConfigReader : public CLI::ConfigBase {
 public:
  std::vector<CLI::ConfigItem> from_config(std::istream& input) const override {
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (text.compare(0, kBom.size(), kBom) == 0) text.erase(0, kBom.size());
    std::istringstream stream(text);
    return CLI::ConfigBase::from_config(stream);
  }
};

std::string DefaultConfigFile(const std::vector<std::filesystem::path>& candidates) {
  for (const auto& candidate : candidates) {
    std::error_code ignored;
    if (std::filesystem::is_regular_file(candidate, ignored)) return candidate.string();
  }
  return {};
}

}  // namespace

Result Parse(const std::vector<std::string>& args,
             const std::vector<std::filesystem::path>& config_candidates, std::ostream& out,
             std::ostream& err) {
  Result result;
  CLI::App app{"ComfyStudy: an LLM-driven image generation bot", "comfystudy"};

  for (const config::Key& key : config::KnownKeys()) {
    app.add_option("--" + key.name, result.options.values[key.name],
                   "Environment variable: " + key.env)
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);  // a repeated key: last wins
  }
  app.add_flag("-v,--verbose", result.options.verbose, "Show debug messages");
#if defined(_WIN32)
  app.add_flag("--dependency-check", result.options.dependency_check,
               "List the DLLs this program loaded (where from, version, vcpkg package) and exit");
#endif

  CLI::Option* config_option = app.set_config(
      "--config", DefaultConfigFile(config_candidates), "Read settings from this INI or TOML file");
  app.config_formatter(std::make_shared<ConfigReader>());
  app.allow_config_extras(CLI::config_extras_mode::error);  // a misspelled key is an error

  try {
    std::vector<std::string> reversed(args.rbegin(), args.rend());  // CLI11 wants them reversed
    app.parse(reversed);
  } catch (const CLI::ParseError& e) {
    result.exit_code = app.exit(e, out, err);
    return result;
  }

  if (config_option->count() > 0) result.options.config_file = config_option->as<std::string>();

  // The command line and the config file are already applied (CLI11 gives the command line
  // priority). What is still empty falls back to the environment, then to the built-in default.
  for (const config::Key& key : config::KnownKeys()) {
    std::string& value = result.options.values[key.name];
    if (value.empty()) value = config::Get(key.env).value_or(key.default_value);
  }
  return result;
}

}  // namespace options
