# 支线 paper-14：评估与 SDXL（扩散支线）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-14-evaluation-sdxl`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.7-inpaint-outpaint`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-14.1 | [LPIPS](paper-14.1-lpips.md) | `chapter-paper-14.1-lpips:` | 解析 Zhang 等人（2018）的《The Unreasonable Effectiveness of Deep Features as a Perceptual Metric》：用深度特征的距离衡量感知差异；对照支线 diffusion-11.2 |
| paper-14.2 | [CLIPScore](paper-14.2-clipscore.md) | `chapter-paper-14.2-clipscore:` | 解析 Hessel 等人（2021）的《CLIPScore: A Reference-free Evaluation Metric for Image Captioning》：用 CLIP 的图文相似度评价；对照支线 diffusion-11.2 |
| paper-14.3 | [SDXL](paper-14.3-sdxl.md) | `chapter-paper-14.3-sdxl:` | 解析 Podell 等人（2023）的《SDXL: Improving Latent Diffusion Models for High-Resolution Image Synthesis》：更大的 UNet、两个文本编码器、尺寸与裁剪条件；对照支线 diffusion-16.1 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
