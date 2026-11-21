# 阶段 29：推理工程（选读）

- 类型：选读（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`appendix/29-inference-engineering`（附录不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.2-ksampler-cfg`
- 完成标准：剖析 UNet 的成本，并用批合并、精度开关与 CUDA Graph 优化，保证跨后端结果可复现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 29.1 | [UNet 剖析器](29.1-unet-profiler.md) | `chapter-29.1-unet-profiler:` | 逐块统计参数量、计算量、激活显存，以及不同分辨率下注意力的成本 |
| 29.2 | [CFG 批合并](29.2-cfg-batching.md) | `chapter-29.2-cfg-batching:` | 把有条件和无条件两路合成一个 batch，一次前向 |
| 29.3 | [内存格式与精度开关](29.3-memory-format-precision.md) | `chapter-29.3-memory-format-precision:` | 比较 channels_last、TF32、cuDNN 选项的效果 |
| 29.4 | [CUDA Graph](29.4-cuda-graph.md) | `chapter-29.4-cuda-graph:` | 用 CUDA Graph 降低小算子启动开销（libtorch C++ 中的可行性需先核实） |
| 29.5 | [跨后端可复现](29.5-cross-backend-repro.md) | `chapter-29.5-cross-backend-repro:` | 同一种子在 libtorch 与 ggml 上结果不同的根源，改用统一的 CPU 端噪声生成保证一致 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
