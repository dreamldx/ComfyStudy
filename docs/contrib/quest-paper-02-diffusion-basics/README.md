# 支线 paper-02：扩散模型基础

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-02-diffusion-basics`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-3.6-euler-sampler`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-2.1 | [非平衡热力学与扩散模型的起点](paper-2.1-nonequilibrium-thermodynamics.md) | `chapter-paper-2.1-nonequilibrium-thermodynamics:` | 解析 Sohl-Dickstein 等人（2015）的《Deep Unsupervised Learning using Nonequilibrium Thermodynamics》：把“逐步加噪、学习它的逆过程”当作生成模型；对照 3.4 的直观说明 |
| paper-2.2 | [得分匹配与朗之万采样](paper-2.2-score-matching-langevin.md) | `chapter-paper-2.2-score-matching-langevin:` | 解析 Song 和 Ermon（2019）的《Generative Modeling by Estimating Gradients of the Data Distribution》：学习得分函数，用朗之万动力学采样，并使用多个噪声级别；对照 3.4 |
| paper-2.3 | [DDPM：去噪扩散概率模型](paper-2.3-ddpm.md) | `chapter-paper-2.3-ddpm:` | 解析 Ho 等人（2020）的《Denoising Diffusion Probabilistic Models》：前向加噪、反向去噪和预测噪声的训练目标；对照 3.5 的 sigma 调度与 3.6 的采样器 |
| paper-2.4 | [DDIM：确定性采样与跳步](paper-2.4-ddim.md) | `chapter-paper-2.4-ddim:` | 解析 Song、Meng 和 Ermon（2021）的《Denoising Diffusion Implicit Models》：同一个训练好的模型换成确定性的采样过程，可以用更少的步数；对照 3.6 |
| paper-2.5 | [用随机微分方程统一扩散模型](paper-2.5-score-sde.md) | `chapter-paper-2.5-score-sde:` | 解析 Song 等人（2021）的《Score-Based Generative Modeling through Stochastic Differential Equations》：把加噪过程写成 SDE，反向 SDE 与概率流 ODE；对照 3.5、3.6 |
| paper-2.6 | [EDM：把设计选择拆开讨论](paper-2.6-edm.md) | `chapter-paper-2.6-edm:` | 解析 Karras 等人（2022）的《Elucidating the Design Space of Diffusion-Based Generative Models》：sigma 调度（Karras 调度）、网络输入输出的预处理、Euler 与 Heun 采样；对照 3.5 的 karras 序列和 3.6 的 Euler 采样器 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
