# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与正文链接 | 负责人 | 关联任务 | 版本 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|
| A-001 | [书稿现状评估](A-001-书稿现状评估.md) | writer | T-001 | 0.1 | draft | 尚无 |
| A-002 | [书稿打磨计划](A-002-book-polishing-plan.md) | project-manager | T-002、T-016、T-017 | 0.8 | draft | 范围与验收依据 D-001~D-013（confirmed）；v0.8 依据用户采用增员建议、PROJECT 配置记录及 H-003，在 T-016 内前置 architect 平台设计、专业核对与用户确认，保留 QEMU/Yocto 分工及 `meta-tiger` patch 模型；计划文本待确认 |
| A-003 | [知识点校验清单](A-003-知识点校验清单.md) | writer | T-003 | 0.1 | draft | 尚无 |
| A-004 | [体例约定](A-004-体例约定草案.md) | writer | T-005、T-013 | 0.5 | approved | v0.5 于 2026-09-27 经用户明确批准（“批准 A-004 v0.5”）；新增硬件型号、粗体/反引号边界及等宽文本规则正式生效 |
| A-005 | [tiger 平台设计说明](A-005-tiger-platform-design.md) | architect | T-016；跨章影响供 T-017 使用 | 0.2 | approved | [D-014](../DECISIONS.md)（confirmed）；2026-10-11 用户明确“批准 v0.2”，固定获批提交 `08bdf9c08d6d8cbd8836df3771ffd28231470059`、原 blob `26c5fdca1103f37ac3e752208c429231b8191eee`；QEMU `5287e43`、Yocto `b3fbff3` 均 pass。本次仅元信息回写，技术正文不变；后续候选与未验证边界保留，实现/运行未执行 |
| A-006 | [chapter 4 章节蓝图](A-006-chapter4-blueprint.md) | writer | T-016 章节蓝图阶段；跨章影响供 T-017 使用 | 0.1 | draft | 2026-10-14 用户通读后同意继续工程实施，project-manager 已核对交接，来源见 [T-016](../TASKS.md)；本条记录实施授权，当前 draft 保留。输入为 A-005 v0.2 approved / D-014。九节教学、最少 devtool 到干净重构建/runqemu 主线、QEMU/Yocto 工程资产分工、E-01～E-10 证据及验收映射；工程实现、运行与章节定稿尚未完成 |

## 工作记录与核验证据（非独立成果）

- [目录说明](work-records/README.md)
- [T-006 E-1～E-3 修订脚本](work-records/t006-e123-fix.py)
- [T-006 统一修订脚本](work-records/t006-unified-revision.py)
- [T-008 chapter 1 环境核验记录](work-records/t008-chapter1-env-check.md)
- [T-009 chapter 1 构建与启动核验记录](work-records/t009-chapter1-build-check.md)
- [T-012 chapter 2 核验记录](work-records/t012-chapter2-env-check.md)
- [T-015 chapter 3 核验记录](work-records/t015-chapter3-env-check.md)
- [T-015 原始执行日志](work-records/t015-chapter3-raw.log)
- [T-014 chapter 3 最终 Docker 复验报告](work-records/t014-chapter3-final-revalidation/report.md)（2026-10-03 执行、2026-10-04 补证，2026-10-08 从源码历史提交 `f5d5fffd972845bb52e6f2a2e9ef36986d3667e1` 补录到 OPC；同目录保存两份原始日志与三份执行脚本）

## T-016 QEMU 工程阶段交付（qemu-engineer）

- [模型、固件与作者模型验证报告](work-records/t016-qemu-model-validation.md)及[原始证据目录](work-records/t016-qemu-model-raw/README.md)：2026-10-15 本阶段最终矩阵 16/16 通过，含冷启动、同进程两次宿主 reset、ROM/参数/BIOS 边界；保留失败和修复。正式 metadata/清理后干净构建/runqemu 系统验收待 Yocto，章节和 T-016 整体未完成。
- 工程仓库：ssh tiger:/home/oops/workspace/meta-tiger，git@github.com:yinzhongkai/meta-tiger.git；feature/t-016-chapter4-revision。模块提交 0e73cd7faacff219fa5de0a7365743ce31402d73 为 recipes-devtools/qemu/files/0001-arm-add-minimal-tiger-machine.patch；阶段交接头 b303ae6d614f067e11d4cbba57feacaf84ee7e8e 同时含 recipes-test/tiger-minboot/files/ 的 start.S/main.c/tiger-minboot.ld/COPYING.MIT 和 tests/model/ 的构建/测试工具及交接说明。来源/应用基点/顺序/hash与覆盖见报告。
- 状态：阶段开发提交已保存、模型作者自测通过；尚未推送或合入 master，不作 WORKSPACE 最终 revision。未删除任务分支或清理 devtool source；2026-10-18 project-manager 工程交接核对通过，下一阶段 Yocto 正式集成已安排待接收。

## T-016 源码目录维护（project-manager）

- [QEMU 源码迁移与长期保存记录](work-records/t016-qemu-source-retention-20261017.md)及[原始回执](work-records/t016-qemu-source-retention-20261017.json)：2026-10-17 按用户请求迁入 `tiger:/home/oops/workspace/sources/qemu-system-native`，保留 `devtool@7ed31149ac01a9d6bb3a682b9d768936c4315ed3` 及全部原 refs，增加恢复标签/bundle；devtool 与 BitBake 新路径解析通过。长期保存与后续可读提交约定见 [D-015](../DECISIONS.md)，正式交付继续位于 meta-tiger。

## T-016 工程交接核对（project-manager）

