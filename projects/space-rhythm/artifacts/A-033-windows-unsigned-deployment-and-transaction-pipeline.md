# Windows 个人未签名部署与事务安装工程流水线

- 项目：space-rhythm
- 成果 ID：A-033
- 负责人：release-engineer-windows-01
- 关联任务：T-037
- 版本：0.2
- 更新日期：2026-09-15
- 状态：draft
- 适用范围：按 D-012/D-013，从固定 Windows x64 Release 构建生成仅供项目用户本人在自有 Windows 电脑使用的 `unsigned-engineering` 工程包，覆盖受控 `windeployqt`、递归 PE 依赖闭包、哈希、SPDX、许可证及显式路径的安装/修复/回滚/卸载事务；不公开分发或交付第三方，不接触签名凭据，不改变 WDAC/SAC，不宣称 SAC/WDAC 或其他 Windows 环境兼容，不批准生产发布。
- 来源及输入版本：用户于 2026-09-15 明确要求继续执行 T-037，并在 D-012/D-013 确认个人未签名范围、当前 `TIGER` 和 SAC-off 验证条件；T-013、T-019、T-032、T-035、T-036 completed；D-003～D-008、D-012、D-013 confirmed；A-011 0.1、A-013 0.2、A-020 0.1、A-025 0.1、A-026 0.1、A-032 0.1；最终受控包源提交 `02c65ce4b596675d102ed3c82459528b60f63297`。
- 批准依据：D-012/D-013 仅批准个人未签名交付范围和当前主机验证条件；不批准公开分发、SAC/WDAC 兼容、发布候选或生产发布。
- 版本记录：2026-09-15，0.1，首次交付 unsigned 受控组包、供应链材料、确定性 ZIP 和事务测试路径；2026-09-15，0.2，对齐 D-012/D-013，生成 schema 3 个人范围包并在 SAC=0 的当前 `TIGER` 完成 App/Worker smoke，收口 T-037。

## 1. 交付结论

T-037 已在 D-012/D-013 的个人未签名范围内形成并验证可执行流水线：

- [`Invoke-UnsignedRelease.ps1`](../../../tooling/windows/Invoke-UnsignedRelease.ps1) 只消费当前 `windows-msvc-x64-release` 构建、固定 Qt 6.11.2 SDK 和固定 vcpkg 安装树，刷新 Release 构建后生成应用私有闭包。
- [`Invoke-UnsignedInstallTransaction.ps1`](../../../tooling/windows/Invoke-UnsignedInstallTransaction.ps1) 是本阶段确认的个人工程交付机制；不提供独立 GUI 安装器，所有操作必须显式给出安装根和状态根。
- [`Test-UnsignedPackage.ps1`](../../../tests/release/Test-UnsignedPackage.ps1) 验证包类型、安全标记、必需/禁止文件、SPDX 结构及安装、App/Worker smoke、修复、回滚和卸载事务。
- [仓库使用说明](../../../docs/windows-unsigned-release.md)记录正常命令、失败边界和剩余门禁。

最终输出是 `space-rhythm-0.1.0-dev-unsigned.zip`，SHA-256 `CD94BC9CABF1B0AD29062EE39DD14DEBCBF2AAEB6B777D69036874221D8C634C`，大小 44,725,623 字节；相同输入连续两次生成一致。其源提交为 `02c65ce4b596675d102ed3c82459528b60f63297`；清单只记录既有未跟踪 `package/` 和 `scripts/__pycache__/`，两者未作为构建或运行时输入，因此 `sourceWorktreeClean=false`。包始终为 `candidateEligible=false`，不是发布候选。

## 2. 输入、来源与 fail-closed 规则

流水线先核对：

1. Qt `Qt6Core.dll` 与 `config.summary` 的 T-012 固定哈希，以及 `windeployqt.exe` 6.11.2 的 SHA-256 `889DFACFB42270715D7640687CB2C5DA82FCFAAA829C2E1588BC9DE00ED8D93D`。
2. Release `CMakeCache.txt` 中的 `Release`、`C:/sr/q/qt6112` 和 `x64-windows-space-rhythm` 元组，并通过 CMake preset 刷新构建。
3. vcpkg checkout `9e593bb18ea69cc5095e012465dcd675a822ed0d` 及 FFmpeg/OpenCV/KissFFT/GoogleTest 当前安装版本、triplet 与 ABI hash。
4. 默认要求 Git 工作区干净；`-AllowDirtySource` 只能生成清楚标记为非候选的工程包，并把全部 dirty 状态写入输入清单。

