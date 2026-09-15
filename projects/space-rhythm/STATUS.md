# 项目状态摘要

- 汇总日期与信息截至点：2026-09-15；依据当前 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md)、[成果索引](artifacts/README.md)、A-030 0.4、A-031 0.5、T-029 execution-readiness-v4/Windows Release 基线、A-032/A-033、T-037 完成证据，以及用户明确要求跳过正式评估并改为个人使用反馈汇总。当前 HEAD 为 `6e52edf`；本轮 D-015 和协调状态变更尚待提交。
- 维护人：[project-manager-01](members/project-manager-01.yaml)。本轮依据用户明确授权取消 T-029 当前阶段执行、同步依赖和下一行动人；不把取消、空参考或 synthetic 基线改写为产品效果通过。
- 当前阶段：一期 MVP 个人试用工程收口与个人未签名 Windows 交付验证。构建、核心、媒体、音频 DSP、Qt Quick UI、Scene Graph 图形、经典视频算法、单用户评估工具和 unsigned 部署主体已完成；正式产品效果/VFR验收按 D-015 延期，剩余执行任务为 T-022 和 T-038。
- 任务进展：共 39 项；36 项 `completed`；T-029 为 `cancelled`；T-022、T-038 为 `todo`；无 `blocked` 或 `in_progress` 任务。
- 关键成果：A-002 0.2 已获 D-001 批准；A-012～A-029 覆盖主要实现、契约与验证；A-030 0.4、A-031 0.5 和 execution-readiness-v4 保留 T-029 已完成准备及未评估边界；A-032 0.1 已完成 Windows 发布计划；A-033 0.2 已记录确定性 `unsigned-engineering` 闭包、SBOM/许可证、事务安装与当前 `TIGER` App/Worker smoke 通过。成果索引共 33 项，A-002 与 A-031 为 `approved`，其余为 `draft`。
- 交付范围决定：D-012 已确认仅供用户本人使用的个人未签名工程包；D-013 进一步确认用户已手动关闭当前 `TIGER` 的 SAC 并将其指定为验证主机，只读状态为 `VerifiedAndReputablePolicyState=0`。不公开分发、不交付第三方、不承诺 SAC/WDAC 兼容的边界不变；公共签名证书、签名主体、发布渠道和独立 GUI 安装器仍不是本阶段必选门禁。
- 交接状态：H-013/H-014/H-015 已取消，H-016/H-017 已关闭。H-014 的 Windows主机子条件已经实测，但个人参考/盲评未形成；H-013 的真实 VFR 从未到位。H-010 仍为 `accepted`，其 T-036/T-037 已完成。
- 当前阻塞：没有活动任务阻塞。T-029、H-013/H-014 是经用户确认取消并延期的未评估范围，不是通过项；风险转由个人实际使用反馈承担。T-022 的其他前置均已完成，可立即启动。
- 发布链状态：T-037 已完成当前 `TIGER` 的个人未签名 App/Worker smoke、确定性归档和安装/修复/回滚/卸载验证。工程包继续为 `unsigned-engineering`、`candidateEligible=false`；T-038 只等待 T-022，不再等待 T-029。
- 其他风险与待确认：最低 Windows、正式容器/H.264/AAC 后端、产品默认音色和视觉风格仍未确认；当前导出格式仅为 `testOnly`。若未来公开分发、交付第三方或要求 SAC/WDAC 兼容，须新立决定并恢复受信任签名和渠道门禁。
- 下一步：tester-cpp-qt-01 立即启动 T-022，执行工程端到端、故障恢复和可测质量门禁，并把产品效果、自然度、真实 VFR 与正式产品性能明确记为 `not-evaluated(deferred-to-personal-use-feedback)`。T-022 完成后由 release-engineer-windows-01 执行 T-038；用户后续实际使用中发现的问题另建缺陷任务，不自动恢复 T-029。

本文件是截至上述信息点的摘要，原始事实仍以任务、决定、交接和成果正文为准；后续变化应更新对应原始记录后再刷新本摘要。
