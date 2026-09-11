# Qt Quick 离屏渲染、降级与一致性测量

- 项目：space-rhythm
- 成果 ID：A-025
- 负责人：graphics-engineer-qt-scenegraph-01
- 关联任务：T-035
- 版本：0.1
- 更新日期：2026-09-11
- 状态：draft
- 适用范围：实现公共 `QQuickRenderControl/QQuickRenderTarget` 离屏 session、默认 D3D11 GPU 与 Qt Software 降级、A-021 帧发布、设备丢失重建、屏上/离屏像素差和 CPU/内存/上传测量；不负责媒体编码，不批准视觉或性能门槛。
- 来源及输入版本：A-021 0.1、A-022 0.1、A-024 0.1；T-025/T-032/T-034 completed；D-002～D-008 confirmed。
- 批准依据：尚无。基准 GPU、像素容差和性能阈值未确认，本成果全部数值只标记 `measured/not-evaluated`。
- 版本记录：2026-09-11，0.1，首次实现 offscreen schema/contract 0.1.0、公共 Qt/D3D11 与软件路径、generation 恢复、7 组测试和测量证据。

## 1. 交付结论

新增 `SpaceRhythm::OffscreenRendering`。`OffscreenRenderSession` 在 GUI 线程拥有 `QQuickRenderControl`、关联但不显示的 `QQuickWindow`、render target、读回资源和 A-021 `BoundedFrameQueue`。每一帧只接受 session 创建时固定的同一份 `shared_ptr<const RenderSnapshot>`，再以 snapshot 中的 recipe、时间范围、尺寸、模板参数和 seed 调用唯一的 `build_geometry_frame()`；屏上参考也调用相同核心，没有导出专用几何分支。

默认 GPU 路径使用 Qt 6.11.2 公共 `QQuickGraphicsDevice::fromDeviceAndContext()`、`QQuickRenderTarget::fromD3D11Texture()` 和 `polishItems -> beginFrame -> sync -> render -> endFrame`。D3D11 target 与 staging readback 由 session 拥有，`QQuickWindow/QQuickRenderControl` 先释放或 invalidate，随后才释放原生资源。

Qt Software 路径使用公共 `QQuickRenderTarget::fromPaintDevice()`，按 Qt 软件适配要求执行 `polishItems -> sync -> render`。实测 Qt Software 不绘制项目的通用彩色 `QSGGeometryNode`，因此降级适配使用公共 `QQuickPaintedItem/QPainter` 消费同一不可变 `GeometryFrame`；默认 GPU 和正常屏上路径仍使用 T-034 的 `QQuickItem + QSGGeometryNode` 批量上传。该差异只在后端适配层，不改变 recipe、snapshot、LOD、帧时间、seed 或几何。

实现和构建未包含 Qt private header、`QRhi` 或私有符号。

## 2. 帧、所有权、背压和取消

发布前将后端图像规范化为 A-021 `RenderedFrame`：

- `rgba8_unorm`、sRGB/RGB/full、top-down；透明输出转为 straight alpha，opaque 输出以不透明黑色清底。
- 宽高逐项等于 recipe，紧凑 `strideBytes = width * 4`，`validBytes = strideBytes * height`。
- snapshot ID、timeline revision、device generation、frame index 和 `frame_time_ns()` 结果逐项验证。
- 每帧独占一个不可变 `FrameLease`；队列容量从 publish 接受持续到最后一个 lease 引用释放。
- 满载返回 `would_block`，不覆盖或丢弃旧帧；`finish()` 进入 drain，`cancel()` 与 `CancellationToken` 保持取消终态并不发布部分帧。

软件降级属于进程级 Qt Quick 后端选择。如果已有 `QQuickWindow` 使用不兼容后端，session 返回结构化 `offscreen.initialize.backend-scope`，要求在隔离渲染 worker 启动时选择 software；不会在活动窗口之间强行切换全局后端。

## 3. 能力探测和设备丢失

`probe_graphics_capabilities()` 只用公共 Qt、D3D11 和 DXGI 接口报告：后端可用性、是否硬件加速、图形 API、adapter 名称、vendor/device ID，以及 adapter 描述给出的专用显存容量。容量不是显存使用量，不能替代运行期显存测量。

`default_gpu_with_software_fallback` 先选择硬件 D3D11；探测或初始化失败时，在尚无不兼容窗口的 worker/process 中创建 Qt Software session并记录 fallback diagnostic。`default_gpu_only` 和 `software_only` 可用于明确部署与测试。

检测到 D3D device removal 时：

1. 旧 session 原子标记 `device_lost`，以 diagnostic ID、后端、旧 generation、stage 和 detail 形成 `OffscreenDiagnostic`。
2. 旧队列 `fail(device_lost)`，未交付帧丢弃，已交付 CPU lease 继续有效；旧 session 拒绝新帧。
3. `rebuild()` 复用不可变 snapshot/config，创建 generation 严格更大的新 session；旧、新 generation 不混接。

普通 sync/render/readback 失败单独发布 `render_failed`，不伪装为设备丢失。测试用结构化注入覆盖旧 session 终止和恢复帧重新发布。

