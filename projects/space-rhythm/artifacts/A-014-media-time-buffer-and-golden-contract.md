# 媒体时间、流、缓冲与黄金样例契约 1.0

- 项目：space-rhythm
- 成果 ID：A-014
- 负责人：multimedia-engineer-ffmpeg-01
- 关联任务：T-017
- 版本：0.4
- 更新日期：2026-09-10
- 状态：draft
- 适用范围：第一阶段媒体探测、流选择、解码帧/PCM 交接、媒体时间到核心 `timeNs` 的映射、seek 结果和合法黄金样例；不冻结发布容器/编码器、硬件加速或许可证组合，不定义事件、项目时间线、IPC 传输或音频 DSP 算法语义。
- 来源及输入版本：[A-004 0.5](A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-04](A-006-domain-work-packages.md)、[A-007 0.1](A-007-four-engineer-execution-plan.md)、[A-012 0.1](A-012-core-domain-contract-0x.md)、[A-018 0.1](A-018-audio-dsp-pcm-feature-candidate-contract.md)；D-003/D-006 confirmed；H-004、H-011；用户于 2026-09-09 对 T-017、2026-09-10 对 T021-DEFECT-001 及 H-011 的明确执行要求。
- 批准依据：尚无。T-017 要求负责人自查并形成候选契约，无独立评审或用户批准要求。
- 媒体契约版本：`mediaContractVersion = 1.0.0`
- 逻辑 DTO schema：`schemaVersion = 2`
- 测试向量集：`vectorSetVersion = 1`
- 黄金样例清单：`goldenManifestVersion = 1`
- 版本记录：2026-09-10，0.4，H-011 将 PCM 的显式声道顺序、segment 原点及逐缓冲 resampler timing provenance 设为必填，按兼容规则将公共契约提升到 1.0.0、schema 2，增加旧版拒绝、音频 seek/格式切换/排空规则和动态音频黄金样例；2026-09-10，0.3，澄清 `ColorDescription.range` 的封闭公开词汇与 FFmpeg 枚举映射，禁止泄露 `tv`/`pc` 私有名称；2026-09-10，0.2，补充 `packet_dts` 事实来源，扩展长素材/动态格式样例并登记实际证据；2026-09-09，0.1，首次定义媒体基础契约。

## 1. 规范词与单一责任边界

“必须”“不得”“应”是规范要求；“可以”“建议”是非强制实现选择。只有满足相关规则和向量，才能声明兼容 `mediaContractVersion 1.0.0`。

媒体层拥有：

- 容器、节目和媒体流事实的探测与可信度；
- PTS、DTS、frame duration、流 `time_base`、format/stream start time、VFR、seek、解码音频采样位置的解释；
- 媒体事实到 [A-012 0.1](A-012-core-domain-contract-0x.md) 所定义 `TimeNs` 的精确映射；
- CPU 视频帧与 PCM 缓冲的格式、所有权、背压和生命周期；
- 旋转、镜像、SAR/DAR、裁剪和颜色事实的保留与显式归一化。

媒体层不拥有：

- 新的“媒体纳秒”“播放纳秒”或浮点秒时间模型；所有公共纳秒字段直接使用 A-012 的 `TimeNs`/`DurationNs`，已提交项目事件仍使用 A-012 的 `ProjectTimeNs`；
- `RhythmEvent`、`timelineRevision`、事务、撤销重做、算法候选、音频特征、渲染配方或 UI 状态；
- IPC 大缓冲传输机制、项目存储编码、发布格式矩阵、H.264 后端或硬件加速路线。

公共契约不得暴露 `AVFormatContext*`、`AVStream*`、`AVPacket*`、`AVFrame*`、`AVBufferRef*` 或只能在创建线程/进程有效的私有句柄。FFmpeg 类型只存在于媒体适配实现内。

## 2. 版本化媒体信息与流选择

### 2.1 `MediaInfo`

下列为逻辑字段，不是冻结的 C++ 头文件：

```text
MediaInfo {
  schemaVersion: 2
  mediaContractVersion: "1.0.0"
  sourceFingerprintSha256: 64 lowercase hex
  probeImplementation: { ffmpegVersion, buildConfigurationDigest }
  formatNames: ordered unique list<ASCII token>
  formatStart: optional RationalTimestamp
  duration: optional RationalDuration
  durationEvidence: exact | header | bitrate_estimate | pts_estimate | unknown
  streams: ordered list<StreamInfo>       // streamIndex 升序
  diagnostics: ordered list<Diagnostic>
  extensions: ExtensionMap
}
```

- 未知时间、时长、帧率、SAR、旋转和颜色必须是显式 `optional/unknown`，不得使用 `-1`、`INT64_MIN`、0/0、NaN 或空字符串冒充事实。
- 容器总时长、平均帧率和扩展名只可带来源使用；不能替代逐流和逐帧事实。
- 素材指纹至少覆盖实际输入字节；文件路径不参与内容身份。流标识只在该素材指纹下稳定。
- 探测上限、读取字节、包数、流数、耗时和取消结果进入诊断；超限不得返回“完整探测成功”。

