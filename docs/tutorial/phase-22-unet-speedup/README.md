# 阶段 22：UNet 加速与少步采样（选读）

- 类型：选读（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`appendix/22-unet-speedup`（附录不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.5-lora`
- 完成标准：同一提示词在基线、DeepCache、ToMe、LCM 下对比速度与画质

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 22.1 | [DeepCache](22.1-deepcache.md) | `chapter-22.1-deepcache:` | 缓存相邻时间步变化很小的深层特征，只重算浅层 |
| 22.2 | [Token Merging（ToMe）](22.2-token-merging.md) | `chapter-22.2-token-merging:` | 合并相似 token 以减少注意力计算量 |
| 22.3 | [LCM 与少步采样](22.3-lcm.md) | `chapter-22.3-lcm:` | 实现 LCM 调度器并加载 LCM-LoRA（依赖 10.5 LoRA），约 4 步出图 |
| 22.4 | [速度与画质综合对比](22.4-speed-quality-compare.md) | `chapter-22.4-speed-quality-compare:` | 对比基线、注意力优化、DeepCache、ToMe、LCM |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
