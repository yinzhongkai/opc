# FFmpeg 媒体探测、解码、时间映射与代理管线

- 项目：space-rhythm
- 成果 ID：A-015
- 负责人：multimedia-engineer-ffmpeg-01
- 关联任务：T-018
- 版本：0.3
- 更新日期：2026-09-10
- 状态：draft
- 适用范围：只读媒体探测、流选择、解复用、视频/音频解码、媒体时间到核心 `TimeNs` 的映射、CPU 格式归一、代理帧、缩略图、波形源 PCM、视频 seek、样本精确音频 seek、缓冲 lease/背压、资源与取消边界；不含 T-019 的播放同步或导出事务，不含 UI、CV、DSP、发布编码器和安装器批准。
- 来源及输入版本：[A-012 0.1](A-012-core-domain-contract-0x.md)、[A-013 0.1](A-013-windows-x64-cmake-ci-skeleton.md)、[A-014 0.4](A-014-media-time-buffer-and-golden-contract.md)、[A-018 0.1](A-018-audio-dsp-pcm-feature-candidate-contract.md)、D-003 confirmed、H-004、H-011；用户于 2026-09-10 对 T-018、T021-DEFECT-001 及 H-011 的明确执行要求。
- 批准依据：尚无；任务完成不自动批准成果。
- 实现契约：`mediaContractVersion = 1.0.0`，`schemaVersion = 2`
- 版本记录：2026-09-10，0.3，处理 H-011：PCM DTO 增加必填声道顺序、segment 原点和逐转换 resample trace，来源绑定实际 AVFrame/目标布局、`swresample_version()`、版本化配置摘要及转换前 `swr_get_delay()`；新增样本精确音频 seek、动态音频格式样例、旧 schema 拒绝和三套媒体专项证据；2026-09-10，0.2，修复 T021-DEFECT-001；2026-09-10，0.1，首次实现并验证 FFmpeg 媒体基础管线。

## 1. 固定依赖解析与许可证

`vcpkg.json` 仍使用原 baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`，没有升级或替换。`media` feature 的 FFmpeg 依赖保持 `default-features: false`，固定解析结果如下：

| 项目 | 实际值 |
|---|---|
| port | `ffmpeg:x64-windows-space-rhythm@8.1.2#3` |
| vcpkg ABI | `0b20ea8b04628351d2acf4b27f5dfbe6ff52836ee735fd5b338c6848ae840253` |
| 显式/隐式 feature | `core,avcodec,avdevice,avfilter,avformat,ffmpeg,ffprobe,swresample,swscale,version3` |
| 未启用 | default feature 集、`gpl`、`nonfree` |
| target/toolchain | Windows x64，MSVC 19.44，shared，Release/CI `/MD`，Debug `/MDd` |
| configuration SHA-256 | `e952f157587581ba14fe1c26e12f358da5c48b779fb3d3493a56ec936c761259` |
| 运行时许可证 | `GNU Lesser General Public License version 3 or later` |

实际 configuration 开启 `version3`、`ffmpeg`、`ffprobe`、`avcodec`、`avdevice`、`avformat`、`avfilter`、`swresample`、`swscale`、shared、Windows threads 与 Windows 媒体/硬件接口；没有 `--enable-gpl` 或 `--enable-nonfree`。完整原文保存在 [黄金媒体实际证据](../../../tests/golden/media/generated/actual-hashes-and-probe-v1.json)，不是由本文重写的推断。

实际库版本为：libavcodec 62.28.102、libavdevice 62.3.102、libavfilter 11.14.102、libavformat 62.12.102、libavutil 60.26.102、libswresample 6.3.102、libswscale 9.5.102。`avdevice`/`avfilter` 供固定的黄金生成工具闭包使用；只读适配器本身使用 avformat/avcodec/avutil/swresample/swscale。

## 2. 实现边界

公共头 [media.hpp](../../../src/media/include/space_rhythm/media/media.hpp) 不暴露 FFmpeg 类型，所有纳秒位置直接使用 A-012 `core::TimeNs`/`DurationNs`。`RationalTimestamp` 只保存容器事实，不构成第二套规范时间。

