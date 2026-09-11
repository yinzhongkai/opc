# T-031 Windows x64 验证与测量摘要

- 证据版本：2
- 执行日期：2026-09-10（原实现）；2026-09-11（H-011 验收修订）
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

## H-011 schema 2 消费方验收修订

- 核验输入：媒体提交 `156b19f`，A-014 0.4/A-015 0.3，`mediaContractVersion=1.0.0/schemaVersion=2`。
- 接口结果：删除 `PcmAdapterContext` 及 DSP 自建 `ResampleTrace::identity`；唯一入口为 `PcmNarrowAdapter::adapt(const media::PcmBuffer&)`。adapter 直接读取并保留媒体 `channel_order`、`segment_origin_time_ns`、`segment_origin_sample_index` 和 `resample_trace`，PCM lease 仍零拷贝共享。
- delay 记账证明：真实 44.1→48 kHz 与 48→44.1 kHz 缓冲出现非零 `delayBeforeInputFrames` 时，断言 `dsp.firstSampleIndex == media.firstSampleIndex == previousBufferEnd`；DSP 时间仅由该索引、segment 原点和输出采样率复算并等于媒体 `timeNs`。trace delay 只保留作证据，未进入时间公式，因此没有二次补偿。
- 真实媒体覆盖：`audio_44100.wav` identity 44.1→44.1、44.1→48、`audio_48000.wav` 48→44.1；两种变采样都观察到非零 delay 与 drain；50 ms seek 得到新 segment/origin 和 sample index 2400；`dynamic_audio.ts` 观察到旧 44.1→48 segment drain 后新 48 kHz identity epoch/segment。
- fail closed：空 `channelOrder`、缺 implementation version/delay unit、schema 1/contract 0.1.0、`performed` 与采样率矛盾、`delayAccountedInFirstSampleIndex=false`、trace 输出率与 PCM 不符全部被拒绝；同 segment 的配置静默变化或 drain 后恢复普通输入也拒绝。adapter 不读 PTS、帧数或日志，不修改媒体 DTO/实现。

| preset | 编译 | T-031 CTest | 验收判定 |
|---|---:|---:|---|
| `windows-msvc-x64-debug` | pass | 22/22 pass | schema 2 消费方验收通过；前两次启动曾被 WDAC 阻止，最终重建复跑通过 |
| `ci-windows-msvc-x64`（RelWithDebInfo） | pass | 22/22 pass | schema 2 消费方验收通过 |
| `windows-msvc-x64-release` | pass | 22/22 pass | schema 2 消费方验收通过；本轮未复现 Release 阻断 |

同一 CI 构建另行执行 `ctest --preset ci-windows-msvc-x64 -L media --output-on-failure`，媒体 schema、双向重采样、seek、format-change、取消和 golden audit 共 27/27 通过。原始命令/日志位于忽略目录 `out/evidence/T-031/H-011-acceptance/`；配置均使用固定 baseline 与 `-UseExistingDependencies`，未升级依赖。Debug 新链接二进制的前两次启动被“应用程序控制策略已阻止此文件”拒绝；重新构建后第三次完整 22/22 直接通过。三配置均由本次源码重新配置、编译并直接运行，无 fallback；瞬态不解释为 T021-ENV-001 已关闭。

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
- A-015 0.3 已公开完整 schema 2 resampler provenance，并由 DSP 对提交 `156b19f` 完成上述消费方验收；缺失或矛盾字段继续 fail closed。H-011 已由发起人关闭，DSP 未修改媒体契约、实现或伪造 trace。
- 本轮 Release T-031 测试通过，但 CI install 和 Debug benchmark 仍观察到 T021-ENV-001；因此不把本轮局部成功解释为 WDAC 环境阻断已关闭。
