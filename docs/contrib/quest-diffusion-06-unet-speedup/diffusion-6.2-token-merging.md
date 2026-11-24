# diffusion-6.2 Token Merging（ToMe）

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/diffusion-06-unet-speedup`（支线不打 tag，用 `git log quest/diffusion-06-unet-speedup --grep="^chapter-diffusion-6.2"` 定位本章提交）
- 所属支线：[支线 diffusion-06：UNet 加速与少步采样](README.md)
- 可开始于：主线 [10.3 节点装配与显存管理](../../tutorial/phase-10-txt2img/10.3-txt2img-assembly.md)
- 一句话目标：合并相似 token 以减少注意力计算量

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-diffusion-6.2-token-merging:` 开头，可用 `git show --stat` 查看。）

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

- [paper-13.2 ToMe for SD](../quest-paper-13-speedup-quant/paper-13.2-tome-sd.md)（支线 paper-13：加速与量化（扩散支线））：解析 Bolya 和 Hoffman（2023）的《Token Merging for Fast Stable Diffusion》：在注意力前合并相似的词元；对照支线 diffusion-6.2（需先完成支线 diffusion-6.2（对照它的实现））

## 11. 下一章预告

待写。
