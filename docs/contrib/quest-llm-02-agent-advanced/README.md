# 支线 llm-02：Agent 进阶

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/llm-02-agent-advanced`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-2.7-vision-feedback-loop`
- 完成标准：机器人能记住用户的偏好、按语义检索自己以前生成的图、并行调用工具、把外部 MCP 服务当作 Skill 使用，并在节点图出错时根据校验信息自行修正；并能搜索、并只读查询自己的聊天历史

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| llm-2.1 | [并行工具调用](llm-2.1-parallel-tool-calls.md) | `chapter-llm-2.1-parallel-tool-calls:` | 一次响应里有多个工具调用时并发执行，并按协议要求把结果成对返回 |
| llm-2.2 | [嵌入与向量检索](llm-2.2-embeddings-search.md) | `chapter-llm-2.2-embeddings-search:` | 调用嵌入接口得到向量，用余弦相似度做最近邻检索，先暴力扫描再讨论索引 |
| llm-2.3 | [长期记忆：用户偏好](llm-2.3-long-term-memory.md) | `chapter-llm-2.3-long-term-memory:` | 把“我喜欢水彩风格”这类偏好存下来，在后续对话里按相关性取出，并让用户能查看和删除 |
| llm-2.4 | [按语义检索历史作品](llm-2.4-semantic-gallery.md) | `chapter-llm-2.4-semantic-gallery:` | 给每张生成的图配上描述并建立向量索引，支持“找出我上周生成的那只猫” |
| llm-2.5 | [MCP 客户端](llm-2.5-mcp-client.md) | `chapter-llm-2.5-mcp-client:` | 实现 Model Context Protocol 的客户端（基于标准输入输出的 JSON-RPC），把外部工具服务当作 Skill 装载 |
| llm-2.6 | [MCP 服务端](llm-2.6-mcp-server.md) | `chapter-llm-2.6-mcp-server:` | 把本项目的节点和 Skill 作为 MCP 服务暴露出去，让别的程序也能调用 |
| llm-2.7 | [节点图校验与自我纠错](llm-2.7-graph-self-repair.md) | `chapter-llm-2.7-graph-self-repair:` | 在执行前校验节点图并给出可读的错误，把错误回灌给 LLM 让它自己修图 |
| llm-2.8 | [规划与子任务](llm-2.8-planner-worker.md) | `chapter-llm-2.8-planner-worker:` | 让一个 LLM 先拆任务，再分给带不同工具集的子 agent 执行并汇总，讨论什么时候不值得拆 |
| llm-2.9 | [搜索聊天历史（全文检索）](llm-2.9-history-search.md) | `chapter-llm-2.9-history-search:` | 用 SQLite 的 FTS5 搜聊天记录；中文分词是难点：试内置分词器的效果，再对比三字符分词或在应用层切词（到时核实各方案是否可用）；和 llm-2.2 的向量检索对比：关键词与语义各适合什么 |
| llm-2.10 | [让 LLM 查自己的历史](llm-2.10-history-tool.md) | `chapter-llm-2.10-history-tool:` | 提供只读的历史查询工具：参数化查询、只读连接、行数上限；记录里的内容当作数据而不是指令，防止历史里的提示注入；说明它与 llm-2.3 长期记忆的分工 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
