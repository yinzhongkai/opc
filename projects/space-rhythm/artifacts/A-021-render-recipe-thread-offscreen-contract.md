# RenderRecipe、Scene Graph 线程与离屏帧契约 0.x

- 项目：space-rhythm
- 成果 ID：A-021
- 负责人：graphics-engineer-qt-scenegraph-01
- 关联任务：T-033
- 版本：0.1
- 更新日期：2026-09-11
- 状态：draft
- 适用范围：定义屏上预览与离屏输出共用的版本化 `RenderRecipe`、不可变渲染快照、Qt Quick Scene Graph 同步边界、GPU 资源 epoch、坐标/命中接口、离屏帧所有权和有界队列语义；不实现 T-034 的时间线、波形、频谱或粒子模板，不冻结视觉参数，不负责媒体编码。
- 来源及输入版本：A-004 0.5、A-005 0.4、A-006 0.1 WP-07、A-011 0.1、A-012 0.1、A-014 0.4、A-018 0.2；D-003 confirmed；T-013/T-014/T-017/T-030 已完成；T-024 的 UI 工作流边界。
- 批准依据：尚无。任务完成不自动批准本成果。
- 版本记录：2026-09-11，0.1，首次冻结 contract/schema 0.x、线程/资源边界、离屏帧和 18 个可执行契约向量。

## 1. 结论和边界

本契约新增纯 C++ `SpaceRhythm::RenderingContract`，公开头文件为 `src/rendering/include/space_rhythm/rendering/render_contract.hpp`。`renderContractVersion = 0.1.0`、`schemaVersion = 1`、`vectorSetVersion = 1`。屏上和离屏消费者必须读取同一份 `shared_ptr<const RenderSnapshot>`，不得分别读取可变 ViewModel、算法工作缓冲或当前时间线并自行拼配。

Qt 适配层只使用 Qt 6.11.2 公共 API：`QQuickItem::updatePaintNode()`、`QSGGeometryNode`/`QSGNode`、`QQuickWindow::scheduleRenderJob()`、`sceneGraphInvalidated`、`QQuickRenderControl` 和 `QQuickRenderTarget`。契约不包含 `QRhi`、`QSGRenderContext` 或其他 Qt 私有头/符号，也未实现任何 T-034 几何、材质、shader 或视觉模板。

## 2. 版本化 `RenderRecipe`

`RenderRecipe` 是一次渲染的完整、可审计输入：

| 字段组 | schema 1 语义 |
|---|---|
| 版本 | `schemaVersion=1`、`renderContractVersion=0.1.0`、有序唯一 `requiredFeatures`、可保留的 namespaced optional extensions |
| 身份和核心修订 | `recipeId`、`projectId`、`timelineRevision` |
| 特征输入 | 每项含 `analysisRevision`、生产者 contract/schema、输入/参数/内容 SHA-256；按 revision ASCII 升序且唯一 |
| 模板参数 | `templateId`、`templateVersion`、参数摘要、整数参数和 required features；T-034 另行定义具体模板及范围 |
| 输出 | 正整数像素尺寸；schema 1 只接受 `rgba8_unorm`、sRGB primaries/transfer、RGB matrix、full range、top-down；alpha 为 straight 或 opaque |
| 时间 | A-012 半开 `TimeRange [startNs,endNs)`；`FrameRate {numerator,denominator}` 为正有理数 |
| 确定性 | `deterministicSeed: UInt64`，任何随机几何都只能从该值和版本化对象身份派生 |

帧 `i` 的唯一展示时间是：

```text
timeNs(i) = startNs
          + nearest_ties_to_even(i * fpsDenominator * 1_000_000_000 / fpsNumerator)
```

结果必须落在配方半开时间范围内；溢出或越界拒绝，不通过累计浮点帧时长推进。`30000/1001` 的第 1 帧为 `33,366,667 ns`。

schema 和 contract 独立演进。当前读者只接受 schema 1 与 contract `0.1.0`：未知 schema/contract、未知 required feature、重复 feature 或非法扩展 fail closed；未知但结构有效的 optional extension 原样保留。新增可选字段提升 minor，改变必填/语义或兼容边界提升 major；0.x 阶段不推定跨版本兼容。

