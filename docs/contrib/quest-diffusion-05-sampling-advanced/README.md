# 支线 diffusion-05：采样与调度进阶

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-05-sampling-advanced`（支线不打 tag、不发 Release）
- 起点：分支 `quest/diffusion-02-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.3-txt2img-assembly`，已包含在该分支的历史中）
- 完成标准：让 LLM 自己跑采样器对比实验，并逐步预览去噪轨迹

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-5.1 | [预测目标：ε、v、x0](diffusion-5.1-prediction-targets.md) | `chapter-diffusion-5.1-prediction-targets:` | 说明网络到底预测什么、三者如何互相换算、SD 2 为何改用 v |
| diffusion-5.2 | [零终端 SNR 与 trailing 间隔](diffusion-5.2-zero-terminal-snr.md) | `chapter-diffusion-5.2-zero-terminal-snr:` | 说明 SD 1.5 噪声调度末端信噪比不为零的缺陷及修法 |
| diffusion-5.3 | [进阶采样器](diffusion-5.3-advanced-samplers.md) | `chapter-diffusion-5.3-advanced-samplers:` | 实现 Heun、DPM++ 2M、UniPC，以及祖先采样与确定性采样的区别 |
| diffusion-5.4 | [CFG 的机理与变体](diffusion-5.4-cfg-mechanics.md) | `chapter-diffusion-5.4-cfg-mechanics:` | 讲几何解释、负向提示词、CFG rescale、按步数动态调整强度 |
| diffusion-5.5 | [PAG 与 SAG 注意力引导](diffusion-5.5-pag-sag.md) | `chapter-diffusion-5.5-pag-sag:` | 通过扰动注意力来提升画质，需要钩子配合（依赖 diffusion-2.1） |
| diffusion-5.6 | [去噪轨迹可视化](diffusion-5.6-denoise-trajectory.md) | `chapter-diffusion-5.6-denoise-trajectory:` | 每一步预览模型当前预测的 x0，直观看到从噪声长出图像 |
| diffusion-5.7 | [采样器对比实验](diffusion-5.7-sampler-compare.md) | `chapter-diffusion-5.7-sampler-compare:` | 对比步数与画质的曲线，让 LLM 自行做实验 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
