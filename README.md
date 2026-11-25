# ComfyStudy · 支线 platform-01：Linux 阶段 0

这是支线分支 `quest/platform-01-linux-foundation`。它不合并回 `main`，也不打 tag、不发 Release。
起点是主线 tag `chapter-0.5-dll-management`；每一章对应一个或几个提交，提交信息以 `chapter-platform-1.N-` 开头，
用 `git log --grep="^chapter-platform-1.1"` 可以找到本章的提交。
本分支在 Debian sid（x86_64、glibc、NVIDIA 显卡）上实现和验证。

完整的教程在 <https://dreamldx.github.io/ComfyStudy/>，这一条支线的说明在
[支线 platform-01：Linux 阶段 0](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-01-linux-foundation/)。
下面按章节列出这个分支上已经实现了什么、为什么这样做，以及每一章详细讲解的链接。

## 构建与运行

目前只有环境检查，还没有 Linux 的构建文件（那是 platform-1.3）。在仓库根目录：

```bash
scripts/check-env.sh                 # 检查工具链，并做两次小的试编译
scripts/check-env.sh --no-smoke      # 只检查工具，不试编译
bash tests/scripts/test-check-env.sh # 这个脚本自己的测试
```

## 本分支实现了什么

### platform-1.1 Linux 工具链与环境检查

- 提交：`chapter-platform-1.1-toolchain-check:`
- 目的：拿到一台 Linux 机器，用一条命令知道工具链是否齐全、版本是否对，对应主线里的 `scripts/check-env.ps1`。
- 说明：
  - `scripts/check-env.sh`：检查 `g++-16`、`gcc-16`（编译器按主版本固定，只认带版本号的名字，不看 `g++` 别名）、CMake ≥ 3.25、Ninja、
    pkg-config、git、ld；报告 glibc 版本；用固定的编译器编译并运行一个用到 concepts、`std::span` 和协程的 C++20 小程序，再带
    AddressSanitizer 和 UndefinedBehaviorSanitizer 编译运行一次。缺东西时列出名字，并打印一行 `apt install`
    建议（只打印，不执行），退出码为 1。输出格式与 `check-env.ps1` 一致。
  - `tests/scripts/test-check-env.sh`：11 个用例，用假命令目录冒充各种缺失和版本不对的情况，不碰真实系统。
- 文档：[platform-1.1 Linux 工具链与环境检查](https://dreamldx.github.io/ComfyStudy/contrib/quest-platform-01-linux-foundation/platform-1.1-toolchain-check/)
