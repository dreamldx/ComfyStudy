# llm-1.2 事件循环：Boost.Asio 的定时器与协程

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.2"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md)
- 一句话目标：弄清事件循环是什么（`io_context`、`post`、executor、strand；Windows 上底层是 IOCP），用 Boost.Asio 写出定时器、跨线程 `post`、取消；先用回调写一遍，再用 C++20 协程（`awaitable`、`co_spawn`、`co_await`）写成顺序代码，对比两种写法；用 `||` 给操作加超时；确认 vcpkg 的 `boost-asio` 在 MSVC 下能编译（`_WIN32_WINNT` 等宏），并写出带测试的小例子；不依赖 llm-1.1 的代码

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-llm-1.2-asio-basics:` 开头，可用 `git show --stat` 查看。）

## 4. 关键代码讲解

待写。（贴真实片段并注明文件与行号范围。）

## 5. 构建与运行

待写。（可复制的命令与预期输出。）

## 6. 验证

待写。（本章的 `ctest` 或手动检查项。）

## 7. 技术资料

待写。（用到的库、版本、API、设计取舍、被否决的方案。）

## 8. 常见问题与坑

待写。

## 9. 练习

待写。

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

待写。
