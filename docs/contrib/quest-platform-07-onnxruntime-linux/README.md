# 支线 platform-07：ONNX Runtime（Linux）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-07-onnxruntime-linux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-04-ggml-linux` 的末端，再合入主线 tag `chapter-11.1-ort-setup`
- 类别：功能支线（依附：主线阶段 11）
- 完成标准：在 Linux 上让 ORT 的 CUDA 执行器与 libtorch 共用 CUDA / cuDNN 运行库

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-7.1 | [与 libtorch 共用 CUDA 运行库](platform-7.1-cuda-runtime-share.md) | `chapter-platform-7.1-cuda-runtime-share:` | ORT 的 CUDA 执行器与 libtorch 共用 CUDA / cuDNN 运行库：版本匹配与 `RUNPATH`；据 2026-11-24 的查询，ORT 1.32.0 的 GPU 版目标是 CUDA 13.0，与 libtorch 的 cu130 主版本一致（取自 PyPI 依赖的推断） |

## 阶段总览

依据：11.1 集成 ORT（CUDA EP），与 libtorch 的 CUDA 运行库共存。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
