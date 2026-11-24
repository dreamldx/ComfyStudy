# 阶段 15：FLUX.1 [schnell]

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-15-flux`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：Discord 里用量化版 FLUX.1 [schnell] 出图

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 15.1 | [双流块与单流块](15.1-double-single-stream.md) | `chapter-15.1-double-single-stream` | 实现先分流再合并的两种块，说明为何这样设计 |
| 15.2 | [T5 与 CLIP 双编码器](15.2-t5-clip-encoders.md) | `chapter-15.2-t5-clip-encoders` | 实现 T5 与 CLIP 双编码器 |
| 15.3 | [16 通道 VAE](15.3-flux-vae.md) | `chapter-15.3-flux-vae` | 实现 16 通道 VAE |
| 15.4 | [VAE 通道数演进](15.4-vae-channel-evolution.md) | `chapter-15.4-vae-channel-evolution` | 对比 4 通道、16 通道与更多通道对细节与文字渲染的影响 |
| 15.5 | [加载量化版 Flux](15.5-flux-load.md) | `chapter-15.5-flux-load` | 用 libtorch 加载量化版 Flux 权重 |
| 15.6 | [CFG 与引导蒸馏](15.6-guidance-distillation.md) | `chapter-15.6-guidance-distillation` | 对比普通 CFG（两次前向）、把引导强度做成输入、以及步数蒸馏 |
| 15.7 | [Flux 文生图](15.7-flux-txt2img.md) | `chapter-15.7-flux-txt2img` | 串起节点完成 Flux 文生图 |
| 15.8 | [Flux.2 与 Flux 3 的变化](15.8-flux2-flux3-changes.md) | `chapter-15.8-flux2-flux3-changes` | 原理阅读：Flux.2、Flux 3 相对 Flux.1 的变化，含 Self-Flow 与多模态统一 |

## 阶段总览

后端分配（已定）：FLUX 用 libtorch，目的是学模型原理；Qwen-Image 用 ggml，目的是学推理引擎（见阶段 16）。
后端藏在统一接口后面：节点接口不暴露 `torch::Tensor` / `ggml_tensor`（3.2 的值类型就是边界），采样器面向去噪器接口。
不默认每个新模型写两套后端：每个模型选一个主后端，另一个仅在有明确收益时才做，阶段 13 的 stable-diffusion.cpp 与支线 diffusion-17 承担“另一种后端的对照”。
在 15.5 之前补一张后端选择表（待定）：各模型及其组件用的后端与理由。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
