---
hide:
  - navigation
  - toc
---

# ComfyStudy

用 C++ 从零实现一个“ComfyUI 式”的图像生成系统：由 LLM 通过 Tool Calling 一步步调用节点，自己组装流程，用户通过 Discord 对话，节点与模型推理全部自己实现。这是一份逐章的中文教程，每一章对应一个 Git tag。

- [教程（主线）](tutorial/README.md)：阶段 0–16，从工具链到出图，按章节顺序读。
- [支线](contrib/README.md)：扩散模型方向（UNet、DiT、采样、训练、量化等）和 LLM 方向（应用工程、Agent、Skills、沙箱等），可整段跳过。
- 源码与完整说明在 [GitHub 仓库](https://github.com/dreamldx/ComfyStudy)。

## 依赖关系

<!-- dep-graph:start -->

每一行是一个主线阶段，按阶段顺序；右边三列是三个方向的支线。支线放在它**最早可以开始**的那一行：行是它要求做完的最晚主线阶段，格子里的“N.M 起”具体到哪一章，同一格里按这个顺序排列。支线名字下面写它和其他支线的关系：“接”是从对方分支的末端切出，“合入”是要把对方分支的提交合进来，“可选”是可选的依赖；从另一条支线接出来的，所在行不早于它所接的支线。主线不依赖任何支线。每个依赖的具体章节写在[支线与主线的衔接表](contrib/reference/quest-links.md)里。

| 主线阶段 | LLM 支线 | 扩散模型支线 | platform 支线（Linux 版） | paper 支线（论文解析） |
|---|---|---|---|---|
| **[0 工具链与骨架](tutorial/phase-00-toolchain/README.md)** | — | — | [platform-01](contrib/quest-platform-01-linux-foundation/README.md) Linux 阶段 0<br>0.5 起 | — |
| **[1 LLM 与对话通道](tutorial/phase-01-llm-chat/README.md)** | [llm-01](contrib/quest-llm-01-llm-app-engineering/README.md) LLM 应用工程<br>接 platform-02 · 1.9 起<br><br>[llm-05](contrib/quest-llm-05-sandbox/README.md) 沙箱与脚本编排<br>接 llm-01 · 合入 llm-04 · 1.9 起 | — | [platform-02](contrib/quest-platform-02-event-loop-network/README.md) 事件循环和网络<br>接 platform-01 · 1.1 起<br><br>[platform-14](contrib/quest-platform-14-sandbox-linux/README.md) 沙箱与脚本编排（Linux 完整版）<br>接 llm-05 · 合入 platform-01 · 1.9 起 | [paper-01](contrib/quest-paper-01-llm-agent/README.md) LLM 与 Agent<br>1.3 起 |
| **[2 OpenCV：句柄系统与图像处理](tutorial/phase-02-opencv-handles/README.md)** | [llm-04](contrib/quest-llm-04-skills-advanced/README.md) Skills 进阶<br>接 llm-01 · 合入 llm-05 · 2.6 起<br><br>[llm-02](contrib/quest-llm-02-agent-advanced/README.md) Agent 进阶<br>2.7 起 | — | [platform-15](contrib/quest-platform-15-skills-advanced-linux/README.md) Skills 进阶（Linux 完整版）<br>接 llm-04 · 合入 platform-01、platform-14 · 2.6 起<br><br>[platform-13](contrib/quest-platform-13-agent-advanced-linux/README.md) Agent 进阶（Linux 完整版）<br>接 llm-02 · 合入 platform-01 · 2.7 起 | — |
| **[3 libtorch 基础与采样数学](tutorial/phase-03-libtorch-sampling/README.md)** | — | — | [platform-03](contrib/quest-platform-03-libtorch-linux/README.md) libtorch（Linux）<br>接 platform-02 · 3.1 起 | [paper-02](contrib/quest-paper-02-diffusion-basics/README.md) 扩散模型基础<br>3.6 起 |
| **[4 权重加载与 VAE](tutorial/phase-04-weights-vae/README.md)** | — | — | [platform-04](contrib/quest-platform-04-mmap-linux/README.md) 内存映射（Linux）<br>接 platform-03 · 4.1 起 | — |
| **[5 CLIP 文本编码器](tutorial/phase-05-clip/README.md)** | — | — | — | [paper-04](contrib/quest-paper-04-transformer-text/README.md) Transformer 与文本编码<br>5.4 起 |
| **[6 UNet 构件（libtorch）](tutorial/phase-06-unet-blocks/README.md)** | — | — | — | — |
| **[7 UNet 组装（libtorch）](tutorial/phase-07-unet-assembly/README.md)** | — | — | — | [paper-05](contrib/quest-paper-05-unet-attention/README.md) UNet 与注意力<br>7.5 起 |
| **[8 ggml 入门与 UNet 构件](tutorial/phase-08-ggml-blocks/README.md)** | — | — | [platform-05](contrib/quest-platform-05-ggml-linux/README.md) ggml（Linux）<br>接 platform-04 · 8.1 起 | — |
| **[9 ggml UNet 组装](tutorial/phase-09-ggml-unet/README.md)** | — | — | [platform-06](contrib/quest-platform-06-perf-unet/README.md) 性能对比·UNet（Linux 与 Windows）<br>接 platform-05 · 合入 diffusion-13、diffusion-14 · 9.4 起 | — |
| **[10 整合出图（SD 1.5）](tutorial/phase-10-txt2img/README.md)** | [llm-03](contrib/quest-llm-03-llm-image-craft/README.md) 用 LLM 改进出图<br>10.4 起 | [diffusion-13](contrib/quest-diffusion-13-inference-engineering/README.md) 推理工程<br>10.2 起<br><br>[diffusion-14](contrib/quest-diffusion-14-quantization/README.md) UNet 量化<br>10.3 起<br><br>[diffusion-02](contrib/quest-diffusion-02-unet-lab/README.md) UNet 内部观察与实验<br>10.4 起<br><br>[diffusion-03](contrib/quest-diffusion-03-latent-playground-adv/README.md) 潜空间玩法·进阶<br>接 diffusion-02 · 10.4 起<br><br>[diffusion-04](contrib/quest-diffusion-04-conditioning-prompts/README.md) 条件与提示技术<br>10.4 起<br><br>[diffusion-05](contrib/quest-diffusion-05-sampling-advanced/README.md) 采样与调度进阶<br>接 diffusion-02 · 10.4 起<br><br>[diffusion-08](contrib/quest-diffusion-08-control-adapters/README.md) 条件控制与适配器<br>接 diffusion-02 · 10.4 起<br><br>[diffusion-11](contrib/quest-diffusion-11-eval-inversion/README.md) 评估与反演<br>接 diffusion-08 · 10.4 起<br><br>[diffusion-06](contrib/quest-diffusion-06-unet-speedup/README.md) UNet 加速与少步采样<br>10.5 起<br><br>[diffusion-01](contrib/quest-diffusion-01-latent-playground-basic/README.md) 潜空间玩法·入门<br>10.7 起<br><br>[diffusion-15](contrib/quest-diffusion-15-vision-planning/README.md) 视觉定位与布局规划<br>接 llm-01 · 可选 diffusion-03、diffusion-01 · 10.7 起<br><br>[diffusion-16](contrib/quest-diffusion-16-sdxl/README.md) SDXL<br>10.7 起 | [platform-07](contrib/quest-platform-07-perf-sd15/README.md) 性能对比·SD 1.5 出图<br>接 platform-05 · 合入 diffusion-06 · 10.7 起 | [paper-03](contrib/quest-paper-03-latent-vae/README.md) 潜空间与 VAE<br>10.3 起<br><br>[paper-11](contrib/quest-paper-11-sampling-guidance/README.md) 采样与引导进阶（扩散支线）<br>可选 diffusion-02、diffusion-03、diffusion-05 · 10.3 起<br><br>[paper-12](contrib/quest-paper-12-control-editing/README.md) 条件控制与编辑（扩散支线）<br>可选 diffusion-08、diffusion-10、diffusion-11 · 10.3 起<br><br>[paper-06](contrib/quest-paper-06-guidance-editing/README.md) 引导、适配与编辑<br>10.7 起<br><br>[paper-14](contrib/quest-paper-14-evaluation-sdxl/README.md) 评估与 SDXL（扩散支线）<br>可选 diffusion-11、diffusion-16 · 10.7 起 |
| **[11 ONNX Runtime](tutorial/phase-11-onnxruntime/README.md)** | — | — | [platform-08](contrib/quest-platform-08-onnxruntime-linux/README.md) ONNX Runtime（Linux）<br>接 platform-05 · 11.1 起<br><br>[platform-12](contrib/quest-platform-12-gcc-clang/README.md) GCC 与 Clang 的编译器性能优化<br>接 platform-08 · 11.1 起 | [paper-07](contrib/quest-paper-07-super-resolution/README.md) 超分<br>11.2 起 |
| **[12 Seedream 远程节点](tutorial/phase-12-seedream-remote/README.md)** | — | — | — | — |
| **[13 stable-diffusion.cpp 后端](tutorial/phase-13-sdcpp-backend/README.md)** | — | — | [platform-11](contrib/quest-platform-11-source-build/README.md) 源码构建与官方版本对比（Linux）<br>接 platform-08 · 13.2 起 | — |
| **[14 DiT 基础与量化格式](tutorial/phase-14-dit-basics/README.md)** | — | — | — | [paper-08](contrib/quest-paper-08-dit-flow/README.md) DiT 与流匹配<br>14.12 起 |
| **[15 FLUX.1 [schnell]](tutorial/phase-15-flux/README.md)** | — | [diffusion-07](contrib/quest-diffusion-07-dit-lab/README.md) DiT 内部观察与实验<br>15.7 起<br><br>[diffusion-09](contrib/quest-diffusion-09-dit-adapters/README.md) DiT 适配与编辑<br>15.7 起<br><br>[diffusion-10](contrib/quest-diffusion-10-training/README.md) 训练<br>15.7 起<br><br>[diffusion-12](contrib/quest-diffusion-12-dit-speedup/README.md) DiT 加速<br>15.7 起 | [platform-09](contrib/quest-platform-09-perf-flux/README.md) 性能对比·FLUX<br>接 platform-08 · 合入 diffusion-12 · 15.7 起 | [paper-09](contrib/quest-paper-09-flux-distillation/README.md) FLUX 与蒸馏<br>15.6 起<br><br>[paper-13](contrib/quest-paper-13-speedup-quant/README.md) 加速与量化（扩散支线）<br>可选 diffusion-06、diffusion-12 · 15.7 起 |
| **[16 Qwen-Image](tutorial/phase-16-qwen-image/README.md)** | — | [diffusion-17](contrib/quest-diffusion-17-sdcpp-compare/README.md) 后端对照：stable-diffusion.cpp<br>16.6 起 | [platform-10](contrib/quest-platform-10-perf-qwen-image/README.md) 性能对比·Qwen-Image<br>接 platform-08 · 16.6 起 | [paper-10](contrib/quest-paper-10-qwen-llm/README.md) Qwen-Image 与大语言模型<br>16.3 起 |

<!-- dep-graph:end -->