## 4. 像素差异测量

最终测量使用 192×108、frame index 50、frame time 500,000,000 ns、seed 14,230,487,856、12 events、16 series、每 series 256 samples。普通 `QQuickWindow` 捕获按物理像素/DPR 归一化，离屏输出使用相同输入。以下只陈述本机观测，不建立容差门槛：

| 后端 | 模板 | 屏上与离屏 exact SHA-256 | 最大通道差异 | 差异像素 / 20,736 | 判定状态 |
|---|---|---|---:|---:|---|
| Qt Software | waveform | `9529f9fbd6df1ce33c7e6ad6925ae1c636edbc62860689a8de7c3220b595d256` | 0 | 0 | measured/not-evaluated |
| Qt Software | spectrum | `9cc6e4937ddc291ba4a5e18259a869b5b528fe7227f3984facd15aee6b6efbd6` | 0 | 0 | measured/not-evaluated |
| Qt Software | pulse | `fe7090a1c16dc23456eefca0bd2c57882a7cb10f0c1f4c0e9c58fd0e208e7384` | 0 | 0 | measured/not-evaluated |
| D3D11 GPU | waveform | `49420df492f0fa5fb52ade35c82951ca2cb0be7699629faa45c03ddd0a075f9e` | 0 | 0 | measured/not-evaluated |
| D3D11 GPU | spectrum | `1954cf2b79dec86683d06acf8eed34c2087879e845f5ea82f4244e935c3a5966` | 0 | 0 | measured/not-evaluated |
| D3D11 GPU | pulse | `3a54ad0ff50e64142692f1b4dcd9cd3fadca22b0efa0706a70f15eb592281a3f` | 0 | 0 | measured/not-evaluated |

同一后端内屏上与离屏 exact hash 全部相等。GPU 与 software 的 hash 不要求相等，因为二者是不同公共 raster adapter；尚无跨后端容差决定。

## 5. 性能和资源观测

本机默认 GPU 探测为 NVIDIA GeForce RTX 5060 Laptop GPU，D3D11，vendor ID 4318、device ID 11609、adapter 专用显存容量 8,189,378,560 bytes。该机器尚未被确认成基准 GPU。

最终 Debug 单次观测的代表值：

| case | total frame | geometry | polish/sync/render/readback | 上传顶点 / bytes | process working set | 状态 |
|---|---:|---:|---:|---:|---:|---|
| software waveform 160×90 | 4.3704 ms | 2.3173 ms | 2.0186 ms | 816 / 9,792 | 46,227,456 B | measured/not-evaluated |
| software empty 640×360 | 3.2567 ms | 0.3435 ms | 2.8689 ms | 0 / 0 | 53,551,104 B | measured/not-evaluated |
| software dense 128×96，50k events + 100k samples | 277.0331 ms | 274.6326 ms | 2.3222 ms | 1,536 / 18,432 | 141,668,352 B | measured/not-evaluated |
| D3D11 waveform 192×108 | 12.6640 ms | 8.1705 ms | 4.4357 ms | 1,224 / 14,688 | 79,278,080 B | measured/not-evaluated |
| D3D11 spectrum 192×108 | 4.4810 ms | 0.2951 ms | 4.1354 ms | 168 / 2,016 | 80,502,784 B | measured/not-evaluated |
| D3D11 pulse 192×108 | 4.5805 ms | 0.2918 ms | 4.2356 ms | 216 / 2,592 | 81,395,712 B | measured/not-evaluated |

`total/geometry/render-readback` 使用 `steady_clock`；process CPU 使用 Windows `GetProcessTimes`，短帧若未跨过主机采样粒度会真实记录 0，而不是推算；working set 使用 `GetProcessMemoryInfo`。上传 bytes 使用实际公共 `QSGGeometry::ColoredPoint2D` stride 乘同步顶点数。

Qt 公共 API 没有为此 redirected path 提供逐帧 GPU timestamp，故 `gpuFrameTime = unavailable`。DXGI adapter 容量不等于动态显存占用，故 `gpuMemoryUsage = unavailable`。两项都未估算。

完整机器可读数据见 `evidence/T-035/measurements-v1.json`。

## 6. 测试覆盖和保留项

`space_rhythm_offscreen_rendering_tests` 共 7 个 GoogleTest case，由两个隔离 CTest 进程运行：software process 覆盖能力选择/fallback、A-021 格式、lease 背压、取消、旧 snapshot、像素布局、空输入、64×64/320×180/640×360、50k/100k 极密数据、三模板像素测量和 device generation 恢复；Windows/D3D11 process 覆盖真实默认 GPU render control、读回、三模板屏上/离屏像素测量和资源指标。

基准 GPU、正式设备矩阵、跨后端像素容差、帧预算、CPU/内存/上传阈值均未确认。因此：

- 本成果不宣称性能通过或失败，不把单次 Debug 数值当基线。
- exact hash 相等是本机观测，不冻结为跨驱动黄金值。
- H-009 仅具备提交架构师验收的证据，仍保持 `accepted`；由 architect-01 核验 T-033～T-035 后决定是否关闭。
