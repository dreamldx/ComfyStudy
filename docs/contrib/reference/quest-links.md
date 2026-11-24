# 支线与主线的衔接表

状态：由设计自动整理，随章节调整更新

## 支线分支

支线分成扩散模型（`diffusion-NN`）、LLM（`llm-NN`）和 platform（`platform-NN`，Linux 版）三个方向，各自从 01 编号。每个支线阶段一个分支，不打 tag、不发 Release，也不合并回 main 分支。

| 支线 | 分支 | 起点 |
|---|---|---|
| diffusion-01 | `quest/diffusion-01-latent-playground-basic` | 主线 tag `chapter-10.7-inpaint-outpaint` |
| diffusion-02 | `quest/diffusion-02-unet-lab` | 主线 tag `chapter-10.4-skills-discord-demo` |
| diffusion-03 | `quest/diffusion-03-latent-playground-adv` | 分支 `quest/diffusion-02-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中） |
| diffusion-04 | `quest/diffusion-04-conditioning-prompts` | 主线 tag `chapter-10.4-skills-discord-demo` |
| diffusion-05 | `quest/diffusion-05-sampling-advanced` | 分支 `quest/diffusion-02-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.3-txt2img-assembly`，已包含在该分支的历史中） |
| diffusion-06 | `quest/diffusion-06-unet-speedup` | 主线 tag `chapter-10.5-lora` |
| diffusion-07 | `quest/diffusion-07-dit-lab` | 主线 tag `chapter-15.7-flux-txt2img` |
| diffusion-08 | `quest/diffusion-08-control-adapters` | 分支 `quest/diffusion-02-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中） |
| diffusion-09 | `quest/diffusion-09-dit-adapters` | 主线 tag `chapter-15.7-flux-txt2img` |
| diffusion-10 | `quest/diffusion-10-training` | 主线 tag `chapter-15.7-flux-txt2img` |
| diffusion-11 | `quest/diffusion-11-eval-inversion` | 分支 `quest/diffusion-08-control-adapters` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中） |
| diffusion-12 | `quest/diffusion-12-dit-speedup` | 主线 tag `chapter-15.7-flux-txt2img` |
| diffusion-13 | `quest/diffusion-13-inference-engineering` | 主线 tag `chapter-10.2-ksampler-cfg` |
| diffusion-14 | `quest/diffusion-14-quantization` | 主线 tag `chapter-10.3-txt2img-assembly` |
| diffusion-15 | `quest/diffusion-15-vision-planning` | 分支 `quest/llm-01-llm-app-engineering` 的末端，再合入主线 tag `chapter-10.7-inpaint-outpaint`（15.7 用 llm-1.10 的录制回放；llm-01 的起点比 10.7 早，所以要另外合入） |
| diffusion-16 | `quest/diffusion-16-sdxl` | 主线 tag `chapter-10.7-inpaint-outpaint` |
| diffusion-17 | `quest/diffusion-17-sdcpp-compare` | 主线 tag `chapter-16.6-qwen-txt2img` |
| llm-01 | `quest/llm-01-llm-app-engineering` | 主线 tag `chapter-1.8-chat-history` |
| llm-02 | `quest/llm-02-agent-advanced` | 主线 tag `chapter-2.7-vision-feedback-loop` |
| llm-03 | `quest/llm-03-llm-image-craft` | 主线 tag `chapter-10.4-skills-discord-demo` |
| llm-04 | `quest/llm-04-skills-advanced` | 分支 `quest/llm-01-llm-app-engineering` 的末端，再合入主线 tag `chapter-2.6-workflow-recording`（4.8 用 llm-1.10 的录制回放；4.4 还需要 llm-05 的沙箱，见下面的“交错推进”） |
| llm-05 | `quest/llm-05-sandbox` | 分支 `quest/llm-01-llm-app-engineering` 的末端（本阶段所需的最晚主线节点是 `chapter-1.3-tool-calling`，已包含在该分支的历史中；5.7 用 llm-1.10 的录制回放） |
| platform-01 | `quest/platform-01-linux-foundation` | 主线 tag `chapter-0.5-dll-management`（主线的 CMake 已按系统拆成平台文件，见 0.1） |
| platform-02 | `quest/platform-02-libtorch-linux` | 分支 `quest/platform-01-linux-foundation` 的末端，再合入主线 tag `chapter-3.1-libtorch-setup` |
| platform-03 | `quest/platform-03-mmap-linux` | 分支 `quest/platform-02-libtorch-linux` 的末端，再合入主线 tag `chapter-4.1-safetensors-parser` |
| platform-04 | `quest/platform-04-ggml-linux` | 分支 `quest/platform-03-mmap-linux` 的末端，再合入主线 tag `chapter-8.1-ggml-build-cuda` |
| platform-05 | `quest/platform-05-perf-unet` | 分支 `quest/platform-04-ggml-linux` 的末端，再合入主线 tag `chapter-9.4-ggml-vs-libtorch-perf` |
| platform-06 | `quest/platform-06-perf-sd15` | 分支 `quest/platform-04-ggml-linux` 的末端，再合入主线 tag `chapter-10.7-inpaint-outpaint` |
| platform-07 | `quest/platform-07-onnxruntime-linux` | 分支 `quest/platform-04-ggml-linux` 的末端，再合入主线 tag `chapter-11.1-ort-setup` |
| platform-08 | `quest/platform-08-perf-flux` | 分支 `quest/platform-07-onnxruntime-linux` 的末端，再合入主线 tag `chapter-15.7-flux-txt2img` |
| platform-09 | `quest/platform-09-perf-qwen-image` | 分支 `quest/platform-07-onnxruntime-linux` 的末端，再合入主线 tag `chapter-16.6-qwen-txt2img`（tag 名按章节文件名推断） |
| platform-10 | `quest/platform-10-source-build` | 分支 `quest/platform-07-onnxruntime-linux` 的末端，再合入主线 tag `chapter-13.2-backend-interface` |
| platform-11 | `quest/platform-11-gcc-clang` | 分支 `quest/platform-07-onnxruntime-linux` 的末端（不需要新的主线 tag） |
| platform-12 | `quest/platform-12-agent-advanced-linux` | 分支 `quest/llm-02-agent-advanced` 的末端，再合入分支 `quest/platform-01-linux-foundation` 的末端 |
| platform-13 | `quest/platform-13-sandbox-linux` | 分支 `quest/llm-05-sandbox` 的末端，再合入分支 `quest/platform-01-linux-foundation` 的末端 |
| platform-14 | `quest/platform-14-skills-advanced-linux` | 分支 `quest/llm-04-skills-advanced` 的末端，再合入分支 `quest/platform-01-linux-foundation` 的末端（4.4、4.5 还需要 platform-13 的沙箱） |
| paper-01 | `quest/paper-01-llm-agent` | 主线 tag `chapter-1.3-tool-calling`（只有文档，不含代码） |
| paper-02 | `quest/paper-02-diffusion-basics` | 主线 tag `chapter-3.6-euler-sampler`（只有文档，不含代码） |
| paper-03 | `quest/paper-03-latent-vae` | 主线 tag `chapter-10.3-txt2img-assembly`（只有文档，不含代码） |
| paper-04 | `quest/paper-04-transformer-text` | 主线 tag `chapter-5.4-clip-verify-node`（只有文档，不含代码） |
| paper-05 | `quest/paper-05-unet-attention` | 主线 tag `chapter-7.5-attention-optimization`（只有文档，不含代码） |
| paper-06 | `quest/paper-06-guidance-editing` | 主线 tag `chapter-10.7-inpaint-outpaint`（只有文档，不含代码） |
| paper-07 | `quest/paper-07-super-resolution` | 主线 tag `chapter-11.2-upscale-node`（只有文档，不含代码） |
| paper-08 | `quest/paper-08-dit-flow` | 主线 tag `chapter-14.12-repa-reading`（只有文档，不含代码） |
| paper-09 | `quest/paper-09-flux-distillation` | 主线 tag `chapter-15.6-guidance-distillation`（只有文档，不含代码） |
| paper-10 | `quest/paper-10-qwen-llm` | 主线 tag `chapter-16.3-qwen25vl-encoder`（只有文档，不含代码） |
| paper-11 | `quest/paper-11-sampling-guidance` | 主线 tag `chapter-10.3-txt2img-assembly`（只有文档，不含代码）；对照 diffusion-02、diffusion-03、diffusion-05 的实现，需先做完对应章节 |
| paper-12 | `quest/paper-12-control-editing` | 主线 tag `chapter-10.3-txt2img-assembly`（只有文档，不含代码）；对照 diffusion-08、diffusion-10、diffusion-11 的实现，需先做完对应章节 |
| paper-13 | `quest/paper-13-speedup-quant` | 主线 tag `chapter-15.7-flux-txt2img`（只有文档，不含代码）；对照 diffusion-06、diffusion-12 的实现，需先做完对应章节 |
| paper-14 | `quest/paper-14-evaluation-sdxl` | 主线 tag `chapter-10.7-inpaint-outpaint`（只有文档，不含代码）；对照 diffusion-11、diffusion-16 的实现，需先做完对应章节 |

## 章节衔接

每个支线章节可以在做完哪一个主线章节之后开始。对应主线章节的“支线”一节（第 10 节）里也列出了这些支线入口；没有支线的章节在这一节写“本章节没有可以开启的支线。”。

