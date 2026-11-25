# platform-1.2 用 vcpkg 管理依赖

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/platform-01-linux-foundation`（支线不打 tag，用 `git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.2"` 定位本章提交）
- 所属支线：[支线 platform-01：Linux 阶段 0](README.md)
- 可开始于：主线 [0.6 检测 GPU 与 CUDA 驱动](../../tutorial/phase-00-toolchain/0.6-gpu-check.md)
- 一句话目标：在 Linux 上装 vcpkg（检出到清单里的基线提交），让它用固定的 `g++-16` 构建 `vcpkg.json` 里的库（自定义三元组 `x64-linux-gcc` 加链式工具链文件），把 `check-env.sh` 扩展到 vcpkg 和它要用的工具（curl、zip、unzip、tar、make、perl），并把 Linux 和 Windows 上解析出的版本逐个对比

## 1. 本章目标与成果

主线在 Windows 上用 vcpkg 解决了两件事：库的版本由 `vcpkg.json` 和 `builtin-baseline` 固定；库由项目的编译器和选项构建。Linux 线要做到同样两件，并且让它们和 Windows 一致：同一份清单，同一个基线，同样的库版本。

platform-1.1 固定了编译器，第三方库还没有着落。本章把 vcpkg 装到机器上，让它用固定的 `g++-16` 构建清单里的库，再把装出的版本和 Windows 上的逐个对比。

做完后你会得到：

- 装在 `~/vcpkg` 的 vcpkg，检出到 `vcpkg.json` 里的基线提交，工具版本 2026-10-29。
- 扩展后的 `scripts/check-env.sh`：多检查 `curl`、`zip`、`unzip`、`tar`、`make`、`perl` 和 vcpkg（通过 `VCPKG_ROOT` 找），缺 vcpkg 时打印克隆、检出基线和自举的命令。
- 三个新文件：`cmake/toolchains/linux-gcc.cmake`（固定编译器）、`triplets/x64-linux-gcc.cmake`（让 vcpkg 用它）、`vcpkg-configuration.json`（告诉 vcpkg 到 `triplets/` 里找三元组）。
- 在 `vcpkg_installed/` 里用清单模式装出 cli11、curl、gtest、nlohmann-json、spdlog 和它们的依赖，编出来的静态库全部由 GCC 16.2.0 编译，装一次约 1 分钟。
- 一张 Linux 和 Windows 的版本对比表：两边都有的端口，版本逐一相同。
- `tests/scripts/test-check-env.sh` 增加到 21 个用例，其中一个断言脚本和工具链文件固定的是同一个 GCC 主版本。

本章不写 `cmake/platform/linux.cmake` 和预设，库装好之后怎么被 CMake 找到，是 platform-1.3。

先看效果：在一台还没有 vcpkg 的机器上，先跑环境检查。

```bash
scripts/check-env.sh
```

```text
OK      g++-16: 16.2.0
OK      gcc-16: 16.2.0
OK      cmake: cmake version 4.4.5
OK      ninja: version 1.13.2
OK      pkg-config: version 2.5.1
OK      git: git version 2.55.0
OK      ld: GNU ld (GNU Binutils for Debian) 2.47.50.20261120
OK      curl: curl 8.24.0-rc1
OK      zip: /usr/bin/zip
OK      unzip: /usr/bin/unzip
OK      tar: tar (GNU tar) 1.35
OK      make: GNU Make 4.4.1
OK      perl: /usr/bin/perl
MISSING vcpkg: VCPKG_ROOT is not set
INFO    glibc: ldd (Debian GLIBC 2.43-7) 2.43
OK      smoke: g++-16 compiled and ran a C++20 program (concepts, std::span, coroutine)
OK      smoke: AddressSanitizer and UndefinedBehaviorSanitizer linked and ran
check-env: FAILED, missing: vcpkg
install: git clone https://github.com/microsoft/vcpkg ~/vcpkg && git -C ~/vcpkg checkout 93ad765c7b66ab9ce97469ec23a90b15c3a14ee5 && ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
```

