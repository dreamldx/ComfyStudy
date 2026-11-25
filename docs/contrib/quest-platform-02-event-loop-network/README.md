# 支线 platform-02：事件循环和网络

- 类型：支线（2.1、2.2 已写，2.3、2.4 是草案，到该阶段前再细化）
- 状态：进行中（2.1、2.2 已验证，2.3、2.4 未开始）
- 分支：`quest/platform-02-event-loop-network`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-01-linux-foundation` 的末端，再合入主线 tag `chapter-1.1-http-json`（platform-01 还没有代码，所以现在分支直接从主线 tag 开始）
- 类别：功能支线（依附：主线阶段 1）
- 完成标准：Boost.Asio 的事件循环和 libcurl 三种用法的对比，在 Windows 上做完并验证（2.1、2.2），在 Linux 上做同样的事并记录差异（2.3、2.4）

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-2.1 | [事件循环：Boost.Asio 的定时器与协程（Windows）](platform-2.1-asio-basics.md) | `chapter-platform-2.1-asio-basics:` | （Windows）弄清事件循环是什么，用 Boost.Asio 写出定时器、协程、超时和取消，讲清多线程 `io_context` 与 `strand`；在三个执行器（事件循环、阻塞线程池、GPU 线程）之间切换；让一个请求的所有步骤共用一个截止时间；用 `TaskGroup` 做关停；用可以手动推进的时钟让测试不依赖真实时间；本章不连网络，后面的章节在它上面接 libcurl |
| platform-2.2 | [libcurl 三种用法：在事件循环里的实现与对比（Windows）](platform-2.2-curl-modes.md) | `chapter-platform-2.2-curl-modes:` | （Windows）把 libcurl 的 easy、multi、share 三种用法做成积木（`Timing`、`Session`、`PostMany`、`Share`），再做三种协程实现并排比较：线程池加 `Session`、一个 curl 线程加 `post`、`curl_multi_socket_action` 接 Asio；讲 HTTP/1.1 与 HTTP/2 的原理和差别，在本机基准（Release）和真实服务（OpenRouter、Anthropic，OpenAI 官方只记 ALPN）上测出差别，得出机器人默认用哪一种；依赖 platform-2.1 |
| platform-2.3 | [事件循环（Linux）](platform-2.3-asio-linux.md) | `chapter-platform-2.3-asio-linux:` | 在 Linux 上重做 2.1：同一套 Boost.Asio 代码在 Linux 的事件循环（epoll）上跑，测试全部通过，记录与 Windows（IOCP）的差异 |
| platform-2.4 | [libcurl 三种用法（Linux）](platform-2.4-curl-linux.md) | `chapter-platform-2.4-curl-linux:` | 在 Linux 上重做 2.2 的对比：线程池、curl 线程、socket_action 三种协程客户端的结果与 Windows 对照；TLS 后端不同（Windows 上是 Schannel）；socket_action 方案在 Linux 上的差别 |

## 阶段总览

依据：主线 1.1 起所有的网络请求都在 Boost.Asio 的事件循环里，用 libcurl 的 multi 接口在一个线程里发出。事件循环的底层在 Windows 和 Linux 上不同（IOCP 与 epoll），libcurl 的 `curl_multi_socket_action` 方案对平台的假设也不同。前两章在 Windows 上做完，后两章在 Linux 上重做并记录差异。

后面的 llm-01 依赖这里：流式输出、钩子等章节建立在 platform-2.2 选定的 curl 线程方案上。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
