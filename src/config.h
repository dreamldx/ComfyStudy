#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace config {

// One setting. `name` is how it is written in a config file ("llm_model = ...") and on the command
// line ("--llm_model"); `env` is the environment variable that can also provide it.
struct Key {
  std::string name;
  std::string env;
  std::string default_value;  // used when nothing else provides a value; empty means "no default"
  bool secret;                // true when the name ends with "_key" or "_token"
};

// The settings ComfyStudy knows about, in a fixed order.
const std::vector<Key>& KnownKeys();

// Resolved settings: one entry per known key, keyed by Key::name; an empty string means "not set".
using Values = std::map<std::string, std::string>;

// "llm_model" -> "LLM_MODEL": the same name in upper case.
std::string EnvName(std::string_view name);

// Value of an environment variable; an empty value counts as not set.
std::optional<std::string> Get(std::string_view env_name);

// "(unset)" for an empty value, "****" if shorter than 8 characters,
// otherwise the first 3 characters followed by "***". Never returns more than 3 characters of a secret.
std::string MaskSecret(std::string_view value);

// "name=value", "name=<masked>" for secrets, "name=(unset)" for an empty or missing value.
std::string Describe(const Values& values, const Key& key);

// One Describe() line per known key.
void Print(std::ostream& out, const Values& values);

// Directory that contains the running executable.
std::filesystem::path ExeDirectory();

// Where a config file is looked for when --config is not given: in the executable's directory
// first, then in the current working directory; in each, config.toml before config.ini.
std::vector<std::filesystem::path> ConfigCandidates(const std::filesystem::path& exe_dir,
                                                    const std::filesystem::path& cwd);

}  // namespace config
