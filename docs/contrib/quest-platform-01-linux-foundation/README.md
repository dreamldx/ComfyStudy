# 支线 platform-01：Linux 阶段 0

- 类型：支线
- 状态：未开始
- 分支：`quest/platform-01-linux-foundation`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-0.5-dll-management`（主线的 CMake 已按系统拆成平台文件，见 0.1）
- 类别：功能支线（依附：主线阶段 0）
- 完成标准：程序在 Linux 上运行时输出已加载的配置项名称；全部测试在 ASan 与 UBSan 下通过

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-1.1 | [Linux 工具链与环境检查](platform-1.1-toolchain-check.md) | `chapter-platform-1.1-toolchain-check:` | 装好 GCC（固定编译器）、CMake ≥ 3.25、Ninja、pkg-config，写 `check-env.sh` |
| platform-1.2 | [Linux 的 CMake 平台文件](platform-1.2-cmake-linux.md) | `chapter-platform-1.2-cmake-linux:` | 在主线已拆分的基础上新增 `cmake/platform/linux.cmake`：C++20、`-Wall -Wextra -Werror`、`compile_commands.json`、sanitizer 选项；预设含 debug / release / asan |
| platform-1.3 | [测试与 Sanitizer](platform-1.3-tests-sanitizers.md) | `chapter-platform-1.3-tests-sanitizers:` | CTest + GTest，在 ASan 与 UBSan 预设下跑测试 |
| platform-1.4 | [用 apt 管理依赖](platform-1.4-apt-deps.md) | `chapter-platform-1.4-apt-deps:` | 列出各库对应的 apt 包，固定到与 Windows 一致的版本并记录（nlohmann-json 与 cli11 除外）；在 `linux.cmake` 里处理 `find_package` 的查找方式（含 curl） |
| platform-1.5 | [命令行与配置（Linux 分支）](platform-1.5-cli-config-linux.md) | `chapter-platform-1.5-cli-config-linux:` | 在 `config.cpp` 等文件的 `#if` 块里补 Linux 分支：`/proc/self/exe` 定位程序、XDG 目录找配置、环境变量、`isatty` 与 `NO_COLOR` |
| platform-1.6 | [GPU 与 CUDA 环境](platform-1.6-cuda-env.md) | `chapter-platform-1.6-cuda-env:` | 添加 NVIDIA 的 CUDA apt 仓库并安装 CUDA 13.x 与 cuDNN；选定驱动；验证 `nvcc` 与宿主编译器（GCC）的配套；`nvidia-smi` 检查 |
| platform-1.7 | [CI 与本地验证](platform-1.7-ci-verify.md) | `chapter-platform-1.7-ci-verify:` | 在 `ubuntu-latest` 上用 `debian:sid`（或固定日期快照）容器跑 CPU 测试（无 GPU），并写 `verify.sh` |

## 阶段总览

按 Linux 自身的流程设计，不逐章对应 Windows。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
