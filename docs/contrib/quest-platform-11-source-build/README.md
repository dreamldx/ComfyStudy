# 支线 platform-11：源码构建与官方版本对比（Linux）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-11-source-build`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-08-onnxruntime-linux` 的末端，再合入主线 tag `chapter-13.2-backend-interface`
- 类别：分析支线（依附：阶段 3、8、11、13 用到的库）
- 完成标准：说清哪些库值得在 Linux 上源码构建、收益多大、代价是什么

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-11.1 | [对比方法与功能清单](platform-11.1-method-features.md) | `chapter-platform-11.1-method-features:` | 同一台机器、同一块 GPU、同一 CUDA 版本；记录编译器、构建选项、构建耗时与内存占用、二进制大小；固定种子与输入，预热后取中位数；功能对比要列出启用的模块、算子与特性 |
| platform-11.2 | [libtorch](platform-11.2-libtorch.md) | `chapter-platform-11.2-libtorch:` | 官方压缩包（2.13.0 或 2.14.1，cu130）对比同版本源码构建（只针对目标 GPU 架构、关闭用不到的组件）；对比 UNet 单步耗时与显存、二进制大小、构建耗时，以及功能差异 |
| platform-11.3 | [ggml 与 stable-diffusion.cpp](platform-11.3-ggml-sdcpp.md) | `chapter-platform-11.3-ggml-sdcpp:` | 对比构建选项：官方 CI 的选项与针对目标 GPU 架构、本机 CPU 优化的选项；Linux 上官方 CUDA 产物是否有裸二进制（而不是 Docker 镜像）还没核实 |
| platform-11.4 | [ONNX Runtime](platform-11.4-onnxruntime.md) | `chapter-platform-11.4-onnxruntime:` | 官方 GPU 1.32.0 包对比同版本源码构建（CUDA 执行器，针对目标架构，cuDNN 9）；对比 11.2 超分节点的推理耗时、可用算子与执行器、包大小 |
| platform-11.5 | [汇总报告](platform-11.5-report.md) | `chapter-platform-11.5-report:` | 哪些库值得源码构建、收益多大、代价是什么（构建时间、维护成本、功能裁剪带来的风险） |

## 阶段总览

分析支线，是叶子。libtorch、ONNX Runtime 在 vcpkg 里都有端口（版本以 `builtin-baseline` 为准，到时核实）；CUDA、cuDNN、NCCL 的 vcpkg 端口版本很旧，仍用 NVIDIA 官方的安装方式。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
