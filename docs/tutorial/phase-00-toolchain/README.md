# 阶段 0：工具链与骨架

- 类型：主线
- 状态：已完成（0.0 至 0.6）
- 阶段 tag：`phase-00-toolchain`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：运行程序时输出已加载的配置项名称，能列出程序加载的每个 DLL 的位置、版本和所属的 vcpkg 包，并能检测 GPU 是否可用

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 0.0 | [初始化仓库与教程骨架](0.0-repo-init.md) | `chapter-0.0-repo-init` | `git init` 并添加远程仓库，建立教程骨架，写“本教程如何阅读”并打第一个 tag |
| 0.1 | [MSVC 环境与第一个 CMakeLists](0.1-msvc-cmake.md) | `chapter-0.1-msvc-cmake` | 进入 MSVC 编译环境，创建第一批源码，写第一个 CMakeLists.txt，并用详细的构建输出看懂 cl 与 link 的编译过程（含 Debug 与 Release），弄清 Ninja 如何追踪头文件依赖，并学会生成 Visual Studio 解决方案、在 Visual Studio 里打开项目 |
| 0.2 | [测试（CTest）](0.2-tests-ctest.md) | `chapter-0.2-tests-ctest` | 加入测试框架并用 CTest 运行 |
| 0.3 | [vcpkg 依赖管理](0.3-vcpkg-deps.md) | `chapter-0.3-vcpkg-deps` | 用 vcpkg manifest 引入 nlohmann/json、libcurl、GoogleTest、spdlog 和 CLI11，并接入 CMake，把测试迁移到 GoogleTest |
| 0.4 | [命令行与配置文件](0.4-cli-config.md) | `chapter-0.4-cli-config` | 用 CLI11 读取命令行、配置文件（INI 或 TOML）和环境变量，优先级为环境变量 < 配置文件 < 命令行，打印配置，用 spdlog 记录加载过程，密钥遮蔽 |
| 0.5 | [DLL 集中管理与依赖检查](0.5-dll-management.md) | `chapter-0.5-dll-management` | 让所有 DLL 与可执行文件放在同一目录，提供 `deploy` 与 `check-dlls` 目标，启动时设置 DLL 搜索目录，用 `--dependency-check` 列出已加载 DLL 的位置、版本和所属 vcpkg 包 |
| 0.6 | [检测 GPU 与 CUDA 驱动](0.6-gpu-check.md) | `chapter-0.6-gpu-check` | 在项目里默认检测 GPU 能不能用：写一个只依赖 NVIDIA 驱动的探针 `gpu_probe`（不要 `nvcc`，不要 `cuda.h`，Windows 与 Linux 同一份源码），它作为 CTest 测试随项目运行，并给 `check-env.ps1` 加 `-Gpu` |

## 阶段总览

这一阶段搭好后面所有阶段共用的基础：一个能在 Windows 上用 MSVC、CMake 和 Ninja 构建的 C++ 项目，带测试、第三方库和配置，并且管得住 DLL。后面调用 LLM、处理图像、实现扩散模型，都在这个项目上接着写。

学完这一阶段，你能：

- 在 MSVC 环境里用 CMake 预设构建 Debug 和 Release，读懂 `cl` 和 `link` 的构建输出，知道 Ninja 为什么只重编改动的文件，也能生成 Visual Studio 解决方案。
- 用 CTest 和 GoogleTest 写测试并运行。
- 用 vcpkg 的 manifest 模式管理依赖（nlohmann/json、libcurl、GoogleTest、spdlog、CLI11）。
- 用 CLI11 同时读取命令行、配置文件（INI 或 TOML）和环境变量，并把密钥遮蔽起来。
- 知道 Windows 怎么找 DLL，让程序的 DLL 集中在一个目录，并能检查、列出它们。
- 确认程序能用上 GPU：只依赖驱动的探针 `gpu_probe`，作为测试随项目运行，也能用 `check-env.ps1 -Gpu` 单独检查。

核心概念：预设与构建目录、静态库与可执行文件、测试夹具、manifest 与基线、配置的优先级（环境变量 < 配置文件 < 命令行）、DLL 搜索顺序与清单。

后面会用到的：阶段 1 起用 libcurl 发请求，用配置文件放 Token 和密钥，用测试防止改坏已有行为。这些本阶段都已备好。

## 演示

```text
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
build\windows-debug\bin\comfystudy.exe --help
build\windows-debug\bin\comfystudy.exe
cmake --build --preset windows-debug --target check-dlls
build\windows-debug\bin\comfystudy.exe --dependency-check
```

这几条命令依次是：构建、跑全部测试、看自动生成的帮助、运行程序并显示配置（密钥已打码，配置可来自配置文件、环境变量和命令行）、检查 DLL 部署、列出程序加载的每个 DLL。

## 技术资料汇总

- 工具链：MSVC（x64）、CMake 4.x、Ninja、vcpkg（manifest 模式，固定基线）。
- 依赖：nlohmann/json、libcurl、GoogleTest、spdlog、CLI11。
- 关键设计决定：
  - 除了 `main()`，所有代码都编进静态库 `comfystudy_core`，程序和每个测试都链接它。
  - 配置文件不分节，键、命令行选项和环境变量用同一个名字；空值等于没有设置。
  - 所有 DLL 放在 `bin` 目录，`dlls.manifest` 记录名字、SHA-256 和所属的 vcpkg 包；`check-dlls` 检查依赖，`--dependency-check` 只展示、不判断。
  - 密钥在输出和日志里一律遮蔽。
- 详细内容见各章“技术资料”一节。
