# 支线 platform-08：性能对比·FLUX

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-08-perf-flux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-07-onnxruntime-linux` 的末端，再合入主线 tag `chapter-15.7-flux-txt2img`
- 类别：分析支线（依附：主线阶段 15）
- 完成标准：给出量化版 FLUX 出图在 Linux 与 Windows 上的耗时与显存对比

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-8.1 | [量化版 FLUX 出图的耗时与显存](platform-8.1-flux-perf.md) | `chapter-platform-8.1-flux-perf:` | 对比量化版 FLUX 出图的耗时与显存 |
| platform-8.2 | [显存紧张时的行为](platform-8.2-vram-pressure.md) | `chapter-platform-8.2-vram-pressure:` | 对比显存紧张时两个平台的行为；据我所知 Windows 驱动可能溢出到系统内存，Linux 通常直接 OOM（未核实，在这里实测） |
| platform-8.3 | [DiT 加速的对比](platform-8.3-dit-accel.md) | `chapter-platform-8.3-dit-accel:` | 对比 diffusion-12 的 DiT 加速（特征缓存、Token 合并） |

## 阶段总览

分析支线，是叶子。第 3 章需要先完成支线 diffusion-12，并合入它的分支。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
