# 支线 diffusion-15：视觉定位与布局规划

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-15-vision-planning`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.7-inpaint-outpaint`
- 完成标准：在 Discord 里说“把左边那只猫换成狗”，机器人自己找出位置、做出遮罩并局部重画；说“左边是山，右边是海边的人”，它先规划布局再按区域出图；需求含糊时先问清楚再做

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-15.1 | [视觉定位：让模型给出物体的框](diffusion-15.1-visual-grounding.md) | `chapter-diffusion-15.1-visual-grounding:` | 让视觉模型返回物体的边界框：约定坐标格式（像素还是归一化），对比不同模型的输出差异与准确度，并校验返回值 |
| diffusion-15.2 | [从框到遮罩](diffusion-15.2-box-to-mask.md) | `chapter-diffusion-15.2-box-to-mask:` | 把框变成矩形遮罩，做膨胀和羽化；需要更精确的轮廓时，接 ONNX Runtime 的分割模型（可选） |
| diffusion-15.3 | [用说的改图](diffusion-15.3-edit-by-description.md) | `chapter-diffusion-15.3-edit-by-description:` | 把定位、遮罩和 inpaint 串成一个工具：“把左边那只猫换成狗”，并让用户看到选中的区域以便确认 |
| diffusion-15.4 | [布局规划](diffusion-15.4-layout-planning.md) | `chapter-diffusion-15.4-layout-planning:` | 让 LLM 把一句话拆成几个带说明的框（谁在哪、多大、什么风格），校验框不越界、不重叠得离谱 |
| diffusion-15.5 | [按布局区域出图](diffusion-15.5-regional-generation.md) | `chapter-diffusion-15.5-regional-generation:` | 按布局给每个区域各自的提示词出图，再整体融合；没有区域提示时退化成一句整体提示，并比较两者 |
| diffusion-15.6 | [SVG 草图](diffusion-15.6-svg-sketch.md) | `chapter-diffusion-15.6-svg-sketch:` | 让 LLM 写 SVG 画构图草图，栅格化后作为 img2img 的起点或控制图（栅格化库到时选定） |
| diffusion-15.7 | [澄清提问](diffusion-15.7-clarifying-questions.md) | `chapter-diffusion-15.7-clarifying-questions:` | 判断需求是不是够清楚：该直接做、还是先问；设计提问的格式，用一组评测用例看它问得对不对 |
| diffusion-15.8 | [visual-edit Skill](diffusion-15.8-visual-edit-skill.md) | `chapter-diffusion-15.8-visual-edit-skill:` | 把定位、遮罩、改图和布局整理成 Skill，按需装载 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
