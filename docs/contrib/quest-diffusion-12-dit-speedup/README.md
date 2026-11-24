# 支线 diffusion-12：DiT 加速

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-12-dit-speedup`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-15.7-flux-txt2img`
- 完成标准：同一提示词在基线、特征缓存、Token 合并下对比速度与画质

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-12.1 | [特征缓存](diffusion-12.1-feature-caching.md) | `chapter-diffusion-12.1-feature-caching:` | 相邻步骤里第一个块输出变化小就复用后面的结果（First-Block Cache、TeaCache 一类思路） |
| diffusion-12.2 | [Token 合并与稀疏注意力](diffusion-12.2-token-merge-sparse.md) | `chapter-diffusion-12.2-token-merge-sparse:` | 用合并或稀疏降低长序列注意力的成本 |
| diffusion-12.3 | [SVDQuant（原理阅读）](diffusion-12.3-svdquant-reading.md) | `chapter-diffusion-12.3-svdquant-reading:` | 原理阅读：低秩分支吸收离群值再做 4 位量化，需激活量化故只讲不实现 |
| diffusion-12.4 | [速度与画质综合对比](diffusion-12.4-dit-speed-compare.md) | `chapter-diffusion-12.4-dit-speed-compare:` | 对比基线、特征缓存、Token 合并 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
