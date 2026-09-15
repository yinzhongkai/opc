# 视频产品效果、性能与可选模型门禁评估

- 项目：space-rhythm
- 成果 ID：A-030
- 负责人：video-algorithm-engineer-cv-01
- 关联任务：T-029
- 版本：0.4
- 更新日期：2026-09-15
- 状态：draft
- 适用范围：代表产品视频、真实 VFR 技术集、个人单用户人工参考/盲评/修正、经典视频算法效果与性能，以及可选模型门禁；不批准产品口径，不把合成样本或平台转码流替代真实 VFR，不直接引入 ONNX 或修改核心时间线。
- 来源及输入版本：用户于 2026-09-15 明确要求依据 A-031 0.5/D-014 继续 T-029、准备 `USER-01` 三阶段流程，并在当前 `TIGER` 最佳性能状态下执行 T-028 Release PE 一次预热加五次正式测量；D-001～D-014 confirmed；A-002 0.2、A-016 0.1、A-028 0.2、A-029 0.1、A-031 0.5 approved；H-017 closed；dataset 0.2.0。
- 批准依据：尚无。
- 版本记录：2026-09-14，0.1，登记 T-029 启动、输入审计、评估协议和模型边界；0.2，实际审计 40 个来源并冻结 36 个可用代理；0.3，按 D-010/A-031 0.3 完成 4 个替换来源，结构恢复 40 条但实测 VFR 仍为 0。2026-09-15，0.4，接收 A-031 0.5/D-014 的个人单用户协议，形成 USER-01 三阶段工具和真实 PTS 界面；在 TIGER 最佳性能 overlay 下原生完成一组有效 T-028 Release PE `1 warmup + 5 measured` 基线。真实 VFR、实际人工记录和正式 T-029 产品/VFR 性能仍缺失，overall 保持 `not-evaluated`。

## 1. 当前输入和执行就绪结论

历史来源审计、替换和配额事实保留在 `input-readiness-v1`、`execution-readiness-v2/v3`；本轮收口见
[`execution-readiness-v4.json`](../evidence/T-029/execution-readiness-v4.json)。

| 输入/前置 | 当前事实 | T-029 判定 |
|---|---|---|
| 来源权限 | D-009 只允许本项目内部测试且不得外发，不构成第三方权利法律结论 | `confirmed_boundary` |
| 产品主集 | 40/40 媒体和 probe 已冻结，`calibration/tuning/final=4/16/20`、四产品类别各 10 | `structural_pass` |
| 真实 VFR 技术集 | D-011 要求 3 个真实原始 VFR、至少 2 个设备/软件族、`tuning=2/final>=1`；实际文件仍为 0 | `not-evaluated(reason=missing_actual_original_VFR_media)` |
| 语义 slice | 声明标签满足静态配额，但尚无 `USER-01` 冻结参考，不能当真值 | `not-evaluated` |
| USER-01 工具 | 40 条参考会话、40 个本地静音 H.264 预览、逐帧 preview→source PTS 映射和三阶段 UI 已就绪 | `tooling_ready` |
| USER-01 实际证据 | 尚无导出的参考、随机化盲评或人工修正记录/hash | `missing` |
| TIGER Release PE | AC 最佳性能 overlay、SAC=0、固定 FFmpeg 2/OpenCV 8 线程；T-028 PE 原生 `1+5` 已实测 | `host_readiness_measured` |
| 正式 T-029 P-* | 本轮只跑 160×90 T-028 合成性能 fixture，未跑确认的产品/VFR 60 s 与 180 s 场景 | `not-evaluated` |
| rubric 与 E-*/P-* | A-031 0.5 继承 D-009/D-011 已确认数值、单位、比较符和 AND gate | `confirmed` |

本轮仅把 T-028 Release PE 当作原生执行和固定线程的 Windows 就绪证据。A-031 规定对应分区的
`single_user_reference` 冻结后才能运行正式 classic；当前该 hash 为空，所以没有运行产品主集或 VFR
技术集的正式 classic、没有生成自然度结论，也没有用 T-028 synthetic golden 代替产品或 VFR 性能。

## 2. USER-01 三阶段输入、界面和隔离

仓库工具为 [`prepare_user01_workflow.py`](../../../tests/evaluation/video/prepare_user01_workflow.py)，操作界面和逐步说明见
[`user01-app/README.md`](../../../tests/evaluation/video/user01-app/README.md)。实际媒体、预览、草稿、隐藏答案表和冻结结果只保存在
Git 忽略的 `out/evaluation/T-029/user01/`。只读 loopback 服务只暴露 UI 和该工作区，不暴露仓库其他文件。

