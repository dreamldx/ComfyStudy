# 支线 paper-12：条件控制与编辑（扩散支线）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/paper-12-control-editing`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.3-txt2img-assembly`
- 完成标准：逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| paper-12.1 | [ControlNet](paper-12.1-controlnet.md) | `chapter-paper-12.1-controlnet:` | 解析 Zhang 等人（2023）的《Adding Conditional Control to Text-to-Image Diffusion Models》：复制一份编码器、用零卷积接回；对照支线 diffusion-8.2 |
| paper-12.2 | [IP-Adapter](paper-12.2-ip-adapter.md) | `chapter-paper-12.2-ip-adapter:` | 解析 Ye 等人（2023）的《IP-Adapter: Text Compatible Image Prompt Adapter for Text-to-Image Diffusion Models》：给交叉注意力加一路图像条件；对照支线 diffusion-8.4 |
| paper-12.3 | [Prompt-to-Prompt](paper-12.3-prompt-to-prompt.md) | `chapter-paper-12.3-prompt-to-prompt:` | 解析 Hertz 等人（2022）的《Prompt-to-Prompt Image Editing with Cross Attention Control》：通过控制交叉注意力图编辑图像；对照支线 diffusion-8.5 |
| paper-12.4 | [Textual Inversion](paper-12.4-textual-inversion.md) | `chapter-paper-12.4-textual-inversion:` | 解析 Gal 等人（2022）的《An Image is Worth One Word: Personalizing Text-to-Image Generation using Textual Inversion》：只学一个新词的嵌入；对照支线 diffusion-10.3 |
| paper-12.5 | [Null-text 反演](paper-12.5-null-text-inversion.md) | `chapter-paper-12.5-null-text-inversion:` | 解析 Mokady 等人（2022）的《Null-text Inversion for Editing Real Images using Guided Diffusion Models》：优化无条件嵌入来补偿 DDIM 反演的误差；对照支线 diffusion-11.4 |

## 阶段总览

论文解析支线：每章解析一篇论文的核心思想与关键公式，并结合本项目的实现讲解，不改代码。出处、年份与标题写在每章的目标里，写章节时再按原文核实。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的论文清单与版本。详细内容记在各章“技术资料”一节。）
