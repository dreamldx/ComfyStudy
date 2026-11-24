// Tests for src/event_loop.h: the event loop, coroutines, timeouts, cancellation, strands, executors and
// groups of tasks. Tests that need to control time use ManualSleeper (time moves only with Advance);
// the few that use the real clock leave a wide margin.
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

#include "event_loop.h"

namespace loop = comfystudy::loop;
namespace asio = boost::asio;
using namespace std::chrono_literals;

namespace {

// Runs everything that is ready, without waiting for timers.
void Poll(asio::io_context& io) {
  io.restart();
  io.poll();
}

template <class T>
std::future<T> Start(asio::io_context& io, asio::awaitable<T> task) {
  return asio::co_spawn(io, std::move(task), asio::use_future);
}

template <class T>
bool Done(std::future<T>& f) {
  return f.wait_for(0ms) == std::future_status::ready;
}

// Sleeps, then returns `value`.
asio::awaitable<int> Work(loop::Sleeper& sleeper, loop::Millis duration, int value) {
  co_await sleeper.SleepFor(duration);
  co_return value;
}

}  // namespace

// --- the same job, written two ways ---------------------------------------------------------------

TEST(EventLoop, CallbacksRunInOrderButNest) {
  asio::io_context io;
  asio::steady_timer timer(io, 1ms);
  std::vector<int> order;
  timer.async_wait([&](const boost::system::error_code&) {
    order.push_back(1);
    timer.expires_after(1ms);
    timer.async_wait([&](const boost::system::error_code&) {
      order.push_back(2);
      timer.expires_after(1ms);
      timer.async_wait([&](const boost::system::error_code&) { order.push_back(3); });
    });
  });
  io.run();
  EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
}

TEST(EventLoop, CoroutinesDoTheSameInStraightLine) {
  asio::io_context io;
  loop::SteadySleeper sleeper;
  std::vector<int> order;
  auto job = [&]() -> asio::awaitable<void> {
    for (int i = 1; i <= 3; ++i) {
      co_await sleeper.SleepFor(1ms);
      order.push_back(i);
    }
  };
  loop::RunBlocking(io, job());
  EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
}

TEST(EventLoop, TimersFireInOrderOfTheirTimeNotOfTheirCreation) {
  asio::io_context io;
  asio::steady_timer a(io, 30ms), b(io, 10ms), c(io, 20ms);
  std::vector<char> order;
  a.async_wait([&](const boost::system::error_code&) { order.push_back('a'); });
  b.async_wait([&](const boost::system::error_code&) { order.push_back('b'); });
  c.async_wait([&](const boost::system::error_code&) { order.push_back('c'); });
  io.run();
  EXPECT_EQ(order, (std::vector<char>{'b', 'c', 'a'}));
}

TEST(EventLoop, PostFromAnotherThreadRunsOnTheLoopThread) {
  asio::io_context io;
  auto guard = asio::make_work_guard(io);  // keeps run() from returning while nothing is queued
  std::thread::id ran_on;
  std::thread outsider([&] {
    asio::post(io, [&] {
      ran_on = std::this_thread::get_id();
      guard.reset();  // nothing left to wait for
    });
  });
  io.run();
  outsider.join();
  EXPECT_EQ(ran_on, std::this_thread::get_id());
}

// --- many waits in one thread ---------------------------------------------------------------------

TEST(EventLoop, TwentySleepsInOneThreadAllWakeTogether) {
  asio::io_context io;
  loop::ManualSleeper sleeper;
  int woke = 0;
  auto job = [&]() -> asio::awaitable<void> {
    co_await sleeper.SleepFor(300ms);
    ++woke;
  };
  for (int i = 0; i < 20; ++i) asio::co_spawn(io, job(), asio::detached);
  Poll(io);
  EXPECT_EQ(sleeper.Sleeping(), 20U);
  sleeper.Advance(299ms);
  Poll(io);
  EXPECT_EQ(woke, 0);
  sleeper.Advance(1ms);
  Poll(io);
  EXPECT_EQ(woke, 20);
  EXPECT_EQ(sleeper.Sleeping(), 0U);
}

TEST(EventLoop, TwentySleepsWithTheRealClockTakeAboutOneSleep) {
  asio::io_context io;
  loop::SteadySleeper sleeper;
  int woke = 0;
  auto job = [&]() -> asio::awaitable<void> {
    co_await sleeper.SleepFor(60ms);
    ++woke;
  };
  const auto start = loop::Clock::now();
  for (int i = 0; i < 20; ++i) asio::co_spawn(io, job(), asio::detached);
  io.run();
  const auto elapsed = loop::Clock::now() - start;
  EXPECT_EQ(woke, 20);
  EXPECT_GE(elapsed, 60ms);
  EXPECT_LT(elapsed, 600ms);  // 20 sequential sleeps would take 1200 ms
}

