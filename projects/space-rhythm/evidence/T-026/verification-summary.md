# T-026 验证摘要

- 日期：2026-09-12
- 执行人：ui-engineer-qt-quick-01
- 成果：A-027 0.1；UI bridge 0.2.0/schema 2；`IntegratedWorkspaceService`；真实六阶段路径；时间线事务；T-019 时钟和安全导出；高 DPI/焦点/可访问性测试
- 环境：Windows x64；MSVC 19.44；Qt 6.11.2 shared Debug；FFmpeg 8.1.2#3；CMake/Ninja；Qt Quick software headless 后端。

## 最终结果

1. 定向构建通过：`space_rhythm_app`、`space_rhythm_ui_bridge_tests`、`space_rhythm_ui_integration_tests`、`space_rhythm_ui_qml_tests`、`space_rhythm_audio_render_tests`。
2. QML 静态检查：`space_rhythm_app_qmllint` 最终通过，无手写 QML 诊断；取消对话框的 Accessible 属性已移动到其 Item 内容，解决首次 lint 的 attached-property 类型警告。
3. T-026 真实集成测试通过 CTest 排除已存在的 `golden-media` 重新生成 fixture 后运行：1/1 passed（Qt Test 内部 7 passed、0 failed），耗时约 2.0 s。覆盖后台真实媒体导入、真实 PCM/DSP 分析、单选/Ctrl 多选、缩放、平移、手工事件、拖动 coalescing、锁定、`BatchOffsetEvents`、撤销/重做、T-019 C++ 播放/seek/pause/resume、保存/重开、冻结快照、软件离屏逐帧、T-032 PCM、FFmpeg NUT 临时文件提交、取消确认语义及活动 worker 断线后丢弃迟到结果。
4. Qt/QML headless 组合：`qt.ui_bridge`、`qml.ui_shell`、`qml.ui_shell_high_dpi`、`app.qml_smoke` 最终 4/4 通过。QML 用例每个进程 7/7；高 DPI 进程设置 `QT_SCALE_FACTOR=1.5`，覆盖 960×640/1600×1000、稳定对象名、Accessible 信息和显式 Tab 顺序。
5. T-019 回归：`space_rhythm_playback_export_tests` 9/9 通过，包含 audio played samples 主时钟、C++ monotonic pause/resume/seek、冻结深复制/旧修订拒绝、取消/worker interruption/编码失败下保留目标及删除临时文件。
6. 核心/系统回归：事务向量、时间线不可变快照、JobCoordinator、ProjectStore 定向 10/10 通过。
7. 媒体回归：探测、PCM 解码、取消生命周期定向 5/5 通过；DSP/adapter 全套 20/20 通过；源 PCM 已接入 QAudioSink，设备可用时只以 `processed_frames()` 驱动 T-019 audio-sample 时钟，设备不可用/headless 使用 C++ monotonic 回退；不可用设备负向测试 1/1 通过。
8. T-035 软件离屏契约：`rendering.offscreen_software_contract` 1/1 通过；与真实工作流、bridge、普通/1.5× QML 和 app smoke 合并复跑为 6/6。
9. 源码边界扫描：应用 QML 中没有 Animation/Timer、raw `frameIndex`、raw revision 或 `timeNs` 属性；64 位权威值和 RenderSnapshot 只通过非 meta-object C++ accessor 进入两个 SceneGraph item。
10. 框架只读校验通过：17 个岗位、24 份知识、1 个实际项目、1 套项目模板、600 处本地链接。

## 环境事实

`qt.ui_real_workflow` 已登记为带 `golden-media` fixture 的 CTest 入口。当前主机单独执行该 CTest 时，fixture 生成器调用仓库固定 FFmpeg CLI 在进程启动前返回 `0xC0E90002`，与项目既有 T021-ENV-001/WDAC-SAC 类环境限制一致；因此该次 CTest 被依赖关系标为未运行。没有把它记成通过，也没有移除正确的 fixture 依赖。

同一现有黄金媒体由链接的 FFmpeg 库完成真实探测/解码，T-026 测试程序直接运行 7/7；媒体 5/5、DSP 20/20、T-019 9/9 及软件离屏 1/1 也实际运行通过。该 CLI 启动策略问题不改变 Debug 功能结论，但 CI/Release 和可重新生成黄金素材的完整门禁继续保持未宣称通过。

## 完成条件追溯

| T-026 条件 | 证据 |
|---|---|
| 导入→分析→编辑→预览→保存→导出 | `realImportAnalysisEditPreviewSaveAndExportPath`；A-027 第 1 节 |
| 缩放、拖动、锁定、批量偏移、撤销/重做 | 同一集成测试；核心事务回归 10/10；A-027 第 3 节 |
| T-019 C++ 播放时钟 | ViewModel 只显示格式化值；T-019 9/9；QML authority 扫描 |
| 作业进度、取消确认、断线与恢复 | `cancellationWaitsForWorkerTerminalAcknowledgement`、`activeWorkerDisconnectIsRecoverableWithoutPublishingLateResults`；QML 对话框/重连断言 |
| 冻结快照和安全导出 | 真实测试导出文件；同修订日志 `r8 == recipe 8`；T-019 事务 6/6 相关用例；T-035 软件路径 |
| 高 DPI、缩放、焦点、可访问性 | 两个 QML 进程最终各 7/7，含 1.5×和显式 Tab 映射 |
| 自测、任务及交接更新 | 本摘要、A-027、T-026 和 H-006 阶段证据 |

## 待确认边界

视觉风格、产品文案、产品默认音色、发布导出格式、产品默认模板参数、设备矩阵及可访问性/性能门槛继续待确认。当前 NUT/raw RGBA/PCM 明确为 T-019 `testOnly` 开发格式，不构成发布决策。
