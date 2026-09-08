# 一期技术栈选型建议

- 项目：space-rhythm
- 成果 ID：A-005
- 负责人：architect-01
- 关联任务：T-006、T-007、T-008
- 版本：0.3
- 更新日期：2026-09-07
- 状态：draft
- 适用范围：D-001 产品方向、D-002 Qt Quick/QML 与 C/C++ 技术基线、D-004/D-005 Windows x64 与 Qt 源码构建范围下的一期原型及 MVP 工程栈；不构成生产部署、采购或许可证法律结论。
- 来源及输入版本：D-001、D-002、D-003 proposed、D-004、D-005、D-006 proposed；[A-004 0.5](A-004-mvp-technical-feasibility-and-requirements.md)；本会话用户于 2026-09-07 确认 Qt Quick/QML、Windows 平台、x64 架构与 Qt 源码构建；各组件当前官方文档。
- 批准依据：文档整体尚无批准决定。Qt Quick/QML 与 C/C++ 已由 D-002 确认，Windows x64 与 Qt 源码构建已由 D-004/D-005 确认；本文件其余选择汇总为 D-003 proposed，编译器路线为 D-006 proposed，等待用户确认或调整。
- 版本记录：
  - 2026-09-07，0.1，形成一期技术栈、排除项、工程目录和冻结门槛建议。
  - 2026-09-07，0.2，依据 D-004 将目标平台收敛为 Windows，移除 Linux 构建、CI、分发和验收要求；保留最低 Windows 版本等待确认项。
  - 2026-09-07，0.3，依据 D-005 固定 Windows x64 和 Qt 源码构建，补充构建前置、ABI、目录隔离与可复现要求；编译器路线登记为 D-006 proposed。

## 1. 推荐结论

建议采用一套“源码构建的 Windows x64 Qt Quick 壳层 + 原生 C++ 应用层 + 纯 C/C++ 算法核心 + FFmpeg/OpenCV 媒体算法基础”的克制型技术栈。先用经典算法通过 G1，再决定是否增加模型运行时；不在一期同时引入 JUCE、Python 生产运行时、数据库或多套媒体管线。

