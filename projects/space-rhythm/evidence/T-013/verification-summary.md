# T-013 可复核摘要

- 负责人：build-engineer-windows-qt-01
- 采集日期：2026-09-09（Asia/Shanghai）
- 完整机器证据根：`out/evidence/T-013`
- 结论：Windows x64 的应用、Worker、核心库、媒体适配和测试工程骨架已完成；Debug、Release、CI 三个预设均配置成功、完成 67/67 编译并通过 7/7 CTest，安装闭包、x64 PE 和 CRT/Qt 运行时依赖检查已生成证据。

## 固定输入

| 项目 | 实际值 |
|---|---|
| Visual Studio Build Tools | 2022 17.14.39，MSVC `cl.exe` 19.44.35228.0，工具目录 14.44.35207 |
| Windows SDK | 10.0.26100.0 |
| CMake / Ninja | 3.31.6-msvc6 / 1.12.1 |
| Qt SDK | 复用 `C:\sr\q\qt6112` 的 Qt 6.11.2 shared Release+Debug SDK；未下载或重新构建 Qt |
| vcpkg | tag `2026.07.29`，tool 2026-07-27，baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d` |
| 目标 ABI | Windows x64、MSVC 19.44、C17/C++20、动态 CRT（Release/RelWithDebInfo `/MD`，Debug `/MDd`） |

入口脚本在配置前核对以下既有 Qt SDK 文件：

| 文件 | 字节 | SHA-256 |
|---|---:|---|
| `bin/Qt6Core.dll` | 10,352,128 | `94E697C5C7B861E1F9072CB2FB251D3C807ECA053D7EE0F1B4A3BA08023AC1AC` |
| `bin/Qt6Cored.dll` | 23,238,656 | `647A6565A3F4B7DAC0410213471C0EFE2F24FE1BBBC513CC589570BEE6254E79` |
| `config.summary` | 12,775 | `4BCBDAA6DCBCB98BF2F44F700B77FB35D8EE2982996E1B2051C278C2355F49A3` |

## 工程与依赖结果

- `space_rhythm_core`：不依赖 Qt Quick/FFmpeg 的纯 C++ 静态核心目标；当前只含链接锚点和构建契约常量。
- `space_rhythm_media_adapter`：独立静态适配边界，当前只含链接锚点，不包含解码、CV 或 DSP 实现。
- `space_rhythm_worker`：Qt Core/Network 控制台进程空壳，提供 `--smoke`。
- `space_rhythm_app`：最小 Qt Quick/QML 应用空壳，提供 offscreen `--smoke`。
- `space_rhythm_core_tests`、Qt Test、Qt Quick Test 及进程冒烟目标由 CTest 统一调度。

默认 vcpkg manifest 的实际安装结果为 `gtest:x64-windows-space-rhythm@1.17.0#3`，另有 vcpkg 构建辅助端口 `vcpkg-cmake@2024-04-23` 和 `vcpkg-cmake-config@2026-07-21`。默认配置不拉取媒体或算法库；可选 feature 固定未来接入入口：

| feature | baseline 端口版本 | 本任务边界 |
|---|---|---|
| `media` | FFmpeg 8.1.2#3 | 关闭默认 feature，仅列出 avcodec/avformat/swresample/swscale/version3；不启用 GPL/nonfree，不实现媒体逻辑 |
| `video-analysis` | OpenCV 4.12.0#7 | 关闭默认 feature，仅列出 intrinsics/thread；不启用 DNN、Qt 或 OpenCV FFmpeg 集成 |
| `audio-analysis` | KissFFT 131.2.0 | 只固定 FFT 依赖入口，不实现算法 |

GoogleTest 的常规 manifest 安装已在固定 baseline 上完整成功一次。最终本机全量复核为规避后续随机阻断 vcpkg 已校验辅助程序，显式使用 `-UseExistingDependencies` 复用同一安装树；正式 CI 不使用该恢复开关。

## 自动测试清单

1. `BuildSkeleton.LinksCoreAndMediaAdapterTargets`：GoogleTest 链接核心与媒体适配目标。
2. `qt.core_smoke`：Qt Test 核对 Qt 6.11.2、x86_64 和 64 位指针。
3. `qml.quick_smoke`：优先执行生成的 Qt Quick Test 目标并创建最小 QML Item。
4. `app.qml_smoke`：应用加载 `SpaceRhythm/Main.qml` 并退出。
5. `worker.process_smoke`：Worker 进程启动并输出固定标记。
6. `build.reject_x86`：x86 请求必须在配置阶段明确失败。
7. `build.reject_arm64`：ARM64 请求必须在配置阶段明确失败。

