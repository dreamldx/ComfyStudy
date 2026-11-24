# 支线 paper-06：引导、适配与编辑

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-06-guidance-editing`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.7-inpaint-outpaint`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-6.1 | [无分类器引导（CFG）](paper-6.1-classifier-free-guidance.md) | `chapter-paper-6.1-classifier-free-guidance:` | 解析 Ho 和 Salimans（2021）的《Classifier-Free Diffusion Guidance》：同一个模型同时学有条件和无条件，采样时按引导强度外推；对照 10.2 的 KSampler 与 CFG |
| paper-6.2 | [LoRA：低秩适配](paper-6.2-lora.md) | `chapter-paper-6.2-lora:` | 解析 Hu 等人（2021）的《LoRA: Low-Rank Adaptation of Large Language Models》：用两个小矩阵的乘积表示权重的增量；对照 10.5 的权重合并 |
| paper-6.3 | [SDEdit：加噪再去噪的编辑](paper-6.3-sdedit.md) | `chapter-paper-6.3-sdedit:` | 解析 Meng 等人（2021）的《SDEdit: Guided Image Synthesis and Editing with Stochastic Differential Equations》：把原图加噪到中间时间步再去噪，噪声量决定改动幅度；对照 10.6 的 img2img |
| paper-6.4 | [RePaint：用扩散模型补全图像](paper-6.4-repaint.md) | `chapter-paper-6.4-repaint:` | 解析 Lugmayr 等人（2022）的《RePaint: Inpainting using Denoising Diffusion Probabilistic Models》：每一步用已知区域的加噪版本替换未遮罩区域；对照 10.7 的 inpaint |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
