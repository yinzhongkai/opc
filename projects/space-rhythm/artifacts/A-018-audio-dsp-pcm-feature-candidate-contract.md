# 音频 DSP PCM、特征、候选与测试音色契约 0.x

- 项目: space-rhythm
- 成果 ID: A-018
- 负责人: audio-dsp-engineer-01
- 关联任务: T-030
- 版本: 0.1
- 更新日期: 2026-09-10
- 状态: draft
- 适用范围: 定义 DSP 消费的 PCM、采样时间、segment、重采样延迟、缓冲所有权、特征帧、分析候选、参数/算法版本、错误、可复现合成向量和测试音色; 不实现 T-031 分析算法或 T-032 混音/试听，不改变媒体 PTS 或核心事件语义。
- 来源及输入版本: [A-012 0.1](A-012-core-domain-contract-0x.md) `TimeNs`、候选 envelope 与 `ErrorInfo`; [A-014 0.3](A-014-media-time-buffer-and-golden-contract.md) 媒体 PCM/采样索引/所有权; [A-015 0.2](A-015-ffmpeg-media-pipeline.md) 实际 FFmpeg PCM 输出; [A-016 0.1](A-016-cpp-qt-test-strategy-and-traceability.md) 可复现、证据和许可规则; A-004 0.5、A-005 0.4、A-006 0.1 WP-06、A-011 0.1; D-001～D-008 confirmed; 用户于 2026-09-10 明确要求接收 H-008 并完成 T-030。
- 批准依据: 尚无。T-030 完成条件为负责人自查并形成候选契约，不等于算法效果、产品音色或发布许可批准。
- DSP 契约版本: `dspContractVersion = 0.1.0`
- 逻辑 DTO schema: `schemaVersion = 1`
- 参数 schema: `parameterSchemaVersion = 1`
- 测试向量集: `vectorSetVersion = 1`
- 测试音色清单: `timbreManifestVersion = 1`
- 版本记录: 2026-09-10，0.1，首次定义 PCM 入口、精确时间、segment/延迟、特征和候选 DTO、低置信语义、参数摘要、错误及 10 个合成向量。

## 1. 规范词与单一责任

“必须”“不得”“应”是规范要求，“可以”“建议”是非强制选择。只有满足全部相关不变量与向量，才能声明兼容 `dspContractVersion 0.1.0`。

DSP 领域拥有:

- 媒体 PCM 进入分析前的格式收窄、采样位置验证和特征窗口规则;
- 短时能量、频段能量、谱变化、瞬态与节拍候选的 payload、参数和版本;
- 低置信、静音、弱瞬态、自由节奏和噪声的可解释结果;
- 合法、可复现测试向量与测试音色的配方、许可和哈希。

DSP 领域不拥有:

- 媒体 PTS/DTS、presentation origin、解码或重采样器实现; 这些仍由 A-014/A-015 所属媒体领域维护;
- A-012 `RhythmEvent`、融合、锁定或时间线修订; DSP 只产生不可变候选;
- 产品默认音色、音色商用许可、采样率默认值、效果或性能放行阈值;
- QML/JavaScript 中的 DSP 计算、媒体封装或设备时钟。

## 2. A-015 实际 PCM 入口兼容性

| A-015 字段/行为 | DSP schema 1 映射 |
|---|---|
| `sample_format = "flt"` | 当前 Windows x64/IEEE-754/小端基线映射为 `f32_le`; 其他表示不得猜测。 |
| `channel_layout = "mono" | "stereo"` | 分别映射为顺序 `FC` 或 `FL,FR`; 不得只传声道数。 |
| `planar = false` | `interleaving = interleaved`，帧内按 `channelOrder` 排列。 |
| `sample_count` | `validFrameCount`，按每声道帧数计，不乘声道数。 |
| plane 0 `row_bytes = channels * 4` | `frameStrideBytes`; `validBytes = validFrameCount * frameStrideBytes`。 |
| `lease.format_epoch()` | `formatEpoch`; 格式变化必须先通知新 epoch。 |
| `segment_id` / `first_sample_index` | 原样保留; 同 segment 严格连续。 |
| `time_ns` / `duration_ns` | 仅作上游声明与交叉验证; DSP 依绝对采样索引重新计算。 |
| 不可变 `BufferLease` | DSP 保留 lease 的共享所有权并只读处理。 |

