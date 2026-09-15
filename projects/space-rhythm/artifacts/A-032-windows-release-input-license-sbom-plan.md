# Windows 发布输入、许可证与 SBOM 计划

- 项目：space-rhythm
- 成果 ID：A-032
- 负责人：release-engineer-windows-01
- 关联任务：T-036；作为 T-037、T-038 的输入
- 版本：0.1
- 更新日期：2026-09-14
- 状态：draft
- 适用范围：第一阶段 Windows x64 未签名发布候选的输入冻结、应用私有依赖布局、许可证/SBOM、安装事务、签名隔离和干净环境验证计划；不作法律结论，不选定未确认的安装器/渠道，不接触签名凭据，不批准生产发布。
- 来源及输入版本：用户于 2026-09-14 要求 release-engineer-windows-01 按会话协议初始化、接收 H-010、优先执行 T-036 并处理当前 Qt DLL、`windeployqt` 与 WDAC/SAC 阻塞；D-003～D-008 confirmed；A-004 0.5、A-005 0.4、A-006 0.1 WP-10、A-009 0.1、A-011 0.1、A-013 0.2；T-018 运行时清单；本轮当前输入审计的 Git HEAD 为 `2b675339971f9edad53c03a01a23921dd95f4189`。
- 批准依据：尚无。D-003～D-008 是已确认技术输入，不等于批准本计划、安装器或生产发布。
- 版本记录：2026-09-14，0.1，首次冻结发布输入和工程门禁，定位当前 Qt DLL/WDAC 根因，并形成 T-037/T-038 的可执行计划。

## 1. 结论

T-036 所需的发布输入、布局、许可证/SBOM、安装事务、签名隔离与干净环境计划已形成。当前可以进入 T-037 的未签名部署工程，但不能冻结为发布候选：

1. 当前 Qt 6.11.2 DLL 来自 T-012 受控 SDK，版本、x64 ABI 与关键哈希正确，`windeployqt` 也已完成一次实际 Release 部署。
2. 当前严格启动失败不是缺 DLL，而是 `VerifiedAndReputableDesktop` SAC/WDAC 拒绝加载未签名 `Qt6QmlMeta.dll`，状态 `0xC0E90002`。这个信任链问题也可能逐个影响其他自建 Qt DLL、应用和 Worker。
3. `windeployqt.exe` 本身为未签名自建工具，本轮先被策略拒绝，后续相同哈希的两次受控诊断和一次实际部署又成功。这是不稳定的声誉/策略结果，重试成功不得记为修复或发布通过。
4. 发布前必须由有权主机策略管理员提供可持续的开发/CI 信任路线，并由有权确认人确定最低 Windows、安装器、正式格式/H.264 后端、签名主体/证书和渠道。

## 2. 当前发布输入快照

