# Windows x64 Qt 6.11.2 源码 SDK 构建与验收记录

- 项目：space-rhythm
- 成果 ID：A-009
- 负责人：build-engineer-windows-qt-01
- 关联任务：T-012
- 版本：0.1
- 更新日期：2026-09-08
- 状态：draft
- 适用范围：按 D-006、D-007、D-008 在当前 Windows 主机建立 MSVC 2022 x64、LGPLv3/shared、Qt 6.11.2 精确版本 SDK，覆盖源码校验、Release/Debug 构建安装、ABI/CRT 检查、QML/Qt Quick/Multimedia 消费端冒烟、部署暂存和证据清单；不创建产品工程骨架、安装器、签名或发布包。
- 来源及输入版本：T-011 completed、T-012；D-002、D-004、D-005、D-006、D-007、D-008 confirmed；[A-004 0.5](A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.3](A-005-mvp-technology-stack-proposal.md)、[A-007 0.1](A-007-four-engineer-execution-plan.md)、[A-008 0.3](A-008-windows-build-environment-audit-and-execution-plan.md)；用户于 2026-09-08 明确要求“启动 T-012”。
- 批准依据：尚无。D-006～D-008 是本次构建输入，不等同于批准本成果或发布。
- 版本记录：2026-09-08，0.1，完成 Qt 6.11.2 Windows x64 shared SDK 的实际源码构建、双配置安装、消费端冒烟、部署和证据固化。

## 1. 结论

T-012 的实际构建与负责人自查已完成，结果为通过：

| 验收项 | 结果 | 证据摘要 |
|---|---|---|
| 官方源码固定 | 通过 | `qt-everywhere-src-6.11.2.tar.xz` 为 1,019,661,552 字节，SHA-256 与 D-008 完全一致 |
| Windows x64 工具链 | 通过 | MSVC 19.44.35228.0、Windows SDK 10.0.26100.0，host/target 均为 x64 |
| 路径隔离 | 通过 | 源码、shadow build、SDK 安装、冒烟构建/暂存和证据目录互不重叠，均使用 `C:\sr` 短路径 |
| shared、Release/Debug | 通过 | `BUILD_SHARED_LIBS=yes`、`debug_and_release=ON`、`developer_build=OFF`、`static_runtime=OFF` |
| 模块与媒体后端 | 通过 | 仅配置 `qtbase,qtdeclarative,qtshadertools,qtmultimedia` 顶层源码模块；WASAPI 和 native Windows backend 为 yes，FFmpeg 为 no |
| 构建与安装 | 通过 | Release、Debug 的最终构建与安装命令均退出 0；最终日志未检出 fatal/error/FAILED 模式 |
| x64 ABI/CRT | 通过 | PE 为 `8664 machine (x64)`；Release 使用无 `d` 后缀 Qt DLL 和 Release CRT，Debug 使用 `*d.dll` 与 Debug CRT |
| QML/Multimedia 冒烟 | 通过 | 两套配置均报告 Qt 6.11.2、`x86_64-little_endian-llp64`，完成 QML load、`MediaPlayer` 实例化和事件循环 |
| 部署暂存 | 通过 | `windeployqt` 生成两套隔离暂存，并显式包含 `qoffscreen`；Release 与 Debug 都从暂存目录运行成功 |
| 清单与 SBOM | 通过 | SDK 二进制、PDB 和部署文件均生成逐文件 SHA-256；四个源码模块均生成 SPDX 2.3 文本 SBOM |

可复核摘要见 [T-012 verification summary](../evidence/T-012/verification-summary.md)，完整机器日志位于 `C:\sr\evidence\T-012`。

## 2. 固定工具链元组

| 项目 | 实际值 |
|---|---|
| 操作系统 | Windows 11 x64，当前主机版本 10.0.26200 |
| Visual Studio | Visual Studio Build Tools 2022 17.14.39，installation version 17.14.37614.0，完整且无需重启 |
| MSVC | v143 tool directory 14.44.35207；`cl.exe` 19.44.35228.0；`link.exe` 14.44.35228.0 |
| Windows SDK | 10.0.26100.0 |
| CMake | 3.31.6-msvc6，来自 Build Tools |
| Ninja | 1.12.1，来自 Build Tools |
| Python | 3.13.15 x64 WindowsPE，NuGet build distribution |
| Git | 2.55.0.windows.4 |
| 生成器 | Ninja Multi-Config，`Release;Debug` |
| C/C++ ABI | MSVC x64；冒烟工程为 C++20；Qt、应用和同进程原生库不混用 MinGW |

Build Tools 安装包含 `Microsoft.VisualStudio.Component.VC.Tools.x86.x64`、`Microsoft.VisualStudio.Component.Windows11SDK.26100` 和 `Microsoft.VisualStudio.Component.VC.CMake.Project`。同时安装英语编译器资源并固定 `VSLANG=1033`，保证 CMake/Ninja 对 `/showIncludes` 的解析不受中文本地化前缀影响。

