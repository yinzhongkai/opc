# 批量 Scene Graph 几何与三类视觉模板实现

- 项目：space-rhythm
- 成果 ID：A-022
- 负责人：graphics-engineer-qt-scenegraph-01
- 关联任务：T-034
- 版本：0.1
- 更新日期：2026-09-11
- 状态：draft
- 适用范围：实现高密度事件时间线、波形/示波器、频谱几何和节奏线条脉冲的共享几何核心及 `QQuickItem + QSGGeometryNode` 屏上适配；不实现 T-035 的离屏驱动、像素一致性、GPU 能力降级或性能门槛，也不负责产品视觉风格、音频分析和媒体编码。
- 来源及输入版本：A-004 0.5、A-005 0.4、A-006 0.1 WP-07、A-011 0.1、A-012 0.1、A-018 0.2、A-021 0.1；D-003 confirmed；T-012/T-013/T-014/T-030/T-033 completed。
- 批准依据：尚无。任务完成不自动批准本成果；本轮默认值是模板 `1.0.0` 的工程默认值，不替代产品视觉确认。
- 版本记录：2026-09-11，0.1，首次实现 geometry schema 1/contract 0.1.0、三种模板 1.0.0、批量动态顶点更新、裁剪/LOD、稳定种子和 device generation 重建。

## 1. 交付结论

实现拆成两个公开目标：

| 目标 | 职责 | 依赖边界 |
|---|---|---|
| `SpaceRhythm::RenderingGeometry` | 验证模板参数，从不可变 `RenderSnapshot` 生成后端无关的三角形批次和更新计划 | 纯 C++，只依赖 T-033 `RenderingContract`，不包含 Qt/GPU 句柄 |
| `SpaceRhythm::SceneGraph` | 在 `SceneGraphRenderItem::updatePaintNode()` 中消费几何帧，创建/复用 `QSGGeometryNode` 并批量上传彩色顶点 | Qt 6.11.2 Core/Gui/Quick 公共 API |

屏上 item 和后续离屏适配都必须调用同一个 `build_geometry_frame()`。T-034 没有新增第二套“导出几何”逻辑，也没有创建 `QQuickRenderControl`、render target、读回或编码路径。

## 2. 版本和确定性边界

- `geometrySchemaVersion = 1`，`geometryContractVersion = 0.1.0`。
- 三种模板 ID 分别为 `space-rhythm.waveform-oscilloscope`、`space-rhythm.spectrum-geometry`、`space-rhythm.rhythm-line-pulse`，当前版本均为 `1.0.0`。
- 参数块规范串包含 schema、模板 ID、模板版本及按名称排序的全部整数参数；`parametersDigestSha256` 对该规范串计算 SHA-256。缺字段、多字段、越界、摘要不符、未知模板、旧模板版本、required feature 或 extension 均 fail closed。
- 随机种子继续由 A-021 `RenderRecipe.deterministicSeed: UInt64` 版本化；脉冲方向和抖动仅由 seed、稳定 event ID 与 particle index 派生，不读取全局随机状态、时钟或容器地址。
- snapshot ID 或 timeline revision 与请求期望值不一致返回 `stale_revision`，禁止把旧快照几何写入新帧。

## 3. 模板 1.0.0 参数、范围和工程默认值

所有可调量均为整数，比例使用 ppm，线宽/最小高度使用千分之一逻辑像素，颜色使用 `0xRRGGBBAA`。

| 参数 | 范围 | 1.0.0 默认值 | 使用模板 |
|---|---:|---:|---|
| `event-marker-width-milli-px` | 500～16,000 | 2,000 | 全部 |
| `event-rgba` | `0x00000000`～`0xffffffff` | `0xffd166ff` | 全部 |
| `lod-samples-per-pixel` | 1～64 | 4 | 全部 |
| `max-total-vertices` | 6,000～4,000,000 | 786,432 | 全部 |
| `max-vertices-per-batch` | 600～1,200,000 | 65,532 | 全部 |
| `primary-rgba` | `0x00000000`～`0xffffffff` | `0x36d8ffff` | 全部 |
| `secondary-rgba` | `0x00000000`～`0xffffffff` | `0x9068ffff` | 全部 |
| `timeline-height-ppm` | 50,000～400,000 | 180,000 | 全部 |
| `amplitude-ppm` | 100,000～1,000,000 | 900,000 | 波形、频谱 |
| `line-width-milli-px` | 250～8,000 | 1,500 | 波形、脉冲 |
| `bar-gap-ppm` | 0～900,000 | 150,000 | 频谱 |
| `minimum-bar-height-milli-px` | 0～10,000 | 1,000 | 频谱 |
| `jitter-ppm` | 0～1,000,000 | 250,000 | 脉冲 |
| `lifetime-ms` | 16～5,000 | 750 | 脉冲 |
| `particles-per-event` | 1～32 | 8 | 脉冲 |
| `travel-distance-ppm` | 10,000～1,000,000 | 250,000 | 脉冲 |

`max-total-vertices` 必须不小于 `max-vertices-per-batch`。改变参数定义、范围或默认值必须提升模板版本并形成新的参数摘要和 RenderSnapshot；不得静默改变既有 1.0.0 的含义。

## 4. 高密度时间线和 LOD

时间坐标复用 A-021 的半开 viewport 与整数 subpixel 转换。可见事件先按目标 x 像素聚合，每个像素只保留强度最高、稳定 ID 排序最大的代表事件，并保留桶内计数；每个占用像素输出一个 6 顶点矩形。50,000 个事件在 64 px 视口中最多输出 64 个时间线 primitive，而不是 50,000 个 QObject/QML item。

