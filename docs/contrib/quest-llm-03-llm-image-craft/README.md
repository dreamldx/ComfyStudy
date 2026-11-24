# 支线 llm-03：用 LLM 改进出图

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/llm-03-llm-image-craft`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.4-skills-discord-demo`
- 完成标准：用户只说一句模糊的话，机器人自动扩写提示词、出图、让视觉模型评审并重画，最后发回最好的一张并解释为什么选它

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| llm-3.1 | [提示词扩写与风格库](llm-3.1-prompt-expansion.md) | `chapter-llm-3.1-prompt-expansion:` | 把一句话扩写成适合出图的提示词，维护一份可复用的风格库，并讨论过度扩写的问题 |
| llm-3.2 | [反推提示词（图转文）](llm-3.2-reverse-prompt.md) | `chapter-llm-3.2-reverse-prompt:` | 让视觉模型描述一张图，再用描述去重现它，比较重现的距离 |
| llm-3.3 | [视觉评审回路](llm-3.3-critic-loop.md) | `chapter-llm-3.3-critic-loop:` | 让视觉模型按清单给结果图打分并指出问题，据此改提示词重画，并限制轮数与成本 |
| llm-3.4 | [LLM 当裁判：成对比较](llm-3.4-llm-as-judge.md) | `chapter-llm-3.4-llm-as-judge:` | 让视觉模型两两比较出图结果，交换顺序检测位置偏差，并与人工判断对照 |
| llm-3.5 | [批量出图与自动挑图](llm-3.5-batch-pick.md) | `chapter-llm-3.5-batch-pick:` | 一次出多张，用评审与裁判挑出最好的，说明“多采样再挑选”的成本与收益 |
| llm-3.6 | [多轮改图对话](llm-3.6-multi-turn-edit.md) | `chapter-llm-3.6-multi-turn-edit:` | 理解“再亮一点”“把背景换成海边”这类指代，决定是重新出图、img2img 还是 inpaint |
| llm-3.7 | [image-craft Skill](llm-3.7-image-craft-skill.md) | `chapter-llm-3.7-image-craft-skill:` | 把本阶段的能力整理成 Skill，按需装载 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
