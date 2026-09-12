# Qt Quick 真实服务、时间线与安全导出集成

- 项目：space-rhythm
- 成果 ID：A-027
- 负责人：ui-engineer-qt-quick-01
- 关联任务：T-026
- 版本：0.2
- 更新日期：2026-09-12
- 状态：draft
- 适用范围：一期 Windows x64 Qt Quick 应用的真实核心、系统、媒体、音频、播放同步、屏上/离屏渲染、项目保存及测试导出格式集成；覆盖完整用户路径、时间线事务、作业取消/断线恢复和 UI 自动化。
- 来源及输入版本：D-001～D-008 confirmed；A-012 0.1、A-014 0.4、A-015 0.3、A-018 0.2、A-019 0.2、A-020 0.1、A-021 0.1、A-022 0.1、A-023 0.1、A-024 0.1、A-025 0.1、A-026 0.1；T-015、T-016、T-018、T-019、T-025、T-032、T-034、T-035 completed。
- 批准依据：尚无。任务完成不自动批准本成果；视觉风格、产品文案、产品默认音色、发布导出格式、产品默认模板参数和可访问性/性能阈值仍待确认。
- 版本记录：2026-09-12，0.2，按 architect-01 的 `T026-DEFECT-001`～`004` 修订手势事务、真实 UI/worker 双进程、事件音轨闭环及非阻塞保存/分块导出；0.1，首次接入真实六阶段路径、时间线事务、T-019 时钟、作业恢复、冻结快照及安全导出。

## 1. 修订结论

应用默认入口仍为 `IntegratedWorkspaceService`，确定性 mock 继续用于页面状态自动化；两者共享 `WorkspaceService`、DTO、错误和异步状态语义。UI bridge 因新增手势 pending/ghost、音色映射状态及命令提升为 `0.3.0/schema 3`。QML 不执行媒体循环、DSP、项目序列化、几何生成、离屏渲染或音频混音。

修订后的真实路径为：

1. UI 只把路径、操作、request/job ID 和文件引用交给 T-016 `LocalIpcClient`。独立 `space-rhythm-worker --ui-worker` 进程执行 `MediaSource::open/select`、音频解码、`PcmNarrowAdapter` 和 `Analyzer`；特征/候选写入 schema 1 二进制结果文件，以长度和 SHA-256 校验的 `DataReference` 返回，IPC JSON 不携带 PCM 或特征块。
2. worker 候选回到 GUI 线程后才以 `MergeAnalysisCandidates` 提交核心；过期 base revision、迟到结果、损坏结果文件或引用不匹配均 fail closed。取消会终止实际 worker 进程，客户端观察到终态后才从 `cancelling` 发布 `cancelled`；进程异常退出走真实断线恢复。
3. 每次核心提交、撤销或重做后，C++ 用同修订 timeline、feature series、模板参数和 seed 重建不可变 `RenderSnapshot`；两个 `SceneGraphRenderItem` 只消费快照，不在 QML 重新生成几何。
4. 产品默认音色仍待确认，初始状态不建立静默默认 mapping。用户只有显式选择“启用 CC0 开发音色映射”后，C++ 才装载 A-018 三个登记测试音色，并固定把 onset/beat/manual 映射到 click/low-pulse/noise-hit。试听和导出消费同一事件、mapping、音色 hash 与 T-032 mixer；试听不再播放解码源 PCM。存在设备时 `QtAudioPreview::processed_frames()` 驱动 T-019 audio-sample 主时钟，headless/设备不可用时使用其 C++ monotonic 回退。
5. 主保存与 autosave 均复制不可变 `ProjectDocument` 后在后台执行；按 IO epoch 串行化主保存、autosave 与 session marker，旧 autosave 不会覆盖较新的 clean/dirty 事实。`post()` 返回时保存仍未发布完成。
6. 导出先由 T-019 `freeze_export` 冻结媒体选择、RenderSnapshot、事件音频参数/音色 hash、范围、帧率和 seed。T-035 逐帧事件泵保持 GUI 响应；视频完成后在后台以 4096 frame 块调用 T-032 `render_chunk` 并流式写入 `ExportSession`，不会在 GUI 线程生成整段 PCM，取消仍删除临时文件并保留既有目标。

发布容器与编码器尚未决定，界面继续明确当前输出为 T-019 `testOnly` NUT/raw RGBA/PCM 开发格式。

