# diffusion-6.3 LCM 与少步采样

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/diffusion-06-unet-speedup`（支线不打 tag，用 `git log quest/diffusion-06-unet-speedup --grep="^chapter-diffusion-6.3"` 定位本章提交）
- 所属支线：[支线 diffusion-06：UNet 加速与少步采样](README.md)
- 可开始于：主线 [10.5 LoRA](../../tutorial/phase-10-txt2img/10.5-lora.md)
- 一句话目标：实现 LCM 调度器并加载 LCM-LoRA（依赖 10.5 LoRA），约 4 步出图

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-diffusion-6.3-lcm:` 开头，可用 `git show --stat` 查看。）

## 4. 关键代码讲解

待写。（贴真实片段并注明文件与行号范围。）

## 5. 构建与运行

待写。（可复制的命令与预期输出。）

## 6. 验证

待写。（本章的 `ctest` 或手动检查项。）

## 7. 技术资料

待写。（用到的库、版本、API、设计取舍、被否决的方案。）

## 8. 常见问题与坑

待写。

## 9. 练习

待写。

## 10. 支线

下面这些其他支线的章节需要先做完本章：

- [paper-13.3 潜空间一致性模型（LCM）](../quest-paper-13-speedup-quant/paper-13.3-lcm.md)（支线 paper-13：加速与量化（扩散支线））：解析 Luo 等人（2023）的《Latent Consistency Models: Synthesizing High-Resolution Images with Few-Step Inference》：把潜空间扩散模型蒸馏成几步出图；对照支线 diffusion-6.3（需先完成支线 diffusion-6.3（对照它的实现））

## 11. 下一章预告

待写。
