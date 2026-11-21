# 阶段 0：工具链与骨架

- 类型：主线
- 状态：编写中（0.0 已完成）
- 阶段 tag：`phase-00-toolchain`（阶段开始时打在主线上；阶段完成后在 GitHub 上发布 Release）
- 完成标准：运行程序时输出已加载的配置项名称，并通过 DLL 自检

## 本阶段章节

| 编号 | 章节 | tag | 目标 |
|---|---|---|---|
| 0.0 | [初始化仓库与教程骨架](0.0-repo-init.md) | `chapter-0.0-repo-init` | `git init` 并添加远程仓库，建立教程骨架，写“本教程如何阅读”并打第一个 tag |
| 0.1 | [MSVC 环境与第一个 CMakeLists](0.1-msvc-cmake.md) | `chapter-0.1-msvc-cmake` | 进入 MSVC 编译环境，创建第一批源码，写第一个 CMakeLists.txt，并通过详细的构建输出说明 cl 与 link 的编译过程（含 Debug 与 Release），并学会生成 Visual Studio 解决方案、在 Visual Studio 里打开项目 |
| 0.2 | [测试（CTest）与依赖追踪](0.2-tests-ctest.md) | `chapter-0.2-tests-ctest` | 加入测试框架并用 CTest 运行，说明 Ninja 如何追踪头文件依赖 |
| 0.3 | [vcpkg 依赖管理](0.3-vcpkg-deps.md) | `chapter-0.3-vcpkg-deps` | 用 vcpkg manifest 引入 nlohmann/json、libcurl，并接入 CMake |
| 0.4 | [`.env` 配置](0.4-dotenv-config.md) | `chapter-0.4-dotenv-config` | 自己实现小型 dotenv 解析器，读取 `.env` 并打印配置，密钥遮蔽 |
| 0.5 | [DLL 集中管理与自检](0.5-dll-management.md) | `chapter-0.5-dll-management` | 让所有 DLL 与可执行文件放在同一目录，提供 `deploy` 与 `check-dlls` 目标，启动时设置 DLL 搜索目录并自检 |

## 阶段总览

待写。（本阶段要解决什么问题、学完能做什么、涉及的核心概念、与前后阶段的关系。）

## 演示

待写。（本阶段结束时可以向读者展示的“演示咒语”或运行效果。）

## 技术资料汇总

待写。（本阶段用到的库与版本、关键设计决定。详细内容记在各章“技术资料”一节。）
