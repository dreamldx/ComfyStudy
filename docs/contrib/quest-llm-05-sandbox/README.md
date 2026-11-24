# 支线 llm-05：沙箱与脚本编排

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/llm-05-sandbox`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-1.3-tool-calling`
- 完成标准：模型写的一小段脚本能在沙箱里安全地运行：限制时间、内存和可用的函数；脚本能在预算内循环调用受控的工具，把批量任务压成一轮；一组逃逸用例保证它出不了沙箱

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| llm-5.1 | [沙箱的威胁模型](llm-5.1-threat-model.md) | `chapter-llm-5.1-threat-model:` | 弄清要防什么：死循环、吃光内存、读写文件、联网、偷密钥，以及模型被注入后写出的恶意脚本；决定什么叫“够安全” |
| llm-5.2 | [嵌入脚本语言与白名单函数](llm-5.2-embedded-script.md) | `chapter-llm-5.2-embedded-script:` | 在进程里嵌入一个脚本语言（候选 Lua、QuickJS，到时选定），只暴露白名单函数，先做计算类的小脚本 |
| llm-5.3 | [资源限制：时间、内存与指令数](llm-5.3-resource-limits.md) | `chapter-llm-5.3-resource-limits:` | 给脚本设时间、内存和指令数上限，超限时能可靠地中止并告诉模型原因 |
| llm-5.4 | [子进程沙箱（Windows）](llm-5.4-process-sandbox.md) | `chapter-llm-5.4-process-sandbox:` | 用子进程运行不可信的脚本，用作业对象（Job Object）限内存和进程数，再看受限令牌与 AppContainer 能多拦一层什么；和嵌入脚本语言对比 |
| llm-5.5 | [用脚本编排工具调用](llm-5.5-script-orchestration.md) | `chapter-llm-5.5-script-orchestration:` | 让脚本在沙箱里调用受控的工具接口，例如批量出图、对比参数网格，把几十轮对话压成一轮 |
| llm-5.6 | [调用预算与失败回报](llm-5.6-call-budget.md) | `chapter-llm-5.6-call-budget:` | 限制一段脚本能调用工具的总次数和总耗时，工具失败时怎样把错误整理后回给模型，让它能修正而不是重复 |
| llm-5.7 | [沙箱逃逸测试](llm-5.7-escape-tests.md) | `chapter-llm-5.7-escape-tests:` | 写一组专门试图逃出沙箱的脚本（读文件、联网、死循环、吃内存、改全局），作为回归测试，每次改沙箱都要跑 |
| llm-5.8 | [sandbox Skill](llm-5.8-sandbox-skill.md) | `chapter-llm-5.8-sandbox-skill:` | 把计算和编排整理成 Skill，按需装载，并声明它需要的权限 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
