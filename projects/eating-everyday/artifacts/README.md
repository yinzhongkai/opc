# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与位置 | 存储位置 | 负责人 | 关联任务 | 版本/精确提交 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|---|
| A-001 | [今天吃什么首版产品需求草案](product-requirements.md) | OPC artifacts；截图参考位于 references/meal-wheel-reference.png | product-manager | [T-001](../TASKS.md#t-001产品需求首轮澄清)、[T-003](../TASKS.md#t-003整理首版需求与架构师交接) | 0.7，2026-10-19；交接指纹见 H-002 | draft | 功能及偏好依据 D-001 至 D-009；D-010、D-011 为阶段与分工依据；数据源与覆盖仍待验证，整份版本未新增批准 |
| A-002 | [高德接口免费额度与本机菜单保存授权核实](amap-usage-assessment.md) | OPC artifacts | product-manager | [T-001](../TASKS.md#t-001产品需求首轮澄清) | 0.2，2026-10-16 | draft | 尚无；已按 D-009 补充长期免费判断，未批准最终选型 |
| A-003 | [首版餐厅数据源可行性比较](data-source-feasibility.md) | OPC artifacts | product-manager | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-16 | draft | 尚无；林萃桥真实覆盖及最终选型未确认 |
| A-004 | [首版页面与交互方案](interface-plan.md) | OPC artifacts；治理附件 meal-wheel-preview.html | product-manager | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-16 | draft | D-011 记录用户同意先沿用界面方向；保留为参考，后续设计由架构师负责 |
| A-005 | [数据源与林萃桥覆盖验证记录](architecture-data-verification.md) | OPC artifacts；官方条件及实际查询失败证据 | architect | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-19 | draft | 尚无；V-03 及项目许可、平台条件未验证通过，新增 Private.coffee 候选未选定 |
| A-006 | [首版系统架构与交互设计](system-architecture.md) | OPC artifacts；系统、模型、接口、状态及 15/9/26 追溯 | architect | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-19 | draft | 尚无；TBD-03 至 TBD-06 与 AR-01 至 AR-11 交 H-003 整理，未改写已确认需求 |

OPC 治理与协作成果使用当前项目 `artifacts/` 内的正文链接；产品代码和直接业务交付物使用“成果物仓库 + 仓库内路径”，并记录其合入后的最终提交。快进合并后可直接引用通过最终验证的普通功能提交，可移动分支名不能代替精确提交。

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
存储位置：<OPC artifacts 路径 | 成果物仓库、仓库内路径与合入后的最终提交>
批准依据：尚无
版本记录：<日期、版本、主要变化和替代关系>
```

任务完成不自动批准成果。重要交接或批准须引用实际版本及可用的 Git 提交或内容摘要；修改批准内容时更新版本并重新说明评审状态。
