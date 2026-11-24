# 支线 paper-04：Transformer 与文本编码

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-04-transformer-text`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-5.4-clip-verify-node`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-4.1 | [BPE 分词](paper-4.1-bpe.md) | `chapter-paper-4.1-bpe:` | 解析 Sennrich 等人（2016）的《Neural Machine Translation of Rare Words with Subword Units》：怎样从语料里学出合并规则，并用它切分文本；对照 5.1 的 BPE 分词器 |
| paper-4.2 | [Transformer 与自注意力](paper-4.2-attention-is-all-you-need.md) | `chapter-paper-4.2-attention-is-all-you-need:` | 解析 Vaswani 等人（2017）的《Attention Is All You Need》：缩放点积注意力、多头、位置编码、残差与层归一化；对照 5.2 与 5.3 的实现 |
| paper-4.3 | [CLIP：图文对比学习](paper-4.3-clip.md) | `chapter-paper-4.3-clip:` | 解析 Radford 等人（2021）的《Learning Transferable Visual Models From Natural Language Supervision》：用对比学习训练图像编码器和文本编码器，文本编码器的结构；对照 5.3、5.4 的 CLIP 文本编码器 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
