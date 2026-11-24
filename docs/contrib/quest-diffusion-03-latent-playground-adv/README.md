# 支线 diffusion-03：潜空间玩法·进阶

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-03-latent-playground-adv`（支线不打 tag、不发 Release）
- 起点：分支 `quest/diffusion-02-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中）
- 完成标准：生成首尾相接的全景图、沿语义方向调节属性，并用梯度引导控制颜色

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-3.1 | [潜空间语义方向](diffusion-3.1-semantic-directions.md) | `chapter-diffusion-3.1-semantic-directions:` | 用两个提示词嵌入的差向量沿方向连续调节属性，如从微笑到大笑 |
| diffusion-3.2 | [TAESD 快速解码](diffusion-3.2-taesd.md) | `chapter-diffusion-3.2-taesd:` | 用小型自编码器代替 VAE 做预览（权重来源需先核实） |
| diffusion-3.3 | [分块解码与分块扩散](diffusion-3.3-tiled-vae-diffusion.md) | `chapter-diffusion-3.3-tiled-vae-diffusion:` | VAE 分块与 MultiDiffusion，在低显存下处理大图 |
| diffusion-3.4 | [全景图](diffusion-3.4-panorama.md) | `chapter-diffusion-3.4-panorama:` | 用 MultiDiffusion 的环形窗口生成首尾相接的全景（依赖本阶段分块扩散一章） |
| diffusion-3.5 | [区域提示](diffusion-3.5-regional-prompting.md) | `chapter-diffusion-3.5-regional-prompting:` | 用遮罩让不同区域看不同提示词，在交叉注意力里实现（依赖 diffusion-2.1 钩子） |
| diffusion-3.6 | [梯度引导](diffusion-3.6-gradient-guidance.md) | `chapter-diffusion-3.6-gradient-guidance:` | 用任意可微损失（颜色、亮度、CLIP 相似度）引导采样，利用 libtorch 的自动求导 |
| diffusion-3.7 | [latent-lab-adv Skill](diffusion-3.7-latent-adv-skill.md) | `chapter-diffusion-3.7-latent-adv-skill:` | 整理进阶潜空间节点为 Skill |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
