# 支线 platform-10：性能对比·Qwen-Image

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-10-perf-qwen-image`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-08-onnxruntime-linux` 的末端，再合入主线 tag `chapter-16.6-qwen-txt2img`（tag 名按章节文件名推断）
- 类别：分析支线（依附：主线阶段 16）
- 完成标准：给出 Qwen-Image 分段加载与逐块卸载在 Linux 与 Windows 上的耗时与显存对比

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-10.1 | [分段加载与逐块卸载的耗时与显存](platform-10.1-staged-offload-perf.md) | `chapter-platform-10.1-staged-offload-perf:` | 对比分段加载与逐块卸载下的耗时与显存 |
| platform-10.2 | [卸载时的传输开销](platform-10.2-transfer-overhead.md) | `chapter-platform-10.2-transfer-overhead:` | 对比卸载时主机与显存之间的传输开销（锁页内存） |

## 阶段总览

分析支线，是叶子。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
