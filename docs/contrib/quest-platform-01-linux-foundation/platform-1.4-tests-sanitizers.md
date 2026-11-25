# platform-1.4 测试与 Sanitizer

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/platform-01-linux-foundation`（支线不打 tag，用 `git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.4"` 定位本章提交）
- 所属支线：[支线 platform-01：Linux 阶段 0](README.md)
- 可开始于：主线 [0.6 检测 GPU 与 CUDA 驱动](../../tutorial/phase-00-toolchain/0.6-gpu-check.md)
- 一句话目标：证明 `linux-asan` 下的测试通过是有意义的：写一个“金丝雀”程序，故意触发 11 种错误（堆越界、释放后使用、栈越界、返回后使用栈变量、全局越界、重复释放、泄漏、整数溢出、移位过大、除以零、空指针），每种错误一个测试，只有 sanitizer 报告了才通过；在 `linux-asan` 测试预设里设 `UBSAN_OPTIONS` 和 `ASAN_OPTIONS`；讲清怎么读 ASan、UBSan 和 LeakSanitizer 的报告

## 1. 本章目标与成果

platform-1.3 里已经顺手跑过一次 `ctest --preset linux-asan`，42 个测试都通过了。问题在于，“通过”说明不了什么：代码没有问题，和 sanitizer 根本没生效（选项没传到，或者被环境变量关掉了，或者泄漏检测没开），结果看起来一模一样。本章要解决的就是这个：证明 sanitizer 真的能抓到错误，并且学会读它的报告。

做法是“金丝雀”：一个故意写错的小程序，每次运行触发一种真实的错误；每种错误对应一个测试，只有 sanitizer 报告了这个错误，测试才算通过。如果哪天 sanitizer 没生效，对应的金丝雀会失败。

做完后你会得到：

- `tests/sanitizer_canaries.cpp`：按参数触发 11 种错误：堆越界、释放后使用、栈越界、返回后使用栈变量、全局数组越界、重复释放、内存泄漏，以及有符号整数溢出、移位过大、除以零、解引用空指针。
- `cmake/platform/linux.cmake` 里的登记：缓存变量 `COMFYSTUDY_CANARIES`（开了 sanitizer 就默认打开），每种错误一个测试 `canary.<名字>`，带标签 `canary`。只开 `address` 登记 7 个，只开 `undefined` 登记 4 个。
- `linux-asan` 测试预设里的环境变量：`UBSAN_OPTIONS=print_stacktrace=1`（UBSan 的报告带调用栈）和 `ASAN_OPTIONS=detect_leaks=1`（显式打开泄漏检测）。
- `linux-asan` 现在是 53 个测试（42 个原有，11 个金丝雀），全部通过；`linux-debug` 和 `linux-release` 仍是 42 个，没有金丝雀。
- 怎么读 sanitizer 的三种报告：AddressSanitizer、UndefinedBehaviorSanitizer 和 LeakSanitizer。
- 一次真实的演示：一个有 bug 的 GoogleTest 测试，在 `linux-debug` 下悄悄通过，在 `linux-asan` 下失败并给出完整的报告。

先看效果：先看没有金丝雀时的状态，然后在“没有 sanitizer”的构建里手动打开金丝雀，它们应该全部失败。

```bash
ctest --preset linux-asan -N
```

```text
Total Tests: 42
```

这是改动之前 `linux-asan` 的测试数，全部通过，但没有任何一个能证明 sanitizer 在工作。加上金丝雀之后，在一个不带 sanitizer 的构建目录里手动打开它们：

```bash
cmake --preset linux-debug -B build/linux-canary-off -DCOMFYSTUDY_CANARIES=ON
cmake --build build/linux-canary-off
ctest --test-dir build/linux-canary-off -L canary
```