### 2.1 `reference_authoring`

- 当前产品主集 40 条全部重新核对媒体 SHA-256 和逐帧 PTS hash；已生成 40 个静音浏览器预览。
- H.264 预览不被当作时间真值。会话同时冻结预览帧和源帧时间序列，要求帧数相同，并按
  presentation-order ordinal 一一映射；界面以实际显示预览帧定位，保存对应源帧真实 `timeNs` 和
  `sourceDecodeOrdinal`。两条预览发生最多 20,449,000 ns 编码时间量化，但没有丢帧或乱序，因此不把
  预览 `currentTime` 冒充源时间。
- 每个事件保存 A-031 3.3 的 kind、子类、节奏作用、前后窗、来源帧、会话、单用户角色和不适用角色；
  新增/修改/删除保留初始值和操作日志。每条 clip 必须显式完成 shot、motion、action、negative 四类检查。
- 当前会话定义已就绪，但 `USER-01` 尚未操作；任何预填或 AI 生成记录都不能成为人工参考。

### 2.2 `blind_rating`

只有参考冻结、对应 classic 运行完成并以同一音色/映射/混音/响度链生成 `classic` 与
`human_reference` 试听版本后，packager 才能创建盲评会话。它将：

1. 要求 render manifest 精确覆盖冻结参考中的全部 final clip；
2. 固定 Windows、显示和音频设备/音量/环境；
3. 使用版本化 seed 随机化 clip 和 A/B 顺序，将 seed 及映射写入独立隐藏答案表，公开会话只含匿名 A/B；
4. 限制 A/B 各完整播放 1～3 次，逐版本记录五维、总体自然度、直接导出状态和 `A|B|tie`；
5. 要求每条 final 恰有 1 组有效结果，N/A 必须有稳定原因，技术故障另建 trial 且保留 invalid 记录。

当前参考和 classic/render 输入不存在，所以 blind session 正确保持 `locked`；随机化工具就绪不等于真实评分。

### 2.3 `manual_correction`

只有对应片段的正式盲评已冻结，才能创建会暴露 classic 来源的修正会话。UI 从冻结 classic 事件开始，
不载入人工参考时间线；记录 `add|delete|move|reclassify|lock` 的 before/after、真实帧时间、原始 UI 操作、
活跃编辑时间、最终事件和 `direct_export|minor_edit|major_edit|unusable`。当前 blind 未执行，因此修正会话保持 `locked`。

三个阶段固定 `acceptanceScope=personal-single-user-acceptance` 和实际操作者 `USER-01`，但使用不同 session ID。
独立复核/裁决写 `not_applicable(reason=single_user_scope)`，评审者间一致性写
`unavailable(reason=single_reviewer_scope)`；结果不得外推为独立多人产品验证。

## 3. “卡点自然”口径

A-031 0.5 保留六个 1～5 整数项：`temporalAlignment`、`salienceAccentMatch`、
`densityAndExtraBeats`、`continuity`、`editReadiness` 和单独填写的 `overallNaturalness`；另记录
`direct_export|minor_edit|major_edit|unusable`、N/A 原因和 A/B/tie。产品主集与真实 VFR 技术集的
每条 final 均须 `USER-01` 恰好 1 组有效评分，不允许缺失、复制或插补。

已确认门槛仍包括：overall 自然度 median `>=4/5`、必测 slice median `>=3.5/5`、1～2 分占比
`<=10%`；五维 overall 各 `>=3.5/5`、slice 各 `>=3/5`；`direct_export|minor_edit >=0.80`、
`unusable <=0.05`；classic 成对 `win+tie >=0.60`；N/A `<=0.05`。这些是通过标准，不是当前测量；
实际评分为 0 条，所以全部自然度指标为 `not-evaluated`。

## 4. TIGER T-028 Release PE 本轮测量

机器可读环境与封装结果见 [`windows-release-baseline-v1.json`](../evidence/T-029/windows-release-baseline-v1.json)，
PE 原始输出见 [`t028-release-windows-measurement-v1.json`](../evidence/T-029/t028-release-windows-measurement-v1.json)。

- 主机：`TIGER` / ASUSTeK `TX Air FA401KM_FA401KM`；Windows 11 `10.0.26200`；Ryzen AI 7 H 350，
  8C/16T，物理内存 33,413,771,264 bytes。
