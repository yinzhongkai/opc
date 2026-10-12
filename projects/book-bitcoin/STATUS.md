# 项目状态摘要

- 汇总日期与信息截至点：2026-10-12，本轮按 D-007 完成 v0.0.4 基线之上的项目历史整理及校验，实际同步收尾通过 T-013 Git 历史定位；审校进展沿用 2026-10-11 确认记录。
- 维护人：`project-manager`，见 [PROJECT.md](PROJECT.md)；D-006 的框架升级与基线事实此前由框架超级管理员补录，2026-10-12。
- 当前阶段：出版前审核筹备。
- 任务进展：T-001、T-002、T-012、T-013 均为 completed；D-005、D-007 已 confirmed；T-003 至 T-011 保持 todo，详见 [TASKS.md](TASKS.md)。
- 书稿：task00–task12 共 13 篇位于独立仓库 `bitcoin/`；本地 main 为 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`。本次 13 篇均与 Git HEAD 逐字节一致，摘要与已登记原稿指纹一致，产品工作树干净；原下载目录目前已不存在，来源核对范围见 PROJECT。本次升级未修改正文。
- 审校与验证：全书内容盘点及计划已形成；专业核查、独立审校和实验仅登记后续计划，尚未执行或通过，尚无出版就绪结论。
- 成员与工作区：四名成员已配置登记，详见 [TEAM.yaml](TEAM.yaml)，成员配置和有效技能本轮未变；成员领取下一项工作前按会话协议刷新。工作区仍为 `git`、默认检出 `../book-bitcoin`、分支 main，精确基线见 [WORKSPACE.yaml](WORKSPACE.yaml)。
- 目标仓库：`git@github.com:yinzhongkai/book-bitcoin.git` 位于 `D:\book-bitcoin`；本地 / 远端 main 均为原基线 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`，工作树干净。OPC 本地 / 远端 main 与项目框架基线均为 `v0.0.4` 发布提交 `464ca52aafc6c73ff780a7120acd3bf63c9d31dd`；基线之上的项目历史已按功能从 11 笔归并为 5 笔，远端更新按 T-013 的显式旧头保护执行，实际最终提交由 Git 历史定位。
- 历史与恢复：升级前项目远程头为 `a16c646edafcf8924f3347671c032104524d3baa`，由 `archive/book-bitcoin-pre-v0.0.4-20261012` 保留；T-001 / T-002 旧推送点 `24d0bb6d76e59a9d0bc906fb0fda94befa5a158f` 及获确认草案的旧提交仍可沿恢复标签追溯。A-001 至 A-003 正文、审批事实和业务任务状态未变，详见 [框架升级记录](PROJECT.md)。
- 本次整理恢复点：旧远程头 `7e0f6a185233ad9856f02894d9c9200a35dd2d44` 保存于 `archive/book-bitcoin-pre-cleanup-20261012`；五组结束树逐组相同，追加治理记录前的整体文件树不变；本次只补充 TASKS、DECISIONS、STATUS 三份治理记录。
- 框架验证：本轮历史整理的 60 项回归测试和结构校验通过，450 处本地链接有效；v0.0.4 为项目祖先，基线之上的 5 笔项目提交中 merge commit 为 0。
- 外部资料：tiger 的 `/home/oops/workspace/bitcoin` HEAD 与 v31.0 tag 对应 `6574cb40869b96b9ffc79c19dc8f4e467d60f321`；`bitcoin-tools` 只有四脚本且无 Git 元数据。具体搜索范围、资源缺口与限制见 [A-001](artifacts/book/content-inventory.md)，不作为新工作区配置或运行验证。
- 关键成果：[A-003 审校标准与配套交付边界](artifacts/book/review-criteria.md) 1.0 已 approved，范围仅标准与交付边界；获确认的 0.1 草案保存在提交 07e2bdcc23625d2e7f2499d7bdcc7242a50d995f。[A-001](artifacts/book/content-inventory.md) 0.1 仍为盘点草案，[A-002](artifacts/book/review-plan.md) 0.2 已同步 D-005 和 A-003，书稿与实验尚未批准。
- 待落实事项及下一位行动人：researcher 在 T-003 核查领域主张，writer 在 T-004 整理同仓图 / 脚本和附录完整参考解答的资料清单；用户在 T-006 运行前落实具体执行者与环境，project-manager 组织证据。交付格式、工期与出版渠道尚未确定；其他成员领取工作前按会话协议初始化 / 刷新，当前未触发其会话。

本文件是摘要，原始事实以 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md) 和 [成果索引](artifacts/README.md) 为准。汇总后注明实际信息范围，过期摘要不能覆盖原始记录。
