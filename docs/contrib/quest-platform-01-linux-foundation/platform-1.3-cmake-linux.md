# platform-1.3 Linux 的 CMake 平台文件与最小移植

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/platform-01-linux-foundation`（支线不打 tag，用 `git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.3"` 定位本章提交）
- 所属支线：[支线 platform-01：Linux 阶段 0](README.md)
- 可开始于：主线 [0.6 检测 GPU 与 CUDA 驱动](../../tutorial/phase-00-toolchain/0.6-gpu-check.md)
- 一句话目标：在主线已拆分的基础上新增 `cmake/platform/linux.cmake` 和 `linux-debug`、`linux-release`、`linux-asan` 三个预设，接入 platform-1.2 的 vcpkg 和固定的 `g++-16`；补上能编过的最小 Linux 分支（`config::Get` 读环境变量，`config::ExeDirectory` 读 `/proc/self/exe`，测试里的环境变量改成可移植的写法），做一次完整构建，42 个测试通过，程序输出已加载的配置项名称

## 1. 本章目标与成果

platform-1.2 把库装好了，但项目本身还不能在 Linux 上构建：没有 Linux 的平台文件，没有预设，`config.cpp` 里还有两处 `#error`，测试里用了 Windows 才有的 `_putenv_s`。本章让整个项目在 Linux 上构建、运行、跑通测试，库和项目由同一个固定的 `g++-16` 编译。

做完后你会得到：

- `cmake/platform/linux.cmake`：只接受 GCC；`-Wall -Wextra -Werror`；一个 sanitizer 开关；用 `find_package` 找 vcpkg 装好的库；顶层要求的变量和函数。
- `cmake/presets/linux.json`：`linux-debug`、`linux-release`、`linux-asan` 三个预设，各有配置、构建和测试三部分；接入 vcpkg 的工具链、三元组 `x64-linux-gcc` 和固定的编译器。
- 最小移植：`config::Get` 用 `getenv`，`config::ExeDirectory` 读 `/proc/self/exe`，换掉原来的两处 `#error`。
- 一个可移植的 `SetEnv`（`tests/env_util.h`），让 `test_config` 和 `test_options` 在 Linux 上能编能跑。
- `linux-debug` 和 `linux-release` 构建没有任何警告，42 个测试全部通过；程序能输出已加载的配置项名称，能找到可执行文件旁边的 `config.toml`。
- `linux-asan` 能构建并运行，`-fsanitize=address,undefined` 同时出现在编译和链接选项里。用它正式跑测试是 platform-1.4。

先看效果：还没有 Linux 的平台文件时，在 Linux 上配置项目。

```bash
cmake -S . -B build/x -G Ninja
```

```text
CMake Error at CMakeLists.txt:15 (message):
  No build support for Linux on this branch (missing
  /home/dreamldx/Projects/ComfyStudy/cmake/platform/linux.cmake)

-- Configuring incomplete, errors occurred!
```

顶层 `CMakeLists.txt` 按系统名找 `cmake/platform/<系统名>.cmake`，找不到就停。加上平台文件和预设之后配置（`VCPKG_ROOT` 要先设好）：

```bash
export VCPKG_ROOT=$HOME/vcpkg
cmake --preset linux-debug
```

```text
-- Running vcpkg install - done
-- The CXX compiler identification is GNU 16.2.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/g++-16 - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found nlohmann_json: /home/dreamldx/Projects/ComfyStudy/vcpkg_installed/x64-linux-gcc/share/nlohmann_json/nlohmann_jsonConfig.cmake (found version "3.12.0")
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Found OpenSSL: /home/dreamldx/Projects/ComfyStudy/vcpkg_installed/x64-linux-gcc/debug/lib/libcrypto.a (found suitable version "3.6.5", minimum required is "3")
-- Found ZLIB: optimized;/home/dreamldx/Projects/ComfyStudy/vcpkg_installed/x64-linux-gcc/lib/libz.a;debug;/home/dreamldx/Projects/ComfyStudy/vcpkg_installed/x64-linux-gcc/debug/lib/libz.a (found suitable version "1.4", minimum required is "1")
-- Configuring done (0.9s)
-- Generating done (0.0s)
-- Build files have been written to: /home/dreamldx/Projects/ComfyStudy/build/linux-debug
```

