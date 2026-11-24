#include "http_client.h"

#include <algorithm>
#include <cctype>

namespace http {
namespace {

std::string Lower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

}  // namespace

std::vector<Header> Redacted(const std::vector<Header>& headers) {
  std::vector<Header> result;
  result.reserve(headers.size());
  for (const Header& header : headers) {
    const std::string name = Lower(header.first);
    if (name == "authorization") {
      // Keep the scheme ("Bearer", "Basic"): it says what kind of credential it was.
      const std::size_t space = header.second.find(' ');
      result.push_back({header.first, space == std::string::npos ? "***" : header.second.substr(0, space) + " ***"});
    } else if (name == "x-api-key" || name == "api-key" || name == "proxy-authorization" || name == "cookie") {
      result.push_back({header.first, "***"});
    } else {
      result.push_back(header);
    }
  }
  return result;
}

}  // namespace http
