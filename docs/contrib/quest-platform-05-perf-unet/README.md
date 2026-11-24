# 支线 platform-05：性能对比·UNet（Linux 与 Windows）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-05-perf-unet`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-04-ggml-linux` 的末端，再合入主线 tag `chapter-9.4-ggml-vs-libtorch-perf`
- 类别：分析支线（依附：主线阶段 7、9）
- 完成标准：给出 UNet 单步前向在 Linux 与 Windows 上的耗时与显存对比，并解释差异

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-5.1 | [对比方法与环境记录](platform-5.1-method-env.md) | `chapter-platform-5.1-method-env:` | 定下对比方法并记录环境：同一台机器、同一块 GPU，记录驱动、CUDA 版本、GPU 频率与功耗模式；两边库的上游版本一致；固定种子、输入、步数与分辨率；预热后多次运行取中位数；统一的 JSON 输出格式 |
| platform-5.2 | [UNet 单步前向的耗时与显存](platform-5.2-unet-step-perf.md) | `chapter-platform-5.2-unet-step-perf:` | 对比 libtorch 版 UNet 单步前向的耗时与显存（对应 7.4） |
| platform-5.3 | [ggml 与 libtorch 的差异分析](platform-5.3-ggml-vs-libtorch.md) | `chapter-platform-5.3-ggml-vs-libtorch:` | 对比 ggml 与 libtorch 的耗时与显存（对应 9.4）在两个平台上的差异，并分析原因 |
| platform-5.4 | [推理工程选项与量化](platform-5.4-engineering-quant.md) | `chapter-platform-5.4-engineering-quant:` | 对比 diffusion-13 的推理工程选项（CUDA Graph、TF32、cuDNN 选项、CFG 批合并）与 diffusion-14 的量化（int8、int4）在两个平台上的差异；据我所知 Windows 驱动下内核启动开销更高，CUDA Graph 的收益可能不同（未核实） |

## 阶段总览

分析支线，是叶子：不作为任何支线的起点。第 4 章需要先完成支线 diffusion-13、diffusion-14，并合入它们的分支。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
