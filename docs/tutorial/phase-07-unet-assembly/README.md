# 阶段 7：UNet 组装（libtorch）

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-07-unet-assembly`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：完整 UNet 单步前向与参考值对照通过，并记录耗时与显存基线

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 7.1 | [UNet 结构组装](7.1-unet-structure.md) | `chapter-7.1-unet-structure` | 组装下采样、中间、上采样块与跳连 |
| 7.2 | [UNet 权重名映射](7.2-unet-weight-mapping.md) | `chapter-7.2-unet-weight-mapping` | 完成 UNet 的权重名映射并加载 |
| 7.3 | [完整前向与单步对照](7.3-unet-forward-verify.md) | `chapter-7.3-unet-forward-verify` | 运行完整前向并与参考值对照 |
| 7.4 | [性能基线](7.4-unet-baseline.md) | `chapter-7.4-unet-baseline` | 记录耗时与显存作为基线 |
| 7.5 | [注意力优化](7.5-attention-optimization.md) | `chapter-7.5-attention-optimization` | 对比朴素实现、注意力切片与 `scaled_dot_product_attention` 的显存与速度 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