```text
StreamInfo {
  schemaVersion: 2
  streamKey: { sourceFingerprintSha256, streamIndex }
  containerStreamId: optional Int64
  programIds: ordered list<Int64>
  kind: video | audio | subtitle | data | attachment | unknown
  disposition: { default, forced, attachedPicture, hearingImpaired, visualImpaired }
  language: optional normalized BCP-47-or-container token
  codec: { name, profile?, level?, extradataDigest? }
  timeBase: TimeBase
  startPts: optional Int64
  durationTicks: optional Int64
  rateMode: constant | variable | unknown
  averageFrameRate: optional Rational       // 描述字段，不是时间真值
  nominalFrameRate: optional Rational       // 描述字段，不是时间真值
  video: optional VideoFormat
  audio: optional AudioFormat
  extensions: ExtensionMap
}
```

### 2.2 流选择请求

调用者对每类流必须显式选择一种模式：

```text
StreamSelectionMode =
  none |
  explicit(streamKey) |
  required_default_then_lowest_index |
  optional_default_then_lowest_index |
  all
```

规则：

1. `explicit` 只接受同一素材指纹下、类型匹配且可解码的流；否则返回 `stream_not_found` 或 `stream_type_mismatch`。
2. `*_default_then_lowest_index` 先排除 attached picture（除非请求显式允许），再按“可解码、default disposition 优先、streamIndex 升序”确定唯一结果。多个 default 也按最低 streamIndex 选，并记录 `multiple_default_streams` 诊断。
3. `required_*` 无候选时返回 `missing_required_stream`；`optional_*` 无候选时返回空选择，不把缺失流伪装成解码失败。
4. `all` 按 streamIndex 升序返回所有匹配流；不得在后续阶段静默丢弃多音轨或多视频流。
5. `StreamSelectionResult` 必须返回选中 `streamKey`、选择模式、候选列表和理由；缓存键包含素材指纹与选择结果。

## 3. 媒体时间到核心 `timeNs`

### 3.1 原始时间戳

```text
RationalTimestamp {
  present: bool
  ticks: Int64                  // present=true 时有效
  timeBase: TimeBase            // 秒/刻度；分子、分母必须 > 0
  origin: packet_pts | packet_dts | frame_pts | best_effort | stream_start |
          format_start | decoded_first_presentation | synthesized_duration
}
```

这是媒体事实的 envelope，不是第二套规范时间。对外可比较和持久化的规范结果仍为 A-012 的 `TimeNs`。

- PTS 表示展示/播放位置，是视频帧和解码 PCM 映射的首选时间事实。
- DTS 表示解码顺序，只用于解码、seek/preroll 和诊断；不得直接成为展示 `timeNs`，也不得在 PTS 缺失时无条件顶替 PTS。
- 解码帧 `pts` 可用时使用它；仅在 `pts` 缺失且 FFmpeg 给出 best-effort timestamp 时可以使用后者，并将 `origin=best_effort` 和恢复诊断写入结果。
- 时间戳缺失时保持 `present=false`。schema 2 严格模式不使用平均帧率、帧序号或 DTS 猜测；没有可验证 PTS/best-effort 时返回 `timestamp_unavailable`。
- 允许显式启用 `synthesized_duration` 恢复策略，但必须以前一已知展示时间和可信 duration 为依据，标记为合成值，并进入映射版本/缓存键；VFR 上不得用 average fps 合成。

### 3.2 公共零点

同一选择结果的所有流必须共享一个 `PresentationOrigin`，不得让每条流各自归零而抹掉音视频起点差：

1. `MediaInfo.formatStart` 存在且可信时使用它；
2. 否则取所有已选择流中已知 `stream.startPts * stream.timeBase` 的数学最小值；
3. 否则在完成必要的解码重排后，取所有 required 选择流第一个有效展示时间的数学最小值；
4. 仍无事实时映射不可发布，返回 `timestamp_origin_unavailable`。

零点包含原始 ticks、time base、来源和选择结果摘要。使用第 3 条时，在全部 required 流得到首个展示时间前状态为 `provisional`，不得生成持久缓存或项目事件；冻结后若发现更早时间，返回 `timestamp_origin_changed` 并使相关派生缓存失效，不得静默整体平移旧结果。

### 3.3 精确换算

源时间戳与零点相同 time base 时先做 checked ticks 相减；不同时以精确有理数做一次差和一次最终舍入：

```text
exactRelativeNs =
  (pts * ptsBaseNumerator / ptsBaseDenominator
   - originTicks * originBaseNumerator / originBaseDenominator)
  * 1_000_000_000

timeNs = round(exactRelativeNs, explicitRoundingMode)
```

- 中间值按数学无限精度判断或使用等价的约分/checked 算法；不得先转 `double`。
- 非整除必须显式使用 A-012 的 `floor`、`ceil`、`toward_zero` 或 `nearest_ties_to_even`；不得引入媒体专用舍入枚举。
- 点位置、展示帧 PTS 和采样位置使用 `nearest_ties_to_even`。
- 半开覆盖区间 `[start,end)` 的起点使用 `floor`，终点使用 `ceil`，两端分别从未舍入的精确有理数计算；不得用已舍入 start 加已舍入 duration。
- 结果超出 Int64 返回 `validation/time_overflow`；非法 time base 返回 `validation/invalid_time_base`。
- 原始负 PTS、负 start time 和归一化后的负预卷都合法，必须保留为 `TimeNs`；只有写入已提交项目事件时才执行 A-012 的 `ProjectTimeNs >= 0` 校验。

