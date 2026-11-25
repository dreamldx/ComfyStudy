# 支线 llm-01：LLM 应用工程

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-02-event-loop-network` 的末端，再合入主线 tag `chapter-1.9-chat-history`
- 完成标准：在 Discord 里机器人边想边发（流式更新同一条消息），输出可校验的结构化 JSON，长对话自动压缩，并给出每个频道的用量与费用；用录好的响应就能确定地回放测试；聊天历史的表结构有版本和升级脚本，一轮对话的消息在一个事务里写入，历史里不存令牌和密钥

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| llm-1.1 | [Agent 钩子：改写、中止，以及用钩子做出的能力](llm-1.1-agent-hooks.md) | `chapter-llm-1.1-agent-hooks:` | 先在主线 1.3 的只读钩子上加上改写和中止的能力（新增“模型调用之前”“围着模型调用”“围着工具调用”几个位置），再用钩子做出具体的能力：脱敏、预算与用量统计、提示缓存、重试与降级、权限确认和提示注入的防御；讲清钩子的执行顺序、能不能短路（比如确认被拒绝就不执行工具）、钩子自己出错怎么办；用假 Transport 测试钩子 |
| llm-1.2 | [流式输出（SSE）](llm-1.2-streaming-sse.md) | `chapter-llm-1.2-streaming-sse:` | 解析服务端推送的事件流，边收边显示；Discord 里用限速的消息编辑模拟打字效果 |
| llm-1.3 | [结构化输出与 JSON Schema 校验](llm-1.3-structured-output.md) | `chapter-llm-1.3-structured-output:` | 让模型按给定的模式输出 JSON，校验失败就把错误回灌给模型重试 |
| llm-1.4 | [上下文窗口与历史压缩](llm-1.4-context-window.md) | `chapter-llm-1.4-context-window:` | 估算 token 数，窗口放不下时用滑动窗口和摘要压缩旧对话，并保证工具调用与结果成对保留 |
| llm-1.5 | [录制与回放测试](llm-1.5-record-replay.md) | `chapter-llm-1.5-record-replay:` | 把 LLM 的请求和响应录成文件，测试时用假 Transport 回放，让 agent 循环的测试不花钱、可重复 |
| llm-1.6 | [聊天历史进阶：版本迁移、事务与脱敏](llm-1.6-chat-history-sqlite.md) | `chapter-llm-1.6-chat-history-sqlite:` | 在主线 1.9 已有的表上加三件事：用 `user_version` 记录表结构版本并写升级脚本；一轮 agent 循环的所有消息在一个事务里写入，出错就回滚，不留悬空的工具调用；历史里不存令牌和密钥（用户消息里出现的也要先脱敏） |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
