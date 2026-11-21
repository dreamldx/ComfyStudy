# 阶段 16：ONNX Runtime

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-16-onnxruntime`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：生成后让 LLM 自行放大或抠图

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 16.1 | [集成 ORT](16.1-ort-setup.md) | `chapter-16.1-ort-setup` | 集成 ORT（CUDA EP），与 libtorch 的 CUDA 运行库共存 |
| 16.2 | [超分节点](16.2-upscale-node.md) | `chapter-16.2-upscale-node` | 用 Real-ESRGAN 的 ONNX 实现超分节点 |
| 16.3 | [抠图或预处理节点](16.3-matting-controlnet-node.md) | `chapter-16.3-matting-controlnet-node` | 实现抠图或 ControlNet 预处理节点（深度等） |
| 16.4 | [enhance Skill](16.4-enhance-skill.md) | `chapter-16.4-enhance-skill` | 整理 `enhance` Skill |
| 16.5 | [多库张量互转总结](16.5-tensor-interop.md) | `chapter-16.5-tensor-interop` | 总结四库之间的张量互转 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
