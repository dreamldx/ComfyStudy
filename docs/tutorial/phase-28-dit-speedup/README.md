# 阶段 28：DiT 加速（选读）

- 类型：选读（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`appendix/28-dit-speedup`（附录不打 tag、不发 Release）
- 起点：主线 tag `chapter-12.8-flux-txt2img`
- 完成标准：同一提示词在基线、特征缓存、Token 合并下对比速度与画质

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| 28.1 | [特征缓存](28.1-feature-caching.md) | `chapter-28.1-feature-caching:` | 相邻步骤里第一个块输出变化小就复用后面的结果（First-Block Cache、TeaCache 一类思路） |
| 28.2 | [Token 合并与稀疏注意力](28.2-token-merge-sparse.md) | `chapter-28.2-token-merge-sparse:` | 用合并或稀疏降低长序列注意力的成本 |
| 28.3 | [SVDQuant（选读·原理阅读）（选读）](28.3-svdquant-reading.md) | `chapter-28.3-svdquant-reading:` | 原理阅读：低秩分支吸收离群值再做 4 位量化，需激活量化故只讲不实现 |
| 28.4 | [速度与画质综合对比](28.4-dit-speed-compare.md) | `chapter-28.4-dit-speed-compare:` | 对比基线、特征缓存、Token 合并 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
