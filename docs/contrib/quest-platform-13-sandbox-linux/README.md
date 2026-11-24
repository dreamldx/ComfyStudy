# 支线 platform-13：沙箱与脚本编排（Linux 完整版）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-13-sandbox-linux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/llm-05-sandbox` 的末端，再合入分支 `quest/platform-01-linux-foundation` 的末端
- 类别：功能支线（依附：支线 llm-05）
- 完成标准：Linux 上 llm-05 的全部章节构建通过，子进程沙箱改用 Linux 的机制并通过逃逸测试

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-13.1 | [沙箱的威胁模型](platform-13.1-threat-model.md) | `chapter-platform-13.1-threat-model:` | Linux 版 llm-5.1：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-13.2 | [嵌入脚本语言与白名单函数](platform-13.2-embedded-script.md) | `chapter-platform-13.2-embedded-script:` | Linux 版 llm-5.2：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-13.3 | [资源限制：时间、内存与指令数](platform-13.3-resource-limits.md) | `chapter-platform-13.3-resource-limits:` | Linux 版 llm-5.3：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-13.4 | [子进程沙箱（Linux）](platform-13.4-process-sandbox.md) | `chapter-platform-13.4-process-sandbox:` | Linux 版 llm-5.4：改成 Linux 子进程沙箱：`rlimit` 与 cgroup v2 限制时间、内存与进程数，namespaces 与 seccomp 隔离文件与网络，并与 5.2 的嵌入脚本语言对照 |
| platform-13.5 | [用脚本编排工具调用](platform-13.5-script-orchestration.md) | `chapter-platform-13.5-script-orchestration:` | Linux 版 llm-5.5：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-13.6 | [调用预算与失败回报](platform-13.6-call-budget.md) | `chapter-platform-13.6-call-budget:` | Linux 版 llm-5.6：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-13.7 | [沙箱逃逸测试](platform-13.7-escape-tests.md) | `chapter-platform-13.7-escape-tests:` | Linux 版 llm-5.7：逃逸用例在 Linux 上增加对应的用例 |
| platform-13.8 | [sandbox Skill](platform-13.8-sandbox-skill.md) | `chapter-platform-13.8-sandbox-skill:` | Linux 版 llm-5.8：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |

## 阶段总览

llm-5.4 的子进程沙箱用了 Windows 的 Job Object 与 AppContainer，Linux 要换成另一套机制。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
