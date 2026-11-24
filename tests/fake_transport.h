#pragma once

#include <boost/asio.hpp>

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include "transport.h"

// A Transport that answers without a network, for tests of code that sends requests. It understands
// the same few paths as the test server of tests/test_transport.cpp, so one set of tests can run
// against both (the contract tests):
//
//   POST /echo         answers with "<body>|<X-Test header>|<Content-Type header>"
//   POST /slow?ms=N    waits N milliseconds, then answers with the body
//   POST /fail         answers with status 500 and the body "no"
//   a host named unreachable.invalid: throws http::Error(kConnect), like a refused connection
//
// A request that would take longer than options.timeout ends with http::Error(kTimeout) at that
// time. It remembers the requests it has seen.
class FakeTransport final : public http::Transport {
 public:
  explicit FakeTransport(http::Options options = {}) : options_(options) {}

  boost::asio::awaitable<http::Response> Post(http::PostRequest request) override {
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      seen_.push_back(request);
    }
    if (request.url.find("unreachable.invalid") != std::string::npos) {
      throw http::Error("request failed: could not connect to the server", http::ErrorKind::kConnect);
    }
    std::chrono::milliseconds delay{0};
    const std::size_t ms = request.url.find("?ms=");
    if (ms != std::string::npos) delay = std::chrono::milliseconds(std::stoi(request.url.substr(ms + 4)));
    if (delay > options_.timeout) {
      co_await Wait(options_.timeout);
      throw http::Error("request failed: timed out", http::ErrorKind::kTimeout);
    }
    co_await Wait(delay);

    if (request.url.find("/fail") != std::string::npos) co_return http::Response{500, "no"};
    if (request.url.find("/echo") != std::string::npos) {
      co_return http::Response{200, request.body + "|" + Header(request, "X-Test") + "|" + Header(request, "Content-Type")};
    }
    co_return http::Response{200, request.body};
  }

  // The requests this transport has been asked to send, in the order they arrived.
  std::vector<http::PostRequest> seen() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return seen_;
  }

 private:
  static boost::asio::awaitable<void> Wait(std::chrono::milliseconds duration) {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, duration);
    co_await timer.async_wait(boost::asio::use_awaitable);  // cancelling the coroutine ends the wait
  }

  static std::string Header(const http::PostRequest& request, const std::string& name) {
    for (const http::Header& header : request.headers) {
      if (header.first == name) return header.second;
    }
    return "";
  }

  http::Options options_;
  mutable std::mutex mutex_;
  std::vector<http::PostRequest> seen_;
};
