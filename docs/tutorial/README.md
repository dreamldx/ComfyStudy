# ComfyStudy 教程

用 C++（MSVC + CMake）从零实现一个由 LLM 通过 Tool Calling 驱动的图像生成系统，
通过 Discord 对话，节点与模型推理全部自己实现。这是一份**教程**：每一步都有一章，
主线每一章对应一个 git tag，每个主线阶段开始时还有一个阶段 tag，可以 checkout 到任一 tag 对照代码；附录的代码放在独立分支，不打 tag。

**当前状态：阶段 0 编写中（0.0、0.1 已完成），其余章节尚未开始，仅有骨架。** 阶段 17–30 是选读附录，放在主线（0–16）之后，按“实现难易与有趣程度”从易到难、从有趣到次要排序，可整段跳过；除阶段 0–10 与 30 外均为草案，到该阶段前再细化。

## 如何跟读
- 主线章节头部写明对应的 tag 与上一个 tag。`git checkout <tag>` 即可看到该章完成时的代码；每个主线阶段开始时有一个阶段 tag（如 `phase-00-toolchain`），完成后在 GitHub 上发布一个 Release。
- 附录章节头部写明所在分支（`appendix/NN-slug`），用 `git log <分支> --grep="^chapter-N.M"` 定位本章提交；分支的起点见 `reference/appendix-links.md`。
- 每个 tag 处代码都能编译并通过该章的测试。
- 仓库只有一位维护者：tag 在必要时会被移动并同步更新章节，以章节里写的 tag 为准。
- 标题带“（选读）”的章节可以跳过，主线章节不依赖它们。
- 教程用中文写，代码标识符与注释用英文。

## 环境要求（预期，随阶段核实）
Windows 11；MSVC（VS 18）；CMake（3.25 以上）与 Ninja；vcpkg；NVIDIA GPU（开发机为 RTX 4090 24GB）与 CUDA 13；
Discord 机器人应用与令牌；OpenAI 或 Anthropic 的 API 密钥。详见 `reference/library-versions.md`。

## 阶段目录