`windeployqt --dry-run --list mapping` 的来源只能是固定 Qt 根，或 Windows Kits `Redist/D3D/x64` 下指定的 `d3dcompiler_47.dll`、`dxcompiler.dll`、`dxil.dll`；后三者还必须通过 Microsoft Authenticode 验证。任一未知来源、绝对/越界目标、缺少 `Qt6QmlMeta.dll`/`qwindows.dll`/`qoffscreen.dll` 都立即失败。

实际部署后逐文件比对源/目标 SHA-256。所有 PE 再由 `dumpbin` 递归解析；未满足“已在包内、固定 vcpkg bin、API set 或 Windows System32”之一的导入会阻止组包。每个进入包的 EXE/DLL 都必须是 PE32+ x64并有受控来源记录。

## 3. 当前运行闭包

本轮 dry-run 为 1,428 项映射：1,425 项来自 Qt 根，3 项来自签名有效的 Windows SDK D3D Redist。最终递归导入闭包为 81 个 PE、103,338,584 字节：

| 组件 | PE 数 | 当前边界 |
|---|---:|---|
| Space Rhythm | 2 | App、Worker；均为当前 Release 构建产物、`NotSigned` |
| Qt 6.11.2 | 70 | 共享 DLL、插件和 QML plugin（含 `qcertonlybackend`、`qschannelbackend`）；均从固定 SDK 映射、`NotSigned` |
| FFmpeg 8.1.2#3 | 5 | 仅实际递归导入的 `avcodec`、`avformat`、`avutil`、`swresample`、`swscale`；未再复制未导入的 `avdevice`/`avfilter` |
| KissFFT 131.2.0 | 1 | 实际导入的 `kissfft-float.dll` |
| Microsoft Windows SDK D3D Redist | 3 | 三个指定文件，均为 Microsoft 签名且 Authenticode `Valid` |

共 78 个 PE 为 `NotSigned`，3 个 Microsoft 文件为 `Valid`。包中没有 `qmltooling/`、`generic/`、translations、Debug CRT、测试程序、PDB、构建工具或 `vc_redist.x64.exe`。VC Runtime 分发方式尚未确认，因此只记录系统导入和限制，不擅自捆绑 redistributable 或私有 CRT。

## 4. SBOM、许可证与签名隔离

payload 同时包含：

- `manifest/runtime-files.sha256.csv`：每个运行 PE 的相对路径、大小、SHA-256、组件/版本、vcpkg ABI 或 Qt/源码构建 hash、受控来源、架构、链接方式与 Authenticode 状态。
- `manifest/build-inputs.json` schema 3：除源提交、dirty 状态、工具链、固定哈希、部署和 7 个组件清单外，机器可读固定 `personal-unsigned`、仅自有 Windows、禁止公开分发/第三方交付及 `sacWdacCompatibilityClaim=none`。
- `manifest/payload-files.sha256.csv` 与 bundle manifest：分别验证完整 payload 和 bundle，拒绝未登记文件。
- `sbom/space-rhythm.spdx.json`：SPDX 2.3 汇总；另保留 Qt 四份上游 SPDX 及 FFmpeg/OpenCV/KissFFT/GoogleTest 的 vcpkg SPDX。
- `licenses/`、`THIRD-PARTY-NOTICES.txt`、Qt 源码/构建/动态替换说明与 known limitations。它们是工程材料，不代替法律意见。
- `signing/signing-request.json`：只保存未来若扩大分发范围时所需的文件哈希和 Authenticode 状态；当前 `status=not-required-for-personal-unsigned-scope`、`credentialAccess=none`，没有签名命令、PIN、token 或私钥。

