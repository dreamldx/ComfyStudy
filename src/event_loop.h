// A few helpers on top of Boost.Asio: a time source that tests can move by hand, timeouts and
// deadlines for coroutines, a way to run work on another thread pool and come back, and a group of
// tasks that can be cancelled and waited for.
//
// Unless a comment says otherwise, an object here is used from one executor at a time (one thread,
// or one strand). Header only: include it where you need it.
#pragma once

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include <chrono>
#include <cstddef>
#include <exception>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace comfystudy::loop {

namespace asio = boost::asio;

using Clock = std::chrono::steady_clock;
using Millis = std::chrono::milliseconds;

// Thrown when an operation does not finish in time.
class TimeoutError : public std::runtime_error {
 public:
  explicit TimeoutError(Millis limit)
      : std::runtime_error("timed out after " + std::to_string(limit.count()) + " ms") {}
};

// --- the time source ----------------------------------------------------------------------------

// Everything that waits goes through a Sleeper, so a test can replace the clock.
class Sleeper {
 public:
  virtual ~Sleeper() = default;
  virtual Clock::time_point Now() const = 0;
  virtual asio::awaitable<void> SleepFor(Millis duration) = 0;
};

// The real clock: a steady_timer per sleep.
class SteadySleeper final : public Sleeper {
 public:
  Clock::time_point Now() const override { return Clock::now(); }

  asio::awaitable<void> SleepFor(Millis duration) override {
    asio::steady_timer timer(co_await asio::this_coro::executor, duration);
    co_await timer.async_wait(asio::use_awaitable);
  }
};

// A clock that only moves when the test says so. Advance() wakes every sleeper whose time has come;
// the woken coroutines run the next time the io_context runs (io.poll()).
class ManualSleeper final : public Sleeper {
 public:
  Clock::time_point Now() const override { return now_; }

  void Advance(Millis by) {
    now_ += by;
    for (auto& [due, waiter] : waiters_) {
      if (due > now_) break;
      if (!waiter->fired) {
        waiter->fired = true;
        waiter->timer.cancel();  // a cancelled wait is how a sleeper is woken
      }
    }
  }

  // How many coroutines are asleep right now.
  std::size_t Sleeping() const { return waiters_.size(); }

  asio::awaitable<void> SleepFor(Millis duration) override {
    if (duration <= Millis::zero()) co_return;
    Waiter waiter{asio::steady_timer(co_await asio::this_coro::executor, asio::steady_timer::time_point::max()), false};
    const auto it = waiters_.emplace(now_ + duration, &waiter);
    struct Unregister {
      std::multimap<Clock::time_point, Waiter*>& map;
      std::multimap<Clock::time_point, Waiter*>::iterator at;
      ~Unregister() { map.erase(at); }
    } unregister{waiters_, it};

    boost::system::error_code ec;
    co_await waiter.timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
    if (!waiter.fired) throw boost::system::system_error(ec ? ec : asio::error::operation_aborted);  // cancelled from outside
  }

 private:
  struct Waiter {
    asio::steady_timer timer;
    bool fired = false;
  };
  Clock::time_point now_ = Clock::time_point{} + std::chrono::hours(1);
  std::multimap<Clock::time_point, Waiter*> waiters_;
};

// --- timeouts and deadlines ---------------------------------------------------------------------

// Runs `op`; if it takes longer than `limit`, cancels it and throws TimeoutError.
template <class T>
asio::awaitable<T> WithTimeout(Sleeper& sleeper, Millis limit, asio::awaitable<T> op) {
  using namespace asio::experimental::awaitable_operators;
  auto first = co_await (std::move(op) || sleeper.SleepFor(limit));  // the loser is cancelled
  if (first.index() == 1) throw TimeoutError(limit);
  if constexpr (!std::is_void_v<T>) co_return std::move(std::get<0>(first));
}

// A point in time that a whole request has to finish by. Every step of the request asks how much
// time is left, so a request made of several steps shares one budget.
class Deadline {
 public:
  Deadline(const Sleeper& sleeper, Millis budget) : sleeper_(&sleeper), at_(sleeper.Now() + budget) {}

  Millis Remaining() const {
    const auto left = at_ - sleeper_->Now();
    return left > Clock::duration::zero() ? std::chrono::duration_cast<Millis>(left) : Millis::zero();
  }
  bool Expired() const { return Remaining() == Millis::zero(); }

 private:
  const Sleeper* sleeper_;
  Clock::time_point at_;
};

// Like WithTimeout, with whatever is left of the deadline.
template <class T>
asio::awaitable<T> WithDeadline(Sleeper& sleeper, const Deadline& deadline, asio::awaitable<T> op) {
  if (deadline.Expired()) throw TimeoutError(Millis::zero());
  if constexpr (std::is_void_v<T>) {
    co_await WithTimeout<void>(sleeper, deadline.Remaining(), std::move(op));
  } else {
    co_return co_await WithTimeout<T>(sleeper, deadline.Remaining(), std::move(op));
  }
}

