# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与正文链接 | 负责人 | 关联任务 | 版本 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|
| A-001 | [自由的皮皮虾需求说明](A-001-需求说明.md) | product-manager | T-001、T-002、T-007、T-008 | 0.4 | approved | D-001 ~ D-008（2026-09-16、2026-09-28 确认） |
| A-002 | [自由的皮皮虾视觉基线包](A-002-自由的皮皮虾视觉基线包.md) | visual-designer | T-003 | 0.4 | approved | 2026-10-06 用户确认头像“我觉这个头像可以”；product-manager 已关闭 PM-001～PM-004；外部 `main` 功能提交 `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` |
| A-003 | [三星堆首站连载规划](A-003-三星堆首站连载规划.md) | planner | T-004 | 0.3 | draft | D-009（仅确认首站目的地；方案正文仍将在 T-012 与 T-006 中验证） |
| A-004 | [皮皮虾 3:4 封面视觉模板](A-004-封面视觉模板.md) | visual-designer | T-005 | 0.2 | approved | D-010（2026-10-12 批准模板及三星堆示例）；产品最终 `main@5404a33758b83fd5aff00ced55a4d893bf700c08`，原路径/哈希不变；完整封面 pass、问题为空；非发布授权 |
| A-005 | [三星堆第1站封面文案](A-005-三星堆第1站封面文案初稿.md) | writer | T-013、T-005 文案修订 | 0.2 | draft | 全文未批准；产品最终 `main@5404a33758b83fd5aff00ced55a4d893bf700c08`；RV-COPY-001／002 pass / closed、K3 原稿保留；D-010 仅采用示例中的候选 A 短句 |

## T-005 工作区集成定位（2026-10-12）

产品仓库 `git@github.com:yinzhongkai/free-pipi-shrimp.git`，当前线性版本 `5404a33758b83fd5aff00ced55a4d893bf700c08`；当前工作区基线见 WORKSPACE.yaml。A-004 的模板/说明/示例位于 `deliverables/T-005-cover-visual-template/v0.2-draft/`，完整清单位于 `manifests/T-005-cover-visual-template/manifest-v0.2-draft.md`；A-005 位于 `deliverables/T-013-cover-copy-draft/v0.2-draft/cover-copy-draft.md`，来源与修订证据位于对应 work/T-013 目录。全部原路径和已登记哈希保持不变。

两仓库原始生成、作者、评审及批准提交由各自远端标签 `archive/t005-pre-squash-20261012` 保留；成果正文中的原固定 feature SHA 属于来源/受批版本证据，不作为当前 main 定位。合并不改变 A-004 的批准范围、A-005 全文 draft 状态或发布权限。

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

## 当前线性历史定位（2026-10-13，T-014）

用户本轮授权按功能整理历史并移除 merge commit。成果物当前 main 为 `5404a33758b83fd5aff00ced55a4d893bf700c08`，T-003 视觉基线功能提交为 `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd`；内容、原路径、SHA-256、生成/作者/评审/批准记录与业务状态不变。正文中的旧集成/feature 提交保留为历史证据，当前定位以本段和 WORKSPACE 为准；对应恢复点及功能映射见 [T-014 历史整理记录](T-014-history-consolidation.md).