配置成功，编译器是 `/usr/bin/g++-16`，库来自 `vcpkg_installed/x64-linux-gcc/`。接着构建，这时还没有移植：

```bash
cmake --build --preset linux-debug -- -k 0
```

```text
/home/dreamldx/Projects/ComfyStudy/src/config.cpp:64:2: error: #error "config::Get is not implemented for this platform on this branch"
/home/dreamldx/Projects/ComfyStudy/src/config.cpp:101:2: error: #error "config::ExeDirectory is not implemented for this platform on this branch"
/home/dreamldx/Projects/ComfyStudy/tests/test_config.cpp:101:3: error: ‘_putenv_s’ was not declared in this scope; did you mean ‘putenv’?
/home/dreamldx/Projects/ComfyStudy/tests/test_options.cpp:46:7: error: ‘_putenv_s’ was not declared in this scope; did you mean ‘putenv’?
```

（`-k 0` 让构建遇到错误也继续，这样一次看到所有失败；上面是筛出的关键行。）正好是这三处。移植之后再构建、测试、运行：

```bash
cmake --build --preset linux-debug
ctest --preset linux-debug
build/linux-debug/bin/comfystudy
```

```text
[11/14] Linking CXX executable bin/test_options
[12/14] Linking CXX executable bin/test_logging
[13/14] Linking CXX executable bin/test_deps
[14/14] Linking CXX executable bin/comfystudy
```

```text
100% tests passed out of 42
```

```text
ComfyStudy 0.5
libcurl/8.23.0 OpenSSL/3.6.5 zlib/1.4
[warning] no config file found (looked next to the executable and in the current directory); using the environment and the defaults only
discord_token=(unset)
llm_provider=openai
llm_model=(unset)
openai_api_key=(unset)
openai_base_url=https://api.openai.com/v1
anthropic_api_key=(unset)
anthropic_base_url=https://api.anthropic.com
model_dir=models
```

构建、测试、运行都过了，最后输出的就是已加载的配置项名称。以上是 2026-11-24 在 Debian sid 的实体机上的结果。

## 2. 背景与概念

### 2.1 顶层、平台文件和预设的分工

主线 0.1 把构建拆成三层：顶层 `CMakeLists.txt` 只写与系统无关的部分；`cmake/platform/<系统名>.cmake` 放这个系统自己的东西；`cmake/presets/<系统>.json` 放这个系统的预设，由 `CMakePresets.json` 的 `include` 列出来。顶层要求平台文件提供：

- `COMFYSTUDY_COMPILE_OPTIONS`：所有目标的编译选项。
- `COMFYSTUDY_PLATFORM_SOURCES`、`COMFYSTUDY_PLATFORM_LIBRARIES`：只在这个系统上存在的源文件和系统库。
- 函数 `comfystudy_platform_targets()` 和 `comfystudy_platform_tests()`：只在这个系统上存在的目标和测试。

Windows 的平台文件在这几处放了 `dll_info.cpp`、`psapi`、`version`，以及 DLL 管理的目标（`deploy`、`check-dlls`）和测试。Linux 上这些都没有：库是静态库，没有 DLL 要管理，所以这五项里有四项是空的，两个函数是空函数。

### 2.2 为什么预设叫 `linux-*`

CMake 规定，同一类预设的名字在所有被包含的文件里不能重复，所以两个系统的预设不能都叫 `debug` 和 `release`。这条线上每个系统的预设都用系统名做前缀：Windows 的是 `windows-debug`、`windows-release`、`windows-vs`，Linux 的是 `linux-debug`、`linux-release`、`linux-asan`。构建目录是 `build/<预设名>`，所以两个系统的构建目录也不会混在一起。