### 3.4 CFR、VFR、重复和异常时间戳

- 有有效 PTS 时，无论 CFR/VFR 都逐帧映射 PTS；`frameIndex / averageFrameRate` 永远不是替代路径。
- `rateMode=constant` 需要观察到的展示 PTS 差值与可信 duration 一致；只有平均/名义帧率而无逐帧证据时为 `unknown`。
- VFR 每帧保留自己的 `timeNs` 和可选 `durationNs`。duration 优先取可信帧 duration；否则可由下一展示 PTS 的精确差得到；最后一帧没有可靠边界时 duration 保持未知。
- 重复 PTS 可以存在，使用 `decodeOrdinal` 作为同 PTS 诊断/交付顺序，不改变 `timeNs`。逆序、跳变、回绕修正或不连续必须保留原值并发出稳定诊断；不得为了“单调”静默夹紧或丢帧。
- 若适配器执行 PTS 回绕展开或容器特定修复，必须记录原始值、展开值、算法版本与触发原因，并纳入缓存键。

## 4. 音频采样索引

`PcmBuffer` 的 `firstSampleIndex` 是所声明 PCM 输出格式中、每声道的有符号 64 位样本索引；`sampleCount` 同样按每声道计数，不乘声道数。

```text
exactSampleTimeNs =
  segmentOriginTimeNs
  + (firstSampleIndex - segmentOriginSampleIndex) * 1_000_000_000 / sampleRate
```

- 样本点 `timeNs` 使用 `nearest_ties_to_even`；PCM 覆盖区间起点 floor、终点 ceil。
- `timeNs -> sampleIndex` 必须带目的：第一条不早于目标的样本用 `ceil`，最后一条不晚于目标的样本用 `floor`，最近样本用 `nearest_ties_to_even`。最近值恰好等距时取偶数索引。
- 同一连续 segment 的 `next.firstSampleIndex` 必须等于 `current.firstSampleIndex + current.sampleCount`。源时间戳缺口、重叠、丢样或格式变化开始新的 `segmentId` 并报告 discontinuity，不能靠改索引隐藏。
- `segmentOriginTimeNs` 是本 segment 第一条实际输入样本映射到 A-012 `TimeNs` 的位置；`segmentOriginSampleIndex` 是同一点在输出采样率索引域的值。两者在 segment 内不变，新 decode、seek、格式变化或时间不连续会创建新 segment 并重新取实际原点。
- 重采样后的索引属于明确的输出采样率。每次有输入转换前，媒体层用 `swr_next_pts` 从实际 resampler 状态取得已补偿 delay 的下一输出时间并以 `nearest_ties_to_even` 转成 `firstSampleIndex`；它必须与同 segment 的已交付连续末端相等，否则返回显式 discontinuity，不允许以累计输出帧数覆盖。drain 没有新输入时间，沿用上一实际输出末端。`delayAccountedInFirstSampleIndex=true` 是强制不变量，DSP 或其他消费者不得再次减/加 delay。

每个 schema 2 `PcmBuffer` 必须携带下列 `resampleTrace`；缺字段不允许以 PTS、累计输出帧数或日志反推：

| 字段 | 来源与规范值 |
|---|---|
| `performed` | 实际已初始化配置的输入/输出采样率是否不同；相同为 `false`，不同为 `true`。 |
| `inputSampleRate` / `outputSampleRate` | 当前已解码 `AVFrame.sample_rate` 与已初始化 `SwrContext` 的目标采样率。 |
| `implementationId` / `implementationVersion` | identity 为 `identity` / `1`；实际变采样为 `ffmpeg.swresample` / 运行时 `swresample_version()` 的 `major.minor.micro`。 |
| `parametersDigestSha256` | `space-rhythm.media.resample-parameters/v1` 规范串的 UTF-8 SHA-256；串包含实际输入/输出采样率、采样格式、声道布局、输出 planar 选择、实现 ID/版本、delay 查询基数及 `swr_next_pts` 索引策略。 |
| `delayBeforeInputFramesNumerator` / `Denominator` | 对本次 `swr_convert` 调用前立即读取的 `swr_get_delay`；查询基数为输入/输出采样率的 LCM，再约分为输入帧单位。分子非负、分母为正。 |
| `delayUnit` | 固定为 `input_frames`。 |
| `delayAccountedInFirstSampleIndex` | 必须为 `true`；这是消费者不得二次补偿的机器可检验声明。 |
| `emittedFromDrain` | 有输入的转换为 `false`；`swr_convert(..., nullptr, 0)` 实际产出的缓冲为 `true`。 |

identity 仍可使用 swresample 做采样格式/声道归一，但 timing 路径必须报告 `performed=false`、`identity/1`、delay `0/1` 且不产生 drain 缓冲；参数摘要仍来自实际配置。trace 的实现、版本与参数摘要在同一 resampler 实例内不变，delay 和 drain 标志按每次实际转换调用记录，不能要求逐缓冲完全相等。

状态变化规则：