波形使用每 x 像素的 min/max envelope，一桶一个 6 顶点线条 quad；100,000 个采样在 128 px 视口中最多输出 128 个波形 primitive。频谱在目标帧时间取每条 series 的最近稳定值，series 多于像素宽度时分组取峰值。统计中的 `lodLevel` 记录最密桶或频谱分组相对 `lod-samples-per-pixel` 的降采样级别。

## 5. 三类模板几何

1. 波形/示波器：上部可视区按像素绘制 min/max envelope，底部保留事件时间线。
2. 频谱几何：series 映射为交替主/辅色柱，超宽数据分组取峰值，底部保留事件时间线。
3. 节奏线条脉冲：仅绘制帧时间前且处于版本化 lifetime 内的事件；从事件像素位置按稳定方向、seed 抖动、强度和年龄生成线条 quad，底部保留事件时间线。

三类都输出 `GeometryBatch{kind, vertices}`，拓扑固定为非索引三角形，每个 primitive 6 顶点；单批大小按 6 对齐并受版本化批次/总顶点预算约束。

## 6. 裁剪和批量上传

- 请求 viewport 必须是 recipe 时间范围的非空子集，时间范围继续采用 `[start,end)`。
- 事件和采样先按时间裁剪，再映射到目标像素；矩形、线段和线宽扩展后的顶点最终都钳制在 `[0,width] x [0,height]`。
- CPU 侧只生成可见几何；不存在“一点一个 QML Item”或“一事件一个 QObject”。
- `QSGGeometry` 使用 `defaultAttributes_ColoredPoint2D()`、`DrawTriangles` 和 `DynamicPattern`。同步时按批次一次 `allocate()`、连续写入全部顶点、`markVertexDataDirty()` 并标记 `DirtyGeometry`。
- 同一 device generation 和批次数量下复用既有 node；批次缩减时删除尾节点，增加时追加节点。

## 7. UI、渲染线程和设备生命周期

`SceneGraphRenderItem` 设置 `ItemHasContents`。UI setter 只在互斥保护下替换 `shared_ptr<const RenderSnapshot>`、viewport 和 frame index，然后调用 `update()`；不保存 QSG/GPU 裸指针。

`updatePaintNode()` 是进入场景图的唯一同步点：渲染线程复制一次 pending 状态，调用共享几何核心，再创建或更新节点。无快照、零尺寸、资源已失效或请求验证失败时，渲染线程删除旧节点并返回空。节点拥有 geometry/material，子节点由父节点拥有。

`sceneGraphInitialized` 和 `sceneGraphInvalidated` 使用 `DirectConnection` 在信号发出线程更新原子状态。每次初始化递增 device generation；`GeometryBatchUpdatePlan` 检测 generation 不同后要求删除旧 generation 的全部节点并重建。T-034 不创建节点树之外的纹理、render target 或其他 GPU 对象，`releaseResources()` 因而不在 UI 线程删除 GPU 资源。

## 8. 屏上/离屏复用边界

共享核心输入完整绑定 snapshot ID、timeline revision、viewport、尺寸、frame index 和 device generation，输出携带模板 ID/version/参数摘要及精确 frame time。屏上 `SceneGraphRenderItem` 只是该输出的一个消费者。

T-035 可以在公共 `QQuickRenderControl` 生命周期中消费同一 `GeometryFrame`，但不得重新解释事件、特征、LOD、seed 或裁剪。T-034 没有实现离屏 session、GPU 能力探测、软件降级、像素读回、编码桥接或性能阈值。

## 9. 可执行测试向量

CTest 标签为 `t034;unit;rendering;headless;gate-g1;gate-g3`，共 11 项：

| 向量 | 覆盖 |
|---|---|
| `RGV_TEMPLATE_001` | 三模板稳定默认值、固定 SHA-256、范围、摘要篡改、未知参数和旧版本拒绝 |
| `RGV_EMPTY_001` | 空事件/空特征不生成伪几何 |
| `RGV_CLIP_001` | viewport 半开边界和顶点目标范围 |
| `RGV_DENSE_001` | 50,000 事件按 64 px 聚合、LOD 和批次上限 |
| `RGV_DENSE_002` | 100,000 波形采样按 128 px min/max envelope |
| `RGV_TEMPLATES_001` | 波形、频谱、脉冲三类批量三角形 |
| `RGV_SEED_001` | 同请求/同 seed 相等、不同 seed 脉冲不同、屏上/离屏同源输入 |
| `RGV_STALE_001` | 旧 snapshot ID 返回 `stale_revision` |
| `RGV_GENERATION_001` | 后端无关更新计划的批次复用与 generation 重建 |
| `RGV_QSG_001..002` | 实际 QSG node 创建/复用/generation 重建及动态顶点原位更新 |

Windows x64 Debug 在 MSVC 19.44、Qt 6.11.2、CMake 3.31.6、Ninja 1.12.1、`/W4 /WX` 下全目标构建通过；最终 `ctest -L '^t034$'` 11/11 通过、0 失败、总实时时间 11.82 s。新生成测试程序的早期运行曾被既有 WDAC/SAC 在进程启动前标记 `BAD_COMMAND`；重建/重试后未使用 fallback 的最终合并专项由最新二进制真实执行 11/11，通过记录未把早期环境阻断改写成断言失败。最终时间包含首个 QSG 测试约 10 s 的主机启动延迟，不作为渲染性能结论。

## 10. 保留项

- 工程默认颜色、布局和数量尚未获得产品批准；不声明视觉效果达标。
- 未确认基准 GPU、像素容差、帧预算、显存/上传阈值或低能力设备矩阵；不声明 G0～G4 或性能门禁通过。
- `SceneGraphRenderItem` 提供 C++ 适配边界，未替 T-024 修改 QML 页面或产品工作流。
- T-035 保持 `todo`，本轮没有创建离屏渲染器、降级策略或性能/像素证据。
