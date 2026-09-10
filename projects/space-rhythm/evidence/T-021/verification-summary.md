# T-021 Windows headless 验证摘要

- 证据版本：2
- 执行日期：2026-09-10
- 执行人：tester-cpp-qt-01
- 原始证据：`out/evidence/T-021/<run-id>/`（按 A-016 忽略，不纳入 Git）
- 机器可读索引：[runs-v1.json](runs-v1.json)（首次执行）、[runs-v2.json](runs-v2.json)（基于 `40b1734` 的独立复测）
- 基础种子：`0x5350414345524859`（十进制 `6003370060466505817`）
- 固定环境：Windows x64、MSVC 19.44、Qt 6.11.2、CMake 3.31.6-msvc6、Ninja 1.12.1、vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`
- 范围排除：T-022、`package/`、`scripts/__pycache__/`

## 首次执行结果

| preset | 构建 | CTest 总数 | pass | fail | blocked | skip | 结论 |
|---|---:|---:|---:|---:|---:|---:|---|
| `windows-msvc-x64-debug` | pass | 61 | 60 | 1 | 0 | 0 | `fail`：颜色范围契约偏差 |
| `windows-msvc-x64-release` | pass | 61 | 34 | 1 | 26 | 0 | `fail + blocked`：同一契约偏差；原有 core GoogleTest 可执行文件被 WDAC 阻止启动 |
| `ci-windows-msvc-x64` | pass | 61 | 60 | 1 | 0 | 0 | `fail`：颜色范围契约偏差 |

三套 preset 的 11 项 tester 独立 GoogleTest 中，时间/溢出、锁定/事务/修订、IPC 状态机/帧协议、schema 迁移、原子保存、缓存损坏、PTS/CFR/VFR 的 10 项均 pass；`GM-ROT-SAR-001` 的颜色范围 1 项均 fail。12 个样例的生成、许可证、配方 hash、实际媒体 hash、期望字段和容差策略审计均 pass。现有媒体 GoogleTest 16 项、Qt Test、Qt Quick Test、应用/worker headless smoke 和 x86/arm64 构建保护均已纳入统一 `t021` 入口并在三套 preset 中执行；原有 core GoogleTest 26 项在 Debug/CI pass，在 Release 为环境 `blocked`。

## `40b1734` 独立复测

独立复测保留 `GM-ROT-SAR-001` 的 `limited` oracle 原样，并新增只经公开 `MediaSource` API 观察的 `full` 与 `unknown` 检查；未引用媒体内部映射函数，未修改媒体实现。运行时无颜色元数据 BMP 是测试代码内固定的 58 字节项目自有输入，SHA-256 为 `74aae4a8fead568755858e56ace08c57c42e6ff168120e3ab86872426b8c4018`。

| preset | 构建起点 | CTest 总数 | pass | fail | blocked | skip | 结论 |
|---|---|---:|---:|---:|---:|---:|---|
| `windows-msvc-x64-debug` | 不存在的 build 目录 | 63 | 63 | 0 | 0 | 0 | `pass` |
| `ci-windows-msvc-x64`（RelWithDebInfo） | 经路径校验后清理的 build 目录 | 63 | 63 | 0 | 0 | 0 | `pass` |

| 公开值 | 公开观察点 | Debug | CI/RelWithDebInfo |
|---|---|---|---|
| `limited` | `MediaSource::info()` 对 GM-ROT-SAR-001 的源颜色范围 | pass | pass |
| `full` | `MediaSource::thumbnail()` 对同一样例输出的 full-range BGRA 描述 | pass | pass |
| `unknown` | `MediaSource::info()` 对无颜色元数据 BMP 的源颜色范围 | pass | pass |

两份 JUnit 均明确记录 63 tests、0 failures、0 skipped；环境、JUnit、CTest、发现、标签、LastTest 及构建日志 SHA-256 见 `runs-v2.json`。因此 `T021-DEFECT-001` 按用户指定复测条件标记为 `resolved`。

## 缺陷与阻断

1. `T021-DEFECT-001`：`resolved`。A-014 0.3 的封闭公开映射 `limited/full/unknown` 及 GM-ROT-SAR-001 已在 Debug 和 CI/RelWithDebInfo 独立复测通过；首次执行的失败记录保留，不回写成通过。
2. `T021-ENV-001`：Release 的 `space_rhythm_core_tests.exe` 被主机 WDAC 阻止启动，CTest 将 26 项记为 `BAD_COMMAND`。独立契约、媒体、Qt/QML 和 smoke 可执行文件仍能启动。首次 PRE_TEST 发现失败及同条件重试均保留，最终改用静态 GoogleTest 注册，使未受影响测试继续执行；被阻断项没有改写为 skip/pass。下一行动人为 build-engineer-windows-qt-01 或主机策略管理员。
3. `T021-HARNESS-002`：本轮首次带 `-Clean` 调用时，现有两阶段入口在 Build 阶段再次删除 build 目录，测试未运行；随后 Debug 从不存在的 build 目录完成终态运行。另一次复用 CI 目录的 63/63 运行因复制到旧 `LastTestsFailed.log` 被排除出终态集合，最终 CI 经显式路径校验和清理后从零构建并通过。两次诊断记录均保留在 `runs-v2.json`，未计作产品失败或通过。

## 判定边界

- 本摘要的 `pass/fail/blocked` 只描述已执行的 T-021 功能契约和入口行为；标签 `gate-g0`～`gate-g4` 是证据检索标签，不代表门槛通过。
- G0～G4 仍为 `not-evaluated`：虽本轮契约复测通过，Release 仍有 26 项 blocked，且各门槛还依赖 T-019、T-022、UI/算法/音频/渲染及产品验收输入。
- CTest 运行时间只是诊断观测，不是性能测试；没有确认的性能、效果、硬件或 Windows 兼容阈值，因此这些结论保持 `not-evaluated`，不写成 pass。
- WDAC 回退仅用于已有 Qt/QML smoke 的显式受管主机路径；没有替代 GoogleTest、媒体测试或产品契约执行。