- 普通 decoder flush 只取出尚未 receive 的输入帧，不新建 segment；其后 resampler drain 保持旧 segment/epoch/原点，实际输出标 `emittedFromDrain=true`。
- format change 先排空旧 resampler（输出仍归旧 segment），再以新 `AVFrame` 配置初始化 resampler，先发布新 `FormatChanged`/epoch，再开启新 segment 和新 trace 配置。
- 时间戳缺口/重叠在排空旧 resampler 后显式 `swr_close` + `swr_init` 清空状态；输出格式未变时 epoch 可不变，但 segment、原点和首个 delay 重新开始。
- audio seek 先执行 demux seek 与 decoder flush；丢弃目标之前的帧，并以 `ceil` 裁至第一条不早于目标的输入样本，然后用全新 resampler/segment/原点发布。seek 前实例不得向 seek 后 segment 排空。
- 取消或下游拒绝后立即停止，不 drain、不发布迟到缓冲、不虚构新 segment；已发布 lease 的生命周期不受影响。一次新的 decode 调用总是新 segment。
- PCM 的项目放置和 DSP 特征算法不属于本契约；消费者直接使用核心 `TimeNs` 和这里的样本索引映射。

## 5. Seek 契约

```text
SeekRequest {
  schemaVersion: 2
  selectedStream: StreamKey
  targetTimeNs: TimeNs
  mode: at_or_before | at_or_after | nearest | exact_or_error
  toleranceBeforeNs: DurationNs
  toleranceAfterNs: DurationNs
}

SeekResult {
  requestedTimeNs: TimeNs
  actualFrameTimeNs: TimeNs
  actualFrameDurationNs: optional DurationNs
  keyframeAnchorTimeNs: optional TimeNs
  decodedPrerollFrames: UInt64
  errorNs: TimeNs
  exact: bool
  streamKey: StreamKey
}
```

- 请求目标换回参考流 ticks 时，`at_or_before` 用 floor、`at_or_after` 用 ceil、`nearest`/`exact_or_error` 用 nearest-ties-to-even；min/max seek 窗分别用 floor/ceil。
- demuxer seek 成功只说明取得候选位置，不是命中目标。必须从可用关键帧/随机访问点解码并按展示 PTS 向前筛选。
- `at_or_before` 返回覆盖目标或最晚不晚于目标的帧；`at_or_after` 返回第一条不早于目标的帧；`nearest` 比较精确绝对误差，等距时选较早 `timeNs`；`exact_or_error` 没有完全相同展示时间时返回 `seek_unreachable`。
- VFR 使用真实展示时间和区间，不使用平均帧率。所有模式都返回实际落点、误差和 preroll 证据。
- 音频 `AudioOutputSpec.seekTargetTimeNs` 先以 floor 换回参考流 ticks 执行 backward demux seek，再按第 4 节解码并以 ceil 裁到第一条不早于目标的输入样本；新的 `segmentOrigin*` 取实际裁切样本，不能仅依赖 demuxer 返回值或包时间戳声称样本精确。

FFmpeg `avformat_seek_file` 的 `timestamp/min_ts/max_ts` 单位取决于参考流，且 seek 目标仍需解码验证；实现不得把 API 返回 0 当作契约成功结果。

## 6. 旋转、SAR/DAR、裁剪与颜色

### 6.1 显示几何

```text
DisplayGeometry {
  codedWidth, codedHeight: positive Int32
  cropTop, cropBottom, cropLeft, cropRight: non-negative Int32
  sampleAspectRatio: optional positive Rational
  displayTransform: optional {
    clockwiseRotationDegrees: Int32 in [0,359]
    mirrorHorizontal: bool
    mirrorVertical: bool
    source: display_matrix | rotate_metadata
    originalMatrixDigest: optional sha256
  }
  displayAspectRatio: optional positive Rational
}
```

- 先应用合法裁剪得到 clean aperture，再计算 `DAR = cleanWidth * SAR.num / (cleanHeight * SAR.den)` 并约分；SAR 未知时 DAR 也未知，不静默假定 1:1。
- 90°/270° 旋转后的展示 DAR 是旋转前 DAR 的倒数；0°/180° 不变。镜像不改变 DAR。
- display matrix 优先于旧 `rotate` metadata；两者冲突时保留双方、使用 matrix 并记录 `conflicting_display_transform`。
- FFmpeg 的 `av_display_rotation_get()` 返回逆时针角度；本契约字段是顺时针角度，因此适配器使用 `normalize(-counterClockwiseDegrees)`。接近整数的纯旋转按最近整数归一；奇异、透视、剪切或无法无损表达的矩阵返回 `unsupported_display_transform`，不得静默丢弃。
- schema 2 公共 CPU 帧保持源像素方向并携带 DisplayGeometry。实际旋转/镜像/重采样只在显式请求的归一化输出规格中发生，并产生新的格式 epoch。

### 6.2 颜色

`ColorDescription` 至少包含 `pixelFormat`、`bitDepth`、`range`、`primaries`、`transfer`、`matrix`、`chromaLocation`，以及可选 mastering display/content light metadata；每项允许显式 `unknown`。

