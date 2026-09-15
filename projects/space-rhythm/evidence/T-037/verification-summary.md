# T-037 unsigned 流水线验证摘要

- 执行日期：2026-09-15
- 执行成员：`release-engineer-windows-01`
- 成果：A-033 0.1
- 源 HEAD：`41ce475c620fbfbb2e299130ea9e18477971ff47`
- 结论：unsigned 受控部署、依赖闭包、SBOM/许可证、确定性 ZIP 和显式路径安装事务已实现并实际通过；输出始终为非候选工程包，T-037 的安装器选型与签名部分仍未完成。

## 实际组包结果

| 项 | 结果 |
|---|---|
| 包名 | `space-rhythm-0.1.0-dev-unsigned.zip` |
| ZIP 大小 / SHA-256 | `44,725,377` / `9591F8B0E3B1CD28789A785D91077D298EE10011B4A7D3EBEA810DAC9781449B` |
| 输入状态 | `sourceWorktreeClean=false`；`candidateEligible=false`；既有 `package/`、`scripts/__pycache__/` 和本轮修改均未作为构建或运行时输入 |
| `windeployqt` | 固定 6.11.2 工具 hash；dry-run/actual 均退出 0；1,428 项映射，其中 Qt 1,425、Microsoft D3D x64 Redist 3 |
| 运行闭包 | 81 个 PE、103,338,584 字节；79 DLL + 2 EXE；全部 PE32+ x64；递归导入无未解析项 |
| 组件 | Space Rhythm 2、Qt 70、FFmpeg 5、KissFFT 1、Microsoft D3D Redist 3；OpenCV/GoogleTest 仅作 build input |
| 签名 | 78 `NotSigned`、3 个 Microsoft 文件 `Valid`；未签名、未访问凭据 |
| 排除项 | `qmltooling`、`generic`、translations、Debug CRT、PDB、测试/构建工具、`vc_redist.x64.exe` 均不存在 |
| 供应链材料 | runtime/payload/bundle hash manifest、build-inputs schema 2（7 个组件及 A-032 必需字段）、SPDX 2.3、上游 SPDX、许可证/notices、Qt 替换说明、known limitations、credential-free signing request 均存在并受 payload hash 覆盖 |
| 可重复归档 | 相同输入连续两次生成的 ZIP SHA-256 均为 `9591F8B0E3B1CD28789A785D91077D298EE10011B4A7D3EBEA810DAC9781449B` |

当前递归导入只从固定包内、`out/vcpkg_installed/x64-windows-space-rhythm/bin` 或 Windows 系统依赖解析。相比旧 CMake install 暂存，未导入的 `avdevice-62.dll`、`avfilter-11.dll` 不再进入闭包；OpenCV 继续记录为 build input，但当前 App/Worker 闭包未导入 OpenCV DLL。

## 事务与启动验证

执行：

```powershell
./tests/release/Test-UnsignedPackage.ps1 `
  -BundleRoot ./out/release/T-037/unsigned/space-rhythm-0.1.0-dev-unsigned `
  -RunSmoke
```

较早同一运行闭包的结果为 pass，实际顺序：

1. bundle payload manifest 校验；
2. 同卷 staging 安装与安装后 manifest 校验；
3. App offscreen smoke 输出 `SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64`，退出 0；
4. Worker smoke 输出 `SPACE_RHYTHM_WORKER_SMOKE_OK Qt=6.11.2 arch=x64`，退出 0；
5. repair 创建已验证 backup 并切换；
6. rollback 验证并交换当前/backup；
7. 已安装 payload 再校验；
8. uninstall 删除登记的应用根与 backup，保留审计状态；外部用户数据未触碰。

最终组件元数据扩展前的受控归档在一次完整通过后，后续复跑只完成第 1～2 步，便在第 3 步 App smoke fail closed；没有执行后续步骤，也没有用较早成功替代最新失败。对应 Code Integrity 窗口对测试安装根的 App 记录 3033/3077/3118、`VerifiedAndReputableDesktop`、`0xC0E90002`。当前最终归档由固定 `windeployqt` 映射额外纳入 `qcertonlybackend.dll`、`qschannelbackend.dll`，形成表中 81 个 PE；为避免反复尝试执行，它只运行不带 smoke 的事务专项，并完整通过 `validate → install → 拒绝含未登记文件的 repair → repair → rollback → installed validate → uninstall`，且显式根外用户数据哨兵保留，从而把事务正确性与 WDAC/SAC 执行阻断分离。

随后只做一次定位性 bundle 原路径诊断：App 为 `started-nonzero-exit:2`，stderr 是 `SPACE_RHYTHM_APP_SMOKE_FAILED: QML root was not created`；probe window 的 4 个新 Code Integrity 事件是 `Qt6QuickDialogs2.dll` 的两组 3033/3077，文件 SHA-256 `CEFC1734C74EE5E2F7B6A1AE7AA68556002C711B56446E78FC0465A5440E9DCB`，状态 `0xC0E90002`。Worker 同轮为 `started-exit-zero:0`、新策略事件 0。测试安装随后通过登记 manifest 单独清理。

首次 App 诊断未设置 offscreen 导致 timeout 且无策略事件；该错误配置记录不作为通过或 WDAC 拒绝结论。整体证据保留“早先 App/Worker 可运行、最终 App/Qt DLL 被拒”的顺序，不用重试美化最新状态。

## 安全边界与未完成项

- 未改变或绕过 WDAC/SAC，未改白名单/ACL，未使用 `-AllowWdacFallback`，未访问或使用签名凭据。
- 78 个运行 PE 仍未签名，严格 App 启动门禁为 `blocked`；H-015 保持 open。此前固定来源二进制曾成功只能说明策略判定不稳定，不能替代后续失败或当前归档的未验证启动状态。
- 正式安装器/scope、VC Runtime、最低 Windows、签名主体/证书/时间戳/渠道、产品格式/H.264/AAC、默认音色/视觉风格、T-022/T-038 仍未完成。
- 完整机器证据位于被忽略的 `out/evidence/T-037/`，生成 bundle 位于被忽略的 `out/release/T-037/`。

## 复现入口

```powershell
./tooling/windows/Invoke-UnsignedRelease.ps1 -Version 0.1.0-dev
./tests/release/Test-UnsignedPackage.ps1 `
  -BundleRoot ./out/release/T-037/unsigned/space-rhythm-0.1.0-dev-unsigned `
  -RunSmoke
```

默认组包入口要求干净 Git 工作区。本次仅因代码尚在形成过程中使用 `-AllowDirtySource`，并由生成清单强制标记非候选；提交后应在干净提交上不带该开关重新生成，届时也仍是 unsigned 工程包，不能自动成为发布候选。

仓库级校验同时通过：`validate_framework.py` 核对 17 个岗位、24 份知识、1 个实际项目、1 套模板和 683 处本地链接；`python -m unittest discover -s scripts -p 'test_*.py'` 为 37/37 通过；`git diff --check` 通过。
