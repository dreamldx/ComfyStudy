# llm-1.1 事件循环：Boost.Asio 的定时器与协程

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.1"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md)
- 一句话目标：弄清事件循环是什么，用 Boost.Asio 写出定时器、协程、超时和取消，讲清多线程 `io_context` 与 `strand`；在三个执行器（事件循环、阻塞线程池、GPU 线程）之间切换；让一个请求的所有步骤共用一个截止时间；用 `TaskGroup` 做关停；用可以手动推进的时钟让测试不依赖真实时间；本章不连网络，后面的章节在它上面接 libcurl

## 1. 本章目标与成果

机器人要同时等很多件事：模型的回答（几百毫秒到几十秒）、Discord 的消息、编辑消息时的限速定时器、几十秒的出图作业。如果每件事占一个线程，线程数会跟着等待的数量涨，而且线程大部分时间只是在睡觉。事件循环换了一种做法：一个线程向系统登记“这些事情有动静时叫我”，然后只处理有动静的那些。

本章先把事件循环本身讲清楚、测清楚，不连网络。后面的章节在它上面接 libcurl，再把整个机器人搭在上面。

做完后你会得到：

- 对 Boost.Asio 的 `io_context`、`post`、`strand` 和 C++20 协程有一个准确的印象，知道回调写法的问题在哪里，协程怎么把它写成顺序的代码。
- `src/event_loop.h`，一个只有头文件的小工具集：
  - `Sleeper`：所有“等一会儿”都经过它，测试里可以换成手动推进的时钟。
  - `WithTimeout` 和 `Deadline`：给一个操作加超时；让一个请求的所有步骤共用一个截止时间。
  - `Executors` 和 `OnPool`：事件循环、阻塞线程池、GPU 线程三个执行器，以及“切过去干活再回来”。
  - `TaskGroup`：一组任务，能启动、能统一取消、能等它们都结束，用来做关停。
  - `RequestContext`：一个请求要带着走的信息，显式地传，不用线程局部变量。
- `demo/asio_demo.cpp`，七个子命令，每个演示一件事。
- `tests/test_asio_basics.cpp`，19 个测试。需要控制时间的测试都用手动推进的时钟，不依赖真实时间。

本章没有网络。一个 HTTP 请求怎么放进事件循环，是 llm-1.2 的内容。

## 2. 背景与概念

### 2.1 事件循环是什么

操作系统提供“等待多个对象，哪个有动静就告诉我”的机制：Linux 上是 epoll，Windows 上是 I/O 完成端口（IOCP）。事件循环就是一个线程反复做两件事：向系统要“已经好了的事情”，再把每件事对应的处理函数（handler）运行一遍。

Asio 里这个线程叫调用 `io_context::run()` 的线程。你把一件事登记给它（比如“500 毫秒后叫我”），它记在队列里；时间到了，对应的 handler 在 `run()` 所在的线程里运行。所以 handler 之间不会同时运行，只要 `run()` 只在一个线程里调用，handler 里就不用加锁。代价是：handler 里不能做耗时的事。一个 handler 占住线程一秒，所有其他事情都要等这一秒。

Windows 上 Asio 用 IOCP，系统在操作做完之后才通知你（proactor）；Linux 上的 epoll 是在“可以读写了”时通知你，再由你去读写（reactor）。Asio 把两种差别藏了起来，写代码时一样用。

### 2.2 回调与协程

用回调写“等 100 毫秒，再等 100 毫秒，再等 100 毫秒”，每一步都要嵌进上一步的 handler 里，三步就嵌了三层（测试 `CallbacksRunInOrderButNest` 就是这样写的）。真实的流程要加上“失败了怎么办”“超时了怎么办”，嵌套会深得读不下去，局部变量的生命周期也要靠手工保证。

C++20 的协程把它写成顺序的代码：`co_await` 把当前函数暂停，线程回去处理别的事，等的东西好了再从暂停的地方继续。Asio 里一个返回 `asio::awaitable<T>` 的函数就是协程；`asio::co_spawn` 把它交给一个执行器去跑；`co_await` 一个异步操作就得到它的结果。协程暂停时，它的局部变量保存在堆上的“协程帧”里，所以不用像回调那样手工保存状态。

要注意两点：协程帧里存的是值，引用和指针指向的东西要比协程活得久；协程可以在不同的线程上恢复，所以线程局部变量（`thread_local`）跟不上它。

### 2.3 超时与取消

Asio 的每个异步操作都可以被取消：取消之后操作会以 `operation_aborted` 结束。取消的入口是 `cancellation_signal`，把它的 `slot` 绑到一个协程上，`emit` 一下，协程当前正在等的那个操作就被取消。