## 3. 不可变渲染快照

`make_render_snapshot()` 在 UI/应用线程执行，先验证并深拷贝：

1. 经 A-012 规范化的 `TimelineSnapshot`；`projectId/timelineRevision` 必须和配方完全相等。
2. 面向显示的 `RenderSeries` 投影；每条 series 的 analysis revision、生产 contract/schema、输入/参数/内容摘要必须逐项命中配方。
3. series ID 有序唯一；样本按 `timeNs` 严格升序、处于配方半开时间范围，强度使用 `NormPpm`，不携带 NaN/无穷。

成功结果只暴露 `shared_ptr<const RenderSnapshot>` 和 const getter。调用者后续修改原始 recipe、timeline 或 series 不影响快照；渲染线程不得反向持有或访问可变 UI 模型。新时间线 revision、新分析 revision、模板参数摘要、尺寸、时间范围、帧率或 seed 都必须创建新 recipe 和新 snapshot ID，旧快照可被在途帧安全持有至自然释放。

图形层只消费 A-018 已版本化的稳定特征投影，不解释 PCM、不重算 FFT、不改变分析置信或核心事件语义。

## 4. UI、渲染线程和 GPU 资源生命周期

| 阶段 | 线程 | 允许动作 | 禁止动作 |
|---|---|---|---|
| recipe/snapshot 构造 | UI/应用线程或普通 worker | 验证、排序、深拷贝、生成新不可变快照 | 创建/修改 `QSGNode`、GPU 资源 |
| `QQuickItem` setter | UI 线程 | 仅替换 pending `shared_ptr<const RenderSnapshot>`，记录 item 几何/状态，调用 `update()` | 保存 `QSGNode*`、写 geometry/material、等待 GPU |
| `updatePaintNode()` 同步 | 渲染线程，GUI 线程被 Qt 阻塞 | 读取一次 pending 快照；比较 snapshot/revision/digest/尺寸；创建或更新节点、geometry、material；设置 dirty flags | 回调 UI、访问可变 QObject/ViewModel、跨同步点保留 UI 裸指针 |
| draw | 渲染线程 | 只读快照和当前 device generation 下的资源 | 分配无界业务对象、修改 UI 状态 |
| `releaseResources()` | UI 线程 | 标记释放并用 `QQuickWindow::scheduleRenderJob()` 安排渲染线程清理 | 在 UI 线程删除 GPU/QSG 资源，使用 `deleteLater()` 延迟 GPU 清理 |
| scene graph invalidation | 发出信号的渲染线程，DirectConnection | 立即丢弃当前 generation 的全部 GPU 句柄/缓存并使在途离屏 session 失败 | 复用旧 generation 资源，假定 context/device 仍存在 |

`SceneGraphLifecycle` 的公共状态机是：

```text
detached -> awaiting_initialization -> ready(generation + 1)
ready -> cleanup_pending -> awaiting_initialization
ready/cleanup_pending/awaiting_initialization -> invalidated
invalidated -> ready(generation + 1) | detached
```

非法转换拒绝。每次 scene graph 初始化/重建都递增 `deviceGeneration`；CPU recipe/snapshot 不依赖设备，可以复用，但节点、材质、纹理、render target 和上传缓存只能在创建它们的 generation 内使用。

## 5. `QQuickItem/QSGGeometryNode` 更新边界

T-034 的实现必须遵守以下适配边界：