| 能力 | 实现与契约点 |
|---|---|
| 探测 | `AVFormatContext` + interrupt callback；记录源 SHA-256、容器、流、codec、time base/start/duration、SAR/DAR、display matrix、颜色和音频格式；探测字节、时长、流数、源大小均有上限。 |
| 流选择 | 显式 key、required/optional default-first、all；按素材指纹、类型、可解码性、attached picture 与 stream index 确定性处理，多 default 发稳定诊断。 |
| 时间映射 | PTS 优先、best-effort 仅作显式恢复、DTS 只作来源/诊断；共同 presentation origin；MSVC 192-bit checked 有理数运算，只在最终一步调用 A-012 舍入语义。 |
| 视频 | send/receive 解码循环；`AVCOL_RANGE_MPEG/JPEG/UNSPECIFIED` 与未知值分别统一为公开 `limited/full/unknown`，探测和解码帧共用映射且不暴露 `tv/pc`；swscale 显式输入矩阵/range 到 `full` BGRA；可选应用纯 0/90/180/270° 显示旋转；尺寸、颜色、方向或输出规格改变时先发 `FormatChanged` 再发新 epoch 帧。 |
| 音频 | swresample 输出 interleaved float mono/stereo；从实际目标 `AVChannelLayout` 逐项发布 `channelOrder`。每个缓冲在调用 `swr_convert` 前以 LCM 基数读取 `swr_get_delay()` 并约分到输入帧，携带实际输入/输出率、运行时 swresample 版本、配置 SHA-256、delay 已计入索引及 drain 标志；格式/时间不连续先排空旧实例再开启新 segment。 |
| 代理与 seek | 代理帧保持宽高上限，缩略图给定规格；视频 seek 将核心时间精确反算为流 ticks并验证展示落点；音频 seek backward 定位后丢弃目标前帧，以 ceil 裁至第一条不早于目标的输入样本，并用全新 resampler/segment 发布。 |
| 所有权/背压 | FFmpeg format/codec/frame/packet/sws/swr/SHA 均由 C++ custom-deleter RAII 管理；发布后为不可变 shared `BufferLease`；有界队列同时限制 items/bytes，最后一个下游 lease 释放前不返还字节配额。 |
| 失败与取消 | corrupt、missing/type mismatch、timestamp、seek、format、decode、resource limit、cancelled 进入 A-012 `ErrorInfo`；取消、drain、close 为独立且幂等的生命周期路径。 |

适配器不按 `frameIndex / averageFrameRate` 生成 CFR/VFR 时间，不把负 PTS 截为 0，不用 DTS 冒充展示时间，也不缓存整段素材。

### 2.1 PCM resampler provenance 实现

公共 `PcmBuffer` 现在直接携带 schema/契约版本、`channelOrder`、`segmentOriginTimeNs`、`segmentOriginSampleIndex` 和 `ResampleTrace`。字段生成路径没有日志解析或输出数量反推：

- 输入率/格式/布局取当前 `AVFrame`；输出率/格式/布局取成功 `swr_init` 的配置，声道顺序由该实际输出 `AVChannelLayout` 的 channel index 逐项查询。
- timing identity 由实际输入/输出率相等确定，标识为 `identity/1`；变采样标识为 `ffmpeg.swresample/<swresample_version()>`。`space-rhythm.media.resample-parameters/v1` 规范串覆盖上述实际配置，使用 libavutil SHA-256 生成小写 64 hex 摘要。
- 每次有输入转换以及每次 drain 的 `swr_convert` 前立即读取 `swr_get_delay(context,lcm(inputRate,outputRate))`，约分成输入帧有理数。输入转换 `emittedFromDrain=false`，空输入 drain 的实际输出为 `true`。
- 每次有输入转换前以 `swr_next_pts` 读取实际 resampler 的 delay-compensated 下一输出时间，按 `nearest_ties_to_even` 形成 `firstSampleIndex`；它与已交付末端不一致时 fail closed，而不是用输出帧数覆盖。drain 无新输入时间才沿用上一输出末端。`delayAccountedInFirstSampleIndex=true` 明确禁止 DSP 再补偿；单测在观测到非零 delay 的相邻缓冲上仍断言严格连续，以防二次偏移。
- decoder EOF flush 不切 segment；随后的 swresample drain 属于旧 segment。格式变化先 drain 旧实例再创建新 epoch/segment；时间不连续 drain 后 `swr_close`/`swr_init`，格式未变时保留 epoch 但更换 segment。seek 使用新的 decoder/resampler/segment；取消或下游拒绝不 drain。

