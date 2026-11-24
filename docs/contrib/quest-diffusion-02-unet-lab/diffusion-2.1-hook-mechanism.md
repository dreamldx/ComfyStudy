# diffusion-2.1 UNet 钩子机制

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/diffusion-02-unet-lab`（支线不打 tag，用 `git log quest/diffusion-02-unet-lab --grep="^chapter-diffusion-2.1"` 定位本章提交）
- 所属支线：[支线 diffusion-02：UNet 内部观察与实验](README.md)
- 可开始于：主线 [7.3 完整前向与单步对照](../../tutorial/phase-07-unet-assembly/7.3-unet-forward-verify.md)
- 一句话目标：在块边界与注意力层暴露中间结果与可改写点（仅 libtorch 后端）

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-diffusion-2.1-hook-mechanism:` 开头，可用 `git show --stat` 查看。）

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

- [diffusion-3.5 区域提示](../quest-diffusion-03-latent-playground-adv/diffusion-3.5-regional-prompting.md)（支线 diffusion-03：潜空间玩法·进阶）：用遮罩让不同区域看不同提示词，在交叉注意力里实现（依赖 diffusion-2.1 钩子）（需先完成支线 diffusion-02.1）
- [diffusion-5.5 PAG 与 SAG 注意力引导](../quest-diffusion-05-sampling-advanced/diffusion-5.5-pag-sag.md)（支线 diffusion-05：采样与调度进阶）：通过扰动注意力来提升画质，需要钩子配合（依赖 diffusion-2.1）（需先完成支线 diffusion-02.1）

## 11. 下一章预告

待写。
