# 支线 paper-03：潜空间与 VAE

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-03-latent-vae`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.3-txt2img-assembly`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-3.1 | [VAE：变分自编码器](paper-3.1-vae.md) | `chapter-paper-3.1-vae:` | 解析 Kingma 和 Welling（2013）的《Auto-Encoding Variational Bayes》：证据下界与重参数化技巧；对照 4.5、4.6 的解码器与编码器和潜变量的采样 |
| paper-3.2 | [潜空间扩散模型（Stable Diffusion 的基础）](paper-3.2-ldm.md) | `chapter-paper-3.2-ldm:` | 解析 Rombach 等人（2022）的《High-Resolution Image Synthesis with Latent Diffusion Models》：在 VAE 的潜空间里做扩散，用交叉注意力注入文本条件；对照 4.7 的 VAE 节点和 10.3 的文生图装配 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
