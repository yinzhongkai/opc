# T-034 验证摘要

- 日期：2026-09-11
- 执行人：graphics-engineer-qt-scenegraph-01
- 成果：A-022 0.1；`SpaceRhythm::RenderingGeometry`；`SpaceRhythm::SceneGraph`；`RenderGeometry.*`；`RenderSceneGraph.*`
- 环境：Windows x64；MSVC 19.44；Qt 6.11.2 shared；CMake 3.31.6-msvc6；Ninja 1.12.1；Debug；`/W4 /WX`。

## 最终验证

1. 配置：`Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Configure -UseExistingDependencies`，通过。
2. 构建：`Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Build -UseExistingDependencies`，全目标通过；新增 `space_rhythm_rendering_geometry.lib`、`space_rhythm_scene_graph.lib` 和两项测试程序。
3. 几何核心专项：`ctest --test-dir out/build/windows-msvc-x64-debug -L geometry-core --output-on-failure`，最终 9/9 通过。
4. Qt Scene Graph 专项：相同入口使用 `-L scene-graph`，最终 2/2 通过。
5. 合并专项：相同入口使用 `-L '^t034$'`，最终 11/11 通过、0 失败，总实时时间 11.82 s；未启用 WDAC fallback。该时间包含首个 QSG 测试约 10 s 的主机进程启动延迟，不作为渲染性能测量。
6. 框架一致性：`scripts/validate_framework.py` 通过，检查 17 个岗位、24 份知识、1 个实际项目、1 套项目模板和 575 处本地链接。

新生成测试程序的早期运行曾在断言前受项目已知 WDAC/SAC 策略阻止并由 CTest 记为 `BAD_COMMAND`。重新构建/重试后，geometry 9/9、scene graph 2/2 及最终合并 11/11 均由对应最新二进制真实运行通过；早期记录属于已解决的进程启动环境波动，不是测试断言失败。

## 追溯结果

| T-034 完成条件 | 证据 |
|---|---|
| `QQuickItem + QSGGeometryNode` 批量渲染 | `scene_graph_render_item.hpp/.cpp`；`RGV_QSG_001..002` |
| 高密度事件时间线和波形 | `geometry_core.cpp` 像素事件桶/min-max envelope；`RGV_DENSE_001..002` |
| 三类视觉模板 | 三个模板 `1.0.0`；`RGV_TEMPLATES_001` |
| 参数、范围、默认、版本、seed | A-022 第 2～3 节；`RGV_TEMPLATE_001`、`RGV_SEED_001` |
| viewport、LOD、批量顶点 | A-022 第 4、6 节；`RGV_CLIP_001`、`RGV_DENSE_001..002`、`RGV_QSG_002` |
| 屏上/离屏共享核心 | 纯 C++ `build_geometry_frame()` + Qt 消费适配；`RGV_SEED_001` |
| 空输入、旧快照、generation | `RGV_EMPTY_001`、`RGV_STALE_001`、`RGV_GENERATION_001`、`RGV_QSG_001` |
| 公共 API / T-035 边界 | Qt include 均为公开头；无 private API、`QRhi`、`QQuickRenderControl` 或离屏实现 |

## 保留项

T-034 的实现和负责人自测完成。产品视觉、效果门槛、基准 GPU、像素容差、性能阈值和设备矩阵仍未确认，均不声明通过。T-035 未启动，离屏驱动、GPU 降级和屏上/离屏像素/性能验证留在其范围。H-009 仅追加 T-034 阶段证据并保持 `accepted`，须等待 T-035 完成后由发起人核对关闭。
