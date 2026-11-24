# 支线

这里是和教程平级的支线内容，结构和 [教程](../tutorial/README.md) 相同：每个阶段一个文件夹，含 `README.md` 与每章一个文件，另有 `reference/`（支线与主线的衔接表）和 `_template/`（章节模板）。

每个支线阶段的代码放在独立分支（`quest/diffusion-NN-<slug>` 或 `quest/llm-NN-<slug>`），不打 tag、不发 Release，也不合并回 `main`；章节头部写明分支名，用 `git log <分支> --grep="^chapter-diffusion-N.M"`（LLM 方向用 `llm`）定位本章提交。每章会写明做完主线哪一步之后就可以开始，分支起点和每章的衔接见 [支线与主线的衔接表](reference/quest-links.md)。

**分支上的 README。** 每个支线分支根目录的 `README.md` 替换成这条支线自己的说明，而不是主线的 README：开头写分支名、起点（主线 tag 或上游分支）和“不合并回 `main`”，然后每章一节，写本章实现的**目的**和**要点说明**，并附上这一章的文档链接，写成站点的绝对地址（形如 `https://dreamldx.github.io/ComfyStudy/contrib/<阶段文件夹>/<章节文件名，去掉 .md>/`）。每个章节提交同时更新这份 README，所以读者检出任何一个提交，都能看到到这一章为止实现了什么、为什么这样做、详细讲解在哪里。链接用绝对地址，是因为分支里的 `docs/` 停在分支起点，会落后于 `main`。以另一个支线分支为起点的支线（比如 llm-04 从 llm-01 的末端切出），README 只写自己这一条支线的内容，开头注明上游支线；合入别的支线分支时，README 手工合并成两段。

支线放在主线（0–16）之后，可整段跳过，分成**扩散模型**、**LLM**、**platform（Linux 版）** 和 **paper（论文解析）** 四个方向，各自从 01 编号。扩散模型方向按“实现难易与有趣程度”从易到难、从有趣到次要排序；LLM 方向是后来加入的，各阶段相互独立。除阶段 0–10 与 `diffusion-14` 外均为草案，到该阶段前再细化。

### 扩散模型方向

| 编号 | 主题 | 类型 | 章节数 | 完成标准 |
|---|---|---|---|---|
| diffusion-01 | [潜空间玩法·入门](quest-diffusion-01-latent-playground-basic/README.md) | 支线·草案 | 9 | 对同一提示词做噪声插值动画与无缝平铺纹理，并逐步预览去噪过程 |
| diffusion-02 | [UNet 内部观察与实验](quest-diffusion-02-unet-lab/README.md) | 支线·草案 | 6 | 在 Discord 里让 LLM 分析某个词影响了画面哪里，并做逐层消融实验、写出报告 |
| diffusion-03 | [潜空间玩法·进阶](quest-diffusion-03-latent-playground-adv/README.md) | 支线·草案 | 7 | 生成首尾相接的全景图、沿语义方向调节属性，并用梯度引导控制颜色 |
| diffusion-04 | [条件与提示技术](quest-diffusion-04-conditioning-prompts/README.md) | 支线·草案 | 4 | 用强调语法、CLIP skip 与条件组合精细控制提示词 |
| diffusion-05 | [采样与调度进阶](quest-diffusion-05-sampling-advanced/README.md) | 支线·草案 | 7 | 让 LLM 自己跑采样器对比实验，并逐步预览去噪轨迹 |
| diffusion-06 | [UNet 加速与少步采样](quest-diffusion-06-unet-speedup/README.md) | 支线·草案 | 4 | 同一提示词在基线、DeepCache、ToMe、LCM 下对比速度与画质 |
| diffusion-07 | [DiT 内部观察与实验](quest-diffusion-07-dit-lab/README.md) | 支线·草案 | 4 | 在 Discord 里让 LLM 分析某个词在联合注意力里看了画面哪里，并做逐块消融实验、写出报告 |
| diffusion-08 | [条件控制与适配器](quest-diffusion-08-control-adapters/README.md) | 支线·草案 | 6 | 用边缘图控制构图，并用参考图或编辑注意力图改变画面 |
| diffusion-09 | [DiT 适配与编辑](quest-diffusion-09-dit-adapters/README.md) | 支线·草案 | 4 | 用参考图让模型“看着原图改”，并给量化模型叠加 LoRA |
| diffusion-10 | [训练](quest-diffusion-10-training/README.md) | 支线·草案 | 6 | 用少量图训练一个 LoRA 或一个新词嵌入，并在 Discord 生成对比 |
| diffusion-11 | [评估与反演](quest-diffusion-11-eval-inversion/README.md) | 支线·草案 | 6 | 用指标评估出图差异，并把真实图反演回噪声后用提示词编辑 |
| diffusion-12 | [DiT 加速](quest-diffusion-12-dit-speedup/README.md) | 支线·草案 | 4 | 同一提示词在基线、特征缓存、Token 合并下对比速度与画质 |
| diffusion-13 | [推理工程](quest-diffusion-13-inference-engineering/README.md) | 支线·草案 | 5 | 剖析 UNet 的成本，并用批合并、精度开关与 CUDA Graph 优化，保证跨后端结果可复现 |
| diffusion-14 | [UNet 量化](quest-diffusion-14-quantization/README.md) | 支线 | 9 | `unet_precision` 可选 fp16、int8、int4，出图差异与显存可量化比较 |
| diffusion-15 | [视觉定位与布局规划](quest-diffusion-15-vision-planning/README.md) | 支线·草案 | 8 | 在 Discord 里说“把左边那只猫换成狗”，机器人自己找出位置、做出遮罩并局部重画；说“左边是山，右边是海边的人”，它先规划布局再按区域出图；需求含糊时先问清楚再做 |
| diffusion-16 | [SDXL](quest-diffusion-16-sdxl/README.md) | 支线·草案 | 7 | 在自己的节点系统里加载 SDXL 检查点并出图：双文本编码器、尺寸与裁剪条件、配置驱动的 UNet 都接进来，原版 VAE 在 fp16 下不出黑图 |
| diffusion-17 | [后端对照：stable-diffusion.cpp](quest-diffusion-17-sdcpp-compare/README.md) | 支线·草案 | 2 | 同一提示词在自研后端与 stable-diffusion.cpp 后端出图并对比（SD 1.5、Flux、Qwen-Image） |

