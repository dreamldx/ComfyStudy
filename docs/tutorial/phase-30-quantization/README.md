# 阶段 30：UNet 量化（选读）

- 类型：选读
- 状态：未开始
- 分支：`appendix/30-quantization`（附录不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.3-txt2img-assembly`
- 完成标准：`unet_precision` 可选 fp16、int8、int4，出图差异与显存可量化比较

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 30.1 | [精度基础](30.1-precision-basics.md) | `chapter-30.1-precision-basics:` | 讲 fp32、fp16、bf16，以及哪些层必须保持高精度 |
| 30.2 | [量化原理](30.2-quantization-theory.md) | `chapter-30.2-quantization-theory:` | 讲对称与非对称，per-tensor、per-channel、分组 |
| 30.3 | [加载时 int8 量化](30.3-load-time-int8.md) | `chapter-30.3-load-time-int8:` | 实现 `QuantLinear`/`QuantConv` 与加载时 int8 量化 |
| 30.4 | [加载时 int4 分组量化](30.4-load-time-int4.md) | `chapter-30.4-load-time-int4:` | 实现 int4 分组量化与位打包 |
| 30.5 | [按层策略与评估](30.5-layer-policy-eval.md) | `chapter-30.5-layer-policy-eval:` | 按层选择精度，评估误差、出图差异、显存与速度，增加 `unet_precision` 参数 |
| 30.6 | [量化敏感度扫描](30.6-sensitivity-scan.md) | `chapter-30.6-sensitivity-scan:` | 逐层量化到 int4 并测影响，用数据得出哪些层不能量化，生成按层策略 |
| 30.7 | [与 ggml 原生量化对比](30.7-ggml-quant-compare.md) | `chapter-30.7-ggml-quant-compare:` | 与 ggml 原生量化类型对比 |
| 30.8 | [缓存量化文件（选读中的选读）（选读）](30.8-quant-cache-file.md) | `chapter-30.8-quant-cache-file:` | 把量化结果缓存成文件 |
| 30.9 | [自写融合 CUDA kernel（选读中的选读）（选读）](30.9-fused-cuda-kernel.md) | `chapter-30.9-fused-cuda-kernel:` | 自写融合反量化与矩阵乘的 CUDA kernel |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
