// Tests for the three coroutine clients of src/async_http.h. The same tests run against all of them
// (TYPED_TEST), against a small server on this machine; the ones that only make sense for the clients
// that can interrupt a running request are in the second suite.
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "async_http.h"
#include "event_loop.h"
#include "test_server.h"

namespace asio = boost::asio;
namespace loop = comfystudy::loop;
using namespace std::chrono_literals;

namespace {

// How to make each client for the tests.
template <class Client>
struct Maker;

template <>
struct Maker<http::PoolClient> {
  static std::unique_ptr<http::PoolClient> Make(asio::io_context&, http::Options options) {
    return std::make_unique<http::PoolClient>(16, std::move(options));
  }
};
template <>
struct Maker<http::CurlThreadClient> {
  static std::unique_ptr<http::CurlThreadClient> Make(asio::io_context&, http::Options options) {
    return std::make_unique<http::CurlThreadClient>(std::move(options));
  }
};
template <>
struct Maker<http::SocketClient> {
  static std::unique_ptr<http::SocketClient> Make(asio::io_context& io, http::Options options) {
    return std::make_unique<http::SocketClient>(io.get_executor(), std::move(options));
  }
};

template <class Client>
class AsyncHttpTest : public ::testing::Test {
 protected:
  void SetUp() override {
    server_.server().new_task_queue = [] { return new httplib::ThreadPool(64); };
    server_.server().Post("/echo", [](const httplib::Request& req, httplib::Response& res) { res.set_content(req.body, "text/plain"); });
    // /slow?ms=N answers after N milliseconds with the request body.
    server_.server().Post("/slow", [](const httplib::Request& req, httplib::Response& res) {
      std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(req.get_param_value("ms"))));
      res.set_content(req.body, "text/plain");
    });
    server_.server().Post("/fail", [](const httplib::Request&, httplib::Response& res) {
      res.status = 500;
      res.set_content("no", "text/plain");
    });
    server_.Start();
  }

  std::unique_ptr<Client> MakeClient(http::Options options = {}) {
    options.version = http::Version::kHttp1_1;  // the test server speaks nothing else
    return Maker<Client>::Make(io_, std::move(options));
  }

  http::PostRequest Request(const std::string& path, const std::string& body) {
    return {server_.Url(path), {{"Content-Type", "text/plain"}}, body};
  }

  asio::io_context io_;
  TestServer server_;
};

using AllClients = ::testing::Types<http::PoolClient, http::CurlThreadClient, http::SocketClient>;
TYPED_TEST_SUITE(AsyncHttpTest, AllClients);

}  // namespace

TYPED_TEST(AsyncHttpTest, ReturnsTheAnswer) {
  auto client = this->MakeClient();
  auto job = [&]() -> asio::awaitable<http::Response> { co_return co_await client->Post(this->Request("/echo", "hello")); };
  const http::Response response = loop::RunBlocking(this->io_, job());
  EXPECT_EQ(response.status, 200);
  EXPECT_EQ(response.body, "hello");
  EXPECT_EQ(response.timing.protocol, "HTTP/1.1");
  EXPECT_EQ(response.timing.new_connections, 1);
}

TYPED_TEST(AsyncHttpTest, AnErrorStatusIsStillAnAnswer) {
  auto client = this->MakeClient();
  auto job = [&]() -> asio::awaitable<http::Response> { co_return co_await client->Post(this->Request("/fail", "")); };
  const http::Response response = loop::RunBlocking(this->io_, job());
  EXPECT_EQ(response.status, 500);
  EXPECT_EQ(response.body, "no");
}

TYPED_TEST(AsyncHttpTest, TwentyRequestsAtTheSameTimeEachGetTheirOwnAnswer) {
  auto client = this->MakeClient();
  std::vector<std::optional<std::string>> bodies(20);
  auto job = [&](int i) -> asio::awaitable<void> {
    const http::Response response = co_await client->Post(this->Request("/slow?ms=300", "answer " + std::to_string(i)));
    bodies[static_cast<std::size_t>(i)] = response.body;
  };
  const auto start = loop::Clock::now();
  for (int i = 0; i < 20; ++i) asio::co_spawn(this->io_, job(i), asio::detached);
  this->io_.run();
  const auto elapsed = loop::Clock::now() - start;
  for (int i = 0; i < 20; ++i) EXPECT_EQ(bodies[static_cast<std::size_t>(i)], "answer " + std::to_string(i));
  EXPECT_GE(elapsed, 300ms);
  EXPECT_LT(elapsed, 1500ms);  // 20 requests one after the other would take 6 seconds
}

TYPED_TEST(AsyncHttpTest, ARequestThatFailsDoesNotAffectTheOthers) {
  auto client = this->MakeClient();
  std::vector<std::string> outcome(5);
  auto job = [&](int i) -> asio::awaitable<void> {
    // The request with index 2 goes to a port where nothing listens.
    const http::PostRequest request = i == 2 ? http::PostRequest{"http://127.0.0.1:1/", {}, "x"} : this->Request("/echo", std::to_string(i));
    try {
      outcome[static_cast<std::size_t>(i)] = (co_await client->Post(request)).body;
    } catch (const http::Error&) {
      outcome[static_cast<std::size_t>(i)] = "error";
    }
  };
  for (int i = 0; i < 5; ++i) asio::co_spawn(this->io_, job(i), asio::detached);
  this->io_.run();
  EXPECT_EQ(outcome, (std::vector<std::string>{"0", "1", "error", "3", "4"}));
}