| 项目 | 当前值 | 冻结判定 |
|---|---|---|
| 源提交 | `2b675339971f9edad53c03a01a23921dd95f4189` | 只是本轮审计基线；候选冻结时必须使用干净、可引用的源提交，不纳入 `package/`、`scripts/__pycache__/` 等无关未跟踪内容 |
| 构建预设 | `windows-msvc-x64-release` | 已可配置/构建/安装；严格 Release 运行门禁尚未通过 |
| 工具链 | MSVC 19.44.35228、Windows SDK 10.0.26100.0、CMake 3.31.6-msvc6、Ninja 1.12.1 | 与 T-012/T-013 基线一致 |
| Qt | 6.11.2，Windows x64，shared/dynamic，LGPLv3 路径 | 版本与 ABI 已冻结；运行时签名/信任尚未冻结 |
| 非 Qt 依赖 | vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`；FFmpeg 8.1.2、OpenCV 4.12.0、KissFFT 131.2.0 | 精确版本已有构建证据；最终产品链接闭包和 notices 待 T-037 冻结 |
| 当前安装暂存 | 136,035,733 字节；92 个 DLL、3 个 EXE、491 个 QML、804 个 PNG | 仅是 T-013 工程暂存，不是发布候选 |
| Qt 来源匹配 | 92 个暂存 DLL 中 81 个按 SHA-256 可回溯到 T-012 Qt SDK，11 个为 FFmpeg/KissFFT/Microsoft 图形运行时 | 说明 `windeployqt` 复制来源正确；仍需进一步缩减发布白名单 |
| 签名状态 | 92 个 DLL 中 89 个 `NotSigned`；App、Worker、`Qt6QmlMeta.dll`均 `NotSigned` | 不得作为 WDAC/SAC 受管环境候选 |
| 质量输入 | T-022 尚未完成；当前严格 Release 有 WDAC/SAC 缺口 | 阻止 T-038 和最终候选冻结 |

## 3. 发布冻结输入门禁

T-037 可以先实现 unsigned 流水线；只有下表全部满足时才能生成待 T-038 验证的候选编号。

| 门禁 | 必需输入 | 失败行为 |
|---|---|---|
| 源码 | 干净 Git 提交、候选版本、受控构建预设 | 不生成候选编号 |
| 工具链 | MSVC/SDK/CMake/Ninja/vcpkg/Qt 精确元组与关键哈希 | 任一漂移即停止 |
| 依赖 | 仅从构建安装图和锁定 SDK/vcpkg 目录复制；每件有版本、来源、架构、哈希、许可证 | 禁止从系统 `PATH` 或开发机其他目录拾取 DLL |
| 产品功能 | T-022 端到端、故障恢复及 G0～G4 前置证据 | 不以“能打包”代替功能验收 |
| 格式/内容 | 正式容器/H.264 后端、音色/模板内容及商用范围 | 未确认时仅保留 `testOnly` 输出，测试音色不进候选 |
| 安装与系统 | 最低 Windows、安装器、升级模式、安装 scope、VC Runtime 策略 | 不声明未测兼容性 |
| 签名与策略 | 有权主体、证书/签名服务、时间戳、开发/CI 与生产身份分离、WDAC 复验 | 未授权时只生成明确标识的 unsigned 产物 |
| 证据 | 构建/测试/部署/签名/扫描日志、文件清单、SBOM、notices、已知限制 | 任一缺失即保持未冻结 |

## 4. 应用私有部署布局

```text
SpaceRhythm/
  bin/
    space-rhythm.exe
    space-rhythm-worker.exe
    Qt6*.dll
    av*.dll, sw*.dll, kissfft-float.dll
    platforms/qwindows.dll, platforms/qoffscreen.dll
    imageformats/, networkinformation/, tls/
    qml/<经 QML import 扫描冻结的模块>/
  licenses/
    qt/, ffmpeg/, opencv/, kissfft/, microsoft/, content/
  notices/THIRD-PARTY-NOTICES.txt
  sbom/space-rhythm.spdx.json
  manifest/runtime-files.sha256.csv
  manifest/build-inputs.json
  docs/qt-source-build-and-replacement.md
  docs/known-limitations.md
