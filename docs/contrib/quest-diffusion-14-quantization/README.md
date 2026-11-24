# 支线 diffusion-14：UNet 量化

- 类型：支线
- 状态：未开始
- 分支：`quest/diffusion-14-quantization`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.3-txt2img-assembly`
- 完成标准：`unet_precision` 可选 fp16、int8、int4，出图差异与显存可量化比较

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-14.1 | [精度基础](diffusion-14.1-precision-basics.md) | `chapter-diffusion-14.1-precision-basics:` | 讲 fp32、fp16、bf16，以及哪些层必须保持高精度 |
| diffusion-14.2 | [量化原理](diffusion-14.2-quantization-theory.md) | `chapter-diffusion-14.2-quantization-theory:` | 讲对称与非对称，per-tensor、per-channel、分组 |
| diffusion-14.3 | [加载时 int8 量化](diffusion-14.3-load-time-int8.md) | `chapter-diffusion-14.3-load-time-int8:` | 实现 `QuantLinear`/`QuantConv` 与加载时 int8 量化 |
| diffusion-14.4 | [加载时 int4 分组量化](diffusion-14.4-load-time-int4.md) | `chapter-diffusion-14.4-load-time-int4:` | 实现 int4 分组量化与位打包 |
| diffusion-14.5 | [按层策略与评估](diffusion-14.5-layer-policy-eval.md) | `chapter-diffusion-14.5-layer-policy-eval:` | 按层选择精度，评估误差、出图差异、显存与速度，增加 `unet_precision` 参数 |
| diffusion-14.6 | [量化敏感度扫描](diffusion-14.6-sensitivity-scan.md) | `chapter-diffusion-14.6-sensitivity-scan:` | 逐层量化到 int4 并测影响，用数据得出哪些层不能量化，生成按层策略 |
| diffusion-14.7 | [与 ggml 原生量化对比](diffusion-14.7-ggml-quant-compare.md) | `chapter-diffusion-14.7-ggml-quant-compare:` | 与 ggml 原生量化类型对比 |
| diffusion-14.8 | [缓存量化文件（更进一步）](diffusion-14.8-quant-cache-file.md) | `chapter-diffusion-14.8-quant-cache-file:` | 把量化结果缓存成文件 |
| diffusion-14.9 | [自写融合 CUDA kernel（更进一步）](diffusion-14.9-fused-cuda-kernel.md) | `chapter-diffusion-14.9-fused-cuda-kernel:` | 自写融合反量化与矩阵乘的 CUDA kernel |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