Linux 的预设都带一个条件：主机系统是 Linux。在 Windows 上 `cmake --list-presets` 列出的是 `windows-debug`、`windows-release`、`windows-vs`，看不到 `linux-*`。反过来，Windows 的预设没有加主机条件，所以在 Linux 上 `cmake --list-presets` 还会列出 `windows-debug` 和 `windows-release`；它们是给 Windows 用的，在 Linux 上请用 `linux-*`。

### 2.3 编译器怎么到项目里

platform-1.2 让 vcpkg 构建库时用 `g++-16`，靠的是三元组里的 `VCPKG_CHAINLOAD_TOOLCHAIN_FILE`，它只管 vcpkg 构建的库。项目本身要用同一个编译器，预设里要再设一次同名的缓存变量，指向同一个 `cmake/toolchains/linux-gcc.cmake`。实际效果是：CMake 记下的编译器是 `/usr/bin/g++-16`，配置日志里是 `Check for working CXX compiler: /usr/bin/g++-16 - skipped`。

### 2.4 警告选项

MSVC 一侧是 `/W4 /WX /permissive-`。GCC 一侧是 `-Wall -Wextra -Werror`：常用的警告全开，警告就是错误。这条线上不用 `-Wno-...` 去压警告：出现警告要改代码，或者写明为什么这条警告不适用。GCC 16 下，0.5 的所有源码在这组选项下构建没有任何警告，Debug、Release 和 asan 三个预设都是如此。

### 2.5 sanitizer 开关

AddressSanitizer 和 UndefinedBehaviorSanitizer 要在编译和链接时都加 `-fsanitize=...`，而且要对所有目标生效，包括测试程序。平台文件里用一个缓存变量 `COMFYSTUDY_SANITIZE` 来控制：空表示不开；`linux-asan` 预设把它设成 `address,undefined`。非空时用 `add_compile_options` 和 `add_link_options` 给后面创建的所有目标加上。

除了 `-fsanitize`，还加了两个：`-fno-omit-frame-pointer` 让报告里的调用栈更完整，`-fno-sanitize-recover=all` 让第一个错误就终止程序，而不是打印一条消息接着跑。

### 2.6 最小移植

0.5 的代码把系统相关的部分放在 `#if defined(_WIN32)` 里，非 Windows 的分支是 `#error`，意思是“要在别的系统上用，先来这里实现”。本章要实现的只有让项目能编、让测试有意义的最小部分：

- **`config::Get`**：读环境变量，用 `getenv`。`config.h` 里写明“空值当作未设置”，Windows 的实现和 Linux 的都遵守，所以空字符串返回“没有”。
- **`config::ExeDirectory`**：程序所在的目录，用来找旁边的 `config.toml`。Linux 上 `/proc/self/exe` 是一个符号链接，指向正在运行的可执行文件，读出它再取父目录。读不出来就返回空路径，和 Windows 分支失败时一样。

别的 Linux 做法，比如按 XDG 规范到 `~/.config` 找配置文件、判断终端是不是支持颜色，这条线上不做。XDG 是新功能而不是移植：现在的程序在 Windows 上也只找可执行文件旁边和当前目录，Linux 上同样够用。终端颜色只有 Windows 专用的 `--dependency-check` 用到，Linux 上没有对应的代码。想给 Linux 加 XDG 目录，见练习 6。

### 2.7 测试里的环境变量

两个测试文件用 `_putenv_s` 设置环境变量，这个函数只有 Windows 有。Windows 上 `_putenv_s(name, "")` 的意思是删除变量，原来的测试也靠这一点（注释里写着“an empty value removes the variable”）。新增的 `tests/env_util.h` 里的 `SetEnv` 在每个系统上都是这个含义：Windows 上调 `_putenv_s`；其余系统上，值为空就 `unsetenv`，否则 `setenv`。

