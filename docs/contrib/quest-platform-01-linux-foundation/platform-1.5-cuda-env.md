# platform-1.5 GPU 与 CUDA 环境

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/platform-01-linux-foundation`（支线不打 tag，用 `git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.5"` 定位本章提交）
- 所属支线：[支线 platform-01：Linux 阶段 0](README.md)
- 可开始于：主线 [0.6 检测 GPU 与 CUDA 驱动](../../tutorial/phase-00-toolchain/0.6-gpu-check.md)
- 一句话目标：确认 Linux 机器上的程序能用上显卡，只检查驱动这一层：复用主线 0.6 的探针 `tools/gpu-probe`，给 `check-env.sh` 加 `--gpu`，并让 `linux-asan` 的测试在 AddressSanitizer 下也能初始化 CUDA 驱动（`protect_shadow_gap=0`）；不装 CUDA 工具包和 cuDNN，用到时再装

## 1. 本章目标与成果

主线 0.6 在 Windows 上写了一个只依赖 NVIDIA 驱动的探针，确认程序能用上显卡。这一章在 Linux 上做同一件事，探针原样使用，只是加上 bash 一侧的检查。这台机器是 Debian sid，带一块 RTX 4000 SFF Ada，驱动已经装好，没有装 CUDA 工具包，也没有 cuDNN。这一章不装它们：到用 `nvcc` 或 cuDNN 的章节，再装、再检查。做完后你会得到：

- **`scripts/check-env.sh --gpu`：** 在原来的检查之后，读 `nvidia-smi`（显卡、驱动版本、驱动支持的 CUDA 版本），再用固定的 `g++-16` 单独配置、构建、运行主线的探针 `tools/gpu-probe`。不带 `--gpu` 时输出和以前完全一样。
- **项目里默认带一个 `gpu_probe` 测试。** 主线 0.6 已经让主项目无条件加入探针，这个分支继承了它。三个预设的测试各多 1 个：`linux-debug` 和 `linux-release` 是 43 个，`linux-asan` 是 54 个，全部通过，构建 0 个警告。
- **一个只在 Linux 的发现：** 在 AddressSanitizer 下，CUDA 驱动的 `cuInit` 会返回 2（内存不足）。原因是 ASan 保护了一段地址空间，驱动要用。给 `linux-asan` 的测试预设加上 `ASAN_OPTIONS=protect_shadow_gap=0` 就好了。
- **脚本测试增加到 29 个**，用假的 `nvidia-smi` 覆盖各种情况；脚本不检查 `nvcc`。
- **一个明确的边界：** 没有工具包时也能用 GPU 的这一层是驱动。`find_package(CUDAToolkit)` 在这台机器上找不到，`nvcc` 不存在，这是预期状态。

先看效果：

```text
scripts/check-env.sh --gpu
```

```text
（前面是 g++-16、gcc-16、cmake、ninja、pkg-config、git、ld、curl、zip、unzip、tar、make、perl、vcpkg，INFO glibc，两次试编译，和以前一样）
OK      nvidia-smi: NVIDIA RTX 4000 SFF Ada Generation, 620.70.10, supports CUDA 13.5
OK      gpu: NVIDIA RTX 4000 SFF Ada Generation (compute capability 8.9), context created
check-env: OK
```

`nvidia-smi` 这一行说明显卡、驱动版本（620.70.10）和驱动支持的 CUDA 版本（13.5）。`gpu` 这一行是探针的结果：它用 `g++-16` 编译，链接了驱动的 `libcuda.so`，在这块显卡上建出了上下文。

## 2. 背景与概念

**这一层是什么。** 主线 0.6 已经讲了驱动和工具包的区别，这里只补 Linux 的部分。Linux 上，驱动包（这台机器是 NVIDIA 自己的 apt 仓库里的 `libcuda1`）提供 `libcuda.so.1` 和一个指向它的开发链接 `libcuda.so`：

```text
/usr/lib/x86_64-linux-gnu/libcuda.so -> libcuda.so.1
/usr/lib/x86_64-linux-gnu/libcuda.so.1 -> libcuda.so.620.70.10
```

