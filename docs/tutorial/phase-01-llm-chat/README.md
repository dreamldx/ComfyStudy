# 阶段 1：LLM 与对话通道

- 类型：主线
- 状态：进行中（已完成 1.1 至 1.6）
- 阶段 tag：`phase-01-llm-chat`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：在 Discord 里与机器人对话，它能调用玩具工具并回答，也能看懂用户发来的图片，重启后能接着聊

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 1.1 | [HTTP 与 JSON](1.1-http-json.md) | `chapter-1.1-http-json` | 用 libcurl 发出异步的 HTTP 请求：定义 `Transport` 接口和它的 libcurl 实现（错误分类、取消、日志里藏起密钥都在这里定下来），把同一个问题按 OpenAI Chat Completions、OpenAI Responses 和 Anthropic Messages 三种格式发给模型，读出回答，并对比三种格式的差别 |
| 1.2 | [LLM 双协议与 Provider 抽象](1.2-llm-providers.md) | `chapter-1.2-llm-providers` | 命令行里能多轮对话，三种请求格式收在同一个入口后面；消息由文本块和推理块组成，推理块原样保存、原样回传，对话中途可以用 `/use` 换 Provider |
| 1.3 | [Tool Calling 循环](1.3-tool-calling.md) | `chapter-1.3-tool-calling` | 模型自己决定调用工具，程序执行后把结果送回去，循环到模型不再调用为止；一个 Turn 由若干个 Step 组成，可以被 Ctrl+C 取消或超时，命令行的输出全部走 Agent 钩子 |
| 1.4 | [工具运行时：注册、校验与执行](1.4-tool-runtime.md) | `chapter-1.4-tool-runtime` | 把工具做成一层运行时：所有来源（C++ 原生、以后的 Skill、MCP、脚本）走同一个 `ToolSource` 接口；每次调用先按 Schema 校验参数（一次列出全部问题），带着时间限制执行，结果过长就在字符边界截断，出错都作为结果回传；工具定义只写一份（C++ 结构体加 Glaze 生成 Schema）；工具按只读、有副作用、危险分类，危险的默认拒绝 |
| 1.5 | [Skill 机制：按需加载的能力](1.5-skills.md) | `chapter-1.5-skills` | 在 1.3 的循环和 1.4 的工具运行时上实现 Skill：模型一开始只看到每个 Skill 的名字和任务，用 `load_skill` 加载才得到用法和它带的工具，`unload_skill` 拿走；加载是真的登记进运行时，共享的工具按工具名引用计数；失败的回合连 Skill 一起退回，`/reset` 卸载全部；用三个玩具 Skill 演示并量化它省下的 token |
| 1.6 | [上下文规划与组装](1.6-context-assembly.md) | `chapter-1.6-context-assembly` | 设计并实现上下文的组装：一个纯函数把工具、系统提示词（几节）、历史对话、状态块、最新用户消息和本回合已产生的消息按固定的顺序拼成发给模型的内容（与具体协议无关，同样的输入得到逐字节相同的输出）；有 token 预算，放不下时整个回合地丢最老的历史，工具调用与结果永远成对；历史按频道放在 `History` 接口后面；`--show-context` 打印每一部分占多少 token，并与服务实际数的对比；把不变的开头标出来（Anthropic 的缓存标记）并读出三种回答里的缓存读数，让提示缓存真的命中 |
| 1.7 | [Discord 机器人](1.7-discord-bot.md) | `chapter-1.7-discord-bot` | 用 DPP 收发消息，每频道独立会话，串行任务队列 |
| 1.8 | [LLM 视觉输入](1.8-vision-input.md) | `chapter-1.8-vision-input` | 支持图像输入：对比两种协议的图像消息格式（base64 与 URL）、大小限制，以及工具结果里能否带图（据我所知 OpenAI 的 tool 消息只能是文本，需改用后续 user 消息，需实测）；Discord 附件转为图像输入 |
| 1.9 | [聊天历史持久化（SQLite）](1.9-chat-history.md) | `chapter-1.9-chat-history` | 用 vcpkg 引入 SQLite，写一层很薄的 RAII 封装（连接、预处理语句、绑定参数）；把每个频道的会话（文本、图片引用、工具调用、工具结果）存成表，图片只存路径；每轮对话结束后落盘，启动时读回，重启后接着聊 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
