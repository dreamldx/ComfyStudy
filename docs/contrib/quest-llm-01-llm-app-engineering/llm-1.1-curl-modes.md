# llm-1.1 libcurl 三种用法的对比（easy、multi、share）

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.1"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md)
- 一句话目标：对比 libcurl 的 easy、multi、share 三种用法：用 `curl_easy_getinfo` 读出各阶段的耗时（域名解析、建立连接、TLS 握手、首字节、总耗时），对比每次新建句柄和复用同一个句柄；用 `curl_multi_*` 在一个线程里同时发出多个请求，写出 `PostMany`（据我所知还有靠自己的事件循环驱动的 `curl_multi_socket_action`，到时核实并说明区别）；用 `curl_share_*` 让多个句柄共享数据（据我所知可以共享 DNS 缓存、TLS 会话、连接缓存、cookie，到时逐项核实），多线程使用时要提供加锁回调；最后对同一批请求（例如 20 个问题）比较顺序 easy、线程池加 easy、线程池加 easy 与 share、单线程 multi 的总耗时、线程数、内存和代码复杂度，并考虑服务的限流（429），得出机器人里该用哪一种

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-llm-1.1-curl-modes:` 开头，可用 `git show --stat` 查看。）

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