- `range` 的公开值封闭为 `limited | full | unknown`。FFmpeg `AVCOL_RANGE_MPEG`（命令行/名称别名 `tv`）必须映射为 `limited`，`AVCOL_RANGE_JPEG`（别名 `pc`）必须映射为 `full`，`AVCOL_RANGE_UNSPECIFIED` 及适配器不认识的未来/非法值必须映射为 `unknown`；公共 DTO 不得暴露 `tv`、`pc` 或其他 FFmpeg 原始名称。
- 探测阶段的 codec parameters、解码帧事实及格式转换后的输出描述必须复用同一映射。转换到 full-range BGRA 的帧报告 `full`，不能报告 `pc`；这不把源素材的 `limited` 改写为源事实 `full`。
- 优先使用解码帧事实，再使用流 codec parameters；二者冲突时不静默覆盖，记录来源和 `color_metadata_changed`。
- 未确认字段不得按分辨率启发式伪装为源事实。若产品允许推断，推断结果必须标 `assumed`、说明规则并进入转换参数摘要。
- swscale/其他转换必须显式给出输入和输出颜色描述；转换后帧记录输出描述和转换参数摘要。丢失 HDR/高位深信息必须显式报告，不得称为无损归一化。

## 7. 帧与 PCM 缓冲所有权

### 7.1 共同 envelope

```text
BufferLease {
  bufferId: OpaqueId
  formatEpoch: UInt64
  byteSize: UInt64
  storage: cpu_read_only
  sharedOwner: implementation-defined reference-counted owner
}
```

- 生产者在发布前独占可写所有权；发布后只能通过不可变 lease 读取。需要修改的消费者必须申请唯一新缓冲或显式复制。
- view 中的 plane pointer/span 只在 lease 存活期间有效；缓存指针越过 lease 释放是契约错误。
- 缓冲池只有在最后一份 lease 释放后才能复用底层内存。取消、flush 或关闭不得回收仍被消费者持有的缓冲。
- schema 2 只交付 CPU 可寻址 plane；硬件 frame/context 留在适配层，必须先映射/转换或升级契约，不能暴露 FFmpeg 硬件上下文。
- 进程内 lease 不能原样跨 IPC。跨进程传输由 C-05 明确共享内存/复制、所有权转移、访问权限和释放握手后另行适配；JSON 不承载帧或 PCM 字节。

```text
VideoFrame {
  schemaVersion: 2
  streamKey: StreamKey
  timeNs: TimeNs
  durationNs: optional DurationNs
  sourcePts: optional RationalTimestamp
  sourceDts: optional RationalTimestamp
  timestampOrigin: frame_pts | best_effort | synthesized_duration
  decodeOrdinal: UInt64
  keyFrame: bool
  decodeHadErrors: bool
  geometry: DisplayGeometry
  color: ColorDescription
  pixelFormat: ASCII token
  planes: ordered list<{ offsetBytes, rowBytes: Int64, rows, validBytes }>
  lease: BufferLease
}

PcmBuffer {
  schemaVersion: 2
  mediaContractVersion: "1.0.0"
  streamKey: StreamKey
  timeNs: TimeNs
  durationNs: DurationNs
  segmentId: OpaqueId
  segmentOriginTimeNs: TimeNs
  segmentOriginSampleIndex: Int64
  firstSampleIndex: Int64
  sampleCount: UInt64
  sampleRate: positive UInt32
  sampleFormat: ASCII token
  channelLayout: versioned layout token
  channelOrder: ordered list<canonical channel token>
  planar: bool
  resampleTrace: ResampleTrace
  planes: ordered list<{ offsetBytes, strideBytes, validBytes }>
  lease: BufferLease
}
```

- plane offset、stride/rowBytes、rows 和 validBytes 必须在 `byteSize` 内 checked 验证；负 video rowBytes 合法但首地址与覆盖范围必须可验证。
- 动态尺寸、像素/采样格式、采样率、声道布局、颜色或方向变化必须先发布 `FormatChanged(newEpoch, newFormat)`，随后帧才可使用新 epoch；消费者不得用旧格式解释新缓冲。
- `decodeHadErrors=true` 的帧是否可消费由请求策略决定；不能无提示当成完整无损帧。

## 8. 背压与生命周期

每条数据通道在打开前必须声明 `maxItems` 与 `maxBytes`，两者均为正且参与资源配置；不得以“内存足够”为由使用无界队列。

```text
PublishResult = accepted | would_block | closed | cancelled
ChannelState = created | open | draining | ended | failed | cancelled | closed
```

- 默认策略是阻塞/异步等待或返回 `would_block`，不得静默丢帧/丢 PCM。预览若需要丢帧，由 T-019 以独立、显式、可诊断策略定义；分析与导出默认不丢。
- item 同时计入条目和字节配额；lease 仍被下游持有时字节配额不得提前返还。
- 取消和失败必须唤醒生产者/消费者。已发布 lease 仍按正常引用生命周期释放；不得通过强制回收制造悬空 view。
- `draining` 禁止新输入但允许消费已接受数据；只有已接受数据全部交付/释放后才发布 `ended`。失败终态携带 A-012 ErrorInfo envelope 和同一 diagnosticId。
- 控制面取消/错误/进度不得被满数据队列永久阻塞；数据面与控制面必须有独立可达路径。
- 关闭操作幂等。终态之后收到新数据返回 `closed/cancelled`，不得复活旧 channel 或复用旧 format epoch。