当前上游结构尚未显式携带重采样延迟、重采样实现版本和版本化声道顺序。进入 DSP 前必须由窄适配层补齐第 3 节 `DspPcmBuffer`; PCM lease 可零拷贝复用，缺失时序元数据不得从帧数、包时间或日志反推。

## 3. DSP PCM、segment 与所有权

```text
DspPcmBuffer {
  schemaVersion: 1
  dspContractVersion: "0.1.0"
  mediaContractVersion: "0.1.0"
  inputFingerprintSha256: 64 lowercase hex
  streamKey: A-014 StreamKey
  segmentId: OpaqueId
  formatEpoch: UInt64
  segmentOriginTimeNs: TimeNs
  segmentOriginSampleIndex: Int64
  firstSampleIndex: Int64
  validFrameCount: UInt64
  sampleRate: positive UInt32
  sampleFormat: "f32_le"
  channelOrder: ["FC"] | ["FL", "FR"]
  interleaving: "interleaved"
  frameStrideBytes: UInt32
  data: { offsetBytes: UInt64, validBytes: UInt64, lease: BufferLease }
  resampleTrace: ResampleTrace
  extensions: ExtensionMap
}

ResampleTrace {
  performed: bool
  inputSampleRate: positive UInt32
  outputSampleRate: positive UInt32
  implementationId: non-empty ASCII token
  implementationVersion: non-empty ASCII string
  parametersDigestSha256: 64 lowercase hex
  delayBeforeInputFrames: { numerator: non-negative Int64, denominator: positive Int64 }
  delayUnit: "input_frames"
  delayAccountedInFirstSampleIndex: true
  emittedFromDrain: bool
}
```

schema 1 不发布项目默认采样率; `sampleRate` 取实际输出正整数，44.1 kHz 和 48 kHz 都必须受支持。频段参数必须按当前 Nyquist 单独验证。

PCM 不变量:

1. `validFrameCount` 是有效帧而非容量; 0 帧不发布 data item，只用显式 segment 终态。
2. `frameStrideBytes = channelOrder.count * 4`，`validBytes = validFrameCount * frameStrideBytes`; 所有算术 checked，view 不得超过 lease `byteSize`。
3. 正常分析样本必须是 `[-1,1]` 内有限 IEEE-754 binary32。NaN/无穷返回 `validation/non_finite_pcm`; 超范围返回 `validation/pcm_out_of_range`，不静默夹紧。
4. schema 1 只接受 `FC` 或 `FL,FR` 交错 float。其他格式/顺序必须由版本化上游转换产生新 epoch; DSP 不在窗内暗中转换。
5. 同 segment 相邻缓冲必须满足 `next.firstSampleIndex = current.firstSampleIndex + current.validFrameCount`，且格式、epoch、origin 和重采样配方不变。
6. 格式、origin、重采样配方变化，或来源缺口/重叠/丢样，必须用新 segment ID 并产生稳定 discontinuity reason; 不得改索引隐藏缺口。

Resample 规则:

- `performed=false` 时输入/输出率相同，delay 为 `0/1`，`emittedFromDrain=false`，implementation 显式为 `identity/1`。
- `performed=true` 时全部字段由媒体适配器产生。delay 是该次输入前缓存的精确输入帧数; EOF/format-change 排空产物标记 `emittedFromDrain=true`。
- `firstSampleIndex` 是经延迟帐务后的输出展示采样位置。消费者不得把 delay 再加到时间上; delay 是证据，不是第二时钟。
- delay、实现版本或帐务状态缺失时返回 `validation/resample_timing_unavailable`，不得标记连续。

所有权规则:

- 生产者发布前独占可写，发布后 PCM 和 segment 描述不可变。DSP 任务持有 lease 直到所有窗口视图结束。
- 需要可写预处理时申请新缓冲; 不得 `const_cast` 或跨 lease 生命期缓存 pointer/span。
- 跨线程队列声明正数 `maxItems`/`maxBytes`，分析不丢 PCM。取消/失败唤醒两端，但不回收仍被持有的 lease。
- 进程内 lease 不原样跨 IPC，JSON 不承载 PCM。跨进程使用受控缓存/文件或后续版本化共享内存适配。