// What a request carries from step to step. A coroutine can resume on another thread, so thread_local
// variables are no place for it: pass it along explicitly.
struct RequestContext {
  std::string request_id;
  std::string channel_id;
  Deadline deadline;
};

// --- executors ----------------------------------------------------------------------------------

// The three places work runs: the event loop (network I/O, timers), a pool for calls that block
// (SQLite, a blocking HTTP call), and one thread for the GPU (it does one thing at a time).
struct Executors {
  explicit Executors(std::size_t blocking_threads = 4) : io(1), blocking(blocking_threads), gpu(1) {}
  asio::io_context io;
  asio::thread_pool blocking;
  asio::thread_pool gpu;
};

// Runs `work` on `pool` and resumes the calling coroutine on its own executor with the result (or the
// exception `work` threw).
template <class F>
asio::awaitable<std::invoke_result_t<F&>> OnPool(asio::thread_pool& pool, F work) {
  using R = std::invoke_result_t<F&>;
  auto task = [work = std::move(work)]() mutable -> asio::awaitable<R> {
    if constexpr (std::is_void_v<R>) {
      work();
      co_return;
    } else {
      co_return work();
    }
  };
  if constexpr (std::is_void_v<R>) {
    co_await asio::co_spawn(pool, std::move(task), asio::use_awaitable);
  } else {
    co_return co_await asio::co_spawn(pool, std::move(task), asio::use_awaitable);
  }
}

// --- groups of tasks ----------------------------------------------------------------------------

// Tasks that belong together: start them, ask all of them to stop, wait until they are gone. Join()
// before the group is destroyed, because the tasks hold a pointer to it.
class TaskGroup {
 public:
  explicit TaskGroup(asio::any_io_executor executor)
      : executor_(std::move(executor)), idle_(executor_, asio::steady_timer::time_point::max()) {}
  TaskGroup(const TaskGroup&) = delete;
  TaskGroup& operator=(const TaskGroup&) = delete;

  // Starts `task`. Returns false (and does not start it) once Close() was called.
  bool Spawn(asio::awaitable<void> task) {
    if (closed_) return false;
    auto entry = std::make_shared<Entry>();
    entries_.push_back(entry);
    asio::co_spawn(executor_, std::move(task),
                   asio::bind_cancellation_slot(entry->signal.slot(),
                                                [this, entry](std::exception_ptr error) { Finished(entry, std::move(error)); }));
    return true;
  }

  // No new tasks from now on (the first step of a shutdown).
  void Close() { closed_ = true; }

  // Asks every running task to stop at its next wait.
  void CancelAll() {
    const auto running = entries_;  // a copy: stopping a task changes the list
    for (const auto& entry : running) entry->signal.emit(asio::cancellation_type::all);
  }

  // Completes when no task is running.
  asio::awaitable<void> Join() {
    while (!entries_.empty()) {
      boost::system::error_code ec;
      co_await idle_.async_wait(asio::redirect_error(asio::use_awaitable, ec));
      if (ec && !(ec == asio::error::operation_aborted && woken_)) throw boost::system::system_error(ec);
      woken_ = false;
    }
  }

  std::size_t Running() const { return entries_.size(); }

  // The first error a task ended with, other than being cancelled (null if there was none).
  std::exception_ptr FirstError() const { return first_error_; }

 private:
  struct Entry {
    asio::cancellation_signal signal;
  };

  void Finished(const std::shared_ptr<Entry>& entry, std::exception_ptr error) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
      if (*it == entry) {
        entries_.erase(it);
        break;
      }
    }
    if (error && !first_error_) {
      try {
        std::rethrow_exception(error);
      } catch (const boost::system::system_error& e) {
        if (e.code() != asio::error::operation_aborted) first_error_ = error;  // being cancelled is not an error
      } catch (...) {
        first_error_ = error;
      }
    }
    if (entries_.empty()) {
      woken_ = true;
      idle_.cancel();
    }
  }

  asio::any_io_executor executor_;
  asio::steady_timer idle_;  // Join() waits on it; Finished() wakes it when the last task is gone
  std::vector<std::shared_ptr<Entry>> entries_;
  std::exception_ptr first_error_;
  bool closed_ = false;
  bool woken_ = false;
};

// --- running coroutines from ordinary code ------------------------------------------------------

// Runs `task` on `io` until `io` has nothing left to do, and returns its result. For programs and
// tests with a real clock; a task that waits on a ManualSleeper never finishes this way (use poll).
template <class T>
T RunBlocking(asio::io_context& io, asio::awaitable<T> task) {
  auto result = asio::co_spawn(io, std::move(task), asio::use_future);
  io.restart();
  io.run();
  return result.get();
}

}  // namespace comfystudy::loop