```

约束：

- DLL 仅使用应用私有目录，不复制、替换或注册系统共享 DLL。
- Qt LGPLv3 路径保持动态链接和 Qt DLL 可替换性；替换说明必须包含备份、哈希、架构/ABI 核对和恢复步骤。
- 符号、测试程序、Debug CRT、`qmltestrunner`、`qmllint`、`windeployqt` 等构建工具不进入产品运行目录；符号单独受控归档。
- `vc_redist.x64.exe` 不放在产品 `bin/` 中当作运行时 DLL。安装器选定后，二选一：由受控 bootstrapper 安装已核对的 Microsoft VC Redistributable，或在发布权限允许时使用应用私有 CRT 并逐文件建清单。
- 当前 QML 扫描会带入全部 Qt Quick Controls 风格；产品风格未确认前不冒险删除。T-037 必须先排除开发期 `qmltooling` 和 `generic` 插件，再依真实用户路径缩减风格及图像插件。

## 5. `windeployqt` 受控调用规则

权威工具固定为 `C:\sr\q\qt6112\bin\windeployqt.exe`，版本 `6.11.2.0`，SHA-256 `889DFACFB42270715D7640687CB2C5DA82FCFAAA829C2E1588BC9DE00ED8D93D`。发布流水线必须：

1. 在调用前核对工具哈希、Qt `config.summary` 和 `Qt6Core.dll` 基线；禁止用 `PATH` 中其他副本。
2. 先使用 `--dry-run --list mapping` 生成部署映射，将映射与批准的运行时白名单比较；出现 Debug DLL、未知根目录或新模块时失败。
3. Release 基线使用 `--release --no-translations --skip-plugin-types qmltooling,generic --qmldir <src/app/qml> --include-plugins qoffscreen`；保留必需的 `qwindows` 和 `qoffscreen`。本轮 dry-run 实测退出 0，计划映射 1,423 项/68 个 DLL，`qmltooling=0`、`generic=0`，并包含 `qwindows`、`qoffscreen`、`Qt6QmlMeta.dll`。
4. 调用后重新建立 SHA-256、PE x64、直接导入、来源和签名清单；不以 `windeployqt` 退出 0 代替运行时白名单和严格启动。
5. 若工具被 WDAC/SAC 拒绝，当次流水线失败并采集 Code Integrity 证据；不使用构建树中同哈希副本、反复重试、重链接换哈希或 `-AllowWdacFallback` 作为发布规避。

## 6. SBOM 与许可证字段

每个源包和运行时文件至少保存：`componentName`、`componentVersion`、`purl`、`sourceUrl`、`sourceCommitOrArchiveHash`、`buildConfigurationHash`、`runtimeRelativePath`、`fileSha256`、`architecture`、`linkage`、`licenseExpression`、`licenseTextPath`、`copyrightNotice`、`modified`、`patches`、`redistributionBasis`、`supplier`、`authenticodeStatus`、`signerThumbprint`、`taskEvidence`。

| 组件 | 当前版本/链接 | 当前工程依据 | 发布前缺口 |
|---|---|---|---|
| Qt | 6.11.2，shared | D-007 LGPLv3；官方源归档及哈希、配置参数、四份 SPDX 2.3 已有 T-012 证据 | 合并 notices、源码取得/书面提供方式、构建与替换说明；最终模块白名单；运行时签名/信任 |
| FFmpeg | 8.1.2#3，dynamic | `default-features=false`，含 `version3`，无 GPL/nonfree；配置哈希 `e952f157...61259`；7 个 Release DLL 有哈希证据 | 最终编解码矩阵与 H.264 后端未确认；合并完整许可文本/notices |
| OpenCV | 4.12.0，仅经典算法 feature | vcpkg baseline 可回溯，不含 DNN/Qt/内建 FFmpeg | 当前应用暂存无 OpenCV DLL；冻结真实产品链接图后确定是静态内含还是运行时缺失，并补 Apache-2.0 notices |
| KissFFT | 131.2.0，dynamic | `kissfft-float.dll` 已进入当前暂存；BSD-3-Clause 文本已安装 | 合并 notices 和精确文件哈希 |
| GoogleTest | 1.17.0 | 测试/构建依赖 | 禁止 `gtest*.dll` 进入产品运行闭包；仅在构建 SBOM 中标记 `development` |
| MSVC/UCRT/D3D/DXC | VS 2022 17.14/MSVC 19.44/SDK 10.0.26100 | T-012/T-013 工具链元组；当前 D3D/DXC 文件为 Microsoft 签名 | 确定 VC Runtime 部署模式、重分发条款与最低 Windows 系统依赖 |
| 音色/模板/模型 | 当前仅测试音色和技术模板；ONNX 未引入 | 测试素材与产品内容分离 | 默认音色/视觉风格、来源和商用权限待确认；未确认时不进候选 |
| 安装器 | 未选定 | 无 | 选型、版本、许可证、构建器哈希和签名格式全部待确认 |

SBOM 以 SPDX JSON 为交付格式，同时生成人可读 `THIRD-PARTY-NOTICES.txt`。项目源组件、构建/测试依赖和运行时依赖必须分 scope，不因文件未被复制就从源级 SBOM 消失。

## 7. 安装、升级、卸载与回滚事务

| 数据/目录 | 安装/升级 | 卸载 | 失败/回滚 |
|---|---|---|---|
| 应用二进制与私有依赖 | 先写新版本暂存，哈希/签名/冒烟通过后原子切换 | 删除已登记应用文件 | 保留上一成功版本；切换失败恢复旧指针 |
| 用户项目/媒体 | 绝不改写原始媒体；项目 schema 先备份再迁移 | 默认保留，只有明确用户动作才删除 | 迁移失败保留旧项目和可识别错误 |
| 用户设置 | 向前兼容或版本化迁移 | 默认保留，卸载器提供明确可选清理 | 旧版本设置可恢复 |
| 代理/缩略图/特征缓存 | 按算法版本和素材指纹失效，不作真值 | 默认可清理，但必须明确告知 | 失败时可删除并重建，不影响项目主文档 |
| 自动保存 | 保留最近成功版本和时间戳 | 默认保留，与项目删除分离 | 不被安装失败或应用回滚覆盖 |
| 日志/崩溃信息 | 按版本与诊断 ID 保留，默认不收集媒体或特征 | 提供独立清理选项 | 保留最小失败证据，设置配额与脱敏规则 |

安装 scope（每用户/每机器）、自动更新是否存在以及项目扩展名关联尚未确认，T-037 不得自行假定。

## 8. 签名隔离与 WDAC/SAC 处置

### 8.1 签名边界

- 普通构建不持有证书私钥。签名由独立服务或受控主机执行，输入必须是已冻结、哈希稳定的未签名闭包。
- 开发/CI 签名身份与生产发布身份分离；策略规则只信任受控 signer/publisher，不信任用户 profile、Desktop、仓库根或任意未签名文件。
- 最终对安装器、App、Worker、全部自建/第三方可执行文件和 DLL 签名并时间戳；签名后重建哈希、SBOM 与签名清单。
- 日志只保存证书主体、拇指、签名算法、时间戳结果和文件哈希，不输出私钥、PIN、令牌或服务凭据。

### 8.2 当前阻塞证据

- `windeployqt.exe`：262,144 字节，SHA-256 `889DFACF...8D93D`，PE32+ x64，`NotSigned`。首次直接调用被应用控制拒绝；随后同哈希 Qt SDK/构建树副本在诊断中均 2/2 退出 0，之后实际部署也退出 0，期间未修改策略、白名单、签名或文件哈希。
- 暂存 App：SHA-256 `EFFD7658...E4878F1`，`NotSigned`；Worker：`5C12BB3...FF15260`，`NotSigned`。
- 被拒绝 Qt DLL：`Qt6QmlMeta.dll`，SHA-256 `F35EF4258D2307EBD966A3107CB295CC10287E916B5680714A846811681A665C`，与 `C:\sr\q\qt6112\bin\Qt6QmlMeta.dll` 同哈希，`NotSigned`。Code Integrity 3033/3077 三次指向该 DLL，策略 `VerifiedAndReputableDesktop`，GUID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`，状态 `0xC0E90002`。
- AppLocker EXE/DLL 和 MSI/Script 未记录对应拒绝；当前根因为 Code Integrity/SAC，不是 AppLocker、Qt 版本漂移、x86/x64 混用、缺 DLL 或 `windeployqt` 复制来源错误。

