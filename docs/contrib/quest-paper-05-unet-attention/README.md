# 支线 paper-05：UNet 与注意力

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-05-unet-attention`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-7.5-attention-optimization`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-5.1 | [残差网络](paper-5.1-resnet.md) | `chapter-paper-5.1-resnet:` | 解析 He 等人（2016）的《Deep Residual Learning for Image Recognition》：残差连接让深层网络容易训练；对照 6.2 的 ResBlock |
| paper-5.2 | [分组归一化](paper-5.2-group-normalization.md) | `chapter-paper-5.2-group-normalization:` | 解析 Wu 和 He（2018）的《Group Normalization》：按通道分组做归一化，不依赖批大小；对照 4.4 的 GroupNorm 层和 6.2 的 ResBlock |
| paper-5.3 | [U-Net](paper-5.3-unet.md) | `chapter-paper-5.3-unet:` | 解析 Ronneberger 等人（2015）的《U-Net: Convolutional Networks for Biomedical Image Segmentation》：编码器、解码器和跳连；对照 7.1 的 UNet 组装 |
| paper-5.4 | [FlashAttention](paper-5.4-flashattention.md) | `chapter-paper-5.4-flashattention:` | 解析 Dao 等人（2022）的《FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness》：分块计算注意力，减少对显存的读写；对照 7.5 里朴素实现、注意力切片和 `scaled_dot_product_attention` 的对比 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
