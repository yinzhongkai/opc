# 项目状态摘要

- 汇总日期与信息截至点：2026-09-15；依据当前 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md)、[成果索引](artifacts/README.md)、T-029 `execution-readiness-v3`、A-032/A-033 及用户对个人未签名范围的确认汇总。最近已提交实现基线为 `bd91d27`，本轮协调记录尚待提交。
- 维护人：[project-manager-01](members/project-manager-01.yaml)。本轮仅依据用户明确确认的 D-012 调整交付范围和团队计划，不替 release-engineer-windows-01 修改 A-032/A-033 的专业结论。
- 当前阶段：一期 MVP 集成收口、产品效果评估与个人未签名 Windows 交付验证。构建、核心、媒体、音频 DSP、Qt Quick UI、Scene Graph 图形、经典视频算法和 unsigned 部署主体已完成；产品效果、端到端质量及干净环境个人交付证据尚未完成。
- 任务进展：共 39 项；35 项 `completed`；T-037 为 `in_progress`；T-029 为 `blocked`；T-022、T-038 为 `todo`。
- 关键成果：A-002 0.2 已获 D-001 批准；A-012～A-029 覆盖主要实现、契约与验证；A-030 0.3 保留产品效果门禁；A-031 0.4 已获 D-011 批准并采用产品主集与独立真实 VFR 技术集双 gate；A-032 0.1 已完成 Windows 发布计划；A-033 0.1 已形成确定性 `unsigned-engineering` 闭包、SBOM/许可证和事务安装流水线。成果索引共 33 项，A-002 与 A-031 为 `approved`，其余为 `draft`。
- 交付范围决定：D-012 已确认仅供用户本人在自有 Windows 电脑上使用的个人未签名工程包，不公开分发、不交付第三方、不承诺 SAC/WDAC 兼容，也不通过关闭主机安全策略制造通过。公共签名证书、签名主体、发布渠道和独立 GUI 安装器不再是本阶段必选门禁。
- 交接状态：H-015 已因 D-012 取消，历史 Code Integrity 证据保留；新增 H-016 等待用户提供允许未签名程序运行的自有干净 Windows x64 电脑或虚拟机。H-010 仍为 `accepted`，已完成 T-036 并推进 T-037；H-013 为 `accepted`，等待真实 VFR 原始素材；H-014 为 `open`，等待真人评审与 D-009 指定的 `TIGER` 基准运行条件。
- 当前阻塞：T-029 仍缺不少于 3 条且 final 不少于 1 条的真实 VFR 原始媒体、5 名独立真人评审者（至少 3 名有相关经验）、裁决/盲评记录，以及 D-009 指定的 `TIGER` Windows 基准运行条件，故效果、性能和可选模型门禁保持 `not-evaluated`。D-012 不放宽这些产品门禁。
- 发布链状态：T-037 的组包、供应链和事务实现已完成，但须由 release-engineer-windows-01 在 H-016 环境复验 App/Worker smoke，并修订 A-033/证据后才能按负责人完成条件收口；当前包继续为 `unsigned-engineering`、`candidateEligible=false`。T-038 等待 T-022/T-037 与相同验证环境。
- 其他风险与待确认：最低 Windows、正式容器/H.264/AAC 后端、产品默认音色和视觉风格仍未确认；当前导出格式仅为 `testOnly`。若未来公开分发、交付第三方或要求 SAC/WDAC 兼容，须新立决定并恢复受信任签名和渠道门禁。
- 下一步：用户先处理 H-016，确认一套允许未签名程序运行的自有 Windows x64 电脑或虚拟机；随后由 release-engineer-windows-01 复验并收口 T-037。产品侧可并行补齐 H-013/H-014 以解除 T-029；T-029 完成后由 tester-cpp-qt-01 执行 T-022，最后由 release-engineer-windows-01 执行 T-038。

本文件是截至上述信息点的摘要，原始事实仍以任务、决定、交接和成果正文为准；后续变化应更新对应原始记录后再刷新本摘要。
