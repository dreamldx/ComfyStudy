# platform-1.1 Linux 工具链与环境检查

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/platform-01-linux-foundation`（支线不打 tag，用 `git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.1"` 定位本章提交）
- 所属支线：[支线 platform-01：Linux 阶段 0](README.md)
- 可开始于：主线 [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md)
- 一句话目标：装好固定主版本的 GCC、CMake ≥ 3.25、Ninja、pkg-config、git，写 `scripts/check-env.sh`（含试编译）和它的测试

## 1. 本章目标与成果

主线阶段 0 在 Windows 上先做了环境检查：`scripts/check-env.ps1` 看 `cl`、CMake、Ninja、git、vcpkg 在不在、版本对不对，缺什么就列出来。Linux 线要有同样的第一步：拿到一台机器，跑一条命令，就知道能不能往下做。

Linux 上多出一个问题：编译器不会只有一个。同一台机器上可以同时装着好几个 GCC，`g++` 这个名字指向哪一个由系统决定，而且滚动发行的 Debian sid 会时不时把它指向新的。后面的 CUDA 要求宿主编译器在它认可的主版本范围里，所以本线把编译器固定在一个主版本上，只通过带版本号的名字使用它。

做完后你会得到：

- 一份清单：这条支线在 Debian sid 上要装哪些包，每个包检查什么，装机当天各是什么版本。
- `scripts/check-env.sh`：检查 `g++-16`、`gcc-16`、CMake ≥ 3.25、Ninja、pkg-config、git、ld，缺东西时列出名字并打印一行 `apt install` 建议（只打印，不执行），退出码 1。输出格式与 `check-env.ps1` 一致。
- 两次试编译：用固定的编译器编译并运行一个用到 concepts、`std::span` 和协程的 C++20 小程序，再带 AddressSanitizer 和 UndefinedBehaviorSanitizer 编译运行一次，证明编译器不只是“存在”。
- `tests/scripts/test-check-env.sh`：11 个用例，用假命令冒充各种缺失和版本不对的情况，不碰真实系统。

本章不装第三方库（下一章 platform-1.2 用 vcpkg 装），也不写构建文件（platform-1.3）。

先看效果：在一台还没装 CMake 和 Ninja 的机器上跑一次，再按提示装好，再跑一次。

```bash
scripts/check-env.sh
```

```text
OK      g++-16: 16.2.0
OK      gcc-16: 16.2.0
MISSING cmake
MISSING ninja
OK      pkg-config: version 2.5.1
OK      git: git version 2.55.0
OK      ld: GNU ld (GNU Binutils for Debian) 2.47.50.20261120
INFO    glibc: ldd (Debian GLIBC 2.43-7) 2.43
OK      smoke: g++-16 compiled and ran a C++20 program (concepts, std::span, coroutine)
OK      smoke: AddressSanitizer and UndefinedBehaviorSanitizer linked and ran
check-env: FAILED, missing: cmake, ninja
install: sudo apt install cmake ninja-build
```

缺的两样被点了名，最后一行是可以直接复制的安装命令，退出码是 1。编译器本身没问题，所以两次试编译照常通过。装上之后再跑：

```bash
sudo apt install cmake ninja-build
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
INFO    glibc: ldd (Debian GLIBC 2.43-7) 2.43
OK      smoke: g++-16 compiled and ran a C++20 program (concepts, std::span, coroutine)
OK      smoke: AddressSanitizer and UndefinedBehaviorSanitizer linked and ran
check-env: OK
```

这一次全部通过，退出码是 0。以上都是 2026-11-24 在 Debian sid 的实体机（x86_64，带 NVIDIA 显卡）上运行的结果。

## 2. 背景与概念

### 2.1 为什么固定编译器，又为什么不用 `g++`

三件事让“固定”变得必要：

