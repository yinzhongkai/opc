# Windows 构建环境审计与执行方案

- 项目：space-rhythm
- 成果 ID：A-008
- 负责人：build-engineer-windows-qt-01
- 关联任务：T-011
- 版本：0.3
- 更新日期：2026-09-08
- 状态：draft
- 适用范围：当前 Windows 主机的只读构建环境盘点、已确认的 MSVC 2022/LGPLv3/Qt 6.11.2 基线，以及 T-012/T-013 的执行门槛和实施顺序；不安装软件、不下载或编译 Qt、不替用户确认最低 Windows 版本或 D-003 构建组合。
- 来源及输入版本：D-002、D-004、D-005、D-006、D-007、D-008 confirmed，D-003 proposed；[A-004 0.5](A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.3](A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-01](A-006-domain-work-packages.md)、[A-007 0.1](A-007-four-engineer-execution-plan.md)；2026-09-08 当前主机只读命令输出；Qt 与 Microsoft 官方资料。
- 批准依据：D-006 确认 MSVC 2022 Build Tools x64 路线，D-007 确认 Qt LGPLv3 路径，D-008 确认 Qt 6.11.2 精确版本。最低 Windows 版本及 D-003 构建组合仍由有权确认人决定。
- 版本记录：
  - 2026-09-08，0.1，完成环境盘点、MSVC/MinGW 对比、最小安装清单、验证命令和 T-012/T-013 执行方案。
  - 2026-09-08，0.2，纳入 D-006/D-007 确认结果，增加 Qt 6.11.2 精确版本建议、LGPLv3 模块/链接门禁和 6.11.3 升级策略；替代 0.1。
  - 2026-09-08，0.3，纳入 D-008，把 Qt 6.11.2 从建议更新为已确认精确版本，并关闭 T-012 的版本决定门禁；替代 0.2。

## 1. 结论

当前主机硬件和磁盘容量足以进入 Windows x64 Qt 源码构建准备，但正式构建工具链尚未就绪。未发现 Visual Studio/Build Tools、MSVC、Windows SDK、MinGW-w64、CMake、Ninja、Qt 或 vcpkg；`python.exe` 只是 Windows Store 的零字节执行别名，调用会提示安装 Python，不能作为构建依赖。Git 2.55.0.windows.4 可直接使用。

**D-006 已确认采用 MSVC 2022 Build Tools x64 路线。** Qt、应用和同进程原生依赖统一使用 MSVC v143 兼容的 x64 ABI 与一致 CRT；MinGW 不再是第一阶段项目基线。MSVC 2022 和 MinGW-w64 13.1 都在 Qt 6.11 的 Windows x86_64 支持矩阵内，但 vcpkg 的 Windows MSVC triplet 是主路径，MinGW triplet 属于未纳入 vcpkg 仓库 CI 的 community 路径；结合本项目的 Qt、FFmpeg/OpenCV、Windows CI、PDB/运行库和后续部署需求，确认结果与审计推荐一致。

**D-007 已确认采用 Qt LGPLv3、shared/dynamic linking，D-008 已确认把 Qt 精确锁定为 6.11.2。** 它是截至 2026-09-08 已正式发布的最新 Qt 6.11 patch，标准支持期到 2027-03-17；计划中的 6.11.3 尚未发布，不作为当前可复现基线。若 6.11.3 后续正式发布，则在新决定、干净重建和冒烟/回归通过后作为受控 patch 升级候选，不自动漂移版本。T-012 的技术决定门禁已经关闭，但本轮确认不自动授权下载安装或修改系统；T-013 的正式依赖基线还需 D-003 构建组合确认。

## 2. 当前主机事实

