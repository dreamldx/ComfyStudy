# 阶段 1：LLM 与对话通道

- 类型：主线
- 状态：进行中（已完成 1.1）
- 阶段 tag：`phase-01-llm-chat`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：在 Discord 里与机器人对话，它能调用玩具工具并回答，也能看懂用户发来的图片，重启后能接着聊

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 1.1 | [HTTP 与 JSON](1.1-http-json.md) | `chapter-1.1-http-json` | 用 libcurl 发出异步的 HTTP 请求：定义 `Transport` 接口和它的 libcurl 实现（错误分类、取消、日志里藏起密钥都在这里定下来），把同一个问题按 OpenAI Chat Completions、OpenAI Responses 和 Anthropic Messages 三种格式发给模型，读出回答，并对比三种格式的差别 |
| 1.2 | [LLM 双协议与 Provider 抽象](1.2-llm-providers.md) | `chapter-1.2-llm-providers` | 命令行对话接入两种协议，先读 ai-sdk-cpp 的设计再决定使用或自写 |
| 1.3 | [Tool Calling 循环](1.3-tool-calling.md) | `chapter-1.3-tool-calling` | 用玩具工具跑通 agent 循环，对比 `tool_calls` 与 `tool_use` |
| 1.4 | [Skill 机制：按需加载的能力](1.4-skills.md) | `chapter-1.4-skills` | 在 1.3 的循环上实现 Skill 目录、`load_skill`/`unload_skill` 与按会话动态工具集，用玩具 Skill 演示按需加载 |
| 1.5 | [上下文规划与组装](1.5-context-assembly.md) | `chapter-1.5-context-assembly` | 设计并实现上下文的组装：把系统提示词、历史对话、当前状态、最新用户信息按固定的顺序拼成发给模型的内容（与具体协议无关），再由 Provider 转成各自的格式；组装是一个纯函数，同样的输入得到逐字节相同的输出，并能打印每一部分占多少 token |
| 1.6 | [Discord 机器人](1.6-discord-bot.md) | `chapter-1.6-discord-bot` | 用 DPP 收发消息，每频道独立会话，串行任务队列 |
| 1.7 | [LLM 视觉输入](1.7-vision-input.md) | `chapter-1.7-vision-input` | 支持图像输入：对比两种协议的图像消息格式（base64 与 URL）、大小限制，以及工具结果里能否带图（据我所知 OpenAI 的 tool 消息只能是文本，需改用后续 user 消息，需实测）；Discord 附件转为图像输入 |
| 1.8 | [聊天历史持久化（SQLite）](1.8-chat-history.md) | `chapter-1.8-chat-history` | 用 vcpkg 引入 SQLite，写一层很薄的 RAII 封装（连接、预处理语句、绑定参数）；把每个频道的会话（文本、图片引用、工具调用、工具结果）存成表，图片只存路径；每轮对话结束后落盘，启动时读回，重启后接着聊 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
