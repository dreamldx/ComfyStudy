# 支线 platform-07：性能对比·SD 1.5 出图

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-07-perf-sd15`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-05-ggml-linux` 的末端，再合入主线 tag `chapter-10.7-inpaint-outpaint`
- 类别：分析支线（依附：主线阶段 10）
- 完成标准：给出 SD 1.5 端到端出图在 Linux 与 Windows 上的耗时拆分与结果一致性

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-7.1 | [端到端耗时拆分](platform-7.1-e2e-breakdown.md) | `chapter-platform-7.1-e2e-breakdown:` | 端到端文生图与图生图的耗时拆分：加载、编码、采样、解码 |
| platform-7.2 | [模型加载耗时](platform-7.2-model-load.md) | `chapter-platform-7.2-model-load:` | 对比模型加载耗时：文件系统、内存映射、冷热缓存 |
| platform-7.3 | [出图结果的一致性](platform-7.3-consistency.md) | `chapter-platform-7.3-consistency:` | 用 PSNR 之类的指标比较出图结果，而不是逐位比较（编译器与库差异会带来微小的数值差） |
| platform-7.4 | [UNet 加速的对比](platform-7.4-unet-accel.md) | `chapter-platform-7.4-unet-accel:` | 对比 diffusion-6 的 UNet 加速（DeepCache、ToMe、LCM）的速度与画质 |

## 阶段总览

分析支线，是叶子。第 4 章需要先完成支线 diffusion-6，并合入它的分支。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
