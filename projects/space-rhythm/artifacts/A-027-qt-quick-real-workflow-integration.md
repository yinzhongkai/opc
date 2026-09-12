# Qt Quick 真实服务、时间线与安全导出集成

- 项目：space-rhythm
- 成果 ID：A-027
- 负责人：ui-engineer-qt-quick-01
- 关联任务：T-026
- 版本：0.1
- 更新日期：2026-09-12
- 状态：draft
- 适用范围：一期 Windows x64 Qt Quick 应用的真实核心、系统、媒体、音频、播放同步、屏上/离屏渲染、项目保存及测试导出格式集成；覆盖完整用户路径、时间线事务、作业取消/断线恢复和 UI 自动化。
- 来源及输入版本：D-001～D-008 confirmed；A-012 0.1、A-014 0.4、A-015 0.3、A-018 0.2、A-019 0.2、A-020 0.1、A-021 0.1、A-022 0.1、A-023 0.1、A-024 0.1、A-025 0.1、A-026 0.1；T-015、T-016、T-018、T-019、T-025、T-032、T-034、T-035 completed。
- 批准依据：尚无。任务完成不自动批准本成果；视觉风格、产品文案、产品默认音色、发布导出格式、产品默认模板参数和可访问性/性能阈值仍待确认。
- 版本记录：2026-09-12，0.1，首次接入真实六阶段路径、时间线事务、T-019 时钟、作业恢复、冻结快照及安全导出。

## 1. 交付结论

应用默认入口已从确定性 mock 切换为 `IntegratedWorkspaceService`；`--ui-scenario` 仍保留给页面状态自动化。真实服务与 mock 继续实现同一个 `WorkspaceService`，UI bridge 提升为 `0.2.0/schema 2`。QML 页面不接触媒体循环、DSP、项目序列化、离屏渲染或编码器。

真实路径如下：

1. 文件选择将轻量路径命令交给 C++；后台线程调用 `MediaSource::open/select`，发布 FFmpeg 探测结果和素材状态。
2. 分析任务用 `MediaSource::decode_audio` 输出 48 kHz 单声道 PCM，经过 `PcmNarrowAdapter` 进入 `Analyzer`，候选通过 `MergeAnalysisCandidates` 事务进入核心时间线。
3. 每次核心提交、撤销或重做后，C++ 用同修订 timeline、feature series、模板参数和 seed 重建不可变 `RenderSnapshot`；屏上预览和时间线宿主都接收同一个快照。
4. 播放只由 T-019 `PreviewSynchronizer` 推进。分析阶段保留同一媒体源解码出的 48 kHz 单声道 PCM；存在可用输出设备时，`QtAudioPreview::processed_frames()` 把 QAudioSink 实际播放帧数交给 T-019 作为 audio-sample 主时钟，headless 或设备不可用时才使用其 C++ monotonic 回退。播放原素材不等同于选择产品默认事件音色，QML 动画在两种模式下都不参与计时。
5. 保存调用 T-016 `ProjectStore` 的 schema 2 原子事务并维护 autosave/session marker；重新打开恢复精确 64 位时间线快照和素材引用。
6. 导出先调用 T-019 `freeze_export` 冻结同修订媒体选择、RenderSnapshot、音频参数、范围、帧率和 seed，再由 T-035 `OffscreenRenderSession` 逐帧产生 `RenderedFrame`、T-032 mixer 产生确定性 PCM，最后由 `ExportSession` 在同目录临时文件上完成原子提交。

发布容器与编码器尚未决定，因此界面明确标记当前输出为 T-019 `testOnly` NUT/raw RGBA/PCM 开发格式；本成果不把它声明为发布格式。

## 2. 权威值与线程边界

`WorkspaceSnapshotDto` 在 C++ 内持有 `TimeNs`、`TimelineRevision`、`FrameIndex`、`TimeRange` 和 `shared_ptr<const RenderSnapshot>`。QML 元对象只得到格式化时间/修订/视口文本、能力布尔值和像素手势入口。`ApplicationViewModel::renderSnapshot()`、`viewportTimeRange()`、`authoritativeFrameIndex()` 是未注册到 Qt meta-object 的 C++ 专用方法，由应用入口直接同步两个 `SceneGraphRenderItem`。

