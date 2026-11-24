// Tests for http::Transport. The contract tests run against every implementation (the libcurl one,
// against a small server on this machine, and the fake of tests/fake_transport.h): what the
// interface promises is what these tests check, so any other implementation can be tested the same
// way. The tests after them are about the libcurl implementation only.
#include "transport.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "curl_transport.h"
#include "fake_transport.h"
#include "test_server.h"

namespace asio = boost::asio;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

namespace {

// Runs `task` and returns its result (or throws what it threw).
template <class T>
T Await(asio::io_context& io, asio::awaitable<T> task) {
  auto result = asio::co_spawn(io, std::move(task), asio::use_future);
  io.restart();
  io.run();
  return result.get();
}

// A port on which nothing listens.
int ClosedPort() {
  TestServer server;
  server.Start();
  return server.port();
}

// The test server with the behaviour of the paths FakeTransport understands.
void Serve(TestServer& server) {
  server.server().new_task_queue = [] { return new httplib::ThreadPool(64); };
  server.server().Post("/echo", [](const httplib::Request& req, httplib::Response& res) {
    res.set_content(req.body + "|" + req.get_header_value("X-Test") + "|" + req.get_header_value("Content-Type"), "text/plain");
  });
  server.server().Post("/slow", [](const httplib::Request& req, httplib::Response& res) {
    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(req.get_param_value("ms"))));
    res.set_content(req.body, "text/plain");
  });
  server.server().Post("/fail", [](const httplib::Request&, httplib::Response& res) {
    res.status = 500;
    res.set_content("no", "text/plain");
  });
}

// How to make each implementation, and how to reach it.
struct CurlSetup {
  CurlSetup() {
    Serve(server);
    server.Start();
  }
  std::unique_ptr<http::Transport> Make(http::Options options) { return http::MakeCurlTransport(options); }
  std::string Url(const std::string& path) const { return server.Url(path); }
  std::string Unreachable() const { return "http://127.0.0.1:" + std::to_string(ClosedPort()) + "/"; }
  static constexpr bool kEndsRunningRequestsWhenDestroyed = true;
  TestServer server;
};

struct FakeSetup {
  std::unique_ptr<http::Transport> Make(http::Options options) { return std::make_unique<FakeTransport>(options); }
  std::string Url(const std::string& path) const { return "http://fake.test" + path; }
  std::string Unreachable() const { return "http://unreachable.invalid/"; }
  static constexpr bool kEndsRunningRequestsWhenDestroyed = false;
};

template <class Impl>
class TransportContract : public ::testing::Test {
 protected:
  http::PostRequest Request(const std::string& path, const std::string& body, std::vector<http::Header> headers = {}) {
    return {setup_.Url(path), std::move(headers), body};
  }

  Impl setup_;
  asio::io_context io_;
};

using Implementations = ::testing::Types<CurlSetup, FakeSetup>;
TYPED_TEST_SUITE(TransportContract, Implementations);

}  // namespace

TYPED_TEST(TransportContract, SendsTheBodyAndTheHeadersAndReturnsStatusAndBody) {
  auto transport = this->setup_.Make({});
  auto post = [&]() -> asio::awaitable<http::Response> {
    co_return co_await transport->Post(this->Request("/echo", R"({"a":1})", {{"Content-Type", "application/json"}, {"X-Test", "hello"}}));
  };
  const http::Response response = Await(this->io_, post());
  EXPECT_EQ(response.status, 200);
  EXPECT_EQ(response.body, R"({"a":1}|hello|application/json)");
}

TYPED_TEST(TransportContract, AnErrorStatusIsAnAnswerNotAnException) {
  auto transport = this->setup_.Make({});
  auto post = [&]() -> asio::awaitable<http::Response> { co_return co_await transport->Post(this->Request("/fail", "{}")); };
  const http::Response response = Await(this->io_, post());
  EXPECT_EQ(response.status, 500);
  EXPECT_EQ(response.body, "no");
}

TYPED_TEST(TransportContract, TwentyRequestsAtTheSameTimeEachGetTheirOwnAnswer) {
  auto transport = this->setup_.Make({});
  std::vector<std::optional<std::string>> bodies(20);
  auto post = [&](int i) -> asio::awaitable<void> {
    const http::Response response = co_await transport->Post(this->Request("/slow?ms=300", "answer " + std::to_string(i)));
    bodies[static_cast<std::size_t>(i)] = response.body;
  };
  const auto start = Clock::now();
  for (int i = 0; i < 20; ++i) asio::co_spawn(this->io_, post(i), asio::detached);
  this->io_.run();
  const auto elapsed = Clock::now() - start;
  for (int i = 0; i < 20; ++i) EXPECT_EQ(bodies[static_cast<std::size_t>(i)], "answer " + std::to_string(i));
  EXPECT_GE(elapsed, 300ms);
  EXPECT_LT(elapsed, 1500ms);  // one after the other they would take 6 seconds
}