`dpkg -S` 显示 `libcuda.so` 属于 `libcuda1:amd64`。CMake 的 `find_library(cuda)` 找到的就是这个链接。这样，没有工具包、没有 `cuda.h` 的机器，也能用 CMake 把程序链接到驱动上。要注意：这个开发链接是驱动包带来的，别的发行版的驱动包不一定提供；没有它，探针的 CMake 会提示找不到库，测试变成 Disabled。

**为什么只有驱动这一层。** 系统里的 CUDA 先要回答“GPU 能不能被程序用”。这一层不需要 `nvcc`，不需要运行时库，也不需要 cuDNN。后面的章节用到 `nvcc`（支线 platform-5.1、platform-12.4，主线 8.1 的 Linux 对应）、cuDNN（ONNX Runtime 的运行时库）时，那些章节自己检查。libtorch 的预编译包自带 CUDA 运行库和 cuDNN。

**为什么单独配置探针。** `check-env.sh` 要在项目的依赖装好之前就能运行（它本来就是用来告诉你缺什么）。主项目一配置就会触发 vcpkg 拉依赖，慢，而且可能失败。探针不依赖任何库，所以 `--gpu` 用 `tools/gpu-probe` 自己的 `CMakeLists.txt` 单独配置，并传上本分支的工具链文件 `cmake/toolchains/linux-gcc.cmake`，编译器就是固定的 `g++-16`。

**ASan 和 CUDA。** AddressSanitizer 把一大段虚拟地址空间留作“影子内存”，并在影子内存之间设了一个保护间隙，任何碰到它的访问都会报错。CUDA 驱动初始化时要在进程里预留很大的地址范围，和这个间隙冲突，`cuInit` 就返回 `CUDA_ERROR_OUT_OF_MEMORY`（2）。`protect_shadow_gap=0` 让 ASan 不保护这个间隙。代价是：程序里碰到影子间隙的野访问，ASan 不再报告。这类访问非常少见，其他检查（堆、栈、全局变量的越界，释放后使用，泄漏）不受影响。

**为什么这样改预设，不在探针上改。** 探针是主线的文件，跨平台共用，不该为 Linux 的 sanitizer 带上特殊处理。`ASAN_OPTIONS` 本来就在这个分支的 `linux-asan` 测试预设里设置（上一章加了 `detect_leaks=1`），这里是同一个地方多一项。

## 3. 本章改动

```text
git show --stat $(git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.5" --format=%h)
```

- 修改 `scripts/check-env.sh`：加 `--gpu`，读 `nvidia-smi`，再单独配置、构建、运行 `tools/gpu-probe`；顺手让 `script_dir` 成为绝对路径，并去掉注释里“nvcc 要认这个版本”的说明（本章不涉及 `nvcc`）。
- 修改 `tests/scripts/test-check-env.sh`：增加 `--gpu` 的用例。
- 修改 `cmake/presets/linux.json`：`linux-asan` 测试预设的 `ASAN_OPTIONS` 加 `protect_shadow_gap=0`。
- 修改 `cmake/toolchains/linux-gcc.cmake`：只去掉一条关于 `nvcc` 的注释。
- 修改 `README.md`：起点改成主线 tag `chapter-0.6-gpu-check`，加这一章。
- 探针 `tools/gpu-probe/` 和主 `CMakeLists.txt` 里加入它的那一行，来自主线 0.6，本章不改。

## 4. 关键代码讲解

**参数。** `--gpu` 是新的开关，和 `--no-smoke` 独立。`script_dir` 要是绝对路径，因为传给 CMake 的工具链文件路径，如果是相对路径，CMake 会按别的目录解析，结果是“找不到编译器”。

<!-- src: scripts/check-env.sh#L17-L36 -->
```bash

script_dir="${BASH_SOURCE[0]%/*}"
[ "$script_dir" = "${BASH_SOURCE[0]}" ] && script_dir=.
script_dir="$(cd "$script_dir" && pwd)"  # absolute: cmake resolves a relative toolchain file path elsewhere

smoke=1
gpu=0
for argument in "$@"; do
  case "$argument" in
    --no-smoke) smoke=0 ;;
    --gpu) gpu=1 ;;
    *)
      echo "usage: scripts/check-env.sh [--no-smoke] [--gpu]"
      exit 2
      ;;
  esac
done

missing=()
packages=()
```

