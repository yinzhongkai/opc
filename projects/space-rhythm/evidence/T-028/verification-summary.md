# T-028 验证摘要

- 日期：2026-09-14
- 执行人：video-algorithm-engineer-cv-01
- 成果：A-029 0.1；`SpaceRhythm::VideoAnalysis`
- 工具链：MSVC 19.44、CMake/Ninja、OpenCV 4.12.0、FFmpeg/FFprobe 8.1.2、GoogleTest 1.17.0
- 参数摘要：`321ae1fc8c905dfd108c003898eaf0f2c761b9eae67f4ae0479b34e68b896574`
- 测量状态：效果、性能和“卡点自然”均为 `measured/not-evaluated`，未设置产品通过阈值。

## 最终验证

1. `windows-msvc-x64-debug`、`ci-windows-msvc-x64`、`windows-msvc-x64-release` 的视频分析库、单元、golden measurement 和 benchmark 目标均在 `/W4 /WX` 下构建通过。
2. 三套最终 Windows 测试 PE 在 `wine-8.0 (Debian 8.0~repack-4)`/Debian bookworm 容器分别实跑 8/8 GoogleTest 通过；断言包括精确候选数量和全局/局部运动曲线，不只检查“附近存在”。Debug 25.021 s、CI 4.192 s、Release 4.245 s。
3. 确定性：相同输入/参数连续分析的状态、候选、诊断和内存估算逐字段相等；10 项 golden 从原始配方连续完整生成两次，全部媒体 SHA-256 相等。
4. PTS/VFR：VFR 解码帧时间严格等于 `0,40000000,100000000,140000000,240000000,400000000,600000000 ns`，motion/action 均输出真实 `240000000 ns`；另以 PTS 回跳断言 segment 重置和 `timestamp_discontinuity` low-quality 诊断。
5. 有界内存：输入帧数、单帧 bytes、特征/时间前缀缓存、运动曲线、候选数和总 working bytes 均在分配/追加前检查；1-byte working limit 和少一帧的 frame limit 均 fail closed。代表样本分析器估算峰值 359960 bytes。
6. 取消：预取消返回 `cancelled` 且无曲线/候选；3000 帧分析中取消实际终止，性能程序 20 次测得中位 1920 us、P95 2344 us。
7. golden manifest 校验返回 `GOLDEN_VIDEO_MANIFEST=PASS fixtures=10 coverage=9 contract=0.1.0`；磁盘媒体 hash/byte count 与 manifest、ffprobe evidence 三方交叉核对通过。
8. JSON 证据可解析；`git diff --check` 通过。

框架级 `scripts/validate_framework.py` 本轮无法完成：可用 Python 本身可启动，但已缓存
PyYAML 的 `yaml/__init__.py` 被主机策略拒绝读取并返回 `PermissionError`。未联网替换宿主
依赖，也未把该项记为通过；T-028 新增文档的实际本地链接目标已逐项检查存在。

## 效果测量

完整逐样本候选、置信度、低质量 token 和 payload 见
[`effect-measurements-v1.json`](effect-measurements-v1.json)。

| kind | TP | FP | FN | precision/recall/F1 ppm | 绝对时间误差 P95 |
|---|---:|---:|---:|---:|---:|
| shot | 5 | 0 | 0 | 1000000/1000000/1000000 | 0 ns |
| motion_peak | 5 | 0 | 0 | 1000000/1000000/1000000 | 33000000 ns |
| action_peak | 4 | 0 | 0 | 1000000/1000000/1000000 | 33000000 ns |

压缩噪声样本为 `low_quality` 且无候选，flash/static 负例无候选。以上仅是项目自制合成契约集
测量，不外推到产品真实素材，不构成自然度或产品效果通过。

## 性能测量

[`performance-measurements-v1.json`](performance-measurements-v1.json) 记录 Release CPU-only 路径：

- 90 帧、160×90、输入时长 2.967 s；3 次 warmup、10 次测量。
- wall time 中位 193615 us、P95 201492 us；中位 464 analyzed fps；中位 realtime factor 15315761 ppm。
- 进程 peak working set 观测前后均为 52215808 bytes；分析器内部估算峰值 359960 bytes。
- GPU timing/memory 为 `unavailable(reason=cpu-only-classic-path)`。

执行环境不是确认的基准硬件，数值保持 `measured/not-evaluated`。

## 原生 Windows 限制

最终 Release 测试 PE 在本机原生启动前返回十进制 `-1058471934` / 十六进制 `0xC0E90002`，
未进入 GoogleTest main；锁定 FFmpeg/FFprobe 和一次全仓 Release 的非 T-028 `qmlcachegen` 也受
同类 WDAC/SAC/Code Integrity 策略限制。本次没有更改、绕过或关闭主机安全策略，也没有把
容器结果写成原生 Windows 通过。

隔离执行通过仓库挂载调用本机 MSVC 产出的原始 Windows PE；未重编译为 Linux 版本。真实
golden 由相同锁定 Windows FFmpeg 8.1.2 生成，并由相同 Windows FFprobe 8.1.2 探测。生成器
证据见
[`actual-hashes-and-probe-v1.json`](../../workspace/tests/golden/video/generated/actual-hashes-and-probe-v1.json)。

## 交接状态

T-028 的负责人实现与自测已完成。H-007 保持 `accepted`，等待发起人核对；T-029 仍为
`todo`，本轮没有启动模型门禁或产品效果评估。
