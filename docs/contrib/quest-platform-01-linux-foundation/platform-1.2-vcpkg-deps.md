# platform-1.2 用 vcpkg 管理依赖

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/platform-01-linux-foundation`（支线不打 tag，用 `git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.2"` 定位本章提交）
- 所属支线：[支线 platform-01：Linux 阶段 0](README.md)
- 可开始于：主线 [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md)
- 一句话目标：在 Linux 上装 vcpkg，沿用主线的 `vcpkg.json` 和 `builtin-baseline`，各库的版本自动与 Windows 一致；先备好 vcpkg 自举要用的工具（git、curl、zip、unzip、tar，到时核实），把它们和 vcpkg 本身加进 `check-env.sh`；用 `x64-linux` 三元组（是否改用动态三元组，到时核实），并让 vcpkg 构建这些库时也用固定的 `g++-16`；在项目目录里用清单模式装出 nlohmann-json、curl、gtest、spdlog、cli11，重点核对 curl（Linux 上用 OpenSSL 而不是 Schannel）；把同一份 manifest 在 Windows 和 Linux 上解析出的版本列出来对比，记录有差异的包

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-platform-1.2-vcpkg-deps:` 开头，可用 `git show --stat` 查看。）

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
