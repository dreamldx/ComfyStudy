#pragma once

#include <string>
#include <vector>

// Version string of the linked libcurl, e.g. "libcurl/8.23.0 ...".
std::string CurlVersion();

// Parses `text` as JSON and returns it re-serialized in compact form.
// Throws json::parse_error when `text` is not valid JSON.
std::string JsonRoundTrip(const std::string& text);

// Parses `args` (the command-line arguments after the program name) with CLI11 and returns the
// value of "--name", or "" when it is not given.
// Throws CLI::ParseError when an argument is not recognized.
std::string NameFromArgs(const std::vector<std::string>& args);
