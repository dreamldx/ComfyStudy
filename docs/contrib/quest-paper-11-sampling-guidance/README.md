# 支线 paper-11：采样与引导进阶（扩散支线）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-11-sampling-guidance`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.3-txt2img-assembly`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-11.1 | [渐进式蒸馏与 v 预测](paper-11.1-progressive-distillation.md) | `chapter-paper-11.1-progressive-distillation:` | 解析 Salimans 和 Ho（2022）的《Progressive Distillation for Fast Sampling of Diffusion Models》：提出 v 预测，以及逐步减半采样步数的蒸馏；对照支线 diffusion-5.1 |
| paper-11.2 | [零终端 SNR](paper-11.2-zero-terminal-snr.md) | `chapter-paper-11.2-zero-terminal-snr:` | 解析 Lin 等人（2023）的《Common Diffusion Noise Schedules and Sample Steps Are Flawed》：噪声调度末端信噪比不为零的缺陷及修法；对照支线 diffusion-5.2 |
| paper-11.3 | [DPM-Solver](paper-11.3-dpm-solver.md) | `chapter-paper-11.3-dpm-solver:` | 解析 Lu 等人（2022）的《DPM-Solver: A Fast ODE Solver for Diffusion Probabilistic Model Sampling in Around 10 Steps》：利用概率流 ODE 的半线性结构设计高阶求解器；对照支线 diffusion-5.3 |
| paper-11.4 | [分类器引导](paper-11.4-classifier-guidance.md) | `chapter-paper-11.4-classifier-guidance:` | 解析 Dhariwal 和 Nichol（2021）的《Diffusion Models Beat GANs on Image Synthesis》：用分类器的梯度引导采样；对照支线 diffusion-3.6 |
| paper-11.5 | [自注意力引导（SAG）](paper-11.5-sag.md) | `chapter-paper-11.5-sag:` | 解析 Hong 等人（2022）的《Improving Sample Quality of Diffusion Models Using Self-Attention Guidance》：利用自注意力图找出要模糊的区域来引导采样；对照支线 diffusion-5.5 |
| paper-11.6 | [扰动注意力引导（PAG）](paper-11.6-pag.md) | `chapter-paper-11.6-pag:` | 解析 Ahn 等人（2024）的《Self-Rectifying Diffusion Sampling with Perturbed-Attention Guidance》：把自注意力图替换成单位矩阵得到“较差”的预测，再往远离它的方向引导；对照支线 diffusion-5.5 |
| paper-11.7 | [FreeU](paper-11.7-freeu.md) | `chapter-paper-11.7-freeu:` | 解析 Si 等人（2023）的《FreeU: Free Lunch in Diffusion U-Net》：重新加权 UNet 里主干特征和跳连特征；对照支线 diffusion-2.5 |
| paper-11.8 | [MultiDiffusion](paper-11.8-multidiffusion.md) | `chapter-paper-11.8-multidiffusion:` | 解析 Bar-Tal 等人（2023）的《MultiDiffusion: Fusing Diffusion Paths for Controlled Image Generation》：把多个窗口上的去噪结果融合成一张大图；对照支线 diffusion-3.3 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
