#pragma once

#include <boost/asio.hpp>

#include "http_client.h"

namespace http {

// How a request gets to a server and the answer comes back. The rest of the program talks to this
// interface, never to libcurl, so a test can put a fake in its place and a recording can be played
// back instead of the real service.
//
//   Post is a coroutine: the caller `co_await`s it, is suspended until the whole answer is there,
//   and resumes on its own executor. Many requests can be under way at the same time.
//
// What every implementation promises:
//   - An answer with an error status (401, 500, ...) is a Response, not an exception.
//   - When no answer arrives, Post throws http::Error. error.kind() says why, and error.SafeToRetry()
//     whether sending the request again cannot make anything happen twice.
//   - When the awaiting coroutine is cancelled, the request is cancelled too, and Post throws
//     boost::system::system_error with boost::asio::error::operation_aborted.
//   - Destroying the transport ends the requests that are still running with an http::Error.
//   - Redirects are not followed: an API key in a header must not be sent to a place the caller did
//     not name.
class Transport {
 public:
  virtual ~Transport() = default;

  virtual boost::asio::awaitable<Response> Post(PostRequest request) = 0;
};

}  // namespace http
