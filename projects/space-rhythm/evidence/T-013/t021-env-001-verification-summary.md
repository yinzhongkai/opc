# T021-ENV-001 Windows Release 应用控制诊断摘要

- 负责人：build-engineer-windows-qt-01
- 采集日期：2026-09-10（事件时间统一为 UTC）
- 源提交：`f6f8270a731ddf7c9df9ceb1fe033ce9c4049380`
- 关联任务：T-013 构建基线的 T021-ENV-001 后续诊断；T-021 的测试状态和首次 blocked 记录不在本成果中改写
- 完整机器证据根：`out/evidence/T021-ENV-001`
- 结论：原 Release `space_rhythm_core_tests.exe` 的确被主机 `VerifiedAndReputableDesktop`（Smart App Control/WDAC）按具体文件哈希拒绝。原路径 clean rebuild 和全新输出目录 rebuild 后，该核心测试程序均可启动；当前 Release 核心测试已真实执行 26/26。此结果排除固定路径、ACL、Zone.Identifier、PE 架构、CRT 或测试入口为原阻断根因，但不能提供稳定的整套 Release 放行：同轮重新链接的其他未签名测试程序仍触发相同策略，Release headless 全套保持 53/63，不能记为 pass。

## 1. 安全边界

本轮没有关闭或绕过 WDAC，没有修改系统安全策略、全局白名单、ACL 或 ADS，没有签名任何文件，也没有使用 `-AllowWdacFallback` 或生产签名证书。只在仓库 `out/build` 内执行一次 clean rebuild，在 `out/diagnostics/T021-ENV-001` 建立一次全新 Release 输出，并对未修改文件做重复启动。`package/` 与 `scripts/__pycache__/` 均未读取、修改或暂存。

## 2. 首次复现与三配置对照

诊断入口为 `tooling/windows/Invoke-WdacBinaryDiagnostic.ps1`。首轮证据目录为 `out/evidence/T021-ENV-001/20260910T075352Z-f6f8270a731d`；每个目标均以 `--gtest_list_tests` 无修改启动三次。

| 配置 | 实际生成路径 | SHA-256 / 字节 | CMake 与链接差异 | 启动行为 |
|---|---|---|---|---|
| Debug | `out/build/windows-msvc-x64-debug/space_rhythm_core_tests.exe` | `34cf058a6282b2228f74c03064af11e347e20ef882d9a1d97a9f9b8075e86b3e` / 3,215,872 | Debug，`/Ob0 /Od /RTC1`，`/debug /INCREMENTAL`，动态 `/MDd` | 3/3 启动，均退出 0，各输出 1,514 字节测试清单 |
| Release | `out/build/windows-msvc-x64-release/space_rhythm_core_tests.exe` | `06ea5bd9ac55a8c16892c104ffd039e968369babb116c7c67e824707e97bf02e` / 747,008 | Release，`/O2 /Ob2 /DNDEBUG`，`/INCREMENTAL:NO`，动态 `/MD` | 3/3 在 `CreateProcess` 前失败；无进程退出码，Win32 4551，消息为“应用程序控制策略已阻止此文件” |
| CI | `out/build/ci-windows-msvc-x64/space_rhythm_core_tests.exe` | `182876c3c97c578ff1234227c25a5ac37b42c2775fc2c21b4b1ac531edbb2e15` / 1,937,408 | RelWithDebInfo，`/O2 /Ob1 /DNDEBUG`，`/debug /INCREMENTAL`，动态 `/MD` | 3/3 启动，均退出 0，各输出 1,514 字节测试清单 |

三者均为 `0x8664` x64、PE32+、console subsystem，均 `NotSigned`，仅有默认 `:$DATA` 流且不存在 `Zone.Identifier`。文件 owner 均为 `tiger\CodexSandboxOffline`，ACL 的 SDDL 相同：

