# 阶段 15：stable-diffusion.cpp 作为第二个后端

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-15-sdcpp-backend`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：同一提示词在自研后端与 stable-diffusion.cpp 后端出图并对比

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 15.1 | [构建 stable-diffusion.cpp](15.1-sdcpp-build.md) | `chapter-15.1-sdcpp-build` | 构建并处理与自编 ggml 的两份拷贝冲突 |
| 15.2 | [后端接口](15.2-backend-interface.md) | `chapter-15.2-backend-interface` | 增加 `backend=sdcpp` |
| 15.3 | [运行 SD 1.5、Flux、Qwen-Image](15.3-sdcpp-models.md) | `chapter-15.3-sdcpp-models` | 通过它运行三类模型 |
| 15.4 | [后端对比](15.4-backend-comparison.md) | `chapter-15.4-backend-comparison` | 对比出图、速度、显存 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
