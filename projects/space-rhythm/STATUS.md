# 项目状态摘要

- 汇总日期与信息截至点：2026-09-15；依据当前 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md)、[成果索引](artifacts/README.md)、T-029 `execution-readiness-v3`、A-032/A-033、T-037 完成证据，以及用户对个人单用户验收范围的明确确认汇总。当前 HEAD 为 `301bfbb`；本轮 D-014/H-017 协调记录尚待提交。
- 维护人：[project-manager-01](members/project-manager-01.yaml)。本轮依据用户明确确认的 D-014 调整产品验收资源和团队计划，不替 product-manager-01 修订 A-031，不替 video-algorithm-engineer-cv-01 执行 T-029 或作出效果结论。
- 当前阶段：一期 MVP 集成收口、产品效果评估与个人未签名 Windows 交付验证。构建、核心、媒体、音频 DSP、Qt Quick UI、Scene Graph 图形、经典视频算法和 unsigned 部署主体已完成；产品效果、端到端质量及干净环境个人交付证据尚未完成。
- 任务进展：共 39 项；36 项 `completed`；T-029 为 `blocked`；T-022、T-038 为 `todo`；无 `in_progress` 任务。
- 关键成果：A-002 0.2 已获 D-001 批准；A-012～A-029 覆盖主要实现、契约与验证；A-030 0.3 保留产品效果门禁；A-031 0.4 已获 D-011 批准并采用产品主集与独立真实 VFR 技术集双 gate，现等待 H-017 按 D-014 修订个人单用户验收条款；A-032 0.1 已完成 Windows 发布计划；A-033 0.2 已记录确定性 `unsigned-engineering` 闭包、SBOM/许可证、事务安装与当前 `TIGER` App/Worker smoke 通过。成果索引共 33 项，A-002 与 A-031 为 `approved`，其余为 `draft`。
- 交付范围决定：D-012 已确认仅供用户本人使用的个人未签名工程包；D-013 进一步确认用户已手动关闭当前 `TIGER` 的 SAC 并将其指定为验证主机，只读状态为 `VerifiedAndReputablePolicyState=0`。不公开分发、不交付第三方、不承诺 SAC/WDAC 兼容的边界不变；公共签名证书、签名主体、发布渠道和独立 GUI 安装器仍不是本阶段必选门禁。
- 交接状态：H-015 已因 D-012 取消，H-016 已关闭。H-010 仍为 `accepted`，T-036/T-037 已完成；H-013 为 `accepted`，等待真实 VFR 原始素材；H-014 为 `open`，已确认 `USER-01` 单用户范围并核实交流电最佳性能覆盖模式，但仍等待实际人工参考/盲评和 T-028 Windows 基准测量；新增 H-017 为 `open`，等待 product-manager-01 修订 A-031。
- 当前阻塞：T-029 仍缺不少于 3 条且 final 不少于 1 条的真实 VFR 原始媒体、`USER-01` 对全部 final 的实际人工参考与随机化盲评记录，以及 D-009 指定的 `TIGER` 一次预热 + 五次正式 Windows 基准测量。D-014 只将五人独立评审改为个人单用户验收，不放宽 VFR、效果或性能门禁；实际证据形成前仍为 `not-evaluated`。
- 发布链状态：T-037 已完成当前 `TIGER` 的个人未签名 App/Worker smoke、确定性归档和安装/修复/回滚/卸载验证。工程包继续为 `unsigned-engineering`、`candidateEligible=false`；T-038 等待 T-022，T-022 等待 T-029。
- 其他风险与待确认：最低 Windows、正式容器/H.264/AAC 后端、产品默认音色和视觉风格仍未确认；当前导出格式仅为 `testOnly`。若未来公开分发、交付第三方或要求 SAC/WDAC 兼容，须新立决定并恢复受信任签名和渠道门禁。
- 下一步：product-manager-01 先接收 H-017，按 D-014 形成 A-031 新版本；同时 H-013 继续取得真实 VFR 原始素材，不再从 B 站标题或平台转码流推断 VFR。输入就绪后由 video-algorithm-engineer-cv-01 准备 `USER-01` 盲评包并完成 T-029 的算法/Windows 基准实测；随后 tester-cpp-qt-01 执行 T-022，最后 release-engineer-windows-01 执行 T-038。

本文件是截至上述信息点的摘要，原始事实仍以任务、决定、交接和成果正文为准；后续变化应更新对应原始记录后再刷新本摘要。