`||` 运算符（`asio::experimental::awaitable_operators`）把两个协程并起来，谁先结束就取消另一个。“操作加超时”就是“操作 `||` 一个定时器”：定时器先到，操作被取消；操作先到，定时器被取消。

### 2.4 线程：strand 与三个执行器

事件循环可以由一个线程跑，也可以由几个线程同时跑 `run()`，这时 handler 可能同时运行。`strand` 是一个保证“同一时刻只运行其中一个 handler”的执行器，不用锁就能让一批 handler 串行。机器人里“同一个频道的一轮对话串行”就是用它。

机器人里有三类工作，不应该混在一起：

| 执行器 | 放什么 | 为什么单独放 |
|---|---|---|
| 事件循环（`io`） | 网络请求、定时器、协程的调度 | 不能被阻塞，一阻塞所有等待都停 |
| 阻塞线程池（`blocking`） | SQLite、同步的 HTTP 调用、读文件 | 这些调用会占住线程，放进池里不影响事件循环 |
| GPU 线程（`gpu`） | 采样、解码 | GPU 一次只做一件事，一个线程排队正好 |

协程里要“切到池里干活，干完回到事件循环”，用 `OnPool`。注意 `asio::post(pool, use_awaitable)` 并不会让协程换线程（见第 8 节），`OnPool` 是用 `co_spawn` 把一个小协程交给池去跑，再 `co_await` 它。

### 2.5 截止时间

给每个步骤各设 30 秒的超时，一个有五步的请求最坏要等 150 秒。更合理的是整个请求定一个截止时间，每一步问“还剩多少时间”，剩的就是这一步的超时。`Deadline` 就是这个点；`WithDeadline` 用剩余的时间做超时。

### 2.6 关停

机器人退出时要做三件事，顺序不能乱：先不再接新任务，再让在途的任务停下来，最后等它们真的结束，才能销毁它们依赖的对象。`TaskGroup` 的 `Close`、`CancelAll`、`Join` 对应这三步。

### 2.7 测试里的时间

带超时的代码如果在测试里用真实时钟，要么慢，要么偶尔失败。做法是让所有等待都经过一个 `Sleeper`：生产环境用 `SteadySleeper`（背后是 `steady_timer`），测试用 `ManualSleeper`，时间只在测试调用 `Advance` 时前进。

## 3. 本章改动

本章的代码在支线分支 `quest/llm-01-llm-app-engineering` 上，不合入 main，也不打 tag。提交信息以 `chapter-llm-1.1-asio-basics:` 开头，`git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.1"` 可以找到它。

1. 依赖和构建：
   - `vcpkg.json`：加 `boost-asio`（只装需要的那一个 Boost 库，头文件为主）。
   - `cmake/platform/windows.cmake`：`find_package(boost_asio)`。
   - `CMakeLists.txt`：新增接口目标 `comfystudy_asio`（链接 `Boost::asio`，定义 `_WIN32_WINNT=0x0A00` 和 `BOOST_ASIO_NO_DEPRECATED`），新增可执行目标 `asio_demo`。
   - `scripts/check-build.ps1`：编译和链接命令的数量随之改成 20 和 12。
2. 库代码：`src/event_loop.h`。
3. 演示和测试：`demo/asio_demo.cpp`、`tests/test_asio_basics.cpp`、`tests/CMakeLists.txt`。
4. 分支的 `README.md`：换成这条支线自己的说明，列出到本章为止实现了什么，并链接到本章。

## 4. 关键代码讲解

### 4.1 Sleeper：所有等待的入口

<!-- src: src/event_loop.h#L40-L57 -->
```cpp
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
```

`SleepFor` 是协程。`co_await asio::this_coro::executor` 取出当前协程所在的执行器，用它建定时器，这样定时器的回调会回到协程自己的线程上。

手动推进的版本：

<!-- src: src/event_loop.h#L59-L101 -->
```cpp
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
```

每个睡眠的协程登记一个定时器，到期时间设成 `time_point::max()`，实际上永远不会自己到期。`Advance` 把时间往前拨，对到点的睡眠者调用 `timer.cancel()`，把它“叫醒”。协程醒来要分清两种情况：`fired` 为真，是 `Advance` 叫醒的，正常返回；为假，是外部取消（比如超时抢先了），要把 `operation_aborted` 抛出去。`Unregister` 保证无论哪种方式结束，都会从登记表里删掉。