| 领域 | 推荐选择 | 状态 | 主要理由 |
|---|---|---|---|
| UI | Qt Quick/QML + Qt Quick Controls | D-002 已确认 | 适合时间线、状态绑定、动画和统一视觉。 |
| Qt 供应方式 | 固定官方源码，Windows x64 shared build | D-005 已确认 | 不依赖预编译 Qt 包；源码、构建、安装和产物清单可追溯，编译器路线另见 D-006。 |
| UI 后端 | Qt/C++ ViewModel、`QAbstractItemModel`、注册 QML 类型 | 建议 | 保持 QML 轻量，把数据、命令和生命周期放在可测试 C++。 |
| 语言 | C++20 为主，C17 用于窄 ABI/第三方库 | D-003 proposed | C++20 足以提供 RAII、`std::span`、`std::jthread/stop_token` 等能力，工具链成熟；不需要为一期追新到 C++23。 |
| 构建 | CMake + `CMakePresets.json` + Ninja | D-003 proposed | 同一套 Windows 配置覆盖开发机和 CI，避免手工参数漂移；本阶段不建立 Linux preset。[CMake Presets](https://cmake.org/cmake/help/latest/guide/user-interaction/index.html#presets) |
| 依赖 | vcpkg manifest 管理非 Qt 依赖，Qt 构建为独立版本化 SDK | D-003 proposed | manifest 锁定 FFmpeg/OpenCV 等依赖；Qt 使用独立源码、配置记录和安装前缀，不交给 vcpkg 隐式漂移。[vcpkg manifest](https://learn.microsoft.com/vcpkg/concepts/manifest-mode) |
| 媒体 | FFmpeg `libavformat/libavcodec/libswresample/libswscale` API | D-003 proposed | 直接掌控 PTS、VFR、解码、重采样、编码和封装；避免预览与导出采用不同媒体真值。[FFmpeg 文档](https://ffmpeg.org/documentation.html) |
| 音频设备 | Qt Multimedia `QAudioSink` 仅作 PCM 输出端 | D-003 proposed | 产品 P0 不要求专业低延迟采集；核心混音和时钟仍在 C++，设备适配留给 Qt。[QAudioSink](https://doc.qt.io/qt-6/qaudiosink.html) |
| 视频分析 | OpenCV 经典算法 | D-003 proposed | 先以帧差/直方图、光流、运动能量和峰值检测建立可解释基线；OpenCV 4.5+ 采用 Apache 2.0。[OpenCV License](https://opencv.org/license/) |
| 音频分析 | 自研 C++ DSP + KissFFT 候选 | D-003 proposed | 一期只需 STFT、频带能量、谱通量、瞬态和节拍候选；KissFFT 是轻量 BSD-3-Clause C 库，易封装和分发。[KissFFT](https://github.com/mborgerding/kissfft) |
| 可选模型 | ONNX Runtime C++ | 延后到 G1 失败后 | 只有经典算法无法满足自然度门槛时才引入，不携带 Python 运行时。[ONNX Runtime C++](https://onnxruntime.ai/docs/get-started/with-cpp.html) |
| 高频绘制 | C++ `QQuickItem` + `QSGGeometryNode`，必要时 Shader Tools | D-003 proposed | 波形、频谱和事件标记批量提交几何，避免一个采样点一个 QML Item。[Qt Quick Scene Graph](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html) |
| 进程模型 | UI 主进程 + `space-rhythm-worker` 后台进程 | D-003 proposed | 解码、分析或导出崩溃不带走未保存 UI；控制消息走本地 IPC，大数据写缓存或共享缓冲。 |
| IPC | 长度前缀的版本化消息 + `QLocalSocket` 适配 | D-003 proposed | 在 Windows 上由 Qt 映射为命名管道；只开放当前用户访问。[QLocalSocket](https://doc.qt.io/qt-6/qlocalsocket.html) |
| 项目存储 | 版本化 JSON 项目文件 + 可重建二进制缓存 | D-003 proposed | 一期事件量可控，JSON 易迁移和诊断；数据库暂时没有必要。 |
| C++ 测试 | GoogleTest + CTest | D-003 proposed | 核心算法和契约测试可在 Windows CI 中无 GUI 运行。[GoogleTest/CMake](https://google.github.io/googletest/quickstart-cmake.html) |
| Qt/QML 测试 | Qt Test + Qt Quick Test | D-003 proposed | 分别覆盖 Qt/C++ 适配层和 QML 交互。[Qt 测试概览](https://doc.qt.io/qt-6/testing-and-debugging.html) |

## 2. 版本建议

### 2.1 Qt

- 若采用 Qt 商业许可证并需要长期维护，优先评估 **Qt 6.8 LTS**；Qt 官方说明该 LTS 维护至 2029 年，但完整 LTS 维护版本属于商业许可证路径。[Qt 6.8 LTS](https://www.qt.io/development/qt-framework/qt-lts)
- 若采用开源 LGPL 路径，建议从截至 2026-09-07 的最新稳定 Qt 6 minor（当前为 Qt 6.11）选择经过本项目验证的确切 patch，并建立定期升级节奏，而不是把“LTS”当成自动获得的社区补丁服务。[Qt Releases](https://doc.qt.io/Qt.html)
- 无论哪条路径，都必须在创建代码仓前确定 Qt 开源/商业许可路线。Qt 官方说明其具有商业和 LGPL/GPL 路径，部分模块对开源用户仅提供 GPL；这需要结合产品分发方式审查，不构成法律意见。[Qt Licensing](https://doc.qt.io/qt-6/licensing.html)
- 开源闭源产品若走 LGPL，默认采用动态链接并保留替换 Qt 库的能力；不要在许可未确认前静态链接或使用仅 GPL 可用模块。
- D-005 已确认不使用预编译 Qt 包。选定版本后必须获取对应官方源码归档或 Git tag，记录来源、版本和 SHA-256/提交哈希；不得从不明镜像获取源码或在 CI 中静默追踪分支最新提交。

本项目不需要 `Qt Quick Timeline` 模块来实现编辑时间线；该名称与产品时间线概念无关。可用基础 Qt Quick、模型和自定义场景图完成，避免在许可证未确认前引入额外模块。

### 2.2 C/C++ 与工具链

- 建议冻结 C++20、C17。
- Qt 6.11 官方支持 Windows x86_64 使用 MSVC 2022 或 MinGW-w64 13.1。当前没有已确认的编译器，D-006 保持 proposed；源码构建不能绕过该前置条件。[Qt Windows 源码构建](https://doc.qt.io/qt-6/windows-building.html)
- 推荐路线是安装 MSVC 2022 Build Tools 的 x64 C++ 工具及 Windows SDK，再用该环境构建 Qt、应用和同进程原生依赖；若 MSVC 明确不可安装，则整套切换为 Qt 支持的 MinGW-w64 版本。不能把 MinGW 构建的 Qt 与 MSVC 构建的应用或 C++ 库混在同一进程。
- Debug、RelWithDebInfo、Release 和 sanitizer/分析预设进入 `CMakePresets.json`；用户私有路径只进入不提交的 `CMakeUserPresets.json`。
- 依赖必须以版本、port revision 或提交哈希锁定；禁止 CI 每次静默获取“latest”。

### 2.3 Qt 源码构建基线

Qt 官方当前要求 Windows 源码构建环境能够直接找到 `cmake.exe`、`ninja.exe` 和 `python.exe`，并具备受支持的 C++ 编译器；CMake 最低为 3.22。若走 MSVC 2022，Visual Studio Installer 中还需选择受支持的 Windows SDK。[Qt Windows 源码构建](https://doc.qt.io/qt-6/windows-building.html)

建议固定以下工程要求：

- 目录采用短且不含空格的绝对路径，并实施 shadow build，例如分别使用 `C:\dev\qt-src\<version>`、`C:\dev\qt-build\<toolchain>-x64`、`C:\dev\qt-sdk\<version>-<toolchain>-x64`；不在 Qt 源码树内生成构建产物。
- 使用 `configure.bat -prefix <install-dir>` 配置，再以 `cmake --build . --parallel` 和 `cmake --install .` 构建、安装。Ninja 存在时由 Qt configure 优先使用，也是本项目推荐生成器。[Qt Configure](https://doc.qt.io/qt-6/configure-options.html)
- 发布 SDK 使用 shared build；不使用面向 Qt 自身开发的 `-developer-build`。Debug/Release 是否同时构建由开发调试需求确定，但两个配置必须来自相同源码、编译器和 configure 参数。
- 初始模块白名单只包含 `qtbase`、`qtdeclarative`、`qtshadertools`、`qtmultimedia` 及其依赖；`qtsvg`、`qtimageformats` 等按实际 UI/格式需求加入，不默认编译 Qt WebEngine、Qt GRPC、Qt Protobuf、示例、测试或 QDoc。
- 构建脚本必须保存 Qt 源码标识、编译器/Windows SDK/CMake/Ninja/Python 版本、环境架构、完整 configure 参数、configure summary、安装文件清单和产物哈希；开发机与 CI 使用同一份版本化配置。
- Qt SDK 先构建成功并通过最小 QML 程序验证，再构建 space-rhythm。FFmpeg、OpenCV 等进入同一进程的原生库必须采用与应用兼容的 x64 编译器和运行时配置；不得仅凭文件是 `.dll` 就假设 ABI 兼容。

## 3. UI 与渲染实现

### 3.1 QML 负责什么

- 窗口布局、面板、菜单、弹窗、主题、键盘交互、属性绑定和轻量动画。
- 通过只读属性、命令方法和 `QAbstractItemModel` 展示项目、轨道、事件和作业状态。
- 不执行媒体遍历、FFT、光流、模型推理或逐像素/逐采样循环。

### 3.2 C++ 负责什么

- ViewModel、命令分发、撤销/重做、播放器状态机、时间映射和 worker 生命周期。
- 时间线高密度标记、波形、频谱和视觉预览使用自定义 `QQuickItem` 批量绘制。
- 优先使用 `QSGGeometryNode` 和 Qt Shader Tools；`QQuickPaintedItem` 只用于低频、低密度内容。Qt 官方说明 `QQuickPaintedItem` 需要先光栅化到中间表面，直接场景图通常更快。[Scene Graph](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html)
- `QSGRenderNode/QRhi` 只在 `QSGGeometryNode` 无法满足离屏或复杂管线时使用；相关接口兼容性和渲染线程约束必须通过原型验证。

## 4. 媒体与音频选择

### 4.1 为什么核心不用 `QMediaPlayer`

`QMediaPlayer` 可用于普通播放，但本项目需要统一掌控源 PTS、可变帧率、逐帧分析、代理生成、离线渲染和导出。建议由 FFmpeg Media Service 作为媒体真值来源，把解码帧和 PCM 提供给预览；否则 Qt 播放后端与 FFmpeg 导出后端可能形成两套时间解释。

### 4.2 音频时钟

- C++ Audio Engine 负责重采样后的 PCM、事件触发、混音、削波检查和环形缓冲。
- `QAudioSink` 只负责把约定格式的 PCM 送入系统设备；播放期间以已消费音频样本数作为主时钟，视频预览向音频时钟对齐。
- 离线导出不经过音频设备，直接由相同混音核心按整数采样索引生成 PCM。

### 4.3 FFmpeg 许可证门槛

建议默认使用动态链接的 LGPL-only FFmpeg 构建，关闭 GPL/nonfree 组件。FFmpeg 官方说明其主体为 LGPL 2.1+，启用部分可选组件会使整个 FFmpeg 构建转为 GPL，启用 nonfree 还可能导致不可再分发。[FFmpeg Legal](https://ffmpeg.org/legal.html)

因此 **H.264 编码后端不能现在静默绑定 libx264**。需要根据发布许可、目标平台和质量要求，在平台编码器、适当许可的实现或其他输出格式之间做单独决定，并保存实际 FFmpeg configure 参数、源码和许可证材料。

## 5. 算法路径

### 5.1 视频到节奏

第一阶段按以下顺序实现：

1. 代理帧生成和颜色/尺寸归一化。
2. 镜头切换：颜色直方图或结构差异，配合渐变切换抑制。
3. 运动强度：稀疏/稠密光流或块运动统计，并区分全局运镜和局部运动。
4. 动作候选：运动能量曲线平滑、峰值和停顿/反转检测。
5. 事件融合：最小间隔、密度、来源权重、置信度和锁定保护。
6. 通过 G1 样例评估；只有传统方法证据不足时，才引入 ONNX 本地模型。

### 5.2 音频到视觉

- 自研 C++ DSP 管线：分帧、窗函数、FFT、谱通量、瞬态、能量包络、频段聚合和节拍候选。
- FFT 实现封装在 `IFftBackend` 后，先以 KissFFT 验证；如性能不足，可替换为平台优化库而不改变上层算法。
- 波形与频谱缓存采用分层降采样，缩放时间线时只读取对应层级，不向 QML 传输全量采样点。

## 6. 进程、存储与 IPC

```text
space-rhythm-ui
  QML + Qt/C++ ViewModel
            │ 控制、进度、结果元数据
            ▼
  QLocalSocket / versioned IPC
            │
            ▼
space-rhythm-worker
  FFmpeg + OpenCV + DSP + Event Engine + Export
            │
            └── 可重建代理/特征/临时导出缓存
```

- IPC 只传命令、进度、错误和稀疏事件；原始帧、PCM 和大特征不通过 JSON 往返复制。
- 首个垂直切片可以先使用单 worker；并发多作业在性能证据出现后再增加。
- 项目文件建议为单个 UTF-8 JSON 主文档，扩展名可后续确定；自动保存单独存放，缓存按素材指纹与算法版本管理。
- 一期不使用 SQLite：当前事件稀疏、查询简单，数据库会增加迁移、锁和恢复路径。只有事件规模、并发写入或查询证据证明 JSON 不足时再引入。

## 7. 建议工程目录

```text
src/
  app/                 # 启动、依赖装配
  ui/qml/              # QML 页面、组件、主题
  ui/bridge/           # QObject、ViewModel、QAbstractItemModel
  domain/              # 纯 C++ 项目、时间、事件、轨道模型
  media/               # FFmpeg RAII 封装与时间映射
  analysis/video/      # OpenCV 视频分析
  analysis/audio/      # DSP、FFT、节拍与瞬态
  rendering/           # QQuickItem、QSG 节点、离屏配方
  audio/               # PCM 混音、QAudioSink 适配
  persistence/         # JSON schema、迁移、原子保存
  worker/              # 后台进程、作业和 IPC
tests/
  unit/                # 无 GUI 核心测试
  qml/                 # Qt Quick Test
  integration/         # worker/媒体/项目集成
  golden/              # 小型合法媒体与期望事件/时间戳
```

依赖方向保持 `QML -> Qt bridge -> domain/application -> media/algorithm abstractions`；`domain` 与 `analysis` 不反向依赖 QML、Qt Quick 或具体页面。

## 8. 明确不建议一期采用

- **JUCE**：它在音频和 UI 上与 Qt 大量重叠，会引入第二套事件、绘制和生命周期体系；P0 文件播放/离线导出不需要插件框架。
- **生产 Python 运行时**：可用于研究阶段离线对照，但不进入发布包或核心运行路径，避免包体、环境和跨语言复制复杂度。
- **默认深度学习模型**：先用经典算法测量；没有 G1 证据不增加模型、GPU 后端和模型许可。
- **Qt Multimedia 作为唯一媒体管线**：设备输出可以使用，但解码、时间戳、分析和导出统一由 FFmpeg 核心控制。
- **直接绑定 OpenGL 或 Direct3D**：优先使用 Qt Quick 场景图抽象，避免业务渲染代码绑定具体 Windows GPU 后端；仅在测量证明需要时进入更低层。
- **一期数据库**：项目 JSON 与可重建缓存足够，避免过早引入迁移和锁语义。
- **运行时下载编解码器**：依赖随包或受控安装并校验，不复制 A-001 中观察到的无固定完整性下载风险。

## 9. 首个工程验证切片

在冻结完整技术栈前，建议用一个最小工程同时验证以下路径：

1. Qt Quick 打开视频，FFmpeg worker 返回真实 PTS 的代理帧。
2. OpenCV 输出镜头/运动事件，Qt/C++ 模型把事件显示在 QML 时间线。
3. 用户移动并锁定事件，重新分析后锁定事件不变。
4. C++ 音频引擎按事件触发一个合法测试音色，`QAudioSink` 播放。
5. C++ `QQuickItem/QSGGeometryNode` 绘制波形或事件密度，不为每个点创建 QML 对象。
6. 保存 JSON、关闭重开并恢复事件；worker 中止或崩溃不破坏保存文件。
7. GoogleTest 验证时间换算和合并规则，Qt Quick Test 验证拖动/锁定交互。

在进入该切片前，先按 D-005 完成 Windows x64 Qt SDK 源码构建，并以最小 QML 程序验证运行和部署；编译器路线须先按 D-006 确认。切片通过后再冻结确切 FFmpeg、OpenCV、KissFFT、vcpkg baseline 和工具集版本。Qt 许可证路径、最低 Windows 版本和 H.264 编码后端必须在发布型构建前确认。

## 10. 待用户确认的技术基线

建议用户下一步确认或调整 D-003 的总体组合。若接受，可以把以下内容作为工程默认值：

- C++20/C17、CMake Presets、Ninja、vcpkg manifest。
- FFmpeg 直接 API，OpenCV 经典算法，自研 C++ DSP + KissFFT。
- Qt Quick 场景图批量绘制，QAudioSink 仅作 PCM 设备输出。
- UI/worker 双进程、版本化本地 IPC、JSON 项目文件。
- GoogleTest + Qt Test/Qt Quick Test。
- ONNX Runtime 只作为 G1 失败后的可选项。

已确认：Windows x64、Qt 官方源码自行构建。仍需单独确认：D-006 的 MSVC 2022 Build Tools 或 MinGW-w64 路线、Qt 商业或 LGPL 路径、Qt 确切版本、最低 Windows 版本、Windows 安装器/签名方式、H.264 编码后端及发布格式矩阵。Linux、x86、ARM64 和 ARM64EC 已排除在第一阶段范围外。
