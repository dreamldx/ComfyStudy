# diffusion-5.1 预测目标：ε、v、x0

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/diffusion-05-sampling-advanced`（支线不打 tag，用 `git log quest/diffusion-05-sampling-advanced --grep="^chapter-diffusion-5.1"` 定位本章提交）
- 所属支线：[支线 diffusion-05：采样与调度进阶](README.md)
- 可开始于：主线 [3.6 Euler 采样器与玩具去噪器](../../tutorial/phase-03-libtorch-sampling/3.6-euler-sampler.md)
- 一句话目标：说明网络到底预测什么、三者如何互相换算、SD 2 为何改用 v

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-diffusion-5.1-prediction-targets:` 开头，可用 `git show --stat` 查看。）

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

- [paper-11.1 渐进式蒸馏与 v 预测](../quest-paper-11-sampling-guidance/paper-11.1-progressive-distillation.md)（支线 paper-11：采样与引导进阶（扩散支线））：解析 Salimans 和 Ho（2022）的《Progressive Distillation for Fast Sampling of Diffusion Models》：提出 v 预测，以及逐步减半采样步数的蒸馏；对照支线 diffusion-5.1（需先完成支线 diffusion-5.1（对照它的实现））

## 11. 下一章预告

待写。
