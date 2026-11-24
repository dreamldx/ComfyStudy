#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
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
// How long each phase of a request took, as libcurl reports it. The times are cumulative: each one
// counts from the start of the request, so the time a phase took on its own is the difference to
// the one before it. A phase that did not happen (for example the connection of a reused
// connection) is 0.
struct Timing {
  std::chrono::microseconds name_lookup{0};  // the domain name has been resolved
  std::chrono::microseconds connect{0};      // the TCP connection is established
  std::chrono::microseconds tls{0};          // the TLS handshake is done (0 for plain http)
  std::chrono::microseconds first_byte{0};   // the first byte of the answer has arrived
  std::chrono::microseconds total{0};        // the whole answer has arrived
  long new_connections = 0;                  // 0 when an existing connection was reused
  std::string protocol;                      // "HTTP/1.1", "HTTP/2", ...
};

struct Response {
  long status = 0;   // the HTTP status code, e.g. 200 or 401
  std::string body;  // the body as received, not interpreted
  Timing timing;
};

// Which HTTP version to speak. HTTP/2 is only used over TLS; plain http always uses HTTP/1.1.
enum class Version {
  kAny,      // let libcurl choose: HTTP/2 when the server offers it (needs libcurl built with it)
  kHttp1_1,  // never HTTP/2
  kHttp2,    // HTTP/2 over TLS, HTTP/1.1 otherwise
};

// Handles that use the same Share use the same caches (see Share).
class Share;

struct Options {
  std::chrono::milliseconds connect_timeout{10000};  // for establishing the connection
  std::chrono::milliseconds timeout{120000};         // for the whole request, including the answer
  Version version = Version::kAny;
  const Share* share = nullptr;  // not owned; must outlive every request that uses it
};

// Sends `body` to `url` with HTTP POST and the given headers (for example Content-Type and the
// authorization header) and waits for the whole answer. Every call starts with a new libcurl
// handle, so it also makes a new connection (unless a Share lets it reuse one).
// Redirects are NOT followed: an API key in a header must not be sent to a place the caller
// did not name. Throws Error when no answer arrives.
Response Post(const std::string& url, const std::vector<Header>& headers, const std::string& body,
              const Options& options = {});

// One libcurl handle that is kept between requests. The connection of an earlier request stays
// open, so the next request to the same server skips the name lookup, the TCP connection and the
// TLS handshake. Not thread-safe: use one Session per thread.
class Session {
 public:
  explicit Session(Options options = {});
  ~Session();
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;

  Response Post(const std::string& url, const std::vector<Header>& headers, const std::string& body);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Caches that several handles use together: the results of name lookups and TLS sessions (so a new
// connection to a known server can skip the full TLS handshake). It does NOT share open connections:
// libcurl 8.23 crashed when threads shared its connection cache. libcurl calls back to lock the
// caches, because handles on different threads may use them at the same time; this class provides
// the locks. Create one, point Options::share at it.
class Share {
 public:
  Share();
  ~Share();
  Share(const Share&) = delete;
  Share& operator=(const Share&) = delete;

  void* handle() const;  // the CURLSH*, for the implementation

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

struct PostRequest {
  std::string url;
  std::vector<Header> headers;
  std::string body;
};

// The result of one request of PostMany: the answer, or the reason there was none.
struct PostResult {
  std::optional<Response> response;
  std::string error;  // set when `response` is empty
};

// Sends all the requests from ONE thread, with up to `max_in_flight` of them under way at the same
// time (libcurl's multi interface). The results are in the order of the requests. A request that
// fails or times out does not affect the others. Never throws for a failed request.
std::vector<PostResult> PostMany(const std::vector<PostRequest>& requests, std::size_t max_in_flight,
                                 const Options& options = {});

// The same headers with the secrets blanked out, for logs: the value of Authorization keeps its
// first word ("Bearer ***"), the values of x-api-key, api-key, Proxy-Authorization and Cookie
// become "***". Anything that writes headers to a log or an error message goes through this.
std::vector<Header> Redacted(const std::vector<Header>& headers);

}  // namespace http
