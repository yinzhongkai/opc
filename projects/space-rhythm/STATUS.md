# 项目状态摘要

- 汇总日期与信息截至点：2026-09-15；依据当前 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md)、[成果索引](artifacts/README.md)、A-030～A-035、T-022/T-037/T-038 验证证据，以及用户确认在项目目录下新建独立工程源码目录的最新指令汇总。
- 维护人：[project-manager-01](members/project-manager-01.yaml)。本轮记录 D-016，新增 T-040～T-042 和 H-018，将工程迁移、独立回归及迁移后发布验证分配给对应专业成员；项目经理不代替其执行构建、测试或发布结论。
- 当前阶段：既有一期 MVP 个人试用工程与个人未签名 Windows 交付验证已经完成，现进入工程根迁移阶段。目标是在 `projects/space-rhythm/` 下新建 `workspace/`，把根 CMake/preset/vcpkg、`cmake/src/tests/tooling/docs` 迁入该目录，再从新工程根完成独立测试和个人交付复验；正式产品效果/VFR验收仍按 D-015 取消并延期。
- 任务进展：共 42 项；38 项 `completed`；T-029 为 `cancelled`；T-040、T-041、T-042 为 `todo`；无 `blocked` 或 `in_progress` 任务。
- 关键成果：A-002 0.2 已获 D-001 批准；A-012～A-029 覆盖主要实现、契约与验证；A-030 0.4、A-031 0.5 和 execution-readiness-v4 保留 T-029 已完成准备及未评估边界；A-032 0.1 已完成 Windows 发布计划；A-033 0.2 已记录确定性 `unsigned-engineering` 闭包、SBOM/许可证和事务安装；A-034 0.1 记录 T-022 三套 preset 各 166/166 工程 oracle pass；A-035 0.1 记录当前 `TIGER` 的个人未签名交付链通过。成果索引共 35 项，A-002 与 A-031 为 `approved`，其余为 `draft`。
- 交付范围决定：D-012 已确认仅供用户本人使用的个人未签名工程包；D-013 进一步确认用户已手动关闭当前 `TIGER` 的 SAC 并将其指定为验证主机，只读状态为 `VerifiedAndReputablePolicyState=0`。不公开分发、不交付第三方、不承诺 SAC/WDAC 兼容的边界不变；公共签名证书、签名主体、发布渠道和独立 GUI 安装器仍不是本阶段必选门禁。
- 工程迁移范围：D-016 已确认 `projects/space-rhythm/workspace/` 为新工程根。`.github/workflows/` 因 GitHub 规则留在仓库根但切换工作目录；项目管理资料保留在其上级目录；框架 AGENTS/协议、roles、knowledge、templates、adapters、`scripts/` 留在根。根 `out/` 历史证据和冻结包、未跟踪 `package/`、`scripts/__pycache__/` 均不移动、不删除。
- 交接状态：新增 H-018 `pending`，等待 build-engineer-windows-qt-01 接收 T-040。H-005、H-010 的原执行工作均已完成但仍为 `accepted`，等待 architect-01 核对关闭；H-013/H-014/H-015 已取消，H-016/H-017 已关闭。
- 当前阻塞：T-040 无前置阻塞；T-041 按计划等待 T-040，T-042 按计划等待 T-040/T-041。T-029、H-013/H-014 是经用户确认取消并延期的未评估范围，不是通过项。
- 测试链状态：T-022 已完成，Windows x64 Debug、CI/RelWithDebInfo、Release 三套 preset 均实际 166/166 pass、0 fail、0 skip；该结论仅限工程 oracle。G2 为 `pass(engineering-scope)`；产品效果、自然度、真实 VFR、正式产品性能及其余未确认门槛继续为 `not-evaluated`，不能把项目整体写成质量门禁全绿。
- 发布链状态：T-038 已在当前 `TIGER` 完成个人未签名交付验证，安装、正常 GUI 首启、App/Worker smoke、核心工作流、修复、回滚、卸载和根外用户数据保留均通过。冻结 ZIP SHA-256 为 `2CC3F8B4AC5A6BC4B3EE8CC42C8305094F372A9E16F9DCAF937B59872FF8C3E6`；结论限定为 `pass(personal-unsigned,current-TIGER,SAC-off)`，工程包继续为 `unsigned-engineering`、`candidateEligible=false`。
- 其他风险与待确认：最低 Windows、正式容器/H.264/AAC 后端、产品默认音色和视觉风格仍未确认；当前导出格式仅为 `testOnly`。若未来公开分发、交付第三方或要求 SAC/WDAC 兼容，须新立决定并恢复受信任签名和渠道门禁。
- 下一步：build-engineer-windows-qt-01 在原会话接收 H-018 并执行 T-040；完成后依次由 tester-cpp-qt-01 执行 T-041、release-engineer-windows-01 执行 T-042。architect-01 关闭 H-005/H-010 的管理核对可在不干扰迁移关键路径时串行安排。迁移期间既有 T-038 冻结包仍可供用户个人试用，但不得把旧证据写成新布局验证。

本文件是截至上述信息点的摘要，原始事实仍以任务、决定、交接和成果正文为准；后续变化应更新对应原始记录后再刷新本摘要。
