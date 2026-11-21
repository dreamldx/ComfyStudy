# 阶段 24：条件控制与适配器（选读）

- 类型：选读（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`appendix/24-control-adapters`（附录不打 tag、不发 Release）
- 起点：分支 `appendix/18-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中）
- 完成标准：用边缘图控制构图，并用参考图或编辑注意力图改变画面

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 24.1 | [ControlNet 原理](24.1-controlnet-principle.md) | `chapter-24.1-controlnet-principle:` | 讲复制 UNet 编码器与零初始化卷积 |
| 24.2 | [ControlNet 实现与加载](24.2-controlnet-impl.md) | `chapter-24.2-controlnet-impl:` | 实现 ControlNet 并加载权重，接入 UNet 的残差注入点 |
| 24.3 | [ControlNet 与 Canny 节点](24.3-controlnet-canny.md) | `chapter-24.3-controlnet-canny:` | 用 OpenCV 的 Canny 做预处理，端到端出图 |
| 24.4 | [IP-Adapter](24.4-ip-adapter.md) | `chapter-24.4-ip-adapter:` | 解耦交叉注意力，用图像做提示（需要额外的图像编码器） |
| 24.5 | [Prompt-to-Prompt](24.5-prompt-to-prompt.md) | `chapter-24.5-prompt-to-prompt:` | 替换或编辑注意力图来修改画面（依赖 18.3） |
| 24.6 | [control Skill](24.6-control-skill.md) | `chapter-24.6-control-skill:` | 整理控制类节点为 Skill |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
