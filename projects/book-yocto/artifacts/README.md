# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与正文链接 | 负责人 | 关联任务 | 版本 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|
| A-001 | [书稿现状评估](A-001-书稿现状评估.md) | writer | T-001 | 0.1 | draft | 尚无 |
| A-002 | [书稿打磨计划](A-002-book-polishing-plan.md) | project-manager | T-002 | 0.5 | draft | 范围与验收依据 D-001~D-012（confirmed）；v0.5 已纳入逐章真实环境核验及内部流程清理，计划文本待确认 |
| A-003 | [知识点校验清单](A-003-知识点校验清单.md) | writer | T-003 | 0.1 | draft | 尚无 |
| A-004 | [体例约定](A-004-体例约定草案.md) | writer | T-005、T-013 | 0.5 | approved | v0.5 于 2026-09-27 经用户明确批准（“批准 A-004 v0.5”）；新增硬件型号、粗体/反引号边界及等宽文本规则正式生效 |

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
