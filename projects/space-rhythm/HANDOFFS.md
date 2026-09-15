# 行动请求与交接

## H-017：按 D-014 修订 T-029 个人单用户验收协议
- 发起人：project-manager-01
- 目标：product-manager-01
- 关联任务：T-039、T-029
- 期望结果：将 D-014 准确写入 A-031 新版本：评审者固定为 `USER-01`，产品主集和真实 VFR 技术集的每条 final 均须 1 份有效评分且不允许缺失；人工参考制作与随机化盲评分开执行；取消五人构成、三名有经验、每条三份评分及独立复核/裁决前置；将结果和不适用统计明确标记为个人单用户范围。保持 D-011 的真实 VFR 配额/probe、双数据集 AND gate、既有评分尺度、E-*/P-* 数值和 Windows 基准要求不变。
- 输入与证据：D-014 confirmed；[A-031 0.4](artifacts/A-031-t029-video-product-evaluation-input.md)；D-009～D-013；H-013、H-014。
- 未完成事项：H-017 范围内无未完成事项。A-030 和 T-029 执行证据仍须由 video-algorithm-engineer-cv-01 基于 A-031 0.5 更新，`USER-01` 尚未实际制作人工参考或评分，真实 VFR 与 Windows 性能前置仍未完成；这些属于 T-029、H-013/H-014 的下游执行，不阻止本交接关闭。
- 状态：closed
- 创建日期：2026-09-15
- 接收反馈：product-manager-01 于 2026-09-15 刷新 D-014、H-017、H-013/H-014、T-029/T-039 和 A-031 0.4 后接收；处理范围仅为产品验收协议，不执行 T-029，不代写 `USER-01` 的实际记录。
- 处理结果与证据：已形成并批准 [A-031 0.5](artifacts/A-031-t029-video-product-evaluation-input.md)，固定 `acceptanceScope=personal-single-user-acceptance`、`reviewerAnonymousId=USER-01`、参考制作/盲评/人工修正会话隔离、主集与 VFR 技术集逐条 final 各 1 组且不得缺失的评分规则，以及单评审者统计的不可用原因。第 6～7 节 E-*/P-* 数值、第 8.2 节真实 VFR `>=3/final>=1` 与逐帧 PTS/双 gate、第 8.3 节 `TIGER` Windows 性能条件均未修改。当前实际评分、真实 VFR 和正式 Windows 测量仍为 `not-evaluated`。
- 关闭或取消依据：project-manager-01 于 2026-09-15 复核提交 `c35f38c` 和 A-031 0.5：`USER-01`、逐条 final 一组且不允许缺失、人工参考/盲评/修正会话隔离、单用户不可外推标记及不可用统计均已落实；真实 VFR `>=3/final>=1`、逐帧 PTS/双 gate、E-*/P-* 数值和 `TIGER` Windows 基准条件保持不变，满足 H-017 期望结果，故由原发起人关闭。

## H-016：提供个人未签名 Windows 交付验证环境
- 发起人：project-manager-01
- 目标：用户
- 关联任务：T-037、T-038；可供 T-021/T-022 的未签名 Release 复验使用
- 期望结果：确认一台用户自有的干净 Windows x64 电脑或虚拟机，允许正常运行本项目未签名 Win32 EXE/DLL，并可由 release-engineer-windows-01 执行应用/Worker 首次启动、安装/修复/回滚/卸载和核心工作流验证；提供 Windows edition/build、物理机或虚拟机类型、是否启用会阻断未签名程序的 SAC/WDAC，以及可用复验窗口。
- 输入与证据：D-012/D-013 confirmed；[A-033 0.2](artifacts/A-033-windows-unsigned-deployment-and-transaction-pipeline.md)；[T-037 个人未签名验证摘要](evidence/T-037/verification-summary.md)。当前 `TIGER` 曾由 Code Integrity 拒绝部分未签名 App/Qt PE；用户现已自行关闭 SAC 并指定继续使用该主机。
- 未完成事项：环境提供事项和 T-037 App/Worker、事务安装复验均已完成；T-038 核心工作流证据仍待 T-022 前置完成后由 release-engineer-windows-01 执行，不属于本交接关闭结论。
- 状态：closed
- 创建日期：2026-09-15
- 接收反馈：用户于 2026-09-15 明确不接受虚拟机，随后要求直接关闭当前 `TIGER` 的 SAC，并在手动操作完成后回复“已关闭”。
- 处理结果与证据：project-manager-01 于用户反馈后只读查询 `HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy`，确认 `VerifiedAndReputablePolicyState=0`。D-013 已将当前 `TIGER` 指定为个人未签名验证环境；环境提供阶段未执行应用或修改其他安全设置。此后 release-engineer-windows-01 已在 T-037 对最新受控包完成 App/Worker smoke 与完整安装事务，结果见 A-033 0.2；成功条件是当前 SAC=0，不构成 SAC/WDAC 兼容证据。
- 关闭或取消依据：用户已提供本交接要求的自有 Windows x64 验证环境和立即可用复验窗口，且只读状态证明 SAC 已关闭，故由发起人 project-manager-01 关闭。测试通过与否仍由 T-037/T-038 记录。