## 3. 输入、路径和来源固定

| 用途 | 路径或来源 |
|---|---|
| Qt 源码归档 | `C:\sr\downloads\qt-everywhere-src-6.11.2.tar.xz` |
| Qt 官方入口 | `https://download.qt.io/official_releases/qt/6.11/6.11.2/single/qt-everywhere-src-6.11.2.tar.xz`；本次由官方 mirror list 解析到 JAIST HTTPS 镜像 |
| Qt 源码 | `C:\sr\s\qt6112` |
| Qt shadow build | `C:\sr\b\qt6112` |
| Qt SDK 安装前缀 | `C:\sr\q\qt6112` |
| 消费端 shadow build | `C:\sr\b\qt6112-smoke` |
| 消费端部署暂存 | `C:\sr\stage\qt6112-smoke\Release`、`C:\sr\stage\qt6112-smoke\Debug` |
| 完整日志和清单 | `C:\sr\evidence\T-012` |

Qt 源码实测 SHA-256 为 `6DCFBCA271D76A6502741A2C0DC6FC98EF7DD0B7B4CFD0ABCEBB285A86A26F33`。下载文件被保留，后续重建必须先重复大小和哈希校验；入口脚本会在每个阶段执行该校验。

## 4. 配置基线

等价的完整配置参数如下；`configure-arguments.txt` 保留逐参数原值：

```text
configure.bat
  -prefix C:\sr\q\qt6112
  -opensource -confirm-license
  -shared -debug-and-release
  -submodules qtbase,qtdeclarative,qtshadertools,qtmultimedia
  -skip qtimageformats,qtlanguageserver,qtsvg,qtquick3d,qtquicktimeline
  -nomake examples -nomake tests
  -no-feature-ffmpeg
  -- -DQT_INSTALL_CONFIG_INFO_FILES=ON
```

配置结果为 `win32-msvc (x86_64)`、`debug and release`、shared libraries yes。未使用 `-developer-build`；缓存明确记录 `FEATURE_developer_build=OFF` 和 `FEATURE_static_runtime=OFF`。安装前缀保留 `config.opt` 与 `config.summary`。

模块白名单约束的是 Qt 顶层源码仓库。`qtbase` 和 `qtdeclarative` 内部按默认特性生成必要工具、QML 模块和平台插件；`qtmultimedia` 内建的 Windows/WASAPI 路径被保留，FFmpeg 特性显式关闭。未引入 Qt GPL-only 顶层模块。该工程控制不构成法律意见。

## 5. SDK 产物、符号和运行库

安装前缀同时存在 `Qt6Core.dll`/`Qt6Cored.dll`、`Qt6Gui.dll`/`Qt6Guid.dll`、`Qt6Qml.dll`/`Qt6Qmld.dll`、`Qt6Quick.dll`/`Qt6Quickd.dll`、`Qt6Multimedia.dll`/`Qt6Multimediad.dll`，以及 Release/Debug 的 `qwindows` 和 `windowsmediaplugin`。

- `qt-sdk-binaries.csv`：497 项 `.dll/.exe/.lib`，合计 911,949,093 字节。
- `qt-sdk-symbols.csv`：170 项 `.pdb`，合计 1,954,070,528 字节。
- Release `Qt6Core.dll` 依赖 `MSVCP140.dll`、`MSVCP140_1.dll`、`VCRUNTIME140.dll`、`VCRUNTIME140_1.dll` 和 UCRT API set。
- Debug `Qt6Cored.dll` 依赖 `MSVCP140D.dll`、`MSVCP140_1D.dll`、`VCRUNTIME140D.dll`、`VCRUNTIME140_1D.dll`、`ucrtbased.dll`。

这证明两套配置使用匹配的动态 CRT；Debug CRT 仅用于开发机验证，不是可分发运行库。

## 6. 独立消费、部署和运行验收

仓库中的 [smoke CMake project](../workspace/tooling/qt/smoke/CMakeLists.txt) 使用 `find_package(Qt6 6.11.2 EXACT)`，要求 Core、Gui、Qml、Quick、Multimedia，固定 C++20 和 `MultiThreadedDLL`/`MultiThreadedDebugDLL`。手写 `main.cpp` 使用 `/W4 /WX`；Qt 生成的 QML cache 源码使用 `/W4`，避免把上游生成代码警告误判为本项目失败。

`windeployqt` 以 `--release`/`--debug`、`--compiler-runtime`、`--qmldir` 和 `--include-plugins qoffscreen` 生成两套暂存。两次运行均输出：

```text
SPACE_RHYTHM_QT_SMOKE qt=6.11.2 arch=x86_64 buildAbi=x86_64-little_endian-llp64
SPACE_RHYTHM_QML_SMOKE multimediaReady=true
SPACE_RHYTHM_CHECKPOINT event-loop-ok
```