// --- timeouts and cancellation --------------------------------------------------------------------

TEST(Timeout, ReturnsTheValueWhenTheWorkIsFastEnough) {
  asio::io_context io;
  loop::ManualSleeper sleeper;
  auto result = Start(io, loop::WithTimeout<int>(sleeper, 500ms, Work(sleeper, 100ms, 42)));
  Poll(io);
  sleeper.Advance(100ms);
  Poll(io);
  ASSERT_TRUE(Done(result));
  EXPECT_EQ(result.get(), 42);
  EXPECT_EQ(sleeper.Sleeping(), 0U);  // the timeout's own sleep was cancelled
}

TEST(Timeout, ThrowsAndCancelsTheWorkWhenItIsTooSlow) {
  asio::io_context io;
  loop::ManualSleeper sleeper;
  auto result = Start(io, loop::WithTimeout<int>(sleeper, 100ms, Work(sleeper, 500ms, 1)));
  Poll(io);
  EXPECT_EQ(sleeper.Sleeping(), 2U);
  sleeper.Advance(100ms);
  Poll(io);
  ASSERT_TRUE(Done(result));
  EXPECT_THROW(result.get(), loop::TimeoutError);
  EXPECT_EQ(sleeper.Sleeping(), 0U);  // the slow work was cancelled, not left running
}

TEST(Cancellation, AnEmittedSignalStopsASleepAtOnce) {
  asio::io_context io;
  loop::SteadySleeper sleeper;
  asio::cancellation_signal signal;
  auto result = asio::co_spawn(io, sleeper.SleepFor(10s), asio::bind_cancellation_slot(signal.slot(), asio::use_future));
  asio::post(io, [&] { signal.emit(asio::cancellation_type::all); });
  const auto start = loop::Clock::now();
  io.run();
  EXPECT_LT(loop::Clock::now() - start, 2s);
  try {
    result.get();
    FAIL() << "the sleep was not cancelled";
  } catch (const boost::system::system_error& e) {
    EXPECT_EQ(e.code(), asio::error::operation_aborted);
  }
}

// --- deadlines ------------------------------------------------------------------------------------

TEST(Deadline, ShrinksAsTimePasses) {
  loop::ManualSleeper sleeper;
  const loop::Deadline deadline(sleeper, 1000ms);
  EXPECT_EQ(deadline.Remaining(), 1000ms);
  sleeper.Advance(400ms);
  EXPECT_EQ(deadline.Remaining(), 600ms);
  EXPECT_FALSE(deadline.Expired());
  sleeper.Advance(700ms);
  EXPECT_EQ(deadline.Remaining(), 0ms);
  EXPECT_TRUE(deadline.Expired());
}

TEST(Deadline, TwoStepsShareOneBudget) {
  asio::io_context io;
  loop::ManualSleeper sleeper;
  const loop::Deadline deadline(sleeper, 1000ms);
  auto request = [&]() -> asio::awaitable<int> {
    const int first = co_await loop::WithDeadline<int>(sleeper, deadline, Work(sleeper, 400ms, 1));
    const int second = co_await loop::WithDeadline<int>(sleeper, deadline, Work(sleeper, 700ms, 2));
    co_return first + second;
  };
  auto result = Start(io, request());
  Poll(io);
  sleeper.Advance(400ms);  // the first step is done; 600 ms are left for a step that needs 700
  Poll(io);
  EXPECT_FALSE(Done(result));
  sleeper.Advance(600ms);
  Poll(io);
  ASSERT_TRUE(Done(result));
  EXPECT_THROW(result.get(), loop::TimeoutError);
}

TEST(Deadline, AnExpiredDeadlineFailsBeforeStartingTheStep) {
  asio::io_context io;
  loop::ManualSleeper sleeper;
  const loop::Deadline deadline(sleeper, 100ms);
  sleeper.Advance(200ms);
  auto result = Start(io, loop::WithDeadline<int>(sleeper, deadline, Work(sleeper, 10ms, 1)));
  Poll(io);
  ASSERT_TRUE(Done(result));
  EXPECT_THROW(result.get(), loop::TimeoutError);
  EXPECT_EQ(sleeper.Sleeping(), 0U);
}

// --- threads: strands and executors ---------------------------------------------------------------

TEST(Strand, KeepsOneHandlerInTheSectionAtATime) {
  asio::thread_pool pool(4);
  const auto strand = asio::make_strand(pool);
  std::atomic<int> inside{0};
  std::atomic<int> most{0};
  for (int i = 0; i < 400; ++i) {
    asio::post(strand, [&] {
      const int now = ++inside;
      int seen = most.load();
      while (now > seen && !most.compare_exchange_weak(seen, now)) {
      }
      std::this_thread::yield();
      --inside;
    });
  }
  pool.join();
  EXPECT_EQ(most.load(), 1);
}