## 2. 权威值和进程/线程边界

`WorkspaceSnapshotDto` 在 C++ 持有 `TimeNs`、`TimelineRevision`、`FrameIndex`、`TimeRange` 和 `shared_ptr<const RenderSnapshot>`。QML 元对象只得到格式化时间/修订/视口、非权威 ghost 比例、状态和像素手势入口。`renderSnapshot()`、`viewportTimeRange()`、`authoritativeFrameIndex()` 仍是未注册到 Qt meta-object 的 C++ 专用接口。

| 工作 | 执行位置 | 返回 UI 的内容 |
|---|---|---|
| 媒体探测、解码、DSP 分析 | 独立 `space-rhythm-worker` | 小型进度 envelope + 已校验结果文件引用 |
| 核心事务、ViewModel 发布、QQuickRenderControl 逐帧调用 | GUI 线程的短事务/事件泵 | 不可变 snapshot 与格式化展示值 |
| 项目主保存、autosave、marker | 串行后台 IO | 成功/结构化错误；只在同修订时 mark saved |
| 事件试听整段准备 | 后台线程 | T-032 事件 PCM；过期 revision 丢弃 |
| 导出音频混音、编码 finish/commit | 后台线程，4096-frame 分块 | 进度、取消或原子完成 |

QML 中没有 `Animation`、`Timer` 或 frame/time/revision 运算充当权威时钟。

## 3. 时间线和播放头事务

| UI 动作 | C++ 权威行为 |
|---|---|
| 左键拖动事件 | begin 冻结 event、初始时间和 base revision；move 只更新 ghost；release 在 base revision 仍匹配时提交至多一个 `MoveEvent` |
| Escape | 清除 drag/seek pending，恢复 seek 前状态；不改时间线 revision 或权威播放头 |
| 拖动期间修订变化 | release 拒绝提交并播报“时间线已更新，请重试” |
| Alt+左键拖动播放头 | begin/update 只发布 pending ghost 和 `seeking`；release 调用 T-019 seek 后确认，Escape 恢复 |
| 双击播放头 | 走同一 begin→commit 路径，不绕过 pending/revision 检查 |
| Shift+双击、锁定、批量偏移、撤销/重做 | 分别调用 `AddEvent`、`SetEventLocked`、一次 `BatchOffsetEvents` 和核心 history API |
| 缩放、平移、命中 | 坐标只在 C++ 结合当前 viewport 与不可变快照换算并钳制 |

新增稳定自动化/可访问锚点：`timelineInteractionGhost`、`timelineInteractionStatus`、`enableDevelopmentAudioMappingButton`、`audioMappingStatus`；既有素材、预览、任务、保存/导出和 SceneGraph 锚点保持稳定。全局 Escape 优先取消时间线手势，没有手势时才进入任务取消确认。

## 4. 错误与恢复

导入、分析和导出继续登记到 `JobCoordinator`，展示 queued/running/cancelling/succeeded/failed/cancelled 与 ppm 进度。worker QProcess 的 `finished/errorOccurred` 是断线事实源；测试故障注入命令实际 kill 外部进程，不再直接伪造 ViewModel 失败。断线保留项目和原素材、拒绝迟到结果并提供重启 worker；重试使用新 request/job ID。

主保存、autosave、结果 codec、T-032/T-035/T-019 错误都映射到版本化 `UiErrorDto`。只读状态不提交核心或磁盘写入，但可检查和在具备显式音频 mapping 时预览/导出冻结快照。

## 5. 保留项

- 视觉风格和最终产品文案：待产品确认；当前为 A-023 工程令牌与说明性中文文案。
- 产品默认音色：待确认；A-018 CC0 音色只能经用户显式启用，文案持续标为开发映射，不成为静默产品默认值。
- 发布导出容器、视频/音频编码器和扩展名：待确认；当前只有明确 `testOnly` 的 NUT/raw RGBA/PCM。
- 产品默认视觉模板与参数：待确认；当前使用确定性 rhythm-line-pulse 工程默认值。
- 可访问性设备矩阵、基准硬件和性能阈值：待确认；本轮只报告自动化结果，不宣称产品门槛获批。

自测与环境限制详见 [T-026 验证摘要](../evidence/T-026/verification-summary.md)。
