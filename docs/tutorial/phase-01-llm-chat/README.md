# 阶段 1：LLM 与对话通道

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-01-llm-chat`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：在 Discord 里与机器人对话，它能调用玩具工具并回答，也能看懂用户发来的图片

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 1.1 | [HTTP 与 JSON](1.1-http-json.md) | `chapter-1.1-http-json` | 用 libcurl 发请求，对比 OpenAI 与 Anthropic 的消息格式 |
| 1.2 | [LLM 双协议与 Provider 抽象](1.2-llm-providers.md) | `chapter-1.2-llm-providers` | 命令行对话接入两种协议，先读 ai-sdk-cpp 的设计再决定使用或自写 |
| 1.3 | [Tool Calling 循环](1.3-tool-calling.md) | `chapter-1.3-tool-calling` | 用玩具工具跑通 agent 循环，对比 `tool_calls` 与 `tool_use` |
| 1.4 | [Discord 机器人](1.4-discord-bot.md) | `chapter-1.4-discord-bot` | 用 DPP 收发消息，每频道独立会话，用户白名单，串行任务队列 |
| 1.5 | [LLM 视觉输入](1.5-vision-input.md) | `chapter-1.5-vision-input` | 支持图像输入：对比两种协议的图像消息格式（base64 与 URL）、大小限制，以及工具结果里能否带图（据我所知 OpenAI 的 tool 消息只能是文本，需改用后续 user 消息，需实测）；Discord 附件转为图像输入 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
