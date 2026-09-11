# T-033 验证摘要

- 日期：2026-09-11
- 执行人：graphics-engineer-qt-scenegraph-01
- 成果：A-021 0.1；`SpaceRhythm::RenderingContract`；`RenderContractVectors.*`
- 环境：Windows x64；MSVC 19.44；Qt 6.11.2 shared；CMake 3.31.6-msvc6；Ninja 1.12.1；Debug；`/W4 /WX`。

## 最终验证

1. 配置：`Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Configure -UseExistingDependencies`，通过。
2. 构建：`Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Build -Parallel 10`，全目标通过；新增 `space_rhythm_rendering_contract.lib` 和 `space_rhythm_render_contract_tests.exe`。
3. 专项：`ctest --test-dir out/build/windows-msvc-x64-debug -R '^RenderContractVectors\.' --output-on-failure`，最新最终复跑 18/18 通过、0 失败，总实时时间 4.85 s。
4. 框架一致性：`scripts/validate_framework.py` 通过，检查 17 个岗位、24 份知识、1 个实际项目、1 套项目模板和 561 处本地链接。

首轮专项曾以 11/18 通过暴露三项测试/实现缺陷：坐标比值误用了核心秒制 time-base 函数、测试事件来源不符合 A-012、optional extension 键未 namespaced。修订为 checked 128-bit 整数比值、合法 user source 和 namespaced key 后全部复跑通过。另补充了取消/设备失败清空未交付帧时在队列锁外释放 lease，避免 guard 回调重入死锁。

## 追溯结果

| T-033 完成条件 | 证据 |
|---|---|
| 版本化 recipe 和不可变快照 | `render_contract.hpp/.cpp`；`RCTV-RECIPE-001`、`RCTV-SNAPSHOT-001..003` |
| UI/渲染/GPU 生命周期 | A-021 第 4～5 节；`SceneGraphLifecycle`；`RCTV-LIFECYCLE-001..002` |
| QQuickItem/QSGGeometryNode 边界 | A-021 第 5 节；仅公共 API 约束，未实现 T-034 |
| 离屏格式/时间/stride/所有权 | `RenderedFrame`/`FrameLease`；`RCTV-FRAME-001` |
| 背压/取消/设备丢失 | `BoundedFrameQueue`；`RCTV-QUEUE-001..004` |
| 坐标和命中 | `CoordinateTransform`/HitTest DTO；`RCTV-COORD-001`、`RCTV-HIT-001` |
| 版本兼容 | `ContractDescriptor`/`negotiate_contract`；`RCTV-VERSION-001..004` |

## 保留项

T-033 契约工作完成。T-034/T-035 未在本任务中启动；实际节点、模板、shader、离屏渲染执行、GPU 能力/降级和性能/像素容差仍由后续任务完成。H-009 只增加本阶段处理证据并保持 `accepted`，须等待 T-033～T-035 全部完成后由发起人核对关闭。