## H-015：为自建 Qt 与 Release 闭包提供最小 WDAC/SAC 信任路线
- 发起人：release-engineer-windows-01
- 目标：用户（请协调有权主机策略管理员）
- 关联任务：T-036、T-037、T-038；同时影响 T-021/T-022 严格 Release 门禁
- 期望结果：管理员核对 `VerifiedAndReputableDesktop` 策略 GUID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`，并二选一提供可持续的最小开发/CI 信任路线：A（首选）组织管理的非生产签名服务 + 最小 signer/publisher 规则；B 为 ACL 受控、普通构建账号不可改写的专用输出根 + 仅对该根的补充策略。变更后提供策略标识、生效时间和严格 Release 复验窗口。
- 输入与证据：[A-032 0.1 第 8 节](artifacts/A-032-windows-release-input-license-sbom-plan.md)、[T-036 可复核摘要](evidence/T-036/verification-summary.md)、[T021-ENV-001 旧诊断](evidence/T-013/t021-env-001-verification-summary.md)。当前最新证据明确指向 `Qt6QmlMeta.dll` SHA-256 `F35EF425...681A665C`，Code Integrity 状态 `0xC0E90002`。
- 未完成事项：不接受关闭 SAC/WDAC、全局允许用户 profile/Desktop/仓库根、对普通账号可写目录建路径规则、对每次重链接产物建易变 hash 白名单，或把 `-AllowWdacFallback`/反复重试当作验收。本成员未被授权更改系统策略或使用签名凭据。
- 状态：cancelled
- 创建日期：2026-09-14
- 接收反馈：用户于 2026-09-15 明确选择“个人未签名范围”，不再要求当前阶段兼容 `TIGER` 的 SAC/WDAC，也不采购公共代码签名证书。
- 处理结果与证据：2026-09-15，历史 SAC/WDAC 开启状态下的连续受控组包与诊断曾先出现 App/Worker 均退出 0，后续又由 Code Integrity 以 `0xC0E90002` 阻断 App 或未签名 `Qt6QuickDialogs2.dll`；这些证据保留为策略开启期间的历史事实。D-012 随后将目标改为仅供用户本人使用的 `unsigned-engineering` 工程包并明确不承诺 SAC/WDAC 兼容，H-016 则按 D-013 提供 SAC 已关闭的当前 `TIGER`。在新范围下，最新受控包已完成 App/Worker smoke 与完整事务，见 [A-033 0.2 第 6 节](artifacts/A-033-windows-unsigned-deployment-and-transaction-pipeline.md)和[T-037 摘要](evidence/T-037/verification-summary.md)；该 SAC=0 结果不证明 H-015 原请求的信任路线或策略兼容性已经实现。
- 关闭或取消依据：用户作为本交接目标和项目最终确认人于 2026-09-15 明确选择个人未签名范围；D-012 confirmed 后，本交接所求的策略管理员信任路线不再属于本阶段前置，故按用户范围变更取消。历史 Code Integrity 证据保留，不把取消解释为当前 `TIGER` 已能运行未签名包。

## H-014：提供 T-029 个人单用户验收证据与合规 Windows 基准运行条件
- 发起人：video-algorithm-engineer-cv-01
- 目标：用户
- 关联任务：T-029
- 期望结果：按 D-014 由匿名验收者 `USER-01` 为产品主集与真实 VFR 技术集的每条 `final_evaluation` 视频提供 1 份有效评分，人工参考制作与随机化盲评分开执行并保存原始记录/hash，结果只标记为 `personal-single-user-acceptance`。同时使确认的 `TIGER` 基准机在接通电源、Windows 最佳性能模式下允许 T-028 的同一 Release PE 原生启动，以便按一次预热 + 五次正式测量完成门禁。
- 输入与证据：D-014 confirmed；[A-031 0.5](artifacts/A-031-t029-video-product-evaluation-input.md) approved、H-017 closed；[A-030 0.4](artifacts/A-030-video-product-evaluation-and-model-gate.md)；[T-029 最新执行就绪证据](evidence/T-029/execution-readiness-v4.json)；[TIGER 原生 Release 基线](evidence/T-029/windows-release-baseline-v1.json)。
- 未完成事项：Windows 子条件已完成；`USER-01` 的实际人工参考、随机化盲评和人工修正未形成，真实 VFR 原始素材也未提供。D-015 已将这些正式产品验收输入从当前个人试用阶段取消，而非判为完成或通过。
- 状态：cancelled
- 创建日期：2026-09-14
- 接收反馈：用户于 2026-09-15 明确要求在当前 `TIGER` 测试并手动关闭 SAC；同日明确本人作为唯一评审者，并在获知单人范围和外推限制后确认采用“个人单用户验收”方案。project-manager-01 只读核对 `VerifiedAndReputablePolicyState=0` 及交流电最佳性能覆盖模式。
- 处理结果与证据：D-014/A-031 0.5 已把五人独立评审前置改为个人单用户验收。video-algorithm-engineer-cv-01 已准备 40 条逐帧真实 PTS 参考会话、40 个本地静音预览、参考/盲评/修正三阶段 UI、隐藏随机化答案表及冻结校验流程。用户首次导出的 40/40 submission 的 40 条 `events` 均为空且 `modificationLog=0`，project-manager-01 未将其冻结为有效参考。2026-09-15 的有效 TIGER Release 序列为 1 warmup + 5 measured，分析 wall 样本 `201162/186496/175500/174688/176782 us`、取消 P95 `2557 us`，原生视频分析单测 8/8 通过；完整环境、PE/source hash 和无效封装尝试见 windows-release-baseline-v1。
- 关闭或取消依据：用户于 2026-09-15 明确要求跳过该步骤、改为个人实际使用中发现问题后反馈；D-015 confirmed 后，本交接所求的实际人工参考/盲评/修正不再属于本阶段前置，故取消。已有 Windows 基线和工具保留，但不构成产品效果通过。

## H-013：替换 T-029 失效来源并恢复实际样本配额
- 发起人：video-algorithm-engineer-cv-01
- 目标：product-manager-01
- 关联任务：T-029、T-039
- 期望结果：D-010 的 4 个失效来源替换须实际恢复 40 条、`calibration/tuning/final=4/16/20` 和每类 10 条；按 D-011，VFR 收口改由 40 条 B 站主集之外的独立真实原始素材技术集承担，仍须实际 `vfr>=3/final>=1`，以原始源流逐帧 PTS 和代理保持证据准入。不得依据标题猜测 VFR、静默缩短时间窗、人工制造 VFR 或由算法负责人自行换源。
- 输入与证据：[A-031 0.4 approved](artifacts/A-031-t029-video-product-evaluation-input.md)、D-011 confirmed、[dataset 0.2.0 manifest](evidence/T-029/product-dataset-manifest-v2.json)、[execution-readiness-v3](evidence/T-029/execution-readiness-v3.json)。历史输入为 A-031 0.2（提交 `68a8c0e`）、来源可用性审计、dataset manifest v1 和 execution-readiness-v2；4 个失效来源已由 D-010/A-031 0.3 完成替换。
- 未完成事项：D-010 的 4 个替换来源已全部获取，结构配额已恢复；但完整 B 站主集为 VFR 0，至少 3 条、其中 final 至少 1 条的真实 VFR 原始媒体从未提供，`vfrRobustnessGate=not-evaluated`。D-015 已将正式 VFR 产品验收从当前个人试用阶段取消，而非降低或判定该门禁通过。
- 状态：cancelled
- 创建日期：2026-09-14
- 接收反馈：product-manager-01 于 2026-09-14 刷新 T-029、D-009、A-031 0.2、A-030 0.2 及三份媒体审计证据后接收。确认 4 个失效来源必须同类别/同分区替换；36 个已成功来源可保留。实际 VFR 不能由标题或预期标签代替，仍须由 T-029 执行人 probe。
- 处理结果与证据：A-031 0.3 的 D-010 替换已完成实测：4 个来源均获取成功，40/40 SHA、结构分区和类别配额通过，4/4 PTS 保持检查通过；但 `GAME-004`、`TRAVEL-001`、`TRAVEL-007` 的实际源流/代理均为 CFR。用户于 2026-09-14 回复“采用”，确认 D-011 并批准 [A-031 0.4](artifacts/A-031-t029-video-product-evaluation-input.md)：保留 40 条 B 站来源为产品主集，另建至少 3 条真实 VFR 原始素材技术集（至少 1 条 final），原始源流逐帧 PTS probe 准入，两套 gate 以 AND 进入 overall。2026-09-15 用户又要求寻找影流之主等 B 站卡点视频；这些页面可作为内容参考，但现有 B 站产品主集已经 40/40 完整，平台转码流不得计入 D-011 的真实 VFR 技术集。方案已生效，但实际 VFR 原始素材仍未冻结，H-013 不关闭。
- 关闭或取消依据：用户于 2026-09-15 明确要求跳过本阶段正式评估并改在个人实际使用中反馈问题；D-015 confirmed 后，真实 VFR 数据收集不再是当前个人试用交付前置，故取消本交接。未来重开正式 T-029 时仍须恢复 D-011 原数量和 probe 要求，不能把本次取消视为通过。

## H-012：确认 T-029 的真实产品输入与数值门槛
- 发起人：product-manager-01
- 目标：用户
- 关联任务：T-039、T-029
- 期望结果：依据 A-031 0.1 第 8 节提供或确认代表产品视频、样本配额与分区、基准 Windows 硬件元组，以及逐项效果/性能/自然度数值阈值；授权来源必须可辨认。确认后允许 video-algorithm-engineer-cv-01 冻结实际输入 hash 并继续 T-029。
- 输入与证据：[A-031 0.2：T-029 视频产品评估输入](artifacts/A-031-t029-video-product-evaluation-input.md)，状态 approved；[A-030 0.1](artifacts/A-030-video-product-evaluation-and-model-gate.md)；D-009 confirmed。
- 未完成事项：本交接的产品确认已完成。实际媒体获取、probe/hash冻结、人工标注、评审和效果/性能运行属于 T-029 执行，本轮按用户指令不由 product-manager-01 代为执行。T-028 合成 golden 和 Wine 测量仍不得替代产品数据或 Windows 基准结果。
- 状态：closed
- 创建日期：2026-09-14
- 接收反馈：用户于 2026-09-14 明确确认 D-009，并与 product-manager-01 逐项确认 C-1～C-3 的来源、配额、硬件、运行条件、评审和数值门槛。
- 处理结果与证据：D-009 已为 confirmed；A-031 已升为 0.2/approved，写入四类 40 个 B 站真实来源与片段时间窗、`4/16/20` 分区、本机 Windows 基准元组、720p 代理/线程/预热条件和已确认 `E-*`/`P-*` gate。
- 关闭或取消依据：发起人 product-manager-01 已核对用户逐项确认与 A-031 0.2，原期望结果全部满足，故关闭 H-012。

## H-011：补齐媒体 PCM resampler timing provenance 公共字段
- 发起人：audio-dsp-engineer-01
- 目标：multimedia-engineer-ffmpeg-01
- 关联任务：T-031、T-018
- 期望结果：由媒体所有者在后续获授权任务中为每个 PCM segment/buffer 公开字段完整且可验证的 resampler trace，使 DSP 无需读取 PTS、帧数或日志即可判断采样时间连续性；不得由 DSP 修改 A-015 或代填媒体事实。
- 输入与证据：原请求依据 [A-015 0.2](artifacts/A-015-ffmpeg-media-pipeline.md)、A-018 0.1、A-019 0.1 与 T-031 `resample_timing_unavailable` fail-closed 测试；消费方验收依据提交 `156b19f`、[A-014 0.4](artifacts/A-014-media-time-buffer-and-golden-contract.md)、[A-015 0.3](artifacts/A-015-ffmpeg-media-pipeline.md)、[A-018 0.2](artifacts/A-018-audio-dsp-pcm-feature-candidate-contract.md)、[A-019 0.2](artifacts/A-019-audio-analysis-implementation-and-oracles.md)及[T-031 验证摘要](evidence/T-031/verification-summary.md)。
- 未完成事项：请求字段为 `channelOrder`、`segmentOriginTimeNs`、`segmentOriginSampleIndex`，以及 `resampleTrace.{performed,inputSampleRate,outputSampleRate,implementationId,implementationVersion,parametersDigestSha256,delayBeforeInputFramesNumerator,delayBeforeInputFramesDenominator,delayUnit,delayAccountedInFirstSampleIndex,emittedFromDrain}`；还需说明 trace 在 seek、format change、flush/drain 和新 segment 时的生成/变化规则。字段应来自实际 FFmpeg/swresample 状态与版本化配置，不接受由 PTS、输出帧数或日志推断。
- 状态：closed
- 创建日期：2026-09-10
- 接收反馈：multimedia-engineer-ffmpeg-01 于 2026-09-10 依据用户明确指令接收；按公共 DTO 必填语义变化提升媒体 schema/API 版本，在实际 FFmpeg/swresample 状态上补齐 timing provenance，并仅执行媒体专项验证，不启动 T-019、不修改 DSP、`package/` 或缓存目录。
- 处理结果与证据：2026-09-10，媒体层已按 [A-014 0.4](artifacts/A-014-media-time-buffer-and-golden-contract.md) 与 [A-015 0.3](artifacts/A-015-ffmpeg-media-pipeline.md) 提升至 `mediaContractVersion=1.0.0/schemaVersion=2`，并实际填充全部请求字段。运行时版本、版本化配置 SHA-256、`AVFrame`/输出 `AVChannelLayout` 和每次转换前 `swr_get_delay` 是唯一来源；identity、44.1→48、48→44.1、drain、50 ms seek、动态格式和取消测试均通过。非零 delay 缓冲仍保持连续 `firstSampleIndex` 且标志为已记账，消费者不得二次补偿。Debug、CI、Release 最终媒体专项均由最新二进制 27/27 通过，未使用回退。详见 [媒体方 H-011 验证摘要](evidence/T-018/H-011-resampler-provenance.md)和实际 [13 项黄金媒体/ffprobe 证据](../../tests/golden/media/generated/actual-hashes-and-probe-v1.json)。2026-09-11，发起人 audio-dsp-engineer-01 验收提交 `156b19f`：T-031 adapter 改为直接消费 schema 2 字段，真实媒体 identity/双向变采样/非零 delay/drain/seek/format-change 及负向回归在 Debug、CI、Release 各 22/22，CI 媒体专项另 27/27。DSP 未修改媒体实现，也未二次补偿 delay。
- 关闭或取消依据：用户于 2026-09-11 明确要求由 H-011 发起人验收并在通过后关闭。发起人核对 `156b19f` 的字段、生命周期、连续性和媒体方 27/27 证据，并以消费方三配置 22/22 实际测试确认期望结果；故将此前错误的非协议状态 `completed` 更正为 `closed`。Debug 前两次 WDAC 瞬态已保留在证据中，最终复跑通过；这不改写 T021-ENV-001 的项目级状态。

## H-010：启动 Windows 发布工作流
- 发起人：architect-01
- 目标：release-engineer-windows-01
- 关联任务：T-036、T-037、T-038
- 期望结果：在自己的项目会话中接收本交接，先把 T-036 更新为 in_progress，形成发布输入、许可证/SBOM、安装事务、签名隔离和干净环境计划；功能闭环与决定输入就绪后依次执行 T-037/T-038，不把候选验证写成生产发布批准。
- 输入与证据：D-003～D-008；A-004 0.5、A-005 0.4、A-006 0.1 WP-10、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-036～T-038。
- 未完成事项：依据 D-012，签名主体/证书、发布渠道和独立 GUI 安装器已移出本阶段必选范围；最低 Windows 与 H.264/发布格式仍未确认。个人未签名包仍须完成 T-037/T-038 的环境复验，不得公开分发。
- 状态：accepted
- 创建日期：2026-09-09
- 接收反馈：2026-09-14，release-engineer-windows-01 已按会话协议刷新身份、岗位、有效知识、项目当前事实及本交接输入，确认在本人 scope 内接收；已将 T-036 更新为 in_progress，本轮优先处理 Qt DLL、`windeployqt` 和 WDAC/SAC 策略阻塞，不使用签名凭据或放宽生产安全策略。
- 处理结果与证据：2026-09-14，T-036 已完成并形成 [A-032 0.1](artifacts/A-032-windows-release-input-license-sbom-plan.md)及[T-036 可复核摘要](evidence/T-036/verification-summary.md)，Qt DLL 来源/哈希和 `windeployqt` 实际复制已验证，历史 SAC/WDAC 开启状态下的阻断也已如实取证。2026-09-15，D-012/D-013 将后续范围确认为用户本人、自有 Windows 的个人未签名交付，并指定 SAC 已关闭的当前 `TIGER`；H-015 取消、H-016 关闭。T-037 已完成：最新受控包由提交 `02c65ce4b596675d102ed3c82459528b60f63297` 生成，ZIP SHA-256 为 `CD94BC9CABF1B0AD29062EE39DD14DEBCBF2AAEB6B777D69036874221D8C634C`，在 `VerifiedAndReputablePolicyState=0` 条件下通过 App/Worker smoke 及完整安装事务，详见 [A-033 0.2](artifacts/A-033-windows-unsigned-deployment-and-transaction-pipeline.md)与[T-037 验证摘要](evidence/T-037/verification-summary.md)。该结果不是 SAC/WDAC 兼容性证明。H-010 继续由同一成员承接尚待 T-022 的 T-038，故保持 `accepted`。
- 关闭或取消依据：暂无。

## H-009：启动 Qt Scene Graph 实时图形工作流
- 发起人：architect-01
- 目标：graphics-engineer-qt-scenegraph-01
- 关联任务：T-033、T-034、T-035
- 期望结果：在自己的项目会话中接收本交接，先把 T-033 更新为 in_progress，形成 RenderRecipe、线程/资源和离屏接口契约；工程/数据契约就绪后执行 T-034/T-035，并保持产品交互、核心事件和媒体编码责任边界。
- 输入与证据：D-002～D-008；A-004 0.5、A-005 0.4、A-006 0.1 WP-07、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-033～T-035。
- 未完成事项：D-003 已确认 Qt Scene Graph 总体路线；视觉风格、基准 GPU 和像素/性能容差尚未确认，可先做契约与测试向量。
- 状态：closed
- 创建日期：2026-09-09
- 接收反馈：2026-09-09，graphics-engineer-qt-scenegraph-01 已完成会话初始化并接收；按交接要求先启动 T-033，T-034/T-035 继续等待其前置依赖。
- 处理结果与证据：2026-09-11，T-033 已完成：[A-021 0.1](artifacts/A-021-render-recipe-thread-offscreen-contract.md)、[T-033 验证摘要](evidence/T-033/verification-summary.md)、[公共接口](../../src/rendering/include/space_rhythm/rendering/render_contract.hpp)及[18 个契约向量](../../tests/contract/render_public_contract_test.cpp)，冻结 RenderRecipe/不可变快照、线程/GPU generation、QSG 更新、离屏帧/队列及坐标命中契约。同日 T-034 已完成：[A-022 0.1](artifacts/A-022-batched-scene-graph-visual-templates.md)、[T-034 验证摘要](evidence/T-034/verification-summary.md)、[共享几何核心](../../src/rendering/include/space_rhythm/rendering/geometry_core.hpp)、[QQuickItem/QSGGeometryNode 适配](../../src/rendering/include/space_rhythm/rendering/scene_graph_render_item.hpp)、[9 项几何测试](../../tests/unit/render_geometry_test.cpp)及[2 项 QSG 测试](../../tests/unit/render_scene_graph_test.cpp)。已实现高密度事件/波形 LOD、波形/频谱/稳定 seed 脉冲三类 1.0.0 模板、裁剪、动态批量上传、旧快照拒绝和 device generation 重建，最终 Debug T-034 专项 11/11；仅用 Qt 公共 API。T-035 现也完成：[A-025 0.1](artifacts/A-025-qt-offscreen-rendering-and-measurements.md)、[离屏公共接口](../../src/rendering/include/space_rhythm/rendering/offscreen_renderer.hpp)、[测量 JSON](evidence/T-035/measurements-v1.json)和[验证摘要](evidence/T-035/verification-summary.md)。公共 D3D11 GPU/Qt Software render-control 路径复用同一 recipe/snapshot/geometry/time/seed，发布 A-021 帧并覆盖 fallback、背压、取消、极密/多分辨率和 generation 恢复；本机 GPU/software 六组三模板屏上/离屏 max diff=0、diff pixels=0，性能与资源仅标记 measured/not-evaluated，GPU timing/动态显存 unavailable。最终 T-033～T-035 合并回归 31/31 通过，无 Qt 私有 API。H-009 的期望工作流已交付并由发起人完成核对。
- 关闭或取消依据：2026-09-12，发起人 architect-01 核对提交 `f63cb5f`、A-021～A-025 的图形链路、T-035 机器可读测量及公共接口边界；独立复跑 T-035 两个隔离 CTest 进程 2/2 通过，并在当前提交上复跑 T-033～T-035 合并回归 31/31 通过。确认屏上/离屏共用 recipe、不可变 snapshot、几何核心、精确帧时间和 seed，D3D11/Qt Software 降级、帧 lease/背压、取消和 device generation 恢复满足交接预期，且未使用 Qt 私有 API。基准 GPU、像素容差、性能阈值和视觉批准继续保持 `measured/not-evaluated`，不影响本交接按既定范围关闭。

## H-008：启动音频 DSP 工作流
- 发起人：architect-01
- 目标：audio-dsp-engineer-01
- 关联任务：T-030、T-031、T-032
- 期望结果：在自己的项目会话中接收本交接，先把 T-030 更新为 in_progress，形成 PCM、采样时间、特征/候选和测试音色契约；媒体/工程输入就绪后执行 T-031/T-032，输出确定性混音 PCM 供图形和媒体导出消费。
- 输入与证据：D-001～D-008；A-004 0.5、A-005 0.4、A-006 0.1 WP-06、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-030～T-032。
- 未完成事项：T-030～T-032 工作流均已完成。产品默认音色、产品效果/性能门槛、设备矩阵和完整发布门禁仍未确认；A-018 三个 CC0 音色仅作测试，不得替代产品选型。
- 状态：closed
- 创建日期：2026-09-09
- 接收反馈：2026-09-10，audio-dsp-engineer-01 依据用户明确指令接收并依次完成 T-030、T-031；2026-09-11，用户明确启动并完成 T-032 授权。
- 处理结果与证据：T-030 已形成 [A-018 0.2](artifacts/A-018-audio-dsp-pcm-feature-candidate-contract.md)及 10 项可复现向量/3 个 CC0 测试音色；T-031 已形成 [A-019 0.2](artifacts/A-019-audio-analysis-implementation-and-oracles.md)，并完成 H-011 schema 2 验收。T-032 已形成 [A-020 0.1](artifacts/A-020-audio-rendering-and-preview-implementation.md)和[验证摘要](evidence/T-032/verification-summary.md)，实现确定性混音、PCM/WAV 与 QAudioSink 适配；Debug/CI 功能与 golden 及 Release 同源专项有成功记录，但最终 Release 单元复跑被 WDAC/SAC 阻断，未宣称最新专项或完整 Release 门禁通过。性能/效果、产品音色和设备矩阵仍为 measured/not-evaluated。H-008 的期望工作流已交付并由发起人完成核对。
- 关闭或取消依据：2026-09-11，发起人 architect-01 核对提交 `2183c98`、A-020 0.1、T-032 验证摘要及公开接口边界，确认 T-030～T-032 已覆盖 H-008 期望的 PCM/特征契约、确定性分析与确定性混音 PCM。发起人独立复跑 Debug 音频渲染单元程序 12/12 通过，黄金向量生成通过；CI 单元进程仍被既有 T021-ENV-001/WDAC-SAC 在启动前阻止，未改写为功能失败或 Release 门禁通过。产品默认音色、效果/性能门槛和设备矩阵不属于本交接完成声明，继续由后续产品、测试与发布流程确认。据此关闭 H-008。

## H-007：启动 C++/OpenCV 视频算法工作流
- 发起人：architect-01
- 目标：video-algorithm-engineer-cv-01
- 关联任务：T-027、T-028、T-029
- 期望结果：在自己的项目会话中接收本交接，先把 T-027 更新为 in_progress，形成视频候选契约、样本矩阵和指标；媒体/工程输入就绪后执行经典算法与评估，不直接修改核心时间线，不在缺少门禁证据时引入模型。
- 输入与证据：D-001～D-008；A-002 0.2、A-004 0.5、A-005 0.4、A-006 0.1 WP-05、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-027～T-029。
- 未完成事项：D-003 已确认 OpenCV 经典算法路线；代表视频、标注和“卡点自然”阈值尚未确认，未定阈值只报告测量值。
- 状态：accepted
- 创建日期：2026-09-09
- 接收反馈：video-algorithm-engineer-cv-01 于 2026-09-14 完成成员身份、岗位、有效知识、相关决定、任务和上游契约刷新后确认接收；本轮先执行 T-027，完成后才按顺序进入 T-028、T-029，不越过效果与模型门禁。
- 处理结果与证据：2026-09-14，T-027 已按顺序完成并形成 [A-028 0.2](artifacts/A-028-video-analysis-contract-sample-metrics.md)、10 项 CC0 合成样本配方和指标契约。同日用户明确启动并完成 T-028，形成 [A-029 0.1](artifacts/A-029-classic-video-analysis-and-golden.md)及[验证摘要](evidence/T-028/verification-summary.md)：实现 OpenCV 4.12.0 经典 shot/motion_peak/action_peak，直接使用媒体 schema 2 真实 `timeNs`，生成固定 FFmpeg 8.1.2 FFV1 golden 并连续两次取得相同 hash；Debug/CI/Release 三套最终 Windows PE 在 Wine 8.0 隔离环境各 8/8 通过，合成矩阵无 FP/FN。最终 PE 本机原生启动仍受既有 WDAC/SAC `0xC0E90002` 拒绝，未伪装为原生通过。同日用户明确授权启动 T-029；首次缺输入审计形成 A-030 0.1，随后 D-009/A-031 0.2 补齐产品定义。负责人按 A-031 0.2 冻结 36 个可用代理并发现 4 个失效来源；D-010/A-031 0.3 随后批准替换。本轮继续执行后，4 个替换项均成功获取，dataset 0.2.0 达到 40 条、`4/16/20` 和每类 10 条，40/40 媒体 SHA 与 4/4 PTS 保持检查通过；但三个 VFR 目标的源流/代理均为 CFR，故 H-013 不关闭。当前 [A-030 0.3](artifacts/A-030-video-product-evaluation-and-model-gate.md)将结构配额记为 pass、VFR 记为 fail、语义 slice 与产品效果/性能记为 `not-evaluated`；classic 未越过裁决标注前置，未提出模型或引入 ONNX。后续仍由 H-013、H-014 跟踪。
- 关闭或取消依据：暂无。

## H-006：启动 Qt Quick/QML UI 设计开发工作流
- 发起人：architect-01
- 目标：ui-engineer-qt-quick-01
- 关联任务：T-024、T-025、T-026
- 期望结果：在自己的项目会话中接收本交接，先把 T-024 更新为 in_progress，形成完整工作流、交互原型和基础设计系统；工程和核心契约就绪后执行 T-025/T-026，可用 mock service 并行，不把媒体或算法重计算放进 QML。
- 输入与证据：D-001～D-008；A-002 0.2、A-004 0.5、A-005 0.4、A-006 0.1 WP-03、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-024～T-026。
- 未完成事项：H-006 范围内无；T-024/T-025/T-026 均已完成，`T026-DEFECT-001`～`006` 已修复并通过发起人复核。视觉风格、产品文案、产品默认音色、发布导出格式、产品默认视觉模板参数、设备矩阵及可访问性/性能门槛仍由产品或对应责任任务确认，不阻止本开发交接关闭；A-018 CC0 映射和 T-019 `testOnly` 格式仍不得作为产品默认或发布决定。
- 状态：closed
- 创建日期：2026-09-09
- 接收反馈：ui-engineer-qt-quick-01 于 2026-09-09 完成成员会话初始化并确认接收；2026-09-11 用户明确要求继续完成 T-024，并要求 T-025、T-035 不得提前启动；T-024 完成后，用户明确确认 T-025 前置全部满足并授权执行 T-025，同时要求不提前执行 T-026。2026-09-12，用户明确要求执行 T-026，并要求接入现有真实核心、媒体、音频、渲染、T-019 时钟及冻结/安全导出能力；architect-01 登记 `001`～`004` 后，用户明确重开并完成首轮修订；architect-01 随后登记 `005`、`006`，同日用户再次明确重开 T-026 修复两项并补回归，要求 H-006 暂不关闭、修复完成前不启动 T-022。
- 处理结果与证据：2026-09-11，T-024 已完成并形成 [A-023 0.1](artifacts/A-023-qt-quick-ui-information-architecture-and-design-system.md)，覆盖完整信息架构、线框、状态矩阵、交互规则、可访问性、设计系统和 ViewModel/mock 清单。T-025 随后完成并形成 [A-024 0.1](artifacts/A-024-qt-quick-workspace-and-viewmodel-bridge.md)及 [验证摘要](evidence/T-025/verification-summary.md)，交付应用壳层、拆分组件、版本化桥接、mock、SceneGraph 挂载和 headless 测试。2026-09-12，T-026 经两轮明确重开，形成 [A-027 0.3](artifacts/A-027-qt-quick-real-workflow-integration.md)及更新后的 [验证摘要](evidence/T-026/verification-summary.md)：除 0.2 已完成的 pending 手势、真实 worker、事件音轨和非阻塞保存/导出外，0.3 进一步隔离作业与试听取消域，并把项目加载迁移到带 generation 防迟到保护的后台 IO，支持打开失败重试。负责人报告 Debug 与 CI/RelWithDebInfo 真实集成 11/11、UI/headless 5/5 和 ProjectStore/取消关联选择集 9/9 通过；architect-01 随后对提交 `6e3f37d` 完成代码检查，并在排除已登记 WDAC/SAC 媒体生成 fixture 后独立复跑 Debug、CI 的 `qt.ui_real_workflow`，两种配置各 1/1 通过。T-022 未启动；H-006 已关闭。
- 关闭或取消依据：已关闭。2026-09-12，architect-01 先后对提交 `f792e64`、`8bdfdbb` 执行架构复核并登记 `T026-DEFECT-001`～`006`；负责人经用户两次明确重开后逐项完成 pending 手势事务、外部 worker IPC、事件音轨、后台保存/分块导出、操作级取消隔离和后台项目加载修复。最终提交 `6e3f37d` 的代码检查确认：worker 作业与试听使用独立取消源和试听操作序号，取消→重连后的试听不再继承 cancelled 状态；`ProjectStore::load()`、可写性检查及测试延迟均在后台执行，GUI 只应用当前 workspace generation 的结果，失败重试仍走异步路径。architect-01 在排除已登记 WDAC/SAC 媒体生成 fixture 后独立复跑 Debug 与 CI/RelWithDebInfo 的 `qt.ui_real_workflow`，两种配置各 1/1 通过，测试程序覆盖 11 个用例，其中包含取消→重连→试听和约 8 MiB 项目 600 ms 延迟下的 GUI heartbeat、损坏文件失败及替换后重试。T-024、T-025、T-026 均达到交接范围，故关闭 H-006；既有产品待确认项继续由相应决定或任务处理，不作为静默默认值。

## H-005：启动 C++/Qt 测试工作流
- 发起人：architect-01
- 目标：tester-cpp-qt-01
- 关联任务：T-020、T-021、T-022
- 期望结果：在自己的项目会话中接收本交接，先把 T-020 更新为 in_progress 并完成测试策略、需求追踪和可复现规则；T-013 测试骨架、T-014/T-017 契约可用且测试框架获确认后执行 T-021；基础实现集成后执行 T-022。每次只按实际状态更新任务，不把计划或等待依赖写成已经完成。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)；D-003 confirmed；T-020～T-022。
- 未完成事项：D-003 已确认 GoogleTest/CTest + Qt Test/Qt Quick Test；基准硬件、代表素材、性能和产品效果阈值尚未确认，未确认阈值只能报告测量值，不能给出通过结论。
- 状态：accepted
- 创建日期：2026-09-08
- 接收反馈：tester-cpp-qt-01 于 2026-09-10 实际读取本交接、T-020～T-022、D-001～D-008、T-013～T-018 及关联成果后接收并完成 T-020；同日用户再次确认 T-021 前置满足并明确启动 T-021。2026-09-15，用户确认 D-015 已生效、T-029 已取消且不再作为个人试用阶段前置，并明确启动 T-022；接收方据此执行工程可测门禁，同时保留产品反馈延期边界。
- 处理结果与证据：T-020 已完成并形成 [A-016 0.1：C++/Qt 测试策略、需求追踪与可复现规则](artifacts/A-016-cpp-qt-test-strategy-and-traceability.md)。T-021 已完成测试基础设施交付并形成 [A-017 0.2：Windows headless 契约测试入口与证据](artifacts/A-017-windows-headless-contract-test-entry-and-evidence.md)及 [T-021 验证摘要](evidence/T-021/verification-summary.md)：首次三 preset 结果保持原始记录；基于 `40b1734` 的独立复测保留既有 oracle，公开 `limited/full/unknown` 映射及 GM-ROT-SAR-001 在 Debug、CI/RelWithDebInfo 均 63/63 pass，`T021-DEFECT-001` 标记为 resolved。T-022 随后完成并形成 [A-034 0.1：工程端到端、故障恢复与质量门禁](artifacts/A-034-t022-engineering-e2e-fault-recovery-quality-gates.md)及 [T-022 验证摘要](evidence/T-022/verification-summary.md)：Windows x64 Debug、CI/RelWithDebInfo、Release 三个 preset 均实际 166/166 pass、0 fail、0 skip，覆盖真实 UI/worker/核心/媒体/存储链、取消与崩溃恢复、错误媒体/缓存/素材、磁盘与并发输出，原始失败和最终证据均保留。所有无门槛性能仅为 `measured`；产品效果、自然度、真实 VFR 与正式产品性能均为 `not-evaluated(deferred-to-personal-use-feedback)`。
- 关闭或取消依据：执行方已完成 T-020～T-022，H-005 仍保持 accepted，等待发起人 architect-01 核对 A-016、A-017、A-034 后关闭。`T021-DEFECT-001` 已由 tester-cpp-qt-01 独立复测解决；`T021-ENV-001` 首次 Release 的 26 项历史记录继续为 blocked，不因本轮 D-015/SAC-off 环境三 preset 成功而追溯改写。G0、G1、G3 正式性能结论和 G4 仍受延期产品反馈/未确认门槛限制，不能写成全绿；本会话未启动 T-038。

## H-004：启动 FFmpeg 多媒体工作流
- 发起人：architect-01
- 目标：multimedia-engineer-ffmpeg-01
- 关联任务：T-017、T-018、T-019
- 期望结果：在自己的项目会话中接收本交接，先把 T-017 更新为 in_progress，交付媒体时间/缓冲契约和黄金样例矩阵；T-013 统一 x64 构建骨架就绪后依据已确认的 D-003 和核心时间契约执行 T-018；核心/worker/媒体基础可集成后执行 T-019。遵守核心拥有规范 `timeNs`、媒体拥有 PTS 解释的单一责任边界。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-04](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)、[A-012 0.1](artifacts/A-012-core-domain-contract-0x.md)；D-003/D-006 confirmed；T-017～T-019。
- 未完成事项：FFmpeg 具体版本、H.264 后端、发布容器/编码器矩阵和许可证路径尚未确认；不得用原型选择代替发布决定，也不得使用许可不清样例。
- 状态：closed
- 创建日期：2026-09-08
- 接收反馈：multimedia-engineer-ffmpeg-01 于 2026-09-09 依据用户明确指令接收并完成 T-017；于 2026-09-10 完成 T-018 及后续颜色/PCM provenance 修订；又于 2026-09-12 依据用户确认全部前置满足的明确指令启动并完成 T-019。全过程沿用现有 `vcpkg.json` 的 `media` feature 与固定 baseline，未修改 `package/`。
- 处理结果与证据：T-017、T-018、T-019 均已完成。媒体契约/管线最终修订为 [A-014 0.4](artifacts/A-014-media-time-buffer-and-golden-contract.md)与 [A-015 0.3](artifacts/A-015-ffmpeg-media-pipeline.md)；预览/导出形成 [A-026 0.1](artifacts/A-026-preview-synchronization-and-safe-export.md)及 [T-019 验证摘要](evidence/T-019/verification-summary.md)。T-019 以实际播放 sample count/单调时钟驱动 C++ 播放头，覆盖 VFR、pause/resume/seek、掉帧/漂移；冻结修订和媒体/渲染/音频输入，直接消费 T-035/T-032，使用明确 `testOnly` NUT/rawvideo/PCM 的 FFmpeg C API 后端和同目录原子提交。Debug 9/9 实际通过，CI/Release 构建通过但运行受既有 WDAC/Code Integrity 阻断并如实登记。固定 baseline、FFmpeg 8.1.2#3、LGPLv3-or-later、default/GPL/nonfree 关闭状态未变；未选择 H.264 或发布容器。
- 关闭或取消依据：2026-09-12，发起人 architect-01 核对提交 `f1eac20`、A-014 0.4、A-015 0.3、A-026 0.1 与 T-019 验证证据，并独立复跑 Debug `t019` 测试 9/9 通过；确认核心拥有规范 `timeNs`、媒体拥有 PTS 解释的边界未漂移，实际播放帧主时钟/VFR 选帧、导出输入冻结、T-035/T-032 直接消费、`testOnly` 编码器和同目录安全文件事务均满足本交接范围。据此关闭 H-004。CI/Release 运行仍受已登记的 WDAC/Code Integrity 环境问题阻断，发布容器/H.264/许可证矩阵继续作为发布决策缺口，不由本次关闭静默确认。

## H-003：启动 C++ 核心与系统工作流
- 发起人：architect-01
- 目标：core-systems-engineer-cpp-01
- 关联任务：T-014、T-015、T-016
- 期望结果：在自己的项目会话中接收本交接，先把 T-014 更新为 in_progress，交付规范时间、事件、修订与事务 0.x 契约；T-013 纯 C++ 构建骨架和契约就绪后执行 T-015；工程骨架和核心实现可用后执行 T-016。保持 domain 无 Qt Quick/FFmpeg 依赖，不私自确认 D-003/D-006。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-02/WP-08](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)、[A-012 0.1](artifacts/A-012-core-domain-contract-0x.md)；D-003 confirmed；T-014～T-016。
- 未完成事项：目标成员范围内的 T-014、T-015、T-016 已全部完成；无实现阻塞。H-003 仍等待发起人 architect-01 核对结果并关闭。
- 状态：accepted
- 创建日期：2026-09-08
- 接收反馈：core-systems-engineer-cpp-01 于 2026-09-08 完成身份、任务、决定和输入版本刷新，确认接收 T-014～T-016；已将 T-014 转为 in_progress，并以 [A-010 0.1](artifacts/A-010-cpp-core-systems-execution-plan.md)登记执行方案。接收范围不包含 QML 页面、媒体解码、CV/DSP 算法或发布策略。
- 处理结果与证据：core-systems-engineer-cpp-01 已完成 T-014 的 [A-012 0.1 核心契约](artifacts/A-012-core-domain-contract-0x.md)、T-015 的纯 C++ 时间线/事务/撤销重做/确定性融合实现，以及 T-016 的纯作业状态机、版本化同用户本地 IPC/mock Worker、项目 schema/迁移/原子保存/恢复、素材重定位和可重建缓存。T-015 证据见 [验证摘要](evidence/T-015/verification-summary.md)；T-016 的故障矩阵及 Debug/Release/CI 三套 Windows x64 CTest 32/32 结果见 [验证摘要](evidence/T-016/verification-summary.md)。实现保持公共 domain/system 接口无 Qt Quick/FFmpeg 依赖，未启动媒体、UI、CV 或 DSP 任务，未修改 `package/`。
- 关闭或取消依据：暂无。

## H-002：启动 Windows/Qt 构建工作流
- 发起人：architect-01
- 目标：build-engineer-windows-qt-01
- 关联任务：T-011、T-012、T-013
- 期望结果：在自己的项目会话中接收本交接，先把 T-011 更新为 in_progress，只读审计当前 Windows 构建环境并提交 D-006 决策输入；用户确认编译器、Qt 版本和许可证路径后执行 T-012；可复现 Qt SDK 就绪且 D-003 构建组合确认后执行 T-013。不得自行安装工具或把候选路线写成已确认决定。
- 输入与证据：D-002～D-008 confirmed；[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-01](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)、[A-009 0.1](artifacts/A-009-windows-qt-6.11.2-source-sdk-build.md)、[A-012 0.1](artifacts/A-012-core-domain-contract-0x.md)；T-011～T-013。
- 未完成事项：T-011/T-012/T-013 已完成；最低 Windows 版本仍将约束后续兼容与发布验证。待发起人 architect-01 核对完整工作流结果并关闭本交接。
- 状态：accepted
- 创建日期：2026-09-08
- 接收反馈：build-engineer-windows-qt-01 于 2026-09-08 已读取 H-002、T-011～T-013、D-002～D-006 及 A-004 0.5、A-005 0.3、A-006 0.1、A-007 0.1，确认在岗位 scope 内接收 Windows/Qt 构建工作流；先执行只读 T-011，不把接收解释为安装、技术定案或发布授权。
- 处理结果与证据：T-011 已完成，见 [A-008 0.3：Windows 构建环境审计与执行方案](artifacts/A-008-windows-build-environment-audit-and-execution-plan.md)。T-012 已完成 MSVC 2022 x64 工具链、Qt 6.11.2 官方源码哈希、shared Release/Debug SDK、ABI/CRT、QML/Multimedia 消费端与部署冒烟验证，见 [A-009 0.1](artifacts/A-009-windows-qt-6.11.2-source-sdk-build.md)及其 [T-012 证据摘要](evidence/T-012/verification-summary.md)。用户于 2026-09-09 确认 D-003 并启动 T-013；现已完成 Windows x64 应用、Worker、核心库、媒体适配和测试的 CMake/Ninja 骨架、固定 vcpkg baseline、GoogleTest/CTest、Qt/QML 冒烟、Windows CI 及三套 preset 验证，见 [A-013 0.2](artifacts/A-013-windows-x64-cmake-ci-skeleton.md)及其 [T-013 证据摘要](evidence/T-013/verification-summary.md)。2026-09-10 又按用户明确指令完成 T021-ENV-001 的构建侧诊断和严格 Release 复核，见 [专项摘要](evidence/T-013/t021-env-001-verification-summary.md)：旧 core 哈希的 WDAC/SAC 拒绝已复现，重建后 core 26/26 实际通过，但 Release 全套仍为 53/63，必须由主机策略管理员提供可持续的最小开发信任路线。本交接覆盖的三项任务均完成，保持 accepted，等待发起人核对关闭。
- 关闭或取消依据：执行方已完成 T-011～T-013；按交接关闭职责等待发起人 architect-01 核对后填写最终关闭依据。

## H-001：协调已确认产品方向的后续计划
- 发起人：product-manager-01
- 目标：project-manager-01
- 关联任务：T-002
- 期望结果：接收已确认的第一阶段产品方向，依据 D-001 和 A-002 0.2 协调 PROJECT/STATUS 摘要同步，并在现有团队职责和用户授权范围内安排后续技术可行性评估；如缺少适合的技术成员或需要新增成员，向用户说明具体缺口，不由本交接自动创建成员或指派实现。
- 输入与证据：[A-002 0.2：音画双向节奏创作工具产品需求](artifacts/A-002-audio-visual-rhythm-product-brief.md)，状态 approved；D-001，状态 confirmed；逆向输入 A-001 0.1 与 A-003 0.1。
- 未完成事项：技术可行性、技术栈、领域拆分和研发编制已由 T-004～T-010、T-023 完成；尚未确认代表产品视频及人工标注、“卡点自然”rubric/评分尺度/播放条件、基准硬件和效果/性能阈值，导致 T-029 blocked。最低 Windows、正式导出格式、安装器、签名和发布渠道仍由发布流程确认；项目工期与预算尚未设定。
- 状态：closed
- 创建日期：2026-09-07
- 接收反馈：project-manager-01 于 2026-09-14 按用户明确要求刷新身份、岗位、有效知识、项目决定、任务、交接及 H-001 输入后确认接收；接收范围是同步已确认产品方向、核对技术可行性和当前团队计划，并整理剩余产品输入，不取得专业结论或最终批准权。
- 处理结果与证据：已依据 D-001 和已批准的 [A-002 0.2](artifacts/A-002-audio-visual-rhythm-product-brief.md)更新 [PROJECT.md](PROJECT.md) 的目标、范围、非目标、阶段和验收边界，并刷新 [STATUS.md](STATUS.md)。技术可行性与需求已由 T-004～T-008/[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、技术栈由 [A-005 0.4](artifacts/A-005-mvp-technology-stack-proposal.md)、领域拆分由 T-009/[A-006 0.1](artifacts/A-006-domain-work-packages.md)、执行计划与完整研发编制由 T-010、T-023/[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)形成；当前 38 项任务中 33 项 completed。剩余关键输入已由 T-029/[A-030 0.1](artifacts/A-030-video-product-evaluation-and-model-gate.md)具体化，下一步需产品侧整理并由用户确认，未将合成样本或 draft 口径当作产品批准。
- 关闭或取消依据：product-manager-01 于 2026-09-14 刷新项目资料后核对：PROJECT/STATUS 已准确同步 D-001 与 A-002 0.2，T-004～T-010、T-023 及 A-004～A-011 已覆盖技术可行性、技术栈、领域拆分和完整研发计划，T-029/A-030 已具体化剩余产品输入且未将合成样本当作产品批准。原期望结果全部满足，故由原发起人关闭 H-001；后续产品输入另由 T-039、D-009 和 H-012 跟踪。

已在任务内明确安排的轻量评审或复核无需重复建立交接；另有补充输入、协调或责任转交请求时再登记。

## 记录样式（不是真实交接）

```text
## H-001：交接主题
- 发起人：<member-id；框架管理员的配置请求注明“框架超级管理员”及授权来源>
- 目标：<一个明确的 member-id 或用户>
- 关联任务：<T-编号；管理员配置请求可写无并说明原因>
- 期望结果：<对方需要完成什么>
- 输入与证据：<成果链接、版本和必要上下文>
- 未完成事项：<缺失信息、约束或需决定的问题>
- 状态：open
- 创建日期：<实际日期>
- 接收反馈：尚未接收
- 处理结果与证据：暂无
- 关闭或取消依据：暂无
```

目标成员实际接收后记录 accepted；提供结果后，由发起人核对并关闭。写入交接不会自动启动其他会话，也不会自动改变任务负责人。

需要超级管理员行动时，目标填写用户，期望结果说明请用户转交框架管理员，不使用虚构的项目管理员成员 ID。管理员仅在配置工作授权内发起交接，不借此安排专业业务任务。
