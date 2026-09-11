# 行动请求与交接

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
- 未完成事项：最低 Windows、安装器、H.264/发布格式、签名主体/证书和渠道尚未确认；未授权时不得使用签名凭据。
- 状态：open
- 创建日期：2026-09-09
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-009：启动 Qt Scene Graph 实时图形工作流
- 发起人：architect-01
- 目标：graphics-engineer-qt-scenegraph-01
- 关联任务：T-033、T-034、T-035
- 期望结果：在自己的项目会话中接收本交接，先把 T-033 更新为 in_progress，形成 RenderRecipe、线程/资源和离屏接口契约；工程/数据契约就绪后执行 T-034/T-035，并保持产品交互、核心事件和媒体编码责任边界。
- 输入与证据：D-002～D-008；A-004 0.5、A-005 0.4、A-006 0.1 WP-07、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-033～T-035。
- 未完成事项：D-003 已确认 Qt Scene Graph 总体路线；视觉风格、基准 GPU 和像素/性能容差尚未确认，可先做契约与测试向量。
- 状态：accepted
- 创建日期：2026-09-09
- 接收反馈：2026-09-09，graphics-engineer-qt-scenegraph-01 已完成会话初始化并接收；按交接要求先启动 T-033，T-034/T-035 继续等待其前置依赖。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-008：启动音频 DSP 工作流
- 发起人：architect-01
- 目标：audio-dsp-engineer-01
- 关联任务：T-030、T-031、T-032
- 期望结果：在自己的项目会话中接收本交接，先把 T-030 更新为 in_progress，形成 PCM、采样时间、特征/候选和测试音色契约；媒体/工程输入就绪后执行 T-031/T-032，输出确定性混音 PCM 供图形和媒体导出消费。
- 输入与证据：D-001～D-008；A-004 0.5、A-005 0.4、A-006 0.1 WP-06、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-030～T-032。
- 未完成事项：T-030～T-032 工作流均已完成。产品默认音色、产品效果/性能门槛、设备矩阵和完整发布门禁仍未确认；A-018 三个 CC0 音色仅作测试，不得替代产品选型。
- 状态：accepted
- 创建日期：2026-09-09
- 接收反馈：2026-09-10，audio-dsp-engineer-01 依据用户明确指令接收并依次完成 T-030、T-031；2026-09-11，用户明确启动并完成 T-032 授权。
- 处理结果与证据：T-030 已形成 [A-018 0.2](artifacts/A-018-audio-dsp-pcm-feature-candidate-contract.md)及 10 项可复现向量/3 个 CC0 测试音色；T-031 已形成 [A-019 0.2](artifacts/A-019-audio-analysis-implementation-and-oracles.md)，并完成 H-011 schema 2 验收。T-032 已形成 [A-020 0.1](artifacts/A-020-audio-rendering-and-preview-implementation.md)和[验证摘要](evidence/T-032/verification-summary.md)，实现确定性混音、PCM/WAV 与 QAudioSink 适配；Debug/CI 功能与 golden 及 Release 同源专项有成功记录，但最终 Release 单元复跑被 WDAC/SAC 阻断，未宣称最新专项或完整 Release 门禁通过。性能/效果、产品音色和设备矩阵仍为 measured/not-evaluated。H-008 的期望工作流已交付，状态保持 accepted，等待发起人 architect-01 关闭。
- 关闭或取消依据：暂无。

