# 支线 llm-01：LLM 应用工程

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-1.7-chat-history`
- 完成标准：测出 libcurl 三种用法的差别并选定机器人里用哪一种；把 libcurl 接进 Boost.Asio 的事件循环（协程）；在 Discord 里机器人边想边发（流式更新同一条消息），输出可校验的结构化 JSON，长对话自动压缩，并给出每个频道的用量与费用；用录好的响应就能确定地回放测试；聊天历史的表结构有版本和升级脚本，一轮对话的消息在一个事务里写入，历史里不存令牌和密钥

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| llm-1.1 | [libcurl 三种用法的对比（easy、multi、share）](llm-1.1-curl-modes.md) | `chapter-llm-1.1-curl-modes:` | 在同一批 LLM 请求上测出 libcurl 的 easy、multi、share 三种用法的差别（各阶段耗时、连接数、总耗时），开启 HTTP/2 并比较它和 HTTP/1.1 多连接的区别，最后选定机器人默认用“线程池加 easy” |
| llm-1.2 | [事件循环：Boost.Asio 的定时器与协程](llm-1.2-asio-basics.md) | `chapter-llm-1.2-asio-basics:` | 弄清事件循环是什么（`io_context`、`post`、executor、strand；Windows 上底层是 IOCP），用 Boost.Asio 写出定时器、跨线程 `post`、取消；先用回调写一遍，再用 C++20 协程（`awaitable`、`co_spawn`、`co_await`）写成顺序代码，对比两种写法；用 `||` 给操作加超时；确认 vcpkg 的 `boost-asio` 在 MSVC 下能编译（`_WIN32_WINNT` 等宏），并写出带测试的小例子；不依赖 llm-1.1 的代码 |
| llm-1.3 | [用 Asio 驱动 libcurl multi](llm-1.3-asio-curl.md) | `chapter-llm-1.3-asio-curl:` | 把 libcurl 的 multi 接进 Asio 的事件循环，两种接法都做并比较：一是一个线程跑 `curl_multi_poll`，完成后 `post` 回 `io_context`（简单，多一个线程）；二是 `curl_multi_socket_action` 加 `CURLMOPT_SOCKETFUNCTION` 和 `CURLMOPT_TIMERFUNCTION`，把 curl 的套接字交给 Asio 的 `assign` 与 `async_wait`，单线程驱动（套接字归 curl 所有，销毁前要 `release`；先做小实验确认 Windows 上 select 反应器与 Schannel 可行）；封装成 `co_await http.Post(request)`，在 `curl_bench` 里加这两种策略，用 llm-1.1 的同一批请求（20 个、每个等 300 ms）和 multi 比较；说明 DPP 有自己的事件循环，回调里用 `co_spawn` 或 `post` 转进我们的 `io_context`；依赖 llm-1.1、llm-1.2 |
| llm-1.4 | [流式输出（SSE）](llm-1.4-streaming-sse.md) | `chapter-llm-1.4-streaming-sse:` | 解析服务端推送的事件流，边收边显示；Discord 里用限速的消息编辑模拟打字效果 |
| llm-1.5 | [结构化输出与 JSON Schema 校验](llm-1.5-structured-output.md) | `chapter-llm-1.5-structured-output:` | 让模型按给定的模式输出 JSON，校验失败就把错误回灌给模型重试 |
| llm-1.6 | [上下文窗口与历史压缩](llm-1.6-context-window.md) | `chapter-llm-1.6-context-window:` | 估算 token 数，窗口放不下时用滑动窗口和摘要压缩旧对话，并保证工具调用与结果成对保留 |
| llm-1.7 | [提示缓存与用量统计](llm-1.7-prompt-cache-usage.md) | `chapter-llm-1.7-prompt-cache-usage:` | 读取响应里的用量字段，按频道统计费用（用量存进表后用 SQL 汇总，见主线 1.7；想试分析型数据库，可把同样的统计用 DuckDB 再做一遍作为练习）；让固定的系统提示与工具列表命中缓存并量化节省 |
| llm-1.8 | [重试、限流与降级](llm-1.8-retry-fallback.md) | `chapter-llm-1.8-retry-fallback:` | 指数退避加抖动，尊重服务端的限流提示，主 Provider 持续失败时切到备用 Provider |
| llm-1.9 | [提示注入与工具权限](llm-1.9-prompt-injection.md) | `chapter-llm-1.9-prompt-injection:` | 演示图片和用户文本里藏指令的攻击，用工具白名单、危险工具二次确认和输出隔离来防御 |
| llm-1.10 | [录制与回放测试](llm-1.10-record-replay.md) | `chapter-llm-1.10-record-replay:` | 把 LLM 的请求和响应录成文件，测试时用假 Provider 回放，让 agent 循环的测试不花钱、可重复 |
| llm-1.11 | [聊天历史进阶：版本迁移、事务与脱敏](llm-1.11-chat-history-sqlite.md) | `chapter-llm-1.11-chat-history-sqlite:` | 在主线 1.7 已有的表上加三件事：用 `user_version` 记录表结构版本并写升级脚本；一轮 agent 循环的所有消息在一个事务里写入，出错就回滚，不留悬空的工具调用；历史里不存令牌和密钥（用户消息里出现的也要先脱敏） |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
