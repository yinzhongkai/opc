# Windows x64 CMake/Ninja 工程与 CI 骨架

- 项目：space-rhythm
- 成果 ID：A-013
- 负责人：build-engineer-windows-qt-01
- 关联任务：T-013
- 版本：0.3
- 更新日期：2026-09-16
- 状态：draft
- 适用范围：Windows x64 的应用、Worker、纯 C++ 核心库、媒体适配库和测试工程骨架；CMake Presets/Ninja、非 Qt vcpkg manifest/baseline、GoogleTest/CTest、Qt Test/Qt Quick Test、QML/进程冒烟、安装闭包和 Windows CI。只复用 T-012 的 Qt 6.11.2 SDK，不重建 Qt，不实现业务逻辑，不创建安装器、签名或发布批准。
- 来源及输入版本：用户于 2026-09-09 确认 D-003 并明确要求启动 T-013；2026-09-10 明确要求诊断 T021-ENV-001；T-011/T-012 completed；D-003、D-006、D-007、D-008 confirmed；[A-005 0.4](A-005-mvp-technology-stack-proposal.md)、[A-006 0.1](A-006-domain-work-packages.md)、[A-007 0.1](A-007-four-engineer-execution-plan.md)、[A-008 0.3](A-008-windows-build-environment-audit-and-execution-plan.md)、[A-009 0.1](A-009-windows-qt-6.11.2-source-sdk-build.md)、[A-012 0.1](A-012-core-domain-contract-0x.md)、[A-017 0.2](A-017-windows-headless-contract-test-entry-and-evidence.md)。
- 批准依据：尚无。D-003 与 D-006～D-008 是本成果的已确认输入，不等同于批准本成果或发布。
- 版本记录：2026-09-09，0.1，完成工程目标、固定依赖、自动测试、部署检查、CI 与三套预设验证；2026-09-10，0.2，增加 T021-ENV-001 的 WDAC/SAC 只读取证、Debug/Release/CI 对照、原路径与新路径重建验证、严格 Release headless 结果及最小策略需求，不改变 0.1 的历史验证记录；2026-09-16，0.3，按 D-016/T-040 把有效工程入口更新为 `projects/space-rhythm/workspace/`，历史验证数字不变，迁移证据由 A-036 0.1 承接。

## 1. 结论

T-013 已达到完成条件。仓库现有一套可由 Windows x64/MSVC 2022/Ninja 消费的最小工程：应用、Worker、纯 C++ 核心、媒体适配与测试边界可独立链接；Qt 6.11.2 只能从既有 SDK 解析；默认非 Qt 依赖由固定 vcpkg baseline 安装；GoogleTest、Qt Test、Qt Quick Test、应用 QML、Worker 进程和非 x64 拒绝检查统一进入 CTest；CI 执行配置、编译、测试、安装、部署、运行和 PE/依赖核对。

三套 T-013 历史最终验证均完成 67/67 编译与 7/7 CTest。详细矩阵、日志哈希和当时的本机 WDAC 限制见 [T-013 可复核摘要](../evidence/T-013/verification-summary.md)。后续 T021-ENV-001 诊断证明当前受管主机的严格 Release 全门禁仍不稳定，见 [专项诊断摘要](../evidence/T-013/t021-env-001-verification-summary.md)；该后续结果不回写或美化 T-021 的 blocked 记录。

## 2. 目标拓扑与责任边界

| CMake 目标 | 类型 | 当前依赖 | 本任务内容 | 后续所有者入口 |
|---|---|---|---|---|
| `SpaceRhythm::Core` | C++ 静态库 | 无 Qt Quick、无 FFmpeg | 链接锚点及 x64/动态 CRT 构建常量 | T-015/T-016 加入核心与系统实现 |
| `SpaceRhythm::MediaAdapter` | C++ 静态库 | Core | 独立链接边界，无解码实现 | T-017/T-018 加入媒体契约和适配 |
| `space_rhythm_worker` | 控制台进程 | Core、MediaAdapter、Qt Core/Network | 事件循环和 `--smoke` | T-016 加入版本化 IPC/作业骨架 |
| `space_rhythm_app` | Qt Quick 应用 | Core、MediaAdapter、Qt Core/Gui/Qml/Quick | 最小 QML module 和 `--smoke` | T-025/T-026 加入 UI 壳层与集成 |
| `space_rhythm_core_tests` | GoogleTest | Core、MediaAdapter | 链接与构建契约验证 | T-021 追加独立核心断言 |
| Qt/QML smoke targets | Qt Test/QuickTest | 固定 Qt SDK | 版本、x64、QML engine 与进程门禁 | 各实现任务扩充覆盖 |