另一处：`test_config` 期望可执行文件旁边有 `test_config.exe`。Linux 上可执行文件没有扩展名，期望按系统决定。

## 3. 本章改动

本章的代码在支线分支 `quest/platform-01-linux-foundation` 上，不合入 main，也不打 tag。提交信息以 `chapter-platform-1.3-cmake-linux:` 开头，`git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.3"` 可以找到本章的提交。

1. 新增 `cmake/platform/linux.cmake` 和 `cmake/presets/linux.json`。
2. `CMakePresets.json`：`include` 里加上 `cmake/presets/linux.json`。
3. `CMakeLists.txt`：只改了一句注释，“这个分支只有 `windows.cmake`”改成“有 `windows.cmake` 和 `linux.cmake`”。
4. `src/config.cpp`：两处 `#error` 换成实现。
5. 新增 `tests/env_util.h`；`tests/test_config.cpp`、`tests/test_options.cpp` 改用它，`test_config` 里对可执行文件名的期望按系统决定。
6. 分支的 `README.md`：加上本章，更新构建与运行。

Windows 的文件除了 `CMakePresets.json` 多包含一个文件，都没有动。

## 4. 关键代码讲解

### 4.1 平台文件

<!-- src: cmake/platform/linux.cmake#L4-L18 -->
```cmake
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "ComfyStudy is built with GCC on Linux (configure with one of the linux-* presets, "
                      "which select the pinned g++ through cmake/toolchains/linux-gcc.cmake).")
endif()

# Compiler options for every target.
set(COMFYSTUDY_COMPILE_OPTIONS -Wall -Wextra -Werror)

# Sanitizers: the linux-asan preset sets this to address,undefined. The options go to every target (tests too),
# for compiling and for linking.
set(COMFYSTUDY_SANITIZE "" CACHE STRING "Sanitizers for -fsanitize, comma separated (empty: none)")
if(COMFYSTUDY_SANITIZE)
  add_compile_options(-fsanitize=${COMFYSTUDY_SANITIZE} -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=${COMFYSTUDY_SANITIZE})
endif()
```

开头先检查编译器：不是 GCC 就停，并告诉你用 `linux-*` 预设。接着是警告选项，然后是 sanitizer 开关。注意 `add_compile_options` 和 `add_link_options` 是整个目录范围的：平台文件在顶层创建任何目标之前被包含，所以后面所有目标，包括 `tests/` 里的，都带着这些选项。

<!-- src: cmake/platform/linux.cmake#L20-L38 -->
```cmake
# Third-party libraries come from vcpkg (see vcpkg.json); the preset points CMake at its toolchain file.
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# Sources and system libraries that exist on Linux only: none yet.
set(COMFYSTUDY_PLATFORM_SOURCES "")
set(COMFYSTUDY_PLATFORM_LIBRARIES "")

# Called by the top-level CMakeLists.txt once the targets exist. Linux has nothing here: the DLL management of
# the Windows build (deploy, check-dlls) has no counterpart, because vcpkg's libraries are static.
function(comfystudy_platform_targets)
endfunction()

# Called by tests/CMakeLists.txt once add_comfystudy_test exists: the tests that run on Linux only. None yet.
function(comfystudy_platform_tests)
endfunction()
```

找库的写法和 Windows 的 `windows.cmake` 一样，目标名也一样，库的来源由预设里的工具链决定，平台文件不需要知道库在哪里。后面是四个空项和两个空函数，注释里写明了为什么 Linux 上没有东西。

### 4.2 预设