| 类别 | 只读探测结果 | 判定 |
|---|---|---|
| 操作系统 | Microsoft Windows 11 家庭版中文版，版本 10.0.26200，64 位 | 满足当前 Qt 6.11 Windows 11 x86_64 平台类别；最低目标 Windows 版本仍未决定 |
| CPU | AMD Ryzen AI 7 H 350，64 位地址宽度，16 个逻辑处理器 | 可作为当前开发机，不代表已确认的产品基准硬件 |
| 内存 | 31.12 GiB | 可开展 Qt 源码构建；并行度须在实际构建中按峰值内存调整 |
| 系统盘 | C: 总计 924.45 GiB，空闲 594.09 GiB | 当前容量充足；Qt 源码、构建、安装与缓存仍须分目录计量 |
| 长路径策略 | `LongPathsEnabled = 0` | 风险项；优先使用短路径，暂不以修改系统策略作为前置 |
| Visual Studio / MSVC | `cl`、`link`、`msbuild`、`vswhere` 均不可发现；标准 VS 2022 目录和卸载登记无匹配 | 未发现 |
| Windows SDK | 标准 Windows Kits 目录及相关环境变量不可发现 | 未发现 |
| MinGW-w64 / MSYS2 | `gcc`、`g++`、`mingw32-make` 不可发现；常见 MSYS2/MinGW 目录和卸载登记无匹配 | 未发现 |
| CMake / CTest | PATH、标准安装目录及卸载登记无匹配 | 未发现 |
| Ninja | PATH、标准安装目录及卸载登记无匹配 | 未发现 |
| Python | PATH 解析到 WindowsApps 零字节别名，执行失败；Codex 私有运行时另有 Python 3.12.14，但不在普通构建环境中 | 系统构建环境不可用；私有运行时不得作为项目基线 |
| Git | `C:\Program Files\Git\cmd\git.exe`，2.55.0.windows.4 | 可用 |
| Qt / vcpkg | `C:\Qt`、`C:\vcpkg`、`C:\src\vcpkg` 及卸载登记无匹配，相关环境变量为空 | 未发现 |
| 构建环境变量 | `VSCMD_ARG_TGT_ARCH`、`VisualStudioVersion`、`WindowsSdkDir`、`WindowsSDKVersion`、`VCPKG_ROOT`、`Qt6_DIR`、`QTDIR`、`CMAKE_PREFIX_PATH`、`INCLUDE`、`LIB`、`LIBPATH` 均为空 | 当前 shell 未初始化任何 C/C++ 工具链 |
| 仓库构建骨架 | 尚无 `CMakeLists.txt`、`CMakePresets.json`、`vcpkg.json`、`src/`、`tests/` 或 CI 配置 | T-013 尚未开始，符合任务记录 |

“未发现”限定为 PATH、标准目录、卸载登记、相关注册表和项目约定环境变量均未发现，不宣称已经搜索整块磁盘的所有自定义目录。

## 3. 审计方法与复核入口

本轮只执行了读取操作：

- 通过 WMI/CIM 读取 Windows、CPU、内存和系统盘信息。
- 用 `Get-Command` 检查编译器、链接器、构建工具和版本控制入口。
- 检查 Visual Studio Installer 的 `vswhere.exe`、Visual Studio 2022、Windows Kits、CMake、Ninja、MSYS2、Qt 和 vcpkg 常见目录。
- 检查 32/64 位系统及当前用户卸载登记，不读取或输出许可证密钥、令牌等敏感信息。
- 检查构建相关进程环境变量和 Windows 长路径策略。
- 执行 `git --version`；对 WindowsApps `python.exe --version` 的失败和 Codex 私有 Python 的实际版本分别记录。
- 扫描仓库是否已有 CMake、依赖、源码、测试和 CI 骨架；保留未跟踪的 `package/`，未对其执行或修改。

## 4. MSVC 与 MinGW 路线比较