`Advance` 只是标记并取消定时器，被叫醒的协程在 `io_context` 下一次运行时才继续，所以测试里在 `Advance` 之后要调用 `io.poll()`。

### 4.2 WithTimeout 和 Deadline

<!-- src: src/event_loop.h#L105-L112 -->
```cpp
// Runs `op`; if it takes longer than `limit`, cancels it and throws TimeoutError.
template <class T>
asio::awaitable<T> WithTimeout(Sleeper& sleeper, Millis limit, asio::awaitable<T> op) {
  using namespace asio::experimental::awaitable_operators;
  auto first = co_await (std::move(op) || sleeper.SleepFor(limit));  // the loser is cancelled
  if (first.index() == 1) throw TimeoutError(limit);
  if constexpr (!std::is_void_v<T>) co_return std::move(std::get<0>(first));
}
```

`op || sleeper.SleepFor(limit)` 返回一个 `variant`，下标 0 是操作先结束，下标 1 是定时器先到。输的那一个已经被取消了，所以不会留下后台还在跑的操作。

<!-- src: src/event_loop.h#L114-L129 -->
```cpp
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
```

`Remaining()` 向 `Sleeper` 要当前时间，所以用 `ManualSleeper` 时截止时间也跟着手动时钟走。

### 4.3 Executors 和 OnPool

<!-- src: src/event_loop.h#L152-L179 -->
```cpp
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
```

`OnPool` 里把要做的事包成一个小协程，用 `co_spawn(pool, ...)` 交给池去跑，再 `co_await` 它的结果。这时调用者协程恢复的执行器仍然是它自己的，所以“切过去、做完、回来”一气呵成，池里抛出的异常也会原样出现在调用者这里。

### 4.4 TaskGroup

<!-- src: src/event_loop.h#L183-L225 -->
```cpp
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
```

`Spawn` 给每个任务一个自己的 `cancellation_signal`，绑在 `co_spawn` 的完成处理器上。`CancelAll` 对每个任务 `emit`，每个任务当前等的那个操作被取消，任务以 `operation_aborted` 结束。注意 `CancelAll` 先复制了一份列表，因为任务结束会改动原来的列表。

<!-- src: src/event_loop.h#L232-L252 -->
```cpp
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
```

任务结束时 `Finished` 把它从列表里删掉，并区分“被取消”和“真的出错”：`operation_aborted` 不算错误，其他异常记下第一个。列表空了就唤醒 `Join`。

使用规则写在类的注释里：对象只在一个执行器上使用；销毁之前要先 `Join`，因为任务里持有指向它的指针。

### 4.5 测试怎么用手动时钟

20 个协程各睡 300 毫秒，在一个线程里：

<!-- src: tests/test_asio_basics.cpp#L106-L124 -->
```cpp
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
```

时间到 299 毫秒时一个都没醒，再走 1 毫秒全醒，没有任何真实的等待。

超时的两个方向：

<!-- src: tests/test_asio_basics.cpp#L157-L168 -->
```cpp
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
```

超时之后 `Sleeping()` 回到 0，说明太慢的那个操作被取消了，没有留下后台任务。

关停：

<!-- src: tests/test_asio_basics.cpp#L334-L352 -->
```cpp
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
```

五个任务永远睡下去，`Close` 之后再 `Spawn` 被拒绝，`CancelAll` 加 `Join` 之后所有睡眠者都被叫醒，且 `FirstError()` 是空的，因为被取消不算错误。

## 5. 构建与运行

在支线分支上构建，演示程序是单独的目标 `asio_demo`：

```powershell
git switch quest/llm-01-llm-app-engineering
.\scripts\dev.ps1 cmake --preset windows-debug
.\scripts\dev.ps1 cmake --build --preset windows-debug
```

第一次配置会让 vcpkg 装 `boost-asio`，大约一分钟。然后逐个运行演示的子命令（下面的数字来自 Debug 构建）：

```powershell
build\windows-debug\bin\asio_demo.exe callbacks
build\windows-debug\bin\asio_demo.exe coroutines
```

```
step 1 at 100 ms        step 1 at 112 ms
step 2 at 207 ms        step 2 at 222 ms
step 3 at 316 ms        step 3 at 330 ms
```

左边是回调版，右边是协程版（实际是依次输出的）。两种写法做的是同一件事：每一步等 100 毫秒。每一步比 100 毫秒多出七到十几毫秒，是 Windows 定时器的精度（约 15.6 毫秒一档）造成的，不是代码的开销。

```powershell
build\windows-debug\bin\asio_demo.exe concurrent 20 300
```