## H-007：启动 C++/OpenCV 视频算法工作流
- 发起人：architect-01
- 目标：video-algorithm-engineer-cv-01
- 关联任务：T-027、T-028、T-029
- 期望结果：在自己的项目会话中接收本交接，先把 T-027 更新为 in_progress，形成视频候选契约、样本矩阵和指标；媒体/工程输入就绪后执行经典算法与评估，不直接修改核心时间线，不在缺少门禁证据时引入模型。
- 输入与证据：D-001～D-008；A-002 0.2、A-004 0.5、A-005 0.4、A-006 0.1 WP-05、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-027～T-029。
- 未完成事项：D-003 已确认 OpenCV 经典算法路线；代表视频、标注和“卡点自然”阈值尚未确认，未定阈值只报告测量值。
- 状态：open
- 创建日期：2026-09-09
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-006：启动 Qt Quick/QML UI 设计开发工作流
- 发起人：architect-01
- 目标：ui-engineer-qt-quick-01
- 关联任务：T-024、T-025、T-026
- 期望结果：在自己的项目会话中接收本交接，先把 T-024 更新为 in_progress，形成完整工作流、交互原型和基础设计系统；工程和核心契约就绪后执行 T-025/T-026，可用 mock service 并行，不把媒体或算法重计算放进 QML。
- 输入与证据：D-001～D-008；A-002 0.2、A-004 0.5、A-005 0.4、A-006 0.1 WP-03、[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；T-024～T-026。
- 未完成事项：视觉风格、部分产品文案和可访问性门槛仍需产品输入；高密度渲染由 H-009 对应成员负责。
- 状态：accepted
- 创建日期：2026-09-09
- 接收反馈：ui-engineer-qt-quick-01 于 2026-09-09 完成成员会话初始化并确认接收；先执行 T-024，T-025/T-026 保持待办并按既有依赖推进。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-005：启动 C++/Qt 测试工作流
- 发起人：architect-01
- 目标：tester-cpp-qt-01
- 关联任务：T-020、T-021、T-022
- 期望结果：在自己的项目会话中接收本交接，先把 T-020 更新为 in_progress 并完成测试策略、需求追踪和可复现规则；T-013 测试骨架、T-014/T-017 契约可用且测试框架获确认后执行 T-021；基础实现集成后执行 T-022。每次只按实际状态更新任务，不把计划或等待依赖写成已经完成。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)；D-003 confirmed；T-020～T-022。
- 未完成事项：D-003 已确认 GoogleTest/CTest + Qt Test/Qt Quick Test；基准硬件、代表素材、性能和产品效果阈值尚未确认，未确认阈值只能报告测量值，不能给出通过结论。
- 状态：accepted
- 创建日期：2026-09-08
- 接收反馈：tester-cpp-qt-01 于 2026-09-10 实际读取本交接、T-020～T-022、D-001～D-008、T-013～T-018 及关联成果后接收并完成 T-020；同日用户再次确认 T-021 前置满足并明确启动 T-021。T-022 继续保持 todo，未启动。
- 处理结果与证据：T-020 已完成并形成 [A-016 0.1：C++/Qt 测试策略、需求追踪与可复现规则](artifacts/A-016-cpp-qt-test-strategy-and-traceability.md)。T-021 已完成测试基础设施交付并形成 [A-017 0.2：Windows headless 契约测试入口与证据](artifacts/A-017-windows-headless-contract-test-entry-and-evidence.md)及 [T-021 验证摘要](evidence/T-021/verification-summary.md)：首次三 preset 结果保持原始记录；基于 `40b1734` 的独立复测保留既有 oracle，公开 `limited/full/unknown` 映射及 GM-ROT-SAR-001 在 Debug、CI/RelWithDebInfo 均 63/63 pass，`T021-DEFECT-001` 标记为 resolved。黄金审计仍为 12/12、时间向量 30/30；未确认阈值与 G0～G4 保持 `not-evaluated`。
- 关闭或取消依据：尚未关闭；等待发起人 architect-01 核对 T-020/A-016 与 T-021/A-017 后关闭。`T021-DEFECT-001` 已由 tester-cpp-qt-01 独立复测解决；`T021-ENV-001` 继续由 build-engineer-windows-qt-01/主机策略管理员处理，首次 Release 的 26 项仍为 blocked，不能写成全绿；T-022 仍须由用户另行明确启动。

## H-004：启动 FFmpeg 多媒体工作流
- 发起人：architect-01
- 目标：multimedia-engineer-ffmpeg-01
- 关联任务：T-017、T-018、T-019
- 期望结果：在自己的项目会话中接收本交接，先把 T-017 更新为 in_progress，交付媒体时间/缓冲契约和黄金样例矩阵；T-013 统一 x64 构建骨架就绪后依据已确认的 D-003 和核心时间契约执行 T-018；核心/worker/媒体基础可集成后执行 T-019。遵守核心拥有规范 `timeNs`、媒体拥有 PTS 解释的单一责任边界。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-04](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)、[A-012 0.1](artifacts/A-012-core-domain-contract-0x.md)；D-003/D-006 confirmed；T-017～T-019。
- 未完成事项：FFmpeg 具体版本、H.264 后端、发布容器/编码器矩阵和许可证路径尚未确认；不得用原型选择代替发布决定，也不得使用许可不清样例。
- 状态：accepted
- 创建日期：2026-09-08
- 接收反馈：multimedia-engineer-ffmpeg-01 于 2026-09-09 依据用户明确指令接收并完成 T-017；又于 2026-09-10 依据用户明确指令启动 T-018，使用现有 `vcpkg.json` 的 `media` feature 与固定 baseline，不提前执行 T-019，不修改 `package/`。
- 处理结果与证据：T-017 已于 2026-09-09 完成；T-018 已于 2026-09-10 完成。交付 [A-014 0.2 媒体契约](artifacts/A-014-media-time-buffer-and-golden-contract.md)、[A-015 0.1 FFmpeg 管线](artifacts/A-015-ffmpeg-media-pipeline.md)、[12 个合法样例配方与 30 个时间向量](../../tests/golden/media/fixtures-v1.json)、[实际媒体 SHA-256/ffprobe 证据](../../tests/golden/media/generated/actual-hashes-and-probe-v1.json)及 [T-018 可复核摘要](evidence/T-018/verification-summary.md)。固定 baseline 未变，解析 FFmpeg 8.1.2#3/ABI 与 LGPLv3-or-later 配置已登记，default/GPL/nonfree 保持关闭；Debug、Release、CI 各通过 49/49 CTest，媒体专项 16/16。T-019 未启动，`package/` 未修改或暂存。
- 关闭或取消依据：暂无。

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
- 未完成事项：典型素材与格式边界、视觉默认风格、本地/云端边界和“卡点自然”评分口径仍需在后续任务细化；技术可行性与工作量尚未评估。
- 状态：open
- 创建日期：2026-09-07
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

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
