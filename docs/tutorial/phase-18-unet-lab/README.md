# 阶段 18：UNet 内部观察与实验（选读）

- 类型：选读（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`appendix/18-unet-lab`（附录不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.4-skills-discord-demo`
- 完成标准：在 Discord 里让 LLM 分析某个词影响了画面哪里，并做逐层消融实验、写出报告

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 18.1 | [UNet 钩子机制](18.1-hook-mechanism.md) | `chapter-18.1-hook-mechanism:` | 在块边界与注意力层暴露中间结果与可改写点（仅 libtorch 后端） |
| 18.2 | [特征图可视化](18.2-feature-map-viz.md) | `chapter-18.2-feature-map-viz:` | 用 PCA 把各块在不同时间步的激活降到 RGB 可视化 |
| 18.3 | [交叉注意力图可视化](18.3-cross-attention-maps.md) | `chapter-18.3-cross-attention-maps:` | 取出交叉注意力权重，按词聚合并叠加成热力图回传 |
| 18.4 | [逐层消融实验](18.4-layer-ablation.md) | `chapter-18.4-layer-ablation:` | 实现消融节点，把某个块置零或跳过，观察画面变化 |
| 18.5 | [FreeU](18.5-freeu.md) | `chapter-18.5-freeu:` | 缩放主干与跳连特征，无需训练提升画质 |
| 18.6 | [unet-lab Skill](18.6-unet-lab-skill.md) | `chapter-18.6-unet-lab-skill:` | 让 LLM 自主做实验并汇总成报告 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
