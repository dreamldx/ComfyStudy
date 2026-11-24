# 支线 paper-10：Qwen-Image 与大语言模型

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-10-qwen-llm`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-16.3-qwen25vl-encoder`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-10.1 | [Qwen-Image 技术报告](paper-10.1-qwen-image-report.md) | `chapter-paper-10.1-qwen-image-report:` | 解析 Qwen 团队（2025）的《Qwen-Image Technical Report》：模型结构、文本编码器的用法和训练方法；对照 16.1 的架构 |
| paper-10.2 | [Qwen2.5-VL 技术报告](paper-10.2-qwen25vl-report.md) | `chapter-paper-10.2-qwen25vl-report:` | 解析 Qwen 团队（2025）的《Qwen2.5-VL Technical Report》：视觉语言模型的结构；对照 16.3 的文本编码器 |
| paper-10.3 | [分组查询注意力](paper-10.3-gqa.md) | `chapter-paper-10.3-gqa:` | 解析 Ainslie 等人（2023）的《GQA: Training Generalized Multi-Query Transformer Models from Multi-Head Checkpoints》：多个查询头共享一组键值头；对照 16.3 |
| paper-10.4 | [SwiGLU](paper-10.4-swiglu.md) | `chapter-paper-10.4-swiglu:` | 解析 Shazeer（2020）的《GLU Variants Improve Transformer》：用门控线性单元改造前馈层；对照 16.3 |
| paper-10.5 | [RMSNorm](paper-10.5-rmsnorm.md) | `chapter-paper-10.5-rmsnorm:` | 解析 Zhang 和 Sennrich（2019）的《Root Mean Square Layer Normalization》：只按均方根缩放、不减均值的归一化；对照 16.3 |
| paper-10.6 | [字节级 BPE](paper-10.6-byte-level-bpe.md) | `chapter-paper-10.6-byte-level-bpe:` | 解析 Radford 等人（2019）的《Language Models are Unsupervised Multitask Learners》（GPT-2）里的字节级 BPE：任何文本都能切成已知的词元；对照 16.3 的分词与聊天模板 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
