#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "config.h"
#include "greeting.h"
#include "net_probe.h"
#include "options.h"

#if defined(_WIN32)
#include "dll_info.h"
#endif

int main(int argc, char** argv) {
#if defined(_WIN32)
  // First thing: from now on DLLs are only loaded from this program's directory and System32.
  try {
    dllinfo::InitSearchPath();
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
#endif

  // Flush std::cout after every output, so its lines and spdlog's lines (which are written
  // immediately) stay in the order the program produced them even when stdout is redirected.
  std::cout << std::unitbuf;

  const std::vector<std::string> args(argv + 1, argv + argc);
  const auto candidates =
      config::ConfigCandidates(config::ExeDirectory(), std::filesystem::current_path());
  const options::Result parsed = options::Parse(args, candidates, std::cout, std::cerr);
  if (parsed.exit_code) return *parsed.exit_code;  // --help, or a bad command line or config file

#if defined(_WIN32)
  if (parsed.options.dependency_check) {
    dllinfo::PrintDependencies(std::cout, dllinfo::StdoutSupportsColor());
    return 0;
  }
#endif

  spdlog::set_pattern("[%l] %v");
  if (parsed.options.verbose) spdlog::set_level(spdlog::level::debug);

  std::cout << Greeting() << '\n';
  std::cout << CurlVersion() << '\n';

  for (const auto& candidate : candidates) {
    spdlog::debug("config candidate {}: {}", candidate.string(),
                  std::filesystem::exists(candidate) ? "found" : "not found");
  }
  if (parsed.options.config_file) {
    spdlog::info("loaded {}", parsed.options.config_file->string());
  } else {
    spdlog::warn(
        "no config file found (looked next to the executable and in the current directory); "
        "using the environment and the defaults only");
  }

  config::Print(std::cout, parsed.options.values);
  return 0;
}
