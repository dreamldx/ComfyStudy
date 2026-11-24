# 支线 paper-08：DiT 与流匹配

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-08-dit-flow`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-14.12-repa-reading`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-8.1 | [DiT：用 Transformer 做扩散](paper-8.1-dit.md) | `chapter-paper-8.1-dit:` | 解析 Peebles 和 Xie（2023）的《Scalable Diffusion Models with Transformers》：把 UNet 换成作用在 patch 上的 Transformer，用自适应层归一化注入条件；对照 14.1 与 14.4 |
| paper-8.2 | [RoFormer：旋转位置编码](paper-8.2-roformer-rope.md) | `chapter-paper-8.2-roformer-rope:` | 解析 Su 等人（2021）的《RoFormer: Enhanced Transformer with Rotary Position Embedding》：用旋转表示相对位置；对照 14.2 的 RoPE 与 14.3 的二维扩展 |
| paper-8.3 | [FiLM：按条件缩放和平移](paper-8.3-film.md) | `chapter-paper-8.3-film:` | 解析 Perez 等人（2018）的《FiLM: Visual Reasoning with a General Conditioning Layer》：用条件算出特征的缩放和平移；AdaLN 调制是同一类做法；对照 14.4 |
| paper-8.4 | [流匹配](paper-8.4-flow-matching.md) | `chapter-paper-8.4-flow-matching:` | 解析 Lipman 等人（2022）的《Flow Matching for Generative Modeling》：学习把噪声搬运到数据的速度场；对照 14.5 |
| paper-8.5 | [整流流](paper-8.5-rectified-flow.md) | `chapter-paper-8.5-rectified-flow:` | 解析 Liu 等人（2022）的《Flow Straight and Fast: Learning to Generate and Transfer Data with Rectified Flow》：让噪声到数据的路径尽量接近直线；对照 14.5 |
| paper-8.6 | [SD3：MM-DiT 与大规模整流流](paper-8.6-sd3-mmdit.md) | `chapter-paper-8.6-sd3-mmdit:` | 解析 Esser 等人（2024）的《Scaling Rectified Flow Transformers for High-Resolution Image Synthesis》：MM-DiT 的联合注意力、整流流训练和随分辨率变化的时间步 shift；对照 14.6 与 14.7 |
| paper-8.7 | [FP8 数值格式](paper-8.7-fp8-formats.md) | `chapter-paper-8.7-fp8-formats:` | 解析 Micikevicius 等人（2022）的《FP8 Formats for Deep Learning》：E4M3 与 E5M2 两种格式的取舍；对照 14.10 的 fp8 权重加载 |
| paper-8.8 | [REPA：表示对齐](paper-8.8-repa.md) | `chapter-paper-8.8-repa:` | 解析 Yu 等人（2024）的《Representation Alignment for Generation: Training Diffusion Transformers Is Easier Than You Think》：训练时让扩散模型的中间表示向预训练视觉编码器对齐；对照 14.12 的原理阅读 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
