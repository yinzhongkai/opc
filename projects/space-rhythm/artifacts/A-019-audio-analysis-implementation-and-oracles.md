# 音频特征、瞬态与节拍候选实现和算法 oracle

- 项目: space-rhythm
- 成果 ID: A-019
- 负责人: audio-dsp-engineer-01
- 关联任务: T-031
- 版本: 0.1
- 更新日期: 2026-09-10
- 状态: draft
- 适用范围: Windows x64 上的 A-018 PCM 窄适配、窗口化、短时/频段能量、谱变化、瞬态及节拍候选；不修改媒体契约、核心时间线或执行 T-032 混音/试听。
- 来源及输入版本: A-012 0.1、A-013 0.2、A-014 0.3、A-015 0.2、A-016 0.1、A-017 0.2、A-018 0.1，D-003 confirmed；用户于 2026-09-10 明确确认前置并启动 T-031。
- 批准依据: 尚无；效果阈值和性能阈值均未确认，本成果只报告 `measured/not-evaluated`。
- 版本记录: 2026-09-10，0.1，首次实现分析链、固定参数、算法 oracle、Windows 验证和性能测量。

## 1. 实现边界

公共入口位于 `space_rhythm/audio/analysis.hpp`，实现位于 `src/audio_analysis/analysis.cpp`。`PcmNarrowAdapter` 只把 A-015 的 `flt`、interleaved、mono/stereo PCM 和 lease 收窄为 A-018 视图；它不解码、不重采样、不复制 PCM、不解释 PTS，也不提交核心事件。`Analyzer` 是非实时、无外部状态的批分析器；每次调用返回完整结果，失败或取消不返回部分候选。

支持的 PCM 入口严格为：IEEE-754 binary32 little-endian、frame-major interleaved；单声道顺序 `FC` 或双声道顺序 `FL,FR`；实际正整数采样率，生产参数当前只冻结 44,100 Hz 和 48,000 Hz；`validFrameCount` 取 A-015 `sample_count`，stride 必须分别为 4/8 字节。lease 从媒体缓冲共享持有，分析结束前保持有效，DSP 不取得写所有权。

时间只由以下精确式及 A-012 checked 整数舍入产生：

```text
timeNs(i) = segmentOriginTimeNs
          + round((i - segmentOriginSampleIndex) * 1_000_000_000 / sampleRate)
```

候选/PCM 时间用 `nearest_ties_to_even`，覆盖起点用 `floor`，覆盖终点用 `ceil`；不累计已舍入 duration。负索引保留，端点、窗锚和覆盖溢出均返回 `time_overflow`。

## 2. resampler provenance fail-closed

身份转换只有在调用者提供完整 `identity/1` trace 时成立：`performed=false`、输入/输出率相同、delay `0/1 input_frames`、delay 已计入 `firstSampleIndex`、`emittedFromDrain=false`，参数摘要由 `identity-v1|sampleRate=<rate>` 计算。DSP 不从 PTS、帧数、时长或日志反推上述事实。

非身份转换要求媒体所有者显式提供 `performed`、输入/输出率、实现 ID/版本、完整参数 SHA-256、调用前精确输入帧 delay 的分子/分母、delay 帐务标记和 drain 标记。任一字段缺失或矛盾均返回 `validation/resample_timing_unavailable`。A-015 0.2 当前公共 DTO 尚不能提供完整字段，因此非身份重采样进入 DSP 时按该错误终止；字段级请求见 H-011。此实现没有修改媒体 DTO 或伪造 provenance。

## 3. 固定依赖与运行时边界

