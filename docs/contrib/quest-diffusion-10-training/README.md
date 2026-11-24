# 支线 diffusion-10：训练

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-10-training`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-15.7-flux-txt2img`
- 完成标准：用少量图训练一个 LoRA 或一个新词嵌入，并在 Discord 生成对比

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-10.1 | [自动求导与训练基础](diffusion-10.1-autograd-basics.md) | `chapter-diffusion-10.1-autograd-basics:` | libtorch 自动求导、优化器、显存与梯度检查点 |
| diffusion-10.2 | [LoRA 训练](diffusion-10.2-lora-training.md) | `chapter-diffusion-10.2-lora-training:` | 在交叉注意力层注入低秩适配并在小数据集上训练 |
| diffusion-10.3 | [Textual Inversion](diffusion-10.3-textual-inversion.md) | `chapter-diffusion-10.3-textual-inversion:` | 只训练一个新词的嵌入 |
| diffusion-10.4 | [DiT 的 LoRA 训练](diffusion-10.4-dit-lora-training.md) | `chapter-diffusion-10.4-dit-lora-training:` | 在 Flux 上训练 LoRA（依赖 diffusion-10.2 与阶段 15） |
| diffusion-10.5 | [训练与评估 Skill](diffusion-10.5-train-eval-skill.md) | `chapter-diffusion-10.5-train-eval-skill:` | 让 LLM 发起训练并评估结果（长任务走串行队列） |
| diffusion-10.6 | [模型融合与剪枝介绍（原理阅读）](diffusion-10.6-model-merging-intro.md) | `chapter-diffusion-10.6-model-merging-intro:` | 只做介绍不实现：检查点合并、任务向量、LoRA 提取、块剪枝 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