Qt 6.11.2 当前官方文档将 Windows 11 x86_64 的受支持编译器列为 MSVC 2022 和 MinGW-w64 13.1。Qt 源码构建还要求 CMake 3.22+、Ninja 和 Python 3 可用；MSVC 路线要求 Windows SDK 10.0.17763 或更高版本。来源：[Qt Windows 源码构建](https://doc.qt.io/qt-6/windows-building.html)、[Qt 支持平台](https://doc.qt.io/qt-6/supported-platforms.html)。

| 维度 | MSVC 2022 Build Tools | MinGW-w64 13.1 |
|---|---|---|
| Qt 官方支持 | Windows 11 x86_64 受支持 | Windows 11 x86_64 受支持 |
| ABI 纪律 | Qt、应用和同进程依赖统一使用 MSVC/v143 与同一 CRT 模型 | Qt、应用和全部原生依赖必须统一使用同一 MinGW；不能混入 MSVC C++ ABI |
| vcpkg | `x64-windows` 等为 Windows MSVC 主路径，可锁定 toolset 和 CRT | `x64-mingw-*` 属 community triplet，官方说明未纳入 vcpkg 仓库 CI，port 更新可能回归 |
| 调试与诊断 | 原生 PDB、Visual Studio/WinDbg 生态和 Windows SDK 工具链衔接直接 | 主要使用 GDB/MSYS2 工具链；Windows 原生崩溃诊断和第三方二进制接入需要更多约束 |
| CI | Visual Studio Build Tools 可通过开发者命令环境初始化，适合 Windows runner | 需维护一致的 MSYS2/MinGW 子系统、PATH 和 host/target triplet，环境混用风险更高 |
| 第三方依赖 | FFmpeg/OpenCV 等可统一从源码或 vcpkg 构建，Windows 现成工程经验较多 | 同样可源码构建，但所有库都必须保持 MinGW ABI，vcpkg 支持风险更高 |
| 安装体量 | 预计高于最小 MinGW 工具链；准确体量应以 Visual Studio Installer 安装前计算值为准 | 通常较小，但仍需 MSYS2/工具链及相同的 CMake/Ninja/Python/源码依赖；本轮未下载，未测量实际体量 |
| 许可证输入 | 需接受 Microsoft Build Tools 条款，发布时按官方方式处理 VC Runtime | 需核对 MinGW/MSYS2 组件许可证和运行时分发；Qt/FFmpeg/应用许可证问题仍独立存在 |

Microsoft 文档说明 Visual Studio 的 C++ workload 提供命令行编译、链接和匹配库，并建议通过开发者命令文件设置架构相关环境，而不是手工拼接变量：[Microsoft C++ Build Tools](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170)。vcpkg 对 MSVC 与 MinGW 的差异见 [Windows with MSVC](https://learn.microsoft.com/en-us/vcpkg/users/platforms/windows) 和 [Mingw-w64](https://learn.microsoft.com/en-us/vcpkg/users/platforms/mingw)。

## 5. 推荐的最小安装清单

以下是 D-006 已确认采用 MSVC 后的安装输入，不是本轮安装授权：

1. Microsoft Visual Studio 2022 Build Tools，选择“使用 C++ 的桌面开发”。
2. MSVC v143 x64/x86 build tools；实际安装后记录完整工具集版本，不仅记录“2022”。
3. 一个受 Qt 目标版本支持的 Windows SDK，且版本不低于 10.0.17763；实际版本进入工具链元组。
4. CMake 3.22 或更高版本、Ninja、可实际执行的 Python 3；分别固定实际版本和来源。
5. Git 沿用当前 2.55.0.windows.4，后续只在确有兼容或安全需要时升级。
6. Qt 源码不随工具链一起隐式安装；LGPLv3 和 Qt 6.11.2 已确认，实际执行阶段从 Qt 官方来源获取并校验 D-008 登记的文件大小与 SHA-256。
7. vcpkg 仅在 D-003 的依赖方案确认后引入 manifest 与 baseline，不在 T-012 中代管 Qt SDK。

MinGW 路线已不再是第一阶段基线；后续若要改变编译器，必须建立替代决定并从 Qt、应用到所有同进程原生依赖整体重建，不得混用既有 MSVC C++ 库。

## 6. 工具链安装后的验证命令

MSVC 路线至少保留以下输出：

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json
```

在 `VsDevCmd.bat -arch=x64 -host_arch=x64` 初始化的 shell 中执行：

```text
where cl.exe
cl.exe /Bv
where link.exe
where rc.exe
cmake.exe --version
ninja.exe --version
python.exe --version
git.exe --version
```

MinGW 备选路线至少执行：

```text
where g++.exe
g++.exe --version
g++.exe -dumpmachine
cmake.exe --version
ninja.exe --version
python.exe --version
git.exe --version
```

两条路线都必须进一步以最小 x64 程序验证编译器、链接器、运行库和架构；T-012/T-013 再保存 Qt configure summary、CMake cache 摘要、产物清单和哈希，不能只以 `PATH` 可见作为通过证据。

## 7. T-012/T-013 执行方案

### 阶段 A：决策门禁

开始任何下载或安装前，核对以下门禁：

1. D-006 已确认：MSVC 2022 Build Tools x64；安装后仍须记录 MSVC、Windows SDK、CMake、Ninja 和 Python 的完整版本。
2. D-007/D-008 已确认：Qt LGPLv3、shared/dynamic linking、Qt 6.11.2。官方源码包 `qt-everywhere-src-6.11.2.tar.xz` 大小为 1,019,661,552 字节，SHA-256 为 `6dcfbca271d76a6502741a2c0dc6fc98ef7dd0b7b4cfd0abcebb285a86a26f33`，实际下载后必须重新校验。来源：[Qt 6.11.2 发布说明](https://www.qt.io/blog/qt-6.11.2-released)、[Qt 6.11.2 官方源码信息](https://download.qt.io/official_releases/qt/6.11/6.11.2/single/qt-everywhere-src-6.11.2.tar.xz.mirrorlist)、[Qt 许可概览](https://doc.qt.io/qt-6/licensing.html)。
3. D-003 中与构建直接相关的 C++20/C17、CMake Presets、Ninja、vcpkg manifest 和测试入口组合。
4. 最低目标 Windows 版本；当前开发机信息不能替代产品兼容范围。
5. CI 采用当前 GitHub 远端对应的 GitHub Actions，或由用户指定其他 Windows runner。
6. 产品源码根目录。建议在当前单项目克隆根目录使用 `src/`、`tests/`、`cmake/`、`tooling/`，避免把产品脚本混入现有框架 `scripts/`。

### Qt 版本基线与升级策略

- 已确认 `Qt 6.11.2`：它是当前已发布的最新 6.11 patch，Qt 官方将 6.11 的标准支持期列到 2027-03-17；源码文件名、大小和 SHA-256 已进入 D-008，获取后仍须实测校验。
- 不等待 `Qt 6.11.3`：Qt 6.11 发布计划当前把它列为 2026-09-15 的目标版本，属于未来计划而非可用输入。正式发布后再作为受控补丁升级，不能让流水线自动跟随“最新”。来源：[Qt 6.11 发布计划](https://wiki.qt.io/Qt_6.11_Release)、[Qt 6.11 官方下载目录](https://download.qt.io/official_releases/qt/6.11/)。
- 不以 `Qt 6.8 LTS` 作为 LGPLv3 持续维护基线：Qt 官方说明 LTS 维护版本面向商业许可证客户；开源/LGPLv3 路线不能把商业 LTS 补丁持续可得性当作保障。来源：[Qt Releases](https://doc.qt.io/qt-6/qt-releases.html)、[Qt LTS](https://www.qt.io/development/qt-framework/qt-lts)。
- 初始模块白名单保持 `qtbase`、`qtdeclarative`、`qtshadertools`、`qtmultimedia` 及其必要依赖。构建前按实际源码许可证清单复核；Qt 官方列出的 GPL-only 模块默认禁止进入基线。该项为工程控制，不替代正式法律意见。

### 阶段 B：T-012 可复现 Qt SDK

1. 经用户授权后安装并验证固定工具链，把 OS、编译器、Windows SDK、CMake、Ninja、Python 和 Git 版本写入工具链清单。
2. 使用短且无空格的隔离路径，例如 `C:\sr\qt-src\<version>`、`C:\sr\qt-build\<version>-<toolchain>-x64`、`C:\sr\qt-sdk\<version>-<toolchain>-x64`；不依赖当前关闭的长路径策略。
3. 从 Qt 官方来源取得确切源码，校验发布哈希；保存来源、文件大小、SHA-256 和获取日期。
4. 使用 shadow build 和显式 `-prefix`；shared build，不使用 `-developer-build`。候选模块白名单为 `qtbase`、`qtdeclarative`、`qtshadertools`、`qtmultimedia` 及必要依赖，最终按消费方需求复核。
5. 保存完整 `configure.bat` 参数、configure summary/options、生成器、Debug/Release 策略和第三方库来源。Qt 6.9+ 可通过 `QT_INSTALL_CONFIG_INFO_FILES=ON` 把配置摘要安装进 SDK；配置和安装规则见 [Qt Configure Options](https://doc.qt.io/qt-6/configure-options.html)。
6. 构建、安装并生成 SDK 文件/符号/运行库清单及哈希；任何重新配置使用干净构建目录或可核对的 `-redo` 流程。
7. 编译运行最小 Qt Quick/QML x64 程序，检查 QML 模块、平台插件、Debug/Release 运行库和部署暂存。使用 `windeployqt --qmldir` 收集依赖并核对额外第三方 DLL；依据见 [Qt Windows Deployment](https://doc.qt.io/qt-6/windows-deployment.html)。

### 阶段 C：T-013 CMake/CI 工程骨架

1. D-003/D-006 确认后先建立无 Qt 的纯 C++ `domain`、媒体接口和测试目标；保持 domain 不依赖 QML、Qt Quick 或 FFmpeg 类型。
2. 建立版本化 `CMakePresets.json`，至少包含开发与 CI 的 configure/build/test/install 入口；本机路径只进不提交的 `CMakeUserPresets.json`。
3. 在配置阶段校验 Windows、x64、编译器 ID/版本、Windows SDK、C/C++ 标准和运行库模型；x86、ARM64、ARM64EC 或 ABI 混用立即失败。
4. D-003 确认后建立 vcpkg manifest/baseline 和明确的 x64 triplet；Qt SDK 保持独立版本化，不由 vcpkg 隐式获取。
5. T-012 SDK 就绪后补齐 `app`、`worker`、Qt/QML smoke 目标；只建立可链接骨架，不实现业务 UI、媒体算法或领域功能。
6. GitHub Actions Windows runner 复用同一 presets，完成干净配置、编译、headless 单测、最小 QML 冒烟、安装暂存和运行时依赖检查；缓存键覆盖工具链元组、Qt SDK 清单、vcpkg baseline 和构建选项。
7. 保存命令、日志、失败首因、测试结果、产物与运行时清单；缓存命中不作为正确性证据，并保留无缓存构建路径。

## 8. 停止条件与风险控制

- MSVC 2022 Build Tools 尚未实际安装或版本验证：不把工具链标记为就绪，不生成虚假的可复现证据。
- Qt 6.11.2 源码实际文件大小或 SHA-256 与 D-008 不一致：停止使用该文件，不通过未知镜像或关闭校验绕过。
- 发现 GPL-only Qt 模块、静态链接需求或无法满足 LGPLv3 动态库替换/材料要求：停止集成，先进行许可证影响复核并取得新决定。
- D-003 构建组合未确认：不提交 vcpkg baseline 或最终测试框架依赖。
- 发现 MSVC/MinGW、x64/其他架构、Debug/Release CRT 或 Qt/第三方库 ABI 混用：停止集成并保留首个有效错误。
- 磁盘不足、路径过长、来源哈希不符或许可证来源不清：停止构建，不用关闭校验或切换未知镜像绕过。
- CI 凭据、签名证书、生产发布和安装器不属于 T-012/T-013 授权范围。

## 9. 自查

- 已覆盖 T-011 要求的 OS、CPU、内存、磁盘、MSVC/SDK、MinGW、CMake、Ninja、Python、Git、环境变量、Qt/vcpkg 与仓库骨架。
- 已将“未发现”“仅执行别名”“私有运行时”和“可用”分开，没有把 Codex 私有 Python 计入项目环境。
- MSVC/MinGW 比较同时覆盖 Qt 支持、ABI、依赖、调试/CI、安装体量和许可证输入，并用官方资料支撑关键判断。
- 已按用户明确选择把 D-006 更新为 MSVC 2022 confirmed，建立 D-007 LGPLv3 confirmed，并建立 D-008 把 Qt 6.11.2 确认为精确版本。
- 未执行下载、安装、Qt 构建或系统配置变更；T-012/T-013 的门禁、产出、验收和停止条件均已同步。