**`--gpu` 的检查。** 先看 `nvidia-smi`。找不到命令，说明没有驱动，只提示先装驱动，不给 `apt install` 的命令（驱动不在这个脚本管的范围，装错驱动可能让图形界面起不来）。命令能找到但运行失败，说明驱动没起来（内核模块没加载，常见于刚升级内核）。成功后，用 bash 的正则从完整输出里读 `CUDA UMD Version`（旧版驱动写作 `CUDA Version`）；显卡名和驱动版本用 `--query-gpu` 读。

<!-- src: scripts/check-env.sh#L264-L292 -->
```bash
# --gpu: the GPU as the NVIDIA driver sees it. nvidia-smi names the card and says which CUDA version the driver
# supports; tools/gpu-probe (the same program the main project runs as the gpu_probe test, here configured on its
# own with the pinned g++) calls the driver the way a program would. Only the driver is needed: nvcc and cuDNN are
# not checked, the chapters that use them check them. The driver itself is not installed by this script.
run_gpu() {
  local smi card full cuda="" probe_dir probe_out step device
  smi="$(command -v nvidia-smi 2>/dev/null)"
  if [ -z "$smi" ]; then
    echo "MISSING nvidia-smi: no NVIDIA driver found (install the driver for your distribution first)"
    missing+=(nvidia-smi)
    return
  fi
  if ! card="$("$smi" --query-gpu=name,driver_version --format=csv,noheader 2>&1)"; then
    printf 'MISSING nvidia-smi: cannot talk to the driver (%s)\n' "$(first_line "$card")"
    missing+=(nvidia-smi)
    return
  fi
  full="$("$smi" 2>&1)"
  if [[ "$full" =~ CUDA\ (UMD\ )?Version:[[:space:]]*([0-9]+\.[0-9]+) ]]; then
    cuda="${BASH_REMATCH[2]}"
  fi
  card="$(first_line "$card")"
  if [ -z "$card" ] || [ -z "$cuda" ]; then
    echo "MISSING nvidia-smi: cannot read the GPU name or the CUDA version"
    missing+=(nvidia-smi)
    return
  fi
  printf 'OK      nvidia-smi: %s, supports CUDA %s\n' "$card" "$cuda"
  [ "$smoke" -eq 1 ] || return
```

**探针。** 在临时目录里按 `tools/gpu-probe` 的 `CMakeLists.txt` 配置、构建、运行。三步任何一步失败，就把那一步的名字和输出末尾 3 行打出来。配置输出里如果带 `no CUDA driver library found`，说明 CMake 没找到 `libcuda.so`，这时配置本身会成功（探针的测试登记成 Disabled），所以要专门看这句话。运行成功时，只从探针的输出里取设备那一行。

<!-- src: scripts/check-env.sh#L294-L322 -->
```bash
  local source_dir="$script_dir/../tools/gpu-probe"
  probe_dir="$(mktemp -d)" || { echo "MISSING gpu: cannot create a temporary directory"; missing+=(gpu); return; }
  # shellcheck disable=SC2064
  trap "rm -rf '$probe_dir'" RETURN
  step=configure
  probe_out="$(cmake -S "$source_dir" -B "$probe_dir" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$script_dir/../cmake/toolchains/linux-gcc.cmake" 2>&1)"
  if [ $? -eq 0 ] && [[ "$probe_out" != *"no CUDA driver library found"* ]]; then
    step=build
    if probe_out="$(cmake --build "$probe_dir" 2>&1)"; then
      step=run
      probe_out="$("$probe_dir/gpu_probe" 2>&1)"
      if [ $? -eq 0 ]; then
        while IFS= read -r device; do
          if [[ "$device" == "device 0: "* ]]; then
            printf 'OK      gpu: %s, context created\n' "${device#device 0: }"
          fi
        done <<< "$probe_out"
        return
      fi
    fi
  fi
  printf 'MISSING gpu: %s failed\n' "$step"
  printf '%s\n' "$probe_out" | grep -v '^$' | tail -n 3 | sed 's/^/        /'
  missing+=(gpu)
}
if [ "$gpu" -eq 1 ]; then
  run_gpu
fi
```

**测试预设。** `linux-asan` 的测试预设里，`ASAN_OPTIONS` 加上 `protect_shadow_gap=0`：

