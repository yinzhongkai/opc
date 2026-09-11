# Qt Quick 工作区与版本化 ViewModel 桥接实现

- 项目：space-rhythm
- 成果 ID：A-024
- 负责人：ui-engineer-qt-quick-01
- 关联任务：T-025
- 版本：0.1
- 更新日期：2026-09-11
- 状态：draft
- 适用范围：一期 Windows x64 应用壳层、页面和工作区 QML 骨架，版本化 Qt/C++ ViewModel、列表模型、可替换 service/mock 边界，以及 Scene Graph 预览宿主和无界面自测；不实现 T-026 的完整时间线编辑和用户路径集成，不实现媒体、DSP、导出事务或离屏渲染本体。
- 来源及输入版本：D-001～D-008 confirmed；A-004 0.5、A-005 0.4、A-006 0.1 WP-03、A-011 0.1、A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1、A-018 0.2、A-019 0.2、A-020 0.1、A-021 0.1、A-022 0.1、A-023 0.1；T-012/T-013/T-014/T-024/T-030/T-031/T-032/T-033/T-034 已完成。
- 批准依据：尚无。任务完成不自动批准本成果；视觉风格、产品文案、默认音色、发布导出格式和产品默认模板参数继续待确认。
- 版本记录：2026-09-11，0.1，首次实现应用壳层、拆分工作区、UI bridge 0.1.0/schema 1、确定性 mock、Scene Graph 挂载和 Qt/C++/QML headless 测试。

## 1. 交付结论

应用现在具有可构建和可无界面验证的 Qt Quick 外壳：启动页负责新建、打开和恢复；工作区持续呈现六阶段工具栏、素材、预览、时间线、检查器与任务抽屉；不可恢复的兼容性错误进入独立致命错误页。页面由 `ApplicationViewModel` 的路由和状态快照驱动，不在 QML 中执行媒体、DSP、几何、保存或 worker 循环。

新增 `SpaceRhythm::UiBridge` 静态库。`WorkspaceService` 是 mock 与后续真实适配器的唯一输入边界，`ApplicationViewModel` 只依赖该抽象；替换真实 service 不改变 QObject 属性、QAbstractItemModel 角色、DTO、错误字段或异步状态语义。当前应用入口注入确定性 mock，便于在下游服务尚未全部接线时验证页面和状态；没有伪造真实媒体或导出结果。

## 2. 应用壳层与组件边界

| 层级 | 文件/类型 | 责任 |
|---|---|---|
| 应用外壳 | `Main.qml` | 窗口、路由 Loader、启动/工作区/致命页切换 |
| 启动与恢复 | `StartPage.qml` | 新建、打开、自动保存恢复和打开主文件 |
| 致命错误 | `FatalErrorPage.qml` | 项目安全说明、阶段、诊断 ID 和返回入口 |
| 工作区编排 | `WorkspacePage.qml` | 三栏工作区、状态条、任务抽屉和快捷键 |
| 六阶段入口 | `StageToolbar.qml` | 导入、分析、编辑、试听、保存、导出动作和只读标识 |
| 素材 | `AssetPanel.qml` | 素材列表、空状态和只读导入门控 |
| 预览 | `PreviewPanel.qml` | 播放控件、格式化时间和 `previewSceneGraphHost` |
| 时间线 | `TimelinePanel.qml` | T-025 页面骨架、缩放/吸附入口和 C++ 高密度绘制说明 |
| 参数 | `InspectorPanel.qml` | 事件/模板/导出分类、版本化模板参数展示 |
| 作业 | `TaskDrawer.qml` | 排队、运行、取消中、成功、失败、已取消状态和动作 |
| 状态 | `StatusBanner.qml` | 加载、运行、取消、失败、恢复、只读反馈 |

工作区保持 A-023 的持续编辑结构。T-025 只提供时间线和参数的工程骨架；拖动、锁定、偏移、撤销重做及完整六阶段用户路径属于 T-026，本轮未启动。

## 3. UI bridge 0.1.0/schema 1

