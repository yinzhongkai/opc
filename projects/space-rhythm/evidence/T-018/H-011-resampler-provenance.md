# H-011 PCM resampler timing provenance 验证摘要

- 日期：2026-09-10
- 负责人：multimedia-engineer-ffmpeg-01
- 任务/交接：T-018 / H-011
- 实现版本：`mediaContractVersion=1.0.0`，`schemaVersion=2`
- 固定依赖：vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`；`ffmpeg:x64-windows-space-rhythm@8.1.2#3`；运行时 libswresample `6.3.102`；未改变 default/GPL/nonfree feature 状态。

## 可复核结果

公共 `PcmBuffer` 已由媒体层实际填充 `channelOrder`、`segmentOriginTimeNs`、`segmentOriginSampleIndex` 及完整 `ResampleTrace`。变采样实现版本直接读取 `swresample_version()`；参数摘要来自 `space-rhythm.media.resample-parameters/v1` 实际配置规范串；delay 在每次 `swr_convert` 前以 `swr_get_delay` 和输入/输出采样率 LCM 查询并约分，`firstSampleIndex` 则在每次有输入转换前由 `swr_next_pts` 的 delay-compensated 状态确定并校验连续性，不读取日志，也不由 PTS 或输出帧数反推 resampler timing。

测试覆盖并通过：

- identity 44.1→44.1 kHz：`performed=false`、`identity/1`、delay `0/1`、无 drain 输出；
- 44.1→48 kHz 与 48→44.1 kHz：实际 `ffmpeg.swresample/6.3.102`、非零转换前 delay、各自产生 4800/4410 samples，drain 输出显式标记；
- 固定实际配置摘要 oracle：44.1 identity=`2ba50712d5d3c0e905fad2116cb85b1f5d21a235f584a77832163bcf023634a1`，44.1→48=`a105c7b1d8dae523e54001bbebb86cb25f7cbe41ce3d796fa39071a4f3da4a64`，48→44.1=`2d5d0829635a6f16d0578499d33df70ef765d12eab89753510c3828a181143cc`；输入布局 token 使用解码帧实际 `AVChannelLayout` 描述（该 WAV 为 `1 channels`），不替换成探测阶段的推测 `mono`；
- 非零 delay 缓冲的 `firstSampleIndex` 仍与上一缓冲末端严格连续，且 `delayAccountedInFirstSampleIndex=true`，证明下游不得再次补偿；
- 50 ms audio seek 精确落在 48 kHz sample index 2400，并创建新 segment、原点和全新 delay `0/1` trace；
- `GM-AUDIO-DYNAMIC-001` 在同一流内从 44.1 kHz 切换到 48 kHz：旧 44.1→48 resampler 先 drain，新 epoch/segment 使用 48 kHz identity trace；
- 取消在首个 PCM 后终止，不 drain、不创建迟到 segment；schema 1/contract 0.1.0 稳定拒绝为 `compatibility/unsupported_schema`。

DSP 边界采用只读复核，没有修改其实现：现有 `PcmNarrowAdapter` 只把 delay 与 `delayAccountedInFirstSampleIndex` 当作完整性证据，时间计算仅使用媒体给出的 `firstSampleIndex`、segment 原点和采样率，代码中没有把 delay 再加一次。当前 Debug `ctest -R Audio` 将 17 项既有 DSP 测试与 4 项媒体音频/生成测试一并实际执行，结果为 21/21 pass（10.26 s）；结合非零 delay 下媒体索引连续断言，证明当前消费路径不会做第二次延迟补偿。

## Windows x64 媒体专项

命令模式为先构建 `space_rhythm_media_tests` 与 `space_rhythm_media_contract_tests`，再执行 `ctest --preset <preset> -L media --output-on-failure`。

| preset | 配置 | 结果 | 实际 CTest 时间 |
|---|---|---|---|
| `windows-msvc-x64-debug` | Debug `/MDd` | 27/27 passed | 17.16 s |
| `ci-windows-msvc-x64` | RelWithDebInfo `/MD` | 27/27 passed | 14.44 s |
| `windows-msvc-x64-release` | Release `/MD` | 27/27 passed | 16.30 s |

三套最终产物的媒体 GTest、公共契约、黄金生成器和审计均直接运行，无 Qt/QML WDAC 回退。提交前代码审阅按 FFmpeg 头文件约束把 `swr_get_out_samples` 与 `swr_get_delay` 放到可能改变状态的 `swr_next_pts` 之后；三套最终数据均来自该次序修正后的重新编译。Release 在修正前的一版旧二进制哈希上曾出现进程创建前策略拒绝，最终重新链接产物已真实执行 27/27；没有修改或绕过主机策略，也没有把旧哈希的失败改写为通过。

固定生成器结果为 `fixtures=13 timeVectors=30 contract=1.0.0`；动态音频实际 SHA-256 为 `ff6c7d5d0a584aa24d86f772441143e101268cbb4ddbd640911a180ef633de4e`，ffprobe exit=0；完整实际证据文件 SHA-256 为 `238ed9c2832560ac04dbc6aaae6f8a24f0a7cdec7df317015931e88918c5b68a`。损坏样例的非零 ffprobe 输出被确定性登记为 `probeJson=null`，连续两次生成 hash 一致。

## 边界

未修改 DSP 实现，未启动 T-019，未处理 WDAC，未修改 `package/`，未纳入 `scripts/__pycache__/`。
