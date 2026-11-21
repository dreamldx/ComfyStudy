# 附录与主线的衔接表

状态：由设计自动整理，随章节调整更新

## 附录分支

每个附录阶段一个分支，不打 tag、不发 Release，也不合并回 main 分支。

| 阶段 | 分支 | 起点 |
|---|---|---|
| 17 | `appendix/17-latent-playground-basic` | 主线 tag `chapter-10.7-inpaint-outpaint` |
| 18 | `appendix/18-unet-lab` | 主线 tag `chapter-10.4-skills-discord-demo` |
| 19 | `appendix/19-latent-playground-adv` | 分支 `appendix/18-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中） |
| 20 | `appendix/20-conditioning-prompts` | 主线 tag `chapter-10.4-skills-discord-demo` |
| 21 | `appendix/21-sampling-advanced` | 分支 `appendix/18-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.3-txt2img-assembly`，已包含在该分支的历史中） |
| 22 | `appendix/22-unet-speedup` | 主线 tag `chapter-10.5-lora` |
| 23 | `appendix/23-dit-lab` | 主线 tag `chapter-12.8-flux-txt2img` |
| 24 | `appendix/24-control-adapters` | 分支 `appendix/18-unet-lab` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中） |
| 25 | `appendix/25-dit-adapters` | 主线 tag `chapter-12.8-flux-txt2img` |
| 26 | `appendix/26-training` | 主线 tag `chapter-12.8-flux-txt2img` |
| 27 | `appendix/27-eval-inversion` | 分支 `appendix/24-control-adapters` 的末端（本阶段所需的最晚主线节点是 `chapter-10.4-skills-discord-demo`，已包含在该分支的历史中） |
| 28 | `appendix/28-dit-speedup` | 主线 tag `chapter-12.8-flux-txt2img` |
| 29 | `appendix/29-inference-engineering` | 主线 tag `chapter-10.2-ksampler-cfg` |
| 30 | `appendix/30-quantization` | 主线 tag `chapter-10.3-txt2img-assembly` |

## 章节衔接

每个附录章节可以在做完哪一个主线章节之后开始。对应主线章节的「练习与下一章预告」里也列出了这些附录入口。

| 附录章节 | 所属阶段 | 可开始于（主线） | 备注 |
|---|---|---|---|
| [17.1 逐步快速预览（Latent2RGB）](../phase-17-latent-playground-basic/17.1-latent-preview.md) | 潜空间玩法·入门（选读） | [10.2 KSampler 与 CFG](../phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [17.2 无缝平铺纹理](../phase-17-latent-playground-basic/17.2-seamless-tiling.md) | 潜空间玩法·入门（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [17.3 噪声插值与潜空间动画](../phase-17-latent-playground-basic/17.3-noise-interpolation.md) | 潜空间玩法·入门（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [17.4 种子变体](../phase-17-latent-playground-basic/17.4-seed-variation.md) | 潜空间玩法·入门（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [17.5 构图锁定](../phase-17-latent-playground-basic/17.5-composition-lock.md) | 潜空间玩法·入门（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [17.6 涂鸦转图](../phase-17-latent-playground-basic/17.6-scribble-to-image.md) | 潜空间玩法·入门（选读） | [10.6 img2img（SDEdit）](../phase-10-txt2img/10.6-img2img.md) |  |
| [17.7 潜空间合成与混合](../phase-17-latent-playground-basic/17.7-latent-compositing.md) | 潜空间玩法·入门（选读） | [10.7 Inpaint 与 Outpaint](../phase-10-txt2img/10.7-inpaint-outpaint.md) |  |
| [17.8 高分辨率修复](../phase-17-latent-playground-basic/17.8-hires-fix.md) | 潜空间玩法·入门（选读） | [10.6 img2img（SDEdit）](../phase-10-txt2img/10.6-img2img.md) |  |
| [17.9 latent-lab Skill](../phase-17-latent-playground-basic/17.9-latent-playground-skill.md) | 潜空间玩法·入门（选读） | [10.7 Inpaint 与 Outpaint](../phase-10-txt2img/10.7-inpaint-outpaint.md) |  |
| [18.1 UNet 钩子机制](../phase-18-unet-lab/18.1-hook-mechanism.md) | UNet 内部观察与实验（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [18.2 特征图可视化](../phase-18-unet-lab/18.2-feature-map-viz.md) | UNet 内部观察与实验（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [18.3 交叉注意力图可视化](../phase-18-unet-lab/18.3-cross-attention-maps.md) | UNet 内部观察与实验（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [18.4 逐层消融实验](../phase-18-unet-lab/18.4-layer-ablation.md) | UNet 内部观察与实验（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [18.5 FreeU](../phase-18-unet-lab/18.5-freeu.md) | UNet 内部观察与实验（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [18.6 unet-lab Skill](../phase-18-unet-lab/18.6-unet-lab-skill.md) | UNet 内部观察与实验（选读） | [10.4 Skills 与 Discord 演示](../phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [19.1 潜空间语义方向](../phase-19-latent-playground-adv/19.1-semantic-directions.md) | 潜空间玩法·进阶（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [19.2 TAESD 快速解码](../phase-19-latent-playground-adv/19.2-taesd.md) | 潜空间玩法·进阶（选读） | [4.7 VAE 节点](../phase-04-weights-vae/4.7-vae-nodes.md) |  |
| [19.3 分块解码与分块扩散](../phase-19-latent-playground-adv/19.3-tiled-vae-diffusion.md) | 潜空间玩法·进阶（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [19.4 全景图](../phase-19-latent-playground-adv/19.4-panorama.md) | 潜空间玩法·进阶（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成附录 19.3 |
| [19.5 区域提示](../phase-19-latent-playground-adv/19.5-regional-prompting.md) | 潜空间玩法·进阶（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成附录 18.1 |
| [19.6 梯度引导](../phase-19-latent-playground-adv/19.6-gradient-guidance.md) | 潜空间玩法·进阶（选读） | [10.2 KSampler 与 CFG](../phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [19.7 latent-lab-adv Skill](../phase-19-latent-playground-adv/19.7-latent-adv-skill.md) | 潜空间玩法·进阶（选读） | [10.4 Skills 与 Discord 演示](../phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [20.1 提示词加权](../phase-20-conditioning-prompts/20.1-prompt-weighting.md) | 条件与提示技术（选读） | [5.4 对照验证与 CLIPTextEncode 节点](../phase-05-clip/5.4-clip-verify-node.md) |  |
| [20.2 CLIP skip](../phase-20-conditioning-prompts/20.2-clip-skip.md) | 条件与提示技术（选读） | [5.4 对照验证与 CLIPTextEncode 节点](../phase-05-clip/5.4-clip-verify-node.md) |  |
| [20.3 条件操作](../phase-20-conditioning-prompts/20.3-conditioning-ops.md) | 条件与提示技术（选读） | [5.4 对照验证与 CLIPTextEncode 节点](../phase-05-clip/5.4-clip-verify-node.md) |  |
| [20.4 conditioning-lab Skill](../phase-20-conditioning-prompts/20.4-conditioning-skill.md) | 条件与提示技术（选读） | [10.4 Skills 与 Discord 演示](../phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [21.1 预测目标：ε、v、x0](../phase-21-sampling-advanced/21.1-prediction-targets.md) | 采样与调度进阶（选读） | [3.6 Euler 采样器与玩具去噪器](../phase-03-libtorch-sampling/3.6-euler-sampler.md) |  |
| [21.2 零终端 SNR 与 trailing 间隔](../phase-21-sampling-advanced/21.2-zero-terminal-snr.md) | 采样与调度进阶（选读） | [3.5 sigma 调度器](../phase-03-libtorch-sampling/3.5-sigma-schedulers.md) |  |
| [21.3 进阶采样器](../phase-21-sampling-advanced/21.3-advanced-samplers.md) | 采样与调度进阶（选读） | [3.6 Euler 采样器与玩具去噪器](../phase-03-libtorch-sampling/3.6-euler-sampler.md) |  |
| [21.4 CFG 的机理与变体](../phase-21-sampling-advanced/21.4-cfg-mechanics.md) | 采样与调度进阶（选读） | [10.2 KSampler 与 CFG](../phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [21.5 PAG 与 SAG 注意力引导](../phase-21-sampling-advanced/21.5-pag-sag.md) | 采样与调度进阶（选读） | [10.2 KSampler 与 CFG](../phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成附录 18.1 |
| [21.6 去噪轨迹可视化](../phase-21-sampling-advanced/21.6-denoise-trajectory.md) | 采样与调度进阶（选读） | [3.6 Euler 采样器与玩具去噪器](../phase-03-libtorch-sampling/3.6-euler-sampler.md) |  |
| [21.7 采样器对比实验](../phase-21-sampling-advanced/21.7-sampler-compare.md) | 采样与调度进阶（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成附录 21.3 |
| [22.1 DeepCache](../phase-22-unet-speedup/22.1-deepcache.md) | UNet 加速与少步采样（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [22.2 Token Merging（ToMe）](../phase-22-unet-speedup/22.2-token-merging.md) | UNet 加速与少步采样（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [22.3 LCM 与少步采样](../phase-22-unet-speedup/22.3-lcm.md) | UNet 加速与少步采样（选读） | [10.5 LoRA（选读）](../phase-10-txt2img/10.5-lora.md) |  |
| [22.4 速度与画质综合对比](../phase-22-unet-speedup/22.4-speed-quality-compare.md) | UNet 加速与少步采样（选读） | [10.5 LoRA（选读）](../phase-10-txt2img/10.5-lora.md) | 需先完成附录 22.1–22.3 |
| [23.1 DiT 钩子机制](../phase-23-dit-lab/23.1-dit-hooks.md) | DiT 内部观察与实验（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [23.2 联合注意力图可视化](../phase-23-dit-lab/23.2-joint-attention-maps.md) | DiT 内部观察与实验（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [23.3 逐块消融与层跳过](../phase-23-dit-lab/23.3-block-ablation.md) | DiT 内部观察与实验（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [23.4 dit-lab Skill](../phase-23-dit-lab/23.4-dit-lab-skill.md) | DiT 内部观察与实验（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [24.1 ControlNet 原理](../phase-24-control-adapters/24.1-controlnet-principle.md) | 条件控制与适配器（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [24.2 ControlNet 实现与加载](../phase-24-control-adapters/24.2-controlnet-impl.md) | 条件控制与适配器（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [24.3 ControlNet 与 Canny 节点](../phase-24-control-adapters/24.3-controlnet-canny.md) | 条件控制与适配器（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [24.4 IP-Adapter](../phase-24-control-adapters/24.4-ip-adapter.md) | 条件控制与适配器（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [24.5 Prompt-to-Prompt](../phase-24-control-adapters/24.5-prompt-to-prompt.md) | 条件控制与适配器（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成附录 18.3 |
| [24.6 control Skill](../phase-24-control-adapters/24.6-control-skill.md) | 条件控制与适配器（选读） | [10.4 Skills 与 Discord 演示](../phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [25.1 上下文编辑](../phase-25-dit-adapters/25.1-in-context-editing.md) | DiT 适配与编辑（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [25.2 DiT 的 ControlNet](../phase-25-dit-adapters/25.2-dit-controlnet.md) | DiT 适配与编辑（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [25.3 量化模型加 LoRA](../phase-25-dit-adapters/25.3-quantized-lora.md) | DiT 适配与编辑（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [25.4 dit-adapters Skill](../phase-25-dit-adapters/25.4-dit-adapters-skill.md) | DiT 适配与编辑（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [26.1 自动求导与训练基础](../phase-26-training/26.1-autograd-basics.md) | 训练（选读） | [3.2 Tensor 与 Latent 值类型](../phase-03-libtorch-sampling/3.2-tensor-latent-values.md) |  |
| [26.2 LoRA 训练](../phase-26-training/26.2-lora-training.md) | 训练（选读） | [10.5 LoRA（选读）](../phase-10-txt2img/10.5-lora.md) |  |
| [26.3 Textual Inversion](../phase-26-training/26.3-textual-inversion.md) | 训练（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [26.4 DiT 的 LoRA 训练](../phase-26-training/26.4-dit-lora-training.md) | 训练（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) | 需先完成附录 26.2 |
| [26.5 训练与评估 Skill](../phase-26-training/26.5-train-eval-skill.md) | 训练（选读） | [10.5 LoRA（选读）](../phase-10-txt2img/10.5-lora.md) |  |
| [26.6 模型融合与剪枝介绍（选读·原理阅读）](../phase-26-training/26.6-model-merging-intro.md) | 训练（选读） | [10.5 LoRA（选读）](../phase-10-txt2img/10.5-lora.md) |  |
| [27.1 基础评估指标](../phase-27-eval-inversion/27.1-basic-metrics.md) | 评估与反演（选读） | [2.4 图像处理节点](../phase-02-opencv-handles/2.4-image-processing-nodes.md) |  |
| [27.2 LPIPS 与 CLIPScore](../phase-27-eval-inversion/27.2-lpips-clipscore.md) | 评估与反演（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [27.3 DDIM 反演](../phase-27-eval-inversion/27.3-ddim-inversion.md) | 评估与反演（选读） | [10.2 KSampler 与 CFG](../phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [27.4 null-text 反演](../phase-27-eval-inversion/27.4-null-text-inversion.md) | 评估与反演（选读） | [10.2 KSampler 与 CFG](../phase-10-txt2img/10.2-ksampler-cfg.md) | 需先完成附录 27.3 |
| [27.5 真实图的提示词编辑](../phase-27-eval-inversion/27.5-real-image-p2p.md) | 评估与反演（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成附录 24.5 与 27.3 |
| [27.6 eval Skill](../phase-27-eval-inversion/27.6-eval-skill.md) | 评估与反演（选读） | [10.4 Skills 与 Discord 演示](../phase-10-txt2img/10.4-skills-discord-demo.md) |  |
| [28.1 特征缓存](../phase-28-dit-speedup/28.1-feature-caching.md) | DiT 加速（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [28.2 Token 合并与稀疏注意力](../phase-28-dit-speedup/28.2-token-merge-sparse.md) | DiT 加速（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [28.3 SVDQuant（选读·原理阅读）](../phase-28-dit-speedup/28.3-svdquant-reading.md) | DiT 加速（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [28.4 速度与画质综合对比](../phase-28-dit-speedup/28.4-dit-speed-compare.md) | DiT 加速（选读） | [12.8 Flux 文生图](../phase-12-flux/12.8-flux-txt2img.md) |  |
| [29.1 UNet 剖析器](../phase-29-inference-engineering/29.1-unet-profiler.md) | 推理工程（选读） | [7.4 性能基线](../phase-07-unet-assembly/7.4-unet-baseline.md) |  |
| [29.2 CFG 批合并](../phase-29-inference-engineering/29.2-cfg-batching.md) | 推理工程（选读） | [10.2 KSampler 与 CFG](../phase-10-txt2img/10.2-ksampler-cfg.md) |  |
| [29.3 内存格式与精度开关](../phase-29-inference-engineering/29.3-memory-format-precision.md) | 推理工程（选读） | [7.4 性能基线](../phase-07-unet-assembly/7.4-unet-baseline.md) |  |
| [29.4 CUDA Graph](../phase-29-inference-engineering/29.4-cuda-graph.md) | 推理工程（选读） | [7.4 性能基线](../phase-07-unet-assembly/7.4-unet-baseline.md) |  |
| [29.5 跨后端可复现](../phase-29-inference-engineering/29.5-cross-backend-repro.md) | 推理工程（选读） | [9.4 耗时与显存对比](../phase-09-ggml-unet/9.4-ggml-vs-libtorch-perf.md) |  |
| [30.1 精度基础](../phase-30-quantization/30.1-precision-basics.md) | UNet 量化（选读） | [4.8 fp16 数值陷阱](../phase-04-weights-vae/4.8-fp16-pitfalls.md) |  |
| [30.2 量化原理](../phase-30-quantization/30.2-quantization-theory.md) | UNet 量化（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [30.3 加载时 int8 量化](../phase-30-quantization/30.3-load-time-int8.md) | UNet 量化（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [30.4 加载时 int4 分组量化](../phase-30-quantization/30.4-load-time-int4.md) | UNet 量化（选读） | [7.3 完整前向与单步对照](../phase-07-unet-assembly/7.3-unet-forward-verify.md) |  |
| [30.5 按层策略与评估](../phase-30-quantization/30.5-layer-policy-eval.md) | UNet 量化（选读） | [10.1 LoadCheckpoint 节点](../phase-10-txt2img/10.1-load-checkpoint.md) |  |
| [30.6 量化敏感度扫描](../phase-30-quantization/30.6-sensitivity-scan.md) | UNet 量化（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) |  |
| [30.7 与 ggml 原生量化对比](../phase-30-quantization/30.7-ggml-quant-compare.md) | UNet 量化（选读） | [9.4 耗时与显存对比](../phase-09-ggml-unet/9.4-ggml-vs-libtorch-perf.md) |  |
| [30.8 缓存量化文件（选读中的选读）](../phase-30-quantization/30.8-quant-cache-file.md) | UNet 量化（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成附录 30.5 |
| [30.9 自写融合 CUDA kernel（选读中的选读）](../phase-30-quantization/30.9-fused-cuda-kernel.md) | UNet 量化（选读） | [10.3 节点装配与显存管理](../phase-10-txt2img/10.3-txt2img-assembly.md) | 需先完成附录 30.5 |