### LLM 方向

| 编号 | 主题 | 类型 | 章节数 | 完成标准 |
|---|---|---|---|---|
| llm-01 | [LLM 应用工程](quest-llm-01-llm-app-engineering/README.md) | 支线·草案 | 12 | 搭好 Boost.Asio 事件循环；测出 libcurl 三种用法在事件循环里的差别并选定机器人里用哪一种；把 `Post` 抽成 `Transport` 接口并用契约测试守住；在 Discord 里机器人边想边发（流式更新同一条消息），输出可校验的结构化 JSON，长对话自动压缩，并给出每个频道的用量与费用；用录好的响应就能确定地回放测试；聊天历史的表结构有版本和升级脚本，一轮对话的消息在一个事务里写入，历史里不存令牌和密钥 |
| llm-02 | [Agent 进阶](quest-llm-02-agent-advanced/README.md) | 支线·草案 | 11 | 机器人能记住用户的偏好、按语义检索自己以前生成的图、并行调用工具、把外部 MCP 服务当作 Skill 使用，并在节点图出错时根据校验信息自行修正；并能搜索、并只读查询自己的聊天历史 |
| llm-03 | [用 LLM 改进出图](quest-llm-03-llm-image-craft/README.md) | 支线·草案 | 7 | 用户只说一句模糊的话，机器人自动扩写提示词、出图、让视觉模型评审并重画，最后发回最好的一张并解释为什么选它 |
| llm-04 | [Skills 进阶](quest-llm-04-skills-advanced/README.md) | 支线·草案 | 9 | Skill 用统一的文件格式描述，模型只在需要时才读到它的正文和资源；有一套评测集能量化“该用哪个 Skill”选得准不准；录好的操作可以自动生成 Skill，并带着权限声明打包、热重载 |
| llm-05 | [沙箱与脚本编排](quest-llm-05-sandbox/README.md) | 支线·草案 | 8 | 模型写的一小段脚本能在沙箱里安全地运行：限制时间、内存和可用的函数；脚本能在预算内循环调用受控的工具，把批量任务压成一轮；一组逃逸用例保证它出不了沙箱 |

### platform 方向（Linux 版）

Linux 实现只存在于这些分支里，不进入主线的 `src/`，也不进入 Release。编号按“依附的主线阶段”排序，依附支线的排在最后。**功能**支线是 Linux 版的适配，01、02、03、04、07 串成一条线；**分析**支线做性能与优化对比，是叶子，不作为任何支线的起点。只给有 Linux 差异的阶段和支线建 platform 支线，没有已知差异的不建空支线，以后发现再按顺序追加。

