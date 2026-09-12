# 预览同步与安全导出事务

- 项目：space-rhythm
- 成果 ID：A-026
- 负责人：multimedia-engineer-ffmpeg-01
- 关联任务：T-019
- 版本：0.1
- 更新日期：2026-09-12
- 状态：draft
- 适用范围：以 C++ 状态实现预览主时钟、VFR 帧调度、暂停/恢复/seek、掉帧与漂移诊断，并以冻结输入、可替换编码器和同目录临时文件实现测试格式安全导出；不选择发布容器、H.264 后端或许可证方案，不实现 QML、视觉模板或 DSP。
- 来源及输入版本：A-004 0.5、A-006 0.1 WP-04、A-007 0.1、A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1、A-020 0.1、A-021 0.1、A-025 0.1；T-015、T-016、T-018、T-032、T-035 completed；D-003 confirmed；用户于 2026-09-12 确认前置满足并明确启动 T-019。
- 批准依据：尚无。测试格式与开发自测不批准发布容器、H.264 后端、G0～G4 或生产发布。
- 预览/导出契约：`contractVersion=0.1.0`，`schemaVersion=1`
- 版本记录：2026-09-12，0.1，首次交付实际播放采样主时钟、单调时钟降级、VFR 调度、冻结导出输入、FFmpeg 测试编码器、同目录事务提交和故障矩阵。

## 1. 模块与时间边界

`SpaceRhythm::PlaybackExport` 是纯 C++20 库，公共入口为 `space_rhythm/media/playback_export.hpp`。它依赖既有核心、媒体、音频渲染和渲染契约；公共 DTO 只保存 A-012 `core::TimeNs`、`TimeRange`、`TimelineRevision`，没有建立浮点秒、播放毫秒或其他同义项目时间类型。`std::chrono::steady_clock::time_point` 只作为无音轨时的瞬时计时器输入，经过核心 `scale_ticks` 映射后才形成播放头。

播放头由 `PreviewSynchronizer` 的 C++ 状态产生。QML/动画不参与计时、VFR 帧选择、seek 基线或掉帧判定；界面层后续只能消费 `PreviewUpdate.playheadTimeNs`、被选帧和诊断。

## 2. 预览时钟、状态与 VFR 调度

有音轨时，唯一主时钟输入是设备实际报告的累计播放 frame 数。开始、恢复或 seek 建立 `(audioAnchorFrames, anchorTimeNs)`，播放头采用核心最近偶数舍入：

```text
playheadTimeNs = anchorTimeNs
               + nearest_ties_to_even(
                   (actualPlayedFrames - audioAnchorFrames) / sampleRate)
```

墙钟经过十小时或更久也不会自行推进音频主时钟。设备在 seek 后重启计数时，调用方把新的实际计数传给 `seek`；同步器在同一操作内重置累计值和 anchor，不从 PTS、提交 PCM 数或日志反推已播放样本。

无音轨时使用 `std::chrono::steady_clock`。计时器 tick 先按其编译期 period 通过核心整数换算为 `TimeNs`，再加到 anchor；墙钟倒退、采样计数倒退和溢出均 fail closed。

- `pause` 先从当前主时钟固化播放头；暂停期间计时器不推进。音频设备可继续上报实际计数，但 `resume` 会从最新计数重新建立 anchor，暂停期间的设备变化不会跳动播放头。
- `resume` 不修改已冻结播放头，只更新单调时钟/音频计数基线。
- `seek` 原子更新目标 `TimeNs`、两类时钟 anchor 和帧选择状态；seek 前的漏显帧不计入 seek 后掉帧。
- VFR 选择不使用平均 fps。每次选择实际 `timeNs <= playheadTimeNs` 的最后一帧；跨过的中间帧计为 presentation drop，累计值与本次值分别公开。
- `video-clock-drift` 报告所选帧 PTS 相对主时钟的 lateness，`video-frames-dropped` 报告跳过数、播放头和所选帧时间；阈值由配置显式提供，不改变规范时间。

## 3. 导出冻结输入

`freeze_export` 在编码器打开前完成全部检查和深拷贝。旧修订以 `conflict/stale_revision` 拒绝；输入不完整或跨契约不一致时不产生作业。

| 冻结项 | 实际来源与检查 |
|---|---|
| `timelineRevision` | 调用方当前修订、`RenderSnapshot.timeline` 与 `RenderRecipe` 三者必须相同 |
| 媒体指纹/流选择 | 复制 `ExportMediaBinding`；每个已选 `StreamKey` 必须使用同一 SHA-256 指纹 |
| `RenderRecipe/RenderSnapshot` | 持有 T-033/T-035 不可变 snapshot，并深拷贝 recipe；后续编辑无法改变作业 |
| 音频参数 | 仅接受 T-032 schema/contract/algorithm/backend/parameter-set 的 48 kHz mono/stereo 配置 |
| 音色哈希 | 接受小写 SHA-256，排序后拒绝重复并冻结 |
| 时间范围、帧率、随机种子 | 调用值必须逐项等于 recipe；音频 seed 也必须相同 |

