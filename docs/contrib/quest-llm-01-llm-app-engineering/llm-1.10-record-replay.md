# llm-1.10 录制与回放测试

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.10"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.3 Tool Calling 循环](../../tutorial/phase-01-llm-chat/1.3-tool-calling.md)
- 一句话目标：把 LLM 的请求和响应录成文件，测试时用假 Provider 回放，让 agent 循环的测试不花钱、可重复

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-llm-1.10-record-replay:` 开头，可用 `git show --stat` 查看。）

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

- [diffusion-15.7 澄清提问](../quest-diffusion-15-vision-planning/diffusion-15.7-clarifying-questions.md)（支线 diffusion-15：视觉定位与布局规划）：判断需求是不是够清楚：该直接做、还是先问；设计提问的格式，用一组评测用例看它问得对不对（用 llm-1.10 的录制回放让评测可重复）
- [llm-4.8 Skill 评测与回归](../quest-llm-04-skills-advanced/llm-4.8-skill-evals.md)（支线 llm-04：Skills 进阶）：为每个 Skill 写触发用例和结果检查，改了说明或工具后跑一遍，防止悄悄变差（依赖 llm-4.3；用 llm-1.10 的录制回放让评测可重复）
- [llm-5.7 沙箱逃逸测试](../quest-llm-05-sandbox/llm-5.7-escape-tests.md)（支线 llm-05：沙箱与脚本编排）：写一组专门试图逃出沙箱的脚本（读文件、联网、死循环、吃内存、改全局），作为回归测试，每次改沙箱都要跑（依赖 llm-5.2 至 llm-5.5；用 llm-1.10 的录制回放）

## 11. 下一章预告

待写。