```text
 1/11 Test #43: canary.heap-overflow .............***Failed  Required regular expression not found. Regex=[AddressSanitizer: heap-buffer-overflow
 2/11 Test #44: canary.heap-use-after-free .......***Failed  Required regular expression not found. Regex=[AddressSanitizer: heap-use-after-free
 3/11 Test #45: canary.stack-overflow ............***Failed  Required regular expression not found. Regex=[AddressSanitizer: stack-buffer-overflow
 4/11 Test #46: canary.stack-use-after-return ....***Failed  Required regular expression not found. Regex=[AddressSanitizer: stack-use-after-return
 5/11 Test #47: canary.global-overflow ...........***Failed  Required regular expression not found. Regex=[AddressSanitizer: global-buffer-overflow
 6/11 Test #48: canary.double-free ...............Subprocess aborted***Exception:
 7/11 Test #49: canary.leak ......................***Failed  Required regular expression not found. Regex=[LeakSanitizer: detected memory leaks
 8/11 Test #50: canary.signed-overflow ...........***Failed  Required regular expression not found. Regex=[runtime error: signed integer overflow
 9/11 Test #51: canary.shift .....................***Failed  Required regular expression not found. Regex=[runtime error: shift exponent
10/11 Test #52: canary.div-zero ..................***Exception: Numerical
11/11 Test #53: canary.null ......................***Exception: SegFault
0% tests passed, 11 tests failed out of 11
```

（CTest 把要求的正则的结尾括号另起一行打印，这里每条只摘了第一行。）11 个全部失败：有的是“找不到要求的 sanitizer 报告”，有的程序干脆崩溃了（重复释放被 glibc 自己发现并中止，除以零是 `SIGFPE`，空指针是段错误），但没有一个出现 sanitizer 的那句话，所以都不算通过。这就是金丝雀的用处：sanitizer 没生效时，它们会变红。现在用 `linux-asan` 预设：

```bash
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
ctest --preset linux-asan -L canary
```

```text
100% tests passed out of 53
```

```text
 1/11 Test #43: canary.heap-overflow .............   Passed
 2/11 Test #44: canary.heap-use-after-free .......   Passed
 3/11 Test #45: canary.stack-overflow ............   Passed
 4/11 Test #46: canary.stack-use-after-return ....   Passed
 5/11 Test #47: canary.global-overflow ...........   Passed
 6/11 Test #48: canary.double-free ...............   Passed
 7/11 Test #49: canary.leak ......................   Passed
 8/11 Test #50: canary.signed-overflow ...........   Passed
 9/11 Test #51: canary.shift .....................   Passed
10/11 Test #52: canary.div-zero ..................   Passed
11/11 Test #53: canary.null ......................   Passed
100% tests passed out of 11
```

53 个全部通过，其中 11 个金丝雀的通过，意味着 11 种错误都被 sanitizer 抓到了。以上是 2026-11-25 在 Debian sid 的实体机上的结果。

## 2. 背景与概念

### 2.1 “通过”和“在工作”是两回事

一个检查工具只报告“没发现问题”的时候，你分不清它是真的没找到问题，还是根本没有在检查。解决办法是让它检查一个已知有问题的东西，看它能不能发现：这就是金丝雀。煤矿里的金丝雀比人先出现症状，这里的金丝雀比真正的 bug 先暴露“sanitizer 没开”。

### 2.2 三种 sanitizer 和它们报告的错误

`-fsanitize=address,undefined` 同时打开了三个检查：

| 错误 | 谁报告 | 报告里的第一句话 |
|---|---|---|
| 堆上越界读写 | AddressSanitizer（ASan） | `AddressSanitizer: heap-buffer-overflow` |
| 释放后使用 | ASan | `AddressSanitizer: heap-use-after-free` |
| 栈上越界 | ASan | `AddressSanitizer: stack-buffer-overflow` |
| 返回后使用栈变量 | ASan | `AddressSanitizer: stack-use-after-return` |
| 全局数组越界 | ASan | `AddressSanitizer: global-buffer-overflow` |
| 重复释放 | ASan | `AddressSanitizer: attempting double-free` |
| 内存泄漏 | LeakSanitizer（随 ASan 一起） | `LeakSanitizer: detected memory leaks` |
| 有符号整数溢出 | UndefinedBehaviorSanitizer（UBSan） | `runtime error: signed integer overflow` |
| 移位超过位数 | UBSan | `runtime error: shift exponent 40 is too large` |
| 除以零 | UBSan | `runtime error: division by zero` |
| 解引用空指针 | UBSan | `runtime error: load of null pointer` |

ASan 管内存访问：它在每块内存周围放“红区”，释放后的内存也标记起来，访问到就立即报告。LeakSanitizer 在程序退出时检查还有哪些分配没有释放、也没有任何指针指向。UBSan 管 C++ 标准里“未定义行为”的那些操作，编译器在这些操作前插入检查。

