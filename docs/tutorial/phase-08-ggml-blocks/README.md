# 阶段 8：ggml 入门与 UNet 构件

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-08-ggml-blocks`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：ggml 版 ResBlock 与注意力输出与 libtorch 版对照通过

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 8.1 | [编译 ggml（CUDA 后端）](8.1-ggml-build-cuda.md) | `chapter-8.1-ggml-build-cuda` | 编译 ggml 并接入 CMake，实测与 libtorch 的 DLL 共存 |
| 8.2 | [上下文、张量与计算图](8.2-ggml-context-graph.md) | `chapter-8.2-ggml-context-graph` | 理解 ggml 的上下文、张量与计算图 |
| 8.3 | [维度顺序等常见陷阱](8.3-ggml-pitfalls.md) | `chapter-8.3-ggml-pitfalls` | 实测并说明 ggml 与 PyTorch 的维度顺序差异 |
| 8.4 | [ggml 版 ResBlock](8.4-ggml-resblock.md) | `chapter-8.4-ggml-resblock` | 用 ggml 实现 ResBlock |
| 8.5 | [ggml 版注意力](8.5-ggml-attention.md) | `chapter-8.5-ggml-attention` | 用 ggml 实现注意力 |
| 8.6 | [与 libtorch 构件对照](8.6-ggml-blocks-verify.md) | `chapter-8.6-ggml-blocks-verify` | 与阶段 6 的 libtorch 构件输出对照 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
