# ComfyStudy · 支线 platform-02：事件循环和网络

这是支线分支 `quest/platform-02-event-loop-network`。它不合并回 `main`，也不打 tag、不发 Release。
起点是主线 tag `chapter-1.1-http-json`；每一章对应一个或几个提交，提交信息以 `chapter-platform-2.N-` 开头，
用 `git log --grep="^chapter-platform-2.1"` 可以找到本章的提交。
本分支目前的两章（platform-2.1、2.2）在 Windows 上实现和验证；Linux 版是 platform-2.3、2.4，还没有写。

完整的教程在 <https://dreamldx.github.io/ComfyStudy/>，这一条支线的说明在
[支线 platform-02：事件循环和网络](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-02-event-loop-network/)。
下面按章节列出这个分支上已经实现了什么、为什么这样做，以及每一章详细讲解的链接。

## 构建与运行

需要 Visual Studio（MSVC x64）、CMake、Ninja 和 vcpkg，见主线 0.1 到 0.3。在仓库根目录：

```powershell
.\scripts\dev.ps1 cmake --preset windows-debug
.\scripts\dev.ps1 cmake --build --preset windows-debug
.\scripts\dev.ps1 ctest --preset windows-debug
```

## 本分支实现了什么

### platform-2.1 事件循环：Boost.Asio 的定时器与协程（Windows）

- 提交：`chapter-platform-2.1-asio-basics:`
- 目的：后面所有的网络请求、流式输出和 Discord 回调都要在一个事件循环里协调，先把事件循环本身讲清楚、测清楚，再去接 libcurl。
- 说明：
  - `src/event_loop.h`（只有头文件）：`Sleeper`（真实时钟 `SteadySleeper`，测试里手动推进时间的 `ManualSleeper`）、
    `WithTimeout` 与 `Deadline`（超时和整个请求共用的截止时间）、`Executors` 与 `OnPool`（事件循环、阻塞线程池、GPU 线程三个执行器，
    切过去干活再回来）、`TaskGroup`（一组任务：启动、取消、等待结束，关停时用）、`RequestContext`（跨线程显式传递的上下文）。
  - `demo/asio_demo.cpp`：`callbacks`、`coroutines`、`concurrent`、`timeout`、`cancel`、`strand`、`hop` 七个子命令，
    例如 `build\windows-debug\bin\asio_demo.exe concurrent 20 300` 在一个线程里同时等 20 个 300 ms，总共约 300 ms。
  - `tests/test_asio_basics.cpp`：19 个测试，需要控制时间的都用 `ManualSleeper`，用真实时钟的留了很宽的余量。
- 文档：[platform-2.1 事件循环：Boost.Asio 的定时器与协程（Windows）](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-02-event-loop-network/platform-2.1-asio-basics/)

### platform-2.2 libcurl 三种用法：在事件循环里的实现与对比（Windows）（进行中）

- 提交：`chapter-platform-2.2-curl-modes:`
- 目的：机器人要一次问很多个问题，而主线 1.1 的 `http::Post` 一次只发一个请求、每次新建句柄。先把 libcurl 的 easy、multi、share 三种用法各做成可以单独测试的积木，再在后面的提交里并排比较。
- 说明：
  - 每个请求各阶段的耗时（`Timing`：域名解析、建立连接、TLS 握手、首字节、总耗时，以及有没有新建连接）。
  - `http::Session`：保留一个句柄，连续请求时复用连接（easy）。
  - `http::PostMany`：在一个线程里同时发出多个请求（multi），可以限制同时在途的数量。
  - `http::Share`：让多个线程的句柄共享 DNS 缓存和 TLS 会话（share；不共享连接缓存，共享连接缓存时 libcurl 会崩）。
  - vcpkg 的 `curl[http2]`，以及 `Options::version` 可以强制 HTTP/1.1 或 HTTP/2。
  - `tests/test_http_modes.cpp`：9 个测试，本机用 cpp-httplib 起一个小服务器。
  - `bench/curl_bench.cpp`（目标 `curl_bench`，不属于机器人程序）：在本机服务器（每个请求等 300 ms）和真实的模型服务上，比较顺序发送、线程池、`Share`、`Session` 和 multi 等发送方式的总耗时与新建连接数。用法：`curl_bench local N DELAY_MS THREADS REPS`、`curl_bench llm N THREADS any|1.1|2`（真实服务的地址和密钥从 `config.ini` 读，不会打印）。
- 文档：[platform-2.2 libcurl 三种用法：在事件循环里的实现与对比（Windows）](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-02-event-loop-network/platform-2.2-curl-modes/)
