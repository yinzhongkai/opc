# 产品工程根迁入项目 workspace

- 项目：space-rhythm
- 成果 ID：A-036
- 负责人：build-engineer-windows-qt-01
- 关联任务：T-040、H-018
- 版本：0.1
- 更新日期：2026-09-16
- 状态：draft
- 适用范围：依据 D-016 将 Windows x64 产品工程完整迁入 `projects/space-rhythm/workspace/`，修订构建、测试、发布、T-029 工具、文档和归档 CI 的有效路径，并完成三套 preset 的 clean configure/build 验证。本成果不执行 T-041 测试回归或 T-042 交付重建。
- 来源及输入版本：用户直接要求接收 H-018 并立即执行 T-040；D-016 confirmed；迁移前提交 `9de4719`；执行前 HEAD `68c5a20`；A-013 0.2、A-034 0.1、A-035 0.1。
- 批准依据：尚无。D-016 是已确认迁移决定，不等同于批准本成果或发布。
- 版本记录：2026-09-16，0.1，完成保留 Git 历史的目录迁移、路径修订、三 preset 构建和隔离审计。

## 1. 结果

`projects/space-rhythm/workspace/` 已成为唯一产品工程根。实际使用 `git mv` 迁移了根 `CMakeLists.txt`、`CMakePresets.json`、`vcpkg.json`、`cmake/`、`src/`、`tests/`、`tooling/`、`docs/` 和 `.github/workflows/`；迁移前后跟踪文件数一致：`cmake=2`、`docs=2`、`src=67`、`tests=67`、`tooling=11`、workflow `=1`。

仓库根已不存在上述产品入口和目录。根 `out/`、未跟踪 `package/`、`scripts/__pycache__/` 均保留原位；T-038 冻结 ZIP 的 SHA-256 仍为 `2CC3F8B4AC5A6BC4B3EE8CC42C8305094F372A9E16F9DCAF937B59872FF8C3E6`。

## 2. 有效入口

- CMake/preset/vcpkg 入口位于 [workspace](../workspace/)；`${sourceDir}/out` 因源码根迁移而自然落到 workspace 内。
- [Invoke-ProjectBuild.ps1](../workspace/tooling/windows/Invoke-ProjectBuild.ps1) 主动切换到产品源码根解析 preset，因此从仓库根或 workspace 调用均不会误用根目录。
- [Invoke-UnsignedRelease.ps1](../workspace/tooling/windows/Invoke-UnsignedRelease.ps1) 分离 Git 仓库根与产品源码根，并按 build tree 直接构建；[Test-PersonalDelivery.ps1](../workspace/tests/release/Test-PersonalDelivery.ps1) 的 source-delta 检查允许 D-016 的纯 `R100` 路径迁移，同时仍只允许既有 Worker stdout flush 这一项内容差异。
- T-029 PowerShell/Python 工具分别区分 repository、project 和 source root；媒体、临时目录与运行输出进入 workspace `out/`，项目成果和版本化证据继续进入其上级项目目录。
- QML 相对 import `tests/qml/t025/../../../src/app/qml` 在迁移后仍解析到同一 workspace 内；本地 Markdown 链接审计为 0 个缺失目标。
- 归档 workflow 位于 [workspace/.github/workflows/windows-x64.yml](../workspace/.github/workflows/windows-x64.yml)，运行目录和上传路径已指向 workspace。按 D-016，仓库根没有 workflow 入口，GitHub Actions 不会自动发现该文件，push/PR/manual 自动构建暂时停用。

## 3. 构建验证

固定输入为 MSVC `19.44.35228`、CMake `3.31.6-msvc6`、Ninja `1.12.1`、Qt `6.11.2` SDK `C:\sr\q\qt6112`、vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`。首次 manifest 安装先因沙箱禁止写固定 vcpkg `buildtrees` 失败；获准重跑后又因主机上的 vcpkg 7-Zip 无法返回版本信息失败。这两次均保留为失败证据，没有改写成通过。

随后只读核对并复制迁移前 T-022/T-038 已验证的同 baseline 安装树到 workspace：787 文件、615,743,725 字节，`vcpkg/status` SHA-256 为 `A59F88B85A93D9BAE237167EE66DE3BC8A98D275A0DF1106FB0102ECCE3AC100`。使用工程既有 `-UseExistingDependencies` 恢复入口完成验证；根依赖树未被移动或写入。

| preset | 构建类型 | clean configure | build | App / Worker |
|---|---|---|---|---|
| `windows-msvc-x64-debug` | Debug | pass | pass | 均存在 |
| `windows-msvc-x64-release` | Release | pass | pass | 均存在 |
| `ci-windows-msvc-x64` | RelWithDebInfo | pass | pass | 均存在 |

三份 `CMakeCache.txt` 的 `CMAKE_HOME_DIRECTORY` 均为新 workspace，`VCPKG_INSTALLED_DIR` 均为 workspace `out/vcpkg_installed`。对三套 build tree 的 87 个 `CMakeCache.txt`、`compile_commands.json`、`build.ninja`、`cmake_install.cmake` 和 `CTestTestfile.cmake` 扫描，旧根产品源码/输出路径匹配数为 0。

完整命令、失败保留、输出哈希和路径审计见 [T-040 验证摘要](../evidence/T-040/verification-summary.md)与[机器可读清单](../evidence/T-040/migration-inventory-v1.json)。原始构建日志位于被忽略的 `projects/space-rhythm/workspace/out/evidence/T-040/`。

## 4. 边界与移交

- 没有修改产品行为，没有重新构建 Qt，没有关闭或绕过 WDAC/SAC，没有创建根 workflow 兼容入口。
- 本轮只证明三套 preset 可在新根配置和编译；没有执行 CTest，不替代 tester-cpp-qt-01 的 T-041 独立回归。
- 没有生成或验证新交付包；T-038 根冻结包只读保留，T-042 仍由 release-engineer-windows-01 在 T-041 完成后执行。
- T-040 完成后，下一行动人为 tester-cpp-qt-01，输入为本成果、T-040 证据和任务提交。