## 4. 采样索引与 `TimeNs`

同一 segment 任意每声道采样索引 `i` 的唯一数学映射为:

```text
exactTimeNs(i) = segmentOriginTimeNs
               + (i - segmentOriginSampleIndex) * 1_000_000_000 / sampleRate
```

中间值按数学无限精度或等价约分/checked 整数算法处理; 不先转 `double`，不逐缓冲累加已舍入 duration。

- 采样点和候选位置用 `nearest_ties_to_even`。
- 缓冲/特征覆盖 `[firstSampleIndex, firstSampleIndex + validFrameCount)`: 起点对精确式用 `floor`，终点用 `ceil`，两端分别计算。
- DTO `timeNs` 必须等于首索引的 nearest-even 结果; 上游 `durationNs` 仅用交叉验证，不作索引真值。
- 索引端点、与 origin 的差及最终 `TimeNs` 必须检查溢出; 错误为 `validation/time_overflow`。
- `firstSampleIndex` 可为负，预卷不夹为 0。只有转 A-012 项目候选/事件时要求 `ProjectTimeNs >= 0`。

逆映射为:

```text
exactIndex(t) = segmentOriginSampleIndex
              + (t - segmentOriginTimeNs) * sampleRate / 1_000_000_000
```

`first-not-before` 用 `ceil`，`last-not-after` 用 `floor`，最近采样用 `nearest_ties_to_even`; 恰好等距取偶数索引。调用方必须显式指定意图。A-014 公式是 `segmentOriginSampleIndex=0` 的直接特例。

| ID | 输入 | 期望 |
|---|---|---|
| `DTV-TIME-001` | 48 kHz，`i=1` | nearest-even `20833 ns` |
| `DTV-TIME-002` | 44.1 kHz，`i=1` | nearest-even `22676 ns` |
| `DTV-TIME-003` | 48 kHz，`i=-3` | `-62500 ns` |
| `DTV-TIME-004` | 48 kHz 覆盖 `[0,1)` | `[0,20834) ns` |
| `DTV-TIME-005` | 1024 Hz，`i=1`/`i=3`，精确值 `976562.5`/`2929687.5 ns` | nearest-even 分别取 `976562`/`2929688 ns` |
| `DTV-TIME-006` | `firstSampleIndex=INT64_MAX, validFrameCount=1` | `validation/time_overflow` |

## 5. 特征帧

```text
AudioFeatureFrame {
  schemaVersion: 1
  featureSchemaVersion: 1
  id: FeatureFrameId
  analysisRevision: AnalysisRevision
  inputFingerprintSha256: 64 lowercase hex
  segmentId: OpaqueId
  windowStartSampleIndex: Int64
  windowFrameCount: positive UInt32
  validInputFrameCount: UInt32 <= windowFrameCount
  paddingBeforeFrames: UInt32
  paddingAfterFrames: UInt32
  anchorSample: { numerator: Int64, denominator: 1 | 2 }
  anchorTimeNs: TimeNs
  coverageStartTimeNs: TimeNs
  coverageEndTimeNs: TimeNs
  channelAggregation: "mono" | "mean_energy" | "per_channel"
  shortTimeEnergy: FeatureMeasurement
  bandEnergies: ordered list<BandEnergy>
  spectralChange: FeatureMeasurement
  source: DspProducerSource
  extensions: ExtensionMap
}

FeatureMeasurement {
  definitionId: namespaced ASCII token
  value: { mantissa: Int64, decimalScale: Int32 in [-12,12], unit: ASCII token }
  normalizedPpm: optional NormPpm
  valid: bool
  invalidReason: optional ASCII token
}

BandEnergy {
  lowMilliHzInclusive: non-negative UInt64
  highMilliHzExclusive: positive UInt64
  measurement: FeatureMeasurement
}
```

