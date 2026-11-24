# llm-1.3 用 Asio 驱动 libcurl multi

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.3"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md)
- 一句话目标：把 libcurl 的 multi 接进 Asio 的事件循环，两种接法都做并比较：一是一个线程跑 `curl_multi_poll`，完成后 `post` 回 `io_context`（简单，多一个线程）；二是 `curl_multi_socket_action` 加 `CURLMOPT_SOCKETFUNCTION` 和 `CURLMOPT_TIMERFUNCTION`，把 curl 的套接字交给 Asio 的 `assign` 与 `async_wait`，单线程驱动（套接字归 curl 所有，销毁前要 `release`；先做小实验确认 Windows 上 select 反应器与 Schannel 可行）；封装成 `co_await http.Post(request)`，在 `curl_bench` 里加这两种策略，用 llm-1.1 的同一批请求（20 个、每个等 300 ms）和 multi 比较；说明 DPP 有自己的事件循环，回调里用 `co_spawn` 或 `post` 转进我们的 `io_context`；依赖 llm-1.1、llm-1.2

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。（要点：本章落到结论：`Transport` 接口的签名和取舍（`asio::awaitable<Response> Post(PostRequest, Options)`，时钟可注入，录制回放与各种 libcurl 实现都是它的实现）；取消如何传到在途的 HTTP 请求；传输层结果带错误分类（可重试、不可重试），供重试使用；日志里对 `Authorization` 脱敏；汇总对比章得出的传输层结论；DPP 的回调在池线程里执行，用 `post` 或 `co_spawn` 转进我们的 `io_context`，不在回调里 `co_await`；`Transport` 的契约测试：同一套测试跑遍所有实现（libcurl 的三种、录制回放、假实现），保证行为一致。）

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-llm-1.3-asio-curl:` 开头，可用 `git show --stat` 查看。）

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
