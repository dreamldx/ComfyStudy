# 阶段 4：权重加载与 VAE

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-04-weights-vae`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：一张图经 VAE 编码再解码能重建，与参考值误差在容差内

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 4.1 | [safetensors 解析](4.1-safetensors-parser.md) | `chapter-4.1-safetensors-parser` | 解析文件头与张量元数据，使用内存映射 |
| 4.2 | [参考数据工具](4.2-reference-data-tool.md) | `chapter-4.2-reference-data-tool` | 用 Python 与 diffusers 离线导出参考张量与对照脚本 |
| 4.3 | [权重名映射](4.3-weight-name-mapping.md) | `chapter-4.3-weight-name-mapping` | 把 safetensors 键名映射到 `torch::nn::Module` 参数 |
| 4.4 | [基础层](4.4-basic-layers.md) | `chapter-4.4-basic-layers` | 实现 Conv2d、GroupNorm、Linear、SiLU，并逐层对照 |
| 4.5 | [VAE 解码器](4.5-vae-decoder.md) | `chapter-4.5-vae-decoder` | 实现并验证 VAE 解码器 |
| 4.6 | [VAE 编码器](4.6-vae-encoder.md) | `chapter-4.6-vae-encoder` | 实现并验证 VAE 编码器 |
| 4.7 | [VAE 节点](4.7-vae-nodes.md) | `chapter-4.7-vae-nodes` | 实现 `VAEEncode` 与 `VAEDecode` 节点 |
| 4.8 | [fp16 数值陷阱](4.8-fp16-pitfalls.md) | `chapter-4.8-fp16-pitfalls` | 讲 VAE 在 fp16 下出现 NaN 或黑图的原因与修法 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