<!-- src: cmake/presets/linux.json#L1-L54 -->
```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "linux-vcpkg",
      "hidden": true,
      "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
      "cacheVariables": {
        "VCPKG_TARGET_TRIPLET": "x64-linux-gcc",
        "VCPKG_INSTALLED_DIR": "${sourceDir}/vcpkg_installed",
        "VCPKG_CHAINLOAD_TOOLCHAIN_FILE": "${sourceDir}/cmake/toolchains/linux-gcc.cmake"
      }
    },
    {
      "name": "linux-base",
      "hidden": true,
      "inherits": "linux-vcpkg",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "condition": { "type": "equals", "lhs": "${hostSystemName}", "rhs": "Linux" },
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "linux-debug",
      "inherits": "linux-base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
    },
    {
      "name": "linux-release",
      "inherits": "linux-base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
    },
    {
      "name": "linux-asan",
      "inherits": "linux-base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "COMFYSTUDY_SANITIZE": "address,undefined"
      }
    }
  ],
  "buildPresets": [
    { "name": "linux-debug", "configurePreset": "linux-debug" },
    { "name": "linux-release", "configurePreset": "linux-release" },
    { "name": "linux-asan", "configurePreset": "linux-asan" }
  ],
  "testPresets": [
    { "name": "linux-debug", "configurePreset": "linux-debug", "output": { "outputOnFailure": true } },
    { "name": "linux-release", "configurePreset": "linux-release", "output": { "outputOnFailure": true } },
    { "name": "linux-asan", "configurePreset": "linux-asan", "output": { "outputOnFailure": true } }
  ]
}
```

`linux-vcpkg` 接入 vcpkg：工具链文件、三元组、安装目录，以及固定编译器的 `VCPKG_CHAINLOAD_TOOLCHAIN_FILE`。`linux-base` 在它之上加了生成器 Ninja、构建目录、`compile_commands.json`，以及“主机是 Linux”的条件。三个具体预设只管构建类型和 sanitizer。两个隐藏预设的名字也带 `linux-` 前缀，和 Windows 的 `windows-vcpkg`、`windows-base` 对应。

### 4.3 两个函数的 Linux 分支

<!-- src: src/config.cpp#L53-L70 -->
```cpp
std::optional<std::string> Get(std::string_view env_name) {
  std::string value;
#if defined(_WIN32)
  char* buffer = nullptr;
  std::size_t length = 0;
  if (_dupenv_s(&buffer, &length, std::string(env_name).c_str()) != 0 || buffer == nullptr) {
    return std::nullopt;
  }
  value = buffer;
  free(buffer);
#else
  const char* raw = getenv(std::string(env_name).c_str());
  if (raw == nullptr) return std::nullopt;
  value = raw;
#endif
  if (value.empty()) return std::nullopt;
  return value;
}
```

非 Windows 的分支就是三行：`getenv`，没有就返回“没有”，有就拿到字符串。空值由下面共用的 `if (value.empty())` 处理，所以 Windows 和 Linux 的行为一致。

<!-- src: src/config.cpp#L89-L109 -->
```cpp
std::filesystem::path ExeDirectory() {
#if defined(_WIN32)
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    const DWORD copied = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (copied == 0) return {};
    if (copied < buffer.size()) {
      buffer.resize(copied);
      break;
    }
    buffer.resize(buffer.size() * 2);  // the path did not fit: try a bigger buffer
  }
  return std::filesystem::path(buffer).parent_path();
#else
  // /proc/self/exe is a symbolic link to the running executable.
  std::error_code error;
  const std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", error);
  if (error) return {};
  return exe.parent_path();
#endif
}
```

Linux 分支把 `/proc/self/exe` 当符号链接读出来，取父目录。用带 `error_code` 的版本，读不出来不抛异常，返回空路径。

### 4.4 测试里的改动