## 9. 结构化错误

媒体错误沿用 A-012 的 `ErrorInfo` envelope、diagnosticId、cause 和有界 context。schema 2 使用下列稳定 code；不得用 FFmpeg 自由文本作为程序分支：

| category/code | 触发条件 |
|---|---|
| `media/unsupported_media` | 容器、编解码器或必要 feature 不受支持。 |
| `media/corrupt_media` | 输入结构损坏或截断，无法按请求产生可信结果。 |
| `media/missing_required_stream` | required 流选择没有候选。 |
| `media/stream_not_found` | 显式 streamKey 不存在或素材指纹不匹配。 |
| `media/stream_type_mismatch` | 显式流类型与请求不符。 |
| `media/timestamp_unavailable` | 帧/PCM 无可信展示时间。 |
| `media/timestamp_origin_unavailable` | 无法冻结公共展示零点。 |
| `media/timestamp_origin_changed` | 冻结后发现更早起点，旧映射必须失效。 |
| `media/timestamp_discontinuity` | 时间跳变、逆序、回绕或缺口需要显式处理。 |
| `media/seek_unreachable` | 目标按请求模式/容差无法到达。 |
| `media/unsupported_display_transform` | 显示矩阵无法由 schema 2 无损表达。 |
| `media/format_changed` | 下游策略拒绝动态格式 epoch。 |
| `media/decode_failed` | 解码器在当前错误策略下不能继续。 |
| `validation/invalid_time_base` | 时间基缺失或分子/分母非正。 |
| `validation/time_overflow` | 精确时间、索引或缓冲算术不可表示。 |
| `resource_limit/resource_limit` | 探测、队列、内存、包、流或时长超过显式上限。 |
| `cancelled/cancelled` | 取消在终态前生效；不伪装成 decode_failed。 |

错误 context 只保存 streamIndex、时间戳、time base、stage、限额和摘要等有界标量；不默认记录媒体内容或完整用户路径。

## 10. 黄金样例矩阵

规范清单位于 [fixtures-v1.json](../workspace/tests/golden/media/fixtures-v1.json)，生成器为 [Generate-GoldenMedia.ps1](../workspace/tests/golden/media/Generate-GoldenMedia.ps1)，只读验证器为 [Test-GoldenMediaManifest.ps1](../workspace/tests/golden/media/Test-GoldenMediaManifest.ps1)，许可声明见 [LICENSE.md](../workspace/tests/golden/media/LICENSE.md)。它们都明确拒绝使用 `package/` 或外部视听内容。

### 10.1 样例登记

| ID | 覆盖 | 来源/许可 | 规范配方 SHA-256 | 主要期望 |
|---|---|---|---|---|
| GM-CFR-001 | CFR + A/V | 合成 testsrc2/静音；CC0-1.0 | `9a93e92f347e5929263fb72d54206d8f47ba36a0403c8a22da2576ae0a99577c` | video PTS ms `[0,40,80,120,160]`；timeNs `[0,40m,80m,120m,160m]`。 |
| GM-VFR-001 | VFR | 合成 testsrc2；CC0-1.0 | `7453fc778b309b98fac0f596b1f5cb14508e22e59bd211ff90137408e3af9d15` | PTS ms `[0,40,100,140,240]` 原样映射；不得按平均 fps 重建。 |
| GM-ROT-SAR-001 | 旋转、SAR/DAR、颜色 | 合成 testsrc2；CC0-1.0 | `5e0467cd9ed47cb2249ef09c838a57c8fc978f06d441335a5d1d25298e109fd3` | 16×8、SAR 4:3、编码 DAR 8:3、显示矩阵逆时针 -90° 映射为顺时针 90°，方向后 DAR 3:8、BT.709 limited。 |
| GM-MULTI-001 | 多流 | 合成视频+48k/44.1k 静音；CC0-1.0 | `39f68e0b16ff87885c4f4eb64b8d18bd9dfe0ad742ef2da0b8e872728ab6ba3b` | 1 video + 2 audio；默认选 eng/48k，显式可选 jpn/44.1k。 |
| GM-AUDIO-44100-001 | 44.1 kHz 采样 | 合成数字静音；CC0-1.0 | `21a78fffe12d8fae31cde268be751814362b0962331ae20050245dc5370bd84b` | 4410 samples；索引 `[0,1,2205,4409,4410]` 对应 `[0,22676,50m,99977324,100m]` ns。 |
| GM-AUDIO-48000-001 | 48 kHz 采样 | 合成数字静音；CC0-1.0 | `6837e8223eb7178c9569b087ee7a5f26a2102808aaa22e5fe05b2e0d9f200ef7` | 4800 samples；索引 `[0,1,2400,4799,4800]` 对应 `[0,20833,50m,99979167,100m]` ns。 |
| GM-AUDIO-DYNAMIC-001 | 动态采样率与 trace epoch | 两段 44.1/48 kHz 合成静音 MPEG-TS 拼接；CC0-1.0 | `a4488648fb7a505ed43567ad90be9123f6e84469de5f458de338392b45cf9820` | 48 kHz 输出先为 44.1→48 kHz swresample trace 并排空旧 segment，再切为 48 kHz identity trace 和新 segment/epoch。 |
| GM-CORRUPT-001 | 损坏媒体 | 固定截断 EBML 字节；CC0-1.0 | `a8bfb71271547ffd8ba34a1e642c76e219617bdbaf0b0b95db089a727c1b2495` | probe 返回 `media/corrupt_media`，不产生部分 MediaInfo 成功。 |
| GM-MISSING-VIDEO-001 | 缺视频流 | 合成静音；CC0-1.0 | `28fd80c48b8b12675c98ece2827c4000fa14453dee165c8bf626c966ca1077aa` | required video 返回 `missing_required_stream`；audio 可选。 |
| GM-MISSING-AUDIO-001 | 缺音频流 | 合成 testsrc2；CC0-1.0 | `57fdc675174150023ccf378665e0676646d0ed020bfa3118921ac241a4ed53e8` | required audio 返回 `missing_required_stream`；video 可选。 |
| GM-NEG-START-001 | 负 start/PTS | 合成 testsrc2+显式 PTS；CC0-1.0 | `f05d1d88be4457662b284bab2259283a23e3dd6f58f98e96410198f693ba39af` | `-copyts` 保留原始 ms `[-80,-40,0,40]`，公共零点 -80ms，timeNs `[0,40m,80m,120m]`。 |
| GM-LONG-001 | 长素材有界内存 | 合成 20 秒 A/V；CC0-1.0 | `268d33f6726fb53bcfaf74de3d2db66647c9a0d50034e102dbe99d31fa9796e4` | 500 个视频帧逐帧交付；16×16 BGRA 单缓冲峰值 1024 bytes，不整段加载。 |
| GM-DYNAMIC-001 | 动态视频格式 epoch | 两段合成 MPEG-TS 拼接；CC0-1.0 | `11b72d19cf3d4f95ab51e005c8dd2d6ba105cf0c253f0349e72acc7b90aedc37` | 16×16 → 32×16；必须先发布 epoch 1/2 的 `FormatChanged` 再交付对应帧。 |