TYPED_TEST(AsyncHttpTest, ATimeoutFailsOnlyThatRequest) {
  http::Options options;
  options.timeout = 300ms;
  auto client = this->MakeClient(options);
  std::vector<std::string> outcome(3);
  auto job = [&](int i) -> asio::awaitable<void> {
    try {
      outcome[static_cast<std::size_t>(i)] = (co_await client->Post(this->Request(i == 1 ? "/slow?ms=1500" : "/slow?ms=50", std::to_string(i)))).body;
    } catch (const http::Error&) {
      outcome[static_cast<std::size_t>(i)] = "timeout";
    }
  };
  for (int i = 0; i < 3; ++i) asio::co_spawn(this->io_, job(i), asio::detached);
  this->io_.run();
  EXPECT_EQ(outcome, (std::vector<std::string>{"0", "timeout", "2"}));
}

TYPED_TEST(AsyncHttpTest, TheCoroutineResumesOnItsOwnThread) {
  auto client = this->MakeClient();
  const auto before = std::this_thread::get_id();
  std::thread::id after;
  auto job = [&]() -> asio::awaitable<void> {
    co_await client->Post(this->Request("/echo", "x"));
    after = std::this_thread::get_id();
  };
  loop::RunBlocking(this->io_, job());
  EXPECT_EQ(after, before);
}

// --- the clients that can interrupt a request ----------------------------------------------------------

template <class Client>
class InterruptibleTest : public AsyncHttpTest<Client> {};
using Interruptible = ::testing::Types<http::CurlThreadClient, http::SocketClient>;
TYPED_TEST_SUITE(InterruptibleTest, Interruptible);

TYPED_TEST(InterruptibleTest, CancellingTheCoroutineCancelsTheRequest) {
  auto client = this->MakeClient();
  asio::cancellation_signal signal;
  const auto start = loop::Clock::now();
  auto answer = asio::co_spawn(
      this->io_,
      [&]() -> asio::awaitable<http::Response> { co_return co_await client->Post(this->Request("/slow?ms=1500", "x")); },
      asio::bind_cancellation_slot(signal.slot(), asio::use_future));
  asio::steady_timer later(this->io_, 100ms);
  later.async_wait([&](const boost::system::error_code&) { signal.emit(asio::cancellation_type::all); });
  this->io_.run();
  EXPECT_LT(loop::Clock::now() - start, 1000ms);  // the answer would have taken 1500 ms
  try {
    answer.get();
    FAIL() << "the request was not cancelled";
  } catch (const boost::system::system_error& e) {
    EXPECT_EQ(e.code(), asio::error::operation_aborted);
  }
}

TYPED_TEST(InterruptibleTest, DestroyingTheClientEndsTheRequestsThatAreStillRunning) {
  auto client = this->MakeClient();
  std::vector<std::string> outcome(3);
  auto job = [&](int i) -> asio::awaitable<void> {
    try {
      co_await client->Post(this->Request("/slow?ms=1000", "x"));
      outcome[static_cast<std::size_t>(i)] = "answer";
    } catch (const http::Error&) {
      outcome[static_cast<std::size_t>(i)] = "error";
    }
  };
  for (int i = 0; i < 3; ++i) asio::co_spawn(this->io_, job(i), asio::detached);
  asio::steady_timer later(this->io_, 100ms);
  later.async_wait([&](const boost::system::error_code&) { client.reset(); });
  this->io_.run();
  EXPECT_EQ(outcome, (std::vector<std::string>{"error", "error", "error"}));
}

// --- the pool cannot interrupt ------------------------------------------------------------------------

TEST(PoolClientLimits, ARunningRequestIsNotInterruptedByCancelling) {
  asio::io_context io;
  TestServer server;
  server.server().Post("/slow", [](const httplib::Request&, httplib::Response& res) {
    std::this_thread::sleep_for(600ms);
    res.set_content("late", "text/plain");
  });
  server.Start();
  http::Options options;
  options.version = http::Version::kHttp1_1;
  http::PoolClient client(2, options);
  asio::cancellation_signal signal;
  const auto start = loop::Clock::now();
  auto answer = asio::co_spawn(
      io, [&]() -> asio::awaitable<http::Response> { co_return co_await client.Post({server.Url("/slow"), {}, "x"}); },
      asio::bind_cancellation_slot(signal.slot(), asio::use_future));
  asio::steady_timer later(io, 100ms);
  later.async_wait([&](const boost::system::error_code&) { signal.emit(asio::cancellation_type::all); });
  io.run();
  // The thread was inside libcurl: nothing could stop it, so the call only ended with the request.
  EXPECT_GE(loop::Clock::now() - start, 500ms);
  try {
    EXPECT_EQ(answer.get().body, "late");
  } catch (const boost::system::system_error&) {
    SUCCEED();  // or it ends with the cancellation error once the request is over: both are fine
  }
}