```text
O:S-1-5-21-2846392102-3215430013-2756637413-1004G:S-1-5-21-2846392102-3215430013-2756637413-513D:AI(A;ID;0x1301bf;;;S-1-5-21-3189121467-1926994209-4268001091-2337047468)(A;ID;0x1301bf;;;S-1-5-21-2846392102-3215430013-2756637413-1003)(A;ID;0x1301bf;;;S-1-5-21-2251540099-4046515987-442712429-898684447)(A;ID;FA;;;SY)(A;ID;FA;;;BA)(A;ID;FA;;;S-1-5-21-2846392102-3215430013-2756637413-1001)
```

Release 与 CI 的直接导入表相同：`gtest_main.dll`、`gtest.dll`、`Qt6Core.dll`、`Qt6Network.dll`、`MSVCP140.dll`、`VCRUNTIME140.dll`、`VCRUNTIME140_1.dll`、UCRT API set 和 `KERNEL32.dll`。Debug 只按预期切换为 `Qt6Cored.dll`、`Qt6Networkd.dll`、`MSVCP140D.dll`、`VCRUNTIME140D.dll`、`VCRUNTIME140_1D.dll`、`ucrtbased.dll`。因此 Release 与可启动 CI 之间不存在架构、CRT、签名或直接 DLL 类型差异；主要构建差异是优化/调试信息、增量链接和由此产生的二进制内容。

## 3. Windows 事件

首轮复现产生同一 Activity 的 Code Integrity 事件 3033/3077/3118；可提交的字段摘录见 [事件 JSON](t021-env-001-code-integrity-events.json)，完整 XML 位于首轮机器证据的 `windows-events.json`。

