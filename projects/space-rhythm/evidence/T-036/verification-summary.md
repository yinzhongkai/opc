# T-036 Windows 发布输入与 WDAC/SAC 可复核摘要

- 负责人：`release-engineer-windows-01`
- 日期：2026-09-14
- 源审计基线：`2b675339971f9edad53c03a01a23921dd95f4189`（本轮项目台账/成果修改未包含在该提交中）
- 结论：A-032 0.1 已覆盖 T-036 计划完成条件。当前 Qt 版本、x64 ABI、SDK 来源和部署哈希正确；严格 Release 阻塞是 SAC/WDAC 对未签名 `Qt6QmlMeta.dll` 的 Code Integrity 拒绝，不是缺 DLL 或 `windeployqt` 参数错误。

## 实际检查

| 检查 | 结果 |
|---|---|
| Qt SDK 关键文件 | `Qt6Core.dll` SHA-256 `94E697C5C7B861E1F9072CB2FB251D3C807ECA053D7EE0F1B4A3BA08023AC1AC`，与 T-012/T-013 基线一致；`Qt6Gui/Qml/Quick.dll` 存在，均 `NotSigned` |
| `windeployqt` | Qt 6.11.2.0，x64，262,144 字节，SHA-256 `889DFACFB42270715D7640687CB2C5DA82FCFAAA829C2E1588BC9DE00ED8D93D`，`NotSigned` |
| `windeployqt` 策略行为 | 首次直接启动被应用控制拒绝；随后受控诊断的 SDK/构建树同哈希副本均 2/2 退出 0；期间无策略、白名单、签名或文件变更，不得以后续成功声明阻塞已稳定解除 |
| 实际 Release 部署 | `Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-release -Stage Install` 完成 CMake install 和 `windeployqt`；随后 App smoke 以 `0xC0E90002` 失败，命令整体退出 1 |
| 暂存闭包 | `bin/` 合计 136,035,733 字节；92 DLL/3 EXE/491 QML/804 PNG；81/92 DLL 按哈希回溯到 Qt SDK，11 个是 FFmpeg/KissFFT/Microsoft 图形文件；89/92 DLL 未签名 |
| 阻塞 DLL | 暂存 `Qt6QmlMeta.dll` SHA-256 `F35EF4258D2307EBD966A3107CB295CC10287E916B5680714A846811681A665C`，与 SDK 同哈希，`NotSigned` |
| Code Integrity | 三次 App 探针产生 3 组 3033/3077，均指向 `Qt6QmlMeta.dll`；状态 `0xC0E90002`，策略 `VerifiedAndReputableDesktop`，GUID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}` |
| AppLocker | EXE/DLL 与 MSI/Script 日志未出现对应拒绝 |
| 缩减 dry-run | `--skip-plugin-types qmltooling,generic` 退出 0，映射 1,423 项/68 DLL，无 qmltooling/generic，保留 `qwindows`、`qoffscreen`、`Qt6QmlMeta.dll` |

## 可复核入口

```powershell
# Qt 工具同源副本、PE、哈希、签名、ACL、ADS 和事件窗口；不改策略，不签名。
./tooling/windows/Invoke-WdacBinaryDiagnostic.ps1 `
  -BuildRoots @('QtInstall=C:\sr\q\qt6112\bin','QtBuild=C:\sr\b\qt6112\qtbase\bin') `
  -ExecutableName windeployqt.exe -LaunchArguments '--version' `
  -EvidenceRoot ./out/evidence/T-036/wdac-windeployqt-current -RepeatCount 2

# 当前 Release 安装暂存及严格 App smoke。
./tooling/windows/Invoke-ProjectBuild.ps1 `
  -Preset windows-msvc-x64-release -Stage Install `
  -EvidenceRoot ./out/evidence/T-036/release-install-current

# 暂存 App 与加载 DLL 的 Code Integrity 窗口证据。
./tooling/windows/Invoke-WdacBinaryDiagnostic.ps1 `
  -BuildRoots @('ReleaseStage=.\out\install\windows-msvc-x64-release\bin') `
  -ExecutableName space-rhythm.exe -LaunchArguments '--smoke' `
  -EvidenceRoot ./out/evidence/T-036/wdac-release-stage-current -RepeatCount 3
```

完整机器证据位于被忽略的 `out/evidence/T-036/`；本摘要只固化可提交的命令、关键哈希、结果与安全边界。本轮未关闭或修改 WDAC/SAC，未改白名单，未签名，未使用 fallback，未访问任何签名凭据。
