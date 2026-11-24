# 支线 platform-11：GCC 与 Clang 的编译器性能优化

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/platform-11-gcc-clang`（支线不打 tag、不发 Release）
- 起点：分支 `quest/platform-07-onnxruntime-linux` 的末端（不需要新的主线 tag）
- 类别：分析支线（依附：阶段 2、8、11 等 CPU 侧代码）
- 完成标准：整理 GCC 与 Clang 的优化选项，给出 Linux 线推荐的优化选项

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-11.1 | [对比方法与约束](platform-11.1-method.md) | `chapter-platform-11.1-method:` | 同一份源码、同一台机器；记录编译器版本与完整构建选项；Clang 固定使用 libstdc++，避免与官方预编译包产生 C++ ABI 冲突；GPU 内核由 nvcc 编译，只比较 CPU 侧 |
| platform-11.2 | [优化选项矩阵](platform-11.2-option-matrix.md) | `chapter-platform-11.2-option-matrix:` | `-O2` 与 `-O3`、`-march=native`、链接期优化（GCC 的 `-flto` 与 Clang 的 ThinLTO）、按运行数据优化（PGO）、`-ffast-math` 一类会改变数值的选项（需要做结果一致性检查） |
| platform-11.3 | [CPU 侧热点](platform-11.3-cpu-hotspots.md) | `chapter-platform-11.3-cpu-hotspots:` | 主线程序自己的代码（JSON 与命令行解析、句柄与节点系统）、权重加载（safetensors、GGUF）、OpenCV 图像处理、ggml 的 CPU 后端；OpenCV 的源码版本自行按 platform-10.2 的方法构建，不使用 platform-10 的分支 |
| platform-11.4 | [nvcc 的宿主编译器](platform-11.4-nvcc-host.md) | `chapter-platform-11.4-nvcc-host:` | nvcc 分别以 GCC 与 Clang 作宿主编译器（Clang 是否可用还没验证），比较宿主端代码的构建结果与运行表现 |
| platform-11.5 | [汇总](platform-11.5-summary.md) | `chapter-platform-11.5-summary:` | 哪些选项值得开、哪些带来风险（数值差异、可移植性、编译时间），给出 Linux 线推荐的优化选项 |

## 阶段总览

分析支线，是叶子。Linux 一侧固定使用 GCC，这条支线实测 Clang 在 CPU 侧是否有收益。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