- `validInput + paddingBefore + paddingAfter = windowFrameCount`。边界 padding 必须来自版本化参数; 特征窗不跨 segment/epoch。
- 锚点是窗口首末采样几何中点: `anchorSample = (2*start + windowFrameCount - 1) / 2`，分母为 1 或 2; `anchorTimeNs` 用 nearest-even。覆盖起止分别用 floor/ceil。
- 帧按 `(anchorTimeNs, FeatureFrameId ASCII bytes)` 升序。跨多个 PCM buffer 的窗必须持有全部相关 lease。
- 短时能量 `definitionId` 必须冻结声道聚合、去直流、窗函数、平方平均/总和和定标方式，不得只写 `energy`。
- 频段使用 `[low,high)` 毫赫兹整数边界，有序、不重叠，且 `high <= sampleRate * 500`。频点归属、窗能量校正和幅值/功率定义属于 `definitionId + parametersDigest`。
- 谱变化必须冻结幅度/功率域、正向差、归一化和首帧语义。首帧无前驱时使用 `valid=false, invalidReason=no_previous_frame`，不写伪 0。
- 稳定 DTO 不携带 NaN/无穷。原值使用带单位的十进制缩放整数; `normalizedPpm` 只在归一化作用域已版本化时填写，不同 definition/version 不可直接比较。

## 6. 瞬态、节拍候选与置信

```text
DspAnalysisCandidate {
  schemaVersion: 1
  candidateSchemaVersion: 1
  id: CandidateId
  analysisRevision: AnalysisRevision
  kind: "onset" | "beat"
  segmentId: OpaqueId
  sampleIndex: Int64
  timeNs: TimeNs
  durationNs: 0
  strengthPpm: NormPpm
  confidencePpm: NormPpm
  supportingFeatureFrameIds: ordered unique list<FeatureFrameId>
  tempoMilliBpm: optional positive UInt32
  beatPeriod: optional { numeratorFrames: positive UInt64, denominator: positive UInt64 }
  source: DspProducerSource
  diagnostics: ordered list<AnalysisDiagnostic>
  extensions: ExtensionMap
}

DspProducerSource {
  producerId: "space-rhythm.audio-dsp"
  producerVersion: ASCII semantic version
  algorithmId: namespaced ASCII token
  algorithmVersion: ASCII semantic version
  fftBackendId: ASCII token
  fftBackendVersion: ASCII string
  inputFingerprintSha256: 64 lowercase hex
  parameterSetId: OpaqueId
  parameterSetVersion: semantic version
  parametersDigestSha256: 64 lowercase hex
  deterministicSeed: optional UInt64
}
```

- `sampleIndex` 按第 4 节生成 `timeNs`。负时间预卷候选不夹到 0，不转 A-012 `AnalysisCandidate`，而记录 `candidate_before_project_origin`。
- 转 A-012 时 kind 原样映射为 `onset/beat`; source 补齐 `origin=analysis`、analysisRevision、输入指纹、producer 版本和参数摘要; payload owner 为 `space-rhythm.audio-dsp`、schema 1，含 segment/sample/特征证据。
- `strengthPpm` 是已声明归一化作用域内的信号强度; `confidencePpm` 是候选可信程度，不是产品自然度或放行结论。
- `tempoMilliBpm`/`beatPeriod` 只出现在 beat，并在参数容差内相符。onset 不携带伪节拍。
- 候选不可变，按 `(timeNs, kind ASCII bytes, CandidateId ASCII bytes)` 排序，不直接修改时间线。

```text
DspAnalysisResult {
  status: "success" | "low_confidence" | "no_signal" | "failed" | "cancelled"
  overallConfidencePpm: optional NormPpm
  reasonCodes: ordered unique list<ASCII token>
  featureFrames: ordered list<AudioFeatureFrame>
  candidates: ordered list<DspAnalysisCandidate>
  error: optional ErrorInfo
}
```

- 静音或低于版本化检测底限时返回 `no_signal`，候选为空，reason 至少有 `silence` 或 `below_signal_floor`。
- 弱瞬态、自由节奏、时长不足、周期性不足或噪声可返回 `low_confidence`，reason 使用 `weak_transients`、`aperiodic`、`insufficient_duration`、`noisy` 等稳定 token。
- `low_confidence/no_signal` 默认是结果而非程序错误。无足够周期证据时 `tempoMilliBpm` 缺省，不为界面伪造稳定 BPM。