空壳源码不包含项目模型、事件业务、IPC 协议、FFmpeg 调用、OpenCV/KissFFT 算法或真实页面交互。目录只建立依赖方向，避免 UI、媒体和算法责任提前耦合进核心库。

## 3. 构建合同

- 生成器固定为 Ninja；CMake 低于 3.31、非 Windows、非 MSVC、非 64 位环境、非 MSVC 19.44 或目标架构不是 x64 时配置失败。
- 语言基线为 C17/C++20；MSVC 动态 CRT 固定为 `/MD`、`/MDd`。
- 项目手写目标统一使用 `/W4 /permissive- /Zc:__cplusplus /utf-8`，并默认 `/WX`。Qt 自动生成的 QML cache 源码保留 `/W4`，不把其上游警告升级为本项目错误。
- Qt 查找使用 `find_package(Qt6 6.11.2 EXACT ...)`，并核对实际 package root 与 `SPACE_RHYTHM_QT_ROOT` 一致；入口脚本再核对 T-012 SDK 的关键哈希。
- 构建、vcpkg 安装、安装暂存和新证据均进入产品 workspace 的 `out/`，不污染源码目录；迁移前根 `out/` 只读保留历史证据和冻结包。

`CMakePresets.json` 提供 `windows-msvc-x64-debug`、`windows-msvc-x64-release`、`ci-windows-msvc-x64` 三个 configure/build/test preset，以及 `ci-windows-msvc-x64-workflow`。权威 PowerShell 入口负责导入 Build Tools x64 环境、调用 preset、部署、运行检查并生成日志和安装清单。

## 4. 非 Qt 依赖策略