<!-- src: tests/env_util.h#L1-L17 -->
```cpp
#pragma once

#include <stdlib.h>

// Sets an environment variable for the test program. An empty value removes the variable on every system
// (on Windows that is what _putenv_s does with ""), so a test can start from "not set".
inline void SetEnv(const char* name, const char* value) {
#if defined(_WIN32)
  _putenv_s(name, value);
#else
  if (*value == '\0') {
    unsetenv(name);
  } else {
    setenv(name, value, 1);
  }
#endif
}
```

<!-- src: tests/test_config.cpp#L110-L118 -->
```cpp
TEST(ConfigPaths, ExeDirectoryContainsThisTestProgram) {
  const std::filesystem::path dir = config::ExeDirectory();
  EXPECT_TRUE(std::filesystem::is_directory(dir));
#if defined(_WIN32)
  EXPECT_TRUE(std::filesystem::exists(dir / "test_config.exe"));
#else
  EXPECT_TRUE(std::filesystem::exists(dir / "test_config"));
#endif
}
```

可执行文件名的期望按系统分开写，其余的测试逻辑没有变。

<!-- src: tests/test_options.cpp#L38-L50 -->
```cpp
// Every test starts with none of the known environment variables set, whatever the developer's
// machine has, and the original values are put back afterwards.
class OptionsTest : public testing::Test {
 protected:
  void SetUp() override {
    for (const config::Key& key : config::KnownKeys()) {
      saved_.emplace_back(key.env, config::Get(key.env));
      SetEnv(key.env.c_str(), "");
    }
  }
  void TearDown() override {
    for (const auto& [name, value] : saved_) SetEnv(name.c_str(), value.value_or("").c_str());
  }
```

`OptionsTest` 的 `SetUp` 和 `TearDown` 现在直接用共用的 `SetEnv`，不再有自己的一份。

## 5. 构建与运行

在支线分支上，`VCPKG_ROOT` 已设好、库已装好（platform-1.2）的前提下：

```bash
git switch quest/platform-01-linux-foundation
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
build/linux-debug/bin/comfystudy
```

用环境变量覆盖一个默认值，和让可执行文件旁边的 `config.toml` 生效：

```bash
LLM_MODEL=test-model build/linux-debug/bin/comfystudy 2>/dev/null | grep llm_model
cp config.toml.example build/linux-debug/bin/config.toml
build/linux-debug/bin/comfystudy -v 2>&1 | grep -E "config candidate|loaded"
```

```text
llm_model=test-model
[debug] config candidate ~/Projects/ComfyStudy/build/linux-debug/bin/config.toml: found
[debug] config candidate ~/Projects/ComfyStudy/build/linux-debug/bin/config.ini: not found
[debug] config candidate ~/Projects/ComfyStudy/config.toml: not found
[debug] config candidate ~/Projects/ComfyStudy/config.ini: not found
[info] loaded ~/Projects/ComfyStudy/build/linux-debug/bin/config.toml
```

`config.toml` 放在可执行文件旁边，第一个候选就被找到并加载，说明 `ExeDirectory` 在 Linux 上是对的（输出里的 `~` 是我把家目录缩写了）。另外两个预设：

```bash
cmake --preset linux-release && cmake --build --preset linux-release && ctest --preset linux-release
cmake --preset linux-asan && cmake --build --preset linux-asan
```

## 6. 验证

