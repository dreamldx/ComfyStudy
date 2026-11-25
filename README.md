# ComfyStudy · 支线 platform-01：Linux 阶段 0

这是支线分支 `quest/platform-01-linux-foundation`。它不合并回 `main`，也不打 tag、不发 Release。
起点是主线 tag `chapter-0.5-dll-management`；每一章对应一个或几个提交，提交信息以 `chapter-platform-1.N-` 开头，
用 `git log --grep="^chapter-platform-1.1"` 可以找到本章的提交。
本分支在 Debian sid（x86_64、glibc、NVIDIA 显卡）上实现和验证。

完整的教程在 <https://dreamldx.github.io/ComfyStudy/>，这一条支线的说明在
[支线 platform-01：Linux 阶段 0](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-01-linux-foundation/)。
下面按章节列出这个分支上已经实现了什么、为什么这样做，以及每一章详细讲解的链接。

## 构建与运行

现在可以在 Linux 上构建、运行和测试整个项目。在仓库根目录：

```bash
scripts/check-env.sh                 # 检查工具链和 vcpkg，并做两次小的试编译
scripts/check-env.sh --no-smoke      # 只检查工具，不试编译
bash tests/scripts/test-check-env.sh # 这个脚本自己的测试

# 第一次：按 check-env.sh 最后一行的提示装好 vcpkg，然后（VCPKG_ROOT 要先设好）
export VCPKG_ROOT=$HOME/vcpkg
$VCPKG_ROOT/vcpkg install --triplet x64-linux-gcc   # 在 vcpkg_installed/ 里装出清单里的库

# 构建、测试、运行（预设叫 linux-debug、linux-release、linux-asan）
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
build/linux-debug/bin/comfystudy
```

## 本分支实现了什么

### platform-1.1 Linux 工具链与环境检查

- 提交：`chapter-platform-1.1-toolchain-check:`
- 目的：拿到一台 Linux 机器，用一条命令知道工具链是否齐全、版本是否对，对应主线里的 `scripts/check-env.ps1`。
- 说明：
  - `scripts/check-env.sh`：检查 `g++-16`、`gcc-16`（编译器按主版本固定，只认带版本号的名字，不看 `g++` 别名）、CMake ≥ 3.25、Ninja、
    pkg-config、git、ld；报告 glibc 版本；用固定的编译器编译并运行一个用到 concepts、`std::span` 和协程的 C++20 小程序，再带
    AddressSanitizer 和 UndefinedBehaviorSanitizer 编译运行一次。缺东西时列出名字，并打印一行 `apt install`
    建议（只打印，不执行），退出码为 1。输出格式与 `check-env.ps1` 一致。
  - `tests/scripts/test-check-env.sh`：11 个用例，用假命令目录冒充各种缺失和版本不对的情况，不碰真实系统。
- 文档：[platform-1.1 Linux 工具链与环境检查](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-01-linux-foundation/platform-1.1-toolchain-check/)

### platform-1.2 用 vcpkg 管理依赖

- 提交：`chapter-platform-1.2-vcpkg-deps:`
- 目的：第三方库的版本由主线的 `vcpkg.json` 和 `builtin-baseline` 决定，在 Linux 上也一样；并且这些库和项目本身用同一个固定的 GCC 构建。
- 说明：
  - `cmake/toolchains/linux-gcc.cmake`：把 C 和 C++ 编译器设成 `gcc-16`、`g++-16`。
  - `triplets/x64-linux-gcc.cmake`：`x64-linux`（静态库，系统的动态运行库），加上链式加载的工具链文件。vcpkg 不使用 `CC`、`CXX` 环境变量，链式工具链是让所有端口用同一个编译器的办法。
  - `vcpkg-configuration.json`：告诉 vcpkg 到 `triplets/` 里找自定义的三元组。
  - `scripts/check-env.sh`：新增 `curl`、`zip`、`unzip`、`tar`、`make`、`perl`（要能加载 `IPC::Cmd`，openssl 端口要用）和 `vcpkg`（通过 `VCPKG_ROOT` 找）的检查；缺 vcpkg 时打印克隆（检出清单基线）和自举的命令。
  - `tests/scripts/test-check-env.sh`：增加到 21 个用例，其中一个断言脚本和工具链文件里固定的 GCC 主版本相同。
  - 在 `x64-linux-gcc` 下装出的库：cli11 2.7.2、curl 8.23.0（用 OpenSSL）、fmt 12.2.1、gtest 1.18.0、nlohmann-json 3.12.0#2、openssl 3.6.5#2、spdlog 1.17.0#1、zlib 1.4，每个静态库的 `.comment` 段都是 `GCC: (Debian 16.2.0-5) 16.2.0`；两边都有的端口，版本与 Windows 完全一致。
- 文档：[platform-1.2 用 vcpkg 管理依赖](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-01-linux-foundation/platform-1.2-vcpkg-deps/)

### platform-1.3 Linux 的 CMake 平台文件与最小移植

- 提交：`chapter-platform-1.3-cmake-linux:`
- 目的：让整个项目在 Linux 上构建、运行、跑通测试，库和项目由同一个固定的 `g++-16` 编译。
- 说明：
  - `cmake/platform/linux.cmake`：只接受 GCC；`-Wall -Wextra -Werror`；缓存变量 `COMFYSTUDY_SANITIZE`（非空时给所有目标加 `-fsanitize=...` 的编译和链接选项）；用 `find_package` 找 vcpkg 装好的库；顶层要求的变量和函数（Linux 上没有额外的源码、库、目标和测试）。
  - `cmake/presets/linux.json`：`linux-debug`、`linux-release`、`linux-asan` 的配置、构建和测试预设；接入 vcpkg 工具链、三元组 `x64-linux-gcc`，并通过 `VCPKG_CHAINLOAD_TOOLCHAIN_FILE` 让项目本身也用 `cmake/toolchains/linux-gcc.cmake`；只在 Linux 主机上可用。`CMakePresets.json` 多包含这一个文件。
  - `src/config.cpp`：`config::Get` 用 `getenv`，`config::ExeDirectory` 读 `/proc/self/exe`，替换原来的两处 `#error`。
  - `tests/env_util.h`：可移植的 `SetEnv`（空值表示删除变量）；`test_config.cpp`、`test_options.cpp` 改用它，`test_config` 里对可执行文件名的期望按系统决定。
  - 结果：`linux-debug` 和 `linux-release` 构建没有警告，`ctest` 42 个测试全部通过；`linux-asan` 能构建并运行（也顺手跑过一次测试，42 个通过，详细处理在 platform-1.4）。Windows 一侧不受影响：`windows-debug` 预设构建没有警告，`ctest` 62 个通过。
- 文档：[platform-1.3 Linux 的 CMake 平台文件与最小移植](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-01-linux-foundation/platform-1.3-cmake-linux/)
