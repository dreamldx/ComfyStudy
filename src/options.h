#pragma once

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "config.h"

namespace options {

struct Options {
  config::Values values;                          // every known key, resolved (see Parse)
  bool verbose = false;                           // -v / --verbose
#if defined(_WIN32)
  bool dependency_check = false;                  // --dependency-check (Windows only)
#endif
  std::optional<std::filesystem::path> config_file;  // the config file that was read, if any
};

struct Result {
  Options options;
  // Set when the program should stop right away: --help (0) or a bad command line or config
  // file (non-zero). The usage or the error message has already been written to out / err.
  std::optional<int> exit_code;
};

// Resolves every known setting from, in increasing priority:
//   built-in default  <  environment variable  <  config file  <  command line
// An empty value counts as "not set" at every level, so an empty `token =` in a config file does
// not hide a token that comes from the environment.
//
// `args` are the command-line arguments after the program name. The config file is the one given
// with --config; without it, the first existing file in `config_candidates` (no merging). It may
// be written as INI or as TOML (a flat list of "name = value" lines). A setting has the same
// name in the file and on the command line: "llm_model = x" is "--llm_model x".
Result Parse(const std::vector<std::string>& args,
             const std::vector<std::filesystem::path>& config_candidates, std::ostream& out,
             std::ostream& err);

}  // namespace options
