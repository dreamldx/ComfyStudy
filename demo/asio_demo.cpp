// A small program that shows the event loop at work. Run it with a sub-command:
//
//   asio_demo callbacks               three waits, written with callbacks (they nest)
//   asio_demo coroutines              the same three waits, written as straight-line code
//   asio_demo concurrent [N] [MS]     N coroutines that each wait MS milliseconds, in one thread
//   asio_demo timeout [LIMIT] [WORK]  work that takes WORK ms with a limit of LIMIT ms
//   asio_demo cancel                  start a ten-second wait and cancel it after 50 ms
//   asio_demo strand [THREADS] [N]    N tasks on THREADS threads, with and without a strand
//   asio_demo hop                     which thread runs what: the loop, the blocking pool, the GPU thread
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "event_loop.h"

namespace loop = comfystudy::loop;
namespace asio = boost::asio;
using namespace std::chrono_literals;

namespace {

int Arg(int argc, char** argv, int index, int fallback) { return argc > index ? std::atoi(argv[index]) : fallback; }

long long SinceMs(loop::Clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(loop::Clock::now() - start).count();
}

void Callbacks() {
  asio::io_context io;
  asio::steady_timer timer(io, 100ms);
  const auto start = loop::Clock::now();
  timer.async_wait([&](const boost::system::error_code&) {
    std::cout << "step 1 at " << SinceMs(start) << " ms\n";
    timer.expires_after(100ms);
    timer.async_wait([&](const boost::system::error_code&) {
      std::cout << "step 2 at " << SinceMs(start) << " ms\n";
      timer.expires_after(100ms);
      timer.async_wait([&](const boost::system::error_code&) { std::cout << "step 3 at " << SinceMs(start) << " ms\n"; });
    });
  });
  io.run();
}

void Coroutines() {
  asio::io_context io;
  loop::SteadySleeper sleeper;
  const auto start = loop::Clock::now();
  auto job = [&]() -> asio::awaitable<void> {
    for (int step = 1; step <= 3; ++step) {
      co_await sleeper.SleepFor(100ms);
      std::cout << "step " << step << " at " << SinceMs(start) << " ms\n";
    }
  };
  loop::RunBlocking(io, job());
}

void Concurrent(int count, int millis) {
  asio::io_context io;
  loop::SteadySleeper sleeper;
  int woke = 0;
  auto job = [&]() -> asio::awaitable<void> {
    co_await sleeper.SleepFor(loop::Millis(millis));
    ++woke;
  };
  const auto start = loop::Clock::now();
  for (int i = 0; i < count; ++i) asio::co_spawn(io, job(), asio::detached);
  io.run();
  std::cout << count << " waits of " << millis << " ms finished in " << SinceMs(start) << " ms (" << woke << " woke), one thread\n";
}

void Timeout(int limit, int work) {
  asio::io_context io;
  loop::SteadySleeper sleeper;
  auto job = [&]() -> asio::awaitable<void> {
    try {
      co_await loop::WithTimeout<void>(sleeper, loop::Millis(limit), sleeper.SleepFor(loop::Millis(work)));
      std::cout << "finished within " << limit << " ms\n";
    } catch (const loop::TimeoutError& e) {
      std::cout << e.what() << "\n";
    }
  };
  loop::RunBlocking(io, job());
}

void Cancel() {
  asio::io_context io;
  loop::SteadySleeper sleeper;
  asio::cancellation_signal signal;
  const auto start = loop::Clock::now();
  loop::Clock::time_point cancelled_at{};
  asio::co_spawn(io, sleeper.SleepFor(10s), asio::bind_cancellation_slot(signal.slot(), [&](std::exception_ptr error) {
    std::cout << "the wait ended " << SinceMs(cancelled_at) << " ms after the cancel request" << (error ? " (cancelled)" : "") << "\n";
  }));
  asio::steady_timer later(io, 50ms);
  later.async_wait([&](const boost::system::error_code&) {
    cancelled_at = loop::Clock::now();
    std::cout << "cancelling after " << SinceMs(start) << " ms\n";
    signal.emit(asio::cancellation_type::all);
  });
  io.run();
}

void Strand(int threads, int count) {
  for (const bool use_strand : {false, true}) {
    asio::thread_pool pool(static_cast<std::size_t>(threads));
    const auto strand = asio::make_strand(pool);
    std::atomic<int> inside{0};
    std::atomic<int> most{0};
    for (int i = 0; i < count; ++i) {
      auto body = [&] {
        const int now = ++inside;
        int seen = most.load();
        while (now > seen && !most.compare_exchange_weak(seen, now)) {
        }
        std::this_thread::sleep_for(100us);
        --inside;
      };
      if (use_strand) {
        asio::post(strand, body);
      } else {
        asio::post(pool, body);
      }
    }
    pool.join();
    std::cout << (use_strand ? "with a strand:    " : "without a strand: ") << "at most " << most.load() << " task(s) inside at the same time\n";
  }
}

void Hop() {
  loop::Executors ex(2);
  auto show = [](const char* where) { std::cout << where << ": thread " << std::this_thread::get_id() << "\n"; };
  auto job = [&]() -> asio::awaitable<void> {
    show("event loop      ");
    co_await loop::OnPool(ex.blocking, [&] { show("blocking pool   "); });
    show("event loop again");
    co_await loop::OnPool(ex.gpu, [&] { show("gpu thread      "); });
    show("event loop again");
  };
  loop::RunBlocking(ex.io, job());
  ex.blocking.join();
  ex.gpu.join();
}

}  // namespace

int main(int argc, char** argv) {
  const std::string command = argc > 1 ? argv[1] : "";
  try {
    if (command == "callbacks") {
      Callbacks();
    } else if (command == "coroutines") {
      Coroutines();
    } else if (command == "concurrent") {
      Concurrent(Arg(argc, argv, 2, 20), Arg(argc, argv, 3, 300));
    } else if (command == "timeout") {
      Timeout(Arg(argc, argv, 2, 100), Arg(argc, argv, 3, 500));
    } else if (command == "cancel") {
      Cancel();
    } else if (command == "strand") {
      Strand(Arg(argc, argv, 2, 4), Arg(argc, argv, 3, 400));
    } else if (command == "hop") {
      Hop();
    } else {
      std::cerr << "usage: asio_demo callbacks | coroutines | concurrent [N] [MS] | timeout [LIMIT] [WORK] | cancel | strand [THREADS] [N] | hop\n";
      return 2;
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