确定性 ZIP 以源提交时间固定所有 entry timestamp；相同输入连续两次实际生成的归档 SHA-256 相同。若未来扩大到公开/第三方分发并重新要求签名，须另立决定并从冻结闭包建立隔离签名阶段；本流水线未访问任何凭据。

## 5. 安装、修复、回滚与卸载事务

D-012 已确认用显式路径事务工具作为个人工程交付机制，不要求独立 GUI 安装器。调用者必须给出互不包含的 `BundleRoot`、`InstallRoot`、`StateRoot`，且拒绝驱动器根：

- `Install/Repair`：先验证源 manifest，复制到目标卷同级 staging，再验证 staging；只允许替换与状态 manifest 一致的已登记 payload，将其移到唯一登记 backup，最后以目录 move 切换并原子写状态；状态提交失败时移走新 payload 并恢复旧目录。
- 失败恢复：切换或状态写入失败时恢复上一安装；不把部分文件写入现有根。
- `Rollback`：验证当前状态 hash 和已登记 backup 后交换目录，并保留反向切换所需的一层 backup；交换或状态写入失败时双向恢复。
- `Uninstall`：先验证当前 payload、状态 hash 和登记 backup，再把两者移入同卷隔离目录，原子提交卸载状态后才清除；只写显式状态根。
- 用户项目、原始媒体、设置、自动保存、缓存和日志位于这两个显式根之外时一律不读、不改、不删；当前没有 purge 用户数据选项。

最终包在当前 `TIGER` 实际完成 `bundle manifest → payload manifest → install → App smoke → Worker smoke → 拒绝含未登记文件的 repair → repair → rollback → installed validate → uninstall`。卸载后安装根不存在，审计状态为 `installed=false/lastAction=uninstall`，显式根外用户数据哨兵仍存在。

## 6. 当前 TIGER 的 SAC-off 启动结果

最终验证主机为 `TIGER`/AMD64；注册表报告 `ProductName=Windows 10 Home China`、`DisplayVersion=25H2`、build `26200.9457`。正式测试前检查且测试结束后复核 `VerifiedAndReputablePolicyState=0`。测试窗口为 `2026-09-15T07:19:05.0834419Z` 至 `2026-09-15T07:19:37.6414752Z`：

- App SHA-256 `7391BE3B859E41A2567AC03842C13D57C9FE7D9314F15C3EEAC0D64AB3EBC082`，offscreen smoke 输出 `SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64`，退出 0。
- Worker SHA-256 `5C12BB3F390544314BDC3B633CEE13E463FF728C959663FC66695916EFF15260`，输出 `SPACE_RHYTHM_WORKER_SMOKE_OK Qt=6.11.2 arch=x64`，退出 0。
- 该窗口内按 `space-rhythm|Qt6|T-037-unsigned-transaction` 过滤 Code Integrity Operational 与 AppLocker EXE and DLL，相关事件数为 0。

结构化记录见 [`tiger-sac-off-smoke-20260915.json`](../evidence/T-037/tiger-sac-off-smoke-20260915.json)。这里证明的仅是这两个精确哈希在当前 TIGER、当前 SAC=0 条件下完成 smoke；SAC 未处于执行兼容性门禁的状态，因此结果不能用于宣称软件兼容 SAC/WDAC，也不能外推到其他 Windows 主机。历史 SAC 开启时的拒绝证据继续保留，但不再是 D-012/D-013 范围内的 T-037 前置。

## 7. 完成结论与后续边界

T-037 的完成条件已满足：受控私有闭包、个人工程安装事务、可复现 ZIP、哈希、schema 3 构建输入、SPDX/许可证、明确 unsigned/个人范围，以及 D-013 当前 TIGER 的 App/Worker smoke 均已验证，任务可标记 `completed`。

T-022、T-038、最低 Windows、VC Runtime 分发、产品容器/H.264/AAC、默认音色和视觉风格仍是各自责任链的后续工作，不反向阻止 T-037，但使当前包继续保持 `unsigned-engineering`、`candidateEligible=false`。独立 GUI 安装器、签名和 SAC/WDAC 兼容已由 D-012/D-013 移出本阶段范围；未来扩大分发时须重新决定，不能沿用本次结果作兼容或生产发布证明。
