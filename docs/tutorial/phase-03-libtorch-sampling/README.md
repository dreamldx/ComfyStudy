# 阶段 3：libtorch 基础与采样数学

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-03-libtorch-sampling`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：不用任何模型，LLM 驱动采样节点把噪声迭代成可见结果

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 3.1 | [集成 libtorch](3.1-libtorch-setup.md) | `chapter-3.1-libtorch-setup` | 处理 CUDA 版本匹配与 `/MD`，在 GPU 上跑通 hello tensor |
| 3.2 | [Tensor 与 Latent 值类型](3.2-tensor-latent-values.md) | `chapter-3.2-tensor-latent-values` | 新增 `Tensor`/`Latent` 值类型，实现与 `cv::Mat` 互转 |
| 3.3 | [噪声与 Latent 节点](3.3-noise-latent-nodes.md) | `chapter-3.3-noise-latent-nodes` | 实现 `EmptyLatent`、噪声生成、张量数学节点 |
| 3.4 | [扩散的直观：得分与朗之万](3.4-score-langevin-intuition.md) | `chapter-3.4-score-langevin-intuition` | 只讲直观：得分函数、沿它走一步再加噪声，不做推导 |
| 3.5 | [sigma 调度器](3.5-sigma-schedulers.md) | `chapter-3.5-sigma-schedulers` | 实现 normal 与 karras 的 sigma 序列 |
| 3.6 | [Euler 采样器与玩具去噪器](3.6-euler-sampler.md) | `chapter-3.6-euler-sampler` | 实现 Euler 采样器，用玩具去噪器验证数学正确 |
| 3.7 | [sampling Skill](3.7-sampling-skill.md) | `chapter-3.7-sampling-skill` | 把采样节点整理为 `sampling` Skill |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
