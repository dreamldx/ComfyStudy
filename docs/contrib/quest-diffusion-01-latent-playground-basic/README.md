# 支线 diffusion-01：潜空间玩法·入门

- 类型：支线（草案，到该阶段前再细化）
- 状态：未开始
- 分支：`quest/diffusion-01-latent-playground-basic`（支线不打 tag、不发 Release）
- 起点：主线 tag `chapter-10.7-inpaint-outpaint`
- 完成标准：对同一提示词做噪声插值动画与无缝平铺纹理，并逐步预览去噪过程

## 本阶段章节

| 编号 | 章节 | 提交前缀 | 目标 |
|---|---|---|---|
| diffusion-1.1 | [逐步快速预览（Latent2RGB）](diffusion-1.1-latent-preview.md) | `chapter-diffusion-1.1-latent-preview:` | 用一个线性矩阵把 4 通道潜变量近似成 RGB，每步近乎零成本地预览 |
| diffusion-1.2 | [无缝平铺纹理](diffusion-1.2-seamless-tiling.md) | `chapter-diffusion-1.2-seamless-tiling:` | 把 UNet 与 VAE 里卷积的填充改成环形填充，生成可无限平铺的纹理 |
| diffusion-1.3 | [噪声插值与潜空间动画](diffusion-1.3-noise-interpolation.md) | `chapter-diffusion-1.3-noise-interpolation:` | 用球面插值（slerp）在两个种子之间过渡，做渐变动画 |
| diffusion-1.4 | [种子变体](diffusion-1.4-seed-variation.md) | `chapter-diffusion-1.4-seed-variation:` | 用种子之间的微小插值得到同一构图的小变化 |
| diffusion-1.5 | [构图锁定](diffusion-1.5-composition-lock.md) | `chapter-diffusion-1.5-composition-lock:` | 说明同一种子配不同提示词为何构图保持一致，并利用它 |
| diffusion-1.6 | [涂鸦转图](diffusion-1.6-scribble-to-image.md) | `chapter-diffusion-1.6-scribble-to-image:` | 在初始潜变量里画色块，再用 img2img 控制构图与配色（依赖 10.6） |
| diffusion-1.7 | [潜空间合成与混合](diffusion-1.7-latent-compositing.md) | `chapter-diffusion-1.7-latent-compositing:` | 在潜空间按遮罩合成两个潜变量，并与像素空间合成对比 |
| diffusion-1.8 | [高分辨率修复](diffusion-1.8-hires-fix.md) | `chapter-diffusion-1.8-hires-fix:` | 先小图再放大重绘，说明直接生成大图为何会出现重复和双头 |
| diffusion-1.9 | [latent-lab Skill](diffusion-1.9-latent-playground-skill.md) | `chapter-diffusion-1.9-latent-playground-skill:` | 整理潜空间玩法节点为 Skill |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
