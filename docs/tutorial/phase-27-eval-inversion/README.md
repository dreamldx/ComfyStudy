# 阶段 27：评估与反演（选读）

- 类型：选读（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`appendix/27-eval-inversion`（附录不打 tag、不发 Release）
- 起点：分支 `appendix/24-control-adapters` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中）
- 完成标准：用指标评估出图差异，并把真实图反演回噪声后用提示词编辑

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 27.1 | [基础评估指标](27.1-basic-metrics.md) | `chapter-27.1-basic-metrics:` | 用 OpenCV 实现 PSNR 与 SSIM |
| 27.2 | [LPIPS 与 CLIPScore](27.2-lpips-clipscore.md) | `chapter-27.2-lpips-clipscore:` | 实现感知距离与图文相似度（需要额外权重，权重来源需先核实） |
| 27.3 | [DDIM 反演](27.3-ddim-inversion.md) | `chapter-27.3-ddim-inversion:` | 把真实图反推回噪声 |
| 27.4 | [null-text 反演](27.4-null-text-inversion.md) | `chapter-27.4-null-text-inversion:` | 优化无条件嵌入以提高反演后重建的精度（依赖 27.3） |
| 27.5 | [真实图的提示词编辑](27.5-real-image-p2p.md) | `chapter-27.5-real-image-p2p:` | 配合 Prompt-to-Prompt 编辑真实图片（依赖 24.5、27.3） |
| 27.6 | [eval Skill](27.6-eval-skill.md) | `chapter-27.6-eval-skill:` | 让 LLM 用指标做对比实验 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
