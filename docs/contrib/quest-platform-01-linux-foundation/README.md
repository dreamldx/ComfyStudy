# 支线 platform-01：Linux 阶段 0

- 类型：支线
- 状态：进行中（1.1 至 1.4 已验证，1.5 至 1.6 未开始）
- 分支：`quest/platform-01-linux-foundation`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-0.5-dll-management`（主线的 CMake 已按系统拆成平台文件，见 0.1）
- 类别：功能支线（依附：主线阶段 0）
- 完成标准：程序在 Linux 上运行时输出已加载的配置项名称；全部测试在 ASan 与 UBSan 下通过

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| platform-1.1 | [Linux 工具链与环境检查](platform-1.1-toolchain-check.md) | `chapter-platform-1.1-toolchain-check:` | 装好固定主版本的 GCC（本线用 16，只用带版本号的 `g++-16`）、CMake ≥ 3.25、Ninja、pkg-config、git，写 `check-env.sh`（含试编译和 sanitizer 链接）及其测试 |
| platform-1.2 | [用 vcpkg 管理依赖](platform-1.2-vcpkg-deps.md) | `chapter-platform-1.2-vcpkg-deps:` | 在 Linux 上装 vcpkg（检出到清单里的基线提交），让它用固定的 `g++-16` 构建 `vcpkg.json` 里的库（自定义三元组 `x64-linux-gcc` 加链式工具链文件），把 `check-env.sh` 扩展到 vcpkg 和它要用的工具（curl、zip、unzip、tar、make、perl），并把 Linux 和 Windows 上解析出的版本逐个对比 |
| platform-1.3 | [Linux 的 CMake 平台文件与最小移植](platform-1.3-cmake-linux.md) | `chapter-platform-1.3-cmake-linux:` | 在主线已拆分的基础上新增 `cmake/platform/linux.cmake` 和 `linux-debug`、`linux-release`、`linux-asan` 三个预设，接入 platform-1.2 的 vcpkg 和固定的 `g++-16`；补上能编过的最小 Linux 分支（`config::Get` 读环境变量，`config::ExeDirectory` 读 `/proc/self/exe`，测试里的环境变量改成可移植的写法），做一次完整构建，42 个测试通过，程序输出已加载的配置项名称 |
| platform-1.4 | [测试与 Sanitizer](platform-1.4-tests-sanitizers.md) | `chapter-platform-1.4-tests-sanitizers:` | 证明 `linux-asan` 下的测试通过是有意义的：写一个“金丝雀”程序，故意触发 11 种错误（堆越界、释放后使用、栈越界、返回后使用栈变量、全局越界、重复释放、泄漏、整数溢出、移位过大、除以零、空指针），每种错误一个测试，只有 sanitizer 报告了才通过；在 `linux-asan` 测试预设里设 `UBSAN_OPTIONS` 和 `ASAN_OPTIONS`；讲清怎么读 ASan、UBSan 和 LeakSanitizer 的报告 |
| platform-1.5 | [GPU 与 CUDA 环境](platform-1.5-cuda-env.md) | `chapter-platform-1.5-cuda-env:` | 安装 CUDA 13.x 与 cuDNN（vcpkg 里的 `cuda`、`cudnn` 端口版本很旧、只能检测，装不了 13.x，这一步仍用 NVIDIA 官方的安装方式，到时核实）；选定驱动；验证 `nvcc` 与宿主编译器（GCC）的配套；`nvidia-smi` 检查 |
| platform-1.6 | [CI 与本地验证](platform-1.6-ci-verify.md) | `chapter-platform-1.6-ci-verify:` | 在 `ubuntu-latest` 上用 `debian:sid`（或固定日期快照）容器跑 CPU 测试（无 GPU），并写 `verify.sh` |

## 阶段总览

按 Linux 自身的流程设计，不逐章对应 Windows。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