### 8.3 唯一可接受的解除条件

1. 首选：主机策略管理员提供组织管理的非生产开发/CI 签名服务，为 App、Worker、自建 Qt/FFmpeg/OpenCV/KissFFT 运行时及需运行的构建工具签名，并用最小 signer/publisher 补充策略验证。
2. 若组织不能提供开发签名：由管理员创建 ACL 受控、普通构建账号不可改写的专用输出根，只为该根建最小补充策略；不得对当前用户可写仓库建路径规则。
3. 管理员变更后，在无 fallback、无重试美化的干净 Release 输出上执行 `windeployqt` 哈希前检、全部严格测试、安装闭包启动、Worker 启动和 Code Integrity 新事件窗口检查；必须所有程序真实运行且窗口内无 3033/3077 才解除。

## 9. 干净 Windows 验证矩阵

T-038 最终矩阵等最低 Windows 版本确认后冻结；每个承诺版本至少覆盖：

- 无 Qt、FFmpeg、OpenCV、KissFFT 开发环境且 `PATH` 不含开发 SDK 的干净 Windows x64。
- 普通用户安装/启动，已确认 scope 下的管理员场景，无网络首次启动。
- 安装、首启、导入、预览、保存/重开、正式导出、升级、修复、回滚、卸载和重装。
- 磁盘不足、文件被占用、安装中断、旧版项目迁移失败、缓存损坏、Worker 崩溃和无网络。
- 用户项目/设置/自动保存保留边界，缓存/日志可选清理，原始媒体不被改写。
- 逐文件哈希、签名主体、时间戳、依赖闭包、SBOM/notices、系统组件导入和 Code Integrity/AppLocker 事件。
- 验证记录包含 VM/物理机标识、Windows build、安全策略、CPU/GPU/RAM/磁盘、命令、时间、退出码、日志哈希、未覆盖项，不把当前开发机或 Wine 替代已承诺 Windows 矩阵。