另以固定 SDK 的 `qmltestrunner -input tests/qml` 和 `qmllint src/app/qml/Main.qml tests/qml/tst_skeleton.qml` 独立复核，二者退出码均为 0。

## 最终验证矩阵

| preset | 配置 | 编译 | CTest | 安装文件 / 总字节 | 原生日志及 SHA-256 | 安装清单 SHA-256 |
|---|---|---|---|---:|---|---|
| `windows-msvc-x64-debug` | 通过 | 67/67 | 7/7，100% | 66 / 143,005,992 | `native-20260909-173845.log` / `887BE3FCBFF1C0664AA9C6B4A02C8298FD63CFD9000887AA67771424800459F0` | `2957EDD476326D18C141EE7DD61A8F69C75447C98AB015E375F1954092741486` |
| `windows-msvc-x64-release` | 通过 | 67/67 | 7/7，100% | 57 / 86,681,040 | `native-20260909-173918.log` / `661F797FB94DBE04514EB67660FC130A393D011BC47127ACFA4F4EF101E5CD21` | `7734AFDB81B747DBE63D2A49EE50D22481ED55F8C4BAE6FBA342D1A8BC481A1A` |
| `ci-windows-msvc-x64` | 通过 | 67/67 | 7/7，100% | 57 / 86,804,608 | `native-20260909-173948.log` / `F8B095F1756B87C0E9DBAC154FF64C12716EC194C354A91DC1EC31168430788E` | `40481E3381D075BDD61CEDB8DC250ADC10865C06AC3B1FACD3DE946BC2F38EC5` |

每个预设的 `dumpbin /headers` 对应用和 Worker 都检出 `8664 machine (x64)`。Debug 依赖 Qt `*d.dll`、`MSVCP140D.dll`、`VCRUNTIME140D.dll`、`VCRUNTIME140_1D.dll` 和 `ucrtbased.dll`；Release/CI 依赖无 `d` 后缀 Qt DLL、Release VC Runtime 与 UCRT API set。安装目录均由 `windeployqt` 生成 Qt/QML 运行闭包，并保存逐文件 SHA-256 CSV。

## 本机应用控制限制

当前受管主机的 Windows Defender Application Control 会随机拒绝新生成或新下载的可执行文件，已观察到 `0xC0E90002`、`unknown error` 以及退出 0 但无输出的变体。这不是编译失败，且同一文件在不同轮次可能直接通过。最后一次三预设全量复核中，Debug、Release、CI 的已安装 App/Worker 均直接输出预期标记；此前轮次曾实际触发受限回退，证明告警和失败边界有效。CTest 在显式恢复环境中执行，故 7/7 表示生成目标优先且必要时允许同一受限回退；正式 CI 没有该例外。

恢复逻辑默认关闭，仅当开发者显式传入 `-AllowWdacFallback` 时才接受上述策略拒绝并改由已固定、受信任的 Qt SDK 工具复核同一 QML 入口。GitHub Actions 工作流调用默认 `-Stage All -Clean`，没有 `-AllowWdacFallback` 或 `-UseExistingDependencies`，因此 runner 上任何生成进程不能执行都会使 CI 失败。

## 仓库复现输入哈希

| 文件 | SHA-256 |
|---|---|
| `CMakeLists.txt` | `13E6C0CE9145A99686A1EF19BE2D6955C03C3ABD23713393E7C22660AA0B987B` |
| `CMakePresets.json` | `128C1ABFB38080901EA7121533DA553611FD4ADC0277A7EC911AF8402D4EE546` |
| `vcpkg.json` | `F6B524006C133A0B741CFD4E5944C747655817968044CC965A353DB02AC54F4B` |
| `cmake/triplets/x64-windows-space-rhythm.cmake` | `9B0D01DEBC8E6EE384FCFFC50CA868DFE332AFC03E5F21F24B660BF133A43B73` |
| `tooling/windows/Invoke-ProjectBuild.ps1` | `87B87E1209A19ECE55EEBEEB7E1BAC48BE9E02CA3EE360CEA9039CD886FB9E65` |
| `.github/workflows/windows-x64.yml` | `9A0DE3450A9292D6E68929C6C4C328BC0DCC17C4C2FC54049AC91401130BD135` |
| `docs/windows-build.md` | `EE0FD20350FFC80982CD126D09BAE06021FA6280A358643FC4C4BAFDAF224B45` |

复现入口为 `tooling/windows/Invoke-ProjectBuild.ps1`，正常命令见 `docs/windows-build.md`。完整日志与安装闭包在 `.gitignore` 排除的 `out/` 中，本摘要固化最后成功结果及其哈希。`package/` 在验证前后均保持未跟踪且未修改；本任务没有读取其内容、执行其文件或将其纳入构建与提交。