TYPED_TEST(TransportContract, ARefusedConnectionIsAConnectErrorThatIsSafeToRetry) {
  auto transport = this->setup_.Make({});
  const std::string url = this->setup_.Unreachable();
  auto post = [&]() -> asio::awaitable<http::Response> { co_return co_await transport->Post({url, {{"Authorization", "Bearer sk-secret"}}, "{}"}); };
  try {
    Await(this->io_, post());
    FAIL() << "expected http::Error";
  } catch (const http::Error& e) {
    EXPECT_EQ(e.kind(), http::ErrorKind::kConnect);
    EXPECT_TRUE(e.SafeToRetry());
    EXPECT_EQ(std::string(e.what()).find("sk-secret"), std::string::npos) << "the error message must not contain the key";
  }
}

TYPED_TEST(TransportContract, ASlowAnswerIsATimeoutErrorThatIsNotSafeToRetry) {
  http::Options options;
  options.timeout = 300ms;
  auto transport = this->setup_.Make(options);
  auto post = [&]() -> asio::awaitable<http::Response> { co_return co_await transport->Post(this->Request("/slow?ms=1500", "{}")); };
  try {
    Await(this->io_, post());
    FAIL() << "expected http::Error";
  } catch (const http::Error& e) {
    EXPECT_EQ(e.kind(), http::ErrorKind::kTimeout);
    EXPECT_FALSE(e.SafeToRetry());  // the server may still be working on it
  }
}

TYPED_TEST(TransportContract, ARequestThatFailsDoesNotAffectTheOthers) {
  auto transport = this->setup_.Make({});
  std::vector<std::string> outcome(5);
  auto post = [&](int i) -> asio::awaitable<void> {
    const http::PostRequest request = i == 2 ? http::PostRequest{this->setup_.Unreachable(), {}, "x"} : this->Request("/echo", std::to_string(i), {{"Content-Type", "text/plain"}});
    try {
      outcome[static_cast<std::size_t>(i)] = (co_await transport->Post(request)).body;
    } catch (const http::Error&) {
      outcome[static_cast<std::size_t>(i)] = "error";
    }
  };
  for (int i = 0; i < 5; ++i) asio::co_spawn(this->io_, post(i), asio::detached);
  this->io_.run();
  EXPECT_EQ(outcome, (std::vector<std::string>{"0||text/plain", "1||text/plain", "error", "3||text/plain", "4||text/plain"}));
}

TYPED_TEST(TransportContract, CancellingTheCoroutineCancelsTheRequest) {
  auto transport = this->setup_.Make({});
  asio::cancellation_signal signal;
  const auto start = Clock::now();
  auto answer = asio::co_spawn(
      this->io_, [&]() -> asio::awaitable<http::Response> { co_return co_await transport->Post(this->Request("/slow?ms=1500", "x")); },
      asio::bind_cancellation_slot(signal.slot(), asio::use_future));
  asio::steady_timer later(this->io_, 100ms);
  later.async_wait([&](const boost::system::error_code&) { signal.emit(asio::cancellation_type::all); });
  this->io_.run();
  EXPECT_LT(Clock::now() - start, 1000ms);  // the answer would have taken 1500 ms
  try {
    answer.get();
    FAIL() << "the request was not cancelled";
  } catch (const boost::system::system_error& e) {
    EXPECT_EQ(e.code(), asio::error::operation_aborted);
  }
}

TYPED_TEST(TransportContract, TheCoroutineResumesOnItsOwnThread) {
  auto transport = this->setup_.Make({});
  const auto before = std::this_thread::get_id();
  std::thread::id after;
  auto post = [&]() -> asio::awaitable<void> {
    co_await transport->Post(this->Request("/echo", "x"));
    after = std::this_thread::get_id();
  };
  Await(this->io_, post());
  EXPECT_EQ(after, before);
}