```
20 waits of 300 ms finished in 315 ms (20 woke), one thread
```

20 个协程各等 300 毫秒，一个线程，总共 315 毫秒；顺序等要 6 秒。这个数字后面和 libcurl 的基准对照。

```powershell
build\windows-debug\bin\asio_demo.exe timeout 100 500
build\windows-debug\bin\asio_demo.exe timeout 500 100
build\windows-debug\bin\asio_demo.exe cancel
```

```
timed out after 100 ms
finished within 500 ms
cancelling after 51 ms
the wait ended 0 ms after the cancel request (cancelled)
```

限制 100 毫秒、活要 500 毫秒，超时；限制 500 毫秒、活要 100 毫秒，正常结束。十秒的等待在 51 毫秒时取消，取消请求发出之后等待立刻结束（0 毫秒是毫秒级计时的下限）。

```powershell
build\windows-debug\bin\asio_demo.exe strand 4 400
build\windows-debug\bin\asio_demo.exe hop
```

```
without a strand: at most 4 task(s) inside at the same time
with a strand:    at most 1 task(s) inside at the same time
event loop      : thread 49780
blocking pool   : thread 62780
event loop again: thread 49780
gpu thread      : thread 37116
event loop again: thread 49780
```

四个线程跑 400 个任务，不用 `strand` 时同一时刻最多有 4 个任务在临界区里，用了之后只有 1 个。`hop` 显示三个执行器是三个不同的线程，而每次干完活都回到同一个事件循环线程（线程号每次运行都不一样）。

## 6. 验证

- `ctest`：112 个测试通过，其中新增的 19 个在 `test_asio_basics` 里：
  - 同一件事的回调写法和协程写法按同样的顺序做完；定时器按到期时间而不是创建顺序触发；从别的线程 `post` 进来的函数在事件循环的线程里运行。
  - 20 个睡眠在一个线程里一起醒来（手动时钟）；真实时钟下 20 个 60 毫秒的睡眠大约一次睡眠的时间做完，不是 20 次的总和。
  - 超时返回值、超时抛出并取消慢的那一个、`cancellation_signal` 让睡眠立刻结束。
  - `Deadline` 随时间缩小；两步共用一个预算，第二步剩下的时间不够就超时；已经过期的截止时间不会再启动这一步。
  - `strand` 保证同一时刻只有一个 handler；`OnPool` 在池线程里干活、回到事件循环线程；池里的异常传到调用者；线程局部变量跟不上协程而显式的 `RequestContext` 跟得上。
  - `TaskGroup`：`Join` 等所有任务；关停时拒绝新任务、取消、等待结束；记下第一个真正的错误并仍然能 `Join`。
- 稳定性：`test_asio_basics` 连续运行 5 次，每次 19 个全部通过。
- `scripts/check-build.ps1`：通过（编译 20 条、链接 12 条）。
- 没有做的：MSVC 没有 ThreadSanitizer，数据竞争只能靠设计（`strand`、一个执行器的规则）和 `strand` 的测试来保证；没有用 AddressSanitizer 跑这些测试。

## 7. 技术资料

- Boost.Asio，Boost 1.92.0，vcpkg 的 `boost-asio`。它是头文件库，只装需要的那一个 Boost 库的依赖。文档：<https://www.boost.org/doc/libs/release/doc/html/boost_asio.html>。
- 为什么用 Boost.Asio 而不是独立版的 `asio`：两者 API 几乎一样，差别在命名空间（`boost::asio` 和 `asio`）。选 Boost 版是因为以后需要 Beast（WebSocket）时可以直接用。
- C++20 协程：MSVC 在 `/std:c++20` 下直接支持，不需要 `/await`。`awaitable_operators`（`||`、`&&`）在 `boost/asio/experimental/` 下，目录名里带 experimental，本章只用它的 `||`，行为由测试覆盖。
- 两个编译定义：`_WIN32_WINNT=0x0A00` 选 Windows 10 的接口，不定义会得到一条“请定义 _WIN32_WINNT”的警告；`BOOST_ASIO_NO_DEPRECATED` 让已废弃的接口直接报错，避免写出以后要改的代码。
- 设计取舍：
  - 为什么用自己的 `Sleeper` 接口，而不是给 Asio 的定时器换一个自定义时钟：自定义时钟要写 `wait_traits`，并且到期仍然要靠真实时间去唤醒，手动推进很绕；`Sleeper` 让测试里所有的等待都变成“登记，等 `Advance`”，简单得多。
  - 为什么 `TaskGroup` 只在一个执行器上使用：加锁会让 `Finished` 和 `Join` 之间的次序更难想清楚。机器人里每个频道一条 `strand`，每个组用在自己的 `strand` 上就够了。
  - 为什么 `OnPool` 用 `co_spawn` 而不是 `post`：见第 8 节。
