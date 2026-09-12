# T-026 验证摘要

- 日期：2026-09-12
- 执行人：ui-engineer-qt-quick-01
- 修订范围：用户明确重开 T-026 后修复 `T026-DEFECT-001`～`004`
- 成果：A-027 0.2；UI bridge 0.3.0/schema 3；真实 UI/worker 双进程；pending 手势事务；事件音轨试听/导出；异步保存与有界分块混音
- 环境：Windows x64；MSVC 19.44；Qt 6.11.2 shared Debug 与 CI/RelWithDebInfo；FFmpeg 8.1.2#3；CMake/Ninja；Qt Quick software/offscreen。

## 最终结果

1. `Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Build -UseExistingDependencies` 与 `-Preset ci-windows-msvc-x64` 均通过；应用、worker、UI bridge、Qt/QML 测试和全部关联目标成功编译，`/W4 /WX` 未产生项目源码告警。
2. 既有黄金媒体直跑 `space_rhythm_ui_integration_tests`：Debug 与 CI/RelWithDebInfo 均为 Qt Test 9 passed、0 failed，分别约 6.8 s 和 7.6 s。七个测试函数覆盖结果 codec/引用校验、真实 worker 导入、真实 worker 取消终态、实际进程 kill 断线恢复、drag/seek pending 事务、完整六阶段路径、异步保存重开。
3. Debug 与 CI/RelWithDebInfo 的 UI/headless CTest：`qt.ui_bridge`、`qml.ui_shell`、`qml.ui_shell_high_dpi`、`app.qml_smoke`、`worker.process_smoke` 均为 5/5 通过。1.5× 高 DPI、960×640/1600×1000 resize、焦点顺序、Accessible 文案和新增稳定 `objectName` 均由 QML 用例覆盖。
4. T-032/T-019/T-035 关联回归共 22/22 通过：A-018 三个 CC0 音色登记/校验、事件映射、任意分块等价、取消无部分 PCM、QAudio 不可用回退、T-019 audio/monotonic 时钟、导出取消/失败安全事务、T-035 软件离屏。
5. T-016 IPC/JobCoordinator 与 worker 回归共 12/12 通过：版本握手、禁止 inline bulk、进度/取消/超时/崩溃、乱序/重复、断线终态及真实 worker process smoke。
6. `git diff --check` 通过；源码扫描确认应用 QML 不包含 `Animation`/`Timer` 权威时钟，不暴露 raw `TimeNs`、revision 或 frame index。T-022 未启动、未修改。

## 四项缺陷的针对性证据

| 缺陷 | 修复 | 针对性验证 |
|---|---|---|
| `T026-DEFECT-001` | drag begin 冻结 event/初始时间/base revision；move 只发布 C++ ghost；release 至多一次 `MoveEvent`；Escape 零提交；stale revision 取消。seek 使用 pending/update/commit/cancel，并在 T-019 确认后更新播放头。 | `dragAndSeekUsePendingTransactionsAndRejectStaleRevision` 验证多次 update revision 不变、release 只增一次、Escape 不变、外部修订使提交失效；seek pending 不改格式化权威时间，commit 才改变。QML 新增 ghost/status 锚点和 Escape 优先级。 |
| `T026-DEFECT-002` | 新增 `space-rhythm-worker --ui-worker`；UI 用 T-016 `LocalIpcClient` 握手和提交。`MediaSource::open/select/decode_audio`、`PcmNarrowAdapter`、`Analyzer` 只在 worker；二进制结果通过 file `DataReference` 的长度/SHA-256 回传。取消/断线由实际进程终止驱动。 | `workerResultReferencesAreVersionedAndVerifiedOutOfBand`、`realImportCompletesOffTheGuiThread`、`cancellationWaitsForWorkerTerminalAcknowledgement`、`activeWorkerDisconnectIsRecoverableWithoutPublishingLateResults`；T-016 回归 12/12。损坏结果和引用不匹配 fail closed。 |
| `T026-DEFECT-003` | 移除源 PCM 试听。产品默认音色继续待确认；只有用户显式启用后才装载 A-018 CC0 开发音色，onset/beat/manual 映射到登记 click/low/noise。试听和冻结导出使用同一事件、mapping、hash 与 T-032 mixer。 | 完整路径先断言 mapping 默认关闭且 preview/export 不可用，再显式启用；后台事件 PCM 进入 T-019 试听；导出后用真实 `MediaSource` 解码输出音轨并断言存在非零 sample。音频/T-019 安全导出回归 22/22。 |
| `T026-DEFECT-004` | 主保存和 autosave 改为后台 `ProjectStore`，用 IO epoch+mutex 串行化并抑制旧 autosave；保存只在同 revision 时 mark saved。导出视频完成后在后台按 4096 frame 调用 `render_chunk`，逐块写音频并 finish/commit。 | 完整路径和重开测试断言 `saveProjectTo()` 返回时仍 dirty，稍后才完成并清 dirty；非静音导出完成且无 `.partial`；T-032 任意分块等价/取消与 T-019 原子事务回归通过。 |

## 环境事实

标准 `ctest` 选择 `qt.ui_real_workflow` 时会按正确配置拉起 `golden-media` fixture。本主机的固定 FFmpeg CLI 仍在进程启动前返回既有 WDAC/SAC `0xC0E90002`，导致 `media.golden_generate` 失败并将真实工作流标为依赖未运行；该次结果未记为通过，也未移除 fixture 依赖。

仓库中既有、hash 已登记的 `cfr_av.mkv` 可由链接的 FFmpeg 库真实探测/解码，修订后的 Debug 与 CI/RelWithDebInfo 测试程序直接运行均为 9/9；两种配置的 UI/headless 选择集也均为 5/5。音频黄金生成器不依赖该受阻 CLI，本轮成功运行；其后音频/播放/导出/离屏 22/22 通过。由于固定 FFmpeg CLI 仍无法重新生成媒体黄金素材，包含该 fixture 的完整标准 CTest 门禁未宣称通过。

## 完成条件追溯

| T-026 条件 | 证据 |
|---|---|
| 导入→分析→编辑→预览→保存→导出 | `realImportAnalysisEditPreviewSaveAndExportPath`；输出事件音轨非静音；A-027 0.2 第 1 节 |
| 缩放、拖动、锁定、批量偏移、撤销/重做 | 完整路径 + 独立 pending/stale 手势测试；A-027 第 3 节 |
| T-019 C++ 播放时钟 | T-019 回归；QML authority 扫描；源 PCM 不再作为试听音轨 |
| 作业进度、取消确认、断线与恢复 | 外部 worker 进程、LocalIpcClient、真实 terminate/kill 测试与迟到结果拒绝 |
| 冻结快照和安全导出 | 同修订 `r8 == recipe 8`；冻结 mapping/timbre hash；4096-frame 后台块；无 partial |
| 高 DPI、缩放、焦点、可访问性 | QML 普通/1.5× 两进程通过；新增 ghost、状态、开发 mapping 锚点 |
| 自测、任务及交接更新 | 本摘要、A-027 0.2、T-026 和 H-006 修订记录 |

## 待确认边界

视觉风格、产品文案、产品默认音色、发布导出格式、产品默认模板参数、设备矩阵及可访问性/性能门槛继续待确认。A-018 CC0 映射必须显式启用且标为开发用途；当前 NUT/raw RGBA/PCM 明确为 T-019 `testOnly`，两者均不构成产品默认或发布决策。
