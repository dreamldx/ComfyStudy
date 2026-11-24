# 支线 diffusion-11：评估与反演

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-11-eval-inversion`（支线不打 tag、不发 Release）
- 起点：分支 `quest/diffusion-08-control-adapters` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中）
- 完成标准：用指标评估出图差异，并把真实图反演回噪声后用提示词编辑

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-11.1 | [基础评估指标](diffusion-11.1-basic-metrics.md) | `chapter-diffusion-11.1-basic-metrics:` | 用 OpenCV 实现 PSNR 与 SSIM |
| diffusion-11.2 | [LPIPS 与 CLIPScore](diffusion-11.2-lpips-clipscore.md) | `chapter-diffusion-11.2-lpips-clipscore:` | 实现感知距离与图文相似度（需要额外权重，权重来源需先核实） |
| diffusion-11.3 | [DDIM 反演](diffusion-11.3-ddim-inversion.md) | `chapter-diffusion-11.3-ddim-inversion:` | 把真实图反推回噪声 |
| diffusion-11.4 | [null-text 反演](diffusion-11.4-null-text-inversion.md) | `chapter-diffusion-11.4-null-text-inversion:` | 优化无条件嵌入以提高反演后重建的精度（依赖 diffusion-11.3） |
| diffusion-11.5 | [真实图的提示词编辑](diffusion-11.5-real-image-p2p.md) | `chapter-diffusion-11.5-real-image-p2p:` | 配合 Prompt-to-Prompt 编辑真实图片（依赖 diffusion-8.5、diffusion-11.3） |
| diffusion-11.6 | [eval Skill](diffusion-11.6-eval-skill.md) | `chapter-diffusion-11.6-eval-skill:` | 让 LLM 用指标做对比实验 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