表中的 hash 是 `canonicalRecipe` UTF-8 字节的 SHA-256，独立于文本换行和 FFmpeg muxer 的版本元数据。它冻结样例语义、来源和生成方法，不伪装成尚未生成的媒体文件 hash。

生成器在项目固定的 FFmpeg/ffprobe 构建可用后生成二进制，并自动写入已登记的 [actual-hashes-and-probe-v1.json](../workspace/tests/golden/media/generated/actual-hashes-and-probe-v1.json)，包含实际媒体 SHA-256、FFmpeg 版本及 configuration、recipe hash、逐帧/流/容器 ffprobe JSON 和退出码。实际媒体 hash 只对同一固定构建与配方有复现意义；媒体二进制仍被忽略，证据 JSON 纳入版本控制。FFmpeg 不可用时脚本在写输出前失败，`-ValidateOnly` 只检查清单和时间算术。

### 10.2 逻辑边界向量

| ID | 输入 | 期望 |
|---|---|---|
| MTV-PTS-001 | PTS=1，tb=1001/30000，origin=0 | nearest-ties-even `33366667ns`。 |
| MTV-PTS-002 | PTS=-1，tb=1/2e9，origin=0 | floor/ceil/toward-zero/nearest 分别 `-1/0/0/0ns`，与 A-012 TV-TIME-007 一致。 |
| MTV-PTS-003 | PTS 缺失、best-effort 缺失、DTS=10 | `timestamp_unavailable`；不得把 DTS 当 PTS。 |
| MTV-PTS-004 | PTS 缺失、best-effort=3、tb=1/25 | `timestampOrigin=best_effort`，`timeNs=120000000`，携带恢复诊断。 |
| MTV-PTS-005 | 两流 start 分别 -80ms 与 0ms | 公共 origin=-80ms；第二流首位置为 80ms，不可各自归零。 |
| MTV-VFR-001 | frame PTS ms `[0,40,100,140,240]` | 同值 `timeNs`；duration 前四项 `[40,60,40,100]ms`，末项未知。 |
| MTV-SEEK-001 | VFR 帧 100/140ms，目标 120ms，nearest | 两者等距，确定性选择较早 100ms，error=-20ms。 |
| MTV-SEEK-002 | 同上，exact_or_error | `media/seek_unreachable`。 |
| MTV-SAMPLE-001 | 44.1kHz，sample 1 | nearest `22676ns`。 |
| MTV-SAMPLE-002 | 48kHz，sample 1 | nearest `20833ns`。 |
| MTV-SAMPLE-003 | 44.1kHz，target=50000001ns | first-not-before index=2206；last-not-after index=2205。 |
| MTV-BUFFER-001 | 消费者仍持有 lease，producer flush/cancel | 底层内存不得复用；取消唤醒等待方，lease 正常释放。 |
| MTV-BACKPRESSURE-001 | 达到 maxBytes 但未达 maxItems | publish=`would_block` 或等待；不得入队或静默丢弃。 |
| MTV-FORMAT-001 | 分辨率/采样率改变但未发布新 epoch | 拒绝并返回 `media/format_changed`。 |

