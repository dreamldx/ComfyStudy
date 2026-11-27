# ComfyStudy 教程

用 C++（MSVC + CMake）从零实现一个由 LLM 通过 Tool Calling 驱动的图像生成系统，
通过 Discord 对话，节点与模型推理全部自己实现。这是一份**教程**：每一步都有一章，
主线每一章对应一个 git tag，每个主线阶段开始时还有一个阶段 tag，可以 checkout 到任一 tag 对照代码；支线的代码放在独立分支，不打 tag。

**当前状态：阶段 0 已完成（0.0 至 0.6），阶段 1 进行中（已完成 1.1 至 1.5），其余章节尚未开始，仅有骨架。** 支线（扩散模型、LLM、platform、paper 四个方向）放在主线（0–16）之后，可整段跳过。

## 如何跟读
- 主线章节头部写明对应的 tag 与上一个 tag。`git checkout <tag>` 即可看到该章完成时的代码；每个主线阶段开始时有一个阶段 tag（如 `phase-00-toolchain`），完成后在 GitHub 上发布一个 Release。
- 支线章节在 [`docs/contrib/`](../contrib/README.md) 里，头部写明所在分支，用 `git log <分支> --grep="^chapter-diffusion-N.M"` 定位本章提交，分支的起点见 [支线与主线的衔接表](../contrib/reference/quest-links.md)。
- 每个 tag 处代码都能编译并通过该章的测试。
- 仓库只有一位维护者：tag 在必要时会被移动并同步更新章节，以章节里写的 tag 为准。
- 支线章节可以跳过，主线章节不依赖它们。
- 教程用中文写，代码标识符与注释用英文。

## 环境要求（预期，随阶段核实）
Windows 11；MSVC（VS 18）；CMake（3.25 以上）与 Ninja；vcpkg（scoop 安装）；NVIDIA GPU（开发机为 RTX 4090 24GB）与 CUDA 13；
Discord 机器人应用与令牌；OpenAI 或 Anthropic 的 API 密钥。详见 `reference/library-versions.md`。

## 阶段目录

| 阶段 | 主题 | 类型 | 章节数 | 完成标准 |
|---|---|---|---|---|
| 0 | [工具链与骨架](phase-00-toolchain/README.md) | 主线 | 6 | 运行程序时输出已加载的配置项名称，并能列出程序加载的每个 DLL 的位置、版本和所属的 vcpkg 包 |
| 1 | [LLM 与对话通道](phase-01-llm-chat/README.md) | 主线 | 9 | 在 Discord 里与机器人对话，它能调用玩具工具并回答，也能看懂用户发来的图片，重启后能接着聊 |
| 2 | [OpenCV：句柄系统与图像处理](phase-02-opencv-handles/README.md) | 主线 | 7 | 在 Discord 里说“把这张图缩小一半并加模糊”，机器人自己装载 Skill、调用节点并发回结果图；LLM 还能看到结果图并自行调整重做 |
| 3 | [libtorch 基础与采样数学](phase-03-libtorch-sampling/README.md) | 主线 | 7 | 不用任何模型，LLM 驱动采样节点把噪声迭代成可见结果 |
| 4 | [权重加载与 VAE](phase-04-weights-vae/README.md) | 主线 | 8 | 一张图经 VAE 编码再解码能重建，与参考值误差在容差内 |
| 5 | [CLIP 文本编码器](phase-05-clip/README.md) | 主线 | 4 | 文本编码输出与参考值误差在容差内，`CLIPTextEncode` 节点可用 |
| 6 | [UNet 构件（libtorch）](phase-06-unet-blocks/README.md) | 主线 | 6 | 每个构件的输出与参考值对照通过 |
| 7 | [UNet 组装（libtorch）](phase-07-unet-assembly/README.md) | 主线 | 5 | 完整 UNet 单步前向与参考值对照通过，并记录耗时与显存基线 |
| 8 | [ggml 入门与 UNet 构件](phase-08-ggml-blocks/README.md) | 主线 | 6 | ggml 版 ResBlock 与注意力输出与 libtorch 版对照通过 |
| 9 | [ggml UNet 组装](phase-09-ggml-unet/README.md) | 主线 | 4 | ggml 版完整 UNet 与 libtorch 版对照通过，并给出耗时与显存对比 |
| 10 | [整合出图（SD 1.5）](phase-10-txt2img/README.md) | 主线 | 7 | Discord 里一句话，LLM 自行装载 Skill、组装节点，发回真实文生图，且可切换后端；也能对一张图做 img2img 与 inpaint。SD 1.5 主线到此完成 |
| 11 | [ONNX Runtime](phase-11-onnxruntime/README.md) | 主线·草案 | 5 | 生成后让 LLM 自行放大或抠图 |
| 12 | [Seedream 远程节点](phase-12-seedream-remote/README.md) | 主线·草案 | 3 | LLM 能把本地节点与 Seedream 远程节点组合使用 |
| 13 | [stable-diffusion.cpp 后端](phase-13-sdcpp-backend/README.md) | 主线·草案 | 2 | 构建出固定提交的 stable-diffusion.cpp，用 `backend=sdcpp` 跑通 SD 1.5 出图 |
| 14 | [DiT 基础与量化格式](phase-14-dit-basics/README.md) | 主线·草案 | 12 | 能读取 GGUF 与 fp8 权重并正确反量化，与参考值对照；讲清 MM-DiT 的联合注意力 |
| 15 | [FLUX.1 [schnell]](phase-15-flux/README.md) | 主线·草案 | 8 | Discord 里用量化版 FLUX.1 [schnell] 出图 |
| 16 | [Qwen-Image](phase-16-qwen-image/README.md) | 主线·草案 | 6 | 用量化版 Qwen-Image 出图，编码器与主模型分段加载 |

## 支线

支线放在主线（0–16）之后，可整段跳过，分成**扩散模型**（17 个阶段）、**LLM**（5 个阶段）、**platform**（Linux 版，14 个阶段）和 **paper**（论文解析，14 个阶段）四个方向，放在与教程平级的 [`docs/contrib/`](../contrib/README.md)，结构和本目录相同。阶段目录、每章的分支和与主线的衔接见 [支线总览](../contrib/README.md)。

## 目录结构

- `phase-NN-<slug>/`：每个主线阶段一个文件夹，含 `README.md`（阶段总览）与每章一个文件 `N.M-<slug>.md`。
- `reference/`：跨章的技术参考（库版本、DLL 清单、类型系统、API 速查、术语表）。
- `_template/chapter.md`：章节模板。
