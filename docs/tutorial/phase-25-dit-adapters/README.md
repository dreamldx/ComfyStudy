# 阶段 25：DiT 适配与编辑（选读）

- 类型：选读（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`appendix/25-dit-adapters`（附录不打 tag、不发 Release）
- 起点：主线 tag `chapter-12.8-flux-txt2img`
- 完成标准：用参考图让模型“看着原图改”，并给量化模型叠加 LoRA

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 25.1 | [上下文编辑](25.1-in-context-editing.md) | `chapter-25.1-in-context-editing:` | 把参考图的 token 拼进序列，让模型看着原图修改（Kontext、Qwen-Image-Edit 一类，权重与许可证需先核实） |
| 25.2 | [DiT 的 ControlNet](25.2-dit-controlnet.md) | `chapter-25.2-dit-controlnet:` | 只在少数块注入控制信号，与 UNet 版对比（权重需先核实） |
| 25.3 | [量化模型加 LoRA](25.3-quantized-lora.md) | `chapter-25.3-quantized-lora:` | 权重已是 int4 时不能直接合并，在线叠加低秩分支（依赖 10.5 与阶段 11） |
| 25.4 | [dit-adapters Skill](25.4-dit-adapters-skill.md) | `chapter-25.4-dit-adapters-skill:` | 整理编辑与适配类节点为 Skill |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