manifest 中另有 30 个可由 BigInteger 精确复算的 `expectedTimeVectors`；其 ID、输入、舍入和期望必须逐项匹配，不能通过修改期望值掩盖实现偏差。

## 11. 兼容与变更规则

- `mediaContractVersion` 使用语义版本。改变时间零点、选择顺序、舍入、seek 落点、所有权、背压或生命周期是破坏性变化，必须提升 major 并给出迁移和向量影响。
- 逻辑 DTO 增删必需字段、改变单位/范围或 enum 语义必须提升 `schemaVersion`。可忽略且可 round-trip 的数据只能放 namespaced `extensions`。
- schema 2 的 PCM provenance 是必填事实，schema 1 没有足够信息可无损迁移；适配器公开 `validate_pcm_schema()`，对 `schemaVersion=1/mediaContractVersion=0.1.0` 或任意不匹配组合稳定返回 `compatibility/unsupported_schema`。调用者必须用 schema 2 重新解码，禁止以 PTS、输出帧数、默认值或日志补字段。
- schema 1 的非 PCM 时间向量、黄金样例媒体字节和选择/颜色语义不被改写；清单继续保留 `vectorSetVersion=1`/`goldenManifestVersion=1`，只把其声明的当前媒体契约更新为 1.0.0，并新增独立动态音频样例 ID。
- `vectorSetVersion`/`goldenManifestVersion` 递增时保留旧 ID；改变旧向量期望必须说明反例与契约版本，不能复用 ID 偷换语义。
- FFmpeg 精确版本和 feature 集由 T-013/T-018 固定；适配器可变化，但公共 DTO、错误和向量不得随 FFmpeg 私有布局漂移。
- 消费者需要新增时间语义时先请求 A-012 所有者；T-017 不得通过 extension 创建第二套规范时间。

## 12. 自查与验证证据

- 完整对照 A-012 0.1：直接复用 `TimeNs`、`DurationNs`、`TimeBase`、四种舍入、checked overflow 和 ErrorInfo envelope；没有定义同义纳秒类型。
- 覆盖 T-017 完成条件：MediaInfo、流选择、旋转/SAR/DAR、颜色、帧/PCM、所有权、背压、生命周期、PTS/DTS/time_base、start/负/未知时间戳、CFR/VFR、seek、采样索引和结构化错误均有规范规则。
- 黄金矩阵覆盖 CFR、VFR、旋转、SAR/DAR/颜色、多流、44.1/48kHz、损坏、缺视频、缺音频、负起点、20 秒长素材、动态视频格式及动态音频采样率/trace；全部来源为项目合成或固定字节，许可证为 CC0-1.0，不使用 `package/`。
- 执行固定 FFmpeg 8.1.2 `Generate-GoldenMedia.ps1`：`GOLDEN_MEDIA_MANIFEST=PASS fixtures=13 timeVectors=30 contract=1.0.0`，`GOLDEN_MEDIA_GENERATION=PASS fixtures=13`；除故意损坏样例 ffprobe exit=1 外其余均为 0。
- 当前文件 SHA-256：manifest `f267c6d2e98d6a35279ff049ec306de3ab13c42c986b8608c033b507a4682704`；验证器 `e7245f13754c6a3ef0e61b36ac6f9eeaca677ae2033c3abe03d9cb35cba0f6fb`；生成器 `9ef146463cf3cd50bffdbe581cc3a7bb592121a8c3b307cafa75380853b11f41`；许可声明 `237a9ddf30e7be10962815cc9314487ed5aab7ddd124f96cd4697c83d00264f7`；实际 hash/ffprobe 证据 `238ed9c2832560ac04dbc6aaae6f8a24f0a7cdec7df317015931e88918c5b68a`。损坏样例的 ffprobe 非零退出时规范化 `probeJson=null`，连续两次生成的证据 hash 相同。
- T-018 已按本契约实现并由 [A-015 0.3](A-015-ffmpeg-media-pipeline.md)登记；H-011 provenance 与 T021-DEFECT-001 范围映射证据均已补齐，T-019 未启动。本成果不宣称发布编码器结论或生产许可批准。

## 13. 参考依据

- [FFmpeg AVFormatContext](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html)：format start time 使用 `AV_TIME_BASE` 单位，探测与资源限制字段属于 format 上下文。
- [FFmpeg AVStream](https://ffmpeg.org/doxygen/trunk/structAVStream.html)：stream time base 是帧时间戳单位，stream start time 是展示顺序首帧 PTS 且可未知。
- [FFmpeg AVFrame](https://ffmpeg.org/doxygen/trunk/structAVFrame.html)：decoded frame PTS/time base、best-effort timestamp、duration、色彩、采样率、plane 与引用计数缓冲事实。
- [FFmpeg swresample](https://ffmpeg.org/doxygen/trunk/group__lswr.html)：`swr_get_delay` 的基数单位、`swr_convert` 输入/排空调用和运行库版本事实。
- [FFmpeg demuxing/seek](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html)：seek 时间戳单位取决于参考流，`avformat_seek_file` 只定位候选展示点。
- [FFmpeg display matrix](https://ffmpeg.org/doxygen/trunk/group__lavu__video__display.html)：display matrix 变换与旋转角度方向。