### 2.3 怎么读 ASan 的报告

下面是 `canary.heap-use-after-free` 的完整输出（`ctest -V` 里每行前面有测试编号，我去掉了；影子内存只留了出错的那一行）：

```text
==115830==ERROR: AddressSanitizer: heap-use-after-free on address 0x7c02d11e0010 at pc 0x55e64b7e63e8 ...
READ of size 4 at 0x7c02d11e0010 thread T0
    #0 0x55e64b7e63e7 in At .../tests/sanitizer_canaries.cpp:21
    #1 0x55e64b7e6bfc in HeapUseAfterFree .../tests/sanitizer_canaries.cpp:40
    #2 0x55e64b7e7256 in main .../tests/sanitizer_canaries.cpp:85
    ...

0x7c02d11e0010 is located 0 bytes inside of 16-byte region [0x7c02d11e0010,0x7c02d11e0020)
freed by thread T0 here:
    #0 0x7fe2d2d25f8f in operator delete[](void*) ../../../../src/libsanitizer/asan/asan_new_delete.cpp:179
    #1 0x55e64b7e640a in Release .../tests/sanitizer_canaries.cpp:22
    #2 0x55e64b7e6beb in HeapUseAfterFree .../tests/sanitizer_canaries.cpp:39
    ...

previously allocated by thread T0 here:
    #0 0x7fe2d2d2559f in operator new[](unsigned long) ../../../../src/libsanitizer/asan/asan_new_delete.cpp:111
    #1 0x55e64b7e691a in HeapUseAfterFree .../tests/sanitizer_canaries.cpp:38
    ...

SUMMARY: AddressSanitizer: heap-use-after-free .../tests/sanitizer_canaries.cpp:21 in At
Shadow bytes around the buggy address:
=>0x7c02d11e0000: fa fa[fd]fd fa fa fa fa fa fa fa fa fa fa fa fa
```

一份报告回答四个问题，自上而下：

1. **出了什么错、在哪里：** 第一行是错误类型（释放后使用）和地址；第二行说是一次 4 字节的读；`#0`、`#1`、`#2` 是出错时的调用栈，最上面一行就是出错的那一行代码（`sanitizer_canaries.cpp:21`）。
2. **这块内存是什么：** “位于一块 16 字节的区域里第 0 个字节”，说明是那块被释放的数组的开头。
3. **谁释放了它：** `freed by thread T0 here` 下面的调用栈，指向 `Release` 里的 `delete[]`（第 22 行），是从 `HeapUseAfterFree` 的第 39 行调用的。
4. **它最初是谁分配的：** `previously allocated by` 下面的调用栈，指向 `HeapUseAfterFree` 的第 38 行。

最后的 `SUMMARY` 是一句话总结，方便搜索；影子内存那几行是 ASan 自己的记账，方括号里的 `fd` 是出错的那个字节，意思是“已释放的堆内存”（报告末尾有图例）。日常排查时，看第 1 到 4 项就够了。

### 2.4 UBSan 和 LeakSanitizer 的报告

UBSan 默认只有一行：

```text
.../tests/sanitizer_canaries.cpp:67:5: runtime error: signed integer overflow: 2147483647 + 4 cannot be represented in type 'int'
```

设置 `UBSAN_OPTIONS=print_stacktrace=1`（`linux-asan` 测试预设里设了）以后，这一行后面多出调用栈：

```text
    #0 0x564caf90befe in SignedOverflow .../tests/sanitizer_canaries.cpp:67
    #1 0x564caf90c340 in main .../tests/sanitizer_canaries.cpp:91
    #2 0x7f1b30433fb9  (/usr/lib/x86_64-linux-gnu/libc.so.6+0x29fb9) (BuildId: ...)
```

LeakSanitizer 在程序退出时报告，告诉你泄漏了多少、是谁分配的：

```text
==117657==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 64 byte(s) in 1 object(s) allocated from:
    #0 0x7efdf792559f in operator new[](unsigned long) ../../../../src/libsanitizer/asan/asan_new_delete.cpp:111
    #1 0x55f69d3d2e02 in Leak .../tests/sanitizer_canaries.cpp:53
    #2 0x55f69d3d32f2 in main .../tests/sanitizer_canaries.cpp:89
```

