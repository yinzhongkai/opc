# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

当前首版以 A-001 v0.9、D-016 为准，全部候选由用户输入，不获取定位或附近餐厅数据。A-002/A-003仅为旧核实资料，A-004保留视觉参考，A-005 v0.3归档旧数据验证。architect基于8e44720实现、10f52d4专项资产及用户手机自测，完成A-006 v0.5/A-008 v0.3复核：11/11/27追溯不变，原B-01～B-04解除，当前无须用户确认；手机来源与未覆盖边界如实保留，成果仍draft，不等于整份批准或全设备验收。

2026-10-22交付更新：T-014/T-015已完成配置适配与独立复测，两类配置各299/299。产品main最终提交f642dcad85d86d4c1adf251fbd2dad6d8cc84065已合入、验证及推送；当前交付状态见[A-015](git-delivery-handoff.md)。下表保留各轮专业成果的原被测版本与历史结论，后续交付引用不改写其执行结果。

| 成果 ID | 名称与位置 | 存储位置 | 负责人 | 关联任务 | 版本/精确提交 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|---|
| A-001 | [今天吃什么首版产品需求草案](product-requirements.md) | OPC artifacts；截图参考位于 references/meal-wheel-reference.png | product-manager | [T-001](../TASKS.md#t-001产品需求首轮澄清)、[T-003](../TASKS.md#t-003整理首版需求与架构师交接)、[T-006](../TASKS.md#t-006写回用户确认的七项规则)、[T-007](../TASKS.md#t-007调整为全手动候选需求) | 0.9，2026-10-20；当前正文指纹见 H-005，旧版本历史见 H-002/H-003 | draft | D-016 确认全手动候选并撤销定位、距离、搜索、餐厅数据源和覆盖验证；有效数量 11/11/27，原菜单边界继续有效，未批准整份架构或实现 |
| A-002 | [高德接口免费额度与本机菜单保存授权核实](amap-usage-assessment.md) | OPC artifacts | product-manager | [T-001](../TASKS.md#t-001产品需求首轮澄清) | 0.2，2026-10-16 | draft | D-016 后仅作历史核实记录，不再作为本版数据接口待办；未批准最终选型 |
| A-003 | [首版餐厅数据源可行性比较](data-source-feasibility.md) | OPC artifacts | product-manager | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-16 | draft | D-016 已取消本版数据源和林萃桥覆盖需求；原未验证事实保留为历史，不视为通过 |
| A-004 | [首版页面与交互方案](interface-plan.md) | OPC artifacts；治理附件 meal-wheel-preview.html | product-manager | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-16 | draft | D-011/D-016 保留截图视觉方向；旧搜索和模拟数据仅作历史，v0.9 页面由架构师适配 |
| A-005 | [数据源与林萃桥覆盖验证记录](architecture-data-verification.md) | OPC artifacts；旧证据与 D-016 撤销说明 | architect | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.3，2026-10-20 | draft | 旧 V-01 至 V-04 已撤销，不再补名单；保留历史失败及接收前 Overture 成功取数，未作最终餐厅去重或选型 |
| A-006 | [首版系统架构与交互设计](system-architecture.md) | OPC artifacts；全手动本机架构、11/11/27、PV/B实现与证据复核 | architect | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.5，2026-10-22；接手d788266，固定实现8e4472006f1d3cdff5b774052e01ce8db5a96453，证据资产10f52d40bb8a62b04a8f84793e44117fe3c28e46；历史被测版本保留，当前交付见A-015 | draft | 原B-01～B-04解除，T-002按原条件completed；明确提交292/292与保留配置工作树291/292、用户手机来源及非阻塞限制，无当前用户确认项；无整份批准 |
| A-007 | [首版业务规则与数据源推进选项](product-decision-options.md) | OPC artifacts；v0.8 七项答复历史和 v0.9 适用/撤销映射 | product-manager | [T-005](../TASKS.md#t-005整理架构反馈中的用户确认选项)、[T-006](../TASKS.md#t-006写回用户确认的七项规则)、[T-007](../TASKS.md#t-007调整为全手动候选需求) | 0.3，2026-10-20；当前指纹见 H-005 | draft | D-016 替代旧来源、样本和跨来源同名范围；D-014 菜单管理保留，历史选项不再作为数据验证待办 |
| A-008 | [当前首版账号与权限前置清单](accounts-access-checklist.md) | OPC artifacts；已解除前置、配置测试差异、用户待办与后续发布边界 | architect | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.3，2026-10-22；同A-006实现/证据版本 | draft | 入口、工具和手机已有实际结果，当前无须用户确认；本机配置字节保留，建议developer/tester后续有界处理测试适配，发布资格/费用仍未核实 |
| A-009 | [产品仓库初始化 README](https://github.com/yinzhongkai/eating-everyday/blob/8fa87fe4cb8f7f33aacf2842798a486150a6105e/README.md) | 产品仓库 git@github.com:yinzhongkai/eating-everyday.git；主机：本机；checkout：../eating-everyday；仓库内路径：README.md | project-manager | [T-009](../TASKS.md#t-009初始化产品仓库并配置独立工作区) | 8fa87fe4cb8f7f33aacf2842798a486150a6105e，main，2026-10-21 | draft | D-017 授权 README 初始化、推送和工作区基线登记；仓库仅完成初始化，无应用实现或验收 |
| A-010 | [全手动首版测试计划与第一阶段交接](core-test-plan.md) | OPC artifacts；27 项 AC 分层用例、边界/状态、故障矩阵、随机及接口观察约定；实际执行另见 A-012 | tester | [T-011](../TASKS.md#t-011准备验收用例并执行不依赖-appid-的测试)，供 T-010 读取 | 0.1，2026-10-21；第一阶段提交1f537859636de9f763e10a355ec20887da29e148，正文未改 | draft | 保留第一阶段设计版本；第二阶段已按固定候选实际执行，结果与缺陷见[报告](core-test-report.md)，不将设计当作验收通过 |
| A-011 | [核心实现与开发自测交接](developer-core-handoff.md) | OPC artifacts；本机产品git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；src/、test/historical-replay.test.js、docs/core-api.md、test/evidence/developer-fix-t011-002/及原工程 | developer | [T-010](../TASKS.md#t-010实现不依赖-appid-的核心逻辑与开发自测)，供T-011复测读取 | 0.3，2026-10-21；最新核心候选54b720fa920dfcda9a3db5fa0e42378be01f5535，父提交tester资产a82d86b；原候选保留历史；历史候选已包含于main，最终交付见A-015 | draft | 001由tester关闭；002及四类历史回放已开发修复，123项开发及原113项开发复核均通过、合并236通过，17份tester资产不变，待tester复测002；T-012未开始，无微信/真机验收 |
| A-012 | [核心独立测试报告与缺陷复测](core-test-report.md) | OPC artifacts；本机产品仓库git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；test/independent/与test/evidence/下三个tester证据目录 | tester | [T-011](../TASKS.md#t-011准备验收用例并执行不依赖-appid-的测试)，D/M/O后续由T-013接续 | 0.3，2026-10-21；通过的被测实现54b720fa920dfcda9a3db5fa0e42378be01f5535，新测试资产f2646b52f7e6746b2814076bfab99e91a5e95f07；旧版本/失败历史保留，治理提交按T-011/Git定位 | draft | 001/002均独立复测closed；新增21项及最终257/257（开发123＋独立134）通过，T-011当前L范围completed。真实wx/UI/工具/手机/离线D/M/O未执行，交T-013，不代表整体验收或方案批准 |
| A-013 | [原生小程序工程与平台复核交接](miniprogram-preview-handoff.md) | OPC artifacts；本机产品git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；本轮test配置适配及test/evidence/developer-t014-appid/ | developer | 当前[T-014](../TASKS.md#t-014适配仓库与本机appid配置检查)，供T-015独立复测；T-012历史保留 | 0.3，2026-10-22；测试修正ac9ae93596633919e527934b5c5f64c40112b49a，证据资产HEAD aa8c6d83946ca7ec0ebe420c408a9511799c77ea；已包含于main交付f642dcad85d86d4c1adf251fbd2dad6d8cc84065 | draft | 两来源完整套件各299/299；本机配置97f3a135…e2db原字节保持且未提交，业务/公共打包未改。新增7项保护，旧291/292历史保留；T-014完成开发交接，独立结论及合并推送交T-015/T-016，不重开T-002/T-013 |
| A-014 | [小程序独立平台测试与缺陷复测](miniprogram-test-report.md) | OPC artifacts；本机产品git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；新test/evidence/tester-t015-appid/及T-013历史证据 | tester | 当前[T-015](../TASKS.md#t-015独立复测两类appid配置并给出合入结论)；[T-013](../TASKS.md#t-013验证小程序导入编译与预览闭环)历史保留 | 0.4，2026-10-22；业务8e4472006f1d3cdff5b774052e01ce8db5a96453，配置测试ac9ae93596633919e527934b5c5f64c40112b49a，被测候选aa8c6d83946ca7ec0ebe420c408a9511799c77ea，新测试资产f642dcad85d86d4c1adf251fbd2dad6d8cc84065；已包含于main交付f642dcad85d86d4c1adf251fbd2dad6d8cc84065 | draft | 两配置独立完整回归各299/299，85/85辅助检查及六类隔离缺陷拒绝均通过；211原文件与本机公共/私有配置字节不变、个人AppID未提交，新临时目录已清理。T-015 completed、可合入，T-016交project-manager；T-013完成/手机用户自测及旧291/292、292/292和设备/附件限制保留，不等于批准或发布 |
| A-015 | [首版Git交付与配置修正记录](git-delivery-handoff.md) | OPC artifacts；产品git@github.com:yinzhongkai/eating-everyday.git，本机../eating-everyday，main；源码与test/evidence/ | project-manager | [T-016](../TASKS.md#t-016合入并推送本轮双仓库代码交付) | 0.1，2026-10-22；产品最终f642dcad85d86d4c1adf251fbd2dad6d8cc84065，GitHub main已核对 | draft | 用户授权配置修正、独立复测通过后合并推送；产品已交付，OPC收尾进行中。保留本机字节、历史证据和专业限制，不包含微信提审或发布 |

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
