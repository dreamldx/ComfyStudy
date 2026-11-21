# 阶段 11：DiT 基础与量化格式

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-11-dit-basics`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：能读取 GGUF 与 fp8 权重并正确反量化，与参考值对照

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 11.1 | [UNet 与 DiT 对比](11.1-unet-vs-dit.md) | `chapter-11.1-unet-vs-dit` | 对比参数与算力分布、分辨率扩展性，说明业界为何转向 DiT |
| 11.2 | [Transformer 块与旋转位置编码](11.2-transformer-rope.md) | `chapter-11.2-transformer-rope` | 实现 DiT 用的 Transformer 块与 RoPE |
| 11.3 | [2D RoPE 与分辨率外推](11.3-rope-2d-extrapolation.md) | `chapter-11.3-rope-2d-extrapolation` | 用旋转位置编码表示二维位置，说明 DiT 为何能自由换分辨率与宽高比 |
| 11.4 | [AdaLN 调制](11.4-adaln-modulation.md) | `chapter-11.4-adaln-modulation` | 实现 AdaLN 调制 |
| 11.5 | [流匹配与采样](11.5-flow-matching.md) | `chapter-11.5-flow-matching` | 直观介绍流匹配，复用阶段 3 的采样框架 |
| 11.6 | [时间步 shift 调度](11.6-timestep-shift.md) | `chapter-11.6-timestep-shift` | 按分辨率调整时间步分布，说明大图为何需要更多高噪声步骤 |
| 11.7 | [GGUF 解析](11.7-gguf-parser.md) | `chapter-11.7-gguf-parser` | 自己解析 GGUF，不依赖 ggml 库 |
| 11.8 | [分块量化的反量化](11.8-block-dequant.md) | `chapter-11.8-block-dequant` | 实现分块量化格式的反量化 |
| 11.9 | [fp8 权重加载](11.9-fp8-loading.md) | `chapter-11.9-fp8-loading` | 加载 fp8 safetensors 并反量化 |
| 11.10 | [视频 DiT 的时空注意力（选读·原理阅读）（选读）](11.10-video-dit-reading.md) | `chapter-11.10-video-dit-reading` | 原理阅读：时空注意力与 3D RoPE，只讲不实现 |
| 11.11 | [表示对齐 REPA（选读·原理阅读）（选读）](11.11-repa-reading.md) | `chapter-11.11-repa-reading` | 原理阅读：表示对齐为何能加速训练，只讲直观 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
