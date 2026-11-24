# 支线 platform-12：Agent 进阶（Linux 完整版）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-12-agent-advanced-linux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/llm-02-agent-advanced` 的末端，再合入分支 `quest/platform-01-linux-foundation` 的末端
- 类别：功能支线（依附：支线 llm-02）
- 完成标准：Linux 上 llm-02 的全部章节构建通过，并记录与 Windows 的差异

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-12.1 | [并行工具调用](platform-12.1-parallel-tool-calls.md) | `chapter-platform-12.1-parallel-tool-calls:` | Linux 版 llm-2.1：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-12.2 | [嵌入与向量检索](platform-12.2-embeddings-search.md) | `chapter-platform-12.2-embeddings-search:` | Linux 版 llm-2.2：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-12.3 | [长期记忆：用户偏好](platform-12.3-long-term-memory.md) | `chapter-platform-12.3-long-term-memory:` | Linux 版 llm-2.3：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-12.4 | [按语义检索历史作品](platform-12.4-semantic-gallery.md) | `chapter-platform-12.4-semantic-gallery:` | Linux 版 llm-2.4：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-12.5 | [MCP 客户端](platform-12.5-mcp-client.md) | `chapter-platform-12.5-mcp-client:` | Linux 版 llm-2.5：用 `posix_spawn`、管道与信号处理实现基于标准输入输出的子进程（处理僵尸进程与 `SIGPIPE`） |
| platform-12.6 | [MCP 服务端](platform-12.6-mcp-server.md) | `chapter-platform-12.6-mcp-server:` | Linux 版 llm-2.6：在 Linux 上验证被其他程序拉起时的标准输入输出 |
| platform-12.7 | [节点图校验与自我纠错](platform-12.7-graph-self-repair.md) | `chapter-platform-12.7-graph-self-repair:` | Linux 版 llm-2.7：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-12.8 | [规划与子任务](platform-12.8-planner-worker.md) | `chapter-platform-12.8-planner-worker:` | Linux 版 llm-2.8：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-12.9 | [搜索聊天历史（全文检索）](platform-12.9-history-search.md) | `chapter-platform-12.9-history-search:` | Linux 版 llm-2.9：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-12.10 | [让 LLM 查自己的历史](platform-12.10-history-tool.md) | `chapter-platform-12.10-history-tool:` | Linux 版 llm-2.10：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |

## 阶段总览

草案，待验证：llm-2.5 基于标准输入输出的子进程 JSON-RPC，是否有 Windows 专属实现还没核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
