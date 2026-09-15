# Windows unsigned 部署与事务安装工程流水线

- 项目：space-rhythm
- 成果 ID：A-033
- 负责人：release-engineer-windows-01
- 关联任务：T-037（unsigned 部分）
- 版本：0.1
- 更新日期：2026-09-15
- 状态：draft
- 适用范围：从固定 Windows x64 Release 构建生成明确标识为非候选的 unsigned 工程包，覆盖受控 `windeployqt`、递归 PE 依赖闭包、哈希、SPDX、许可证、签名交接清单及显式路径的安装/修复/回滚/卸载事务；不选择正式安装器或 scope，不接触签名凭据，不改变 WDAC/SAC，不批准生产发布。
- 来源及输入版本：用户于 2026-09-15 明确要求继续执行 T-037 的 unsigned 部分；T-013、T-019、T-032、T-035、T-036 completed；D-003～D-008 confirmed；A-011 0.1、A-013 0.2、A-020 0.1、A-025 0.1、A-026 0.1、A-032 0.1；当前工程验证源 HEAD `41ce475c620fbfbb2e299130ea9e18477971ff47`。
- 批准依据：尚无。unsigned 工程包和事务测试成功不等于安装器选型、签名、兼容矩阵、发布候选或生产发布批准。
- 版本记录：2026-09-15，0.1，首次交付 unsigned 受控组包、供应链材料、确定性 ZIP 和事务测试路径。

## 1. 交付结论

T-037 的 unsigned 部分已经形成可执行流水线：

- [`Invoke-UnsignedRelease.ps1`](../../../tooling/windows/Invoke-UnsignedRelease.ps1) 只消费当前 `windows-msvc-x64-release` 构建、固定 Qt 6.11.2 SDK 和固定 vcpkg 安装树，刷新 Release 构建后生成应用私有闭包。
- [`Invoke-UnsignedInstallTransaction.ps1`](../../../tooling/windows/Invoke-UnsignedInstallTransaction.ps1) 是正式安装器未定期间的后端中立工程工具；所有操作必须显式给出安装根和状态根。
- [`Test-UnsignedPackage.ps1`](../../../tests/release/Test-UnsignedPackage.ps1) 验证包类型、安全标记、必需/禁止文件、SPDX 结构及安装、App/Worker smoke、修复、回滚和卸载事务。
- [仓库使用说明](../../../docs/windows-unsigned-release.md)记录正常命令、失败边界和剩余门禁。

当前实际输出是 `space-rhythm-0.1.0-dev-unsigned.zip`，SHA-256 `9591F8B0E3B1CD28789A785D91077D298EE10011B4A7D3EBEA810DAC9781449B`，大小 44,725,377 字节。由于本轮源码修改和既有 `package/`、`scripts/__pycache__/` 未跟踪内容尚未提交，清单明确记录 `sourceWorktreeClean=false`、`candidateEligible=false`；这只是工程验证包，不是发布候选。

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
- `manifest/build-inputs.json` schema 2：源提交、dirty 状态、工具链、固定哈希、`windeployqt` 参数/映射数、系统依赖和未决输入；7 个源/构建/运行组件逐项包含 purl、来源、源码 hash/声明、构建摘要、许可证路径、修改/补丁、再分发依据、运行文件 hash、签名聚合与任务证据。
- `manifest/payload-files.sha256.csv` 与 bundle manifest：分别验证完整 payload 和 bundle，拒绝未登记文件。
- `sbom/space-rhythm.spdx.json`：SPDX 2.3 汇总；另保留 Qt 四份上游 SPDX 及 FFmpeg/OpenCV/KissFFT/GoogleTest 的 vcpkg SPDX。
- `licenses/`、`THIRD-PARTY-NOTICES.txt`、Qt 源码/构建/动态替换说明与 known limitations。它们是工程材料，不代替法律意见。
- `signing/signing-request.json`：只保存待签文件哈希、当前 Authenticode 状态和待授权输入；`status=not-requested`、`credentialAccess=none`，没有签名命令、PIN、token 或私钥。

确定性 ZIP 以源提交时间固定所有 entry timestamp；相同输入连续两次实际生成的归档 SHA-256 相同。签名阶段未来必须消费已冻结的 unsigned 哈希闭包，并在隔离环境重新生成签名后哈希/SBOM；本流水线未访问任何凭据。