`vcpkg.json` 固定 builtin baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`，配合仓库内 `x64-windows-space-rhythm` 动态链接 triplet。默认只安装 GoogleTest 1.17.0#3；Qt 不进入 vcpkg，也不会被下载或重新构建。

FFmpeg、OpenCV 与 KissFFT 只作为可选 manifest feature 登记。FFmpeg 关闭默认 feature 并排除 GPL/nonfree 路径；OpenCV 关闭 DNN、Qt 和其 FFmpeg 集成；这些 feature 只有对应实现负责人接入并补齐许可、ABI 与功能验证后才能启用。端口选择是可复现构建输入，不是发布许可结论。

## 5. 测试与验证门禁

CTest 当前包含七项：GoogleTest 的核心/媒体适配链接，生成的 Qt Test 目标，生成的 Qt Quick Test 目标，应用 QML，Worker 进程，以及 x86/ARM64 配置拒绝。测试预设统一启用 `QT_QPA_PLATFORM=offscreen`，不要求桌面交互。

安装阶段用 `windeployqt` 生成隔离的运行闭包，再执行已安装 App/Worker 冒烟，随后用 `dumpbin /headers` 和 `/dependents` 核对 PE x64、Qt 配置和动态 CRT，最后生成逐文件 SHA-256 CSV。Debug、Release、CI 三套安装闭包均已实际产生并核对。

本机 `VerifiedAndReputableDesktop`（Smart App Control/WDAC）会按生成文件的具体内容、哈希及信任状态拒绝部分未签名二进制；同一路径 clean rebuild 后可能改变判定，而无修改重复启动对已拒绝哈希不能稳定恢复。入口保留默认关闭的显式开发机恢复开关，但该开关只用于早期骨架/QML 开发诊断，不得用于 T-021 Release 验收。正式 CI 没有启用任何回退，生成程序不能真实运行即失败。

## 6. CI 合同

workspace 内归档的 `.github/workflows/windows-x64.yml` 使用带 `self-hosted, Windows, X64, space-rhythm-qt6112` 标签的 runner。runner 必须预置与 T-012 一致的 `C:\sr\q\qt6112` 和 MSVC Build Tools；工作流只克隆 vcpkg tag `2026.07.29`、核对精确 baseline 并执行正常的 `-Stage All -Clean`。按 D-016，仓库根没有 workflow 入口，GitHub 当前不会自动发现该文件。

CI 不构建 Qt、不启用本机依赖复用或 WDAC 回退。无论成功失败，安装闭包与 `out/evidence/T-013/ci-windows-msvc-x64` 都作为 14 天构建工件上传，便于复核配置、测试、PE 和逐文件哈希。

## 7. 验证结果

| preset | 构建 | CTest | PE/运行库 | 安装与清单 |
|---|---|---|---|---|
| Debug | 67/67 | 7/7 | App/Worker 均为 x64；Debug Qt + Debug CRT | 66 文件，143,005,992 字节 |
| Release | 67/67 | 7/7 | App/Worker 均为 x64；Release Qt + Release CRT | 57 文件，86,681,040 字节 |
| CI / RelWithDebInfo | 67/67 | 7/7 | App/Worker 均为 x64；Release Qt + Release CRT | 57 文件，86,804,608 字节 |

本表保存 0.1 阶段的历史骨架验收，不能解释为 2026-09-10 的 T-021 Release 全门禁已通过。专项严格运行实际为 53/63；其中原先 blocked 的 26 个 core 用例重建后均真实通过，另 10 项仍受主机策略影响并保持 fail/blocked。

固定 Qt SDK 的 `qmltestrunner` 与 `qmllint` 也独立退出 0；CMake 能列出全部 configure/build/test/workflow presets。三套原生日志及安装清单的 SHA-256 已写入可复核摘要。

## 8. 使用入口

从普通 PowerShell 运行以下命令；脚本会自行导入 x64 Build Tools 环境：

```powershell
Set-Location projects/space-rhythm/workspace
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage All -Clean
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-release -Stage All -Clean
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset ci-windows-msvc-x64 -Stage All -Clean
```

vcpkg 首次准备、直接使用 preset、恢复开关和 CI runner 前置条件见 workspace 的 `docs/windows-build.md`。T-040 迁移映射、三 preset 构建与隔离审计见 [A-036 0.1](A-036-product-workspace-root-migration.md)。

## 9. 边界与后续

- 本成果未修改、读取或执行 `package/`，也不把已有安装包纳入新工程。
- 最低 Windows 版本仍未确认；这不阻止当前开发/CI 骨架，但必须在后续兼容矩阵与发布 preset 中收敛。
- 安装闭包只是工程验证暂存，不是正式安装器或发布包；许可证文本、第三方 notices、SBOM 汇总、VC Runtime 分发、签名主体/证书和发布批准仍归后续任务。
- H-002 覆盖的 T-011～T-013 已由本负责人完成，但交接保持 accepted，等待发起人 architect-01 核对并关闭。

## 10. 自查

- 已核对所有新增目录和目标均属于 T-013；没有业务 UI、媒体、算法或 IPC 实现。
- 已核对 Qt 6.11.2 来自现有 SDK 且关键哈希与 T-012 一致，没有 Qt 下载或重建步骤。
- 已核对 CI 不使用本机恢复开关，vcpkg baseline 和 runner 合同可见且固定。
- 已完成 JSON、PowerShell 语法、preset 列表、三套 configure/build/test/install/deploy/smoke、x64/CRT 和逐文件哈希验证。
- 成果保持 draft；任务完成不自动批准成果或发布。

## 11. T021-ENV-001 后续诊断

workspace 内的 `tooling/windows/Invoke-WdacBinaryDiagnostic.ps1` 以只读方式保存实际路径、SHA-256、PE/x64、CRT/导入表、ACL、Zone.Identifier、签名、CMake/Ninja 链接命令、重复启动结果、Code Integrity/AppLocker 事件和策略只读结果。旧 Release core 哈希 `06ea...f02e` 连续返回 Win32 4551，Code Integrity 3077 给出 `0xC0E90002` 和 Policy GUID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`；Debug 与 CI 同类程序可启动。

原路径 clean rebuild、全新目录 rebuild 均使 core 程序实际启动；当前 Release core 以 GoogleTest XML 证明 26/26。严格 Release headless 全套仍为 53/63，未使用 fallback，故 T021-ENV-001 仍需主机策略管理员提供组织管理的非生产开发签名/签名服务，或在非用户可写的专用构建根上设置最小补充策略。不得使用全局用户目录白名单、易变 hash 白名单或测试入口降级。完整事实、事件摘录和复核命令见 [专项诊断摘要](../evidence/T-013/t021-env-001-verification-summary.md)。
