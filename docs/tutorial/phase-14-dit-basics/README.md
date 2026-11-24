# 阶段 14：DiT 基础与量化格式

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-14-dit-basics`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：能读取 GGUF 与 fp8 权重并正确反量化，与参考值对照；讲清 MM-DiT 的联合注意力

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 14.1 | [UNet 与 DiT 对比](14.1-unet-vs-dit.md) | `chapter-14.1-unet-vs-dit` | 对比参数与算力分布、分辨率扩展性，说明业界为何转向 DiT |
| 14.2 | [Transformer 块与旋转位置编码](14.2-transformer-rope.md) | `chapter-14.2-transformer-rope` | 实现 DiT 用的 Transformer 块与 RoPE |
| 14.3 | [2D RoPE 与分辨率外推](14.3-rope-2d-extrapolation.md) | `chapter-14.3-rope-2d-extrapolation` | 用旋转位置编码表示二维位置，说明 DiT 为何能自由换分辨率与宽高比 |
| 14.4 | [AdaLN 调制](14.4-adaln-modulation.md) | `chapter-14.4-adaln-modulation` | 实现 AdaLN 调制 |
| 14.5 | [流匹配与采样](14.5-flow-matching.md) | `chapter-14.5-flow-matching` | 直观介绍流匹配，复用阶段 3 的采样框架 |
| 14.6 | [时间步 shift 调度](14.6-timestep-shift.md) | `chapter-14.6-timestep-shift` | 按分辨率调整时间步分布，说明大图为何需要更多高噪声步骤 |
| 14.7 | [MM-DiT 原理](14.7-mmdit-principle.md) | `chapter-14.7-mmdit-principle` | 讲 MM-DiT 结构与联合注意力（FLUX 与 Qwen-Image 共用） |
| 14.8 | [GGUF 解析](14.8-gguf-parser.md) | `chapter-14.8-gguf-parser` | 自己解析 GGUF，不依赖 ggml 库 |
| 14.9 | [分块量化的反量化](14.9-block-dequant.md) | `chapter-14.9-block-dequant` | 实现分块量化格式的反量化，含 Q4_K / Q5_K / Q6_K 这类带超级块的 K 系列 |
| 14.10 | [fp8 权重加载](14.10-fp8-loading.md) | `chapter-14.10-fp8-loading` | 加载 fp8 safetensors 并反量化 |
| 14.11 | [视频 DiT 的时空注意力（原理阅读）](14.11-video-dit-reading.md) | `chapter-14.11-video-dit-reading` | 原理阅读：时空注意力与 3D RoPE，只讲不实现 |
| 14.12 | [表示对齐 REPA（原理阅读）](14.12-repa-reading.md) | `chapter-14.12-repa-reading` | 原理阅读：表示对齐为何能加速训练，只讲直观 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