公开接口位于 `src/ui/include/space_rhythm/ui/`：

- `WorkspaceService`：`descriptor()`、`current_snapshot()`、`set_observer()`、`post()`；`post()` 必须立即返回，耗时媒体、worker、磁盘和渲染结果只能通过后续快照发布。
- `UiBridgeDescriptor`：同时公布 UI bridge、core contract、system IPC/project schema 和 rendering contract 版本，连接处可 fail closed。
- `UiCommand`：包含 schema、contract、请求 ID、命令类型和轻量参数；不携带媒体缓冲或几何。
- `WorkspaceSnapshotDto`：不可变值快照语义，包含路由、工作区状态、项目标志、权威时间/修订/帧、素材、任务、模板参数和结构化错误。
- `UiErrorDto`：`category/code/stage/message_key/diagnostic_id/retryable/project_safe`，mock 与真实适配器不得改变字段语义。
- `AssetListModel`、`TaskListModel`、`TemplateParameterListModel`：稳定角色名，不向 QML 暴露容器地址或 worker 对象。
- `ApplicationViewModel`：唯一 QML-facing QObject，公开展示字符串、能力标志、列表模型和非阻塞命令。

真实 service 的接入点是 `ApplicationViewModel(std::unique_ptr<WorkspaceService>)`。当前 `make_mock_workspace_service()` 与未来 production adapter 必须实现同一个 `WorkspaceService`，不得为 mock 另造属性、错误或成功状态。

## 4. 64 位权威值边界

`WorkspaceSnapshotDto` 在 C++ 中保留真实类型：

- `core::TimeNs preview_time_ns`
- `core::TimelineRevision timeline_revision`
- `rendering::FrameIndex frame_index`

QML 元对象只公开 `previewTimeText` 和 `timelineRevisionText` 两个已格式化 `QString`。没有 `timeNs`、`previewTimeNs`、`timelineRevision` 或 `frameIndex` 属性；模板的 64 位整数范围和值也只以 `minimumText/maximumText/engineeringDefaultText/valueText` 字符串角色进入 QML。时间格式化、修订保真和参数整数转换全部在 C++ 完成。

任务进度的 `NormPpm` 属于安全范围，C++ 同时提供 0～1 展示值和 ppm 诊断值；QML 只做进度条和百分比展示，不据此作权威作业判断。

## 5. 状态与异步语义

确定性 mock 通过 `--ui-scenario=<name>` 支持 `start/idle/loading/running/cancelling/failed/recovery/readonly/fatal`。状态含义如下：

| 状态 | 页面/反馈 | 允许与禁止 |
|---|---|---|
| `idle` | 启动页或就绪工作区 | 按项目和素材能力启用动作 |
| `loading` | 信息条和素材加载空态 | 不保存、导出或分析 |
| `running` | 后台任务及进度 | 允许浏览；可请求取消 |
| `cancelling` | 保持“正在取消”直到确认 | 禁止重复取消，不提前显示已取消 |
| `failed` | 阶段、项目安全、诊断 ID、重试 | 仅可恢复动作可用 |
| `recovery` | 启动页恢复卡片 | 恢复不覆盖主文件，恢复副本标 dirty |
| `readOnly` | 全局只读反馈和工具栏标识 | 禁写；允许浏览、试听、导出当前快照 |
| `fatal` | 独立致命错误页 | 显示诊断并安全返回启动页 |

mock 的加载、保存、恢复和取消确认通过 `QTimer::singleShot(0)` 发布后续快照；generation token 丢弃过期回调。取消严格经历 `running → cancelling → cancelled/idle`，不把发出取消请求等同于 worker 已终止。ViewModel 接受来自任意 service 线程的快照，并通过 queued invoke 在自身线程更新模型和发信号。

## 6. Scene Graph 集成

`PreviewPanel.qml` 只提供带稳定对象名的布局宿主 `previewSceneGraphHost`。C++ `attach_scene_graph_render_item()` 在宿主下创建真实的 `rendering::SceneGraphRenderItem`，同步尺寸和可见性；QML 不接收 `RenderSnapshot`、`FrameIndex`、顶点、批次或渲染线程资源，也没有第二套几何生成代码。

