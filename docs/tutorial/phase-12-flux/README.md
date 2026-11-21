# 阶段 12：FLUX.1 [schnell]

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-12-flux`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：Discord 里用量化版 FLUX.1 [schnell] 出图

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 12.1 | [MM-DiT 原理](12.1-mmdit-principle.md) | `chapter-12.1-mmdit-principle` | 讲 MM-DiT 结构与联合注意力 |
| 12.2 | [双流块与单流块](12.2-double-single-stream.md) | `chapter-12.2-double-single-stream` | 实现先分流再合并的两种块，说明为何这样设计 |
| 12.3 | [T5 与 CLIP 双编码器](12.3-t5-clip-encoders.md) | `chapter-12.3-t5-clip-encoders` | 实现 T5 与 CLIP 双编码器 |
| 12.4 | [16 通道 VAE](12.4-flux-vae.md) | `chapter-12.4-flux-vae` | 实现 16 通道 VAE |
| 12.5 | [VAE 通道数演进](12.5-vae-channel-evolution.md) | `chapter-12.5-vae-channel-evolution` | 对比 4 通道、16 通道与更多通道对细节与文字渲染的影响 |
| 12.6 | [加载量化版 Flux](12.6-flux-load.md) | `chapter-12.6-flux-load` | 加载量化版 Flux 权重 |
| 12.7 | [CFG 与引导蒸馏](12.7-guidance-distillation.md) | `chapter-12.7-guidance-distillation` | 对比普通 CFG（两次前向）、把引导强度做成输入、以及步数蒸馏 |
| 12.8 | [Flux 文生图](12.8-flux-txt2img.md) | `chapter-12.8-flux-txt2img` | 串起节点完成 Flux 文生图 |
| 12.9 | [Flux.2 与 Flux 3 的变化](12.9-flux2-flux3-changes.md) | `chapter-12.9-flux2-flux3-changes` | 原理阅读：Flux.2、Flux 3 相对 Flux.1 的变化，含 Self-Flow 与多模态统一 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
