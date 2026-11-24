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