## 10. 待确认输入与下一步

| 待确认项 | 确认来源 | 阻塞范围 |
|---|---|---|
| 最低/推荐 Windows 版本与安全策略矩阵 | 用户/产品，技术成员提供证据 | T-038 兼容性承诺 |
| 正式容器、H.264/AAC 后端与格式矩阵 | 用户确认，架构/多媒体/发布提供证据 | 正式导出和 FFmpeg 许可冻结 |
| 安装器、每用户/每机器 scope、升级与自动更新 | 用户/项目计划 | T-037 安装事务实现 |
| 签名主体、证书、时间戳和发布渠道 | 用户/组织安全与发布管理 | 正式签名和可验证发布者身份 |
| 开发/CI WDAC 信任路线 | 主机策略管理员，由用户协调 | 严格 Release、自建 Qt DLL 加载和 T-038 |
| 默认音色/视觉风格及内容权限 | 用户/产品，必要时法务复核 | 内容进入候选 |

T-037 按顺序实现：独立 Release 暂存根 → `windeployqt` dry-run 映射/白名单 → 实际复制 → 非 Qt 运行时闭包 → SBOM/notices → unsigned 安装事务 → 隔离签名接口 → 严格启动和策略取证。T-038 仅在 T-022、T-037 和待确认输入齐备后启动。

## 11. 自查

- 已列出源提交、构建预设、工具链、Qt/非 Qt 依赖、质量与版本冻结输入。
- 已定义应用私有布局、`windeployqt` 受控调用、安装/升级/卸载/回滚与用户数据边界。
- 已定义 Qt/FFmpeg/OpenCV/KissFFT/音色/模型/安装器的 SBOM 字段、当前依据和发布缺口，不作法律结论。
- 已把签名凭据与普通构建隔离，将未签名候选与生产签名产物区分，未访问或使用任何凭据。
- 已定位当前阻塞为 `Qt6QmlMeta.dll` 的 Code Integrity/SAC 信任失败，并限定为组织管理 signer/publisher 或 ACL 受控专用根的最小策略路线；没有关闭/绕过 WDAC/SAC，没有创建宽泛路径或易变 hash 白名单。