导入和分析使用可取消后台线程；`WorkspaceService::post()` 不等待它们。结果通过 queued invoke 返回 GUI 线程后才提交核心事务和发布模型。T-035 要求 `QQuickRenderControl` 位于 GUI 线程，因此导出使用零间隔事件泵逐帧执行并在帧间返回事件循环，支持进度、取消和窗口响应。QML 中没有 `Animation`、`Timer` 或帧索引计算充当播放时钟。

## 3. 时间线交互

| UI 动作 | C++ 权威行为 |
|---|---|
| 滚轮、`+/-`、工具按钮 | 围绕当前视口中心缩放，限制在 100 ms（短素材取素材全长）至素材全长 |
| 中键/右键拖动、左右键 | 像素差由 `CoordinateTransform` 映射为视口平移并钳制到素材范围 |
| 单击、Ctrl+单击 | C++ 以当前快照、修订、视口和 subpixel 坐标命中单选/追加选择 |
| Shift+双击 | 通过 `AddEvent` 增加手工事件；时间由 C++ 坐标映射产生 |
| 左键拖动 | `MoveEvent` 事务带稳定 coalescing key；核心拥有冲突、锁定和修订检查 |
| 锁定/解锁 | `SetEventLocked` 事务；锁定事件的移动/偏移由核心 fail closed |
| 批量偏移 | Ctrl 多选后一次提交 `BatchOffsetEvents`，毫秒文本在 C++ 校验并转为纳秒 |
| Ctrl+Z/Ctrl+Y | 调用核心 `undo/redo(expectedRevision)`，随后重建同修订渲染快照 |
| 双击 | C++ 映射播放头并调用 T-019 `seek`；QML 只显示格式化结果 |

## 4. 作业、取消和恢复

导入、分析、导出都登记到 T-016 `JobCoordinator`，模型显示 queued/running/cancelling/succeeded/failed/cancelled 及 ppm 进度。取消按钮和 Escape 先打开确认对话框；确认后状态保持 `cancelling`，直到后台工作返回取消终态才显示 `cancelled`。过期的后台完成回调会按 active request 丢弃，断线后不得提交迟到候选或覆盖失败状态。

worker 断线调用 `worker_disconnected()` 将活动任务转为可重试失败，保留项目和原素材，状态条展示阶段、诊断 ID与“重新连接 worker”。重新连接只恢复可执行状态，重试仍使用新 request/job ID 和原命令输入。媒体、核心、系统、渲染和编码错误统一映射到版本化 `UiErrorDto`。

## 5. QML 与可访问性

启动页、素材区、阶段工具栏和检查器使用原生 `FileDialog` 完成项目打开、媒体导入、项目保存和开发格式导出。关键新增自动化锚点包括：

- `timelineSceneGraphHost`、`timelineSceneGraphRenderItem`、`timelinePointerArea`
- `timelineUndoButton`、`timelineRedoButton`、`eventLockButton`
- `batchOffsetMillisecondsField`、`batchOffsetButton`
- `taskCancelConfirmationDialog`、`workerReconnectButton`
- `assetImportDialog`、`saveProjectDialog`、`exportProjectDialog`

控件提供 Accessible name/description，取消确认正文使用可访问静态文本。时间线明确设置 `timelineSurface → timelineUndoButton → timelineRedoButton` 的 Tab 顺序。应用在创建 `QGuiApplication` 前启用 `PassThrough` 高 DPI rounding；布局在 960×640 和 1600×1000、1.0×和 1.5×缩放下执行 Qt Quick Test。

## 6. 保留项

- 视觉风格和最终产品文案：待产品确认；当前继续使用 A-023 工程令牌与说明性中文文案。
- 产品默认音色：待确认；A-018 CC0 音色仍只用于测试，不自动进入产品预览或导出默认值。
- 发布导出容器、视频/音频编码器和扩展名：待确认；当前只有明确 `testOnly` 的 NUT/raw RGBA/PCM。
- 产品默认视觉模板与参数：待确认；当前使用确定性 rhythm-line-pulse 工程默认值。
- 可访问性设备矩阵、基准硬件和性能阈值：待确认；本轮只报告自动化功能结果，不宣称产品级门槛批准。

自测与环境限制详见 [T-026 验证摘要](../evidence/T-026/verification-summary.md)。
