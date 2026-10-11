# 项目状态摘要

- 汇总日期与信息截至点：2026-10-11，用户确认 T-002 整体建议、A-003 定稿与下游标准输入同步。
- 维护人：`project-manager`，见 [PROJECT.md](PROJECT.md)。
- 当前阶段：出版前审核筹备。
- 任务进展：T-001、T-002 均为 completed；D-005 已 confirmed；T-003 至 T-011 保持 todo，详见 [TASKS.md](TASKS.md)。
- 书稿：task00–task12 共 13 篇位于独立仓库 `bitcoin/`；输入与本地 main 为 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`。本次 13 篇均与 Git HEAD 逐字节一致，产品工作树干净；盘点未修改正文。
- 审校与验证：全书内容盘点及计划已形成；专业核查、独立审校和实验仅登记后续计划，尚未执行或通过，尚无出版就绪结论。
- 成员与工作区：四名成员已配置登记，详见 [TEAM.yaml](TEAM.yaml)；project-manager 本会话已初始化，其余成员会话状态本次未核实。工作区仍为 `git`、默认检出 `../book-bitcoin`、分支 main，精确基线见 [WORKSPACE.yaml](WORKSPACE.yaml)。
- 目标仓库：`git@github.com:yinzhongkai/book-bitcoin.git` 位于 `D:\book-bitcoin`；书稿仍为原基线且工作树干净。OPC 的 T-001 提交 `9ee20db2683c79328a6e110b3028e5cb66e0a480` 已合入并验证；T-002 从该基线整理标准与确认记录，具体提交 / 合入通过任务 ID 与 Git 历史定位。本轮本地整理采用线性方式，未推送远端。
- 外部资料：tiger 的 `/home/oops/workspace/bitcoin` HEAD 与 v31.0 tag 对应 `6574cb40869b96b9ffc79c19dc8f4e467d60f321`；`bitcoin-tools` 只有四脚本且无 Git 元数据。具体搜索范围、资源缺口与限制见 [A-001](artifacts/book/content-inventory.md)，不作为新工作区配置或运行验证。
- 关键成果：[A-003 审校标准与配套交付边界](artifacts/book/review-criteria.md) 1.0 已 approved，范围仅标准与交付边界；获确认的 0.1 草案保存在提交 07e2bdcc23625d2e7f2499d7bdcc7242a50d995f。[A-001](artifacts/book/content-inventory.md) 0.1 仍为盘点草案，[A-002](artifacts/book/review-plan.md) 0.2 已同步 D-005 和 A-003，书稿与实验尚未批准。
- 待落实事项及下一位行动人：researcher 在 T-003 核查领域主张，writer 在 T-004 整理同仓图 / 脚本和附录完整参考解答的资料清单；用户在 T-006 运行前落实具体执行者与环境，project-manager 组织证据。交付格式、工期与出版渠道尚未确定；其他成员领取工作前按会话协议初始化 / 刷新，当前未触发其会话。

本文件是摘要，原始事实以 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md) 和 [成果索引](artifacts/README.md) 为准。汇总后注明实际信息范围，过期摘要不能覆盖原始记录。
