# platform-1.3 Linux 的 CMake 平台文件与最小移植

- 状态：未开始 ｜ 类型：支线 ｜ 分支：`quest/platform-01-linux-foundation`（支线不打 tag，用 `git log quest/platform-01-linux-foundation --grep="^chapter-platform-1.3"` 定位本章提交）
- 所属支线：[支线 platform-01：Linux 阶段 0](README.md)
- 可开始于：主线 [0.5 DLL 集中管理与依赖检查](../../tutorial/phase-00-toolchain/0.5-dll-management.md)
- 一句话目标：在主线已拆分的基础上新增 `cmake/platform/linux.cmake`：C++20、`-Wall -Wextra -Werror`、`compile_commands.json`、sanitizer 选项，第三方库用 `find_package`（来自 platform-1.2 的 vcpkg）；预设含 debug / release / asan，并在预设里设 `CMAKE_TOOLCHAIN_FILE` 和 `VCPKG_TARGET_TRIPLET`；补上能编过的最小 Linux 分支（`config::Get` 读环境变量，`config::ExeDirectory` 用 `/proc/self/exe`），做一次完整构建并跑通测试（Windows 专用的除外）

## 1. 本章目标与成果

待写。

## 2. 背景与概念

待写。

## 3. 本章改动

待写。（列出本章提交新增/修改的文件；提交信息以 `chapter-platform-1.3-cmake-linux:` 开头，可用 `git show --stat` 查看。）

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