只缺 vcpkg，最后一行是可以直接复制的命令，里面的提交号是项目 `vcpkg.json` 里的基线。照着做，再设好 `VCPKG_ROOT`，再跑一次：

```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg
git -C ~/vcpkg checkout 93ad765c7b66ab9ce97469ec23a90b15c3a14ee5
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=$HOME/vcpkg
scripts/check-env.sh
```

```text
OK      g++-16: 16.2.0
OK      gcc-16: 16.2.0
OK      cmake: cmake version 4.4.5
OK      ninja: version 1.13.2
OK      pkg-config: version 2.5.1
OK      git: git version 2.55.0
OK      ld: GNU ld (GNU Binutils for Debian) 2.47.50.20261120
OK      curl: curl 8.24.0-rc1
OK      zip: /usr/bin/zip
OK      unzip: /usr/bin/unzip
OK      tar: tar (GNU tar) 1.35
OK      make: GNU Make 4.4.1
OK      perl: /usr/bin/perl
OK      vcpkg: vcpkg package management program version 2026-10-29-139f2e53438b1529e178f6295bc563f4e0de734f
INFO    glibc: ldd (Debian GLIBC 2.43-7) 2.43
OK      smoke: g++-16 compiled and ran a C++20 program (concepts, std::span, coroutine)
OK      smoke: AddressSanitizer and UndefinedBehaviorSanitizer linked and ran
check-env: OK
```

环境齐了。在项目目录里，让 vcpkg 按清单装库，并指定我们的三元组：

```bash
$VCPKG_ROOT/vcpkg install --triplet x64-linux-gcc
```

```text
Detecting compiler hash for triplet x64-linux...
Compiler found: /usr/bin/c++
Detecting compiler hash for triplet x64-linux-gcc...
Compiler found: /usr/bin/g++-16
The following packages will be built and installed:
    cli11:x64-linux-gcc@2.7.2
    curl[core,openssl,ssl]:x64-linux-gcc@8.23.0
  * fmt:x64-linux-gcc@12.2.1
    gtest:x64-linux-gcc@1.18.0
    nlohmann-json:x64-linux-gcc@3.12.0#2
  * openssl:x64-linux-gcc@3.6.5#2
    spdlog[core,fmt,tz-offset]:x64-linux-gcc@1.17.0#1
  * vcpkg-cmake:x64-linux@2026-11-09
  * vcpkg-cmake-config:x64-linux@2026-07-21
（后面是每个库的构建过程，这里省略）
All requested installations completed successfully in: 1.1 min
```

前四行里，vcpkg 为普通的 `x64-linux` 找到的是 `/usr/bin/c++`（系统默认），为我们的 `x64-linux-gcc` 找到的是 `/usr/bin/g++-16`。最后确认编出来的库真的是 GCC 16 的产物，每个静态库里都有编译器留下的标记：

```bash
for l in libspdlog.a libfmt.a libz.a libcurl.a libssl.a libcrypto.a; do
  printf '%s: ' $l
  readelf -p .comment vcpkg_installed/x64-linux-gcc/lib/$l | grep -oE 'GCC: [^"]*' | sort -u
done
```

```text
libspdlog.a: GCC: (Debian 16.2.0-5) 16.2.0
libfmt.a: GCC: (Debian 16.2.0-5) 16.2.0
libz.a: GCC: (Debian 16.2.0-5) 16.2.0
libcurl.a: GCC: (Debian 16.2.0-5) 16.2.0
libssl.a: GCC: (Debian 16.2.0-5) 16.2.0
libcrypto.a: GCC: (Debian 16.2.0-5) 16.2.0
```

以上都是 2026-11-24 在 Debian sid 的实体机上的结果。

## 2. 背景与概念

### 2.1 为什么 Linux 上也用 vcpkg

