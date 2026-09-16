# 音频事件渲染、确定性混音与预览实现

- 项目: space-rhythm
- 成果 ID: A-020
- 负责人: audio-dsp-engineer-01
- 关联任务: T-032
- 版本: 0.1
- 更新日期: 2026-09-11
- 状态: draft
- 适用范围: 由 A-012 `RhythmEvent` 生成 48 kHz mono/stereo f32_le PCM，提供确定性离线 PCM/WAV 和 `QAudioSink` 试听传输；不选择产品默认音色，不修改媒体实现或容器导出，不执行后续任务。
- 来源及输入版本: A-012 0.1、A-013 0.2、A-016 0.1、A-017 0.2、A-018 0.2、A-019 0.2，D-003 confirmed；用户于 2026-09-11 明确启动 T-032 并限定使用 A-018 三个合法 CC0 测试音色。
- 批准依据: 尚无。测试音色、性能和设备覆盖均不等于产品音色、效果、硬件或发布批准。
- 渲染契约: `renderContractVersion=0.1.0`，`schemaVersion=1`
- 版本记录: 2026-09-11，0.1，首次实现纯 C++ 混音核心、Qt 预览适配、PCM/WAV oracle、测试与测量。

## 1. 模块和责任边界

`SpaceRhythm::AudioRender` 是纯 C++20 静态库，只公开 `space_rhythm/audio/render.hpp`，链接 `SpaceRhythm::Core`，不链接 Qt、FFmpeg、KissFFT 或设备 API。`DeterministicMixer::prepare` 完成事件映射、输入验证、时间换算、稳定排序和资源预算；`render` 与 `render_chunk` 运行同一整数混音路径。

`SpaceRhythm::AudioPreviewQt` 是薄 Qt 传输层，只接受核心已经产生的 `RenderedPcm`。它以 48 kHz、mono/stereo、IEEE float32、interleaved 的 `QAudioFormat` 创建 `QAudioSink`，用 `QBuffer` 提供完全相同的 PCM 字节；设备层音量固定为 1.0，不再次增益、削波、重采样或混音。设备为空、格式不支持或 sink 初始化失败均只终止试听，不改变离线 PCM/WAV。

WAV/PCM 是音频渲染结果，不是媒体容器导出实现。schema 1 WAV 为 RIFF/WAVE、`WAVE_FORMAT_IEEE_FLOAT=3`、32 bit、little-endian，`data` 与 `pcm_f32le_bytes` 逐字节一致；更复杂容器、metadata 和编码仍属于媒体工作流。

## 2. 合法测试音色清单

实现只接受 A-018 `timbreManifestVersion=1` 的下列三项。加载时同时校验 fixture ID、固定元数据、字节数、SHA-256、有限范围 `[-1,1]` 和精确 Q23 表示；构造后 `prepare` 再根据 Q23 重建规范 f32_le 并复核 SHA，避免调用者修改已登记内容。未知、变造或非有限音色 fail closed。

| fixture | timbre ID | frames | PCM SHA-256 | 许可/来源 |
|---|---|---:|---|---|
| `AT-CLICK-001` | `test.click.linear-decay.v1` | 960 | `c7c80114be72af053793540bc5b5b6b80e9923672670405719d0902f0618ce28` | CC0-1.0；项目整数公式生成 |
| `AT-LOW-PULSE-001` | `test.low-triangle.linear-decay.v1` | 4,800 | `328fe2e5b80b188dfd3a3fc7af33f1801429b0754038eda9f77bc478cff75e82` | CC0-1.0；项目整数公式生成 |
| `AT-NOISE-HIT-001` | `test.noise-hit.linear-decay.v1` | 2,400 | `bf7868e48350a6fcbf3b2509e15c67658f203807e141d77df077b4ac8629152a` | CC0-1.0；项目固定 seed LCG/整数公式生成 |

清单和生成许可仍以 A-018 的 `fixtures-v1.json`、`Generate-AudioDspVectors.ps1` 和 `LICENSE.md` 为准。本成果没有添加录音或第三方素材，也没有把这些音色登记为产品默认音色；产品选型状态为 `not-evaluated`。

## 3. 事件映射与采样级触发

`RenderParameters.mapping` 以 A-012 `EventKind` 显式映射到一个已加载 fixture，并携带规则增益与线性声像。没有规则的事件被忽略；同一 kind 的重复规则、未知 fixture、重复事件 ID、负项目时间、非法 strength/gain/pan 或版本不匹配均拒绝。schema 1 不提供隐式产品默认映射；测试参数使用 onset→click、beat→low-pulse、manual→noise-hit。

输出起点为 `startTimeNs`，事件相对采样索引的唯一换算是：

