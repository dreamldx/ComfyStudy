# 阶段 10：整合出图（SD 1.5）

- 类型：主线
- 状态：未开始
- 阶段 tag：`phase-10-txt2img`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：Discord 里一句话，LLM 自行装载 Skill、组装节点，发回真实文生图，且可切换后端；也能对一张图做 img2img 与 inpaint。SD 1.5 主线到此完成

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 10.1 | [LoadCheckpoint 节点](10.1-load-checkpoint.md) | `chapter-10.1-load-checkpoint` | 实现加载 SD 1.5 检查点的节点 |
| 10.2 | [KSampler 与 CFG](10.2-ksampler-cfg.md) | `chapter-10.2-ksampler-cfg` | 实现含 CFG 的 `KSampler`，带 `backend` 参数 |
| 10.3 | [节点装配与显存管理](10.3-txt2img-assembly.md) | `chapter-10.3-txt2img-assembly` | 装配文生图节点链并管理显存 |
| 10.4 | [Skills 与 Discord 演示](10.4-skills-discord-demo.md) | `chapter-10.4-skills-discord-demo` | 整理各 Skill 并在 Discord 演示文生图 |
| 10.5 | [LoRA（选读）（选读）](10.5-lora.md) | `chapter-10.5-lora` | 实现 LoRA 权重合并 |
| 10.6 | [img2img（SDEdit）](10.6-img2img.md) | `chapter-10.6-img2img` | 把原图编码后加噪到中间时间步再去噪，噪声量决定改动幅度 |
| 10.7 | [Inpaint 与 Outpaint](10.7-inpaint-outpaint.md) | `chapter-10.7-inpaint-outpaint` | 在潜空间用遮罩混合，每步把未遮罩区域替换回原图的加噪版本 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
