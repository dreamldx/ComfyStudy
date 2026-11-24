# 支线 diffusion-07：DiT 内部观察与实验

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-07-dit-lab`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-15.7-flux-txt2img`
- 完成标准：在 Discord 里让 LLM 分析某个词在联合注意力里看了画面哪里，并做逐块消融实验、写出报告

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-7.1 | [DiT 钩子机制](diffusion-7.1-dit-hooks.md) | `chapter-diffusion-7.1-dit-hooks:` | 在块边界与联合注意力层暴露中间结果与可改写点（仅 libtorch 后端，以 Flux 为目标） |
| diffusion-7.2 | [联合注意力图可视化](diffusion-7.2-joint-attention-maps.md) | `chapter-diffusion-7.2-joint-attention-maps:` | 取出文本与图像在联合注意力里的权重，叠加成热力图回传 |
| diffusion-7.3 | [逐块消融与层跳过](diffusion-7.3-block-ablation.md) | `chapter-diffusion-7.3-block-ablation:` | 关掉某些块观察画面变化，并评估层跳过带来的提速 |
| diffusion-7.4 | [dit-lab Skill](diffusion-7.4-dit-lab-skill.md) | `chapter-diffusion-7.4-dit-lab-skill:` | 让 LLM 自主做实验并汇总成报告 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