```text
triggerFrame = nearest_ties_to_even(
    (event.timeNs - startTimeNs) * 48_000 / 1_000_000_000)
```

实现先把纳秒差拆成整秒和非负余数，再以 checked int64 运算完成换算；half tie 看最终整数采样奇偶，不用 `double`。事件早于输出起点时索引可为负，只要音色尾音与 `[0, frameCount)` 相交就参与；晚于结尾或尾音完全在起点前则不分配输出贡献。事件按 `(triggerFrame,eventId ASCII bytes)` 稳定排序。

## 4. 确定性混音、增益与削波

音色加载后以有符号 Q23 保存。每个样本严格按以下顺序做整数 `nearest_ties_to_even`：事件 `strengthPpm`、映射 `gainPpm`、`masterGainPpm`、输出声道 pan gain；每一级分母固定为 1,000,000。允许的规则/主增益为 0～4,000,000 ppm，声像为 -1,000,000～+1,000,000 ppm。

mono 直接输出。stereo 使用确定性线性声像：中心左右均为 1.0；负值保持左声道并线性衰减右声道，正值相反。所有事件以 int64 累加，资源上限保证最坏贡献不会溢出；完成一个 chunk 后硬削波到 Q23 `[-8,388,608,+8,388,608]`，再精确转换为 float。`RenderStatistics` 报告映射事件数、贡献数、削波样本数和削波前/后峰值。

这一路径不使用随机分支、FPU 累加顺序或设备回调时序。seed 固定为 `0x5350414345524859` 并进入参数摘要，为后续版本化扩展保留；当前更换 seed 不产生随机音频。algorithm/backend/parameter 固定为：

- `space-rhythm.audio-render.integer-q23@1.0.0`
- `portable-cxx20@1.0.0`
- `space-rhythm.audio-render.test-timbres@1.0.0`
- 规范参数摘要 `3e0eb79a7a61b75bf671c6c17ece300408e2a0e0ea29f2f158f4b2b63ffa17c7`

`render_chunk` 使用输出绝对 frame 范围重新求交，不保存块间 DSP 状态；任意合法分块拼接必须与 `render` 一次输出逐 float/逐字节相等。取消在开始和每 256 个贡献帧检查；观察到取消返回 `cancelled/cancelled`，不发布部分 PCM。输出帧、样本、事件、音色帧与贡献数均有独立上限。

## 5. Oracle、测试和设备失败

[render-oracles-v1.json](../workspace/tests/golden/audio/render-oracles-v1.json) 冻结一个 4,800-frame stereo low-pulse 场景：PCM SHA-256 为 `9a8211f7d16e8e42a69d76623c3b5f1d80e7fc78484c752017975e2f53f3d182`，WAV SHA-256 为 `a2b9c9f33733e9c3e64486aaeabe706a44334a49155012bb2c05224a4de34614`。精确值、事件顺序、重复运行和分块边界的数值容差均为 0。

自动化覆盖三音色清单/哈希/许可、变造与 NaN、最近采样触发、起点前尾音、重叠、规则/主增益、声像、硬削波、重复运行、事件乱序、1/257/1024/17/3333-frame 分块、PCM/WAV 同源、取消、资源上限、未知产品音色和 null `QAudioDevice`。null 设备返回 `device_unavailable`，测试同时复核调用前后离线 PCM 字节未变。

最终 Windows 结果和原始日志位置见 [T-032 验证摘要](../evidence/T-032/verification-summary.md)。Debug/CI 功能与 golden 有 13/13 同源成功记录，Release 同源专项曾为 14/14；最终提交前复跑时，当前 WDAC/SAC 策略又阻止 Release 单元测试进程，故最新 Release 专项按 blocked 记录。Release golden、性能程序仍可运行并提供测量。项目级 Release 全门禁另有既有 Qt smoke 失败，没有宣称完整 Release 门禁通过。

## 6. 验收边界

- 技术完成：事件映射、采样级触发、重叠/尾音、非有限值拒绝、增益/声像、削波、离线 PCM/WAV、Qt 设备适配、取消与资源限制已经实现并受自动化覆盖。
- 确定性完成：同事件、音色、版本、参数与 seed 的 PCM 字节一致；任意分块与单次渲染相同；Debug/CI/Release 的黄金 PCM/WAV hash 一致，Release 的 120 秒测量另有固定 hash。
- 未批准：产品默认音色、主观效果、设备矩阵、最低硬件、吞吐/内存/取消门槛和项目 G0～G4 都仍为 `not-evaluated`。
- 下游：媒体导出或 UI 可以消费本模块公开输出，但本任务没有修改媒体实现、应用 UI 或启动任何后续任务。