应用初始进入启动页时预览宿主尚不存在。入口监听 ViewModel 状态变化，并在 Loader 切到工作区后延迟挂载；返回启动页再进入工作区时会对新的宿主重新挂载。屏上几何仍由 A-022 的 C++ Scene Graph 实现负责。

## 7. 稳定对象名与可访问性

关键自动化锚点包括：

| 区域 | 稳定 `objectName` 示例 | 可访问语义 |
|---|---|---|
| 壳层/页面 | `applicationShell`、`applicationPageLoader`、`startPage`、`workspacePage`、`fatalErrorPage` | 页面名称和当前状态描述 |
| 主动作 | `createProjectButton`、`openProjectButton`、`analyzeStageButton`、`saveStageButton`、`exportStageButton` | 名称、用途、禁用原因 |
| 工作区 | `assetPanel`、`previewPanel`、`timelinePanel`、`inspectorPanel`、`taskDrawer` | 面板用途和键盘提示 |
| Scene Graph | `previewSceneGraphHost`、`sceneGraphRenderItem` | 图形预览角色和格式化当前时间 |
| 恢复/错误 | `recoveryPanel`、`recoverAutosaveButton`、`statusRetryButton`、`fatalDiagnosticId`、`fatalReturnButton` | 安全性、恢复动作和诊断信息 |
| 动态列表 | `assetRow_<assetId>`、`taskRow_<requestId>`、`taskCancel_<requestId>`、`templateParameter_<name>` | 行内容、状态、进度和动作结果 |

所有关键按钮、列表、画布和面板均提供 `Accessible.name`；需要解释禁用、只读、取消或恢复语义的控件同时提供 `Accessible.description`。布局使用 Qt Quick Layouts 和逻辑像素，保留 960×640 最小窗口并由 Qt 承担设备像素缩放；高 DPI/焦点的完整检查仍属于 T-026。

## 8. 测试入口与结果

| CTest 名称 | 实现 | 覆盖 |
|---|---|---|
| `qt.ui_bridge` | `tests/qt/ui_bridge_test.cpp` | 版本、元对象 64 位边界、模型角色、异步加载/取消、失败/恢复/只读/致命、模板整数、Scene Graph 挂载 |
| `qml.ui_shell` | `tests/qml/t025/tst_workspace_shell.qml` | 页面组件、状态显示、稳定对象名、可访问名称、恢复和致命页 |
| `app.qml_smoke` | 实际 `space-rhythm.exe --smoke` | QML module、注入 ViewModel、工作区 Loader 和真实 Scene Graph 宿主挂载 |

三项均设置 `QT_QPA_PLATFORM=offscreen`，并在当前企业签名策略主机使用不依赖 `Qt6QuickEffects.dll` 的 Qt Quick Controls Basic 样式。Debug 最终 `ctest -L '^t025$'` 为 3/3 通过；`space_rhythm_app_qmllint` 通过。完整命令和追溯见 [T-025 验证摘要](../evidence/T-025/verification-summary.md)。

## 9. 待确认和后续边界

以下内容没有因实现工程骨架而获得产品批准：

- 视觉风格、品牌色、图标和最终动效；当前颜色只用于工程可读性。
- 产品文案和本地化术语；当前中文是工程占位文案。
- 默认音色与合法音色素材；本任务未嵌入产品音色。
- 发布导出容器、编码器、质量预设和许可证；界面明确显示“格式待确认”。
- 产品默认视觉模板与参数；当前值来自 A-022 版本化工程模板，并明确不是产品默认。
- 基准硬件、高 DPI/窗口、可访问性和性能验收门槛。

T-025 已完成工程外壳和桥接条件。T-026 仍为 `todo`，本轮未实现时间线编辑、撤销重做、完整用户路径或高 DPI/焦点验收；T-035 现在仅获得其 T-025 前置事实，不在本任务中启动。