- [结构化核对回执](work-records/t016-qemu-pm-handoff-20261018.json)：2026-10-18 校验 9 个日志包及 263 份内嵌原始文本的字节数/hash、最终 16 项作者测试记录和远端工程/源码/实际二进制/固件身份，结果一致；patch 隔离重应用与持久源码保护核对通过。结论为可交接，无发现的交接阻断项；本轮未重跑模型测试，不代表独立审查、系统验收或章节批准。
- Yocto 阶段范围、精确输入版本、D-015 保留约束及出口见 [T-016 正式集成安排](../TASKS.md)。本轮提交后串行通知本人接收，正式系统集成与运行尚待执行，WORKSPACE 最终 revision 不变。

## T-016 Yocto 正式集成与系统验证（yocto-engineer）

- [本阶段报告](work-records/t016-yocto-system-validation.md)及[完整原始证据目录](work-records/t016-yocto-system-raw/README.md)：2026-10-18 正式 append、独立 tiger machine、target fixture/helper/生成/deploy 集成完成；新构建树脱离 override，真实 runqemu 冷启动/同进程两次 reset、正式二进制模型矩阵 16/16、三项部署诊断及恢复主线通过。E-01～E-10 覆盖、完整失败/告警、启动器返回码局限和未覆盖项见报告。
- 工程仓库 ssh tiger:/home/oops/workspace/meta-tiger，feature/t-016-chapter4-revision 最终 `4dd7dfda8f2af2d59bdcfe6585418cf43eaa73e8`；六个普通模块提交在报告列明。实际构建 metadata 为 `cd6c348836346004e801cb13f32ad3fcb12c1dfd`，后续只修改部署测试/README；工程干净，未推送/合并。原 QEMU patch、固件源、模型工具、持久 source/全部 refs/归档 bundle 与输入不变，master 和 WORKSPACE 正式 revision 未推进。
- [归档清单](work-records/t016-yocto-system-raw/handoff-2/manifest.json)及[本地核验回执](work-records/t016-yocto-system-raw/verification.json)：9 包、291 份原始文本按字节数/SHA-256 全部通过。本人阶段记录与治理提交完成后停止，待 project-manager 核对后衔接 writer；T-016 整体、独立审校及章节批准尚未完成。

## T-016 系统交接核对与正文修订安排（project-manager）

- [2026-10-19 结构化核对回执](work-records/t016-yocto-pm-handoff-20261019.json)：9 包/291 份原文完整性、正式构建/任务依赖、两轮 runqemu 的实际执行/reset、16 项模型结果与部署诊断记录核对通过；远端精确版本、90 份关键任务日志、正式发布资产/helper/工具 hash 及持久源码保护一致。无发现的工程交接阻断项，可供 writer 回填；未重跑运行测试，不代替独立审校或章节批准。
- 按用户明确授权衔接 writer，唯一正文对象为第 4 章；固定输入、教学校准、已知限制和阶段出口见 [T-016 writer 正文修订安排](../TASKS.md)。提交后串行通知本人接收，正文实际修订尚待执行；原专业报告、失败/告警及获批设计保持原样。

## T-016 chapter 4 作者候选稿（writer）

- [作者修订与自查记录](work-records/t016-chapter4-writer-revision.md)：2026-10-19 依据固定设计/蓝图和真实模型/系统证据重写九节教学主线；候选稿 v0.1 draft，待 PM 核对后组织后续审校及用户通读，不作章节批准。
- 书稿位置：本机 `D:/book-yocto`，`git@github.com:yinzhongkai/book-yocto.git`，`feature/t-016-chapter4-revision`；唯一正文 `yocto/task05-4-让QEMU长出tiger这块板.md`，固定开发提交 `003910bf2971e10702019940535c690957740431`，blob `394494f1f7cb078b197d562ff376f6ef8d76e9af`。792 行、只改本章；未推送/合入 main，不推进 WORKSPACE 正式 revision。
- 本轮为作者静态校准与自查，未重跑工程。输入、E-01～E-10 对应、源码/原文检查、配套工程获取和跨章待核对项见记录；实际交付与停止写在 [T-016 writer 阶段](../TASKS.md)，原工程结论及其边界保持不变。

## T-016 第 1 轮正文审校（project-manager）

- 当前受评成果：第 4 章候选稿 v0.1，状态 in_review，书稿仓库 `D:/book-yocto` / `feature/t-016-chapter4-revision`，路径 `yocto/task05-4-让QEMU长出tiger这块板.md`，固定提交 `003910bf2971e10702019940535c690957740431`、blob `394494f1f7cb078b197d562ff376f6ef8d76e9af`。作者交付时的 draft/自查记录按上节保留，本轮不代表章节批准。
- 2026-10-19 PM 已核对交接并依用户“你来协调执行吧”安排 reviewer → qemu-engineer → yocto-engineer → architect 串行审校；范围、署名意见区、用户通读和统一修订条件见 [T-016 正文第 1 轮安排](../TASKS.md)。受评正文和工程输入不变；2026-10-20 reviewer 提交独立审校 pass（`d2d9420af3756fcb0978d5f2b118abec502614b7`），2026-10-21 PM 核对交接，下一位 qemu-engineer，其后 Yocto/architect 按序等待，用户意见待收集。PM-C4-01 继续跟踪正式试读前同版配套资产的获取条件，尚未整章批准。

## 正文元信息样式（不是真实成果）

```text
项目：<project-id>
成果 ID：A-001
负责人：<member-id>
关联任务：<T-编号>
版本：0.1
更新日期：<实际日期>
状态：draft
适用范围：<本成果覆盖的内容和边界>
来源及输入版本：<授权来源、所依赖成果及版本>
批准依据：尚无
版本记录：<日期、版本、主要变化和替代关系>
```

任务完成不自动批准成果。重要交接或批准须引用实际版本及可用的 Git 提交或内容摘要；修改批准内容时更新版本并重新说明评审状态。