1. 自定义 item 设置 `ItemHasContents`；QML/GUI 侧仅持有业务属性和不可变快照，不公开节点或 GPU 句柄。
2. `updatePaintNode(oldNode, data)` 是 UI 状态进入场景图的唯一同步点。一次调用只读取同一个 snapshot ID；若同步期间 UI 已产生更新，只能等待下一次同步，不混合修订。
3. 创建、替换、挂接和修改 `QSGGeometryNode` 及其 geometry/material 只发生在渲染线程。geometry 顶点或索引改变后标记 `DirtyGeometry`，材质/统一量改变后标记 `DirtyMaterial`，拓扑/子节点改变标记相应节点 dirty。
4. 节点树使用 `OwnedByParent`；geometry/material 由节点拥有时显式设置 `OwnsGeometry`/`OwnsMaterial`。非节点树资源另设 generation 所有者并走渲染 job/invalidation 清理。
5. 差量更新键至少包含 snapshot ID、timeline revision、全部 feature revisions/digests、模板 ID/version/参数摘要、输出尺寸、可见时间范围和 seed。任一键变化不得沿用语义不匹配的批次。
6. 一个事件/采样点一个 QML item 不符合边界；批量 geometry/纹理策略属于 T-034。空输入、不可见或 item 无尺寸时允许无内容节点，但不能发布伪造命中结果。

Qt 自动在正确时机/线程删除从 `updatePaintNode()` 返回并留在节点树中的节点；应用只负责按所有权 flags 管理子对象。窗口外资源必须显式处理 `releaseResources()` 和 `sceneGraphInvalidated`。

## 6. 离屏渲染、帧格式和所有权

离屏适配使用公共 `QQuickRenderControl` 生命周期：初始化后按 `polishItems -> beginFrame -> sync -> render -> endFrame` 驱动；GPU 后端的 sync/render 包含在 begin/end frame 内。`QQuickRenderTarget` 不拥有调用者传入的原生 render target，创建者必须让该资源存活至 render control 不再引用并在正确渲染线程/generation 释放。软件后端差异由 T-035 验证，不能暗中改变帧契约。

屏上和离屏路径复用同一 recipe/snapshot、帧时间函数和后续 T-034 渲染核心。离屏不得读取“当前 UI 状态”覆盖已提交快照。

`RenderedFrame` schema 1：

| 字段 | 约束 |
|---|---|
| 身份 | snapshot ID、timeline revision、正 `deviceGeneration`、`frameIndex` |
| 时间 | `timeNs` 必须逐位等于 recipe 的 `frame_time_ns(frameIndex)` |
| 格式 | recipe 同尺寸；`rgba8_unorm`；sRGB/RGB/full；straight 或 opaque alpha；top-down |
| 布局 | `strideBytes >= width*4`，允许行尾 padding；`validBytes = strideBytes*height`；lease 至少覆盖 validBytes |
| 所有权 | `FrameLease` 是进程内不可变引用计数字节；发布时每帧必须独占一个未发布 lease，发布后只读，可跨普通消费线程持有 |

进程内指针/lease 不直接跨 IPC。媒体编码消费者若在另一个进程，必须通过后续版本化共享内存、受控文件或复制适配传输，并保留本契约的格式、步幅、时间、snapshot/revision 和 generation 元数据。

## 7. 背压、结束、取消和设备丢失

`BoundedFrameQueue(maxOutstandingFrames,maxOutstandingBytes)` 同时按帧数和 `validBytes` 限流。容量计数从 `publish()` 接受开始，持续到该帧最后一个 `FrameLease` 引用释放；仅 `try_take()` 出队不会释放容量。满载返回 `would_block`，生产者暂停/让出并观察取消，不丢帧、不覆盖旧帧、不忙等。

- `drain()`：禁止新发布；队列已交付且所有 lease 释放后进入 `ended`。消费者持有 lease 时状态保持 `draining`。
- `cancel()`：原子进入 `cancelled`，使生产者/消费者在下一次队列调用观察终态并丢弃尚未交付帧；已经交付的 lease 字节仍有效到最后引用释放。当前接口是非阻塞队列，不承诺条件变量唤醒。取消不伪装成正常 EOF，也不发布部分帧冒充完整结果。
- `fail(device_lost)`：记录失败的 device generation 和稳定 diagnostic ID，丢弃未交付帧、拒绝该 session 后续发布；已交付 lease 的 CPU 字节仍有效。恢复必须等待新 scene graph generation，并以新离屏 session 重新提交未完成 frame index；不得混接旧/新 generation。
- 普通 `render_failed` 与 `device_lost` 分开报告。队列失败不改变 recipe/snapshot；是否重试由上层根据错误、取消状态和输出事务决定。

