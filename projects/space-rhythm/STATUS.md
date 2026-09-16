# 项目状态摘要

- 汇总日期与信息截至点：2026-09-16；依据当前 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md)、[成果索引](artifacts/README.md)、A-036 0.1、A-037 0.1、T-040/T-041 验证证据及截至提交 `dc6ee8c` 的实际记录汇总。
- 维护人：[project-manager-01](members/project-manager-01.yaml)。本轮已核对 tester-cpp-qt-01 的 T-041 结果，并把工程迁移关键路径推进到 T-042；项目经理不把工程 oracle 通过扩写为产品效果、稳定性或发布批准。
- 当前阶段：既有一期 MVP 个人试用工程与个人未签名 Windows 交付验证已经完成；T-040 已将完整产品工程迁入 `projects/space-rhythm/workspace/`，T-041 已从新工程根完成独立构建/测试回归。当前只剩 T-042 从新工程根重建并复验个人未签名交付包；正式产品效果/VFR 验收仍按 D-015 取消并延期。
- 任务进展：共 42 项；40 项 `completed`；T-029 为 `cancelled`；T-042 为 `todo`；无 `blocked` 或 `in_progress` 任务。
- 关键成果：A-002 0.2 已获 D-001 批准；A-012～A-029 覆盖主要实现、契约与验证；A-030 0.4、A-031 0.5 和 execution-readiness-v4 保留 T-029 已完成准备及未评估边界；A-032 0.1 已完成 Windows 发布计划；A-033 0.2 已记录确定性 `unsigned-engineering` 闭包、SBOM/许可证和事务安装；A-034 0.1 记录迁移前三套 preset 各 166/166 工程 oracle pass；A-035 0.1 记录当前 `TIGER` 的个人未签名交付链通过；A-036 0.1 记录工程根迁移；A-037 0.1 记录迁移后三套 preset 最终各 166/166、0 fail、0 skip 和路径隔离通过，并保留一次未复现 Debug 瞬态失败。成果索引共 37 项，A-002 与 A-031 为 `approved`，其余为 `draft`。
- 交付范围决定：D-012 已确认仅供用户本人使用的个人未签名工程包；D-013 进一步确认用户已手动关闭当前 `TIGER` 的 SAC 并将其指定为验证主机，只读状态为 `VerifiedAndReputablePolicyState=0`。不公开分发、不交付第三方、不承诺 SAC/WDAC 兼容的边界不变；公共签名证书、签名主体、发布渠道和独立 GUI 安装器仍不是本阶段必选门禁。
- 工程迁移结果：T-040 已按 D-016 完成，提交为 `6cfe34d`；根 CMake/preset/vcpkg、`cmake/src/tests/tooling/docs` 和 `.github/workflows/` 已迁入 `projects/space-rhythm/workspace/`，根对应产品入口不存在。三套 preset clean configure/build 通过，生成路径未匹配旧工程根；本轮没有执行 CTest 或重建交付包。仓库根不保留工作流入口，故 GitHub Actions 自动构建、PR 检查和手动 workflow dispatch 仍暂时停用。根 `out/` 历史证据和冻结包、未跟踪 `package/`、`scripts/__pycache__/` 均按决定保留原位。
- 交接状态：H-018 已由 project-manager-01 核对 A-036、T-040 证据、提交和实际路径后关闭；其下游 T-041/T-042 继续由各自负责人执行。H-005、H-010 的原执行工作均已完成但仍为 `accepted`，等待 architect-01 核对关闭；H-013/H-014/H-015 已取消，H-016/H-017 已关闭。
- 当前阻塞：无任务处于 `blocked`。T-042 的 T-040/T-041 前置均已完成，已具备执行条件。T-029、H-013/H-014 是经用户确认取消并延期的未评估范围，不是通过项。
- 测试链状态：T-041 已在新 workspace 的干净 clone 上完成 Windows x64 Debug、CI/RelWithDebInfo、Release 三套 preset 独立回归，最终均为 166/166、0 fail、0 skip，路径隔离和证据哈希核对通过；测试数与 T-022 迁移前基线一致。执行中曾有一次 Debug 取消后重连场景约 113 秒后失败，后续未修改 oracle、测试或产品实现的独立运行通过，该现象保留为未复现瞬态，不能声称从未发生或稳定性问题已被证明消失。结论仅限 `pass(engineering-scope)`；产品效果、自然度、真实 VFR、正式产品性能继续为 `not-evaluated`。
- 发布链状态：T-038 已在当前 `TIGER` 完成个人未签名交付验证，安装、正常 GUI 首启、App/Worker smoke、核心工作流、修复、回滚、卸载和根外用户数据保留均通过。冻结 ZIP SHA-256 为 `2CC3F8B4AC5A6BC4B3EE8CC42C8305094F372A9E16F9DCAF937B59872FF8C3E6`；结论限定为 `pass(personal-unsigned,current-TIGER,SAC-off)`，工程包继续为 `unsigned-engineering`、`candidateEligible=false`。
- 其他风险与待确认：最低 Windows、正式容器/H.264/AAC 后端、产品默认音色和视觉风格仍未确认；当前导出格式仅为 `testOnly`。若未来公开分发、交付第三方或要求 SAC/WDAC 兼容，须新立决定并恢复受信任签名和渠道门禁。
- 下一步：release-engineer-windows-01 在原会话执行 T-042，从 `projects/space-rhythm/workspace/` 重建确定性 `unsigned-engineering` 包，并复验连续归档一致性、安装/repair/rollback/uninstall、用户数据保留、正常 GUI 首启、App/Worker smoke、核心工作流及路径隔离。architect-01 关闭 H-005/H-010 的管理核对可在不干扰该关键路径时串行安排。既有 T-038 冻结包仍可供用户个人试用，但不得把迁移前证据写成新布局验证。

本文件是截至上述信息点的摘要，原始事实仍以任务、决定、交接和成果正文为准；后续变化应更新对应原始记录后再刷新本摘要。
