# 支线 llm-01：LLM 应用工程

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-1.9-chat-history`
- 完成标准：搭好 Boost.Asio 事件循环；测出 libcurl 三种用法在事件循环里的差别并选定机器人里用哪一种；在 Discord 里机器人边想边发（流式更新同一条消息），输出可校验的结构化 JSON，长对话自动压缩，并给出每个频道的用量与费用；用录好的响应就能确定地回放测试；聊天历史的表结构有版本和升级脚本，一轮对话的消息在一个事务里写入，历史里不存令牌和密钥

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| llm-1.1 | [事件循环：Boost.Asio 的定时器与协程](llm-1.1-asio-basics.md) | `chapter-llm-1.1-asio-basics:` | 弄清事件循环是什么，用 Boost.Asio 写出定时器、协程、超时和取消，讲清多线程 `io_context` 与 `strand`；在三个执行器（事件循环、阻塞线程池、GPU 线程）之间切换；让一个请求的所有步骤共用一个截止时间；用 `TaskGroup` 做关停；用可以手动推进的时钟让测试不依赖真实时间；本章不连网络，后面的章节在它上面接 libcurl |
| llm-1.2 | [libcurl 三种用法：在事件循环里的实现与对比](llm-1.2-curl-modes.md) | `chapter-llm-1.2-curl-modes:` | 把 libcurl 的 easy、multi、share 三种用法做成积木（`Timing`、`Session`、`PostMany`、`Share`），再做三种协程实现并排比较：线程池加 `Session`、一个 curl 线程加 `post`、`curl_multi_socket_action` 接 Asio；讲 HTTP/1.1 与 HTTP/2 的原理和差别，在本机基准（Release）和真实服务（OpenRouter、Anthropic，OpenAI 官方只记 ALPN）上测出差别，得出机器人默认用哪一种；依赖 llm-1.1 |
| llm-1.3 | [流式输出（SSE）](llm-1.3-streaming-sse.md) | `chapter-llm-1.3-streaming-sse:` | 解析服务端推送的事件流，边收边显示；Discord 里用限速的消息编辑模拟打字效果 |
| llm-1.4 | [结构化输出与 JSON Schema 校验](llm-1.4-structured-output.md) | `chapter-llm-1.4-structured-output:` | 让模型按给定的模式输出 JSON，校验失败就把错误回灌给模型重试 |
| llm-1.5 | [上下文窗口与历史压缩](llm-1.5-context-window.md) | `chapter-llm-1.5-context-window:` | 估算 token 数，窗口放不下时用滑动窗口和摘要压缩旧对话，并保证工具调用与结果成对保留 |
| llm-1.6 | [提示缓存与用量统计](llm-1.6-prompt-cache-usage.md) | `chapter-llm-1.6-prompt-cache-usage:` | 读取响应里的用量字段，按频道统计费用（用量存进表后用 SQL 汇总，见主线 1.9；想试分析型数据库，可把同样的统计用 DuckDB 再做一遍作为练习）；让固定的系统提示与工具列表命中缓存并量化节省 |
| llm-1.7 | [重试、限流与降级](llm-1.7-retry-fallback.md) | `chapter-llm-1.7-retry-fallback:` | 指数退避加抖动，尊重服务端的限流提示，主 Provider 持续失败时切到备用 Provider |
| llm-1.8 | [提示注入与工具权限](llm-1.8-prompt-injection.md) | `chapter-llm-1.8-prompt-injection:` | 演示图片和用户文本里藏指令的攻击，用工具白名单、危险工具二次确认和输出隔离来防御 |
| llm-1.9 | [录制与回放测试](llm-1.9-record-replay.md) | `chapter-llm-1.9-record-replay:` | 把 LLM 的请求和响应录成文件，测试时用假 Transport 回放，让 agent 循环的测试不花钱、可重复 |
| llm-1.10 | [聊天历史进阶：版本迁移、事务与脱敏](llm-1.10-chat-history-sqlite.md) | `chapter-llm-1.10-chat-history-sqlite:` | 在主线 1.9 已有的表上加三件事：用 `user_version` 记录表结构版本并写升级脚本；一轮 agent 循环的所有消息在一个事务里写入，出错就回滚，不留悬空的工具调用；历史里不存令牌和密钥（用户消息里出现的也要先脱敏） |
| llm-1.11 | [Agent 循环的钩子（中间件）](llm-1.11-agent-hooks.md) | `chapter-llm-1.11-agent-hooks:` | 在 Agent 循环的固定位置（模型调用前后、工具调用前后）留出钩子，把脱敏、预算、轨迹、权限确认做成钩子，而不是写进循环；讲清钩子的执行顺序、能不能短路（比如确认被拒绝就不执行工具）、钩子自己出错怎么办；与 Agent 事件的区别是事件只观察，钩子可以改写和中止；用假 Transport 测试钩子；依赖主线 1.3、1.4 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
