# 阶段 2：OpenCV：句柄系统与图像处理

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-02-opencv-handles`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：在 Discord 里说“把这张图缩小一半并加模糊”，机器人自己装载 Skill、调用节点并发回结果图；LLM 还能看到结果图并自行调整重做

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 2.1 | [值与句柄](2.1-values-handles.md) | `chapter-2.1-values-handles` | 实现 `Value`、`ValueStore` 与句柄 `n<id>.<port>`，管理引用与释放 |
| 2.2 | [节点基础设施](2.2-node-core.md) | `chapter-2.2-node-core` | 实现 `NodeSpec`、`NodeRegistry`、Schema 自动生成与类型校验 |
| 2.3 | [OpenCV 基础节点](2.3-opencv-basic-nodes.md) | `chapter-2.3-opencv-basic-nodes` | 集成 OpenCV，实现 `LoadImage`、`SaveImage`、`Resize` |
| 2.4 | [图像处理节点](2.4-image-processing-nodes.md) | `chapter-2.4-image-processing-nodes` | 实现模糊、调色、裁剪、混合、遮罩与合成 |
| 2.5 | [图像 Skill 与图片附件](2.5-skills.md) | `chapter-2.5-skills` | 把图像节点整理成 Skill（沿用 1.4 的 Skill 机制），并让工具结果能回传图片附件 |
| 2.6 | [调用记录与工作流导出](2.6-workflow-recording.md) | `chapter-2.6-workflow-recording` | 记录调用并导出可重放工作流，实现元工具并打磨错误信息 |
| 2.7 | [视觉反馈闭环](2.7-vision-feedback-loop.md) | `chapter-2.7-vision-feedback-loop` | 出图后把结果图发回给 LLM 评价，让它调整参数再做一次 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
