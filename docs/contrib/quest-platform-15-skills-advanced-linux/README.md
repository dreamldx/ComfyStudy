# 支线 platform-15：Skills 进阶（Linux 完整版）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-15-skills-advanced-linux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/llm-04-skills-advanced` 的末端，再合入分支 `quest/platform-01-linux-foundation` 的末端（4.4、4.5 还需要 platform-14 的沙箱）
- 类别：功能支线（依附：支线 llm-04）
- 完成标准：Linux 上 llm-04 的全部章节构建通过，并记录与 Windows 的差异

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-15.1 | [Skill 文件格式与元数据](platform-15.1-skill-format.md) | `chapter-platform-15.1-skill-format:` | Linux 版 llm-4.1：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-15.2 | [渐进式披露](platform-15.2-progressive-disclosure.md) | `chapter-platform-15.2-progressive-disclosure:` | Linux 版 llm-4.2：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-15.3 | [说明怎么写：让模型选对 Skill](platform-15.3-skill-descriptions.md) | `chapter-platform-15.3-skill-descriptions:` | Linux 版 llm-4.3：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-15.4 | [Skill 自带的资源与脚本](platform-15.4-skill-resources.md) | `chapter-platform-15.4-skill-resources:` | Linux 版 llm-4.4：使用 platform-14 的 Linux 沙箱运行 Skill 自带的脚本 |
| platform-15.5 | [权限与沙箱](platform-15.5-skill-permissions.md) | `chapter-platform-15.5-skill-permissions:` | Linux 版 llm-4.5：权限与沙箱使用 platform-14 的 Linux 沙箱 |
| platform-15.6 | [热重载与版本](platform-15.6-skill-hot-reload.md) | `chapter-platform-15.6-skill-hot-reload:` | Linux 版 llm-4.6：用 inotify 监视目录（处理重命名、批量写入的合并、事件队列溢出）；如果 llm-4.6 用库或轮询实现，就没有平台差异 |
| platform-15.7 | [从录制生成 Skill](platform-15.7-skill-from-recording.md) | `chapter-platform-15.7-skill-from-recording:` | Linux 版 llm-4.7：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-15.8 | [Skill 评测与回归](platform-15.8-skill-evals.md) | `chapter-platform-15.8-skill-evals:` | Linux 版 llm-4.8：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |
| platform-15.9 | [打包与分发](platform-15.9-skill-packaging.md) | `chapter-platform-15.9-skill-packaging:` | Linux 版 llm-4.9：与原章节一致，在 Linux 上构建并运行验证，记录与 Windows 的差异 |

## 阶段总览

草案，待验证：llm-4.6 监视目录在 Windows 上用 `ReadDirectoryChangesW`；如果 llm-4.6 用库或轮询实现，就没有平台差异。llm-4.4、4.5 复用 platform-14 的沙箱，所以本支线依赖 platform-14。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