这是必填语义的破坏性变化，因此公共版本由 0.1.0/schema 1 提升为 1.0.0/schema 2。schema 1 不含可无损恢复的 delay/版本/配置事实，`validate_pcm_schema()` 对旧版本稳定返回 `compatibility/unsupported_schema`；唯一迁移行为是使用当前适配器重新解码，禁止补默认值。

## 3. 黄金媒体与实测摘要

固定生成器完成 13 个 CC0-1.0 合成/固定字节样例、30 个精确时间向量；新增动态音频格式样例用于真实 swresample epoch/segment 证据。实际媒体 hash 与逐帧 ffprobe 原文见 [actual-hashes-and-probe-v1.json](../../../tests/golden/media/generated/actual-hashes-and-probe-v1.json)，证据文件 SHA-256 为 `238ed9c2832560ac04dbc6aaae6f8a24f0a7cdec7df317015931e88918c5b68a`。

| fixture | 实际媒体 SHA-256 | ffprobe |
|---|---|---|
| GM-CFR-001 | `85cd9761dc428617431f53dcc124df766d3813ef6c1671d2b6a2b3c0c7f1297c` | 0 |
| GM-VFR-001 | `cfa0bd714598b7034382c7ca2359ea72183e83f16ccb7dfc30f37a1f72dc43b2` | 0 |
| GM-ROT-SAR-001 | `20f26854c2489fb40803c8b4073db4990b614bff9c6f59be605d96baa9b412eb` | 0 |
| GM-MULTI-001 | `9ef2430d551f2af3ff22d7af16334fc64736e097afcbec63512dede8f870b549` | 0 |
| GM-AUDIO-44100-001 | `f971a9aaa8626894132d1d3d7139e0f06b01f3beb894f680f5033eccfbbcd969` | 0 |
| GM-AUDIO-48000-001 | `639dad0ac2923f5fe9e9ccfb99aa9b3084048e2e53317d1903e6e899a4f6296a` | 0 |
| GM-AUDIO-DYNAMIC-001 | `ff6c7d5d0a584aa24d86f772441143e101268cbb4ddbd640911a180ef633de4e` | 0 |
| GM-CORRUPT-001 | `ae9dd0845c07d6a876f88162a29130453a3ac4e3999a71296416f926211dffb5` | 1（预期损坏） |
| GM-MISSING-VIDEO-001 | `a5bace0a9919f277b41bf534f3be64fad14eea07351a179e179265935436b9ec` | 0 |
| GM-MISSING-AUDIO-001 | `a609dd9b3a41d8897c2050ef77f87d542115fd0d5bae83611478b2a5d20679b4` | 0 |
| GM-NEG-START-001 | `ed410c2d20dd29280cb8c4e808b5b1c9c38f9e907b5024745edd2fbfd73bc0fb` | 0 |
| GM-LONG-001 | `a22465b40aeeb91c6771f4346b66ccd1076236d23676d938dd0421d2d4612ffe` | 0 |
| GM-DYNAMIC-001 | `34f411f2fef1c71708ad0dc93e53c0839818aa5855bf7e72b18886ba175230e5` | 0 |

关键实测：CFR/VFR 输出分别精确匹配 `[0,40,80,120,160] ms` 与 `[0,40,100,140,240] ms`；负 PTS 原始值为 `[-80,-40,0,40] ms`，共同零点映射为 `[0,40,80,120] ms`；48 kHz 输入重采样到 44.1 kHz 后含 drain 共 4410 samples；VFR 120 ms nearest seek 在等距时命中 100 ms；动态样例在 16×16/32×16 帧前分别发布 epoch 1/2。

长样例为 20 秒、500 帧。16×16 BGRA 每帧 1024 bytes，实测 `peak_single_buffer_bytes=1024`、累计交付 512000 bytes；测试回调逐帧释放，不随素材时长增长。队列另验证 2 items/4 bytes 上限及取出后仍持有 lease 时的 4-byte 配额保留。

## 4. 构建、CTest 与运行时闭包