### 2.5 为什么 `-fno-sanitize-recover=all`

UBSan 默认发现问题后只打印一行，程序接着往下跑。本章的预设加了 `-fno-sanitize-recover=all`，让它在第一个错误处就终止（platform-1.3 加的）。区别是这样的：对同一个有符号整数溢出的程序，不加这个选项，程序打印一行报告后继续，最终以它自己的返回值退出（这里是 3）；加了，程序在报告后立即终止，退出码是 1。对测试来说，前者的退出码是 0 或者别的什么，测试不会失败，后者才会让 `ctest` 把它记为失败。

### 2.6 为什么用 `PASS_REGULAR_EXPRESSION`，不用 `WILL_FAIL`

金丝雀要的是“sanitizer 报告了这个错误”，而不是“程序失败了”。CTest 提供两种办法：

- `WILL_FAIL`：把“退出码非零”倒过来算通过。它不要求是 sanitizer 报的；而且对段错误这类异常，我试过，即使设了 `WILL_FAIL`，CTest 还是显示 `Exception: SegFault` 并算失败，行为不一致。
- `PASS_REGULAR_EXPRESSION`：输出里出现这个正则才算通过，退出码被忽略（[CTest 的说明](https://cmake.org/cmake/help/latest/prop_test/PASS_REGULAR_EXPRESSION.html)）。一次普通的崩溃没有 sanitizer 的那句话，所以不通过；sanitizer 没开，也不通过。这正是我们要的。

### 2.7 为什么金丝雀要写得让编译器看不出来

直接写 `int a[4]; return a[4];`，看起来是最直接的越界。但项目的所有目标都带 `-Wall -Wextra -Werror`，在优化开着的时候（Release 预设是 `-O3`）GCC 在编译期就看出了这个越界，报 `-Werror=array-bounds`，构建直接失败，根本到不了运行。在 `-O0` 下编译器没有这个分析，所以只在 Debug 下试过的人看不出问题。

所以金丝雀里每个“错误的值”都不是写死的：错误的下标和除数放在 `volatile` 变量里，访问和释放通过 `noinline` 函数。编译器无法证明它们的值，于是不报警，而运行时的 sanitizer 照样能抓到。这样同一个文件在 `-O0`、`-O2`、`-O3` 下都能通过项目的警告选项，也不需要给它放宽任何警告。

### 2.8 环境变量放在测试预设里

`UBSAN_OPTIONS` 和 `ASAN_OPTIONS` 是 sanitizer 运行时读的环境变量。放在 `linux-asan` 的测试预设里，有三个好处：一眼能在 `linux.json` 里看到；只对用这个预设跑的测试生效；预设里设的值会盖过你 shell 里的同名变量。最后一点很重要：如果你的 shell 里恰好设了 `ASAN_OPTIONS=detect_leaks=0`，用预设运行时，泄漏检测依然是开着的。代价是，直接运行测试程序时这些变量不生效，要自己设。

### 2.9 CTest 和 GoogleTest 怎么配合

`tests/CMakeLists.txt` 里用 `gtest_discover_tests` 把每个 `TEST` 登记成一个单独的 CTest 测试，一个测试一个进程。所以 sanitizer 在某个测试里发现错误，只让这个进程终止，只有这个测试失败，其他的照常跑；预设里 `outputOnFailure` 是开着的，失败的那个测试的输出（也就是 sanitizer 的报告）会直接打印出来。

### 2.10 它看不到的地方

vcpkg 构建的库（libcurl、OpenSSL、spdlog 等）是用 platform-1.2 的三元组编出来的，三元组里没有加 `-fsanitize`。所以 sanitizer 只检查项目自己的代码；库内部的错误，只有在它们的内存访问穿过项目代码时才可能被看到。

## 3. 本章改动

本章的代码在支线分支 `quest/platform-01-linux-foundation` 上，不合入 main，也不打 tag。提交信息以 `chapter-platform-1.4-tests-sanitizers:` 开头，`git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.4"` 可以找到本章的提交。

1. 新增 `tests/sanitizer_canaries.cpp`。
2. `cmake/platform/linux.cmake`：新增缓存变量 `COMFYSTUDY_CANARIES`；在原来的空函数 `comfystudy_platform_tests()` 里登记金丝雀；新增辅助函数 `comfystudy_add_canary`。
3. `cmake/presets/linux.json`：`linux-asan` 的测试预设加 `environment`。
4. 分支的 `README.md`：加上本章，更新构建与运行。

没有改产品代码，也没有改已有的测试；顶层 `CMakeLists.txt`、`tests/CMakeLists.txt` 和 Windows 的文件都没有动。

## 4. 关键代码讲解

### 4.1 金丝雀程序

<!-- src: tests/sanitizer_canaries.cpp#L1-L28 -->
```cpp
// Canaries for the sanitizers: each argument triggers one real bug, and the tests in
// cmake/platform/linux.cmake pass only if the right sanitizer reports it. If a sanitizer is not switched on
// (or is switched off by an environment variable), its canary fails, so a green sanitizer run means something.
//
// The bad values go through volatile variables and noinline functions on purpose. The project builds with
// -Wall -Wextra -Werror, and at -O2 GCC finds a plain a[4] on an int[4] at compile time and stops the build;
// this way the file compiles at any optimisation level and only the sanitizers can see the bugs.
#include <climits>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

volatile int g_four = 4;
volatile int g_zero = 0;
volatile int g_forty = 40;
int g_global[4] = {1, 2, 3, 4};
int* volatile g_sink = nullptr;

__attribute__((noinline)) int At(const int* p, int i) { return p[i]; }
__attribute__((noinline)) void Release(int* p) { delete[] p; }
__attribute__((noinline)) void ReleaseOne(int* p) { delete p; }
__attribute__((noinline)) int* DanglingLocal() {
  int local[4] = {1, 2, 3, 4};
  int* volatile escape = local;
  return escape;
}
```

开头的注释说明了为什么这么写。`g_four`、`g_zero`、`g_forty` 是 `volatile`：编译器每次都要真的去读，无法提前算出值。`At`、`Release`、`ReleaseOne` 是 `noinline`：编译器不会把它们展开到调用处，也就看不到“释放之后又读”这样的前后关系。`DanglingLocal` 返回一个局部数组的地址（通过 `volatile` 指针，防止编译器发现并警告）。

<!-- src: tests/sanitizer_canaries.cpp#L30-L50 -->
```cpp
int HeapOverflow() {
  int* p = new int[4]{1, 2, 3, 4};
  const int v = At(p, g_four);
  Release(p);
  return v;
}

int HeapUseAfterFree() {
  int* p = new int[4]{1, 2, 3, 4};
  Release(p);
  return At(p, 0);
}

int StackOverflow() {
  int a[4] = {1, 2, 3, 4};
  return At(a, g_four);
}

int StackUseAfterReturn() { return At(DanglingLocal(), 0); }

int GlobalOverflow() { return At(g_global, g_four); }
```

每种错误一个函数。堆越界读第 4 个元素（一共 4 个）；释放后使用在 `Release` 之后再读；栈越界和全局越界都通过 `At`，所以是 ASan 报告，而不是 UBSan（如果直接写 `a[4]`，UBSan 的“下标越界”检查会抢先报告）；`StackUseAfterReturn` 读一个已经返回的函数的局部数组。

<!-- src: tests/sanitizer_canaries.cpp#L52-L56 -->
```cpp
int Leak() {
  g_sink = new int[16];
  g_sink = nullptr;  // the only pointer to the block is gone
  return 0;
}
```

泄漏：分配一块 64 字节的内存，唯一的指针存在一个全局的 `volatile` 里，然后置空。程序退出时 LeakSanitizer 发现这块内存既没有释放，也没有任何指针指向它。（用局部变量做这件事会被 `-Werror=unused-but-set-variable` 拦住，所以用全局的。）

### 4.2 登记

<!-- src: cmake/platform/linux.cmake#L20-L28 -->
```cmake
# Canaries: tests/sanitizer_canaries.cpp triggers one bug per run, and one test per bug passes only if the sanitizer
# reports it. On by default when a sanitizer is on. Turning it on without a sanitizer makes every canary fail,
# which is how you see that the check notices a missing sanitizer.
if(COMFYSTUDY_SANITIZE)
  set(comfystudy_canaries_default ON)
else()
  set(comfystudy_canaries_default OFF)
endif()
set(COMFYSTUDY_CANARIES ${comfystudy_canaries_default} CACHE BOOL "Build the sanitizer canaries and register their tests")
```

`COMFYSTUDY_CANARIES` 默认值取决于有没有开 sanitizer：开了就是 ON，没开就是 OFF，所以 `linux-debug` 和 `linux-release` 不会多出测试。你也可以手动打开它，上面“先看效果”就是这么做的。

<!-- src: cmake/platform/linux.cmake#L48-L80 -->
```cmake
function(comfystudy_add_canary name sentence)
  add_test(NAME canary.${name} COMMAND sanitizer_canaries ${name})
  set_tests_properties(canary.${name} PROPERTIES PASS_REGULAR_EXPRESSION "${sentence}" LABELS canary)
endfunction()

# Called by tests/CMakeLists.txt once add_comfystudy_test exists: the tests that run on Linux only, which are the
# canaries. They are registered for the sanitizers that are on (all of them when none is, so that they fail).
function(comfystudy_platform_tests)
  if(NOT COMFYSTUDY_CANARIES)
    return()
  endif()
  add_executable(sanitizer_canaries sanitizer_canaries.cpp)
  target_link_libraries(sanitizer_canaries PRIVATE comfystudy_options)

  # AddressSanitizer, and LeakSanitizer which comes with it.
  if(NOT COMFYSTUDY_SANITIZE OR COMFYSTUDY_SANITIZE MATCHES "address")
    comfystudy_add_canary(heap-overflow "AddressSanitizer: heap-buffer-overflow")
    comfystudy_add_canary(heap-use-after-free "AddressSanitizer: heap-use-after-free")
    comfystudy_add_canary(stack-overflow "AddressSanitizer: stack-buffer-overflow")
    comfystudy_add_canary(stack-use-after-return "AddressSanitizer: stack-use-after-return")
    comfystudy_add_canary(global-overflow "AddressSanitizer: global-buffer-overflow")
    comfystudy_add_canary(double-free "AddressSanitizer: attempting double-free")
    comfystudy_add_canary(leak "LeakSanitizer: detected memory leaks")
  endif()

  # UndefinedBehaviorSanitizer.
  if(NOT COMFYSTUDY_SANITIZE OR COMFYSTUDY_SANITIZE MATCHES "undefined")
    comfystudy_add_canary(signed-overflow "runtime error: signed integer overflow")
    comfystudy_add_canary(shift "runtime error: shift exponent")
    comfystudy_add_canary(div-zero "runtime error: division by zero")
    comfystudy_add_canary(null "runtime error: load of null pointer")
  endif()
endfunction()
```

`comfystudy_add_canary` 是登记一个测试的辅助函数：命令是 `sanitizer_canaries <名字>`，通过的条件是输出里有 `sentence`。`comfystudy_platform_tests()` 是顶层要求平台文件提供的那个函数，platform-1.3 里是空的，现在登记金丝雀。目标 `sanitizer_canaries` 链接 `comfystudy_options`，所以项目的警告选项照样管它。正则只写错误类型那一小段（比如 `AddressSanitizer: heap-buffer-overflow`），不写地址和行号。

两个 `if` 的条件是“没开 sanitizer，或者开了对应的那个”。第一种情况是为了在没有 sanitizer 的构建里手动打开时，11 个金丝雀都登记出来，好让它们失败；第二种情况是只开 `undefined` 时只登记 UBSan 的 4 个（ASan 的 7 个在没有 ASan 的构建里本来就无法通过）。

### 4.3 测试预设

<!-- src: cmake/presets/linux.json#L49-L62 -->
```json
  "testPresets": [
    { "name": "linux-debug", "configurePreset": "linux-debug", "output": { "outputOnFailure": true } },
    { "name": "linux-release", "configurePreset": "linux-release", "output": { "outputOnFailure": true } },
    {
      "name": "linux-asan",
      "configurePreset": "linux-asan",
      "output": { "outputOnFailure": true },
      "environment": {
        "UBSAN_OPTIONS": "print_stacktrace=1",
        "ASAN_OPTIONS": "detect_leaks=1"
      }
    }
  ]
}
```

只有 `linux-asan` 带 `environment`。`print_stacktrace=1` 让 UBSan 的报告带调用栈；`detect_leaks=1` 显式打开泄漏检测，不依赖默认值，也不会被 shell 里的设置关掉。

## 5. 构建与运行

在支线分支上，`VCPKG_ROOT` 已设好、库已装好的前提下：

```bash
git switch quest/platform-01-linux-foundation
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
```

只跑金丝雀，或者单独看一份报告：

```bash
ctest --preset linux-asan -L canary
ctest --preset linux-asan -R "^canary.heap-use-after-free$" -V
build/linux-asan/bin/sanitizer_canaries leak
```

只开 UBSan（不开 ASan）：

```bash
cmake --preset linux-asan -B build/linux-ubsan -DCOMFYSTUDY_SANITIZE=undefined
cmake --build build/linux-ubsan
ctest --test-dir build/linux-ubsan -L canary
```

```text
1/4 Test #43: canary.signed-overflow ...........   Passed
2/4 Test #44: canary.shift .....................   Passed
3/4 Test #45: canary.div-zero ..................   Passed
4/4 Test #46: canary.null ......................   Passed
100% tests passed out of 4
```

## 6. 验证

- `ctest --preset linux-asan`：53 个测试全部通过（42 个原有，11 个金丝雀）；构建 0 个警告，包括金丝雀。
- 数量：`linux-debug`、`linux-release` 仍是 42 个，没有金丝雀；只开 UBSan 的构建登记 4 个金丝雀，全部通过。
- 改动检验（四个）：
  - 在没有 sanitizer 的构建里手动打开金丝雀（`-DCOMFYSTUDY_CANARIES=ON`）：11 个全部失败（见“先看效果”）。
  - 泄漏检测被环境变量关掉：直接运行（不通过预设）`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-asan -R "^canary.leak$"`，失败，找不到 `LeakSanitizer` 的报告；同样的 shell 设置，通过预设运行，测试通过，因为预设里设了 `detect_leaks=1`。
  - 写成编译器看得穿的样子：把 `HeapOverflow` 里的 `At(p, g_four)` 临时换成 `p[4]`，用 Release 预设（`-O3`）并打开金丝雀构建：`error: array subscript 4 is outside array bounds of 'void [16]' [-Werror=array-bounds=]`，构建失败。改回去。
  - 一个有 bug 的 GoogleTest 测试：临时在 `tests/test_config.cpp` 里加一个读数组末尾之后一个元素的测试。`linux-debug` 下该测试通过（越界读悄悄发生）；`linux-asan` 下该测试失败，输出里有 `ERROR: AddressSanitizer: heap-buffer-overflow`、`READ of size 4`、出错的调用栈（第一行就是 `SanitizerDemo_ReadsOnePastTheEnd_Test::TestBody() .../tests/test_config.cpp:131`）和分配位置（第 129 行），汇总是 `0% tests passed, 1 tests failed out of 1`。验证完撤销，没有提交。
- 退出码：`sanitizer_canaries heap-overflow` 在 sanitizer 构建里退出码是 1；对同一个有符号整数溢出的程序，`-fsanitize=undefined` 不加 `-fno-sanitize-recover=all` 时退出码是 3（程序继续运行），加了是 1。
- Windows 一侧：在这个分支上 `windows-debug` 配置成功，构建没有警告，`ctest` 62 个全部通过，没有金丝雀。
- 没有做的：
  - 没有在容器里验证泄漏检测（容器里可能需要额外的权限，见第 8 节），留给 platform-1.6。
  - 只用了 GCC 16，`stack-use-after-return` 在这个版本下不需要任何选项就能抓到；别的版本的默认值我没有验证。
  - 没有试 ThreadSanitizer 和 MemorySanitizer（见练习）。
  - 没有在产品代码里发现任何 sanitizer 报告的问题，42 个原有测试一直通过。

## 7. 技术资料

- 环境：GCC 16.2.0（Debian 包 16.2.0-5），运行库 `libasan.so.8` 和 `libubsan.so.1`，CMake 4.4.5，Debian sid。
- AddressSanitizer 的说明：<https://github.com/google/sanitizers/wiki/AddressSanitizer>。GCC 的 sanitizer 选项：<https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html>。
- CTest 的 `PASS_REGULAR_EXPRESSION`：<https://cmake.org/cmake/help/latest/prop_test/PASS_REGULAR_EXPRESSION.html>。
- 设计取舍：
  - 证明 sanitizer 在工作：金丝雀加测试（采用）；只看原有测试通过（否决，分不出“没问题”和“没生效”）；临时在产品代码里放错误（只在演示里用，不做长期检查）。
  - 判定方式：`PASS_REGULAR_EXPRESSION`（采用）；`WILL_FAIL`（否决，见 2.6）；脚本包装器检查输出（否决，多一层，没有收益）。
  - 让编译器看不出错误：`volatile` 加 `noinline`（采用）；给金丝雀文件加 `-Wno-array-bounds` 之类（否决，违反“不压警告”，还要随编译器版本维护）。
  - 环境变量：测试预设（采用）；CMake 的 `ENVIRONMENT` 测试属性（否决，藏在 CMake 里，改起来不直观）；写进文档让开发者自己设（否决，容易被忽略）。
- 被否决的：在普通构建里也注册金丝雀（会让普通的 `ctest` 多出一组必须配合 sanitizer 的测试）；只注册能通过的金丝雀（会漏掉“sanitizer 没生效”这个最该发现的问题）。

## 8. 常见问题与坑

**直接写 `a[4]` 的“金丝雀”在 Release 下编不过。** `-Wall -Wextra -Werror` 加优化后，GCC 在编译期发现越界，报 `-Werror=array-bounds`；在 `-O0` 下看不出，所以只试 Debug 的人会漏掉这个问题。写法见 2.7。

**“赋值后没用过”的变量也会被拦。** 泄漏用例最初用一个局部的 `volatile` 指针，被 `-Werror=unused-but-set-variable` 拦下，换成全局的才通过。

**数组越界和空指针，UBSan 可能比 ASan 先报告。** 对固定大小的数组直接写 `a[4]`，UBSan 的“下标越界”检查抢先报告了 `runtime error: index 4 out of bounds`，ASan 没有机会；要让 ASan 报告，必须通过指针访问。空指针也是 UBSan 先报告 `load of null pointer`。

**UBSan 默认不终止程序。** 不加 `-fno-sanitize-recover=all` 的话，打印一行后程序继续跑，测试不会失败。见 2.5。

**报告里的路径和行号对不上？** 报告里的行号来自调试信息，所以构建要带 `-g`（Debug 预设带）。

**`WILL_FAIL` 不能代替 `PASS_REGULAR_EXPRESSION`。** 见 2.6。

**shell 里的 `ASAN_OPTIONS` 会影响直接运行的测试程序。** 通过 `linux-asan` 预设运行时，预设里设的值生效；直接运行 `build/linux-asan/bin/...` 时，用的是你 shell 里的值。

**容器里的泄漏检测。** LeakSanitizer 在容器里可能需要额外的权限才能工作（没有的话会报致命错误并退出）。这台机器上是正常的，本章没有在容器里试过，platform-1.6 用到容器时会实测。

**vcpkg 的库没有被检查。** 见 2.10。

## 9. 练习

1. 加第 12 个金丝雀：未对齐的内存访问（UBSan 的 `alignment` 检查，包含在 `undefined` 里）。先在 `tests/sanitizer_canaries.cpp` 里写函数，再在 `comfystudy_platform_tests()` 里登记，运行看报告的第一句话是什么。
2. 试 ThreadSanitizer：配置 `-DCOMFYSTUDY_SANITIZE=thread`。它能和 `address` 同时开吗？看编译器怎么说。写一个数据竞争的金丝雀，看 TSan 的报告。
3. 去掉 `-fno-sanitize-recover=all`，重新构建 `linux-asan`，再运行 `canary.signed-overflow`：它还通过吗？为什么？再写一个会触发 UBSan 的普通 GoogleTest 测试，看 `ctest` 的结果有什么不同。
4. 在一个 GoogleTest 测试里故意泄漏一块内存，运行 `ctest --preset linux-asan`：是哪一步报告的？测试本身通过了，为什么 `ctest` 仍然把它记为失败？
5. 设 `ASAN_OPTIONS=detect_stack_use_after_return=0`，运行 `canary.stack-use-after-return`：还能通过吗？这对“换了编译器版本”意味着什么？

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 platform-1.5 检查 GPU：复用主线 0.6 的探针，给 `check-env.sh` 加 `--gpu`，确认这台机器上的程序能用上显卡。只检查驱动这一层，不装 CUDA 工具包和 cuDNN。探针要在 `linux-asan` 下运行，所以会遇到 AddressSanitizer 和 CUDA 驱动的冲突。