- Event 3033，Record 6671，`2026-09-10T07:45:14.3744337Z`：RequestedPolicy 2、ValidatedPolicy 1、status `3236495362`。
- Event 3077，Record 6673，`2026-09-10T07:45:14.3769944Z`：status `0xC0E90002`，flat SHA-256 与文件 `06ea...f02e` 完全一致；PolicyName `VerifiedAndReputableDesktop`，Policy GUID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`，Requested/Validated Signing Level 为 2/1。
- Event 3118，Record 6675，`2026-09-10T07:45:14.3770159Z`：`Smart App Control Block Details`；Defender 未报告 threat name，`IsUnfriendlyFile=false`，但缓存信任结果不足且没有完成云端调用。
- AppLocker `EXE and DLL` 与 `MSI and Script` 对目标文件均为 0 条，因此本次拒绝来自 Code Integrity/SAC，不是 AppLocker。
- 普通构建账号读取 `CiTool -lp -json` 与 DeviceGuard CIM 均被拒绝，返回 `0x80070005`；未为读取策略清单提升权限，事件中的策略标识已足够复核实际判定。

历史 T-021 的 CTest discovery/run 对旧哈希返回 exit 8 和 `BAD_COMMAND`；进程本身没有启动，因此不存在可伪造为 0 的被测进程退出码。

## 4. 内容、路径与瞬态实验

| 实验 | 生成路径 | SHA-256 | 无修改重复启动 | 结论 |
|---|---|---|---|---|
| 原文件 | 原 Release 路径 | `06ea...f02e` | 3/3 Win32 4551 | 确定性拒绝，重复启动不恢复 |
| 原路径 clean rebuild | 原 Release 路径 | `f1367f5363a96f0fabd2a8ab99d428f59b5981d68467aad337b0a949fbbc8efa` | 3/3 exit 0 | 固定路径规则不是根因 |
| 全新输出目录 rebuild | `out/diagnostics/T021-ENV-001/release-fresh` | `94933ec23988ff6b6b018f7cc502bcf3484719eaa2e155fa7321da2328168d55` | 3/3 exit 0 | 新路径不触发固定目录拒绝 |
| 当前标准 Release 核心测试 | 原 Release 路径 | `5aac18ee9b0144d1bc6d2d180d31483c7e19d818f30759b1e19a2629fef86c66` | 实际运行全部用例 | 26/26、exit 0、stdout 5,734 字节、XML 已生成 |

两次重建的大小、PE/CRT/导入和链接开关一致；哈希因重新链接而变化。结论只能收敛为主机 SAC/WDAC 对具体未签名内容及其信任/声誉缓存的判定，不支持把问题归因于 CMake 目标、测试发现逻辑、固定路径、ACL 或 Zone.Identifier。

## 5. Release headless 结果

未启用 fallback 的标准入口运行 ID 为 `20260910T075941Z-f6f8270a731d-windows-msvc-x64-release-01`，JUnit SHA-256 为 `4FDBD01A7174D26C07D72A2A68CC4DF6C4CAE44D70E16AEAB7B47A0631F10A76`：63 项中 53 pass、10 fail/blocked，CTest exit 8。

- `space_rhythm_core_tests.exe` 对应 26 项全部实际通过；随后独立运行再次得到 26/26、0 failure、0 error、0 disabled，GoogleTest XML SHA-256 为 `6A643CDC6EFDA8DA7DB325A5CA54ED99E35719DDBDEA6C0E7520987A7BDDA4E1`。
- 同轮 `space_rhythm_contract_tests.exe` 的 8 项为 `BAD_COMMAND`；其 SHA-256 `72d779...286e` 连续三次 Win32 4551，而全新目录同源目标 `743857...bcf3` 连续三次可启动。
- `qt.core_smoke` 与 `qml.quick_smoke` 两项未产生要求的测试输出，入口按约定判失败；Code Integrity 对当前 Qt smoke 哈希 `ed0825...2704` 记录了同一 `0xC0E90002`。退出 0 但无输出也未被改写为 pass。

因此“旧 Release core 的 26 项”已有真实通过证据，但 T021-ENV-001 作为受管主机 Release 全门禁问题仍保持 blocked，不能通过在测试入口放宽判定来关闭。

## 6. 最小策略需求与安全处置建议

此问题需要主机策略管理员提供可持续的开发代码信任路线，优先级如下：

1. 依据 Policy GUID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}` 和事件 3033/3077/3118 核对 `VerifiedAndReputableDesktop` 的实际部署来源、SAC 状态及生效补充策略；先解释为什么同一 MSVC/Qt/vcpkg 输入生成的不同哈希得到不同判定。
2. 首选组织管理的、非生产用途开发/CI 代码签名身份和独立签名服务，并用最小 publisher/signer 规则只信任该开发身份；凭据不得落到普通开发仓库或当前构建账号。
3. 若必须使用路径规则，应由管理员建立 ACL 受控、非用户可写的专用构建输出根，并只为该根建立补充策略。不要对白名单加入整个 `Desktop`、用户 profile、仓库根或任意 unsigned executable。
4. 不建议使用文件哈希白名单：MSVC 每次重新链接都会产生新哈希，无法形成可维护且安全的构建门禁。也不应把当前显式 Qt fallback 用于 T-021 Release 验收。
5. 管理员变更后，必须在无 `-AllowWdacFallback` 的环境从 clean output 重新配置、编译并运行 `Invoke-HeadlessTests.ps1 -Preset windows-msvc-x64-release`；只有 63/63 都产生真实测试输出且 exit 0，才可另行解除 blocked。

## 7. 复核入口

```powershell
# 只读采集三套 core 测试程序；默认每个启动三次并读取近 72 小时 CI/AppLocker 事件。
./tooling/windows/Invoke-WdacBinaryDiagnostic.ps1

# 采集任意同目录测试程序，退出 0 且无输出会单独标为 started-exit-zero-no-output。
./tooling/windows/Invoke-WdacBinaryDiagnostic.ps1 `
  -ExecutableName space_rhythm_contract_tests.exe

# 策略管理员处理后的严格 Release 复验；不得传 -AllowWdacFallback。
./tooling/windows/Invoke-HeadlessTests.ps1 `
  -Preset windows-msvc-x64-release `
  -Clean
```

诊断入口的 `summary.json`、每个二进制的 `binary.json`、`dumpbin` headers/dependents/loadconfig、Ninja 实际链接命令、每次 stdout/stderr、Windows 事件、策略只读结果及 SHA-256 manifest 均写入被忽略的 `out/evidence/T021-ENV-001`。