- **同一台机器上有好几个 GCC。** Debian 的 `g++-11` 到 `g++-16` 是各自独立的包，可以同时装。装机当天这台机器上，`g++` 是 16，`gcc-14`、`gcc-15`、`gcc-16` 也都在。
- **`g++` 是一个别名。** 它指向哪个版本由系统的默认设置决定。sid 把默认值升到新版本时，你什么都没改，项目就换了编译器。
- **nvcc 只接受它认可的宿主编译器。** nvcc 按宿主编译器的主版本做检查，超出范围就拒绝。NVIDIA 的资料显示 CUDA 13.0 起支持 GCC 15（[CUDA 13.0 的介绍](https://developer.nvidia.com/blog/whats-new-and-important-in-cuda-toolkit-13-0/)），13.4 起支持 GCC 16（[CUDA 特性归档](https://docs.nvidia.com/cuda/cuda-features-archive/index.html)）。官方安装指南里的宿主编译器表我没有直接查到上限，这一点留到 platform-1.6 装 CUDA 时核对。

所以本线的做法是：脚本顶部写死主版本 `GCC_MAJOR=16`，只认 `g++-16` 和 `gcc-16`，从不看 `g++`。后面的 CMake 预设也会写 `g++-16`。将来要换版本，改这一个数字和包名，别处不动。

不用 `update-alternatives` 把 `g++` 指过去：那会改整个系统的默认，影响机器上别的项目，而且脚本通过靠的是改系统，不是编译器本身。

选 16 的理由很直接：它是 sid 里最高的版本，也是这台机器已经装好的默认编译器，同时在 CUDA 13.4 之后的支持范围内。如果 platform-1.6 装的 CUDA 低于 13.4，就改成 15，多装一个 `g++-15`。GCC 16 很新，第三方库有可能出警告，这会在 platform-1.3 的 `-Werror` 和 platform-1.2 的 vcpkg 构建里暴露，到时退一档即可，只改这一处。

### 2.2 滚动发行与“装机当天”

Debian sid 每天都在变。`apt install g++-16` 装到的是当天仓库里的 16.x 的最新小版本，不同的人在不同的日子装，小版本不一定一样。本章不用日期快照把它钉死，那是 platform-1.7 的 CI 容器要做的事。这里只做两件事：只认主版本，并记下装机当天的版本，让读者知道这一套是哪一天验证过的（2026-11-24）：

| 工具 | 包 | 版本 |
|---|---|---|
| `g++-16` | `g++-16` | 16.2.0-5 |
| `gcc-16` | `gcc-16` | 16.2.0-5 |
| `cmake` | `cmake` | 4.4.5-1 |
| `ninja` | `ninja-build` | 1.13.2-1 |
| `pkg-config` | `pkg-config`（由 `pkgconf` 提供实际程序） | 2.5.1-4 |
| `git` | `git` | 1:2.55.0-1 |
| `ld` | `binutils` | 2.47.50.20261120-1 |

glibc 是 2.43。它不是检查项，只是报告出来：后面预编译的 libtorch 和 ONNX Runtime 对 glibc 有下限，早点看到有好处。

### 2.3 工具“在”和“能用”是两回事

`command -v g++-16` 能找到，不等于它能编出能跑的 C++20 程序，更不等于带上 sanitizer 也行。所以脚本在检查完各个工具之后，做两次很小的试编译：

1. 编译并运行一个用到概念约束、`std::span` 和一个最小协程的程序，用 `-std=c++20 -Wall -Wextra -Werror`。程序最后检查结果，不对就返回非零，所以“编译通过但跑错了”也会被发现。协程用到的是本项目 Asio 部分要用的那一套语言特性。
2. 同一个程序加 `-fsanitize=address,undefined` 再编译、再运行。后面 platform-1.4 要在这两个 sanitizer 下跑整个测试集，现在先确认它们能用。

### 2.4 脚本的约定

- **只检查，不改系统。** 脚本里没有 `sudo`、`apt`。缺东西时打印一行建议，由你决定要不要照做。
- **查完再汇总。** 不用 `set -e`，缺第一样东西时不退出，继续检查剩下的，最后一起报告，与 `check-env.ps1` 一致。
- **退出码。** 全部通过是 0，有缺失是 1，命令行参数不认识是 2。
- **`--no-smoke`。** 只检查工具，不做试编译，大约 0.03 秒。

## 3. 本章改动

本章的代码在支线分支 `quest/platform-01-linux-foundation` 上，不合入 main，也不打 tag。提交信息以 `chapter-platform-1.1-toolchain-check:` 开头，`git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.1"` 可以找到本章的提交。

1. `scripts/check-env.sh`：环境检查脚本。
2. `tests/scripts/test-check-env.sh`：脚本的测试。
3. 分支的 `README.md`：换成这条支线自己的说明，列出到本章为止实现了什么，并链接到本章。

## 4. 关键代码讲解

### 4.1 固定的版本和检查一个工具

<!-- src: scripts/check-env.sh#L9-L14 -->
```bash
# The compiler is pinned by its major version and always called by its versioned name (g++-N), never
# through the g++ alias, so a distribution that moves its default compiler does not change what we use.
# nvcc (chapter platform-1.6) must accept this major version as its host compiler.
GCC_MAJOR=16
CMAKE_MIN_MAJOR=3
CMAKE_MIN_MINOR=25
```

三个固定值放在最前面。注释里写明为什么固定、固定给谁用（nvcc），以后换版本的人先看到这一段。

<!-- src: scripts/check-env.sh#L44-L63 -->
```bash
# check_tool NAME PACKAGE DESCRIBE: DESCRIBE prints one line of detail for the tool found at the given path;
# a line starting with "FAIL:" means the tool is there but not acceptable.
check_tool() {
  local name="$1" package="$2" describe="$3" path text
  path="$(command -v "$name" 2>/dev/null)"
  if [ -z "$path" ]; then
    printf 'MISSING %s\n' "$name"
    missing+=("$name")
    add_package "$package"
    return
  fi
  text="$("$describe" "$name" "$path")"
  if [[ "$text" == FAIL:* ]]; then
    printf 'MISSING %s: %s\n' "$name" "${text#FAIL: }"
    missing+=("$name")
    add_package "$package"
    return
  fi
  printf 'OK      %s: %s\n' "$name" "$text"
}
```

`check_tool` 只管流程：`command -v` 找不到就报 `MISSING`；找到了就交给“描述函数”去看版本，函数输出以 `FAIL:` 开头表示这个工具在但不合格。每次失败都记下工具名和建议的包名。各个工具只需要各自写一个描述函数，流程不用动。

### 4.2 编译器和 CMake 的版本判断

<!-- src: scripts/check-env.sh#L65-L83 -->
```bash
describe_compiler() {
  local full machine major
  full="$("$1" -dumpfullversion 2>/dev/null)"
  major="${full%%.*}"
  if [[ ! "$major" =~ ^[0-9]+$ ]]; then
    echo "FAIL: cannot read the compiler version"
    return
  fi
  if [ "$major" -ne "$GCC_MAJOR" ]; then
    echo "FAIL: major version $major, need $GCC_MAJOR"
    return
  fi
  machine="$("$1" -dumpmachine 2>/dev/null)"
  if [[ "$machine" != x86_64-* ]]; then
    echo "FAIL: target is not x86_64 ($machine)"
    return
  fi
  echo "$full"
}
```

版本从 `-dumpfullversion` 取，按第一个点拆出主版本，用整数比较，不去解析 `--version` 的那一行文字。后面再确认目标是 `x86_64-*`，对应 Windows 线对 x64 的要求。

<!-- src: scripts/check-env.sh#L85-L98 -->
```bash
describe_cmake() {
  local line
  line="$(first_line "$("$1" --version 2>&1)")"
  if [[ ! "$line" =~ cmake\ version\ ([0-9]+)\.([0-9]+) ]]; then
    echo "FAIL: cannot read the CMake version ($line)"
    return
  fi
  local major="${BASH_REMATCH[1]}" minor="${BASH_REMATCH[2]}"
  if [ "$major" -lt "$CMAKE_MIN_MAJOR" ] || { [ "$major" -eq "$CMAKE_MIN_MAJOR" ] && [ "$minor" -lt "$CMAKE_MIN_MINOR" ]; }; then
    echo "FAIL: CMake $CMAKE_MIN_MAJOR.$CMAKE_MIN_MINOR or newer is required ($line)"
    return
  fi
  echo "$line"
}
```

CMake 的下限是 3.25，主次版本分开比，4.x 自然通过。注意这里只设下限：这台机器上的 CMake 是 4.4.5，比 3.25 新得多，项目的 `cmake_minimum_required` 在新版本下是否照常工作，要到 platform-1.3 配置时才能验证。

### 4.3 试编译

<!-- src: scripts/check-env.sh#L172-L183 -->
```bash
  if ! output="$("$cxx" -std=c++20 -Wall -Wextra -Werror "$dir/smoke.cpp" -o "$dir/smoke" 2>&1)"; then
    printf 'MISSING smoke: %s cannot compile a C++20 program with concepts, std::span and coroutines\n' "$cxx"
    printf '%s\n' "$output" | head -n 5 | sed 's/^/        /'
    missing+=(smoke)
    return
  fi
  if ! output="$("$dir/smoke" 2>&1)"; then
    printf 'MISSING smoke: the compiled C++20 program gave a wrong result (%s)\n' "$output"
    missing+=(smoke)
    return
  fi
  printf 'OK      smoke: %s compiled and ran a C++20 program (concepts, std::span, coroutine)\n' "$cxx"
```

编译用 `-Wall -Wextra -Werror`，所以一个警告也会让它失败。失败时把编译输出的前几行缩进后一起打印，读者能直接看到原因。运行结果不对也单独报告。

<!-- src: scripts/check-env.sh#L185-L197 -->
```bash
  if ! output="$("$cxx" -std=c++20 -fsanitize=address,undefined -g "$dir/smoke.cpp" -o "$dir/smoke_san" 2>&1)"; then
    printf 'MISSING smoke: %s cannot link AddressSanitizer and UndefinedBehaviorSanitizer\n' "$cxx"
    printf '%s\n' "$output" | head -n 5 | sed 's/^/        /'
    missing+=(smoke)
    return
  fi
  if ! output="$("$dir/smoke_san" 2>&1)"; then
    printf 'MISSING smoke: the sanitizer build of the program failed to run\n'
    printf '%s\n' "$output" | head -n 5 | sed 's/^/        /'
    missing+=(smoke)
    return
  fi
  printf 'OK      smoke: AddressSanitizer and UndefinedBehaviorSanitizer linked and ran\n'
```

带 sanitizer 的这一次不加 `-Werror`，也不再检查结果，只看能不能编、能不能跑起来。

### 4.4 汇总、安装建议和退出码

<!-- src: scripts/check-env.sh#L207-L217 -->
```bash

if [ "${#missing[@]}" -gt 0 ]; then
  names="${missing[*]}"
  echo "check-env: FAILED, missing: ${names// /, }"
  if [ "${#packages[@]}" -gt 0 ]; then
    echo "install: sudo apt install ${packages[*]}"
  fi
  exit 1
fi
echo "check-env: OK"
exit 0
```

缺失项用逗号连起来，建议里每个包只出现一次，所以同时缺 `gcc-16` 和 `ninja` 时是 `install: sudo apt install gcc-16 ninja-build`。

### 4.5 测试：用假命令，且只给 PATH 一个目录

<!-- src: tests/scripts/test-check-env.sh#L17-L26 -->
```bash
# make_fake DIR NAME BODY: an executable called NAME in DIR whose body is a /bin/sh script.
make_fake() {
  printf '#!/bin/sh\n%s\n' "$3" > "$1/$2"
  chmod +x "$1/$2"
}

# make_gcc DIR NAME VERSION MACHINE: a fake compiler that answers -dumpfullversion and -dumpmachine.
make_gcc() {
  make_fake "$1" "$2" "case \"\$1\" in -dumpfullversion) echo $3;; -dumpmachine) echo $4;; esac"
}
```

每个用例建一个临时目录，在里面放一组假命令（一个小的 `/bin/sh` 脚本，只回答 `-dumpfullversion`、`-dumpmachine`、`--version`），然后：

<!-- src: tests/scripts/test-check-env.sh#L45-L51 -->
```bash
# run_check DIR [ARGS...]: runs the script with only DIR on PATH; sets $out and $rc.
run_check() {
  local dir="$1"
  shift
  out="$(PATH="$dir" "$bash_bin" "$script" "$@" 2>&1)"
  rc=$?
}
```

关键是 `PATH="$dir"`：被测脚本运行时，`PATH` 里只有这个目录，真正的编译器根本找不到，所以“缺 `g++-16`”这样的用例才能成立。这也是脚本里尽量只用 bash 内建功能的原因，比如取第一行用的是参数展开，不依赖 `head`。试编译需要真的编译器，不在假命令下测，由真机运行覆盖。

用例里有一个专门检查“不看 `g++` 别名”：假的 `g++` 和 `gcc` 报 99.0.0，而输出里不能出现 `99.0.0`。

## 5. 构建与运行

本章没有要编译的东西。在 Debian sid 上，按提示装好缺的包，然后：

```bash
git switch quest/platform-01-linux-foundation
scripts/check-env.sh
```

第一次在新机器上，先用 `--no-smoke` 看缺什么（不到一秒），装完再不带参数跑一遍：

```bash
scripts/check-env.sh --no-smoke
sudo apt install cmake ninja-build
scripts/check-env.sh
```

脚本自己的测试：

```bash
bash tests/scripts/test-check-env.sh
```

```text
PASS everything present
PASS an install hint is not printed when all is well
PASS the unversioned g++ alias is not looked at
PASS g++-16 missing
PASS g++-16 reports another major version
PASS compiler target is not x86_64
PASS cmake too old
PASS cmake version unreadable
PASS several tools missing, packages listed once each
PASS ld missing maps to binutils
PASS unknown option

check-env tests: 11 passed, 0 failed
```

## 6. 验证

- `bash tests/scripts/test-check-env.sh`：11 个用例全部通过：全部正常；缺 `g++-16`；编译器报别的主版本；目标不是 x86_64；CMake 3.24；CMake 版本读不出来；同时缺几样时包名只列一次；缺 `ld` 对应 `binutils`；不认识的参数。
- 在实体机上先看失败：`cmake`、`ninja` 都没装，脚本报这两样缺失、给出安装命令、退出码 1，试编译两步仍然通过。装完再跑：全部 `OK`，退出码 0。
- 改动检验：把 `GCC_MAJOR` 临时改成 15（机器上有 `gcc-15` 但没有 `g++-15`），脚本报 `g++-15` 缺失；同一份改动下测试有 8 个失败。证明固定的版本号确实在起作用，测试也确实在检查它。改回 16 之后 11 个全过。
- `--no-smoke`：运行一次约 0.03 秒。
- 没有做的：
  - 没有在干净的容器里验证安装清单。清单是否完整，只靠这台机器上“装完 `cmake` 和 `ninja-build` 之后全部通过”来判断，而这台机器本来就装了编译器和 git。容器验证留给 platform-1.7。
  - 没有验证 AddressSanitizer 附带的泄漏检测（LeakSanitizer）在容器里的行为，见第 8 节。
  - 没有验证 CUDA 对 GCC 16 的支持上限，留给 platform-1.6。

## 7. 技术资料

- 环境：Debian GNU/Linux forky/sid，x86_64，glibc 2.43，2026-11-24。GCC 16.2.0（Debian 包版本 16.2.0-5）。包版本见 2.2 节的表。
- GCC 的 `-dumpfullversion`（完整版本号，如 `16.2.0`）和 `-dumpmachine`（目标三元组，如 `x86_64-linux-gnu`）：[GCC 文档里的 Developer Options](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html)。
- CUDA 对 GCC 的支持：见 2.1 节的两个链接。nvcc 只检查宿主编译器的主版本。
- 设计取舍：
  - 固定编译器的三种做法：脚本只认 `g++-N`（采用）；用 `update-alternatives` 改系统默认（否决，改了整个系统，通过检查靠的是改系统）；用 snapshot.debian.org 的日期快照装包（本章不做，步骤太多，留给 CI 容器）。
  - 为什么用整数比较主次版本而不是 `sort -V`：与 `check-env.ps1` 的写法一致，行为一样，而且好测。
  - 为什么用 `PATH` 只放一个目录来测试：这样“缺工具”的用例才能真的构造出来，而不是依赖这台机器恰好缺了什么。
- 被否决的：在脚本里自动执行 `apt install`（脚本不应该悄悄改系统）；识别发行版并给出不同的安装命令（本线只做 Debian sid）。

## 8. 常见问题与坑

**输出给人看，退出码给脚本看。** 输出里的 `OK`、`MISSING`、`INFO` 前缀对齐成八个字符，方便肉眼扫一遍；退出码 0 和 1 给后面的脚本（比如 platform-1.7 的 `verify.sh`）用。

**`set -u` 加空数组。** bash 里展开一个空数组，在 `set -u` 下会报“未绑定的变量”。脚本里用 `"${packages[@]+"${packages[@]}"}"` 的写法：数组不存在或为空时展开成空。

**`local x="$(命令)"` 会吞掉命令的退出码。** `local` 自己的退出码是 0。脚本里需要判断命令成败的地方，先 `local x`，再 `x="$(命令)"`。

**测试里的假命令必须用 `#!/bin/sh`。** 测试把 `PATH` 限制成一个目录，`#!/usr/bin/env bash` 这样的开头会因为 `PATH` 里找不到 `bash` 而失败。用绝对路径 `/bin/sh` 就不依赖 `PATH`。

**`g++` 也许碰巧就是 16。** 这台机器上默认的 `g++` 正好是 16，所以光看真机的运行结果，分不出脚本是看了 `g++-16` 还是 `g++`。所以“不看别名”这件事用假命令测（别名报 99.0.0）。

**从 Windows 来的文件。** 仓库的 `.gitattributes` 里有 `* text=auto eol=lf`，`.sh` 在 Windows 上检出时保持 LF 换行，放到 Linux 上直接能跑。但 Windows 上没有“可执行”权限位，把工作目录里的文件直接拷过去会丢失它。通过 git 推送或克隆，权限位（模式 100755）会保留；拷贝过去的话用 `bash scripts/check-env.sh` 运行。

**容器里的泄漏检测。** AddressSanitizer 自带泄漏检测（LeakSanitizer），本章没有在容器里验证过它。如果在容器里试编译的第二步失败，输出里出现 LeakSanitizer 的错误，可以试试给容器加 `--cap-add=SYS_PTRACE`，或者运行时设 `ASAN_OPTIONS=detect_leaks=0`。platform-1.7 用到容器时会实测。

## 9. 练习

1. 给脚本加一个 `--quiet` 选项：全部通过时只打印最后一行 `check-env: OK`，有缺失时照常打印全部。先写测试，再改脚本。
2. 让 `GCC_MAJOR` 可以被环境变量覆盖：`GCC_MAJOR=15 scripts/check-env.sh` 检查 `g++-15`。先写测试，再改脚本。想一想：这会不会破坏“固定”的意思？
3. 把 `ldd --version` 那行报告改成读 `getconf GNU_LIBC_VERSION`，两者在什么情况下结果会不同？
4. 故意让试编译失败：把 `smoke.cpp` 里的一个括号删掉，运行脚本，看错误输出是否足够让人定位问题。你会怎么改进“失败时打印前五行”这个做法？

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 platform-1.2 在 Linux 上装 vcpkg：先把 vcpkg 自举要用的工具（git、curl、zip、unzip、tar）和 vcpkg 本身加进 `check-env.sh`，再让 vcpkg 用这一章固定下来的 `g++-16` 把主线清单里的几个库装出来，并和 Windows 上解析出的版本对比。构建文件（`cmake/platform/linux.cmake` 和预设）放在再下一章 platform-1.3。