## 5. 安装、修复、回滚与卸载事务

正式安装器和每用户/每机器 scope 未确认，因此事务工具不提供默认系统目录。调用者必须给出互不包含的 `BundleRoot`、`InstallRoot`、`StateRoot`，且拒绝驱动器根：

- `Install/Repair`：先验证源 manifest，复制到目标卷同级 staging，再验证 staging；只允许替换与状态 manifest 一致的已登记 payload，将其移到唯一登记 backup，最后以目录 move 切换并原子写状态；状态提交失败时移走新 payload 并恢复旧目录。
- 失败恢复：切换或状态写入失败时恢复上一安装；不把部分文件写入现有根。
- `Rollback`：验证当前状态 hash 和已登记 backup 后交换目录，并保留反向切换所需的一层 backup；交换或状态写入失败时双向恢复。
- `Uninstall`：先验证当前 payload、状态 hash 和登记 backup，再把两者移入同卷隔离目录，原子提交卸载状态后才清除；只写显式状态根。
- 用户项目、原始媒体、设置、自动保存、缓存和日志位于这两个显式根之外时一律不读、不改、不删；当前没有 purge 用户数据选项。

在最终组件元数据扩展之前，一份受控归档曾在仓库忽略的测试根完整通过 `validate → install → App smoke → Worker smoke → repair → rollback → installed validate → uninstall`；随后对该归档复跑时完成 manifest 校验和 staging 安装，却在 App smoke 处受主机策略阻断并 fail closed，没有继续执行或用较早结果替代最新失败。定位完成后清理了已登记测试安装。当前最终归档由固定 `windeployqt` 映射额外纳入两个 Qt TLS 插件，闭包为上节记录的 81 个 PE；为避免在已经获得策略拒绝证据后反复尝试执行，未再次启动 App。最终事务专项实际完成 `validate → install → 拒绝含未登记文件的 repair → repair → rollback → installed validate → uninstall`，并确认显式根外的用户数据哨兵仍存在。

## 6. 启动与 WDAC/SAC 结果（最终元数据扩展前的受控归档）

较早的正确 offscreen 运行中，App 输出 `SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64` 并退出 0，Worker 输出对应 marker 并退出 0，两个 probe window 均无新增 Code Integrity/AppLocker 事件。首次遗漏 offscreen 的 App 诊断超时且无策略事件，已明确判为错误配置，不作为通过或 WDAC 拒绝。

该受控归档的后续事务复跑出现相反结果：从测试安装根启动 App 时，Code Integrity 3033/3077/3118 记录 `VerifiedAndReputableDesktop`、`0xC0E90002`，拒绝未签名 App。为定位而进行的一次 bundle 原路径诊断允许 App 进程启动，但在加载 `Qt6QuickDialogs2.dll` 时产生两组 3033/3077；该 DLL SHA-256 为 `CEFC1734C74EE5E2F7B6A1AE7AA68556002C711B56446E78FC0465A5440E9DCB`，与当前最终归档中的同名 DLL 一致，App 报告 `QML root was not created` 并退出 2。Worker 在该轮为 `started-exit-zero:0`，窗口内无新策略事件。

这组先成功、后分别阻断 App 和 Qt DLL 的真实记录进一步确认 SAC/WDAC 判定不稳定，H-015 不能关闭。78 个运行 PE 仍为 `NotSigned`。本轮未修改策略、白名单、ACL 或签名，没有 fallback，也没有用再次重试把最新失败美化为通过。

## 7. 剩余门禁

T-037 保持 `in_progress`：unsigned 组包、供应链和事务实现已完成，但 unsigned 严格 App 启动仍被 H-015 阻断；以下也阻止整个任务完成：

- 正式安装器、每用户/每机器 scope、升级/自动更新/文件关联和 VC Runtime 策略尚未确认。
- 签名主体、证书或托管签名服务、时间戳和渠道尚未授权；H-015 的组织管理 WDAC/SAC 信任路线未完成。
- 产品容器/H.264/AAC、默认音色和视觉风格未确认；当前应用仍内嵌开发用 CC0 测试音色，故包强制非候选。
- T-022、最低 Windows 与 T-038 干净机器矩阵尚未完成。

这些缺口不影响继续复核 unsigned 工程脚本，但任何输出都必须保持 `unsigned-engineering`、`candidateEligible=false`，不得被重命名为 installer、release candidate 或 production release。