Debian 的包管理器也能装这些库的开发包，比如 `libspdlog-dev`。但那样版本是 sid 当天的，和 Windows 线不一样，两边出了不同的行为很难查。vcpkg 的清单模式把版本交给 `vcpkg.json` 里的 `builtin-baseline`：同一个基线，两个平台解析出同样的版本。基线、清单模式和三元组是什么，主线 0.3 讲过，这里只讲 Linux 上不同的地方。

### 2.2 vcpkg 装在哪里，用哪个版本

Windows 上 vcpkg 随 Visual Studio 一起来，`VCPKG_ROOT` 由 VS 的命令提示符设置。Linux 上要自己克隆：

- **装在 `~/vcpkg`，不在项目里。** 它是工具，不属于项目；项目只通过 `VCPKG_ROOT` 找到它。
- **完整克隆，不用 `--depth`。** 清单里的基线是一个提交号，vcpkg 要在 git 历史里找到这个提交才能知道各个库当时是什么版本。
- **检出到基线提交。** vcpkg 自带的辅助脚本和各个库的构建说明（端口）是同一个时间点的，不会出现“新脚本配旧端口”。这一章检出的提交是 `93ad765c7b`（"[libnfs] Update to 8.1.0 (#55214)"），自举后的工具版本是 2026-10-29。
- **`VCPKG_ROOT` 只在当前会话里设置。** 想让它长期有效，自己把 `export VCPKG_ROOT=$HOME/vcpkg` 写进 `~/.bashrc`；脚本不替你改。

### 2.3 三元组：静态库，系统的运行库

Windows 线用 `x64-windows`，库是动态库（DLL），所以 0.5 要专门管理 DLL。Linux 上这一章用 `x64-linux`：库是静态库，直接链进可执行文件；C 和 C++ 的运行库仍然是系统的动态库。好处是本章和下一章不用管理 `.so` 的搜索路径。后面的 libtorch 等预编译库本身是 `.so`，运行库路径留到那时在各自的章节里处理。

### 2.4 为什么要自定义三元组来固定编译器

