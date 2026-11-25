# platform-2.3 事件循环（Linux）

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/platform-02-event-loop-network`（支线不打 tag，用 `git log quest/platform-02-event-loop-network --grep="^chapter-platform-2.3"` 定位本章提交）
- 所属支线：[支线 platform-02：事件循环和网络](README.md)
- 可开始于：主线 [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md)
- 一句话目标：在 Linux 上重做 2.1：同一套 Boost.Asio 代码在 Linux 的事件循环（epoll）上跑，测试全部通过，记录与 Windows（IOCP）的差异

## 1. 本章目标与成果

待写。（格式参照主线 1.1：先一段导语，接“做完后你会得到：”和一个要点列表；再写“先看效果：”，给出真实的运行（命令放在代码块里，实际得到的输出放在另一个代码块里），最后用一两句话说明输出里每一部分是什么。不用小标题。）

## 2. 背景与概念

待写。（要点：2.1 在 Windows 上做的 `Sleeper`、`WithTimeout`、执行器、`TaskGroup` 在 Linux 上是否原样可用；`_WIN32_WINNT` 之类的 Windows 专属设置在这里怎么处理；线程池和 `strand` 的行为有没有差别）

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-platform-2.3-asio-linux:` 开头，可用 `git show --stat` 查看。）

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
