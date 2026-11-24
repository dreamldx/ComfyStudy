#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace http {

// Why a request ended without an answer. The kind tells the caller what it may do next.
enum class ErrorKind {
  kConnect,  // the connection could not be made (name lookup, TCP, TLS): the request was never sent
  kTimeout,  // no complete answer in time: the server may still be working on the request
  kLost,     // the connection broke after the request was sent: the server may have processed it
  kOther,    // anything else (a bad URL, an option libcurl refused, the transport was shut down)
};

// The request could not be completed at all. A server that answers with an error status is NOT an
// error here; see Response.
class Error : public std::runtime_error {
 public:
  explicit Error(const std::string& what, ErrorKind kind = ErrorKind::kOther)
      : std::runtime_error(what), kind_(kind) {}

  ErrorKind kind() const { return kind_; }

  // True when sending the request again cannot make anything happen twice, because the first one
  // never left this machine. After a timeout or a lost connection the answer is unknown, and for a
  // request that costs money the caller has to decide.
  bool SafeToRetry() const { return kind_ == ErrorKind::kConnect; }

 private:
  ErrorKind kind_;
};

using Header = std::pair<std::string, std::string>;  // name, value

// One HTTP POST: where, with which headers (for example Content-Type and the authorization), what.
struct PostRequest {
  std::string url;
  std::vector<Header> headers;
  std::string body;
};

struct Response {
  long status = 0;   // the HTTP status code, e.g. 200 or 401
  std::string body;  // the body as received, not interpreted
};

struct Options {
  std::chrono::milliseconds connect_timeout{10000};  // for establishing the connection
  std::chrono::milliseconds timeout{120000};         // for the whole request, including the answer
};

// The same headers with the secrets blanked out, for logs: the value of Authorization keeps its
// first word ("Bearer ***"), the values of x-api-key, api-key, Proxy-Authorization and Cookie
// become "***". Anything that writes headers to a log or an error message goes through this.
std::vector<Header> Redacted(const std::vector<Header>& headers);

}  // namespace http
