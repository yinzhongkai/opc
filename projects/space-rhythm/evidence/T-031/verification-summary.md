# T-031 Windows x64 验证与测量摘要

- 证据版本：1
- 执行日期：2026-09-10
- 执行人：audio-dsp-engineer-01
- 原始证据：`out/evidence/T-031/<preset>/`（按 A-016 忽略，不纳入 Git）
- 环境：Windows NT 10.0.26200.0 x64；AMD64 Family 26 Model 96，16 logical processors；MSVC 19.44.35228；CMake 3.31.6-msvc6；Ninja 1.12.1。
- 固定输入：vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`，manifest features `media;audio-analysis`，未升级 baseline。
- 范围排除：T-032、`package/`、`scripts/__pycache__/`。

## 依赖与运行时证据

- `kissfft:x64-windows-space-rhythm@131.2.0`，installed ABI `f4efd3cab5045a3ac10d1d4371c5898cac55ca956cec0339b15618316a9390b2`。
- port 131.2.0 暴露的 upstream CMake package version 实际为 131.1.0；配置守卫已固定该事实。DSP 只链接 `kissfft::kissfft-float`。
- 许可证为 BSD-3-Clause，版权归 Mark Borgerding（2003–2010）。CI install 已实际写入 `bin/kissfft-float.dll`（23,040 bytes，SHA-256 `74feb9b701ab34dd1b6815a21ac2c7da19ae71d072b9abe20ac215458bcf95cc`）和 `licenses/kissfft-BSD-3-Clause.txt`（SHA-256 `a2840585f8411be8e6826a31ef15ae65c950bd74a2437a73b013398a934ad0c6`）。
- `dumpbin /dependents` 确认 T-031 测试与 benchmark 的 KissFFT 直接运行时只有 `kissfft-float.dll`；port 中的 double/int16/int32 DLL 未进入直接 import 或本模块安装规则。
- CI install 在上述 DSP 文件安装成功后，既有 `windeployqt.exe` 被 WDAC `0xc0e90002` 阻止，故整个应用 install stage 记录为 `blocked`，不将部分安装写成全流程 pass。

## oracle 与功能测试

`Generate-AudioDspVectors.ps1 -ValidateOnly`：10/10 A-018 输入/音色的配方、PCM SHA-256、许可和时间 oracle 通过；新增的 7 个算法 oracle 已与 A-018 fixture ID、vector set、PCM SHA-256、algorithm/backend/parameter 版本交叉校验。自由节奏、弱瞬态、segment 中断和取消的附加合成向量也登记了 recipe、CC0-1.0 和实际 hash。

| preset | 构建 | T-031 CTest | 结论 |
|---|---:|---:|---|
| `windows-msvc-x64-debug` | pass | 17/17 pass | PCM/provenance、oracle、边界、取消和资源上限均通过 |
| `ci-windows-msvc-x64`（RelWithDebInfo） | pass | 17/17 pass | 最终完整复跑通过 |
| `windows-msvc-x64-release` | pass | 17/17 pass | 本轮未复现 Release 测试阻断 |

CI 较早一轮的首个 GoogleTest 进程曾被 WDAC `0xc0e90002` 阻止，而同一二进制的其余用例可运行；目标单项复跑和之后两次完整 CI 复跑均为 17/17。该记录保留为 T021-ENV-001 的环境瞬态，不改写成算法失败，也不据此宣称策略问题已解决。最终 Debug benchmark 重建后的启动也被同一策略阻止，因此最终性能结论采用成功完成的 CI/Release 原始运行，不用 Debug 旧数据替代。

专项通过后额外执行过一次非完成条件的 Debug 全仓 80-test 并行探测：T-031 17 项和媒体项仍通过，但既有 core/Qt 测试进程有 38 项以 `0xc0000135` 在断言前启动失败；单独复跑 core 仍同码失败。该探测没有算法断言失败，按主机依赖/策略环境异常保留，不计作 T-031 专项 pass，也不覆盖 T-021 自有证据结论。

测试覆盖：固定/变速节拍、脉冲、自由节奏、弱瞬态、确定性噪声、静音、NaN/±Inf、44.1/48 kHz、`FL,FR` 双声道、负索引、segment 中断、显式 identity/incomplete resampler trace、同 segment gap、time overflow、取消和资源上限。固定输入/参数/seed 的 feature/candidate 重复等价，候选稳定排序。

## 性能与取消测量

方法：48 kHz mono `AV-FIXED-BEAT-120-001`（96,000 frames）预热 5 次、记录 30 次；另以已登记 SHA-256 的 480,000-frame 静音输入记录 30 次取消请求到返回的延迟。峰值内存为进程 `PeakWorkingSetSize`，同时记录分析器按 DTO/FFT scratch 上界估算。全部使用单进程默认参数；原始 samples 保存在被忽略 JSON。

| preset | duration median / p95 | throughput median | process peak working set | analyzer estimated peak | cancel median / p95 | raw JSON SHA-256 |
|---|---:|---:|---:|---:|---:|---|
| CI / RelWithDebInfo | 13,320 / 13,827 µs | 7,199,100 frames/s | 14,442,496 bytes | 1,346,512 bytes | 239 / 645 µs | `b75bac3e674b145b7d98a5505ae87636dc2445f29757633bf900aa3e91a727c0` |
| Release | 12,623 / 13,528 µs | 7,584,136 frames/s | 12,795,904 bytes | 1,346,512 bytes | 221 / 659 µs | `d8b38dfcb4e83f2d208ad3d6eb25ecc38010e46f714e75f4a3d21a4fbc7214bb` |

算法/参数来源：`space-rhythm.audio-analysis.classic@1.0.0`、`kissfft-float@131.2.0-vcpkg.port+cmake.131.1.0`、`space-rhythm.audio-analysis.production@1.0.0`；44.1/48 kHz 参数摘要分别为 `c76b32e4cd30fdbc0f540235ce97401248b9d6cea5fbb9dc5985062c88b10a82` 和 `3c7493a42ae78009be222d29754e6a11ca8eb0e234307dde0a8bfe73d33c22f8`。

## 判定边界

- 吞吐、峰值内存和取消延迟状态均为 `measured`；项目未确认硬件基线或性能阈值，故为 `not-evaluated`，不标 pass/fail。
- 合成 oracle 证明确定性和契约行为；产品代表素材、标注和效果阈值未确认，效果为 `not-evaluated`。
- A-015 0.2 未公开完整非身份 resampler provenance；DSP 对缺失字段返回 `resample_timing_unavailable`。字段级接口请求已登记 H-011，未修改媒体契约或伪造 trace。
- 本轮 Release T-031 测试通过，但 CI install 和 Debug benchmark 仍观察到 T021-ENV-001；因此不把本轮局部成功解释为 WDAC 环境阻断已关闭。