- 运行条件：电池 100%、`BatteryStatus=2`，AC overlay
  `ded574b5-45a0-4f42-8737-46345c09c238`（Best Performance），传统 plan 名为“平衡”；SAC 状态 0。
- 构建/算法：`windows-msvc-x64-release` 原生 Windows PE，SHA-256
  `0fc8b1c56687ca76d184a2fd00e7176495a90dfd631320c9a2bb92b9276a0782`；classic 1.0.0，OpenCV 4.12.0
  线程 8，FFmpeg 8.1.2 解码线程 2，CPU-only。
- 有效序列：1 次预热、5 次正式、5 次取消；此前 4 次 PE 虽完成但证据封装因空 stderr/PowerShell
  单对象处理错误未形成完整 envelope，均显式列为 invalid，不进入下列正式样本。
- 90 帧、160×90、2.967 s fixture 的 5 个分析 wall 样本为
  `201162/186496/175500/174688/176782 us`；P95 `201162 us`，median `176782 us`；analyzed fps
  `447/482/512/515/509`，median `509`。
- 取消延迟 `1467/2004/1582/2557/2189 us`，P95 `2557 us`；内部估算峰值工作内存 359,960 bytes；
  wrapper 轮询峰值 working/private bytes 为 26,431,488 / 21,512,192，峰值线程数 8。
- 原生视频分析单测同轮 8/8 通过。ACPI 温度传感器没有返回可读行，明确记录为 unavailable；CPU 前后
  WMI clock 均为 2000 MHz，但这不能替代温度/降频传感器。

这些数值证明当前主机能按指定次数原生运行 T-028 Release PE，并保留可复核样本；fixture 尺寸、时长和
数据来源不满足 A-031 8.3 的产品/VFR 典型/最大场景，不能据此对任一 T-029 `P-*` 判 pass 或 fail。

## 5. 效果、人工修正和性能门禁状态

A-031 0.5 第 6～7 节确认的 shot/motion/action precision/recall、FP/min、时间误差 P95、渐变 IoU、
人工新增/删除/移动/改类/总修正/活跃编辑时间和所有性能阈值均保持原值。当前判定：

| Gate | 当前结果 | 原因 |
|---|---|---|
| `productRepresentativeGate` | `not-evaluated` | 结构已通过，但 `single_user_reference`、正式 classic、盲评和修正均未执行 |
| `vfrRobustnessGate` | `not-evaluated` | 真实原始 VFR 为 0；不得用 40 条 B 站 CFR 转码流替代 |
| `TIGER T-028 Release readiness` | `measured` | 最佳性能 overlay 下原生 1+5 完成 |
| `T-029 formal P-*` | `not-evaluated` | 确认的 CFR/VFR、60 s/180 s、1080p/4K 场景未运行 |
| `naturalness` / `manualCorrection` | `not-evaluated` | USER-01 实际记录为 0 |
| `overall` | `not-evaluated` | 双数据集 AND gate 和必选 E-*/P-* 尚不完整 |

## 6. 可选模型门禁

`modelEvaluationStarted=false`、`modelProposalCreated=false`、`onnxIntroduced=false`。只有冻结的 final
产品/VFR 集、USER-01 参考/评分/修正和合规 Windows 正式性能全部形成后，classic 1.0.0 真实违反至少
一项已确认门槛，且排除输入质量、单用户标注偏差、媒体时间映射和错误基准环境，才允许提出模型方案。
即使满足也只形成收益、性能、许可、CPU/GPU、包体和部署影响建议；引入 ONNX 仍需新决定。

## 7. 当前阻塞、交接和下一步

1. `H-013 accepted`：继续等待至少 3 条、final 至少 1 条、至少 2 个设备/软件族的真实原始 VFR；
   B 站或其他平台转码流、人工丢/复制帧、改时间戳和 T-028 synthetic 均不得计入。
2. `H-014 open`：Windows Release 子条件已经形成可复核基线；仍等待 USER-01 的实际参考、盲评和修正。
3. USER-01 可先在已准备的本地界面制作 40 条产品主集参考。冻结参考 hash 后，负责人才能运行对应
   classic、生成匿名试听对并提供盲评 URL；对应盲评提交后才提供修正 URL。
4. 真实 VFR 到位后另建技术集 reference/blind/correction 会话和正式性能场景。所有必选证据齐全前，
   T-029 保持 blocked/`not-evaluated`，不提前启动 T-022 或提出模型方案。