TYPED_TEST(TransportContract, DestroyingTheTransportEndsTheRequestsThatAreStillRunning) {
  if constexpr (!TypeParam::kEndsRunningRequestsWhenDestroyed) {
    GTEST_SKIP() << "this implementation has no request in flight to end";
  } else {
    auto transport = this->setup_.Make({});
    std::vector<std::string> outcome(3);
    auto post = [&](int i) -> asio::awaitable<void> {
      try {
        co_await transport->Post(this->Request("/slow?ms=1000", "x"));
        outcome[static_cast<std::size_t>(i)] = "answer";
      } catch (const http::Error&) {
        outcome[static_cast<std::size_t>(i)] = "error";
      }
    };
    for (int i = 0; i < 3; ++i) asio::co_spawn(this->io_, post(i), asio::detached);
    asio::steady_timer later(this->io_, 100ms);
    later.async_wait([&](const boost::system::error_code&) { transport.reset(); });
    this->io_.run();
    EXPECT_EQ(outcome, (std::vector<std::string>{"error", "error", "error"}));
  }
}

// --- the libcurl implementation only ---------------------------------------------------------------

TEST(CurlTransport, RedirectsAreNotFollowed) {
  TestServer server;
  server.server().Post("/", [](const httplib::Request&, httplib::Response& res) { res.set_redirect("/elsewhere", 302); });
  server.server().Post("/elsewhere", [](const httplib::Request&, httplib::Response& res) { res.set_content("should not be reached", "text/plain"); });
  server.Start();

  asio::io_context io;
  auto transport = http::MakeCurlTransport();
  auto post = [&]() -> asio::awaitable<http::Response> { co_return co_await transport->Post({server.Url("/"), {{"X-Api-Key", "secret"}}, "{}"}); };
  const http::Response response = Await(io, post());
  EXPECT_EQ(response.status, 302);
  EXPECT_NE(response.body, "should not be reached");
}

TEST(CurlTransport, ABodyWithChineseTextAndAMegabyteSurvives) {
  TestServer server;
  server.server().Post("/echo", [](const httplib::Request& req, httplib::Response& res) { res.set_content(req.body, "text/plain"); });
  server.Start();

  asio::io_context io;
  auto transport = http::MakeCurlTransport();
  const std::string chinese = "\xE4\xBD\xA0\xE5\xA5\xBD\xEF\xBC\x8C\xE4\xB8\x96\xE7\x95\x8C";  // UTF-8 bytes
  const std::vector<http::Header> plain = {{"Content-Type", "text/plain"}};
  auto post = [&](const std::string& body) -> asio::awaitable<std::size_t> {
    const http::Response response = co_await transport->Post({server.Url("/echo"), plain, body});
    EXPECT_EQ(response.body, body);
    co_return response.body.size();
  };
  EXPECT_EQ(Await(io, post(chinese)), chinese.size());
  // Without a Content-Type libcurl says application/x-www-form-urlencoded, and this test server
  // (like many real ones) refuses a form body of that size with 413. A JSON body always gets its own
  // Content-Type header.
  const std::string large(1 << 20, 'x');
  EXPECT_EQ(Await(io, post(large)), large.size());
}

// --- the types around it -------------------------------------------------------------------------------

TEST(HttpTypes, OnlyAFailedConnectionIsSafeToRetry) {
  EXPECT_TRUE(http::Error("x", http::ErrorKind::kConnect).SafeToRetry());
  EXPECT_FALSE(http::Error("x", http::ErrorKind::kTimeout).SafeToRetry());
  EXPECT_FALSE(http::Error("x", http::ErrorKind::kLost).SafeToRetry());
  EXPECT_FALSE(http::Error("x").SafeToRetry());
}

TEST(HttpTypes, RedactedHidesTheSecretsButKeepsTheRest) {
  const std::vector<http::Header> headers = {{"Content-Type", "application/json"},
                                             {"Authorization", "Bearer sk-secret"},
                                             {"x-api-key", "sk-ant-secret"},
                                             {"X-API-KEY", "other"},
                                             {"anthropic-version", "2023-06-01"}};
  const std::vector<http::Header> hidden = http::Redacted(headers);
  ASSERT_EQ(hidden.size(), headers.size());
  EXPECT_EQ(hidden[0].second, "application/json");
  EXPECT_EQ(hidden[1].second, "Bearer ***");  // the scheme stays, the credential goes
  EXPECT_EQ(hidden[2].second, "***");
  EXPECT_EQ(hidden[3].second, "***");  // the name is compared without regard to case
  EXPECT_EQ(hidden[4].second, "2023-06-01");
  EXPECT_EQ(headers[1].second, "Bearer sk-secret");  // the original is untouched
}
