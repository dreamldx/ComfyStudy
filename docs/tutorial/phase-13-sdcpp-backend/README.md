# 阶段 13：stable-diffusion.cpp 后端

- 类型：主线（草案，到该阶段前再细化）
- 状态：未开始
- 阶段 tag：`phase-13-sdcpp-backend`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：构建出固定提交的 stable-diffusion.cpp，用 `backend=sdcpp` 跑通 SD 1.5 出图

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 13.1 | [构建 stable-diffusion.cpp](13.1-sdcpp-build.md) | `chapter-13.1-sdcpp-build` | 构建 stable-diffusion.cpp，固定到与 8.1 的 ggml 配套的提交，避免进程里出现两份 ggml |
| 13.2 | [后端接口](13.2-backend-interface.md) | `chapter-13.2-backend-interface` | 增加 `backend=sdcpp` |

## 阶段总览

前置（据章节目标整理，到该阶段前再核对）：13.1 要处理与自编 ggml 的拷贝冲突（阶段 8）；13.2 在阶段 10 的 `backend` 参数上增加 `sdcpp`。本阶段排在 DiT 阶段之前，只跑 SD 1.5；Flux 与 Qwen-Image 的对照放在支线 diffusion-17。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
