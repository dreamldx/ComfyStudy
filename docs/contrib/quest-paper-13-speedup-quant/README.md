# 支线 paper-13：加速与量化（扩散支线）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-13-speedup-quant`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-15.7-flux-txt2img`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-13.1 | [DeepCache](paper-13.1-deepcache.md) | `chapter-paper-13.1-deepcache:` | 解析 Ma 等人（2023）的《DeepCache: Accelerating Diffusion Models for Free》：相邻时间步的高层特征很相似，可以缓存；对照支线 diffusion-6.1 |
| paper-13.2 | [ToMe for SD](paper-13.2-tome-sd.md) | `chapter-paper-13.2-tome-sd:` | 解析 Bolya 和 Hoffman（2023）的《Token Merging for Fast Stable Diffusion》：在注意力前合并相似的词元；对照支线 diffusion-6.2 |
| paper-13.3 | [潜空间一致性模型（LCM）](paper-13.3-lcm.md) | `chapter-paper-13.3-lcm:` | 解析 Luo 等人（2023）的《Latent Consistency Models: Synthesizing High-Resolution Images with Few-Step Inference》：把潜空间扩散模型蒸馏成几步出图；对照支线 diffusion-6.3 |
| paper-13.4 | [SVDQuant](paper-13.4-svdquant.md) | `chapter-paper-13.4-svdquant:` | 解析 Li 等人（2024）的《SVDQuant: Absorbing Outliers by Low-Rank Components for 4-Bit Diffusion Models》：用低秩分支吸收量化的离群值；对照支线 diffusion-12.3 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
