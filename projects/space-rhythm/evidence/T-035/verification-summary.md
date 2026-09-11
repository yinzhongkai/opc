# T-035 验证摘要

- 日期：2026-09-11
- 执行人：graphics-engineer-qt-scenegraph-01
- 成果：A-025 0.1；`SpaceRhythm::OffscreenRendering`；`OffscreenContract.*`；`OffscreenSoftware.*`；`OffscreenDefaultGpu.*`
- 环境：Windows x64；MSVC 19.44；Qt 6.11.2 shared；CMake 3.31.6-msvc6；Ninja 1.12.1；Debug；`/W4 /WX`。
- 结果状态：`measured/not-evaluated`。基准 GPU、像素容差和性能阈值未确认。

## 最终验证

1. 配置：`Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Configure -UseExistingDependencies`，通过。
2. 构建：同入口 `-Stage Build`，全目标通过；新增 `space_rhythm_offscreen_rendering.lib` 和 `space_rhythm_offscreen_rendering_tests.exe`。
3. T-035 专项：`ctest --test-dir out/build/windows-msvc-x64-debug -L t035 --output-on-failure -V`，2/2 CTest 进程通过，内部 7/7 GoogleTest case 通过；software 与 default GPU 各在隔离进程运行。
4. T-033～T-035 合并回归：`ctest ... -R '^(RenderContract|RenderGeometry|RenderSceneGraph|rendering\.offscreen)' --output-on-failure`，31/31 通过、0 失败，总实时时间 9.33 s。
5. 公共 API 扫描：渲染实现与新增测试无 `private/*`、`*_p.h`、`QRhi` 或 `QSGRenderContext` 引用。
6. 框架一致性：使用隔离 PyYAML 6.0.3 依赖运行 `scripts/validate_framework.py`，通过；检查 17 个岗位、24 份知识、1 个实际项目、1 套项目模板和 593 处本地链接。

新构建测试程序在早期重试中曾被本机既有 WDAC/SAC 在进程启动前短暂标记 `BAD_COMMAND`；相同最终二进制随后正常执行并通过。实现迭代还修正了 Qt Software 对通用彩色 `QSGGeometryNode` 不出图的能力差异，改用公共 `QQuickPaintedItem` 软件适配；GPU 仍走同一 `GeometryFrame` 的 `QSGGeometryNode`。普通窗口捕获按本机 DPR 1.5 归一化到目标物理像素，消除了逻辑/物理尺寸混淆。最终证据不包含这些已修正状态的失败判定。

## 追溯结果

| T-035 完成条件 | 最终证据 |
|---|---|
| 公共 QQuickRenderControl/Target | `offscreen_renderer.cpp` 的 D3D11 texture 与 software paint device 两条路径 |
| 同 recipe/snapshot/geometry/time/seed | session 固定 immutable snapshot；屏上/离屏均调用 `build_geometry_frame()`；三模板 time/hash 校验 |
| A-021 RGBA8/sRGB/top-down 与 lease | `OffscreenSoftware.FrameContractMetricsAndLeaseBackpressure` |
| 默认 GPU 与软件 fallback | RTX 5060 Laptop GPU D3D11 实机路径；forced-unavailable selection vector；Qt Software 实际渲染 |
| device loss/generation | `CancellationAndOldGenerationTerminationRebuild` 的旧 session fail、diagnostic 和新 generation |
| 三模板像素差 | `measurements-v1.json`：software/GPU 共 6 组 exact hash；最终 max diff 0、diff pixels 0 |
| 性能/资源 | total/CPU/geometry/render-readback、uploaded vertices/bytes、working set 实测；GPU time/usage unavailable |
| 边界输入 | 取消、lease 背压、空输入、64²/320×180/640×360、50k events + 100k samples、旧 snapshot/布局拒绝 |
| 不设未知门槛 | 代码、CTest 标签、A-025 和 JSON 均只使用 `measured/not-evaluated` |

## 测量摘要

- 默认 GPU：Direct3D 11，NVIDIA GeForce RTX 5060 Laptop GPU；该设备不是已确认基准。
- 软件三模板屏上/离屏 exact hash 各自相同；GPU 三模板屏上/离屏 exact hash 各自相同；六组最大通道差异均为 0，差异像素均为 0/20,736。
- 代表单帧范围：普通 192×108 GPU case 4.4810～12.6640 ms；software 160×90 waveform 4.3704 ms；极密 software case 277.0331 ms。均为 Debug 单次观测，不能作为性能门禁。
- GPU frame time 与动态 GPU memory usage 无公共 API 数据，均明确 `unavailable`，没有估算。

完整数值和 hash 见 [measurements-v1.json](measurements-v1.json)。

## 交接状态

T-035 负责人实现和自测完成。H-009 只追加 T-033～T-035 完整处理证据并保持 `accepted`；下一行动人为 architect-01，由其验收后决定关闭。视觉风格、基准 GPU、设备矩阵、像素容差和性能阈值继续待确认。
