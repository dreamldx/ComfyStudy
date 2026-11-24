# llm-1.7 重试、限流与降级

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.7"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.2 LLM 双协议与 Provider 抽象](../../tutorial/phase-01-llm-chat/1.2-llm-providers.md)
- 一句话目标：指数退避加抖动，尊重服务端的限流提示，主 Provider 持续失败时切到备用 Provider

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。（要点：用主线 1.1 的 `Transport` 返回的错误分类决定是否重试；退避的等待用可取消的 `steady_timer`，取消时立刻结束。）

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-llm-1.7-retry-fallback:` 开头，可用 `git show --stat` 查看。）

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

本章节没有可以开启的支线。

## 11. 下一章预告

待写。