| 编号 | 主题 | 类型 | 类别 | 章节数 | 完成标准 |
|---|---|---|---|---|---|
| platform-01 | [Linux 阶段 0](quest-platform-01-linux-foundation/README.md) | 支线 | 功能 | 7 | 程序在 Linux 上运行时输出已加载的配置项名称；全部测试在 ASan 与 UBSan 下通过 |
| platform-02 | [libtorch（Linux）](quest-platform-02-libtorch-linux/README.md) | 支线·草案 | 功能 | 2 | 在 Linux 上用官方预编译包跑通 libtorch 的 hello tensor |
| platform-03 | [内存映射（Linux）](quest-platform-03-mmap-linux/README.md) | 支线·草案 | 功能 | 1 | safetensors 的内存映射读取在 Linux 上通过测试，大文件与对齐都已验证 |
| platform-04 | [ggml（Linux）](quest-platform-04-ggml-linux/README.md) | 支线·草案 | 功能 | 2 | 在 Linux 上编译出带 CUDA 后端的 ggml，并与 libtorch 共存 |
| platform-05 | [性能对比·UNet（Linux 与 Windows）](quest-platform-05-perf-unet/README.md) | 支线·草案 | 分析 | 4 | 给出 UNet 单步前向在 Linux 与 Windows 上的耗时与显存对比，并解释差异 |
| platform-06 | [性能对比·SD 1.5 出图](quest-platform-06-perf-sd15/README.md) | 支线·草案 | 分析 | 4 | 给出 SD 1.5 端到端出图在 Linux 与 Windows 上的耗时拆分与结果一致性 |
| platform-07 | [ONNX Runtime（Linux）](quest-platform-07-onnxruntime-linux/README.md) | 支线·草案 | 功能 | 1 | 在 Linux 上让 ORT 的 CUDA 执行器与 libtorch 共用 CUDA / cuDNN 运行库 |
| platform-08 | [性能对比·FLUX](quest-platform-08-perf-flux/README.md) | 支线·草案 | 分析 | 3 | 给出量化版 FLUX 出图在 Linux 与 Windows 上的耗时与显存对比 |
| platform-09 | [性能对比·Qwen-Image](quest-platform-09-perf-qwen-image/README.md) | 支线·草案 | 分析 | 2 | 给出 Qwen-Image 分段加载与逐块卸载在 Linux 与 Windows 上的耗时与显存对比 |
| platform-10 | [源码构建与官方版本对比（Linux）](quest-platform-10-source-build/README.md) | 支线·草案 | 分析 | 5 | 说清哪些库值得在 Linux 上源码构建、收益多大、代价是什么 |
| platform-11 | [GCC 与 Clang 的编译器性能优化](quest-platform-11-gcc-clang/README.md) | 支线·草案 | 分析 | 5 | 整理 GCC 与 Clang 的优化选项，给出 Linux 线推荐的优化选项 |
| platform-12 | [Agent 进阶（Linux 完整版）](quest-platform-12-agent-advanced-linux/README.md) | 支线·草案 | 功能 | 11 | Linux 上 llm-02 的全部章节构建通过，并记录与 Windows 的差异 |
| platform-13 | [沙箱与脚本编排（Linux 完整版）](quest-platform-13-sandbox-linux/README.md) | 支线·草案 | 功能 | 8 | Linux 上 llm-05 的全部章节构建通过，子进程沙箱改用 Linux 的机制并通过逃逸测试 |
| platform-14 | [Skills 进阶（Linux 完整版）](quest-platform-14-skills-advanced-linux/README.md) | 支线·草案 | 功能 | 9 | Linux 上 llm-04 的全部章节构建通过，并记录与 Windows 的差异 |

### paper 方向（论文解析）

每个支线阶段解析一组论文，每章一篇：讲清论文的核心思想与关键公式，并结合本项目的实现讲解，只有文档，不改代码。编号按“对应的主线阶段”排序，后面四个对应扩散支线。

| 编号 | 主题 | 类型 | 章节数 | 完成标准 |
|---|---|---|---|---|
| paper-01 | [LLM 与 Agent](quest-paper-01-llm-agent/README.md) | 支线·草案 | 2 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-02 | [扩散模型基础](quest-paper-02-diffusion-basics/README.md) | 支线·草案 | 6 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-03 | [潜空间与 VAE](quest-paper-03-latent-vae/README.md) | 支线·草案 | 2 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-04 | [Transformer 与文本编码](quest-paper-04-transformer-text/README.md) | 支线·草案 | 3 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-05 | [UNet 与注意力](quest-paper-05-unet-attention/README.md) | 支线·草案 | 4 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-06 | [引导、适配与编辑](quest-paper-06-guidance-editing/README.md) | 支线·草案 | 4 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-07 | [超分](quest-paper-07-super-resolution/README.md) | 支线·草案 | 1 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-08 | [DiT 与流匹配](quest-paper-08-dit-flow/README.md) | 支线·草案 | 8 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-09 | [FLUX 与蒸馏](quest-paper-09-flux-distillation/README.md) | 支线·草案 | 3 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-10 | [Qwen-Image 与大语言模型](quest-paper-10-qwen-llm/README.md) | 支线·草案 | 6 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-11 | [采样与引导进阶（扩散支线）](quest-paper-11-sampling-guidance/README.md) | 支线·草案 | 8 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-12 | [条件控制与编辑（扩散支线）](quest-paper-12-control-editing/README.md) | 支线·草案 | 5 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-13 | [加速与量化（扩散支线）](quest-paper-13-speedup-quant/README.md) | 支线·草案 | 4 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |
| paper-14 | [评估与 SDXL（扩散支线）](quest-paper-14-evaluation-sdxl/README.md) | 支线·草案 | 3 | 逐篇读懂本组论文，并能指出每篇的方法在本项目哪几行代码里实现 |

## 目录结构

- `quest-diffusion-NN-<slug>/`、`quest-llm-NN-<slug>/`、`quest-platform-NN-<slug>/`、`quest-paper-NN-<slug>/`：每个支线阶段一个文件夹，含 `README.md`（阶段总览）与每章一个文件 `diffusion-N.M-<slug>.md` 或 `llm-N.M-<slug>.md`。
- `reference/`：[支线与主线的衔接表](reference/quest-links.md)。
- `_template/chapter.md`：支线章节模板。