| 支线章节 | 所属支线 | 可开始于（主线） | 备注 |
|---|---|---|---|
| [diffusion-1.1 逐步快速预览（Latent2RGB）](../quest-diffusion-01-latent-playground-basic/diffusion-1.1-latent-preview.md) | 潜空间玩法·入门 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [diffusion-1.2 无缝平铺纹理](../quest-diffusion-01-latent-playground-basic/diffusion-1.2-seamless-tiling.md) | 潜空间玩法·入门 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-1.3 噪声插值与潜空间动画](../quest-diffusion-01-latent-playground-basic/diffusion-1.3-noise-interpolation.md) | 潜空间玩法·入门 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-1.4 种子变体](../quest-diffusion-01-latent-playground-basic/diffusion-1.4-seed-variation.md) | 潜空间玩法·入门 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-1.5 构图锁定](../quest-diffusion-01-latent-playground-basic/diffusion-1.5-composition-lock.md) | 潜空间玩法·入门 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-1.6 涂鸦转图](../quest-diffusion-01-latent-playground-basic/diffusion-1.6-scribble-to-image.md) | 潜空间玩法·入门 | [10.6 img2img（SDEdit）](../../tutorial/phase-10-txt2img/10.6-img2img.md) |  |
| [diffusion-1.7 潜空间合成与混合](../quest-diffusion-01-latent-playground-basic/diffusion-1.7-latent-compositing.md) | 潜空间玩法·入门 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) |  |
| [diffusion-1.8 高分辨率修复](../quest-diffusion-01-latent-playground-basic/diffusion-1.8-hires-fix.md) | 潜空间玩法·入门 | [10.6 img2img（SDEdit）](../../tutorial/phase-10-txt2img/10.6-img2img.md) |  |
| [diffusion-1.9 latent-lab Skill](../quest-diffusion-01-latent-playground-basic/diffusion-1.9-latent-playground-skill.md) | 潜空间玩法·入门 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) |  |
| [diffusion-2.1 UNet 钩子机制](../quest-diffusion-02-unet-lab/diffusion-2.1-hook-mechanism.md) | UNet 内部观察与实验 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-2.2 特征图可视化](../quest-diffusion-02-unet-lab/diffusion-2.2-feature-map-viz.md) | UNet 内部观察与实验 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-2.3 交叉注意力图可视化](../quest-diffusion-02-unet-lab/diffusion-2.3-cross-attention-maps.md) | UNet 内部观察与实验 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-2.4 逐层消融实验](../quest-diffusion-02-unet-lab/diffusion-2.4-layer-ablation.md) | UNet 内部观察与实验 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-2.5 FreeU](../quest-diffusion-02-unet-lab/diffusion-2.5-freeu.md) | UNet 内部观察与实验 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-2.6 unet-lab Skill](../quest-diffusion-02-unet-lab/diffusion-2.6-unet-lab-skill.md) | UNet 内部观察与实验 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [diffusion-3.1 潜空间语义方向](../quest-diffusion-03-latent-playground-adv/diffusion-3.1-semantic-directions.md) | 潜空间玩法·进阶 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-3.2 TAESD 快速解码](../quest-diffusion-03-latent-playground-adv/diffusion-3.2-taesd.md) | 潜空间玩法·进阶 | [4.7 VAE 节点](../../tutorial/phase-04-weights-vae/4.7-vae-nodes.md) |  |
| [diffusion-3.3 分块解码与分块扩散](../quest-diffusion-03-latent-playground-adv/diffusion-3.3-tiled-vae-diffusion.md) | 潜空间玩法·进阶 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-3.4 全景图](../quest-diffusion-03-latent-playground-adv/diffusion-3.4-panorama.md) | 潜空间玩法·进阶 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-03.3 |
| [diffusion-3.5 区域提示](../quest-diffusion-03-latent-playground-adv/diffusion-3.5-regional-prompting.md) | 潜空间玩法·进阶 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-02.1 |
| [diffusion-3.6 梯度引导](../quest-diffusion-03-latent-playground-adv/diffusion-3.6-gradient-guidance.md) | 潜空间玩法·进阶 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [diffusion-3.7 latent-lab-adv Skill](../quest-diffusion-03-latent-playground-adv/diffusion-3.7-latent-adv-skill.md) | 潜空间玩法·进阶 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [diffusion-4.1 提示词加权](../quest-diffusion-04-conditioning-prompts/diffusion-4.1-prompt-weighting.md) | 条件与提示技术 | [5.4 对照验证与 CLIPTextEncode 节点](../../tutorial/phase-05-clip/5.4-clip-verify-node.md) |  |
| [diffusion-4.2 CLIP skip](../quest-diffusion-04-conditioning-prompts/diffusion-4.2-clip-skip.md) | 条件与提示技术 | [5.4 对照验证与 CLIPTextEncode 节点](../../tutorial/phase-05-clip/5.4-clip-verify-node.md) |  |
| [diffusion-4.3 条件操作](../quest-diffusion-04-conditioning-prompts/diffusion-4.3-conditioning-ops.md) | 条件与提示技术 | [5.4 对照验证与 CLIPTextEncode 节点](../../tutorial/phase-05-clip/5.4-clip-verify-node.md) |  |
| [diffusion-4.4 conditioning-lab Skill](../quest-diffusion-04-conditioning-prompts/diffusion-4.4-conditioning-skill.md) | 条件与提示技术 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [diffusion-5.1 预测目标：ε、v、x0](../quest-diffusion-05-sampling-advanced/diffusion-5.1-prediction-targets.md) | 采样与调度进阶 | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) |  |
| [diffusion-5.2 零终端 SNR 与 trailing 间隔](../quest-diffusion-05-sampling-advanced/diffusion-5.2-zero-terminal-snr.md) | 采样与调度进阶 | [3.5 sigma 调度器](../../tutorial/phase-03-libtorch-sampling/3.5-sigma-schedulers.md) |  |
| [diffusion-5.3 进阶采样器](../quest-diffusion-05-sampling-advanced/diffusion-5.3-advanced-samplers.md) | 采样与调度进阶 | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) |  |
| [diffusion-5.4 CFG 的机理与变体](../quest-diffusion-05-sampling-advanced/diffusion-5.4-cfg-mechanics.md) | 采样与调度进阶 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [diffusion-5.5 PAG 与 SAG 注意力引导](../quest-diffusion-05-sampling-advanced/diffusion-5.5-pag-sag.md) | 采样与调度进阶 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成支线 diffusion-02.1 |
| [diffusion-5.6 去噪轨迹可视化](../quest-diffusion-05-sampling-advanced/diffusion-5.6-denoise-trajectory.md) | 采样与调度进阶 | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) |  |
| [diffusion-5.7 采样器对比实验](../quest-diffusion-05-sampling-advanced/diffusion-5.7-sampler-compare.md) | 采样与调度进阶 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-05.3 |
| [diffusion-6.1 DeepCache](../quest-diffusion-06-unet-speedup/diffusion-6.1-deepcache.md) | UNet 加速与少步采样 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-6.2 Token Merging（ToMe）](../quest-diffusion-06-unet-speedup/diffusion-6.2-token-merging.md) | UNet 加速与少步采样 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-6.3 LCM 与少步采样](../quest-diffusion-06-unet-speedup/diffusion-6.3-lcm.md) | UNet 加速与少步采样 | [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md) |  |
| [diffusion-6.4 速度与画质综合对比](../quest-diffusion-06-unet-speedup/diffusion-6.4-speed-quality-compare.md) | UNet 加速与少步采样 | [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md) | 需先完成支线 diffusion-06.1–diffusion-6.3 |
| [diffusion-7.1 DiT 钩子机制](../quest-diffusion-07-dit-lab/diffusion-7.1-dit-hooks.md) | DiT 内部观察与实验 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-7.2 联合注意力图可视化](../quest-diffusion-07-dit-lab/diffusion-7.2-joint-attention-maps.md) | DiT 内部观察与实验 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-7.3 逐块消融与层跳过](../quest-diffusion-07-dit-lab/diffusion-7.3-block-ablation.md) | DiT 内部观察与实验 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-7.4 dit-lab Skill](../quest-diffusion-07-dit-lab/diffusion-7.4-dit-lab-skill.md) | DiT 内部观察与实验 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-8.1 ControlNet 原理](../quest-diffusion-08-control-adapters/diffusion-8.1-controlnet-principle.md) | 条件控制与适配器 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-8.2 ControlNet 实现与加载](../quest-diffusion-08-control-adapters/diffusion-8.2-controlnet-impl.md) | 条件控制与适配器 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-8.3 ControlNet 与 Canny 节点](../quest-diffusion-08-control-adapters/diffusion-8.3-controlnet-canny.md) | 条件控制与适配器 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-8.4 IP-Adapter](../quest-diffusion-08-control-adapters/diffusion-8.4-ip-adapter.md) | 条件控制与适配器 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-8.5 Prompt-to-Prompt](../quest-diffusion-08-control-adapters/diffusion-8.5-prompt-to-prompt.md) | 条件控制与适配器 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-02.3 |
| [diffusion-8.6 control Skill](../quest-diffusion-08-control-adapters/diffusion-8.6-control-skill.md) | 条件控制与适配器 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [diffusion-9.1 上下文编辑](../quest-diffusion-09-dit-adapters/diffusion-9.1-in-context-editing.md) | DiT 适配与编辑 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-9.2 DiT 的 ControlNet](../quest-diffusion-09-dit-adapters/diffusion-9.2-dit-controlnet.md) | DiT 适配与编辑 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-9.3 量化模型加 LoRA](../quest-diffusion-09-dit-adapters/diffusion-9.3-quantized-lora.md) | DiT 适配与编辑 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-9.4 dit-adapters Skill](../quest-diffusion-09-dit-adapters/diffusion-9.4-dit-adapters-skill.md) | DiT 适配与编辑 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-10.1 自动求导与训练基础](../quest-diffusion-10-training/diffusion-10.1-autograd-basics.md) | 训练 | [3.2 Tensor 与 Latent 值类型](../../tutorial/phase-03-libtorch-sampling/3.2-tensor-latent-values.md) |  |
| [diffusion-10.2 LoRA 训练](../quest-diffusion-10-training/diffusion-10.2-lora-training.md) | 训练 | [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md) |  |
| [diffusion-10.3 Textual Inversion](../quest-diffusion-10-training/diffusion-10.3-textual-inversion.md) | 训练 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-10.4 DiT 的 LoRA 训练](../quest-diffusion-10-training/diffusion-10.4-dit-lora-training.md) | 训练 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) | 需先完成支线 diffusion-10.2 |
| [diffusion-10.5 训练与评估 Skill](../quest-diffusion-10-training/diffusion-10.5-train-eval-skill.md) | 训练 | [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md) |  |
| [diffusion-10.6 模型融合与剪枝介绍（原理阅读）](../quest-diffusion-10-training/diffusion-10.6-model-merging-intro.md) | 训练 | [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md) |  |
| [diffusion-11.1 基础评估指标](../quest-diffusion-11-eval-inversion/diffusion-11.1-basic-metrics.md) | 评估与反演 | [2.4 图像处理节点](../../tutorial/phase-02-opencv-handles/2.4-image-processing-nodes.md) |  |
| [diffusion-11.2 LPIPS 与 CLIPScore](../quest-diffusion-11-eval-inversion/diffusion-11.2-lpips-clipscore.md) | 评估与反演 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-11.3 DDIM 反演](../quest-diffusion-11-eval-inversion/diffusion-11.3-ddim-inversion.md) | 评估与反演 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [diffusion-11.4 null-text 反演](../quest-diffusion-11-eval-inversion/diffusion-11.4-null-text-inversion.md) | 评估与反演 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成支线 diffusion-11.3 |
| [diffusion-11.5 真实图的提示词编辑](../quest-diffusion-11-eval-inversion/diffusion-11.5-real-image-p2p.md) | 评估与反演 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-08.5 与 diffusion-11.3 |
| [diffusion-11.6 eval Skill](../quest-diffusion-11-eval-inversion/diffusion-11.6-eval-skill.md) | 评估与反演 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [diffusion-12.1 特征缓存](../quest-diffusion-12-dit-speedup/diffusion-12.1-feature-caching.md) | DiT 加速 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-12.2 Token 合并与稀疏注意力](../quest-diffusion-12-dit-speedup/diffusion-12.2-token-merge-sparse.md) | DiT 加速 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-12.3 SVDQuant（原理阅读）](../quest-diffusion-12-dit-speedup/diffusion-12.3-svdquant-reading.md) | DiT 加速 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-12.4 速度与画质综合对比](../quest-diffusion-12-dit-speedup/diffusion-12.4-dit-speed-compare.md) | DiT 加速 | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) |  |
| [diffusion-13.1 UNet 剖析器](../quest-diffusion-13-inference-engineering/diffusion-13.1-unet-profiler.md) | 推理工程 | [7.4 性能基线](../../tutorial/phase-07-unet-assembly/7.4-unet-baseline.md) |  |
| [diffusion-13.2 CFG 批合并](../quest-diffusion-13-inference-engineering/diffusion-13.2-cfg-batching.md) | 推理工程 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [diffusion-13.3 内存格式与精度开关](../quest-diffusion-13-inference-engineering/diffusion-13.3-memory-format-precision.md) | 推理工程 | [7.4 性能基线](../../tutorial/phase-07-unet-assembly/7.4-unet-baseline.md) |  |
| [diffusion-13.4 CUDA Graph](../quest-diffusion-13-inference-engineering/diffusion-13.4-cuda-graph.md) | 推理工程 | [7.4 性能基线](../../tutorial/phase-07-unet-assembly/7.4-unet-baseline.md) |  |
| [diffusion-13.5 跨后端可复现](../quest-diffusion-13-inference-engineering/diffusion-13.5-cross-backend-repro.md) | 推理工程 | [9.4 耗时与显存对比](../../tutorial/phase-09-ggml-unet/9.4-ggml-vs-libtorch-perf.md) |  |
| [diffusion-14.1 精度基础](../quest-diffusion-14-quantization/diffusion-14.1-precision-basics.md) | UNet 量化 | [4.8 fp16 数值陷阱](../../tutorial/phase-04-weights-vae/4.8-fp16-pitfalls.md) |  |
| [diffusion-14.2 量化原理](../quest-diffusion-14-quantization/diffusion-14.2-quantization-theory.md) | UNet 量化 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-14.3 加载时 int8 量化](../quest-diffusion-14-quantization/diffusion-14.3-load-time-int8.md) | UNet 量化 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-14.4 加载时 int4 分组量化](../quest-diffusion-14-quantization/diffusion-14.4-load-time-int4.md) | UNet 量化 | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [diffusion-14.5 按层策略与评估](../quest-diffusion-14-quantization/diffusion-14.5-layer-policy-eval.md) | UNet 量化 | [10.1 LoadCheckpoint 节点](../../tutorial/phase-10-txt2img/10.1-load-checkpoint.md) |  |
| [diffusion-14.6 量化敏感度扫描](../quest-diffusion-14-quantization/diffusion-14.6-sensitivity-scan.md) | UNet 量化 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-14.7 与 ggml 原生量化对比](../quest-diffusion-14-quantization/diffusion-14.7-ggml-quant-compare.md) | UNet 量化 | [9.4 耗时与显存对比](../../tutorial/phase-09-ggml-unet/9.4-ggml-vs-libtorch-perf.md) |  |
| [diffusion-14.8 缓存量化文件（更进一步）](../quest-diffusion-14-quantization/diffusion-14.8-quant-cache-file.md) | UNet 量化 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-14.5 |
| [diffusion-14.9 自写融合 CUDA kernel（更进一步）](../quest-diffusion-14-quantization/diffusion-14.9-fused-cuda-kernel.md) | UNet 量化 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-14.5 |
| [diffusion-15.1 视觉定位：让模型给出物体的框](../quest-diffusion-15-vision-planning/diffusion-15.1-visual-grounding.md) | 视觉定位与布局规划 | [1.7 LLM 视觉输入](../../tutorial/phase-01-llm-chat/1.7-vision-input.md) | 各模型的坐标约定与准确度要到时实测 |
| [diffusion-15.2 从框到遮罩](../quest-diffusion-15-vision-planning/diffusion-15.2-box-to-mask.md) | 视觉定位与布局规划 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 依赖 diffusion-15.1；可选依赖 16 的分割节点 |
| [diffusion-15.3 用说的改图](../quest-diffusion-15-vision-planning/diffusion-15.3-edit-by-description.md) | 视觉定位与布局规划 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 依赖 diffusion-15.1、diffusion-15.2 |
| [diffusion-15.4 布局规划](../quest-diffusion-15-vision-planning/diffusion-15.4-layout-planning.md) | 视觉定位与布局规划 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [diffusion-15.5 按布局区域出图](../quest-diffusion-15-vision-planning/diffusion-15.5-regional-generation.md) | 视觉定位与布局规划 | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 依赖 diffusion-15.4；可选：区域提示见 diffusion-3.5（支线），不做则用整体提示 |
| [diffusion-15.6 SVG 草图](../quest-diffusion-15-vision-planning/diffusion-15.6-svg-sketch.md) | 视觉定位与布局规划 | [10.6 img2img（SDEdit）](../../tutorial/phase-10-txt2img/10.6-img2img.md) | 仅对照：与 diffusion-1.6（涂鸦转图）对照，不做也不影响本章 |
| [diffusion-15.7 澄清提问](../quest-diffusion-15-vision-planning/diffusion-15.7-clarifying-questions.md) | 视觉定位与布局规划 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 用 llm-1.10 的录制回放让评测可重复 |
| [diffusion-15.8 visual-edit Skill](../quest-diffusion-15-vision-planning/diffusion-15.8-visual-edit-skill.md) | 视觉定位与布局规划 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) | 依赖 diffusion-15.3、diffusion-15.5 |
| [diffusion-16.1 SDXL 与 SD 1.5 的差异](../quest-diffusion-16-sdxl/diffusion-16.1-sdxl-overview.md) | SDXL | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) |  |
| [diffusion-16.2 双文本编码器](../quest-diffusion-16-sdxl/diffusion-16.2-dual-text-encoders.md) | SDXL | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 用到 5.4 的设计备忘 |
| [diffusion-16.3 尺寸与裁剪条件](../quest-diffusion-16-sdxl/diffusion-16.3-size-crop-conditioning.md) | SDXL | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) |  |
| [diffusion-16.4 SDXL 的 UNet](../quest-diffusion-16-sdxl/diffusion-16.4-sdxl-unet.md) | SDXL | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 用到 7.1 的设计备忘 |
| [diffusion-16.5 键名映射与权重加载](../quest-diffusion-16-sdxl/diffusion-16.5-key-mapping.md) | SDXL | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 用到 4.3 的设计备忘 |
| [diffusion-16.6 SDXL 的 VAE 与 fp16](../quest-diffusion-16-sdxl/diffusion-16.6-sdxl-vae-fp16.md) | SDXL | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 接 4.8（fp16 数值陷阱） |
| [diffusion-16.7 SDXL 文生图](../quest-diffusion-16-sdxl/diffusion-16.7-sdxl-txt2img.md) | SDXL | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) |  |
| [diffusion-17.1 运行 SD 1.5、Flux、Qwen-Image](../quest-diffusion-17-sdcpp-compare/diffusion-17.1-sdcpp-models.md) | 后端对照：stable-diffusion.cpp | [16.6 Qwen-Image 文生图](../../tutorial/phase-16-qwen-image/16.6-qwen-txt2img.md) | 依赖主线阶段 13 的 `backend=sdcpp` 与阶段 15、16 的自研实现 |
| [diffusion-17.2 后端对比](../quest-diffusion-17-sdcpp-compare/diffusion-17.2-backend-comparison.md) | 后端对照：stable-diffusion.cpp | [16.6 Qwen-Image 文生图](../../tutorial/phase-16-qwen-image/16.6-qwen-txt2img.md) | 依赖主线阶段 13 的 `backend=sdcpp` 与阶段 15、16 的自研实现 |
| [llm-1.1 libcurl 三种用法的对比（easy、multi、share）](../quest-llm-01-llm-app-engineering/llm-1.1-curl-modes.md) | LLM 应用工程 | [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md) |  |
| [llm-1.2 事件循环：Boost.Asio 的定时器与协程](../quest-llm-01-llm-app-engineering/llm-1.2-asio-basics.md) | LLM 应用工程 | [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md) |  |
| [llm-1.3 用 Asio 驱动 libcurl multi](../quest-llm-01-llm-app-engineering/llm-1.3-asio-curl.md) | LLM 应用工程 | [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md) | 依赖 llm-1.1、llm-1.2 |
| [llm-1.4 流式输出（SSE）](../quest-llm-01-llm-app-engineering/llm-1.4-streaming-sse.md) | LLM 应用工程 | [1.2 LLM 双协议与 Provider 抽象](../../tutorial/phase-01-llm-chat/1.2-llm-providers.md) |  |
| [llm-1.5 结构化输出与 JSON Schema 校验](../quest-llm-01-llm-app-engineering/llm-1.5-structured-output.md) | LLM 应用工程 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) |  |
| [llm-1.6 上下文窗口与历史压缩](../quest-llm-01-llm-app-engineering/llm-1.6-context-window.md) | LLM 应用工程 | [1.5 上下文规划与组装](../../tutorial/phase-01-llm-chat/1.5-context-assembly.md) |  |
| [llm-1.7 提示缓存与用量统计](../quest-llm-01-llm-app-engineering/llm-1.7-prompt-cache-usage.md) | LLM 应用工程 | [1.5 上下文规划与组装](../../tutorial/phase-01-llm-chat/1.5-context-assembly.md) | 用量存进表需要主线 1.8（也可先放内存） |
| [llm-1.8 重试、限流与降级](../quest-llm-01-llm-app-engineering/llm-1.8-retry-fallback.md) | LLM 应用工程 | [1.2 LLM 双协议与 Provider 抽象](../../tutorial/phase-01-llm-chat/1.2-llm-providers.md) |  |
| [llm-1.9 提示注入与工具权限](../quest-llm-01-llm-app-engineering/llm-1.9-prompt-injection.md) | LLM 应用工程 | [1.7 LLM 视觉输入](../../tutorial/phase-01-llm-chat/1.7-vision-input.md) |  |
| [llm-1.10 录制与回放测试](../quest-llm-01-llm-app-engineering/llm-1.10-record-replay.md) | LLM 应用工程 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) |  |
| [llm-1.11 聊天历史进阶：版本迁移、事务与脱敏](../quest-llm-01-llm-app-engineering/llm-1.11-chat-history-sqlite.md) | LLM 应用工程 | [1.8 聊天历史持久化（SQLite）](../../tutorial/phase-01-llm-chat/1.8-chat-history.md) | 封装方式到时选定（C API、SQLiteCpp、sqlite-orm）；消息格式见 1.2；事务对应 1.3 的回滚设计，取最近 N 条与 llm-1.6（上下文压缩）配合 |
| [llm-2.1 并行工具调用](../quest-llm-02-agent-advanced/llm-2.1-parallel-tool-calls.md) | Agent 进阶 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) |  |
| [llm-2.2 嵌入与向量检索](../quest-llm-02-agent-advanced/llm-2.2-embeddings-search.md) | Agent 进阶 | [1.2 LLM 双协议与 Provider 抽象](../../tutorial/phase-01-llm-chat/1.2-llm-providers.md) |  |
| [llm-2.3 长期记忆：用户偏好](../quest-llm-02-agent-advanced/llm-2.3-long-term-memory.md) | Agent 进阶 | [1.6 Discord 机器人](../../tutorial/phase-01-llm-chat/1.6-discord-bot.md) |  |
| [llm-2.4 按语义检索历史作品](../quest-llm-02-agent-advanced/llm-2.4-semantic-gallery.md) | Agent 进阶 | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-2.2 |
| [llm-2.5 MCP 客户端](../quest-llm-02-agent-advanced/llm-2.5-mcp-client.md) | Agent 进阶 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) |  |
| [llm-2.6 MCP 服务端](../quest-llm-02-agent-advanced/llm-2.6-mcp-server.md) | Agent 进阶 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-2.5 |
| [llm-2.7 节点图校验与自我纠错](../quest-llm-02-agent-advanced/llm-2.7-graph-self-repair.md) | Agent 进阶 | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) |  |
| [llm-2.8 规划与子任务](../quest-llm-02-agent-advanced/llm-2.8-planner-worker.md) | Agent 进阶 | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) |  |
| [llm-2.9 搜索聊天历史（全文检索）](../quest-llm-02-agent-advanced/llm-2.9-history-search.md) | Agent 进阶 | [1.8 聊天历史持久化（SQLite）](../../tutorial/phase-01-llm-chat/1.8-chat-history.md) | 与 llm-2.2 对比 |
| [llm-2.10 让 LLM 查自己的历史](../quest-llm-02-agent-advanced/llm-2.10-history-tool.md) | Agent 进阶 | [1.8 聊天历史持久化（SQLite）](../../tutorial/phase-01-llm-chat/1.8-chat-history.md) | 依赖 llm-2.9；与 llm-1.9（提示注入）配合 |
| [llm-3.1 提示词扩写与风格库](../quest-llm-03-llm-image-craft/llm-3.1-prompt-expansion.md) | 用 LLM 改进出图 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [llm-3.2 反推提示词（图转文）](../quest-llm-03-llm-image-craft/llm-3.2-reverse-prompt.md) | 用 LLM 改进出图 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [llm-3.3 视觉评审回路](../quest-llm-03-llm-image-craft/llm-3.3-critic-loop.md) | 用 LLM 改进出图 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) | 在 2.7 的视觉反馈闭环之上加评分清单与停止条件 |
| [llm-3.4 LLM 当裁判：成对比较](../quest-llm-03-llm-image-craft/llm-3.4-llm-as-judge.md) | 用 LLM 改进出图 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [llm-3.5 批量出图与自动挑图](../quest-llm-03-llm-image-craft/llm-3.5-batch-pick.md) | 用 LLM 改进出图 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) | 依赖 llm-3.3、llm-3.4 |
| [llm-3.6 多轮改图对话](../quest-llm-03-llm-image-craft/llm-3.6-multi-turn-edit.md) | 用 LLM 改进出图 | [10.6 img2img（SDEdit）](../../tutorial/phase-10-txt2img/10.6-img2img.md) |  |
| [llm-3.7 image-craft Skill](../quest-llm-03-llm-image-craft/llm-3.7-image-craft-skill.md) | 用 LLM 改进出图 | [10.4 Skills 与 Discord 演示](../../tutorial/phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [llm-4.1 Skill 文件格式与元数据](../quest-llm-04-skills-advanced/llm-4.1-skill-format.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) | 对照的约定以到时的官方文档为准 |
| [llm-4.2 渐进式披露](../quest-llm-04-skills-advanced/llm-4.2-progressive-disclosure.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) |  |
| [llm-4.3 说明怎么写：让模型选对 Skill](../quest-llm-04-skills-advanced/llm-4.3-skill-descriptions.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) |  |
| [llm-4.4 Skill 自带的资源与脚本](../quest-llm-04-skills-advanced/llm-4.4-skill-resources.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) | 依赖 llm-5.2、llm-5.3（脚本放进受限环境，沙箱见 llm-05） |
| [llm-4.5 权限与沙箱](../quest-llm-04-skills-advanced/llm-4.5-skill-permissions.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) | 与 llm-1.9（提示注入与工具权限）配合 |
| [llm-4.6 热重载与版本](../quest-llm-04-skills-advanced/llm-4.6-skill-hot-reload.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) |  |
| [llm-4.7 从录制生成 Skill](../quest-llm-04-skills-advanced/llm-4.7-skill-from-recording.md) | Skills 进阶 | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) |  |
| [llm-4.8 Skill 评测与回归](../quest-llm-04-skills-advanced/llm-4.8-skill-evals.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) | 依赖 llm-4.3；用 llm-1.10 的录制回放让评测可重复 |
| [llm-4.9 打包与分发](../quest-llm-04-skills-advanced/llm-4.9-skill-packaging.md) | Skills 进阶 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) |  |
| [llm-5.1 沙箱的威胁模型](../quest-llm-05-sandbox/llm-5.1-threat-model.md) | 沙箱与脚本编排 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 与 llm-1.9（提示注入与工具权限）配合 |
| [llm-5.2 嵌入脚本语言与白名单函数](../quest-llm-05-sandbox/llm-5.2-embedded-script.md) | 沙箱与脚本编排 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) |  |
| [llm-5.3 资源限制：时间、内存与指令数](../quest-llm-05-sandbox/llm-5.3-resource-limits.md) | 沙箱与脚本编排 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-5.2 |
| [llm-5.4 子进程沙箱（Windows）](../quest-llm-05-sandbox/llm-5.4-process-sandbox.md) | 沙箱与脚本编排 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 与 llm-5.2 对照 |
| [llm-5.5 用脚本编排工具调用](../quest-llm-05-sandbox/llm-5.5-script-orchestration.md) | 沙箱与脚本编排 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-5.2、llm-5.3；与 llm-4.5（Skill 权限）配合 |
| [llm-5.6 调用预算与失败回报](../quest-llm-05-sandbox/llm-5.6-call-budget.md) | 沙箱与脚本编排 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-5.5 |
| [llm-5.7 沙箱逃逸测试](../quest-llm-05-sandbox/llm-5.7-escape-tests.md) | 沙箱与脚本编排 | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-5.2 至 llm-5.5；用 llm-1.10 的录制回放 |
| [llm-5.8 sandbox Skill](../quest-llm-05-sandbox/llm-5.8-sandbox-skill.md) | 沙箱与脚本编排 | [1.4 Skill 机制：按需加载的能力](../../tutorial/phase-01-llm-chat/1.4-skills.md) | 依赖 llm-5.5、llm-4.1（Skill 文件格式）、llm-4.5（权限声明） |
| [platform-1.1 Linux 工具链与环境检查](../quest-platform-01-linux-foundation/platform-1.1-toolchain-check.md) | Linux 阶段 0 | [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md) |  |
| [platform-1.2 Linux 的 CMake 平台文件](../quest-platform-01-linux-foundation/platform-1.2-cmake-linux.md) | Linux 阶段 0 | [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md) | 在主线已拆分的平台文件的基础上加 linux.cmake（见 0.1） |
| [platform-1.3 测试与 Sanitizer](../quest-platform-01-linux-foundation/platform-1.3-tests-sanitizers.md) | Linux 阶段 0 | [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md) |  |
| [platform-1.4 用 vcpkg 管理依赖](../quest-platform-01-linux-foundation/platform-1.4-vcpkg-deps.md) | Linux 阶段 0 | [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md) |  |
| [platform-1.5 命令行与配置（Linux 分支）](../quest-platform-01-linux-foundation/platform-1.5-cli-config-linux.md) | Linux 阶段 0 | [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md) |  |
| [platform-1.6 GPU 与 CUDA 环境](../quest-platform-01-linux-foundation/platform-1.6-cuda-env.md) | Linux 阶段 0 | [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md) |  |
| [platform-1.7 CI 与本地验证](../quest-platform-01-linux-foundation/platform-1.7-ci-verify.md) | Linux 阶段 0 | [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md) |  |
| [platform-2.1 预编译包选择](../quest-platform-02-libtorch-linux/platform-2.1-libtorch-package.md) | libtorch（Linux） | [3.1 集成 libtorch](../../tutorial/phase-03-libtorch-sampling/3.1-libtorch-setup.md) | 依赖上一条功能支线的末端 |
| [platform-2.2 链接与运行库路径](../quest-platform-02-libtorch-linux/platform-2.2-link-runtime-path.md) | libtorch（Linux） | [3.1 集成 libtorch](../../tutorial/phase-03-libtorch-sampling/3.1-libtorch-setup.md) | 依赖上一条功能支线的末端 |
| [platform-3.1 内存映射的跨平台封装](../quest-platform-03-mmap-linux/platform-3.1-mmap-wrapper.md) | 内存映射（Linux） | [4.1 safetensors 解析](../../tutorial/phase-04-weights-vae/4.1-safetensors-parser.md) | 依赖上一条功能支线的末端 |
| [platform-4.1 编译 ggml 的 CUDA 后端](../quest-platform-04-ggml-linux/platform-4.1-ggml-cuda-build.md) | ggml（Linux） | [8.1 编译 ggml（CUDA 后端）](../../tutorial/phase-08-ggml-blocks/8.1-ggml-build-cuda.md) | 依赖上一条功能支线的末端 |
| [platform-4.2 与 libtorch 共存](../quest-platform-04-ggml-linux/platform-4.2-shared-lib-coexist.md) | ggml（Linux） | [8.1 编译 ggml（CUDA 后端）](../../tutorial/phase-08-ggml-blocks/8.1-ggml-build-cuda.md) | 依赖上一条功能支线的末端 |
| [platform-5.1 对比方法与环境记录](../quest-platform-05-perf-unet/platform-5.1-method-env.md) | 性能对比·UNet（Linux 与 Windows） | [7.4 性能基线](../../tutorial/phase-07-unet-assembly/7.4-unet-baseline.md) | 分析支线（叶子） |
| [platform-5.2 UNet 单步前向的耗时与显存](../quest-platform-05-perf-unet/platform-5.2-unet-step-perf.md) | 性能对比·UNet（Linux 与 Windows） | [7.4 性能基线](../../tutorial/phase-07-unet-assembly/7.4-unet-baseline.md) | 分析支线（叶子） |
| [platform-5.3 ggml 与 libtorch 的差异分析](../quest-platform-05-perf-unet/platform-5.3-ggml-vs-libtorch.md) | 性能对比·UNet（Linux 与 Windows） | [9.4 耗时与显存对比](../../tutorial/phase-09-ggml-unet/9.4-ggml-vs-libtorch-perf.md) | 分析支线（叶子） |
| [platform-5.4 推理工程选项与量化](../quest-platform-05-perf-unet/platform-5.4-engineering-quant.md) | 性能对比·UNet（Linux 与 Windows） | [9.4 耗时与显存对比](../../tutorial/phase-09-ggml-unet/9.4-ggml-vs-libtorch-perf.md) | 分析支线（叶子）；5.4 需合入 diffusion-13、14 |
| [platform-6.1 端到端耗时拆分](../quest-platform-06-perf-sd15/platform-6.1-e2e-breakdown.md) | 性能对比·SD 1.5 出图 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 分析支线（叶子） |
| [platform-6.2 模型加载耗时](../quest-platform-06-perf-sd15/platform-6.2-model-load.md) | 性能对比·SD 1.5 出图 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 分析支线（叶子） |
| [platform-6.3 出图结果的一致性](../quest-platform-06-perf-sd15/platform-6.3-consistency.md) | 性能对比·SD 1.5 出图 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 分析支线（叶子） |
| [platform-6.4 UNet 加速的对比](../quest-platform-06-perf-sd15/platform-6.4-unet-accel.md) | 性能对比·SD 1.5 出图 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 分析支线（叶子）；6.4 需合入 diffusion-6 |
| [platform-7.1 与 libtorch 共用 CUDA 运行库](../quest-platform-07-onnxruntime-linux/platform-7.1-cuda-runtime-share.md) | ONNX Runtime（Linux） | [11.1 集成 ORT](../../tutorial/phase-11-onnxruntime/11.1-ort-setup.md) | 依赖上一条功能支线的末端 |
| [platform-8.1 量化版 FLUX 出图的耗时与显存](../quest-platform-08-perf-flux/platform-8.1-flux-perf.md) | 性能对比·FLUX | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) | 分析支线（叶子） |
| [platform-8.2 显存紧张时的行为](../quest-platform-08-perf-flux/platform-8.2-vram-pressure.md) | 性能对比·FLUX | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) | 分析支线（叶子） |
| [platform-8.3 DiT 加速的对比](../quest-platform-08-perf-flux/platform-8.3-dit-accel.md) | 性能对比·FLUX | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) | 分析支线（叶子）；8.3 需合入 diffusion-12 |
| [platform-9.1 分段加载与逐块卸载的耗时与显存](../quest-platform-09-perf-qwen-image/platform-9.1-staged-offload-perf.md) | 性能对比·Qwen-Image | [16.6 Qwen-Image 文生图](../../tutorial/phase-16-qwen-image/16.6-qwen-txt2img.md) | 分析支线（叶子） |
| [platform-9.2 卸载时的传输开销](../quest-platform-09-perf-qwen-image/platform-9.2-transfer-overhead.md) | 性能对比·Qwen-Image | [16.6 Qwen-Image 文生图](../../tutorial/phase-16-qwen-image/16.6-qwen-txt2img.md) | 分析支线（叶子） |
| [platform-10.1 对比方法与功能清单](../quest-platform-10-source-build/platform-10.1-method-features.md) | 源码构建与官方版本对比（Linux） | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-10.2 libtorch](../quest-platform-10-source-build/platform-10.2-libtorch.md) | 源码构建与官方版本对比（Linux） | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-10.3 ggml 与 stable-diffusion.cpp](../quest-platform-10-source-build/platform-10.3-ggml-sdcpp.md) | 源码构建与官方版本对比（Linux） | [13.2 后端接口](../../tutorial/phase-13-sdcpp-backend/13.2-backend-interface.md) | 分析支线（叶子） |
| [platform-10.4 ONNX Runtime](../quest-platform-10-source-build/platform-10.4-onnxruntime.md) | 源码构建与官方版本对比（Linux） | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-10.5 汇总报告](../quest-platform-10-source-build/platform-10.5-report.md) | 源码构建与官方版本对比（Linux） | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-11.1 对比方法与约束](../quest-platform-11-gcc-clang/platform-11.1-method.md) | GCC 与 Clang 的编译器性能优化 | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-11.2 优化选项矩阵](../quest-platform-11-gcc-clang/platform-11.2-option-matrix.md) | GCC 与 Clang 的编译器性能优化 | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-11.3 CPU 侧热点](../quest-platform-11-gcc-clang/platform-11.3-cpu-hotspots.md) | GCC 与 Clang 的编译器性能优化 | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-11.4 nvcc 的宿主编译器](../quest-platform-11-gcc-clang/platform-11.4-nvcc-host.md) | GCC 与 Clang 的编译器性能优化 | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-11.5 汇总](../quest-platform-11-gcc-clang/platform-11.5-summary.md) | GCC 与 Clang 的编译器性能优化 | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 分析支线（叶子） |
| [platform-12.1 并行工具调用](../quest-platform-12-agent-advanced-linux/platform-12.1-parallel-tool-calls.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.2 嵌入与向量检索](../quest-platform-12-agent-advanced-linux/platform-12.2-embeddings-search.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.3 长期记忆：用户偏好](../quest-platform-12-agent-advanced-linux/platform-12.3-long-term-memory.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.4 按语义检索历史作品](../quest-platform-12-agent-advanced-linux/platform-12.4-semantic-gallery.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.5 MCP 客户端](../quest-platform-12-agent-advanced-linux/platform-12.5-mcp-client.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.6 MCP 服务端](../quest-platform-12-agent-advanced-linux/platform-12.6-mcp-server.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.7 节点图校验与自我纠错](../quest-platform-12-agent-advanced-linux/platform-12.7-graph-self-repair.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.8 规划与子任务](../quest-platform-12-agent-advanced-linux/platform-12.8-planner-worker.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.9 搜索聊天历史（全文检索）](../quest-platform-12-agent-advanced-linux/platform-12.9-history-search.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-12.10 让 LLM 查自己的历史](../quest-platform-12-agent-advanced-linux/platform-12.10-history-tool.md) | Agent 进阶（Linux 完整版） | [2.7 视觉反馈闭环](../../tutorial/phase-02-opencv-handles/2.7-vision-feedback-loop.md) | 依赖 llm-02 与 platform-01 |
| [platform-13.1 沙箱的威胁模型](../quest-platform-13-sandbox-linux/platform-13.1-threat-model.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-13.2 嵌入脚本语言与白名单函数](../quest-platform-13-sandbox-linux/platform-13.2-embedded-script.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-13.3 资源限制：时间、内存与指令数](../quest-platform-13-sandbox-linux/platform-13.3-resource-limits.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-13.4 子进程沙箱（Windows）](../quest-platform-13-sandbox-linux/platform-13.4-process-sandbox.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-13.5 用脚本编排工具调用](../quest-platform-13-sandbox-linux/platform-13.5-script-orchestration.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-13.6 调用预算与失败回报](../quest-platform-13-sandbox-linux/platform-13.6-call-budget.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-13.7 沙箱逃逸测试](../quest-platform-13-sandbox-linux/platform-13.7-escape-tests.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-13.8 sandbox Skill](../quest-platform-13-sandbox-linux/platform-13.8-sandbox-skill.md) | 沙箱与脚本编排（Linux 完整版） | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 依赖 llm-05 与 platform-01 |
| [platform-14.1 Skill 文件格式与元数据](../quest-platform-14-skills-advanced-linux/platform-14.1-skill-format.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01 |
| [platform-14.2 渐进式披露](../quest-platform-14-skills-advanced-linux/platform-14.2-progressive-disclosure.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01 |
| [platform-14.3 说明怎么写：让模型选对 Skill](../quest-platform-14-skills-advanced-linux/platform-14.3-skill-descriptions.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01 |
| [platform-14.4 Skill 自带的资源与脚本](../quest-platform-14-skills-advanced-linux/platform-14.4-skill-resources.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01；4.4、4.5 依赖 platform-13 |
| [platform-14.5 权限与沙箱](../quest-platform-14-skills-advanced-linux/platform-14.5-skill-permissions.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01；4.4、4.5 依赖 platform-13 |
| [platform-14.6 热重载与版本](../quest-platform-14-skills-advanced-linux/platform-14.6-skill-hot-reload.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01 |
| [platform-14.7 从录制生成 Skill](../quest-platform-14-skills-advanced-linux/platform-14.7-skill-from-recording.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01 |
| [platform-14.8 Skill 评测与回归](../quest-platform-14-skills-advanced-linux/platform-14.8-skill-evals.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01 |
| [platform-14.9 打包与分发](../quest-platform-14-skills-advanced-linux/platform-14.9-skill-packaging.md) | Skills 进阶（Linux 完整版） | [2.6 调用记录与工作流导出](../../tutorial/phase-02-opencv-handles/2.6-workflow-recording.md) | 依赖 llm-04 与 platform-01 |
| [paper-1.1 ReAct：推理与行动交替](../quest-paper-01-llm-agent/paper-1.1-react.md) | LLM 与 Agent | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 对照 1.3 的实现 |
| [paper-1.2 Toolformer：模型自己学会用工具](../quest-paper-01-llm-agent/paper-1.2-toolformer.md) | LLM 与 Agent | [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md) | 对照 1.3 的实现 |
| [paper-2.1 非平衡热力学与扩散模型的起点](../quest-paper-02-diffusion-basics/paper-2.1-nonequilibrium-thermodynamics.md) | 扩散模型基础 | [3.4 扩散的直观：得分与朗之万](../../tutorial/phase-03-libtorch-sampling/3.4-score-langevin-intuition.md) | 对照 3.4 的实现 |
| [paper-2.2 得分匹配与朗之万采样](../quest-paper-02-diffusion-basics/paper-2.2-score-matching-langevin.md) | 扩散模型基础 | [3.4 扩散的直观：得分与朗之万](../../tutorial/phase-03-libtorch-sampling/3.4-score-langevin-intuition.md) | 对照 3.4 的实现 |
| [paper-2.3 DDPM：去噪扩散概率模型](../quest-paper-02-diffusion-basics/paper-2.3-ddpm.md) | 扩散模型基础 | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) | 对照 3.5、3.6 的实现 |
| [paper-2.4 DDIM：确定性采样与跳步](../quest-paper-02-diffusion-basics/paper-2.4-ddim.md) | 扩散模型基础 | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) | 对照 3.6 的实现 |
| [paper-2.5 用随机微分方程统一扩散模型](../quest-paper-02-diffusion-basics/paper-2.5-score-sde.md) | 扩散模型基础 | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) | 对照 3.5、3.6 的实现 |
| [paper-2.6 EDM：把设计选择拆开讨论](../quest-paper-02-diffusion-basics/paper-2.6-edm.md) | 扩散模型基础 | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) | 对照 3.5、3.6 的实现 |
| [paper-3.1 VAE：变分自编码器](../quest-paper-03-latent-vae/paper-3.1-vae.md) | 潜空间与 VAE | [4.6 VAE 编码器](../../tutorial/phase-04-weights-vae/4.6-vae-encoder.md) | 对照 4.5、4.6 的实现 |
| [paper-3.2 潜空间扩散模型（Stable Diffusion 的基础）](../quest-paper-03-latent-vae/paper-3.2-ldm.md) | 潜空间与 VAE | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 对照 4.7、10.3 的实现 |
| [paper-4.1 BPE 分词](../quest-paper-04-transformer-text/paper-4.1-bpe.md) | Transformer 与文本编码 | [5.1 BPE 分词器](../../tutorial/phase-05-clip/5.1-bpe-tokenizer.md) | 对照 5.1 的实现 |
| [paper-4.2 Transformer 与自注意力](../quest-paper-04-transformer-text/paper-4.2-attention-is-all-you-need.md) | Transformer 与文本编码 | [5.3 Transformer 编码器](../../tutorial/phase-05-clip/5.3-clip-transformer.md) | 对照 5.2、5.3 的实现 |
| [paper-4.3 CLIP：图文对比学习](../quest-paper-04-transformer-text/paper-4.3-clip.md) | Transformer 与文本编码 | [5.4 对照验证与 CLIPTextEncode 节点](../../tutorial/phase-05-clip/5.4-clip-verify-node.md) | 对照 5.3、5.4 的实现 |
| [paper-5.1 残差网络](../quest-paper-05-unet-attention/paper-5.1-resnet.md) | UNet 与注意力 | [6.2 ResBlock](../../tutorial/phase-06-unet-blocks/6.2-resblock.md) | 对照 6.2 的实现 |
| [paper-5.2 分组归一化](../quest-paper-05-unet-attention/paper-5.2-group-normalization.md) | UNet 与注意力 | [6.2 ResBlock](../../tutorial/phase-06-unet-blocks/6.2-resblock.md) | 对照 4.4、6.2 的实现 |
| [paper-5.3 U-Net](../quest-paper-05-unet-attention/paper-5.3-unet.md) | UNet 与注意力 | [7.1 UNet 结构组装](../../tutorial/phase-07-unet-assembly/7.1-unet-structure.md) | 对照 7.1 的实现 |
| [paper-5.4 FlashAttention](../quest-paper-05-unet-attention/paper-5.4-flashattention.md) | UNet 与注意力 | [7.5 注意力优化](../../tutorial/phase-07-unet-assembly/7.5-attention-optimization.md) | 对照 7.5 的实现 |
| [paper-6.1 无分类器引导（CFG）](../quest-paper-06-guidance-editing/paper-6.1-classifier-free-guidance.md) | 引导、适配与编辑 | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) | 对照 10.2 的实现 |
| [paper-6.2 LoRA：低秩适配](../quest-paper-06-guidance-editing/paper-6.2-lora.md) | 引导、适配与编辑 | [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md) | 对照 10.5 的实现 |
| [paper-6.3 SDEdit：加噪再去噪的编辑](../quest-paper-06-guidance-editing/paper-6.3-sdedit.md) | 引导、适配与编辑 | [10.6 img2img（SDEdit）](../../tutorial/phase-10-txt2img/10.6-img2img.md) | 对照 10.6 的实现 |
| [paper-6.4 RePaint：用扩散模型补全图像](../quest-paper-06-guidance-editing/paper-6.4-repaint.md) | 引导、适配与编辑 | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 对照 10.7 的实现 |
| [paper-7.1 Real-ESRGAN：真实场景超分](../quest-paper-07-super-resolution/paper-7.1-real-esrgan.md) | 超分 | [11.2 超分节点](../../tutorial/phase-11-onnxruntime/11.2-upscale-node.md) | 对照 11.2 的实现 |
| [paper-8.1 DiT：用 Transformer 做扩散](../quest-paper-08-dit-flow/paper-8.1-dit.md) | DiT 与流匹配 | [14.4 AdaLN 调制](../../tutorial/phase-14-dit-basics/14.4-adaln-modulation.md) | 对照 14.1、14.4 的实现 |
| [paper-8.2 RoFormer：旋转位置编码](../quest-paper-08-dit-flow/paper-8.2-roformer-rope.md) | DiT 与流匹配 | [14.3 2D RoPE 与分辨率外推](../../tutorial/phase-14-dit-basics/14.3-rope-2d-extrapolation.md) | 对照 14.2、14.3 的实现 |
| [paper-8.3 FiLM：按条件缩放和平移](../quest-paper-08-dit-flow/paper-8.3-film.md) | DiT 与流匹配 | [14.4 AdaLN 调制](../../tutorial/phase-14-dit-basics/14.4-adaln-modulation.md) | 对照 14.4 的实现 |
| [paper-8.4 流匹配](../quest-paper-08-dit-flow/paper-8.4-flow-matching.md) | DiT 与流匹配 | [14.5 流匹配与采样](../../tutorial/phase-14-dit-basics/14.5-flow-matching.md) | 对照 14.5 的实现 |
| [paper-8.5 整流流](../quest-paper-08-dit-flow/paper-8.5-rectified-flow.md) | DiT 与流匹配 | [14.5 流匹配与采样](../../tutorial/phase-14-dit-basics/14.5-flow-matching.md) | 对照 14.5 的实现 |
| [paper-8.6 SD3：MM-DiT 与大规模整流流](../quest-paper-08-dit-flow/paper-8.6-sd3-mmdit.md) | DiT 与流匹配 | [14.7 MM-DiT 原理](../../tutorial/phase-14-dit-basics/14.7-mmdit-principle.md) | 对照 14.6、14.7 的实现 |
| [paper-8.7 FP8 数值格式](../quest-paper-08-dit-flow/paper-8.7-fp8-formats.md) | DiT 与流匹配 | [14.10 fp8 权重加载](../../tutorial/phase-14-dit-basics/14.10-fp8-loading.md) | 对照 14.10 的实现 |
| [paper-8.8 REPA：表示对齐](../quest-paper-08-dit-flow/paper-8.8-repa.md) | DiT 与流匹配 | [14.12 表示对齐 REPA（原理阅读）](../../tutorial/phase-14-dit-basics/14.12-repa-reading.md) | 对照 14.12 的实现 |
| [paper-9.1 T5：文本到文本的 Transformer](../quest-paper-09-flux-distillation/paper-9.1-t5.md) | FLUX 与蒸馏 | [15.2 T5 与 CLIP 双编码器](../../tutorial/phase-15-flux/15.2-t5-clip-encoders.md) | 对照 15.2 的实现 |
| [paper-9.2 引导蒸馏](../quest-paper-09-flux-distillation/paper-9.2-guided-distillation.md) | FLUX 与蒸馏 | [15.6 CFG 与引导蒸馏](../../tutorial/phase-15-flux/15.6-guidance-distillation.md) | 对照 15.6 的实现 |
| [paper-9.3 潜空间对抗蒸馏](../quest-paper-09-flux-distillation/paper-9.3-ladd.md) | FLUX 与蒸馏 | [15.6 CFG 与引导蒸馏](../../tutorial/phase-15-flux/15.6-guidance-distillation.md) | 对照 15.6 的实现 |
| [paper-10.1 Qwen-Image 技术报告](../quest-paper-10-qwen-llm/paper-10.1-qwen-image-report.md) | Qwen-Image 与大语言模型 | [16.1 20B MMDiT 架构](../../tutorial/phase-16-qwen-image/16.1-qwen-architecture.md) | 对照 16.1 的实现 |
| [paper-10.2 Qwen2.5-VL 技术报告](../quest-paper-10-qwen-llm/paper-10.2-qwen25vl-report.md) | Qwen-Image 与大语言模型 | [16.3 Qwen2.5-VL 文本编码器](../../tutorial/phase-16-qwen-image/16.3-qwen25vl-encoder.md) | 对照 16.3 的实现 |
| [paper-10.3 分组查询注意力](../quest-paper-10-qwen-llm/paper-10.3-gqa.md) | Qwen-Image 与大语言模型 | [16.3 Qwen2.5-VL 文本编码器](../../tutorial/phase-16-qwen-image/16.3-qwen25vl-encoder.md) | 对照 16.3 的实现 |
| [paper-10.4 SwiGLU](../quest-paper-10-qwen-llm/paper-10.4-swiglu.md) | Qwen-Image 与大语言模型 | [16.3 Qwen2.5-VL 文本编码器](../../tutorial/phase-16-qwen-image/16.3-qwen25vl-encoder.md) | 对照 16.3 的实现 |
| [paper-10.5 RMSNorm](../quest-paper-10-qwen-llm/paper-10.5-rmsnorm.md) | Qwen-Image 与大语言模型 | [16.3 Qwen2.5-VL 文本编码器](../../tutorial/phase-16-qwen-image/16.3-qwen25vl-encoder.md) | 对照 16.3 的实现 |
| [paper-10.6 字节级 BPE](../quest-paper-10-qwen-llm/paper-10.6-byte-level-bpe.md) | Qwen-Image 与大语言模型 | [16.3 Qwen2.5-VL 文本编码器](../../tutorial/phase-16-qwen-image/16.3-qwen25vl-encoder.md) | 对照 16.3 的实现 |
| [paper-11.1 渐进式蒸馏与 v 预测](../quest-paper-11-sampling-guidance/paper-11.1-progressive-distillation.md) | 采样与引导进阶（扩散支线） | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) | 需先完成支线 diffusion-5.1（对照它的实现） |
| [paper-11.2 零终端 SNR](../quest-paper-11-sampling-guidance/paper-11.2-zero-terminal-snr.md) | 采样与引导进阶（扩散支线） | [3.5 sigma 调度器](../../tutorial/phase-03-libtorch-sampling/3.5-sigma-schedulers.md) | 需先完成支线 diffusion-5.2（对照它的实现） |
| [paper-11.3 DPM-Solver](../quest-paper-11-sampling-guidance/paper-11.3-dpm-solver.md) | 采样与引导进阶（扩散支线） | [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md) | 需先完成支线 diffusion-5.3（对照它的实现） |
| [paper-11.4 分类器引导](../quest-paper-11-sampling-guidance/paper-11.4-classifier-guidance.md) | 采样与引导进阶（扩散支线） | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成支线 diffusion-3.6（对照它的实现） |
| [paper-11.5 自注意力引导（SAG）](../quest-paper-11-sampling-guidance/paper-11.5-sag.md) | 采样与引导进阶（扩散支线） | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成支线 diffusion-5.5（对照它的实现） |
| [paper-11.6 扰动注意力引导（PAG）](../quest-paper-11-sampling-guidance/paper-11.6-pag.md) | 采样与引导进阶（扩散支线） | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成支线 diffusion-5.5（对照它的实现） |
| [paper-11.7 FreeU](../quest-paper-11-sampling-guidance/paper-11.7-freeu.md) | 采样与引导进阶（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-2.5（对照它的实现） |
| [paper-11.8 MultiDiffusion](../quest-paper-11-sampling-guidance/paper-11.8-multidiffusion.md) | 采样与引导进阶（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-3.3（对照它的实现） |
| [paper-12.1 ControlNet](../quest-paper-12-control-editing/paper-12.1-controlnet.md) | 条件控制与编辑（扩散支线） | [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md) | 需先完成支线 diffusion-8.2（对照它的实现） |
| [paper-12.2 IP-Adapter](../quest-paper-12-control-editing/paper-12.2-ip-adapter.md) | 条件控制与编辑（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-8.4（对照它的实现） |
| [paper-12.3 Prompt-to-Prompt](../quest-paper-12-control-editing/paper-12.3-prompt-to-prompt.md) | 条件控制与编辑（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-8.5（对照它的实现） |
| [paper-12.4 Textual Inversion](../quest-paper-12-control-editing/paper-12.4-textual-inversion.md) | 条件控制与编辑（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-10.3（对照它的实现） |
| [paper-12.5 Null-text 反演](../quest-paper-12-control-editing/paper-12.5-null-text-inversion.md) | 条件控制与编辑（扩散支线） | [10.2 KSampler 与 CFG](../../tutorial/phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成支线 diffusion-11.4（对照它的实现） |
| [paper-13.1 DeepCache](../quest-paper-13-speedup-quant/paper-13.1-deepcache.md) | 加速与量化（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-6.1（对照它的实现） |
| [paper-13.2 ToMe for SD](../quest-paper-13-speedup-quant/paper-13.2-tome-sd.md) | 加速与量化（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-6.2（对照它的实现） |
| [paper-13.3 潜空间一致性模型（LCM）](../quest-paper-13-speedup-quant/paper-13.3-lcm.md) | 加速与量化（扩散支线） | [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md) | 需先完成支线 diffusion-6.3（对照它的实现） |
| [paper-13.4 SVDQuant](../quest-paper-13-speedup-quant/paper-13.4-svdquant.md) | 加速与量化（扩散支线） | [15.7 Flux 文生图](../../tutorial/phase-15-flux/15.7-flux-txt2img.md) | 需先完成支线 diffusion-12.3（对照它的实现） |
| [paper-14.1 LPIPS](../quest-paper-14-evaluation-sdxl/paper-14.1-lpips.md) | 评估与 SDXL（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-11.2（对照它的实现） |
| [paper-14.2 CLIPScore](../quest-paper-14-evaluation-sdxl/paper-14.2-clipscore.md) | 评估与 SDXL（扩散支线） | [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成支线 diffusion-11.2（对照它的实现） |
| [paper-14.3 SDXL](../quest-paper-14-evaluation-sdxl/paper-14.3-sdxl.md) | 评估与 SDXL（扩散支线） | [10.7 Inpaint 与 Outpaint](../../tutorial/phase-10-txt2img/10.7-inpaint-outpaint.md) | 需先完成支线 diffusion-16.1（对照它的实现） |

## 支线之间的依赖

下面列出所有跨支线的依赖。约定：**起点包含**表示按“支线分支”表里的起点切出分支后，被依赖的内容已经在分支历史里；**起点不包含**表示要自己合入对应分支或按顺序交错推进。主线不依赖任何支线，所以没有从支线指向主线的依赖。

| 依赖方 | 被依赖方 | 性质 | 具体章节与说明 | 起点 |
|---|---|---|---|---|
| diffusion-03 | diffusion-02 | 硬依赖 | 3.5 区域提示依赖 2.1 钩子 | 包含（起点是 diffusion-02 的末端） |
| diffusion-05 | diffusion-02 | 硬依赖 | 5.5 PAG / SAG 依赖 2.1 | 包含 |
| diffusion-08 | diffusion-02 | 硬依赖 | 8.5 Prompt-to-Prompt 依赖 2.3 | 包含 |
| diffusion-11 | diffusion-08 | 硬依赖 | 11.5 依赖 8.5 与 11.3 | 包含 |
| llm-04 | llm-01 | 硬依赖 | 4.8 用 llm-1.10 的录制回放让评测可重复 | 包含（llm-01 的末端，再合入主线 2.6） |
| llm-05 | llm-01 | 硬依赖 | 5.7 用 llm-1.10 的录制回放 | 包含（起点是 llm-01 的末端） |
| diffusion-15 | llm-01 | 硬依赖 | 15.7 用 llm-1.10 的录制回放让评测可重复 | 包含（llm-01 的末端，再合入主线 10.7） |
| llm-04 | llm-05 | 硬依赖 | 4.4 的脚本放进 llm-05 的沙箱（llm-5.2、5.3）；4.5 与 llm-5.5 配合 | 不包含，见下面的“交错推进” |
| llm-05 | llm-04 | 硬依赖 | 5.8 用 llm-4.1 的文件格式与 llm-4.5 的权限声明 | 不包含，见下面的“交错推进” |
| diffusion-15 | diffusion-03 | 可选 | 15.5 区域提示见 diffusion-3.5，不做则用整体提示 | 不包含 |
| diffusion-15 | diffusion-01 | 仅对照 | 15.6 与 diffusion-1.6 对照 | 不包含 |
| platform-02 至 04、07 | 上一条功能支线 | 硬依赖 | Linux 功能线 01 → 02 → 03 → 04 → 07 串成一条，每条合入本阶段所需的主线 tag | 包含 |
| platform-05、06 | platform-04 | 硬依赖 | 分析支线，从功能线分叉 | 包含 |
| platform-08 至 11 | platform-07 | 硬依赖 | 分析支线，从功能线分叉；主线 11 之后的阶段会用到 ORT | 包含 |
| platform-05 | diffusion-13、14 | 硬依赖 | 5.4 对比推理工程选项与量化 | 不包含，合入对方分支 |
| platform-06 | diffusion-6 | 硬依赖 | 6.4 对比 UNet 加速 | 不包含，合入对方分支 |
| platform-08 | diffusion-12 | 硬依赖 | 8.3 对比 DiT 加速 | 不包含，合入对方分支 |
| platform-12、13、14 | llm-02、llm-05、llm-04 | 硬依赖 | Linux 完整版，依附对应的 llm 支线 | 包含（起点是被依附支线的末端，再合入 platform-01） |
| platform-14 | platform-13 | 硬依赖 | 4.4、4.5 使用 platform-13 的 Linux 沙箱 | 不包含，合入对方分支 |
| paper-02 至 10 | 主线章节 | 对照实现 | 每篇论文对照主线某几章的实现，读完对应章节再读 | 不涉及分支（文档） |
| paper-11 | diffusion-02、diffusion-03、diffusion-05 | 对照实现 | 每篇论文对应一个支线章节，需先做完那一章再读 | 不涉及分支（文档） |
| paper-12 | diffusion-08、diffusion-10、diffusion-11 | 对照实现 | 每篇论文对应一个支线章节，需先做完那一章再读 | 不涉及分支（文档） |
| paper-13 | diffusion-06、diffusion-12 | 对照实现 | 每篇论文对应一个支线章节，需先做完那一章再读 | 不涉及分支（文档） |
| paper-14 | diffusion-11、diffusion-16 | 对照实现 | 每篇论文对应一个支线章节，需先做完那一章再读 | 不涉及分支（文档） |

**不会出现在表里的依赖**：platform 的分析支线是叶子，不作为任何支线的起点，彼此之间也没有依赖。llm-02 的 2.9、2.10 不再依赖 llm-01：聊天历史的基础表现在在主线 1.8 里。

### llm-04 与 llm-05 的交错推进

两条支线互相依赖，但在章节层面不构成死循环。推荐顺序：

1. 先在 llm-05 做 5.1 至 5.5（沙箱与脚本编排）。
2. 在 llm-04 做 4.1 至 4.3；做 4.4 之前，把 llm-05 做完 5.2、5.3 的提交合入 llm-04 的分支。
3. 在 llm-04 做 4.5 之后，回到 llm-05：把 llm-04 的 4.1 与 4.5 的提交合入 llm-05 的分支，再做 5.8。

如果不想合并分支，也可以让 4.4 先用一个简单的受限运行方式，等 llm-05 做完再换成沙箱。

<!-- 下面这些注释是主页依赖图的数据：起点没写出来的跨支线依赖。格式：<!-- edge: 被依赖方 依赖方 merge|optional --> -->
<!-- edge: llm-05 llm-04 merge -->
<!-- edge: llm-04 llm-05 merge -->
<!-- edge: diffusion-03 diffusion-15 optional -->
<!-- edge: diffusion-01 diffusion-15 optional -->
<!-- edge: diffusion-13 platform-05 merge -->
<!-- edge: diffusion-14 platform-05 merge -->
<!-- edge: diffusion-06 platform-06 merge -->
<!-- edge: diffusion-12 platform-08 merge -->
<!-- edge: platform-13 platform-14 merge -->
<!-- edge: diffusion-02 paper-11 optional -->
<!-- edge: diffusion-03 paper-11 optional -->
<!-- edge: diffusion-05 paper-11 optional -->
<!-- edge: diffusion-08 paper-12 optional -->
<!-- edge: diffusion-10 paper-12 optional -->
<!-- edge: diffusion-11 paper-12 optional -->
<!-- edge: diffusion-06 paper-13 optional -->
<!-- edge: diffusion-12 paper-13 optional -->
<!-- edge: diffusion-11 paper-14 optional -->
<!-- edge: diffusion-16 paper-14 optional -->