## 7. 参数、算法和摘要版本

```text
DspAnalysisParameters {
  schemaVersion: 1
  parameterSchemaVersion: 1
  parameterSetId: OpaqueId
  parameterSetVersion: semantic version
  algorithmId: namespaced ASCII token
  algorithmVersion: semantic version
  sampleRate: positive UInt32
  channelAggregation: closed schema 1 token
  frameLengthFrames: positive UInt32
  hopLengthFrames: positive UInt32
  boundaryPolicy: "drop_incomplete" | "zero_pad"
  window: { id, version, coefficientsDigestSha256 }
  fft: { size: positive power-of-two UInt32, backendId, backendVersion, normalization }
  bands: ordered list<{ id, lowMilliHzInclusive, highMilliHzExclusive }>
  energyDefinitionId: namespaced ASCII token
  spectralChangeDefinitionId: namespaced ASCII token
  smoothing: VersionedIntegerParameters
  onset: VersionedIntegerParameters
  beat: VersionedIntegerParameters
  confidence: VersionedIntegerParameters
  deterministicSeed: optional UInt64
  requiredFeatures: ordered unique list<ASCII token>
  extensions: ExtensionMap
}
```

- T-030 只冻结 schema，不选定生产默认窗长、hop、FFT size、频段、平滑、峰值或 BPM 范围; 这些由 T-031 以独立 `parameterSetVersion` 提交。
- D-003 已确认自研 C++ DSP + KissFFT，但实际 KissFFT port/version/feature/ABI 只能来自固定构建 manifest，缺失时不填写推测版本。
- 参数使用整数、有理数和 token，不用 NaN/无穷或未规范化浮点文本。摘要是完整参数对象的 SHA-256，不是人类描述。
- schema 1 摘要编码为 UTF-8 JSON: object key 按 ASCII 升序，无多余空白，整数无前导零，array 保留语义顺序，字符串按 JSON 规则转义; SHA-256 为 64 位小写十六进制。
- `algorithmVersion` 或 backend 改变都必须进入来源并改变 `analysisRevision`。跨 backend 等价只能由已声明数值容差的向量证明。

## 8. 错误、诊断与取消

DSP 复用 A-012 `ErrorInfo` envelope。下列是 schema 1 需新增的稳定逻辑 code; 后续实现必须显式扩展映射，不得全折叠为 `internal_error`。

| category/code | 触发条件 |
|---|---|
| `validation/invalid_pcm_buffer` | 计数、stride、view、lease、epoch 或字节覆盖无效。 |
| `compatibility/unsupported_pcm_format` | 非 `f32_le` 或非 interleaved。 |
| `compatibility/unsupported_channel_layout` | 非 `FC`/`FL,FR` 或顺序不明。 |
| `validation/non_finite_pcm` | 有效帧内出现 NaN 或无穷。 |
| `validation/pcm_out_of_range` | 有限样本超出 `[-1,1]`。 |
| `validation/pcm_discontinuity` | 同 segment 索引不连续，或 origin/format/配方静默变化。 |
| `validation/resample_timing_unavailable` | 重采样 delay、版本或帐务状态缺失。 |
| `validation/timestamp_mismatch` | 上游时间/时长与精确索引映射不一致。 |
| `validation/invalid_analysis_parameters` | 窗/hop/FFT/频段/参数范围或摘要无效。 |
| `compatibility/unsupported_parameter_schema` | 参数 schema/required feature 不支持。 |
| `validation/invalid_feature_frame` | 窗口、padding、时间、测量或来源无效。 |
| `validation/invalid_analysis_candidate` | 候选时间、强度、置信、节拍字段或来源无效。 |
| `validation/time_overflow` | 沿用 A-012; 索引/时间/覆盖区间不可表示。 |
| `resource_limit/resource_limit` | 窗口、队列、帧数、内存或作业超限。 |
| `cancelled/cancelled` | 结果提交前观察到取消; 不发布部分候选伪装完整结果。 |

错误 context 可保存 fixture/segment/frame index、实际/期望格式、参数摘要和限额; 不默认记录 PCM 字节、完整用户路径或媒体内容。

