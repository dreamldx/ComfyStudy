# 支线 platform-03：libtorch（Linux）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-03-libtorch-linux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-02-event-loop-network` 的末端，再合入主线 tag `chapter-3.1-libtorch-setup`
- 类别：功能支线（依附：主线阶段 3）
- 完成标准：在 Linux 上用官方预编译包跑通 libtorch 的 hello tensor

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-3.1 | [预编译包选择](platform-3.1-libtorch-package.md) | `chapter-platform-3.1-libtorch-package:` | 选对官方预编译包：C++ ABI 版本、CUDA 版本匹配 |
| platform-3.2 | [链接与运行库路径](platform-3.2-link-runtime-path.md) | `chapter-platform-3.2-link-runtime-path:` | 处理链接与运行库路径，在 GPU 上跑通 hello tensor |

## 阶段总览

依据：3.1 处理 CUDA 版本匹配与 `/MD`，这些是 Windows 专属的做法。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
