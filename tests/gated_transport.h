#pragma once

#include <boost/asio.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "transport.h"

// A Transport whose requests do not come back until the test lets them: Count() says how many have arrived,
// Release(i, response) answers the i-th one. A request whose coroutine is cancelled before it is released ends
// with the cancellation, like a request that is stopped on the network. For tests that need requests to be in
// flight at the same time. Everything runs on the thread of the io_context.
class GatedTransport final : public http::Transport {
 public:
  boost::asio::awaitable<http::Response> Post(http::PostRequest request) override {
    auto entry = std::make_shared<Entry>(co_await boost::asio::this_coro::executor);
    entry->request = std::move(request);
    entries_.push_back(entry);
    boost::system::error_code ignored;
    co_await entry->gate.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ignored));
    if (!entry->response) throw boost::system::system_error(boost::asio::error::operation_aborted);  // cancelled, not released
    co_return *entry->response;
  }

  std::size_t Count() const { return entries_.size(); }
  const std::string& Body(std::size_t index) const { return entries_.at(index)->request.body; }

  void Release(std::size_t index, http::Response response) {
    entries_.at(index)->response = std::move(response);
    entries_.at(index)->gate.cancel();
  }

 private:
  struct Entry {
    explicit Entry(const boost::asio::any_io_executor& executor) : gate(executor, boost::asio::steady_timer::time_point::max()) {}
    boost::asio::steady_timer gate;
    http::PostRequest request;
    std::optional<http::Response> response;
  };
  std::vector<std::shared_ptr<Entry>> entries_;
};