Release 冒烟程序 SHA-256 为 `B3F6BA80E799B8A11F536F893C9F9BC392BC40E56A7F300BD9B854AECFCA7C13`；Debug 为 `3943E8E2860ECE3ED4D706E4D9410A2C26EF697C2A9E5D95AE46749BB2101AAA`。`dumpbin` 对两者均确认 PE32+ x64，并分别显示 Release/Debug Qt DLL 与 CRT 依赖。

Debug 暂存仅是开发证据：`windeployqt --debug --compiler-runtime` 会复制不可再分发的 Debug CRT，Debug UCRT 仍来自初始化后的 Windows SDK 环境。T-012 不把该目录当产品发布包；Release 打包策略、VC Runtime 安装方式、签名和安装器另行决定。

## 7. 构建过程中收敛的问题

1. 中文 MSVC `/showIncludes` 前缀不能被当前 CMake/Ninja 稳定识别。安装 Build Tools 英语资源并固定 `VSLANG=1033` 后，CMake 缓存前缀为 `Note: including file: `，干净重配后解决。
2. Debug host tools 位于 `qtbase\bin\Debug`，运行时需要父级 `qtbase\bin` 的 Debug Qt DLL 和 MSVC `debug_nonredist` CRT。入口脚本自动发现两处路径并加入子进程 `PATH`，Qt 自带的 `rcc/uic/qmake/qtpaths/qdbus` 运行测试随后通过。
3. 初版消费端把 `/WX` 施加到 Qt 自动生成的 QML cache 源码，引发上游 C4702。最终将 `/WX` 限定到手写 `main.cpp`，保留生成代码 `/W4`。
4. `windeployqt` 默认只部署检测到的 `qwindows`，而 headless 验收明确使用 `offscreen`。最终增加 `--include-plugins qoffscreen`，两套暂存独立运行通过。

这些修复已进入可重复脚本和说明；最终证据目录中的同名 build/install/smoke 日志均是最后一次成功运行。

## 8. 可重复执行入口

入口为 [Invoke-Qt6112Build.ps1](../workspace/tooling/qt/Invoke-Qt6112Build.ps1)，说明见 [tooling/qt/README.md](../workspace/tooling/qt/README.md)。完整重建命令为：

```powershell
pwsh -File .\tooling\qt\Invoke-Qt6112Build.ps1 -Stage All -CleanBuild -Parallel 10
```

也可按 `Configure`、`Build`、`Install`、`Smoke`、`Manifest` 分阶段执行。脚本只允许清理受校验的 `C:\sr\b`/`C:\sr` 子路径；原生 MSVC 探针只在 `Configure/All` 运行；所有阶段都校验 Qt 归档大小、哈希、工具链组件和 x64 目标。

## 9. LGPLv3 工程控制与边界

- Qt 以 shared/dynamic 方式构建，未启用 static runtime，不把 Qt 合并进应用二进制。
- 原始 Qt 6.11.2 源码归档、精确来源、大小与哈希被保留；源码树中的 `LICENSES/LGPL-3.0-only.txt` SHA-256 为 `DA7EABB7BAFDF7D3AE5E9F223AA5BDC1EECE45AC569DC21B3B037520B4464768`。
- 配置参数、模块白名单、完整二进制/PDB 清单和模块级 SPDX SBOM 已保留，便于后续发布材料核对和用户替换 Qt 动态库。
- 最终产品仍需提供适用许可证文本、版权/第三方 notices、Qt 源码或有效书面提供方式，并验证替换动态库的实际流程；这些发布动作不属于 T-012。
- 当前 SDK 使用 Windows 系统 ICU；Release 部署时需在产品基准 Windows 版本上验证该系统依赖。最低 Windows 版本仍未确认，因此本任务只证明当前开发机可构建运行，不宣称兼容矩阵已通过。
- Qt 配置摘要显示 SBOM generation 与 SPDX 2.3 为可用并实际生成四份 `.spdx`；可选的 Python SPDX 工具未找到，因此没有把可选 JSON 转换作为完成条件。

## 10. 后续边界

T-013 仍受 D-003 构建组合确认约束。本成果只提供已验证的 Qt SDK 与复现入口，不自行建立产品级 CMake Presets、vcpkg baseline、CI runner、安装器或签名流程。最低 Windows 版本、FFmpeg 版本/H.264 后端和发布运行库策略仍由后续决定与对应负责人处理。

## 11. 自查

- 逐项核对 T-012 完成条件：源码/构建/安装隔离、来源与哈希、工具版本、完整配置、模块白名单、双配置、符号/运行库清单、x64/CRT、QML/Multimedia 编译运行和部署暂存均有实际证据。
- 最终 configure、Release/Debug build、Release/Debug install、Release/Debug smoke 和 manifest 命令退出码均为 0；最终日志扫描未检出构建失败模式。
- 保留用户所有 `package/` 内容，未修改、执行或纳入本任务产物。
- 成果保持 draft；任务完成不自动批准成果，也不自动授权 T-013 或发布。
