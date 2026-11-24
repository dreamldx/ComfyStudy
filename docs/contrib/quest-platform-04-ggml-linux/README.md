# 支线 platform-04：ggml（Linux）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-04-ggml-linux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-03-mmap-linux` 的末端，再合入主线 tag `chapter-8.1-ggml-build-cuda`
- 类别：功能支线（依附：主线阶段 8）
- 完成标准：在 Linux 上编译出带 CUDA 后端的 ggml，并与 libtorch 共存

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-4.1 | [编译 ggml 的 CUDA 后端](platform-4.1-ggml-cuda-build.md) | `chapter-platform-4.1-ggml-cuda-build:` | 编译 ggml 的 CUDA 后端：nvcc 与宿主编译器的配套，固定到 stable-diffusion.cpp 的 ggml 分叉（见 8.1） |
| platform-4.2 | [与 libtorch 共存](platform-4.2-shared-lib-coexist.md) | `chapter-platform-4.2-shared-lib-coexist:` | 处理与 libtorch 共存时的共享库与符号冲突 |

## 阶段总览

依据：8.1 编译 ggml 并实测与 libtorch 的 DLL 共存。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