## 9. 可复现向量与合法测试音色

规范清单为 [fixtures-v1.json](../../../tests/golden/audio/fixtures-v1.json)，生成/自校脚本为 [Generate-AudioDspVectors.ps1](../../../tests/golden/audio/Generate-AudioDspVectors.ps1)，许可声明为 [LICENSE.md](../../../tests/golden/audio/LICENSE.md)，实际字节证据为 [actual-hashes-v1.json](../../../tests/golden/audio/generated/actual-hashes-v1.json)。生成器仅使用整数/Q23 配方、固定 LCG 和 IEEE-754 binary32 小端编码; 不读取 FFmpeg、`package/`、用户媒体或第三方录音。

| ID | 类别 | 格式/长度 | PCM SHA-256 | 主要用途 |
|---|---|---|---|---|
| `AV-IMPULSE-001` | 脉冲 | 48 kHz mono，1 s | `bb58d0b6ca0c2b73b36b5629f64b2a141155f3592e50008099cd088f84ed5ba1` | 首/中/尾采样和窗边界 |
| `AV-FIXED-BEAT-120-001` | 固定节拍 | 48 kHz mono，2 s | `c44e3d6c621a7bd8c3d481e941688cb4a31a036a6c915b376594a616b23dcf0a` | 120 BPM 精确采样落点 |
| `AV-TEMPO-CHANGE-001` | 变速节拍 | 48 kHz mono，3 s | `9a023e968e7539bb17eb86147596378b800e1579e485b1de03597079ca72b6e3` | 120→180→240 BPM 间隔变化 |
| `AV-SILENCE-44100-001` | 静音 | 44.1 kHz mono，1 s | `fd6f479534cdd14635e88dfedf25c3859c01062b645f2a85570f20451b4a95bc` | `no_signal`、44.1 kHz 舍入 |
| `AV-NOISE-001` | 固定噪声 | 48 kHz mono，1 s | `d0a412e27b90675e067edf2aa674209a543601f6b5639dfcd91f407500aaa9d7` | 噪声/低置信语义 |
| `AV-BOUNDARY-001` | 有限边界信号 | 48 kHz stereo，16 frames | `4a4bc46ced2d02248933f73d233ab82cea0c43ce50561f22abba85b4beed0266` | `[-1,1]`、声道顺序、负索引 |
| `AV-NONFINITE-001` | 负向边界 | 48 kHz mono，4 frames | `9759c8518a6bc15a409b93899ec0b9b17013e2b747af489364d4ef42f4e95d54` | NaN/±Inf 必须拒绝 |
| `AT-CLICK-001` | 测试 click 音色 | 48 kHz mono，20 ms | `c7c80114be72af053793540bc5b5b6b80e9923672670405719d0902f0618ce28` | 采样精确触发/重叠 |
| `AT-LOW-PULSE-001` | 测试低频三角音色 | 48 kHz mono，100 ms | `328fe2e5b80b188dfd3a3fc7af33f1801429b0754038eda9f77bc478cff75e82` | 尾音/增益/削波 |
| `AT-NOISE-HIT-001` | 测试噪声打击音色 | 48 kHz mono，50 ms | `bf7868e48350a6fcbf3b2509e15c67658f203807e141d77df077b4ac8629152a` | 瞬态音色/确定性混音 |

所有 10 项都登记稳定 ID、输出文件、采样率、声道顺序、帧数、segment/origin、重采样声明、生成器/参数、规范配方、配方 SHA-256、PCM SHA-256、来源和 `CC0-1.0`。三个 `AT-*` 是 schema 1 的全部测试音色，只用于测试，不构成产品默认音色批准。

向量只冻结 PCM 和时间/错误 oracle。T-030 不写入短时能量、谱变化、瞬态或 BPM 的事后期望; 这些只能在 T-031 算法/参数版本确定后另行冻结。

## 10. 兼容性、版本与缓存