- `ctest --preset linux-debug`：42 个测试全部通过，它们来自五个测试程序（`test_greeting`、`test_deps`、`test_logging`、`test_options`、`test_config`）。Windows 上有的 `test_dll_info`、`check_dlls`、`dependency_check` 在 Linux 上不存在。
- 构建没有警告：`linux-debug`、`linux-release`、`linux-asan` 三个预设，构建日志里 `warning:` 和 `error:` 都是 0 行。`linux-release` 的 `ctest` 也是 42 个全过。
- 编译器确实是 GCC 16：`CMakeFiles/*/CMakeCXXCompiler.cmake` 里记的是 `/usr/bin/g++-16`；`CMakeCache.txt` 里 `VCPKG_TARGET_TRIPLET` 是 `x64-linux-gcc`，`VCPKG_CHAINLOAD_TOOLCHAIN_FILE` 指向项目里的 `linux-gcc.cmake`；`readelf -p .comment build/linux-debug/bin/comfystudy` 读到 `GCC: (Debian 16.2.0-5) 16.2.0`。`compile_commands.json` 已生成。
- 运行程序：无参数时输出问候、libcurl 版本和八个配置项名称；`LLM_MODEL=test-model` 能覆盖默认值；`config.toml` 放在可执行文件旁边能被找到并加载。
- 改动检验（两个）：
  - 把 `ExeDirectory` 的 Linux 分支临时改成返回空路径：`ConfigPaths.ExeDirectoryContainsThisTestProgram` 失败，其余通过。改回后 42 个全过。
  - 把工具链文件里的主版本临时改成 15（没有 `g++-15`）：配置失败，vcpkg 报 `error: while detecting compiler information`。改回去。
- sanitizer 预设：`build/linux-asan` 的缓存里 `COMFYSTUDY_SANITIZE` 是 `address,undefined`；`compile_commands.json` 里的编译命令带 `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all`；`build.ninja` 里每个目标的 `LINK_FLAGS` 都以 `-fsanitize=address,undefined` 开头；`ldd` 显示 `comfystudy` 加载了 `libasan.so.8` 和 `libubsan.so.1`，而 debug 版的 `comfystudy` 一个也没有。程序在 sanitizer 下能正常运行。顺手在这个预设下跑了一次 `ctest`，42 个也通过了，怎么把它当成正式的一步和怎么处理结果，是 platform-1.4。
- Windows 一侧：`scripts/dev.ps1 cmake --list-presets` 只列出 `windows-debug`、`windows-release`、`windows-vs`；在这个分支上 `cmake --preset windows-debug` 配置成功，构建没有警告，`ctest` 62 个全部通过。
- 没有做的：
  - CMake 只试了 4.4.5，项目声明的最低版本 3.25 没有试过。
  - 只用了 GCC，没有试 Clang。
  - sanitizer 下的测试结果只看了通过与否，没有分析（platform-1.4）。
  - 没有给 Windows 的预设加主机条件，所以 Linux 上 `cmake --list-presets` 仍会列出 `windows-debug`、`windows-release`。

## 7. 技术资料

- 环境：CMake 4.4.5、Ninja 1.13.2、GCC 16.2.0（Debian 包 16.2.0-5）、vcpkg 工具 2026-10-29，Debian sid，2026-11-24。
- CMake 预设的格式和条件：<https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html>。
- `/proc/self/exe`：<https://man7.org/linux/man-pages/man5/proc_pid_exe.5.html>。
- GCC 的 sanitizer 选项：<https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html>。
- vcpkg 的三元组变量（`VCPKG_CHAINLOAD_TOOLCHAIN_FILE`）：<https://learn.microsoft.com/vcpkg/users/triplets>。
- 设计取舍：
  - 预设命名：每个系统用系统名做前缀，`windows-*` 和 `linux-*`（采用）；Windows 保持 `debug`、`release`，只给 Linux 加前缀（否决，两边不对称，命令不好记）；同名加条件（不可行，CMake 不允许同名）。
  - sanitizer 开关：缓存变量加 `add_compile_options` 和 `add_link_options`（采用）；在 `comfystudy_options` 接口库上加链接选项（否决，要改顶层，而顶层目前只传编译选项）。
  - 项目也用链式工具链固定编译器，而不是在平台文件里写 `CMAKE_CXX_COMPILER`：这样固定编译器的来源只有 `linux-gcc.cmake` 一个文件。
  - 最小移植只做能编、让测试有意义的部分，不加新功能：XDG 配置目录是新功能（Windows 一侧也没有对应的 `%APPDATA%`），终端颜色在 Linux 上没有使用者，这条线上都不做。