| 项 | 实际值 |
|---|---|
| vcpkg baseline | `9e593bb18ea69cc5095e012465dcd675a822ed0d`，未升级 |
| manifest feature | `audio-analysis`；依赖 `kissfft`，CMake presets 显式启用 `media;audio-analysis` |
| vcpkg port/version | `kissfft:x64-windows-space-rhythm@131.2.0` |
| installed ABI | `f4efd3cab5045a3ac10d1d4371c5898cac55ca956cec0339b15618316a9390b2` |
| upstream CMake package version | port 131.2.0 实际暴露 `KISSFFT_VERSION=131.1.0`；配置时显式校验该差异 |
| 选用 target/feature | `kissfft::kissfft-float` / float；未调用 double、int16、int32 或 OpenMP/tools |
| triplet/CRT | `x64-windows-space-rhythm`，x64 dynamic library、dynamic CRT |
| 许可证 | SPDX `BSD-3-Clause`；Copyright © 2003–2010 Mark Borgerding |
| DSP 直接新增运行时 | `kissfft-float.dll`；Debug 实测 78,848 bytes，Release/CI 实测 23,040 bytes |

`space_rhythm_audio_analysis` 自身是静态库，最终消费程序通过 import library 依赖 `kissfft-float.dll`。vcpkg port 同时安装的其他 KissFFT datatype DLL 不是 DSP 的直接 PE import，也不由本模块安装规则暂存。安装规则只复制与配置匹配的 `kissfft-float*.dll`，并把 vcpkg 的版权/许可证文件安装为 `licenses/kissfft-BSD-3-Clause.txt`。测试程序还继承媒体模块的 FFmpeg 运行时边界；那不是本 feature 新增的依赖。

## 4. 生产参数集 1.0.0

算法来源固定为：

- `algorithmId = space-rhythm.audio-analysis.classic`
- `algorithmVersion = 1.0.0`
- `producerVersion = 0.1.0`
- `backendId = kissfft-float`
- `backendVersion = 131.2.0-vcpkg.port+cmake.131.1.0`
- `parameterSetId = space-rhythm.audio-analysis.production`
- `parameterSetVersion = 1.0.0`
- `deterministicSeed = 0x5350414345524859`（当前算法不使用随机分支，但 seed 仍进入来源、摘要和 revision）

| 参数 | 固定值 |
|---|---|
| frame / hop / FFT | 1024 / 256 / 1024 frames |
| boundary | 每个 segment 从首样本起步；末窗 zero-pad；窗不跨 segment |
| channel aggregation | `mean_energy` |
| window | periodic Hann `0.5 - 0.5*cos(2πn/N)`，version 1 |
| bands | `[0,200000)`、`[200000,2000000)`、`[2000000,20000000)` milliHz |
| onset | amplitude threshold 200,000 ppm；minimum spacing 64 frames |
| signal floor | 100 ppm |
| beat | period tolerance 20,000 ppm；minimum 3 onsets；60,000–300,000 milliBPM |

窗系数 binary64 little-endian摘要为 `4906ede244241938adc967b93ef9d0502baea192dd78abbb644436c61000467c`。完整规范 JSON（含 required features 和显式 `smoothing=none/1`）摘要分别为：44.1 kHz `c76b32e4cd30fdbc0f540235ce97401248b9d6cea5fbb9dc5985062c88b10a82`；48 kHz `3c7493a42ae78009be222d29754e6a11ca8eb0e234307dde0a8bfe73d33c22f8`。

## 5. 数值定义与候选

- 短时能量：有效输入逐声道平方和，除以 `1024 * channelCount`；zero padding 因而进入分母。DTO 用 `mantissa * 10^-12 linear_power`。
- 频段能量：各声道乘 periodic Hann 后分别做 1024 点 KissFFT；复数模平方先按声道平均，再除以 `FFTSize²`，按 bin 的整数 milliHz 落入 `[low,high)` 后求和。schema 1 不做 one-sided double。
- 谱变化：相邻特征帧一侧谱幅值逐 bin 正差之和，再除以 `N/2+1`；每个 segment 首帧为 `valid=false/no_previous_frame`。
- 瞬态：每个采样位置先计算跨声道 mean-square 的平方根。连续超过 amplitude threshold 的区域选最大采样；相邻峰少于 64 frames 时只保留更强者，完全相等保持先出现者。
- 节拍：只在同 segment 内处理相邻 onset period；period 必须落在 BPM 范围。按首次出现的 representative 和 2% 容差稳定聚类，最多 3 类且每类至少有两个 period 才发布 beat。tempo 使用整数最近值 `(sampleRate*60000 + period/2)/period` milliBPM。
- 置信度：onset 为 peak amplitude 的 ppm；beat 为 `max(700000, 1000000 - 75000*(periodClusterCount-1))`；overall 是全部候选最大 confidence。无 beat 时根据 peak/RMS、瞬态数量和时长返回 `low_confidence` 及 `weak_transients`、`aperiodic`、`noisy` 或 `insufficient_duration`；低于 floor 返回 `no_signal/silence`。

