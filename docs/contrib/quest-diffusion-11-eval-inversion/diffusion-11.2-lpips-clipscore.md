# diffusion-11.2 LPIPS 与 CLIPScore

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/diffusion-11-eval-inversion`（支线不打 tag，用 `git log quest/diffusion-11-eval-inversion --grep="^chapter-diffusion-11.2"` 定位本章提交）
- 所属支线：[支线 diffusion-11：评估与反演](README.md)
- 可开始于：主线 [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md)
- 一句话目标：实现感知距离与图文相似度（需要额外权重，权重来源需先核实）

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-diffusion-11.2-lpips-clipscore:` 开头，可用 `git show --stat` 查看。）

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

- [paper-14.1 LPIPS](../quest-paper-14-evaluation-sdxl/paper-14.1-lpips.md)（支线 paper-14：评估与 SDXL（扩散支线））：解析 Zhang 等人（2018）的《The Unreasonable Effectiveness of Deep Features as a Perceptual Metric》：用深度特征的距离衡量感知差异；对照支线 diffusion-11.2（需先完成支线 diffusion-11.2（对照它的实现））
- [paper-14.2 CLIPScore](../quest-paper-14-evaluation-sdxl/paper-14.2-clipscore.md)（支线 paper-14：评估与 SDXL（扩散支线））：解析 Hessel 等人（2021）的《CLIPScore: A Reference-free Evaluation Metric for Image Captioning》：用 CLIP 的图文相似度评价；对照支线 diffusion-11.2（需先完成支线 diffusion-11.2（对照它的实现））

## 11. 下一章预告

待写。
