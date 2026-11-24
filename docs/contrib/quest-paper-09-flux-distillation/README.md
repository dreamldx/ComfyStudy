# 支线 paper-09：FLUX 与蒸馏

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-09-flux-distillation`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-15.6-guidance-distillation`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-9.1 | [T5：文本到文本的 Transformer](paper-9.1-t5.md) | `chapter-paper-9.1-t5:` | 解析 Raffel 等人（2019）的《Exploring the Limits of Transfer Learning with a Unified Text-to-Text Transformer》：T5 的编码器结构、相对位置偏置和分词；对照 15.2 的 T5 编码器 |
| paper-9.2 | [引导蒸馏](paper-9.2-guided-distillation.md) | `chapter-paper-9.2-guided-distillation:` | 解析 Meng 等人（2022）的《On Distillation of Guided Diffusion Models》：把 CFG 的两次前向蒸馏成一次；对照 15.6 里普通 CFG、引导强度做成输入、步数蒸馏三种做法 |
| paper-9.3 | [潜空间对抗蒸馏](paper-9.3-ladd.md) | `chapter-paper-9.3-ladd:` | 解析 Sauer 等人（2024）的《Fast High-Resolution Image Synthesis with Latent Adversarial Diffusion Distillation》：在潜空间里用对抗训练做少步生成；FLUX.1 [schnell] 具体用了哪种蒸馏，以官方说明为准，写到本章时核实；对照 15.6 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
