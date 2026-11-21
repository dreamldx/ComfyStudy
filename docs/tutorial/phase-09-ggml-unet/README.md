# 阶段 9：ggml UNet 组装

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-09-ggml-unet`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：ggml 版完整 UNet 与 libtorch 版对照通过，并给出耗时与显存对比

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 9.1 | [完整 UNet 的计算图](9.1-ggml-unet-graph.md) | `chapter-9.1-ggml-unet-graph` | 构建完整 UNet 的 ggml 计算图 |
| 9.2 | [权重装入 ggml 张量](9.2-ggml-weight-loading.md) | `chapter-9.2-ggml-weight-loading` | 把权重装入 ggml 张量 |
| 9.3 | [单步前向对照](9.3-ggml-unet-verify.md) | `chapter-9.3-ggml-unet-verify` | 单步前向并与阶段 7 对照 |
| 9.4 | [耗时与显存对比](9.4-ggml-vs-libtorch-perf.md) | `chapter-9.4-ggml-vs-libtorch-perf` | 对比 ggml 与 libtorch 的耗时与显存 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