- 被否决的：在源码里到处写 `#if defined(__linux__)`（系统相关的部分只放在 `config.cpp` 已有的 `#if` 块里）；去掉 `-Werror` 或加 `-Wno-...` 来压警告。

## 8. 常见问题与坑

**`VCPKG_ROOT` 没设置，配置会报一个奇怪的路径。** 预设里的工具链路径是 `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`，变量为空时路径变成 `/scripts/buildsystems/vcpkg.cmake`，报错是 `Could not find toolchain file: "/scripts/buildsystems/vcpkg.cmake"`，后面还跟着“找不到 Ninja”“编译器没设置”这样的连带错误，容易看错方向。我就是通过 ssh 在非交互的会话里运行时撞上的：那种会话不读 `~/.zshrc`，变量是空的。`scripts/check-env.sh` 的 vcpkg 检查会提前发现它。

**预设名字在所有被包含的文件里不能重复。** 即使加了“只在 Linux 上可用”的条件也不行，CMake 按名字检查，不看条件。

**移植前的构建里，一个 `#error` 会带出一串连带错误。** 上面的“先看效果”里，`config.cpp` 实际报了四条：两条 `#error`，另外两条是 `unused parameter` 和 `no return statement`，因为 `#error` 之后函数是空的，在 `-Wextra -Werror` 下成了错误。看第一条就够了，修好之后连带的就没了。

**环境变量“设成空”和“没有”在 Linux 上是两回事。** Windows 的 `_putenv_s(name, "")` 会删掉变量，Linux 的 `setenv(name, "", 1)` 只是把它设成空字符串。`config::Get` 把空值当作没有，所以对程序来说没有区别；但测试辅助函数 `SetEnv` 要自己保证“空值就删除”，这样 Windows 和 Linux 上的测试起点是一样的。

**sanitizer 下的可执行文件依赖 `libasan.so` 和 `libubsan.so`。** 它们是动态库，来自 GCC 的包。换一台机器运行 `linux-asan` 构建出的程序，要有对应版本的这两个库。

**Windows 的预设还在 Linux 的列表里。** `cmake --list-presets` 在 Linux 上会列出 `windows-debug` 和 `windows-release`，它们是 Windows 的，用的是 Windows 的工具链，请不要用。

## 9. 练习

1. 给 `COMFYSTUDY_COMPILE_OPTIONS` 加上 `-Wshadow -Wconversion`，重新构建。哪些地方出了警告？你会改代码还是去掉哪条选项？理由是什么？
2. 加一个 `linux-tsan` 预设，用 ThreadSanitizer（`COMFYSTUDY_SANITIZE=thread`）。它能和 `address` 同时开吗？试一下，看编译器怎么说。
3. 不改预设，直接在命令行里用 `-DCOMFYSTUDY_SANITIZE=undefined` 配置一个只有 UBSan 的构建目录，确认编译和链接选项都变了。
4. 用 `readlink` 系统调用重写 `ExeDirectory` 的 Linux 分支，测试要保持通过。和 `std::filesystem::read_symlink` 比，哪个更适合这个项目？
5. 在 `comfystudy_platform_tests()` 里加一个只在 Linux 上存在的测试：运行 `comfystudy` 并检查输出里有 `llm_model=`。
6. 让 Linux 也到 `$XDG_CONFIG_HOME/comfystudy/`（没设置时是 `~/.config/comfystudy/`）找配置文件：改 `config::ConfigCandidates`，先写测试，决定它排在“可执行文件旁边”和“当前目录”的哪个位置，并想一想为什么。Windows 上要不要也加一个对应的目录？

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 platform-1.4 在 `linux-asan` 预设下正式跑测试。这一章已经顺手跑过一次，42 个测试通过；下一章要把它做成一个明确的步骤，看清 sanitizer 的输出、泄漏检测的行为，以及用 CTest 和 GoogleTest 组织这一步。