TEST(Executors, WorkRunsOnThePoolAndTheCoroutineComesBack) {
  loop::Executors ex(2);
  const auto loop_thread = std::this_thread::get_id();
  std::thread::id pool_thread, back_on;
  auto job = [&]() -> asio::awaitable<int> {
    const int value = co_await loop::OnPool(ex.blocking, [&] {
      pool_thread = std::this_thread::get_id();
      return 7;
    });
    back_on = std::this_thread::get_id();
    co_return value;
  };
  EXPECT_EQ(loop::RunBlocking(ex.io, job()), 7);
  EXPECT_NE(pool_thread, loop_thread);
  EXPECT_EQ(back_on, loop_thread);
  ex.blocking.join();
  ex.gpu.join();
}

TEST(Executors, AnExceptionInThePoolWorkReachesTheCaller) {
  loop::Executors ex(1);
  auto job = [&]() -> asio::awaitable<void> {
    co_await loop::OnPool(ex.blocking, []() -> void { throw std::runtime_error("disk full"); });
  };
  EXPECT_THROW(loop::RunBlocking(ex.io, job()), std::runtime_error);
  ex.blocking.join();
  ex.gpu.join();
}

namespace {
thread_local int tls_value = 0;
}

TEST(Executors, AThreadLocalDoesNotFollowACoroutineButAnExplicitContextDoes) {
  loop::Executors ex(1);
  loop::ManualSleeper clock;
  tls_value = 7;  // set on the loop thread
  const loop::RequestContext context{"r-1", "channel-9", loop::Deadline(clock, 1s)};
  int seen_in_pool = -1;
  std::string id_in_pool;
  auto job = [&]() -> asio::awaitable<void> {
    co_await loop::OnPool(ex.blocking, [&, context] {
      seen_in_pool = tls_value;  // another thread: its own copy, still 0
      id_in_pool = context.request_id;
    });
  };
  loop::RunBlocking(ex.io, job());
  EXPECT_EQ(seen_in_pool, 0);
  EXPECT_EQ(id_in_pool, "r-1");
  EXPECT_EQ(tls_value, 7);
  ex.blocking.join();
  ex.gpu.join();
}

// --- groups of tasks ------------------------------------------------------------------------------

TEST(TaskGroup, JoinWaitsForEveryTask) {
  asio::io_context io;
  loop::ManualSleeper sleeper;
  loop::TaskGroup group(io.get_executor());
  int finished = 0;
  auto task = [&](loop::Millis d) -> asio::awaitable<void> {
    co_await sleeper.SleepFor(d);
    ++finished;
  };
  for (int i = 1; i <= 3; ++i) ASSERT_TRUE(group.Spawn(task(loop::Millis(100 * i))));
  auto joined = Start(io, group.Join());
  Poll(io);
  EXPECT_EQ(group.Running(), 3U);
  sleeper.Advance(200ms);
  Poll(io);
  EXPECT_EQ(finished, 2);
  EXPECT_FALSE(Done(joined));
  sleeper.Advance(100ms);
  Poll(io);
  EXPECT_EQ(finished, 3);
  EXPECT_TRUE(Done(joined));
  EXPECT_EQ(group.FirstError(), nullptr);
}

TEST(TaskGroup, ShutdownStopsAcceptingCancelsEverythingAndJoins) {
  asio::io_context io;
  loop::ManualSleeper sleeper;
  loop::TaskGroup group(io.get_executor());
  auto forever = [&]() -> asio::awaitable<void> { co_await sleeper.SleepFor(std::chrono::hours(24)); };
  for (int i = 0; i < 5; ++i) ASSERT_TRUE(group.Spawn(forever()));
  Poll(io);
  EXPECT_EQ(sleeper.Sleeping(), 5U);

  group.Close();
  EXPECT_FALSE(group.Spawn(forever()));  // the sixth task is refused
  group.CancelAll();
  auto joined = Start(io, group.Join());
  Poll(io);
  EXPECT_TRUE(Done(joined));
  EXPECT_EQ(group.Running(), 0U);
  EXPECT_EQ(sleeper.Sleeping(), 0U);
  EXPECT_EQ(group.FirstError(), nullptr);  // being cancelled is not an error
}

TEST(TaskGroup, KeepsTheFirstRealErrorAndStillJoins) {
  asio::io_context io;
  loop::TaskGroup group(io.get_executor());
  // A coroutine needs a co_await (or co_return) to be one: without it, the throw would happen on the call.
  auto failing = []() -> asio::awaitable<void> {
    co_await asio::this_coro::executor;
    throw std::runtime_error("boom");
  };
  ASSERT_TRUE(group.Spawn(failing()));
  auto joined = Start(io, group.Join());
  Poll(io);
  EXPECT_TRUE(Done(joined));
  ASSERT_NE(group.FirstError(), nullptr);
  EXPECT_THROW(std::rethrow_exception(group.FirstError()), std::runtime_error);
}