<!-- src: cmake/presets/linux.json#L52-L60 -->
```json
    {
      "name": "linux-asan",
      "configurePreset": "linux-asan",
      "output": { "outputOnFailure": true },
      "environment": {
        "UBSAN_OPTIONS": "print_stacktrace=1",
        "ASAN_OPTIONS": "detect_leaks=1:protect_shadow_gap=0"
      }
    }
```

**脚本测试。** 用假的 `nvidia-smi`：带 `--query-gpu` 参数时输出显卡名和驱动版本，不带参数时输出一行含 `CUDA UMD Version` 的头。`--no-smoke` 让脚本跳过探针，因为探针要真的编译器和真的显卡。

<!-- src: tests/scripts/test-check-env.sh#L188-L229 -->
```bash

# --gpu: nvidia-smi is faked; tools/gpu-probe needs a real compiler and a real GPU, so --no-smoke skips it here
# (the probe itself is the gpu_probe test of the main project and runs on the real machine).
make_smi() {
  make_fake "$1" nvidia-smi "case \"\$1\" in --query-gpu*) echo \"NVIDIA RTX 4000 SFF Ada Generation, 620.70.10\";; *) echo \"| NVIDIA-SMI 620.70.10   Driver Version: 620.70.10   $2 |\";; esac"
}

dir="$(new_good_dir)"
run_check "$dir" --no-smoke
expect_absent "without --gpu nvidia-smi is not looked at" "nvidia-smi"

dir="$(new_good_dir)"
make_smi "$dir" "CUDA UMD Version: 13.5"
run_check "$dir" --no-smoke --gpu
expect "--gpu with a working driver" 0 "OK      nvidia-smi: NVIDIA RTX 4000 SFF Ada Generation, 620.70.10, supports CUDA 13.5" "check-env: OK"

dir="$(new_good_dir)"
make_smi "$dir" "CUDA Version: 12.9"
run_check "$dir" --no-smoke --gpu
expect "--gpu reads the older 'CUDA Version' wording" 0 "supports CUDA 12.9"

dir="$(new_good_dir)"
run_check "$dir" --no-smoke --gpu
expect "--gpu without nvidia-smi" 1 "MISSING nvidia-smi: no NVIDIA driver found" "check-env: FAILED, missing: nvidia-smi"
expect_absent "no apt command is suggested for a missing driver" "sudo apt install"

dir="$(new_good_dir)"
make_fake "$dir" nvidia-smi 'echo "NVIDIA-SMI has failed because it could not communicate with the NVIDIA driver."; exit 9'
run_check "$dir" --no-smoke --gpu
expect "--gpu when nvidia-smi cannot talk to the driver" 1 "MISSING nvidia-smi: cannot talk to the driver (NVIDIA-SMI has failed"

dir="$(new_good_dir)"
make_smi "$dir" "no version here"
run_check "$dir" --no-smoke --gpu
expect "--gpu when the CUDA version cannot be read" 1 "MISSING nvidia-smi: cannot read the GPU name or the CUDA version"

dir="$(new_good_dir)"
make_smi "$dir" "CUDA UMD Version: 13.5"
make_fake "$dir" nvcc 'echo "Cuda compilation tools, release 13.5, V13.5.37"'
run_check "$dir" --no-smoke --gpu
expect_absent "nvcc is not checked, whether it is there or not" "nvcc"

```

## 5. 构建与运行

在仓库根目录（`VCPKG_ROOT` 已设好）：

