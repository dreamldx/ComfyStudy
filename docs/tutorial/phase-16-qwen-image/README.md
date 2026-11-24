# 阶段 16：Qwen-Image

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-16-qwen-image`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：用量化版 Qwen-Image 出图，编码器与主模型分段加载

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 16.1 | [20B MMDiT 架构](16.1-qwen-architecture.md) | `chapter-16.1-qwen-architecture` | 讲 Qwen-Image 的架构 |
| 16.2 | [LLM 当文本编码器](16.2-llm-as-text-encoder.md) | `chapter-16.2-llm-as-text-encoder` | 对比 T5、CLIP 与 Qwen2.5-VL 这类 LLM 编码器，说明取哪一层隐藏状态 |
| 16.3 | [Qwen2.5-VL 文本编码器](16.3-qwen25vl-encoder.md) | `chapter-16.3-qwen25vl-encoder` | 先用 stable-diffusion.cpp 或 llama.cpp 的现成实现得到文本条件；是否自己实现 Qwen2.5-VL 编码器待定 |
| 16.4 | [分段加载与卸载](16.4-staged-load-offload.md) | `chapter-16.4-staged-load-offload` | 编码后卸载编码器，再加载主模型；按计划这一步足以在 24GB 上跑量化版（待实测） |
| 16.5 | [逐块卸载（block offload）](16.5-block-offload.md) | `chapter-16.5-block-offload` | （选做）一块一块地换入换出显存，与分段加载互补；分段加载不够用时才需要 |
| 16.6 | [Qwen-Image 文生图](16.6-qwen-txt2img.md) | `chapter-16.6-qwen-txt2img` | 串起节点完成文生图 |

## 阶段总览

后端（已定）：Qwen-Image 用 ggml，目的是学推理引擎；原理上与 FLUX 同属 MM-DiT 变体，新增的主要是“LLM 当编码器”，原理负担留给阶段 15。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