所有对外能量和谱值先舍入为十进制定点，DTO 不携带 NaN/Inf。生产 oracle 容差为：sample index、`TimeNs`、tempo 和 confidence 绝对容差 0；strength 1 ppm；特征 mantissa 4（scale `-12`）。候选固定按 `(timeNs, kind ASCII, CandidateId ASCII)` 排序；feature 固定按 `(anchorTimeNs, FeatureFrameId ASCII)` 排序。revision/ID 包含输入指纹、segment/range/epoch、algorithm/backend、参数摘要和 seed；同输入、参数、backend 和 seed 重复执行产生等价 DTO 与稳定顺序。

## 6. oracle、错误与覆盖

[algorithm-oracles-v1.json](../../../tests/golden/audio/algorithm-oracles-v1.json) 为 A-018 `vectorSetVersion=1` 的 7 个分析/负向输入增加算法 1.0.0 oracle；[fixtures-v1.json](../../../tests/golden/audio/fixtures-v1.json) 登记 oracle 元数据，生成器会交叉校验版本、输入 ID 和 PCM SHA-256。固定节拍和变速节拍的 onset/beat 索引、tempo、overall confidence 为精确 oracle；脉冲还冻结首帧短时能量和重复等价；静音、噪声、双声道负索引边界与非有限值冻结状态/原因/错误和无部分结果语义。

同一 oracle 文件另登记自由节奏、弱瞬态、双 segment 中断和取消用的确定性内存向量，包含规范 recipe、来源、`CC0-1.0` 和实际 PCM SHA-256；资源上限复用已登记固定节拍向量。三个 A-018 `AT-*` 测试音色未被本任务用于生产算法或产品默认内容，许可与 hash 保持不变。`package/` 和来源不明素材均未读取或修改。

覆盖的终止错误包括 `invalid_pcm_buffer`、`unsupported_pcm_format`、`unsupported_channel_layout`、`non_finite_pcm`、`pcm_out_of_range`、`pcm_discontinuity`、`resample_timing_unavailable`、`timestamp_mismatch`、`invalid_analysis_parameters`、`unsupported_parameter_schema`、`time_overflow`、`resource_limit` 和 `cancelled`。取消和失败都会清空 feature/candidate，不伪装为完整低置信结果。

## 7. Windows 验证与测量

最终可复核结果见 [T-031 verification summary](../evidence/T-031/verification-summary.md)。Debug 与 CI/RelWithDebInfo 均完成构建和 T-031 标签测试；CI 首轮曾有单个进程启动被 WDAC `0xc0e90002` 拦截，单项复跑及完整复跑均通过，记录为环境瞬态而非算法失败。Release 的真实状态按最终复核记录，不以其他配置替代。

性能程序对 48 kHz mono、96,000 frames 固定节拍执行 5 次预热和 30 次原始采样，并对 480,000 frames 合成静音执行 30 次取消延迟采样；记录进程 peak working set 和分析器有界内存估算。由于项目尚未确认性能或效果阈值，所有结果只标为 `measured`，结论为 `not-evaluated`，不声称 pass/fail。

## 8. 未完成输入与下游边界

- H-011 等待媒体所有者提供公开、字段完整的 resampler provenance；在此之前只有显式 identity trace 可进入 DSP，其他情况稳定返回 `resample_timing_unavailable`。
- 产品代表素材、效果验收口径、性能硬件基线和阈值尚未确认；本任务不从合成 oracle 推导产品效果已通过。
- T-032 未启动；本成果不实现音色触发、混音、QAudioSink 试听或离线 WAV/PCM 输出。