```text
scripts/check-env.sh --gpu
bash tests/scripts/test-check-env.sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

配置时多了一行（主线 0.6 的探针）：

```text
-- GPU: CUDA driver library /usr/lib/x86_64-linux-gnu/libcuda.so
```

三个预设的结果：

```text
linux-debug:   100% tests passed out of 43
linux-release: 100% tests passed out of 43
linux-asan:    100% tests passed out of 54
```

三个预设都构建成功（`-Wall -Wextra -Werror`，所以没有警告）。单独看探针：

```text
ctest --preset linux-debug -R gpu_probe -V
```

```text
driver_cuda: 13.5 (13050)
device_count: 1
device 0: NVIDIA RTX 4000 SFF Ada Generation (compute capability 8.9)
context: created and released
```

## 6. 验证

**不带 `--gpu` 的输出不变。** `scripts/check-env.sh` 的输出仍然是上一章的 17 行加 `check-env: OK`，没有 `nvidia-smi` 和 `gpu`；我把改动前后的脚本在这台机器上各跑一次，输出逐行相同。脚本测试里有一个用例检查这一点：没有 `nvidia-smi` 命令，不带 `--gpu` 也退出 0，输出里没有 `nvidia-smi`。

**脚本测试。** 29 个用例，其中新增 8 个：

```text
check-env tests: 29 passed, 0 failed
```

新增的用例是：不带 `--gpu`、驱动正常、旧版 `CUDA Version` 写法、没有 `nvidia-smi`（并且不建议 `apt install`）、`nvidia-smi` 运行失败、读不出版本、有没有 `nvcc` 都不出现 `nvcc`。

**同一份探针，两个平台。** 主线 0.6 的探针在 Windows 上读到驱动支持 CUDA 13.4，这里读到 13.5，两边都在各自的显卡上建出了上下文。

**ASan 的发现。** 不带环境变量，在 `linux-asan` 下运行探针：

```text
ctest --test-dir build/linux-asan -R gpu_probe --output-on-failure
```

```text
1/1 Test #54: gpu_probe ........................***Failed    0.16 sec
error: cuInit returned 2
```

带上 `protect_shadow_gap=0`：

```text
driver_cuda: 13.5 (13050)
device_count: 1
```

一个常见的疑问是，会不会是 `detect_leaks` 或别的选项导致的。我试过 `protect_shadow_gap=0` 单独设置、再加上 `replace_intrin=0`，只要有 `protect_shadow_gap=0`，探针都正常；不设，就一直是 2。所以只加这一项。

**金丝雀仍然有效。** `linux-asan` 下的 11 个金丝雀测试仍然全部通过，说明 `protect_shadow_gap=0` 没有影响 ASan 对其他错误的检测：

```text
ctest --preset linux-asan -R canary
```

```text
100% tests passed out of 11
```

**改动检验。** 把探针 `CMakeLists.txt` 里 `find_library` 的库名临时改成不存在的名字，在 Linux 上配置：

```text
-- GPU: no CUDA driver library found (libcuda.so, or cuda.lib in CUDA_PATH), the gpu_probe test is disabled
```

配置成功，测试是 Disabled。`check-env.sh --gpu` 在这种情况下会报 `MISSING gpu: configure failed`，并附上配置输出的末尾。

## 7. 技术资料

- Driver API 和 `libcuda.so`：见主线 0.6 的第 7 节；`CUDA_ERROR_OUT_OF_MEMORY` 的值是 2。
- AddressSanitizer 的 `protect_shadow_gap` 选项（默认 1）：设为 0 时，不在影子内存之间设保护页。
- CMake：`find_library`、`CMAKE_TOOLCHAIN_FILE`（相对路径的解析）；`ctest -L gpu`。
- `nvidia-smi --query-gpu=name,driver_version --format=csv,noheader`。
- NVIDIA 的 CUDA 13.5 安装指南（只作备忘，本章没有执行）：x86_64 上受支持的 GCC 是 6.x 到 16.x；受支持的发行版是 Debian 12 和 13，Debian 12 从 13.5 起标为弃用，testing 和 sid 没有列出；Debian 的仓库安装用 `cuda-toolkit`，用 `cuda-toolkit-13-5` 可以固定小版本。
- 设计取舍：
  - **只检查驱动，不装工具包。** 这一章的目的是确认 GPU 能被程序用。工具包和 cuDNN 到用的时候再装，到那时才知道要哪个版本、装在哪里。
  - **探针不改，预设改。** 见第 2 节。
  - **`--gpu` 不检查 `nvcc` 和 cuDNN，连 `INFO` 都不打印。** 它们没装是正常状态，打印出来只会让人以为缺了什么。
  - **没有 `apt install` 驱动的建议。** 装驱动要看显卡、内核和发行版，不能一条命令说清。
  - **被否决：** 在 sid 上装 NVIDIA 仓库的 `cuda-toolkit-13-5` 和 `cudnn9-cuda-13`（我只用 `apt-get -s` 模拟过：80 个新包，不动驱动，下载约 4.4 GB，安装约 8.9 GB，没有真的装）；用 Debian 自己的 `nvidia-cuda-toolkit`（是 CUDA 12.4，依赖的 GCC 只到 13，不认 GCC 16）；runfile 安装（会和 apt 装的驱动冲突）。
  - **`CMAKE_CUDA_HOST_COMPILER`：** 没有 `nvcc`，这个变量没法验证，所以没有加到工具链文件里，留给用到 CUDA 语言的章节。
- 这台机器：Debian sid，驱动 620.70.10（开源内核模块，`nvidia-smi` 显示 `CUDA UMD Version: 13.5`），RTX 4000 SFF Ada，计算能力 8.9。

## 8. 常见问题与坑

- **`check-env.sh --gpu` 报 `MISSING nvidia-smi: no NVIDIA driver found`。** 没装驱动，或者 `nvidia-smi` 不在 `PATH` 里。先装驱动；驱动怎么装，看你的发行版和显卡。
- **`MISSING nvidia-smi: cannot talk to the driver`。** 驱动装了，但内核模块没加载。常见于刚升级内核、还没重启，或者 DKMS 没编译成功。
- **`MISSING gpu: configure failed`，输出里有 `no CUDA driver library found`。** 没有 `libcuda.so` 这个开发链接。看 `ls /usr/lib/x86_64-linux-gnu/libcuda*`；驱动包没带的话，`libcuda.so.1` 在也不够，CMake 要的是不带 `.1` 的名字。
- **`MISSING gpu: run failed`，输出是 `error: cuInit returned 2`。** 在 ASan 下，见第 2 节。不是 ASan 的话，2 也可能是显存真的不够。
- **`skip: the driver reports no CUDA device`。** `cuInit` 返回了 100，驱动报告没有设备。容器里要把设备透传进去。
- **探针在 `linux-asan` 下失败，但 `ctest --test-dir build/linux-asan` 才失败，用 `ctest --preset linux-asan` 不失败。** `--test-dir` 绕过了预设，预设里的 `ASAN_OPTIONS` 没有生效。用预设运行。
- **`check-env.sh --gpu` 报 `CMake Error: CMAKE_CXX_COMPILER not set`。** 工具链文件的路径是相对路径，CMake 按别的目录解析了。脚本里的 `script_dir` 现在是绝对路径；自己写类似脚本时要注意。
- **想在 sid 上装 CUDA 工具包。** 先看第 7 节的备忘，真要装时重新核对：Debian 12 的仓库从 CUDA 13.5 起被标为弃用，sid 没有列出，没有验证过。
- **`nvcc` 不在 `PATH` 里。** 这一章不需要它。

## 9. 练习

练习：

1. 运行 `scripts/check-env.sh --gpu --no-smoke`，和不带 `--no-smoke` 的输出比较，找出少了哪一行。
2. 把 `ASAN_OPTIONS` 里的 `protect_shadow_gap=0` 去掉，运行 `ctest --preset linux-asan`，确认只有 `gpu_probe` 失败；再加回来。
3. 用 `ASAN_OPTIONS=protect_shadow_gap=0:verbosity=1` 运行 `build/linux-asan/bin/gpu_probe`，读 ASan 打印的内存布局，看看影子内存和间隙在哪里。
4. 给 `test-check-env.sh` 加一个用例：`nvidia-smi` 的头里同时出现两个 `Version`，确认脚本取的是 `CUDA` 那个。
5. 用 `ldd build/linux-debug/bin/gpu_probe` 看探针依赖了哪些库，找到 `libcuda.so.1`，再用 `readelf -d` 看它是 `NEEDED` 还是 `RUNPATH`。
6. 在容器里（比如 podman）不透传设备运行探针，看是 Skipped 还是 Failed，解释为什么。
7. 想一想：`--gpu` 为什么不检查 `nvcc`？如果要加一个只在装了 `nvcc` 时才做的检查，该写成 `INFO` 还是 `OK`？
8. 读 [platform-1.6](platform-1.6-ci-verify.md) 的规划，想一想 CI 容器里没有显卡，`gpu_probe` 在那里会是什么状态，为什么不会让 CI 失败。

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 platform-1.6 把这一切放进 CI 和本地验证：在 `ubuntu-latest` 的容器里跑环境检查、构建和测试，并给出一条本地命令，在一台 Linux 机器上完整验证这条支线。容器里没有显卡，`gpu_probe` 会是 Disabled 或 Skipped，不影响结果。
