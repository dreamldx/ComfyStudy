#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "http_client.h"
#include "test_server.h"

namespace {

using Clock = std::chrono::steady_clock;
constexpr int kDelayMs = 200;

// A server that answers every POST to "/" after `delay_ms`, with the request body, and counts the
// requests it has seen.
struct SlowServer {
  explicit SlowServer(int delay_ms) {
    server.server().new_task_queue = [] { return new httplib::ThreadPool(32); };
    server.server().Post("/", [this, delay_ms](const httplib::Request& req, httplib::Response& res) {
      ++requests;
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
      res.set_content(req.body, "text/plain");
    });
    server.Start();
  }
  TestServer server;
  std::atomic<int> requests{0};
};

std::vector<http::PostRequest> Requests(const std::string& url, int count) {
  std::vector<http::PostRequest> requests;
  for (int i = 0; i < count; ++i) {
    requests.push_back({url, {{"Content-Type", "text/plain"}}, "request-" + std::to_string(i)});
  }
  return requests;
}

std::chrono::milliseconds Since(Clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
}

// --- easy: timing and connection reuse -----------------------------------------------------------

TEST(HttpTiming, AResponseCarriesTheTimesOfItsPhases) {
  SlowServer slow(kDelayMs);
  const http::Response r = http::Post(slow.server.Url("/"), {{"Content-Type", "text/plain"}}, "x");
  const http::Timing& t = r.timing;
  EXPECT_EQ(t.protocol, "HTTP/1.1");
  EXPECT_EQ(t.new_connections, 1);
  EXPECT_GT(t.connect.count(), 0);
  EXPECT_EQ(t.tls.count(), 0) << "plain http has no TLS handshake";
  // the times are cumulative: each phase ends no earlier than the one before it
  EXPECT_LE(t.name_lookup, t.connect);
  EXPECT_LE(t.connect, t.first_byte);
  EXPECT_LE(t.first_byte, t.total);
  EXPECT_GE(t.first_byte, std::chrono::milliseconds(kDelayMs)) << "the server waited before answering";
}

TEST(HttpTiming, PostMakesANewConnectionEveryTimeButASessionReusesIt) {
  SlowServer slow(0);
  const std::string url = slow.server.Url("/");
  const std::vector<http::Header> headers = {{"Content-Type", "text/plain"}};

  EXPECT_EQ(http::Post(url, headers, "a").timing.new_connections, 1);
  EXPECT_EQ(http::Post(url, headers, "b").timing.new_connections, 1);

  http::Session session;
  const http::Response first = session.Post(url, headers, "a");
  const http::Response second = session.Post(url, headers, "b");
  EXPECT_EQ(first.timing.new_connections, 1);
  EXPECT_EQ(second.timing.new_connections, 0) << "the connection of the first request was reused";
  EXPECT_EQ(second.timing.connect.count(), 0) << "a reused connection has no connect phase";
  EXPECT_EQ(second.body, "b") << "the answers do not get mixed up";
}

TEST(HttpTiming, ASessionRecoversAfterAFailedRequest) {
  SlowServer slow(0);
  http::Session session;
  EXPECT_THROW(session.Post("http://127.0.0.1:1/", {}, "x"), http::Error);
  EXPECT_EQ(session.Post(slow.server.Url("/"), {{"Content-Type", "text/plain"}}, "ok").body, "ok");
}

// --- multi: PostMany -----------------------------------------------------------------------------

TEST(PostMany, AnswersComeBackInTheOrderOfTheRequests) {
  SlowServer slow(0);
  const auto results = http::PostMany(Requests(slow.server.Url("/"), 10), 4);
  ASSERT_EQ(results.size(), 10u);
  for (std::size_t i = 0; i < results.size(); ++i) {
    ASSERT_TRUE(results[i].response) << results[i].error;
    EXPECT_EQ(results[i].response->body, "request-" + std::to_string(i));
  }
}

TEST(PostMany, TheRequestsRunAtTheSameTimeButNotMoreThanAllowed) {
  SlowServer slow(kDelayMs);
  const auto requests = Requests(slow.server.Url("/"), 8);

  auto start = Clock::now();
  http::PostMany(requests, 8);
  const auto all_at_once = Since(start);
  EXPECT_LT(all_at_once, std::chrono::milliseconds(3 * kDelayMs / 2)) << "8 requests in one round, also with the default version setting";

  start = Clock::now();
  http::PostMany(requests, 4);
  const auto four_at_a_time = Since(start);
  EXPECT_GE(four_at_a_time, std::chrono::milliseconds(2 * kDelayMs)) << "8 requests in two rounds";
  EXPECT_LT(four_at_a_time, std::chrono::milliseconds(3 * kDelayMs + kDelayMs / 2));
}

TEST(PostMany, OneFailingRequestDoesNotAffectTheOthers) {
  SlowServer slow(0);
  auto requests = Requests(slow.server.Url("/"), 5);
  requests[2].url = "http://127.0.0.1:1/";  // nothing listens there
  const auto results = http::PostMany(requests, 5);
  for (std::size_t i = 0; i < results.size(); ++i) {
    if (i == 2) {
      EXPECT_FALSE(results[i].response);
      EXPECT_FALSE(results[i].error.empty());
    } else {
      ASSERT_TRUE(results[i].response) << results[i].error;
      EXPECT_EQ(results[i].response->body, "request-" + std::to_string(i));
    }
  }
}

TEST(PostMany, ARequestThatTimesOutFailsAloneAndNoRequestsIsFine) {
  SlowServer slow(kDelayMs);
  http::Options options;
  options.timeout = std::chrono::milliseconds(50);
  const auto results = http::PostMany(Requests(slow.server.Url("/"), 3), 3, options);
  for (const auto& r : results) {
    EXPECT_FALSE(r.response);
    EXPECT_NE(r.error.find("request failed"), std::string::npos) << r.error;
  }
  EXPECT_TRUE(http::PostMany({}, 4).empty());
}

// --- share: several threads --------------------------------------------------------------------

TEST(HttpShare, ThreadsThatShareCachesAllGetTheirOwnAnswers) {
  SlowServer slow(10);
  http::Share share;
  http::Options options;
  options.share = &share;

  std::atomic<int> wrong{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < 6; ++t) {
    threads.emplace_back([&, t] {
      for (int i = 0; i < 10; ++i) {
        const std::string body = "t" + std::to_string(t) + "-" + std::to_string(i);
        try {
          const http::Response r = http::Post(slow.server.Url("/"), {{"Content-Type", "text/plain"}}, body, options);
          if (r.status != 200 || r.body != body) ++wrong;
        } catch (const http::Error&) {
          ++wrong;
        }
      }
    });
  }
  for (std::thread& t : threads) t.join();
  EXPECT_EQ(wrong, 0);
  EXPECT_EQ(slow.requests, 60);
}

// --- HTTP version ------------------------------------------------------------------------------

TEST(HttpVersion, PlainHttpIsAlwaysHttp1AndAskingForHttp2Works) {
  SlowServer slow(0);
  http::Options options;
  options.version = http::Version::kHttp2;  // only used over TLS
  EXPECT_EQ(http::Post(slow.server.Url("/"), {{"Content-Type", "text/plain"}}, "x", options).timing.protocol,
            "HTTP/1.1");
}

}  // namespace
