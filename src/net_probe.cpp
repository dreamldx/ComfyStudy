#include "net_probe.h"

#include <curl/curl.h>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

std::string CurlVersion() { return curl_version(); }

std::string JsonRoundTrip(const std::string& text) {
  return nlohmann::json::parse(text).dump();
}

std::string NameFromArgs(const std::vector<std::string>& args) {
  CLI::App app;
  std::string name;
  app.add_option("--name", name);
  std::vector<std::string> reversed(args.rbegin(), args.rend());  // CLI11 wants them reversed
  app.parse(reversed);
  return name;
}