| 阶段 | 主题 | 类型 | 章节数 | 完成标准 |
|---|---|---|---|---|
| 0 | [工具链与骨架](phase-00-toolchain/README.md) | 主线 | 6 | 运行程序时输出已加载的配置项名称，并通过 DLL 自检 |
| 1 | [LLM 与对话通道](phase-01-llm-chat/README.md) | 主线 | 5 | 在 Discord 里与机器人对话，它能调用玩具工具并回答，也能看懂用户发来的图片 |
| 2 | [OpenCV：句柄系统与图像处理](phase-02-opencv-handles/README.md) | 主线 | 7 | 在 Discord 里说“把这张图缩小一半并加模糊”，机器人自己装载 Skill、调用节点并发回结果图；LLM 还能看到结果图并自行调整重做 |
| 3 | [libtorch 基础与采样数学](phase-03-libtorch-sampling/README.md) | 主线 | 7 | 不用任何模型，LLM 驱动采样节点把噪声迭代成可见结果 |
| 4 | [权重加载与 VAE](phase-04-weights-vae/README.md) | 主线 | 8 | 一张图经 VAE 编码再解码能重建，与参考值误差在容差内 |
| 5 | [CLIP 文本编码器](phase-05-clip/README.md) | 主线 | 4 | 文本编码输出与参考值误差在容差内，`CLIPTextEncode` 节点可用 |
| 6 | [UNet 构件（libtorch）](phase-06-unet-blocks/README.md) | 主线 | 6 | 每个构件的输出与参考值对照通过 |
| 7 | [UNet 组装（libtorch）](phase-07-unet-assembly/README.md) | 主线 | 5 | 完整 UNet 单步前向与参考值对照通过，并记录耗时与显存基线 |
| 8 | [ggml 入门与 UNet 构件](phase-08-ggml-blocks/README.md) | 主线 | 6 | ggml 版 ResBlock 与注意力输出与 libtorch 版对照通过 |
| 9 | [ggml UNet 组装](phase-09-ggml-unet/README.md) | 主线 | 4 | ggml 版完整 UNet 与 libtorch 版对照通过，并给出耗时与显存对比 |
| 10 | [整合出图（SD 1.5）](phase-10-txt2img/README.md) | 主线 | 7 | Discord 里一句话，LLM 自行装载 Skill、组装节点，发回真实文生图，且可切换后端；也能对一张图做 img2img 与 inpaint。SD 1.5 主线到此完成 |
| 11 | [DiT 基础与量化格式](phase-11-dit-basics/README.md) | 主线·草案 | 11 | 能读取 GGUF 与 fp8 权重并正确反量化，与参考值对照 |
| 12 | [FLUX.1 [schnell]](phase-12-flux/README.md) | 主线·草案 | 9 | Discord 里用量化版 FLUX.1 [schnell] 出图 |
| 13 | [Qwen-Image](phase-13-qwen-image/README.md) | 主线·草案 | 6 | 用量化版 Qwen-Image 出图，编码器与主模型分段加载 |
| 14 | [Seedream 远程节点](phase-14-seedream-remote/README.md) | 主线·草案 | 3 | LLM 能把本地节点与 Seedream 远程节点组合使用 |
| 15 | [stable-diffusion.cpp 作为第二个后端](phase-15-sdcpp-backend/README.md) | 主线·草案 | 4 | 同一提示词在自研后端与 stable-diffusion.cpp 后端出图并对比 |
| 16 | [ONNX Runtime](phase-16-onnxruntime/README.md) | 主线·草案 | 5 | 生成后让 LLM 自行放大或抠图 |
| 17 | [潜空间玩法·入门（选读）](phase-17-latent-playground-basic/README.md) | 选读·草案 | 9 | 对同一提示词做噪声插值动画与无缝平铺纹理，并逐步预览去噪过程 |
| 18 | [UNet 内部观察与实验（选读）](phase-18-unet-lab/README.md) | 选读·草案 | 6 | 在 Discord 里让 LLM 分析某个词影响了画面哪里，并做逐层消融实验、写出报告 |
| 19 | [潜空间玩法·进阶（选读）](phase-19-latent-playground-adv/README.md) | 选读·草案 | 7 | 生成首尾相接的全景图、沿语义方向调节属性，并用梯度引导控制颜色 |
| 20 | [条件与提示技术（选读）](phase-20-conditioning-prompts/README.md) | 选读·草案 | 4 | 用强调语法、CLIP skip 与条件组合精细控制提示词 |
| 21 | [采样与调度进阶（选读）](phase-21-sampling-advanced/README.md) | 选读·草案 | 7 | 让 LLM 自己跑采样器对比实验，并逐步预览去噪轨迹 |
| 22 | [UNet 加速与少步采样（选读）](phase-22-unet-speedup/README.md) | 选读·草案 | 4 | 同一提示词在基线、DeepCache、ToMe、LCM 下对比速度与画质 |
| 23 | [DiT 内部观察与实验（选读）](phase-23-dit-lab/README.md) | 选读·草案 | 4 | 在 Discord 里让 LLM 分析某个词在联合注意力里看了画面哪里，并做逐块消融实验、写出报告 |
| 24 | [条件控制与适配器（选读）](phase-24-control-adapters/README.md) | 选读·草案 | 6 | 用边缘图控制构图，并用参考图或编辑注意力图改变画面 |
| 25 | [DiT 适配与编辑（选读）](phase-25-dit-adapters/README.md) | 选读·草案 | 4 | 用参考图让模型“看着原图改”，并给量化模型叠加 LoRA |
| 26 | [训练（选读）](phase-26-training/README.md) | 选读·草案 | 6 | 用少量图训练一个 LoRA 或一个新词嵌入，并在 Discord 生成对比 |
| 27 | [评估与反演（选读）](phase-27-eval-inversion/README.md) | 选读·草案 | 6 | 用指标评估出图差异，并把真实图反演回噪声后用提示词编辑 |
| 28 | [DiT 加速（选读）](phase-28-dit-speedup/README.md) | 选读·草案 | 4 | 同一提示词在基线、特征缓存、Token 合并下对比速度与画质 |
| 29 | [推理工程（选读）](phase-29-inference-engineering/README.md) | 选读·草案 | 5 | 剖析 UNet 的成本，并用批合并、精度开关与 CUDA Graph 优化，保证跨后端结果可复现 |
| 30 | [UNet 量化（选读）](phase-30-quantization/README.md) | 选读 | 9 | `unet_precision` 可选 fp16、int8、int4，出图差异与显存可量化比较 |

## 目录结构

- `phase-NN-<slug>/`：每个阶段一个文件夹，含 `README.md`（阶段总览）与每章一个文件 `N.M-<slug>.md`。
- `reference/`：跨章的技术参考（库版本、DLL 清单、类型系统、API 速查、术语表、附录与主线的衔接表）。
- `_template/chapter.md`：章节模板。
