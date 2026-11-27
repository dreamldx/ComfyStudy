# ComfyStudy

[![CI](https://github.com/dreamldx/ComfyStudy/actions/workflows/ci.yml/badge.svg)](https://github.com/dreamldx/ComfyStudy/actions/workflows/ci.yml)

用 C++ 从零实现一个“ComfyUI 式”的图像生成系统：**由 LLM 通过 Tool Calling 一步步调用节点，自己组装流程**，用户通过 Discord 对话，生成的图片作为附件发回。节点与模型推理全部自己实现，并配有一份逐章讲解的中文教程。

> 这是一个学习项目，也是一份教程：代码是教程的配套，每一章对应一个 Git tag，可以随时回到“这一章做完时”的代码。

**在线阅读教程：** https://dreamldx.github.io/ComfyStudy/ （每完成一个章节、推送对应的章节 tag 后更新，内容对应最近完成的章节）

## 特点

- **节点由 C++ 实现**，不依赖 ComfyUI 进程：句柄式即时执行，LLM 每调用一个节点就立刻执行并返回句柄。
- **Skills 按需加载工具**：LLM 起初只看到 Skill 目录，加载后才看到对应的强类型工具。
- **LLM 双协议**：同时支持 OpenAI 与 Anthropic，并支持图像输入，可以看图自我修正。
- **推理自己手写**：用 libtorch 与 ggml 各写一遍 SD 1.5 的 UNet 并逐数对照；进阶线支持 FLUX.1 [schnell]、Qwen-Image，可直接加载 GGUF、fp8 等量化权重。
- **Discord 作为对话通道**，配置放在 `config.toml`（或 `config.ini`），也可以用命令行和环境变量覆盖。
- **构建用 MSVC + CMake（Ninja）**，依赖用 vcpkg。

## 学习路线

### 主线（阶段 0–16）

| 阶段 | 主题 | 完成标准 |
|---|---|---|
| 0 | [工具链与骨架](docs/tutorial/phase-00-toolchain/README.md) | 运行程序时输出已加载的配置项名称，并能列出程序加载的每个 DLL 的位置、版本和所属的 vcpkg 包 |
| 1 | [LLM 与对话通道](docs/tutorial/phase-01-llm-chat/README.md) | 在 Discord 里与机器人对话，它能调用玩具工具并回答，也能看懂用户发来的图片，重启后能接着聊 |
| 2 | [OpenCV：句柄系统与图像处理](docs/tutorial/phase-02-opencv-handles/README.md) | 在 Discord 里说“把这张图缩小一半并加模糊”，机器人自己装载 Skill、调用节点并发回结果图；LLM 还能看到结果图并自行调整重做 |
| 3 | [libtorch 基础与采样数学](docs/tutorial/phase-03-libtorch-sampling/README.md) | 不用任何模型，LLM 驱动采样节点把噪声迭代成可见结果 |
| 4 | [权重加载与 VAE](docs/tutorial/phase-04-weights-vae/README.md) | 一张图经 VAE 编码再解码能重建，与参考值误差在容差内 |
| 5 | [CLIP 文本编码器](docs/tutorial/phase-05-clip/README.md) | 文本编码输出与参考值误差在容差内，`CLIPTextEncode` 节点可用 |
| 6 | [UNet 构件（libtorch）](docs/tutorial/phase-06-unet-blocks/README.md) | 每个构件的输出与参考值对照通过 |
| 7 | [UNet 组装（libtorch）](docs/tutorial/phase-07-unet-assembly/README.md) | 完整 UNet 单步前向与参考值对照通过，并记录耗时与显存基线 |
| 8 | [ggml 入门与 UNet 构件](docs/tutorial/phase-08-ggml-blocks/README.md) | ggml 版 ResBlock 与注意力输出与 libtorch 版对照通过 |
| 9 | [ggml UNet 组装](docs/tutorial/phase-09-ggml-unet/README.md) | ggml 版完整 UNet 与 libtorch 版对照通过，并给出耗时与显存对比 |
| 10 | [整合出图（SD 1.5）](docs/tutorial/phase-10-txt2img/README.md) | Discord 里一句话，LLM 自行装载 Skill、组装节点，发回真实文生图，且可切换后端；也能对一张图做 img2img 与 inpaint。SD 1.5 主线到此完成 |
| 11 | [ONNX Runtime](docs/tutorial/phase-11-onnxruntime/README.md) | 生成后让 LLM 自行放大或抠图 |
| 12 | [Seedream 远程节点](docs/tutorial/phase-12-seedream-remote/README.md) | LLM 能把本地节点与 Seedream 远程节点组合使用 |
| 13 | [stable-diffusion.cpp 后端](docs/tutorial/phase-13-sdcpp-backend/README.md) | 构建出固定提交的 stable-diffusion.cpp，用 `backend=sdcpp` 跑通 SD 1.5 出图 |
| 14 | [DiT 基础与量化格式](docs/tutorial/phase-14-dit-basics/README.md) | 能读取 GGUF 与 fp8 权重并正确反量化，与参考值对照；讲清 MM-DiT 的联合注意力 |
| 15 | [FLUX.1 [schnell]](docs/tutorial/phase-15-flux/README.md) | Discord 里用量化版 FLUX.1 [schnell] 出图 |
| 16 | [Qwen-Image](docs/tutorial/phase-16-qwen-image/README.md) | 用量化版 Qwen-Image 出图，编码器与主模型分段加载 |

### 支线（扩散模型 17 个、LLM 5 个、platform 15 个、paper 14 个）

支线分成**扩散模型**、**LLM** 和 **platform（Linux 版）**、**paper（论文解析）** 四个方向，各自从 01 编号，可整段跳过。扩散模型方向按“实现难易与有趣程度”从易到难排序；LLM 方向的阶段相互独立。每个支线阶段是一个独立分支（`quest/diffusion-NN-<名字>` 或 `quest/llm-NN-<名字>`），不打 tag、不发 Release，也不合并回 `main`。

#### 扩散模型方向

| 编号 | 主题 | 分支 |
|---|---|---|
| diffusion-01 | [潜空间玩法·入门](docs/contrib/quest-diffusion-01-latent-playground-basic/README.md) | `quest/diffusion-01-latent-playground-basic` |
| diffusion-02 | [UNet 内部观察与实验](docs/contrib/quest-diffusion-02-unet-lab/README.md) | `quest/diffusion-02-unet-lab` |
| diffusion-03 | [潜空间玩法·进阶](docs/contrib/quest-diffusion-03-latent-playground-adv/README.md) | `quest/diffusion-03-latent-playground-adv` |
| diffusion-04 | [条件与提示技术](docs/contrib/quest-diffusion-04-conditioning-prompts/README.md) | `quest/diffusion-04-conditioning-prompts` |
| diffusion-05 | [采样与调度进阶](docs/contrib/quest-diffusion-05-sampling-advanced/README.md) | `quest/diffusion-05-sampling-advanced` |
| diffusion-06 | [UNet 加速与少步采样](docs/contrib/quest-diffusion-06-unet-speedup/README.md) | `quest/diffusion-06-unet-speedup` |
| diffusion-07 | [DiT 内部观察与实验](docs/contrib/quest-diffusion-07-dit-lab/README.md) | `quest/diffusion-07-dit-lab` |
| diffusion-08 | [条件控制与适配器](docs/contrib/quest-diffusion-08-control-adapters/README.md) | `quest/diffusion-08-control-adapters` |
| diffusion-09 | [DiT 适配与编辑](docs/contrib/quest-diffusion-09-dit-adapters/README.md) | `quest/diffusion-09-dit-adapters` |
| diffusion-10 | [训练](docs/contrib/quest-diffusion-10-training/README.md) | `quest/diffusion-10-training` |
| diffusion-11 | [评估与反演](docs/contrib/quest-diffusion-11-eval-inversion/README.md) | `quest/diffusion-11-eval-inversion` |
| diffusion-12 | [DiT 加速](docs/contrib/quest-diffusion-12-dit-speedup/README.md) | `quest/diffusion-12-dit-speedup` |
| diffusion-13 | [推理工程](docs/contrib/quest-diffusion-13-inference-engineering/README.md) | `quest/diffusion-13-inference-engineering` |
| diffusion-14 | [UNet 量化](docs/contrib/quest-diffusion-14-quantization/README.md) | `quest/diffusion-14-quantization` |
| diffusion-15 | [视觉定位与布局规划](docs/contrib/quest-diffusion-15-vision-planning/README.md) | `quest/diffusion-15-vision-planning` |
| diffusion-16 | [SDXL](docs/contrib/quest-diffusion-16-sdxl/README.md) | `quest/diffusion-16-sdxl` |
| diffusion-17 | [后端对照：stable-diffusion.cpp](docs/contrib/quest-diffusion-17-sdcpp-compare/README.md) | `quest/diffusion-17-sdcpp-compare` |

#### LLM 方向

| 编号 | 主题 | 分支 |
|---|---|---|
| llm-01 | [LLM 应用工程](docs/contrib/quest-llm-01-llm-app-engineering/README.md) | `quest/llm-01-llm-app-engineering` |
| llm-02 | [Agent 进阶](docs/contrib/quest-llm-02-agent-advanced/README.md) | `quest/llm-02-agent-advanced` |
| llm-03 | [用 LLM 改进出图](docs/contrib/quest-llm-03-llm-image-craft/README.md) | `quest/llm-03-llm-image-craft` |
| llm-04 | [Skills 进阶](docs/contrib/quest-llm-04-skills-advanced/README.md) | `quest/llm-04-skills-advanced` |
| llm-05 | [沙箱与脚本编排](docs/contrib/quest-llm-05-sandbox/README.md) | `quest/llm-05-sandbox` |

#### platform 方向（Linux 版）

| 编号 | 主题 | 分支 |
|---|---|---|
| platform-01 | [Linux 阶段 0](docs/contrib/quest-platform-01-linux-foundation/README.md) | `quest/platform-01-linux-foundation` |
| platform-02 | [事件循环和网络](docs/contrib/quest-platform-02-event-loop-network/README.md) | `quest/platform-02-event-loop-network` |
| platform-03 | [libtorch（Linux）](docs/contrib/quest-platform-03-libtorch-linux/README.md) | `quest/platform-03-libtorch-linux` |
| platform-04 | [内存映射（Linux）](docs/contrib/quest-platform-04-mmap-linux/README.md) | `quest/platform-04-mmap-linux` |
| platform-05 | [ggml（Linux）](docs/contrib/quest-platform-05-ggml-linux/README.md) | `quest/platform-05-ggml-linux` |
| platform-06 | [性能对比·UNet（Linux 与 Windows）](docs/contrib/quest-platform-06-perf-unet/README.md) | `quest/platform-06-perf-unet` |
| platform-07 | [性能对比·SD 1.5 出图](docs/contrib/quest-platform-07-perf-sd15/README.md) | `quest/platform-07-perf-sd15` |
| platform-08 | [ONNX Runtime（Linux）](docs/contrib/quest-platform-08-onnxruntime-linux/README.md) | `quest/platform-08-onnxruntime-linux` |
| platform-09 | [性能对比·FLUX](docs/contrib/quest-platform-09-perf-flux/README.md) | `quest/platform-09-perf-flux` |
| platform-10 | [性能对比·Qwen-Image](docs/contrib/quest-platform-10-perf-qwen-image/README.md) | `quest/platform-10-perf-qwen-image` |
| platform-11 | [源码构建与官方版本对比（Linux）](docs/contrib/quest-platform-11-source-build/README.md) | `quest/platform-11-source-build` |
| platform-12 | [GCC 与 Clang 的编译器性能优化](docs/contrib/quest-platform-12-gcc-clang/README.md) | `quest/platform-12-gcc-clang` |
| platform-13 | [Agent 进阶（Linux 完整版）](docs/contrib/quest-platform-13-agent-advanced-linux/README.md) | `quest/platform-13-agent-advanced-linux` |
| platform-14 | [沙箱与脚本编排（Linux 完整版）](docs/contrib/quest-platform-14-sandbox-linux/README.md) | `quest/platform-14-sandbox-linux` |
| platform-15 | [Skills 进阶（Linux 完整版）](docs/contrib/quest-platform-15-skills-advanced-linux/README.md) | `quest/platform-15-skills-advanced-linux` |

#### paper 方向（论文解析）

| 编号 | 主题 | 分支 |
|---|---|---|
| paper-01 | [LLM 与 Agent](docs/contrib/quest-paper-01-llm-agent/README.md) | `quest/paper-01-llm-agent` |
| paper-02 | [扩散模型基础](docs/contrib/quest-paper-02-diffusion-basics/README.md) | `quest/paper-02-diffusion-basics` |
| paper-03 | [潜空间与 VAE](docs/contrib/quest-paper-03-latent-vae/README.md) | `quest/paper-03-latent-vae` |
| paper-04 | [Transformer 与文本编码](docs/contrib/quest-paper-04-transformer-text/README.md) | `quest/paper-04-transformer-text` |
| paper-05 | [UNet 与注意力](docs/contrib/quest-paper-05-unet-attention/README.md) | `quest/paper-05-unet-attention` |
| paper-06 | [引导、适配与编辑](docs/contrib/quest-paper-06-guidance-editing/README.md) | `quest/paper-06-guidance-editing` |
| paper-07 | [超分](docs/contrib/quest-paper-07-super-resolution/README.md) | `quest/paper-07-super-resolution` |
| paper-08 | [DiT 与流匹配](docs/contrib/quest-paper-08-dit-flow/README.md) | `quest/paper-08-dit-flow` |
| paper-09 | [FLUX 与蒸馏](docs/contrib/quest-paper-09-flux-distillation/README.md) | `quest/paper-09-flux-distillation` |
| paper-10 | [Qwen-Image 与大语言模型](docs/contrib/quest-paper-10-qwen-llm/README.md) | `quest/paper-10-qwen-llm` |
| paper-11 | [采样与引导进阶（扩散支线）](docs/contrib/quest-paper-11-sampling-guidance/README.md) | `quest/paper-11-sampling-guidance` |
| paper-12 | [条件控制与编辑（扩散支线）](docs/contrib/quest-paper-12-control-editing/README.md) | `quest/paper-12-control-editing` |
| paper-13 | [加速与量化（扩散支线）](docs/contrib/quest-paper-13-speedup-quant/README.md) | `quest/paper-13-speedup-quant` |
| paper-14 | [评估与 SDXL（扩散支线）](docs/contrib/quest-paper-14-evaluation-sdxl/README.md) | `quest/paper-14-evaluation-sdxl` |

完整目录、每章的 tag 见 [教程目录](docs/tutorial/README.md)；支线阶段和与主线的衔接见 [支线总览](docs/contrib/README.md)。

## 如何跟读

```text
git clone https://github.com/dreamldx/ComfyStudy
cd ComfyStudy
git fetch --tags
git tag -l "chapter-*"
git checkout chapter-0.0-repo-init
```

- **章节 tag**：主线每一章一个，形如 `chapter-2.2-node-core`。`git checkout <tag>` 就能看到那一章完成时的全部代码。
- **阶段 tag 与 Release**：每个主线阶段**开始时**打一个阶段 tag，形如 `phase-02-opencv-handles`，同时创建一个预发布的 [Release](https://github.com/dreamldx/ComfyStudy/releases)；阶段完成后这个 Release 转为正式发布，并列出本阶段的章节清单。
- 回到最新进度：`git checkout main`。

## 开发环境

Windows 11；**PowerShell**（必需的开发 shell）；MSVC（Visual Studio 18）；CMake（3.25 以上）与 Ninja；vcpkg（用 scoop 装：`scoop install vcpkg`）；NVIDIA GPU 与 CUDA 13（开发机为 RTX 4090 24GB）；Discord 机器人应用与令牌；OpenAI 或 Anthropic 的 API 密钥。具体版本随章节记录在教程里。

### 辅助脚本

开发环境**强制使用 PowerShell**（Windows 自带的 Windows PowerShell 5.1 即可，PowerShell 7 也可以）。`scripts/` 下的脚本都是 PowerShell 脚本（`make-site-nav.py` 是唯一的 Python 脚本），在 PowerShell 里**直接运行**，不用再套一层 `powershell` 命令，也不支持在 cmd 或 Git Bash 里运行：

```text
.\scripts\verify.ps1
```

- 脚本用自己的位置找仓库根目录，所以在哪个目录运行都可以，不在仓库根目录就写完整路径。
- 如果提示“禁止运行脚本”，在 PowerShell 里执行一次 `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`。
- 除 `env.ps1` 外，脚本运行结束后会还原你的 PowerShell 会话：环境变量和当前目录都不会被改动。

`scripts/` 下的脚本是开发和验证用的辅助工具，不属于教程内容：

| 脚本 | 作用 | 用法 |
|---|---|---|
| `env.ps1` | 把**当前 PowerShell 会话**切换到 MSVC x64 编译环境：定位 Visual Studio，运行 `vcvars64.bat`，并把环境变量导入当前会话；再把 `VCPKG_ROOT` 设成 scoop 装的 vcpkg（没装就报错，提示 `scoop install vcpkg`），项目只用这一个 vcpkg，不用 Visual Studio 自带的（原因见 0.3 的“常见问题与坑”）。可以重复运行 | `. .\scripts\env.ps1`（注意开头的点和空格，称为“点源”），成功会打印 `MSVC x64 environment ready` |
| `dev.ps1` | 在 MSVC 环境里运行任意命令，并先进入仓库根目录；命令结束后还原会话，所以不必先运行 `env.ps1` | `.\scripts\dev.ps1 cmake --preset windows-debug` |
| `check-env.ps1` | 检查需要的工具是否齐全：`cl`（x64）、`link`、`dumpbin`、`cmake`（3.25 以上）、`ninja`、`git`、`vcpkg`、`powershell`，缺哪个就列出哪个；最后多一行 `INFO VCPKG_ROOT`，显示用的 vcpkg 在哪里 | 先 `. .\scripts\env.ps1`，再 `.\scripts\check-env.ps1` |
| `check-build.ps1` | 验证 CMake 构建的行为：Debug 与 Release 分别构建；第二次构建无事可做；改一个头文件只重编包含它的源文件；Release 不碰 Debug 的文件；两者的运行库不同；`clean` 目标有效 | 先 `. .\scripts\env.ps1`，再 `.\scripts\check-build.ps1` |
| `check-vs.ps1` | 验证 `windows-vs` 预设：能生成 Visual Studio 解决方案和项目文件，`comfystudy` 是启动项目，能构建，且程序输出正确 | 先 `. .\scripts\env.ps1`，再 `.\scripts\check-vs.ps1` |
| `check-docs.ps1` | 检查教程章节：状态行合法、已完成的章节九个小节齐全、`<!-- src: -->` 标注的代码片段与源码一致（已打 tag 的章节按 tag 当时的源码比对）、支线章节的分支名正确。加 `-CheckTags` 还会检查章节 tag 与阶段 tag 是否真的存在 | `.\scripts\check-docs.ps1 [-CheckTags]` |
| `verify.ps1` | 总验证入口：依次运行上面的检查（有对应脚本才运行，测试加入后还会运行 `ctest`），任何一步失败就停下并指出是哪一步；每个阶段在独立的 PowerShell 进程里运行，并自己设置 MSVC 环境。每个 tag 上的代码都应通过它 | `.\scripts\verify.ps1 [-CheckTags]` |
| `make-site-nav.py` | 从各阶段 README 里的章节表生成教程网站的导航文件 `mkdocs.generated.yml`，下面的本地预览和 GitHub Pages 构建都会用到 | `python scripts/make-site-nav.py` |

**执行了 `env.ps1` 却找不到 `cl` 或 `link`？** `env.ps1` 只改变**当前这个 PowerShell 进程**的环境变量，下面几种情况会让它看起来“没有效果”：

- `cl`、`cmake` 等命令不是在**同一个 PowerShell 会话**里运行的（比如开了另一个窗口）。
- 在 cmd 或 Git Bash 里启动了 `powershell` 去运行它：那是一个子进程，退出后环境就丢了。
- 在 Claude Code 里用 `!` 前缀执行：每条命令都是一个独立的 shell，上一条设置的环境不会留给下一条。

办法：在同一个 PowerShell 窗口里先运行 `. .\scripts\env.ps1`，再运行后面的命令；或者每条命令前面加 `.\scripts\dev.ps1`（例如 `.\scripts\dev.ps1 cl`）；或者直接使用开始菜单里的 “Developer PowerShell for VS”，它已经设置好了环境。

## 本地预览教程

```text
pip install -r requirements-docs.txt
python scripts/make-site-nav.py
mkdocs serve -f mkdocs.generated.yml
```

## 当前进度

阶段 0「工具链与骨架」已完成（0.0 至 0.6），阶段 1 尚未开始。

## 仓库结构

| 路径 | 内容 |
|---|---|
| `docs/tutorial/` | 教程（主线）：每个阶段一个文件夹，每章一个文件 |
| `docs/contrib/` | 支线：与教程相同的结构，扩散模型、LLM、platform 和 paper 四个方向 |
| `src/`、`tests/` | 源码与测试（随章节逐步加入） |

## 许可证

[MIT](LICENSE)，版权所有 © 2026 dreamldx。

教程中提到的模型权重不在本仓库内，也不受本许可证约束，使用时请遵守各自的许可协议。