所有冻结字段进入 `frozenInputsSha256` 的规范串。摘要用于作业审计，不替代源媒体指纹、渲染 snapshot ID 或音频内容本身的校验。

## 4. 直接输入与测试编码器

导出协调器没有复制定义视频帧或音频 PCM DTO：

- `write_video_frame(const rendering::RenderedFrame&)` 直接消费 T-035/T-033 类型，并再次调用 `validate_rendered_frame` 校验 snapshot ID、修订、frame index、`timeNs`、尺寸、RGBA8/sRGB/full/top-down 和 lease；编码器在调用内逐行复制，调用返回后不保留 lease。
- `write_audio_pcm(const audio::render::RenderedPcm&)` 直接消费 T-032 输出，要求 frozen sample rate/channel、连续 `firstFrame`、精确样本长度和有限 `[-1,1]` f32 值；测试由 `DeterministicMixer` 实际生成 PCM。

`ExportEncoder` 是可替换接口。当前唯一允许的 descriptor 是显式测试格式 `space-rhythm.test.nut.raw-rgba-pcm-f32le`，`testOnly=true`，使用 NUT、RGBA rawvideo 和 interleaved PCM f32_le。实现通过 FFmpeg 8.1.2 C API/RAII 创建 codec/format/frame/packet，写 header、逐帧/逐 PCM chunk 编码、drain 和 trailer；没有 shell 命令、GPL/nonfree feature、H.264 或发布格式别名。

固定 vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`、`media` feature、FFmpeg `8.1.2#3`、动态 CRT、LGPLv3-or-later 配置及运行时 DLL 清单均沿用 A-015 0.3，本任务未修改 baseline 或依赖 feature。

## 5. 同目录提交与中断语义

临时文件名为 `<target>.<jobId>.space-rhythm-export.tmp`，由协调器强制放在规范化目标绝对路径的同一目录。只有视频/PCM 数量完整、编码器 drain/trailer 成功后才调用同卷 `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` 提交。

取消、资源不足、PCM/帧序列错误和编码失败都会先 `abort` 编码器，再删除本作业临时文件；已有目标从未在这些阶段打开。正常对象销毁模拟 worker 有序中断，同样 abort 并清理。若进程被强制终止，带 job ID 的临时文件可能留下供 T-016 恢复清理，但现有目标仍不会被写开或部分覆盖；重启不得把该临时文件当作成功结果。

同名 job 临时文件已存在时拒绝启动，避免静默接管未知中间产物。发布编码器、跨作业目标锁和崩溃恢复编排仍由后续集成/发布流程确认；本成果不宣称生产导出已经批准。

## 6. 测试矩阵与结果

`space_rhythm_playback_export_tests` 共 9 项 headless 用例，标签为 `t019;unit;integration;media;export;headless;gate-g1;gate-g3;gate-g4`：

| 覆盖项 | 证据 |
|---|---|
| 音频主时钟、VFR、丢帧、漂移 | 实际播放 sample count 精确推进；十小时墙钟无影响；VFR 选择与两类诊断断言 |
| 无音轨、暂停/恢复、seek | 单调时钟精确推进，暂停冻结，恢复重设 anchor，seek 不误计掉帧；另覆盖音频设备计数重启 |
| 一输出帧误差 | 30 fps 精确上限 `33,333,334 ns`；最大测试误差 `33,000,000 ns`，并有超限反例 |
| 旧修订/冻结深拷贝 | stale revision 拒绝；调用方后续修改媒体、音频和音色输入不影响 frozen snapshot |
| 真实 FFmpeg 与已有目标 | 两帧 RGBA `RenderedFrame` + T-032 `DeterministicMixer` 的 3,840-frame PCM 写入测试 NUT；已有字节只在成功后替换，并通过 `MediaSource` 重新探测到 video/audio 流 |
| 取消、磁盘不足、编码失败 | 可替换编码器注入对应失败；临时文件删除，原目标逐字节不变 |
| worker 中断 | 未 finish 即析构，验证 abort、无提交、临时文件清理和原目标不变 |
| 不完整/错序 | 缺帧或错误 `firstFrame` fail closed，不提交 |

Windows x64 `/W4 /WX` 三个 preset 均构建成功。Debug 实际执行 9/9 pass。CI/RelWithDebInfo 与 Release 的程序启动均返回 `0xC0E90002`；这与项目既有 `T021-ENV-001` Code Integrity/WDAC 阻断一致，未使用 fallback，也未把未执行写成通过。完整命令、哈希和原始日志位置见 [T-019 验证摘要](../evidence/T-019/verification-summary.md)。

## 7. 验收边界

- T-019 技术范围已完成：主时钟、VFR/状态/诊断、冻结导出输入、直接消费 T-035/T-032、测试编码器、同目录事务和要求的故障矩阵均有实现与 Debug 实际运行证据。
- 未批准：H.264、发布容器/codec、硬件编码、码率/质量、产品格式范围、许可证法律结论、G0～G4 和生产发布均未确认。
- H-004 保持 `accepted`；只有发起人 architect-01 核对 A-026/T-019 后才能关闭。