三套 x64 preset 均在 MSVC `/W4 /WX` 下完成配置和编译，CTest 最终结果均为 49/49，媒体专项为 16/16：

| preset | 配置 | CTest | 证据目录 |
|---|---|---|---|
| `windows-msvc-x64-debug` | Debug `/MDd` | 49/49，20.31 s | `out/evidence/T-018/windows-msvc-x64-debug-test-verified` |
| `windows-msvc-x64-release` | Release `/MD` | 49/49，17.81 s | `out/evidence/T-018/windows-msvc-x64-release-test-verified-retry` |
| `ci-windows-msvc-x64` | RelWithDebInfo `/MD` | 49/49，19.74 s | `out/evidence/T-018/ci-windows-msvc-x64-test-verified` |

本机对新生成 Qt smoke EXE 偶发在 `main()` 前无输出；最终 CTest 使用 T-013 已提供的显式 `-AllowWdacFallback`，只把已知 WDAC 情形转交固定 Qt 6.11.2 `qmltestrunner` 复核。Release clean build 后首次 GTest discovery 也遇到一次同类无输出，未修改二进制的重试通过 49/49；FFmpeg 媒体测试没有走回退，16 项全部直接运行。

安装命令已分别从 clean Debug/Release 构建实际复制 7 个 FFmpeg shared DLL。逐文件核验表明 Debug 安装件与 vcpkg `debug/bin` 全部同哈希、Release 安装件与 vcpkg `bin` 全部同哈希，且每个 Debug DLL 都与对应 Release DLL 不同；Release 名称、大小和 SHA-256 见 [runtime-dlls.sha256.csv](../evidence/T-018/runtime-dlls.sha256.csv)。安装后的应用 QML smoke 又被同一主机策略超时，因此这里只确认媒体 DLL 的配置匹配安装闭包，不宣称发布安装或产品运行批准。

### 4.1 T021-DEFECT-001 修复回归

独立测试 oracle 仍为 A-014 的 `limited`，没有改成 FFmpeg 原名 `tv`。新增映射单测覆盖 MPEG、JPEG、UNSPECIFIED、`AVCOL_RANGE_NB` 和非法负值；集成测试同时验证探测信息为 `limited`、full-range BGRA 缩略图/代理帧为 `full`。Debug `media` 标签通过 22/22；T-021 Debug headless 与 CI/RelWithDebInfo headless 最终各通过 62/62。CI 首轮仅有一次 `MediaResource.LongMaterialKeepsWorkingBuffersBounded` 进程未启动，颜色相关测试全部通过；未改代码、二进制、测试或主机策略的完整重试通过 62/62。原始日志保存在忽略的 `out/evidence/T-018/T021-DEFECT-001/`，可提交摘要见 [缺陷修复证据](../evidence/T-018/T021-DEFECT-001.md)。既有 Qt/QML smoke 显式回退未修改，本修订不处理 WDAC。

### 4.2 H-011 PCM provenance 回归

新增/扩展媒体测试覆盖 identity、44.1→48 kHz、48→44.1 kHz、非零 delay 下连续索引、drain、50 ms/sample 2400 音频 seek、动态采样率切换、取消不排空，以及 schema 1 拒绝。Windows x64 Debug、CI/RelWithDebInfo、Release 的最终 `media` 标签均为 27/27 通过，三套都由次序修正后的最新二进制直接执行，未使用回退。只读核对既有 DSP adapter 确认 delay 只参与 provenance 校验，时间仍仅由 `firstSampleIndex` 与 segment 原点计算；Debug `ctest -R Audio` 的既有 DSP/媒体音频组合回归为 21/21 pass，未修改 DSP 实现。完整字段来源、用例结果、命令形态和实际哈希见 [H-011 验证摘要](../evidence/T-018/H-011-resampler-provenance.md)。

## 5. 结论与边界

固定 baseline 的 FFmpeg 8.1.2#3 满足 T-018/A-014 所需只读探测、解码、精确时间、格式、seek、PCM provenance、缓冲与资源契约，无需换版。T021-DEFECT-001 与 H-011 均已修复并回归；T-018 恢复完成，T-019 保持 `todo`，没有实现预览主时钟、同步、导出事务、DSP 或发布编码器。
