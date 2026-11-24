// Three ways to send an HTTP request from a coroutine, one of the choices this branch compares. They
// all have the same shape:
//
//     boost::asio::awaitable<http::Response> Post(http::PostRequest request);
//
// The coroutine that awaits Post is suspended until the answer is there, and its thread is free to run
// other coroutines in the meantime. Post throws http::Error when no answer arrives, and
// boost::system::system_error (operation_aborted) when the coroutine was cancelled.
//
//   PoolClient        a pool of threads, each with its own Session; the blocking call runs in the pool
//   CurlThreadClient  ONE extra thread that drives libcurl's multi interface for all requests
//   SocketClient      no extra thread: libcurl's sockets are handed to the event loop
//
// The Options (timeouts, HTTP version, Share) are given when the client is made and apply to every
// request.
#pragma once

#include <boost/asio.hpp>

#include <cstddef>
#include <memory>

#include "http_client.h"

namespace http {

// Runs each request on a thread of its own pool, with a Session (and so a connection) that the pool
// keeps. At most `threads` requests are under way at the same time; the others wait for a free thread.
// A request that is running cannot be interrupted: cancelling the coroutine takes effect when the
// request ends. The destructor waits for the running requests.
class PoolClient {
 public:
  explicit PoolClient(std::size_t threads, Options options = {});
  ~PoolClient();
  PoolClient(const PoolClient&) = delete;
  PoolClient& operator=(const PoolClient&) = delete;

  boost::asio::awaitable<Response> Post(PostRequest request);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// One thread runs curl_multi_poll for all requests; when one is done, the coroutine that waits for it
// resumes on its own executor. Any number of requests can be under way (they share the connections
// the HTTP version allows). Safe to use from several threads. Cancelling the coroutine cancels the
// request. The destructor cancels the requests that are still running.
class CurlThreadClient {
 public:
  explicit CurlThreadClient(Options options = {});
  ~CurlThreadClient();
  CurlThreadClient(const CurlThreadClient&) = delete;
  CurlThreadClient& operator=(const CurlThreadClient&) = delete;

  boost::asio::awaitable<Response> Post(PostRequest request);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// libcurl's multi interface driven by the event loop itself (curl_multi_socket_action): libcurl says
// which sockets it wants to watch and when it wants to be woken, and the event loop does the waiting.
// No thread besides the event loop. All calls, and the destructor, must happen on `executor`, which
// must run on one thread at a time (an io_context run by one thread, or a strand).
class SocketClient {
 public:
  SocketClient(boost::asio::any_io_executor executor, Options options = {});
  ~SocketClient();
  SocketClient(const SocketClient&) = delete;
  SocketClient& operator=(const SocketClient&) = delete;

  boost::asio::awaitable<Response> Post(PostRequest request);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace http
