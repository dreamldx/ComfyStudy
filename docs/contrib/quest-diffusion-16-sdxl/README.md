# 支线 diffusion-16：SDXL

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-16-sdxl`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.7-inpaint-outpaint`
- 完成标准：在自己的节点系统里加载 SDXL 检查点并出图：双文本编码器、尺寸与裁剪条件、配置驱动的 UNet 都接进来，原版 VAE 在 fp16 下不出黑图

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-16.1 | [SDXL 与 SD 1.5 的差异](diffusion-16.1-sdxl-overview.md) | `chapter-diffusion-16.1-sdxl-overview:` | 对照两者的结构：更大的 UNet、两个文本编码器、尺寸与裁剪条件，列出要改动的地方 |
| diffusion-16.2 | [双文本编码器](diffusion-16.2-dual-text-encoders.md) | `chapter-diffusion-16.2-dual-text-encoders:` | 接入两个 CLIP 文本编码器，取倒数第二层隐藏状态（clip skip），并输出 pooled 向量（见 5.4 的设计备忘） |
| diffusion-16.3 | [尺寸与裁剪条件](diffusion-16.3-size-crop-conditioning.md) | `chapter-diffusion-16.3-size-crop-conditioning:` | 把原始尺寸、裁剪坐标与目标尺寸编码后加进时间步嵌入 |
| diffusion-16.4 | [SDXL 的 UNet](diffusion-16.4-sdxl-unet.md) | `chapter-diffusion-16.4-sdxl-unet:` | 用配置驱动的 UNet（见 7.1 的设计备忘）实现各级 Transformer 深度不同的结构 |
| diffusion-16.5 | [键名映射与权重加载](diffusion-16.5-key-mapping.md) | `chapter-diffusion-16.5-key-mapping:` | 用表驱动的权重名映射（见 4.3 的设计备忘）加载 SDXL 检查点 |
| diffusion-16.6 | [SDXL 的 VAE 与 fp16](diffusion-16.6-sdxl-vae-fp16.md) | `chapter-diffusion-16.6-sdxl-vae-fp16:` | 原版 VAE 容易在 fp16 下溢出，对比修复版 VAE 与 fp32 解码（见 4.8） |
| diffusion-16.7 | [SDXL 文生图](diffusion-16.7-sdxl-txt2img.md) | `chapter-diffusion-16.7-sdxl-txt2img:` | 串起节点出 SDXL 图，并与 SD 1.5 对比耗时与显存 |

## 阶段总览

SDXL 是 SD 1.5 的放大加补丁，难度介于 SD 1.5 与 FLUX 之间。可选，主线不依赖它。

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
