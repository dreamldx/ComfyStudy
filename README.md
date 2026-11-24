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

### platform-2.2 libcurl 三种用法：在事件循环里的实现与对比（Windows）

- 提交：`chapter-platform-2.2-curl-modes:`
- 目的：机器人要一次问很多个问题。这一章先把 libcurl 的 easy、multi、share 三种用法各做成可以单独测试的积木，再在后面的提交里并排比较。主线 1.1 的 `http::Transport` 采用的就是比较出来的结论：一个线程跑 libcurl 的 multi 接口。
- 说明：
  - 每个请求各阶段的耗时（`Timing`：域名解析、建立连接、TLS 握手、首字节、总耗时，以及有没有新建连接）。
  - `http::Session`：保留一个句柄，连续请求时复用连接（easy）。
  - `http::PostMany`：在一个线程里同时发出多个请求（multi），可以限制同时在途的数量。
  - `http::Share`：让多个线程的句柄共享 DNS 缓存和 TLS 会话（share；不共享连接缓存，共享连接缓存时 libcurl 会崩）。
  - vcpkg 的 `curl[http2]`，以及 `Options::version` 可以强制 HTTP/1.1 或 HTTP/2。
  - `tests/test_http_modes.cpp`：9 个测试，本机用 cpp-httplib 起一个小服务器。
  - `src/async_http.h`、`src/async_http.cpp`：三个协程客户端，签名相同：`boost::asio::awaitable<http::Response> Post(http::PostRequest)`。线程池方案（`PoolClient`）把阻塞的请求放进线程池，每个线程各有一个 `Session`；curl 线程方案（`CurlThreadClient`）用一个专门的线程跑 `curl_multi_poll`，完成后回到协程所在的执行器；socket_action 方案（`SocketClient`）用 `curl_multi_socket_action`，把 libcurl 的套接字交给事件循环等待，没有额外的线程。curl 线程方案和 socket_action 方案可以取消在途的请求，线程池方案不能。
  - `src/curl_support.h`：上面几个文件共用的 libcurl 辅助代码（原来在 `http_client.cpp` 里），内部使用。
  - `tests/test_async_http.cpp`：23 个测试，同一组测试（`TYPED_TEST`）跑遍三个客户端：答案、错误状态、20 个请求同时在途、一个请求失败不影响其他、超时只影响那一个、协程回到自己的线程；curl 线程方案和 socket_action 方案另有取消和“销毁客户端时还在途的请求”两个测试，线程池方案有一个“取消要等当前请求结束”的测试。
  - Windows 上用事件循环等 libcurl 的套接字有一个坑：连接被拒绝不是“可写”，而是出现在 select 的异常集合里，不额外等 `wait_error` 的话，失败要等到 libcurl 自己的连接超时（默认 10 秒）才发现。
  - `bench/curl_bench.cpp`（目标 `curl_bench`，不属于机器人程序）：在本机服务器（每个请求等 300 ms）和真实的模型服务上，比较顺序发送、线程池、`Share`、`Session`、multi，以及三个协程客户端的总耗时和新建连接数。用法：`curl_bench local N DELAY_MS THREADS REPS`、`curl_bench llm N THREADS any|1.1|2 [--llm_provider anthropic]`（真实服务的地址和密钥从 `config.ini` 读，不会打印）。三个只读的探测：`curl_bench probe`（三家服务协商出的协议、并发流上限、`Alt-Svc`，不需要密钥）、`curl_bench idle`（连接空闲 5、30、90 秒后还在不在）、`curl_bench stream`（一个流式请求在 HTTP/2 下的到达情况）；再加一个 `curl_bench mux`：12 到 300 个请求同时在途，比较 HTTP/1.1 和 HTTP/2 下的连接数、同时在途的请求数和总耗时。
- 文档：[platform-2.2 libcurl 三种用法：在事件循环里的实现与对比（Windows）](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-02-event-loop-network/platform-2.2-curl-modes/)
