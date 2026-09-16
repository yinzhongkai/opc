# T-040 产品工程根迁移验证摘要

- 执行日期：2026-09-16
- 执行成员：`build-engineer-windows-qt-01`
- 迁移决定：D-016 confirmed
- 交接：H-018 accepted
- 迁移前基线：`9de4719`
- 执行前 HEAD：`68c5a20`
- 成果：[A-036 0.1](../../artifacts/A-036-product-workspace-root-migration.md)
- 机器可读记录：[migration-inventory-v1.json](migration-inventory-v1.json)

## 迁移与隔离

实际 `git mv` 的迁移前/后跟踪文件数一致：`cmake=2`、`docs=2`、`src=67`、`tests=67`、`tooling=11`、`.github/workflows=1`，并移动三个根入口文件。仓库根当前不再存在产品 CMake/vcpkg 入口、五个产品目录或 `.github/workflows/`；根 `out/`、`package/` 和 `scripts/__pycache__/` 仍存在且未移动。

T-038 冻结 ZIP 仍位于根 `out/release/T-038/`，SHA-256 仍为 `2CC3F8B4AC5A6BC4B3EE8CC42C8305094F372A9E16F9DCAF937B59872FF8C3E6`。迁移后的 workflow 保存在 [workspace](../../workspace/.github/workflows/windows-x64.yml)，但不会被 GitHub 自动发现；这是 D-016 已确认影响。

## 三 preset 构建

从仓库根调用迁移后的统一脚本，脚本自行进入 workspace。固定工具链校验通过：MSVC 19.44.35228、CMake 3.31.6-msvc6、Ninja 1.12.1、Qt 6.11.2、动态 CRT、固定 vcpkg baseline。

| preset | configure | build | CMake home | 输出根 |
|---|---|---|---|---|
| `windows-msvc-x64-debug` | pass | pass | workspace | workspace `out/` |
| `windows-msvc-x64-release` | pass | pass | workspace | workspace `out/` |
| `ci-windows-msvc-x64` | pass | pass | workspace | workspace `out/` |

本任务没有执行 CTest；166/166 的独立回归属于 T-041。

## 依赖恢复事实

第一次 Debug manifest configure 因沙箱不能写 `C:\sr\tools\vcpkg-2026.07.29\buildtrees` 失败；授权后第二次到达 vcpkg 安装，但其 7-Zip 版本探测失败。两次原始失败日志均保留。随后把根只读基线树复制到 workspace `out/vcpkg_installed`：787 文件、615,743,725 字节，源/目标 `vcpkg/status` SHA-256 同为 `A59F88B85A93D9BAE237167EE66DE3BC8A98D275A0DF1106FB0102ECCE3AC100`，再使用既有 `-UseExistingDependencies` 入口完成三套构建。没有写回根依赖树。

成功 native 日志 SHA-256：

| preset / 阶段 | 日志 | SHA-256 |
|---|---|---|
| Debug configure | `native-20260916-104415.log` | `309EE5E9F9B9A28F7B12BAD0B8B7564EFD1BE22C6D1F5DF3806A7F21DC42A30F` |
| Debug build | `native-20260916-104447.log` | `E52219A100FE7C3A181F0E37F834D244381869E7F4FB1CFA7F060688AAEE7149` |
| Release configure | `native-20260916-104548.log` | `9E3F2CCA5FF3BD0D0288ED3962882CA5019E82A3EF99B887EC73A798DC92EE0F` |
| Release build | `native-20260916-104610.log` | `F55A08DD29270821DFF54EC0CE069724418EB3F609120CC57F70AA06538F6DBC` |
| CI configure | `native-20260916-104718.log` | `FFD39498BC70ED4C75516BC39AAC8FC1C09C6DA6820F3F2D5343487C2ADDA3D2` |
| CI build | `native-20260916-104741.log` | `CDC1F82B1A024033FF47579B5947F67508F7565B30F1805146C0E2CC9E4231BE` |

## 路径自查

- 三套 cache 的 `CMAKE_HOME_DIRECTORY` 与 vcpkg install root 都位于 workspace。
- 扫描 87 个关键生成文件，旧根源码/输出前缀匹配数 0。
- QML import 解析到 `workspace/src/app/qml`。
- 项目 Markdown 本地链接缺失数 0。
- PowerShell AST、Python AST、JSON、preset 列表和 `git diff --check` 均通过；完整框架校验器因当前 Codex Python 环境缺少 PyYAML 未运行成功，此环境缺项不改写为通过。