- 被否决的：libuv（C 接口，没有协程支持，要自己写）；cppcoro、stdexec、Boost.Cobalt（调研文档里看过，Asio 自带的协程已经够用）。

## 8. 常见问题与坑

**一个 lambda 里没有 `co_await`、`co_return`、`co_yield`，它就不是协程。** 写测试时遇到的：一个返回 `awaitable<void>` 的 lambda 里只有一句 `throw`，异常在调用 lambda 的那一刻就抛出来了，而不是在协程里。要让它成为协程，里面至少要有一个 `co_await`。

**`asio::post(pool, use_awaitable)` 不会让协程换线程。** 它只是把“完成通知”投递到 `pool`，协程恢复时仍然回到自己原来的执行器。要在池里运行一段代码，要把代码放进另一个协程，用 `co_spawn(pool, ...)` 去跑它，这就是 `OnPool` 做的事。

**`co_spawn(..., asio::detached)` 会吞掉异常。** 协程里抛出的异常没有人接，整个任务悄悄结束。要接异常，给 `co_spawn` 传一个完成处理器（`TaskGroup` 的 `Finished` 就是这样记下第一个错误的）。

**协程里的引用和指针。** 协程的参数和局部变量在协程帧里，但参数如果是引用，它指向的东西要比协程活得长。lambda 协程有一个陷阱：直接调用 lambda 得到协程之后，如果 lambda 对象先被销毁，它的捕获就悬空了。把 lambda 本身交给 `co_spawn`（而不是先调用它），Asio 会让它活到协程结束。

**`io_context::run()` 在没有事情做的时候直接返回。** 手动时钟下的睡眠者不算“有事情做”，所以用 `ManualSleeper` 的测试要用 `poll()`，不能用 `run()`。`run()` 返回之后想再用，要先 `restart()`。

**用 `cancel()` 叫醒和真的被取消，在 Asio 里看上去一样。** 两者都是 `operation_aborted`。`ManualSleeper` 靠 `fired` 标志区分；`TaskGroup` 的 `Join` 靠 `woken_` 区分，不然 `Join` 自己被取消时会误以为是“任务都结束了”。

**Windows 的定时器精度是 15.6 毫秒左右。** 100 毫秒的等待实际是 107 到 112 毫秒，这是系统的精度，不是 Asio 的开销。测试里用真实时钟的断言都留了很宽的余量。

**线程局部变量跟不上协程。** 协程在 `co_await` 之后可能在另一个线程上恢复，`thread_local` 里存的请求 ID 就丢了。要带的信息放进 `RequestContext`，作为参数显式传下去。

**`TaskGroup` 要先 `Join` 再销毁。** 任务里持有指向它的指针，任务还在跑时销毁组会让它们访问已经释放的内存。

## 9. 练习

1. 把 `asio_demo concurrent` 的数量从 20 改成 2000，等待的时间改成 100 毫秒，总耗时有变化吗？为什么 2000 个协程也不需要 2000 个线程？
2. 给 `TaskGroup` 加一个 `JoinFor(Millis)`：关停时最多等这么久，超过就放弃并返回还有几个任务没结束。用 `ManualSleeper` 写测试。
3. 写一个带重试的函数：失败后等 100、200、400 毫秒再试，最多三次，等待用 `Sleeper`。用 `ManualSleeper` 测出“第三次成功”和“三次都失败”两种结果，不用真实等待。
4. 把 `strand` 的演示改成每个任务睡 1 毫秒的 `co_await`（用 `co_spawn` 到 `strand` 上），看“一个 `strand` 上的协程在 `co_await` 的时候，别的协程能不能进来”，想想这对“一个频道一轮对话”意味着什么。
5. 在 `OnPool` 里让 `work` 运行一个会阻塞三秒的函数，同时在事件循环上跑 20 个 300 毫秒的睡眠。事件循环有没有被拖慢？如果直接在事件循环上调用那个函数呢？

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 llm-1.2 把 libcurl 接到事件循环里：先把 easy、multi、share 三种用法各做成可以单独测试的积木，再做三种协程实现（线程池加 `Session`、一个 curl 线程加 `post`、`curl_multi_socket_action` 接 Asio），讲 HTTP/1.1 与 HTTP/2 的差别，在本机和真实服务上测出差别，决定机器人默认用哪一种。