- `dspContractVersion`、`featureSchemaVersion`、`candidateSchemaVersion` 和 `parameterSchemaVersion` 分别演进；未知 major 或未知 required field 必须拒绝，不能静默降级。向后兼容的可选字段新增提升 minor，语义或必填约束改变提升 major。
- fixture/timbre ID 一旦发布即不可换字节、换配方或换许可；内容变化必须创建新 ID，并递增 `vectorSetVersion` 或 `timbreManifestVersion`。SHA-256 不匹配视为测试失败，不自动更新清单。
- 分析缓存键至少包含 `inputFingerprintSha256`、stream/segment/epoch、有效采样范围、DSP contract/schema 版本、algorithm/backend 版本、`parametersDigestSha256` 和确定性 seed。任一项变化不得命中旧结果。
- `analysisRevision` 必须唯一标识上述输入与处理配置；实现可用内容寻址 ID，但不得只使用文件路径、mtime 或显示名。
- JSON 只保存索引、时间、摘要和小型特征/候选 DTO。PCM、FFT scratch 和大批量内部帧不得内嵌到任务/事件 JSON；它们沿受生命周期约束的 buffer/lease 或专用二进制缓存传递。
- 未知 optional extension 必须保留或明确丢弃并记录诊断；未知 required feature、未知声道布局、未知时间帐务模式必须 fail closed。

## 11. 实现和消费者交接边界

- T-031 的 PCM adapter 应直接复用 A-015 的字节与 lease，补充本契约要求的 channel order 和 `ResampleTrace`；不得复制 PCM 来掩盖所有权问题，也不得从 PTS 猜 delay。若 FFmpeg 端尚未暴露 delay/version，返回 `resample_timing_unavailable` 或在生产边界显式补齐后再分析。
- T-031 冻结生产参数集和算法 oracle 时，需为每项期待值注明 algorithm/version、backend/version、参数摘要与容差；不得把本契约中的信号构造意图当作算法已经通过。
- T-032 只能使用已登记、许可清楚且 hash 匹配的音色。当前允许的测试音色仅为 `AT-CLICK-001`、`AT-LOW-PULSE-001`、`AT-NOISE-HIT-001`；产品默认音色仍需独立选型、许可核验和验收。
- core 只消费 A-012 映射后的不可变候选并负责用户批准/时间线提交；graphics 只消费稳定特征/候选视图；media 负责实际 PCM 与 resampler trace；DSP 不拥有媒体解码、时间线提交或产品资产授权。
- 音频回调线程不创建参数对象、不解析 JSON、不计算 SHA-256、不分配无界容器。参数与音色在非实时线程校验/准备，回调只读取不可变快照与已准备缓冲。

## 12. 追溯与自检

| 要求 | 契约位置/证据 |
|---|---|
| DSP 输入 PCM、格式、采样率、声道、交错和有效帧 | 第 2-3 节；`AV-BOUNDARY-001` |
| segment、`firstSampleIndex`、重采样 delay | 第 3 节；`DspPcmBuffer`/`ResampleTrace` |
| 样本索引到 `TimeNs` 的精确换算和舍入 | 第 4 节；`DTV-TIME-*` 与 44.1/48 kHz fixtures |
| buffer 所有权和跨窗生命周期 | 第 3、5、11 节 |
| 能量、频段、谱变化、瞬态、节拍候选 | 第 5-7 节；只定义 DTO 和语义，不实现算法 |
| 置信、来源、算法/参数版本与摘要 | 第 6-7、10 节 |
| 错误/诊断/取消 | 第 8 节；`AV-NONFINITE-001` 等负向向量 |
| 脉冲、固定/变速节拍、静音、噪声、边界信号 | 第 9 节和 `fixtures-v1.json` |
| 合法测试音色、生成方式、许可与 SHA-256 | 第 9、11 节，manifest 与 `LICENSE.md` |

自检结论：A-015 0.2 的 mono/stereo interleaved `flt` PCM、有效帧、采样起点、segment 和 lease 可无拷贝映射到本契约；其公共 DTO 尚不能证明 resampler delay、版本和显式声道顺序，因此本契约将其列为 adapter 的必填 provenance，而不虚构值。生成器已从规范配方重建全部 10 项并核对配方/PCM SHA-256、字节长度、许可和 schema；`-ValidateOnly` 可在不改写 PCM 时复核清单。T-030 不包含算法实现、产品音色批准、T-031/T-032 工作或 `package/` 内容。
