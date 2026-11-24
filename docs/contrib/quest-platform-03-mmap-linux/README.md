# 支线 platform-03：内存映射（Linux）

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-03-mmap-linux`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-02-libtorch-linux` 的末端，再合入主线 tag `chapter-4.1-safetensors-parser`
- 类别：功能支线（依附：主线阶段 4）
- 完成标准：safetensors 的内存映射读取在 Linux 上通过测试，大文件与对齐都已验证

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-3.1 | [内存映射的跨平台封装](platform-3.1-mmap-wrapper.md) | `chapter-platform-3.1-mmap-wrapper:` | 用 `mmap` 实现内存映射的跨平台封装，并验证大文件与对齐 |

## 阶段总览

依据：4.1 解析文件头与张量元数据时使用内存映射。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