## 8. 坐标转换和命中接口

坐标使用 `1 logical pixel = 1024 subpixels` 的有符号整数，不以 `qreal` 作为持久契约。`CoordinateTransform` 绑定 snapshot ID、timeline revision、半开时间范围和 item content rect。

```text
xSp(t) = rect.xSp + floor((t - startNs) * rect.widthSp / (endNs - startNs))
t(xSp) = startNs + floor((xSp - rect.xSp) * (endNs - startNs) / rect.widthSp)
```

中间乘法使用 checked 128-bit 等价运算。几何转换允许 `t=endNs <-> x=rightSp` 表示右边界；实际命中只接受 `[left,right) x [top,bottom)`，事件/帧语义继续使用 `[start,end)`。

`HitTestRequest` 必带 schema、snapshot ID、timeline revision、item 坐标和非负 radius；任一 revision/ID 不一致返回 `stale_revision`。`HitTestResult` 必须绑定同一 request/snapshot，candidate 距离不得超过 checked `radiusSp²`；candidate 含 kind、稳定 target ID、timeNs、整数 distance squared 和 z-order，排序固定为：`zOrder` 降序、距离升序、kind（event/series_sample/track）、timeNs、target ID ASCII。命中只读已同步的同一快照，不修改选择、时间线或 UI 状态。

## 9. 错误和兼容语义

本模块复用 A-012 `ErrorInfo`：结构/范围/stride/所有权错误为 `validation/invalid_dto`；时间不一致为 `validation/timestamp_mismatch`；snapshot/feature/命中修订不一致为 `conflict/stale_revision` 且 retryable；schema/contract 或 required feature 不支持为 `compatibility/unsupported_schema|unsupported_feature`；溢出/容量为既有 `time_overflow|resource_limit`。错误 context 不默认记录帧字节、用户媒体或完整路径。

缓存键至少包含 recipe/snapshot ID、所有 revisions/digests、模板版本/参数摘要、尺寸/颜色、时间范围/fps、seed 和实现/设备能力版本。设备 generation 只参与 GPU/帧有效性，不改变 CPU 快照内容身份。

## 10. 可执行契约向量

测试入口为 `tests/contract/render_public_contract_test.cpp`，通过 CTest 标签 `t033;contract;rendering;headless;gate-g2;gate-g3;gate-g4` 运行。vector set 1 共 18 项：

| 向量 | 覆盖 |
|---|---|
| `RCTV-VERSION-001..004` | 当前版本和 optional extension；未来 schema、未知 required feature、不同 contract 拒绝 |
| `RCTV-RECIPE-001` | 完整 recipe、revision/feature/template/output/time/fps/color/seed |
| `RCTV-SNAPSHOT-001..003` | 深拷贝不可变；timeline revision 和 feature digest stale fail closed |
| `RCTV-TIME-001` | `30000/1001` 最近偶数帧时间及半开范围 |
| `RCTV-COORD-001` | 左/中/右边界、逆映射和越界 |
| `RCTV-HIT-001` | revision 校验和稳定排序 |
| `RCTV-LIFECYCLE-001..002` | 非法转换、失效重建 generation、显式 cleanup |
| `RCTV-FRAME-001` | frame timestamp、格式、stride、valid bytes、lease |
| `RCTV-QUEUE-001..004` | lease 级背压、取消、设备丢失、drain/ended |

## 11. 自检与后续边界

- Windows x64 Debug 使用 MSVC 19.44、Qt 6.11.2、CMake/Ninja 构建通过，`/W4 /WX` 下无警告。
- `RenderContractVectors.*` 18/18 通过；兼容、不可变、坐标、命中、生命周期、离屏帧和队列均有正负向证据。
- 源码和 CMake 未包含 Qt 私有 API，也没有新增实际 QQuickItem、QSGGeometryNode、shader、模板参数默认值或 GPU 性能结论。
- T-034 仍为 `todo`，需后续明确启动后按本契约实现批量节点；T-035 再验证真实屏上/离屏像素一致性、GPU 降级、设备矩阵和性能阈值。