vcpkg 构建每个库时，不使用环境变量 `CC` 和 `CXX`。想让它们都用 `g++-16`，官方支持的做法是在三元组里设置 `VCPKG_CHAINLOAD_TOOLCHAIN_FILE`，指向一个 CMake 工具链文件，vcpkg 会在构建每个库之前先加载它（[三元组变量的说明](https://learn.microsoft.com/vcpkg/users/triplets)）。

所以本章新增：

- `cmake/toolchains/linux-gcc.cmake`：把 C 和 C++ 编译器设成 `gcc-16`、`g++-16`。platform-1.3 的预设也会用同一个文件构建项目本身，库和项目由同一个编译器编译。
- `triplets/x64-linux-gcc.cmake`：`x64-linux` 加上上面那个工具链文件。名字不带版本号，以后换 GCC 的主版本不用改三元组名。
- `vcpkg-configuration.json`：里面只有一项 `overlay-triplets`，指向 `./triplets`，vcpkg 才会把我们的文件当三元组。它和 `vcpkg.json` 里的 `builtin-baseline` 同时使用没有问题：三元组找得到，版本照样按基线解析。

主版本现在写在两个地方：`check-env.sh` 里的 `GCC_MAJOR` 和工具链文件里的 `COMFYSTUDY_GCC_MAJOR`。让脚本去读工具链文件可以只剩一个来源，但 `check-env.sh` 要能单独运行，不该依赖仓库里的文件布局。所以保留两处，加一个测试断言它们一致。

### 2.5 清单模式和重复安装

在项目目录运行 `vcpkg install`，vcpkg 读 `vcpkg.json`，把库装到项目下的 `vcpkg_installed/`（已经在 `.gitignore` 里）。装完再运行一次，它发现一切已经装好，不重新编译：第二次运行总共用了 0.67 秒，输出是 `The following packages are already installed`。

### 2.6 `check-env.sh` 新检查的依据

- `curl`、`zip`、`unzip`、`tar`：vcpkg 自举和下载解压源码包要用。
- `make` 和 `perl`：openssl 的端口在 Linux 上是这样构建的。端口文件 `ports/openssl/unix/portfile.cmake` 找不到 `make` 会直接报致命错误，还要求 Perl 能加载 `IPC::Cmd` 模块。curl 在 Linux 上用 OpenSSL（Windows 上用系统自带的 SSPI），所以这条依赖只在 Linux 上存在。
- `vcpkg`：从 `VCPKG_ROOT` 找，因为它通常不在 `PATH` 里。

## 3. 本章改动

本章的代码在支线分支 `quest/platform-01-linux-foundation` 上，不合入 main，也不打 tag。提交信息以 `chapter-platform-1.2-vcpkg-deps:` 开头，`git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.2"` 可以找到本章的提交。

1. 新增 `cmake/toolchains/linux-gcc.cmake`、`triplets/x64-linux-gcc.cmake`、`vcpkg-configuration.json`。
2. `scripts/check-env.sh`：新增 `curl`、`zip`、`unzip`、`tar`、`make`、`perl` 和 `vcpkg` 的检查，缺 vcpkg 时的安装建议。
3. `tests/scripts/test-check-env.sh`：从 11 个用例增加到 21 个。
4. 分支的 `README.md`：加上本章，更新构建与运行。

`vcpkg.json` 没有改：依赖和基线与主线 0.5 完全一样。

## 4. 关键代码讲解

### 4.1 新增的检查

<!-- src: scripts/check-env.sh#L115-L134 -->
```bash
# What vcpkg needs to bootstrap itself and to fetch and unpack the sources of the libraries.
describe_first_line() { first_line "$("$1" --version 2>&1)"; }
describe_path() { echo "$2"; }
# curl --version lists every library it was built with on one line; keep the part before the first " (".
describe_curl() { local line; line="$(first_line "$("$1" --version 2>&1)")"; echo "${line%% (*}"; }
check_tool curl curl describe_curl
check_tool zip zip describe_path
check_tool unzip unzip describe_path
check_tool tar tar describe_first_line

# The openssl port (a dependency of curl on Linux) is built with make and a Perl that can load IPC::Cmd.
describe_perl() {
  if ! "$1" -e 'use IPC::Cmd;' > /dev/null 2>&1; then
    echo "FAIL: the Perl module IPC::Cmd cannot be loaded (the openssl port needs it)"
    return
  fi
  echo "$2"
}
check_tool make make describe_first_line
check_tool perl perl describe_perl
```

这几个只需要“在、且能报出版本或路径”的工具沿用 `check_tool`。`curl --version` 会把它链接的所有库列在一行里，很长，所以只留第一个 ` (` 之前的部分。`perl` 要多做一件事：试着加载 `IPC::Cmd`，加载不了就报 `FAIL`，让这条依赖在这里就暴露，而不是等到 vcpkg 编 openssl 时才失败。

<!-- src: scripts/check-env.sh#L136-L161 -->
```bash
# vcpkg lives in $VCPKG_ROOT and is usually not on PATH.
vcpkg_missing=0
check_vcpkg() {
  local program version
  if [ -z "${VCPKG_ROOT:-}" ]; then
    echo "MISSING vcpkg: VCPKG_ROOT is not set"
    missing+=(vcpkg)
    vcpkg_missing=1
    return
  fi
  program="$VCPKG_ROOT/vcpkg"
  if [ ! -x "$program" ]; then
    echo "MISSING vcpkg: no executable at $program"
    missing+=(vcpkg)
    vcpkg_missing=1
    return
  fi
  if ! version="$("$program" version 2>&1)"; then
    echo "MISSING vcpkg: cannot run $program"
    missing+=(vcpkg)
    vcpkg_missing=1
    return
  fi
  printf 'OK      vcpkg: %s\n' "$(first_line "$version")"
}
check_vcpkg
```

vcpkg 不在 `PATH` 里，所以不能用 `check_tool`。三种失败各有各的原因文字：`VCPKG_ROOT` 没设；设了但那里没有可执行的 `vcpkg`；有但运行不了。`vcpkg_missing` 记下“缺的是 vcpkg”，结尾要用它决定打印哪种建议。

### 4.2 建议里带上基线

<!-- src: scripts/check-env.sh#L258-L280 -->
```bash
if [ "${#missing[@]}" -gt 0 ]; then
  names="${missing[*]}"
  echo "check-env: FAILED, missing: ${names// /, }"
  if [ "${#packages[@]}" -gt 0 ]; then
    echo "install: sudo apt install ${packages[*]}"
  fi
  if [ "$vcpkg_missing" -eq 1 ]; then
    # Check vcpkg out at the baseline of the project's vcpkg.json, so its scripts and the ports are from the same time.
    script_dir="${BASH_SOURCE[0]%/*}"
    [ "$script_dir" = "${BASH_SOURCE[0]}" ] && script_dir=.
    checkout=""
    if [ -r "$script_dir/../vcpkg.json" ]; then
      manifest="$(<"$script_dir/../vcpkg.json")"
      if [[ "$manifest" =~ \"builtin-baseline\"[[:space:]]*:[[:space:]]*\"([0-9a-f]{40})\" ]]; then
        checkout=" && git -C ~/vcpkg checkout ${BASH_REMATCH[1]}"
      fi
    fi
    echo "install: git clone https://github.com/microsoft/vcpkg ~/vcpkg${checkout} && ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics"
  fi
  exit 1
fi
echo "check-env: OK"
exit 0
```

缺 vcpkg 时打印的不是 `apt install`，而是克隆、检出、自举的命令。检出的提交号不写死在脚本里，而是从项目的 `vcpkg.json` 里读 `builtin-baseline`，读不到就省略这一段。这样清单里的基线改了，建议也跟着变。`apt` 的建议和 vcpkg 的建议各占一行，同时缺的话两行都出现。

### 4.3 固定编译器的三个文件

<!-- src: cmake/toolchains/linux-gcc.cmake#L1-L9 -->
```cmake
# One pinned GCC for everything on Linux. vcpkg chain-loads this file (triplets/x64-linux-gcc.cmake) to build the
# libraries, and the Linux CMake presets will use it for the project itself, so both are built by the same compiler.
#
# The major version must be the same as GCC_MAJOR in scripts/check-env.sh; tests/scripts/test-check-env.sh checks that.
# nvcc (chapter platform-1.5) must accept this major version as its host compiler.
set(COMFYSTUDY_GCC_MAJOR 16)

set(CMAKE_C_COMPILER gcc-${COMFYSTUDY_GCC_MAJOR})
set(CMAKE_CXX_COMPILER g++-${COMFYSTUDY_GCC_MAJOR})
```

<!-- src: triplets/x64-linux-gcc.cmake#L1-L9 -->
```cmake
# x64-linux (static libraries, the system's dynamic C and C++ runtime) with the compiler pinned by
# cmake/toolchains/linux-gcc.cmake. vcpkg ignores the CC and CXX environment variables, so a chain-loaded
# toolchain file is how every port gets built with the same GCC.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../cmake/toolchains/linux-gcc.cmake")
```

<!-- src: vcpkg-configuration.json#L1-L3 -->
```json
{
  "overlay-triplets": [ "./triplets" ]
}
```

工具链文件只做一件事：把两个编译器设成带版本号的名字。三元组文件是 `x64-linux` 的写法再加一行链式加载。第三个文件只告诉 vcpkg 去哪里找自定义三元组。

### 4.4 测试：假的 vcpkg 和两处主版本一致

<!-- src: tests/scripts/test-check-env.sh#L48-L62 -->
```bash
  # vcpkg is not on PATH: it lives in $VCPKG_ROOT, which run_check points at this directory by default.
  mkdir "$dir/vcpkg-root"
  make_fake "$dir/vcpkg-root" vcpkg 'echo "vcpkg package management program version 2026-11-09-fake"'
  echo "$dir"
}

# run_check DIR [ARGS...]: runs the script with only DIR on PATH; sets $out and $rc.
# VCPKG_ROOT is DIR/vcpkg-root unless the case sets vcpkg_root_case (an empty value means "not set").
run_check() {
  local dir="$1"
  shift
  out="$(PATH="$dir" VCPKG_ROOT="${vcpkg_root_case-$dir/vcpkg-root}" "$bash_bin" "$script" "$@" 2>&1)"
  rc=$?
  unset vcpkg_root_case
}
```

vcpkg 也要用假的：在临时目录里放一个会回答 `version` 的 `vcpkg`，`VCPKG_ROOT` 默认指向它；用例里设置 `vcpkg_root_case` 就可以把它改成“没设置”（空值）或“指向没有 vcpkg 的目录”。

<!-- src: tests/scripts/test-check-env.sh#L189-L201 -->
```bash
# The compiler is pinned in two places; they must name the same major version.
script_text="$(<"$script")"
toolchain_text="$(<"$here/../../cmake/toolchains/linux-gcc.cmake")"
script_major="" toolchain_major=""
[[ "$script_text" =~ GCC_MAJOR=([0-9]+) ]] && script_major="${BASH_REMATCH[1]}"
[[ "$toolchain_text" =~ COMFYSTUDY_GCC_MAJOR[[:space:]]+([0-9]+) ]] && toolchain_major="${BASH_REMATCH[1]}"
if [ -n "$script_major" ] && [ "$script_major" = "$toolchain_major" ]; then
  passed=$((passed + 1))
  echo "PASS check-env.sh and the toolchain file pin the same GCC major version ($script_major)"
else
  failed=$((failed + 1))
  echo "FAIL check-env.sh pins '$script_major' but cmake/toolchains/linux-gcc.cmake pins '$toolchain_major'"
fi
```

这个用例不测脚本的行为，只测两个文件的一致：从脚本里取 `GCC_MAJOR`，从工具链文件里取 `COMFYSTUDY_GCC_MAJOR`，两者必须相同。以后有人改了一处忘了另一处，这个用例会失败。

## 5. 构建与运行

按 `check-env.sh` 提示装好 vcpkg 并设置 `VCPKG_ROOT` 之后：

```bash
git switch quest/platform-01-linux-foundation
scripts/check-env.sh
$VCPKG_ROOT/vcpkg install --triplet x64-linux-gcc
```

第一次在新机器上，先用 `--no-smoke` 看缺什么，不到一秒：

```bash
scripts/check-env.sh --no-smoke
```

脚本自己的测试：

```bash
bash tests/scripts/test-check-env.sh
```

```text
check-env tests: 21 passed, 0 failed
```

## 6. 验证

- `bash tests/scripts/test-check-env.sh`：21 个用例全部通过，在 Windows 的 Git Bash 里和 Debian sid 的实体机上都是。新增的 10 个用例：缺 `curl`；缺 `zip` 和 `unzip`；缺 `make`；缺 `perl`；`perl` 加载不了 `IPC::Cmd`；`VCPKG_ROOT` 没设置；`VCPKG_ROOT` 里没有 `vcpkg`；`vcpkg` 运行不了；缺 vcpkg 时不建议 `apt`；脚本和工具链文件的 GCC 主版本一致。
- 先看失败：没有设 `VCPKG_ROOT` 时脚本报 `MISSING vcpkg: VCPKG_ROOT is not set`，打印带基线提交号的克隆命令，退出码 1。克隆、自举、设置之后：全部 `OK`，退出码 0。
- `vcpkg install --triplet x64-linux-gcc` 在实体机（32 核）上成功，约 1.1 分钟。再运行一次 0.67 秒，没有重新编译。
- 编译器确实是 GCC 16：vcpkg 为这个三元组找到的编译器是 `/usr/bin/g++-16`；端口配置日志里 C 和 C++ 编译器都是 `GNU 16.2.0`；`libspdlog.a`、`libfmt.a`、`libz.a`、`libcurl.a`、`libssl.a`、`libcrypto.a` 的 `.comment` 段都是 `GCC: (Debian 16.2.0-5) 16.2.0`。
- 改动检验（三个）：
  - 把工具链文件里的主版本临时改成 15（机器上有 `gcc-15`，没有 `g++-15`），再运行安装：失败，vcpkg 报 `error: while detecting compiler information` 和 `vcpkg was unable to detect the active compiler's information`。说明编译器确实由三元组的链式工具链决定。改回 16 后恢复。
  - 在本地把工具链文件里的主版本改成 15，再跑测试：一致性用例失败（脚本是 16，工具链文件是 15），其余用例通过。（这一步是在加 `make` 和 `perl` 的用例之前做的，当时一共 18 个用例，17 个通过。）
  - 去掉 `perl` 的那一行检查：缺 `perl` 和 `perl` 加载不了 `IPC::Cmd` 两个用例失败。
- 版本对比（Windows 一侧是在项目目录里运行 `scripts/dev.ps1 vcpkg list --x-install-root=vcpkg_installed`，对项目里已经装好的库得到的；Linux 一侧是 `vcpkg list --x-install-root=vcpkg_installed`）：

| 端口 | Windows（x64-windows） | Linux（x64-linux-gcc） |
|---|---|---|
| cli11 | 2.7.2 | 2.7.2 |
| curl | 8.23.0 | 8.23.0 |
| fmt | 12.2.1 | 12.2.1 |
| gtest | 1.18.0 | 1.18.0 |
| nlohmann-json | 3.12.0#2 | 3.12.0#2 |
| spdlog | 1.17.0#1 | 1.17.0#1 |
| zlib | 1.4 | 1.4 |
| vcpkg-cmake | 2026-11-09 | 2026-11-09 |
| vcpkg-cmake-config | 2026-07-21 | 2026-07-21 |
| openssl | 没有 | 3.6.5#2 |
| vcpkg-cmake-get-vars | 没有 | 2025-05-29 |

  两边共有的 9 个端口，版本连端口修订号（`#` 后面的数字）都相同。只在 Linux 上出现的两个：`openssl`（curl 在 Linux 上用它，Windows 上用系统自带的 SSPI），`vcpkg-cmake-get-vars`（openssl 的端口文件会调用它，`vcpkg_cmake_get_vars`）。curl 的特性也不同：Linux 上是 `openssl` 和 `ssl`；Windows 上是 `ssl`、`sspi`，以及 `http2`。`http2` 不属于这一章的清单：Windows 一侧装的是主线当前清单，后面的章节给 curl 加了这个特性，0.5 的清单里没有。
- 没有做的：
  - 没有在一台干净的机器上验证自举：`curl`、`zip`、`unzip`、`tar`、`make`、`perl` 在这台机器上本来就有，脚本对“缺它们”的行为只用假命令测过。
  - `autoconf`、`automake`、`libtool` 这台机器上没装，这 5 个库没有它们也编完了，所以没有加进检查。以后有库需要它们再加。
  - 只试了 GCC 16，没有试 GCC 15。
  - 只检查了静态库里的编译器标记；只有头文件的库（nlohmann-json、cli11）没有编译产物可查。

## 7. 技术资料

- vcpkg：克隆自 <https://github.com/microsoft/vcpkg>，检出基线提交 `93ad765c7b66ab9ce97469ec23a90b15c3a14ee5`，自举后的工具版本 2026-10-29-139f2e5。克隆完成的目录约 205 MB；构建完 5 个库之后，`~/vcpkg`（含下载、构建目录和打包）约 885 MB。
- 三元组变量，包括 `VCPKG_CHAINLOAD_TOOLCHAIN_FILE`：<https://learn.microsoft.com/vcpkg/users/triplets>。
- 本章用到的库版本、`x64-linux-gcc` 的编译器：GCC 16.2.0（Debian 包版本 16.2.0-5）。
- 设计取舍：
  - 固定编译器的三种做法：自定义三元组加链式工具链（采用）；环境变量 `CC`、`CXX`（否决，vcpkg 构建端口时不使用）；改系统的默认编译器（否决，platform-1.1 讲过）。
  - 主版本写在两处加一致性测试，而不是让脚本读工具链文件：`check-env.sh` 要能单独运行。
  - 检出基线提交，而不是默认分支的最新提交：脚本和端口同一时间点。没有对比过用最新提交会不会出问题。
  - 三元组名字不带版本号：换主版本只改两处数字，不用改三元组名。
- 被否决的：从 Debian 的包管理器装开发库（版本和 Windows 不一致）；动态三元组 `x64-linux-dynamic`（要处理 `.so` 的搜索路径，本章不需要）。

## 8. 常见问题与坑

**vcpkg 不使用 `CC` 和 `CXX`。** 在命令行前面加 `CXX=g++-16` 对 vcpkg 构建的库没有作用，它们仍然用系统默认的编译器。固定编译器只能通过三元组里的工具链文件。

**日志里出现 `/usr/bin/c++` 不一定是错的。** 安装日志里有一行 `Detecting compiler hash for triplet x64-linux...`，找到的是 `/usr/bin/c++`。这是“宿主三元组”：`vcpkg-cmake` 这类只含 CMake 脚本的辅助端口在宿主三元组下构建，它们没有编译产物。真正的库用的是 `x64-linux-gcc`，看它那一行是不是 `g++-16`。

**这一章用的是完整克隆，没有试过浅克隆。** 基线是一个提交号，vcpkg 要在 git 历史里查它当时各个库的版本，所以这里不用 `--depth`。

**检出基线提交会让 `git status` 显示游离的 HEAD。** 这是预期的：`~/vcpkg` 是工具，不在上面开发。

**改了工具链文件，vcpkg 会重新检测编译器。** 上面的改动检验就是靠这一点失败的：把 `linux-gcc.cmake` 里的主版本改成没有安装的 15，vcpkg 在检测编译器那一步就停了。改了这个文件之后会不会重新构建所有库，本章没有试。

**`VCPKG_ROOT` 在新终端里没有。** 本章只在当前会话里设置。新开终端后 `check-env.sh` 会再次报 `VCPKG_ROOT is not set`，把 `export` 那一行写进 `~/.bashrc` 就不会了。

**在 zsh 里 `env -u VCPKG_ROOT` 和 `export -n` 的差别。** 想测“没设置”的情形，用 `env -u VCPKG_ROOT 命令`，不要用 `export -n`（zsh 的 `export` 不认这个选项）。

## 9. 练习

1. 把 `GCC_MAJOR` 和 `COMFYSTUDY_GCC_MAJOR` 都改成 15，装上 `g++-15`，运行 `vcpkg install --triplet x64-linux-gcc`。有没有哪个库编不过？比较两个主版本下 `.comment` 的结果。
2. 新建一个 `x64-linux-gcc-dynamic` 三元组（`VCPKG_LIBRARY_LINKAGE dynamic`），装一遍，看 `vcpkg_installed/` 里的内容有什么不同。链接时需要处理什么？
3. 往 `vcpkg.json` 里加一个依赖，比如 `zstd`，重新安装：哪些库重新编了，哪些没有？为什么？
4. 让 `check-env.sh` 在 `vcpkg_installed/x64-linux-gcc` 存在时，再检查一个库的 `.comment` 是不是固定的 GCC 主版本。想一想：这条检查该放在 `check-env.sh` 里，还是单独写一个脚本？

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 platform-1.3 写 `cmake/platform/linux.cmake` 和预设：C++20、`-Wall -Wextra -Werror`、`compile_commands.json`、sanitizer 选项，预设有 debug、release、asan，预设里接入这一章的 vcpkg 工具链和 `x64-linux-gcc`，用 `find_package` 找到这一章装好的库。0.5 的代码里 `config::Get` 和 `config::ExeDirectory` 在 Linux 上还是 `#error`，所以下一章还要补上能编过的最小 Linux 分支，才能做一次完整构建并跑通测试。
