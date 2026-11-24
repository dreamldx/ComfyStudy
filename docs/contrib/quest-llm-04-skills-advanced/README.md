# 支线 llm-04：Skills 进阶

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/llm-04-skills-advanced`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-2.6-workflow-recording`
- 完成标准：Skill 用统一的文件格式描述，模型只在需要时才读到它的正文和资源；有一套评测集能量化“该用哪个 Skill”选得准不准；录好的操作可以自动生成 Skill，并带着权限声明打包、热重载

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| llm-4.1 | [Skill 文件格式与元数据](llm-4.1-skill-format.md) | `chapter-llm-4.1-skill-format:` | 设计 SKILL.md：头部写名字、说明、所需工具，正文写用法；解析并校验，对照 Anthropic 的 Agent Skills 约定 |
| llm-4.2 | [渐进式披露](llm-4.2-progressive-disclosure.md) | `chapter-llm-4.2-progressive-disclosure:` | 先只给模型每个 Skill 的名字和说明，需要时再读正文，再按需读附带的资源；量化它省下的 token |
| llm-4.3 | [说明怎么写：让模型选对 Skill](llm-4.3-skill-descriptions.md) | `chapter-llm-4.3-skill-descriptions:` | 说明写得含糊或重叠时模型会选错；做一组“用户的话 → 该用哪个 Skill”的用例，看写法对选择准确率的影响 |
| llm-4.4 | [Skill 自带的资源与脚本](llm-4.4-skill-resources.md) | `chapter-llm-4.4-skill-resources:` | 让 Skill 附带参考文档、模板和脚本，提供按需读取的工具；脚本放进受限环境里运行 |
| llm-4.5 | [权限与沙箱](llm-4.5-skill-permissions.md) | `chapter-llm-4.5-skill-permissions:` | Skill 声明它要用哪些工具，装载时只开放声明过的；危险工具要用户确认 |
| llm-4.6 | [热重载与版本](llm-4.6-skill-hot-reload.md) | `chapter-llm-4.6-skill-hot-reload:` | 监视 Skill 目录的变化并重新装载，会话进行中的 Skill 如何平滑切换；Skill 之间的依赖与冲突检测 |
| llm-4.7 | [从录制生成 Skill](llm-4.7-skill-from-recording.md) | `chapter-llm-4.7-skill-from-recording:` | 把 2.6 录下的一次成功的操作，让 LLM 整理成可复用的 Skill，并由人审阅 |
| llm-4.8 | [Skill 评测与回归](llm-4.8-skill-evals.md) | `chapter-llm-4.8-skill-evals:` | 为每个 Skill 写触发用例和结果检查，改了说明或工具后跑一遍，防止悄悄变差 |
| llm-4.9 | [打包与分发](llm-4.9-skill-packaging.md) | `chapter-llm-4.9-skill-packaging:` | 把 Skill 打成一个带清单和哈希的包，装载前校验，和 0.5 的 DLL 清单是同一个思路 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
