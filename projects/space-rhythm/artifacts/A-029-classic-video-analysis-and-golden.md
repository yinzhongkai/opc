# 经典视频分析实现、真实 golden 与自测

- 项目：space-rhythm
- 成果 ID：A-029
- 负责人：video-algorithm-engineer-cv-01
- 关联任务：T-028
- 版本：0.1
- 更新日期：2026-09-14
- 状态：draft
- 适用范围：经典镜头切换、全局/局部运动和动作峰值分析，真实合成 golden、确定性、PTS/VFR、资源、取消及性能自测；不包含核心时间线融合、T-029 模型评估、产品效果批准或“卡点自然”门禁。
- 来源及输入版本：用户于 2026-09-14 明确要求执行 T-028 且不得提前启动 T-029；D-003 confirmed；A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1、A-028 0.2。
- 批准依据：尚无。
- 版本记录：2026-09-14，0.1，完成经典 OpenCV 实现、10 项真实 golden、三配置自测及 measured/not-evaluated 效果和性能基线。

## 1. 实现结论

新增 `SpaceRhythm::VideoAnalysis` 纯 C++ 静态库，公共接口见
[`analysis.hpp`](../workspace/src/video_analysis/include/space_rhythm/video/analysis.hpp)，实现见
[`analysis.cpp`](../workspace/src/video_analysis/analysis.cpp)。实现直接消费媒体 schema 2 已归一化
BGRA `VideoFrame` 及其真实 `timeNs`，输出核心 schema 1 `AnalysisCandidate`；不重建名义帧率时间，
不写核心时间线，也不改变媒体层 PTS 所有权。

版本基线为 `videoAnalysisContractVersion=0.1.0`、算法
`space-rhythm.video-analysis.classic@1.0.0`、参数集
`space-rhythm.video-analysis.production@1.0.0`、OpenCV `4.12.0`。最终规范化参数摘要为
`321ae1fc8c905dfd108c003898eaf0f2c761b9eae67f4ae0479b34e68b896574`。

## 2. 经典算法

- 镜头：灰度直方图 Bhattacharyya 距离与平均结构变化联合判硬切；连续单向亮度/结构变化判渐变；相反方向的单帧亮度突变与恢复识别为 flash 并抑制伪切镜。
- 运动：Farneback 稠密光流固定参数；采样流中位数作为全局运动，强残差分位作为局部运动；按真实时间窗口、显著性和确定性间距选择局部峰，并标记 `global/local/mixed`。
- 动作：组合局部残差幅度变化、方向反转和真实 PTS 窗口内的“历史运动→后续近静止”分数；全局运镜占主导时抑制动作候选。输出只表示编辑候选，不声称完成主体识别。
- 确定性：算法无隐式随机源；固定 seed、整数 PPM 归一化、规范参数 SHA-256、稳定候选 ID 和 `(timeNs,kindRank,id)` 排序。相同输入/参数在测试中逐字段复跑一致。

每个候选均包含真实 `timeNs`、`durationNs`、`strengthPpm`、`confidencePpm`、输入 fingerprint、
分析 revision、算法/参数版本和证据窗口。shot payload 含边界种类与直方图/结构证据；motion
含全局/局部值、显著性与运动类别；action 含 acceleration/reversal/stop 与全局抑制值。
`AnalysisResult` 镜像 job、输入 fingerprint、stream key、seed、revision、input range、core/media
契约版本、参数 schema/hash 和运动曲线 schema，失败也保留可追溯请求身份。
结果同时返回逐相邻帧的有界 `MotionCurveSample`，含真实前后 `timeNs`、segment、全局运动、
局部残差、综合运动和动作值；取消或失败会清除曲线与候选，不暴露半成品。
低质量事实使用稳定 token，如 `flash_ambiguous`、`compression_noise`、
`timestamp_discontinuity`、`sampling_gap`、`format_epoch_boundary` 和 `decode_errors`。

## 3. 真实 golden

[`Generate-GoldenVideo.ps1`](../workspace/tests/golden/video/Generate-GoldenVideo.ps1) 以项目自制 C#
确定性 BGRA 帧和锁定 FFmpeg `8.1.2` 生成 10 项 FFV1/bgr0 MKV。生成器支持正常生成、
`-RawOnly` 隔离编码和 `-ValidateOnly`；媒体文件保持 gitignored，仓库提交配方、实际 SHA-256、
字节数和 ffprobe 证据。完整记录见
[`actual-hashes-and-probe-v1.json`](../workspace/tests/golden/video/generated/actual-hashes-and-probe-v1.json)。

同一生成流程连续执行两次，10 项媒体 SHA-256 全部一致。CFR 样本分别为 30/1 或 60/1；
VFR 样本实际 PTS 为 `0, 40, 100, 140, 240, 400, 600 ms`，算法候选落在真实 240 ms，未按
frame index 重建。

## 4. 自测与测量

Debug、CI/RelWithDebInfo、Release 三套 MSVC x64 目标均在 `/W4 /WX` 下构建通过。三套最终
Windows 测试二进制分别在 `wine-8.0-debian-bookworm-container` 隔离环境实跑，均为 8/8
GoogleTest 通过，覆盖契约/非法输入、硬切/渐变/flash、全局/局部运动、慢动作、VFR、压缩
噪声、混合快切动作、确定性、PTS 回跳分段、有界内存和分析中取消。

合成矩阵测量见 [`effect-measurements-v1.json`](../evidence/T-028/effect-measurements-v1.json)：
shot 为 TP=5/FP=0/FN=0，motion_peak 为 5/0/0，action_peak 为 4/0/0；三类匹配绝对时间误差
P95 分别为 0/33/33 ms。该结果仅代表冻结的合成契约集，状态为 `measured`，效果和
“卡点自然”均为 `not-evaluated`。

性能记录见 [`performance-measurements-v1.json`](../evidence/T-028/performance-measurements-v1.json)：
160×90、90 帧全局运镜样本在 Release 隔离运行的 10 次中位耗时 193615 us、P95 201492 us、
中位 464 analyzed fps、约 15.32× realtime；分析器估算峰值工作内存 359960 bytes；20 次取消
延迟中位 1920 us、P95 2344 us。CPU-only 路径的 GPU 时间/显存明确 unavailable；基准硬件和
性能门槛未确认，仍为 `measured/not-evaluated`。

## 5. 环境限制与边界

本机对最终 Release 测试程序的原生启动在进入 `main` 前返回 `0xC0E90002`，与项目已登记的
WDAC/SAC/Code Integrity 环境问题一致；锁定 FFmpeg 的原生启动也受同类限制。因此本成果不
宣称 Windows 原生运行通过。为取得真实执行证据，使用未改写的同一 Windows PE 测试、FFmpeg
和 FFprobe 二进制在 Wine 8.0/Debian bookworm 容器执行；构建仍由本机 MSVC 完成。详细命令、
结果和限制见 [`verification-summary.md`](../evidence/T-028/verification-summary.md)。

产品真实素材、人工修正量、自然度 rubric、基准硬件及通过阈值仍未提供。本任务不据合成集
批准产品效果，不引入模型。T-029 保持 `todo`，本轮未启动。
