# T-032 Windows x64 验证与测量摘要

- 证据版本：1
- 执行日期：2026-09-11
- 执行人：audio-dsp-engineer-01
- 原始证据：`out/evidence/T-032/<preset>/`（按 A-016 忽略，不纳入 Git）
- 环境：Windows NT 10.0.26200.0 x64；AMD64 Family 26 Model 96，16 logical processors；MSVC 19.44.35228；CMake 3.31.6-msvc6；Ninja 1.12.1；Qt 6.11.2 source SDK。
- 固定输入：vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`，manifest features `media;audio-analysis`，未升级 baseline；T-032 纯核心未新增 vcpkg 依赖，Qt 适配使用 D-003 已确认的 `Qt6::Multimedia/QAudioSink`。
- 范围排除：产品默认音色、媒体实现、`package/`、`scripts/__pycache__/` 和后续任务。

## 功能、黄金结果与确定性

最终源码的 12 个 GoogleTest 覆盖三份 A-018 CC0 音色强校验、非有限/变造输入拒绝、采样级触发、起点前尾音、重叠、增益/声像/硬削波、重复运行、事件输入乱序、任意分块、PCM/WAV 同源、取消、资源限制和设备不可用。`audio.golden_generate` 继续验证 A-018 10 个向量；性能 smoke 是第 14 个 T-032 CTest。

| preset | 编译 | 功能+golden | 性能 smoke | T-032 判定 |
|---|---:|---:|---:|---|
| Debug | pass | 13/13 pass（同源运行） | blocked | 功能断言通过；benchmark 在进程启动前被阻止 |
| CI / RelWithDebInfo | pass | 13/13 pass（同源运行） | blocked | 功能断言通过；benchmark 在进程启动前被阻止 |
| Release | pass | 同源较早 13/13 pass；最终复跑 golden 1/1、12 个单元进程 blocked | pass | 最新 T-032 门禁受 SAC 阻断，不宣称最终 14/14 |

Code Integrity Operational 日志在 11:45:55（Debug）和 11:46:54（CI）对 `space_rhythm_audio_render_benchmark.exe` 记录事件 3033/3077；最终审计又在 11:56:12 对 Release `space_rhythm_audio_render_tests.exe` 记录相同事件。均为未满足 Enterprise signing level，策略 ID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`。这是 T021-ENV-001/SAC 环境阻断，不是断言失败；没有用 fallback 或其他配置结果伪装被阻止的可执行文件已经运行。

同一源码的 Release 专项曾完成 14/14，但最终提交前复跑时 SAC 阻止了单元测试进程，只有 golden 与性能 smoke 可运行；因此最新状态按 blocked 记录，较早成功结果不替代最终门禁。较早的全仓探测中，CI 为 98/101（`qt.core_smoke`、`qml.quick_smoke`、`app.qml_smoke` 超时），Release 为 99/101（两个 Qt smoke 无输出）；这些不是 T-032 断言，但也说明项目完整 CI/Release 门禁未全绿，不能宣称 T021-ENV-001 已解决。

黄金 oracle 位于 `tests/golden/audio/render-oracles-v1.json`：固定 4,800-frame stereo low-pulse 的 PCM/WAV SHA-256 分别为 `9a8211f7d16e8e42a69d76623c3b5f1d80e7fc78484c752017975e2f53f3d182` 和 `a2b9c9f33733e9c3e64486aaeabe706a44334a49155012bb2c05224a4de34614`。相同输入重复运行、倒序输入和不同分块逐字节相等，容差 0。

## 性能与取消测量

最终 Release 以 48 kHz stereo、120 秒/5,760,000 frames、480 个每 250 ms 事件运行完整渲染；取消场景为 50,000 个同采样 long-tail 事件。原始 JSON 为 `out/evidence/T-032/windows-msvc-x64-release/benchmark-measured-final.json`，SHA-256 `f42d2dad0595fced2a5b065cc6804b372261d21fca90c93c7c4c93f81192025e`。

| 项 | Release 实测 |
|---|---:|
| prepare | 0.281 ms |
| render | 40.931 ms |
| throughput | 140,724,290 frames/s |
| realtime factor | 2,931.756× |
| process peak working set | 243,499,008 bytes |
| cancel latency | 0.085 ms |
| 120 秒 PCM SHA-256 | `f1f26ab93fe29e58944919a15eca47314f7fc4a638b99e15de0daeb73345d3f8` |

这是单次开发机技术测量，不是统计分布或硬件矩阵。项目尚未确认吞吐、峰值内存和取消延迟阈值，全部状态为 `measured/not-evaluated`，不得标为性能 pass/fail。

## 结论边界

- T-032 实现、断言覆盖和黄金结果已完成；Debug/CI 功能断言及同源较早的 Release 专项运行提供了成功证据。
- Debug/CI 性能进程与最终 Release 单元进程被 WDAC/SAC 阻止；另有非 T-032 Qt smoke 缺口，最新 T-032/完整 Release 门禁都不宣称通过。
- null 设备路径已自动验证；没有在无基准设备矩阵的主机上宣称实际声卡兼容或试听质量通过。
- 三个音色仅是 A-018 登记的 CC0 测试音色，产品默认音色仍未选择、未批准。
