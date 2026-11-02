# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

2026-11-02OPC采用完成：[T-024](../TASKS.md#t-024按功能整理并合入opc默认菜单记录)按用户恢复指令，将WORKSPACE.revision锁定为已合入且同步远端的产品main/v1.0.1提交3800e6dd8bc32b7d767bfc873a4b9ae57d10232b。需求/架构/研发/独立测试原专业正文保持，OPC按功能整理并快进合入project/eating-everyday，合入前后结构/链接、目录及精确版本检查通过，当前采用见[A-018 v0.3](product-release-v1.0.1.md)。项目主线同步和分支清理按最终交付步骤及Git引用校验，全部现有OPC标签保持；下述暂缓/未推送文字保留各自时点事实。

2026-11-02Git交付：[T-027](../TASKS.md#t-027调整产品标签并创建v101发布merge)/[T-028](../TASKS.md#t-028删除产品功能分支并同步主线及标签到远端)/[A-018 v0.2](product-release-v1.0.1.md)记录标签更名、双父v1.0.1发布及追加授权后的安全分支清理与原子推送。产品main/origin/main/远端main及v1.0.1目标均为3800e6dd8bc32b7d767bfc873a4b9ae57d10232b，v1.0.0仍为6c9ef2b3135a0013fb0b06ebbe761b298c4b7260；tag对象/目标均已读回核对，两边旧v0.1.0名称和产品feature均不存在，工作树干净。当前GitHub及本地交付以此为准，下述T-025/T-026及A-016保留历史时点事实；OPC合并/WORKSPACE采用仍暂缓，未创建GitHub Release页面。

2026-11-02正式AppID补充：[T-026](../TASKS.md#t-026纳入用户确认的正式小程序appid)按用户确认，将wx8c3f40e0d7f5acab作为正式小程序AppID纳入产品project.config.json，并修正README账号说明；独立单行提交8ad2ae35f8a9da39c1c9202c072b9045c9edc4b4已快进合入本机main，工作树/干净导出/main各501/501，产品工作树干净。git@github.com:yinzhongkai/eating-everyday.git当前本地交付以此版本为准，下述T-025保留默认菜单合入时点事实；OPC仍暂缓合并/版本采用，未推送或移动标签，不代表账号权限或正式发布验收。

2026-11-02产品本地合入：用户最新明确“先合并产品仓库，opc仓库稍后再说”。[T-025](../TASKS.md#t-025整理并本地合入产品默认菜单)已将git@github.com:yinzhongkai/eating-everyday.git的feature/default-menu整理为三组单行提交，快进合入本机main，精确版本94afc6fd191ac09fa8fc2e216bb2fb8e3c7726d5；位置包括src/default-menu.js、src/startup.js、src/repository.js、pages/index/、test/default-menu*.test.js、test/independent/default-menu.acceptance.test.js及下述两类证据目录。本机配置/干净导出/main三环境各501/501，未推送。[T-024](../TASKS.md#t-024按功能整理并合入opc默认菜单记录)的OPC合并及WORKSPACE采用按用户指令暂缓，项目长期分支和已有标签保持；恢复时再核对最终版本与引用。

2026-11-02默认菜单进展：T-020～T-023完成，A-001 v0.10/A-007 v0.4为12/12/32需求，A-006 v0.6/A-008 v0.4为一次性初始化契约。A-013 v0.4原交付代码dcb22d3bdcec196e37b544805f79196de126f676，开发资产33e3e64cd3e59d48adbd13d69622a82087468016，目录test/evidence/developer-t022-default-menu/。A-014 v0.6保留独立129/129、两配置各501/501，续测12组真实wx差异、实际重试/保存加载/抽取及隔离清理完成，无新产品缺陷；证据位于test/evidence/tester-t023-default-menu/及resume-20261102/，原精确版本12649a797966299c4f23b6d4a4b9f0b3d2d1619e。这些为整理前历史被测/证据SHA，由T-025所列产品original.bundle保留，不误作压缩后main祖先；本地合入版本以上段为准。8项及首次/已有菜单保护按D-019，各成果仍draft，实体手机及物理故障等限制保留。

2026-11-02后续计划更新：用户明确“不用在github release”，依据[D-018](../DECISIONS.md#d-018取消github-release页面安排)取消GitHub Release页面创建，记录同步见[T-019](../TASKS.md#t-019同步取消github-release页面安排)。已有产品v0.1.0和OPC project/eating-everyday/v0.1.0标签保持固定，README/发布说明及新最终独立复核仍待后续。以下2026-10-22发布记录及A-016/A-017正文保留原执行时点事实，其中“Release页面待后续”的安排由D-018替代；未创建页面不再作为当前待办，不改变成果draft状态。

以下为2026-10-22首版交付历史摘要，不覆盖上述默认菜单当前进展：当时以A-001 v0.9、D-016为准，候选完全由用户输入，不获取定位或附近餐厅数据。A-002/A-003为旧核实资料，A-004为视觉参考，A-005 v0.3归档旧数据验证。architect基于8e44720实现、10f52d4专项资产及用户手机自测完成A-006 v0.5/A-008 v0.3复核，11/11/27追溯及原B-01～B-04解除为当时结论，不等于本次默认功能通过、整份批准或全设备验收。

2026-10-22 OPC项目发布更新：用户追加明确要求标签名project/eating-everyday/v0.1.0且必须指向最新提交；本次所有发布回执先提交，再生成最终双父发布merge，新附注tag剥离目标须与本机/GitHub项目主线HEAD一致，旧名eating-everyday/v0.1.0按授权删除；见[A-017 v0.3](opc-release-tag.md)。用户另要求折叠旧发布merge与回执，当前拓扑以A-017 v0.3为准；下段及A-017旧版本为历史，既有框架v0.0.*与产品v0.1.0不改。

2026-10-22发布标签更新：按用户最新要求先执行T-017，产品main为6c9ef2b3135a0013fb0b06ebbe761b298c4b7260，附注v0.1.0指向该双父发布merge，299/299通过且树与95173d4相同；见[A-016 v0.1](product-release-tag.md)。下段及A-015保留此前源码交付与单提交整理事实；当前GitHub Release页面、文档更新及新最终独立复核尚未执行。

2026-10-22交付更新：T-014/T-015已完成配置适配与独立复测，两类配置各299/299。按用户追加的单提交要求，本轮产品17条、OPC项目27条提交各整理为一条单行提交；当前产品main为95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0，本次按用户追加要求纳入工程配置及配套校验/说明，业务实现保持；主线和干净导出各299/299，GitHub核对通过，当前交付状态见[A-015 v0.4](git-delivery-handoff.md)。本次工程配置提交改变了此前“实际AppID仅保留在本机”的收尾约定；下表A-013/A-014的未提交说明仅指其原执行时点，专业成果的原被测版本与历史结论保留，旧SHA可通过两仓库archive/eating-everyday/t016-before-squash-20261022标签追溯，不表示压缩后main的祖先。

| 成果 ID | 名称与位置 | 存储位置 | 负责人 | 关联任务 | 版本/精确提交 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|---|
| A-001 | [今天吃什么首版产品需求草案](product-requirements.md) | OPC artifacts；截图参考位于 references/meal-wheel-reference.png | product-manager | [T-001](../TASKS.md#t-001产品需求首轮澄清)、[T-003](../TASKS.md#t-003整理首版需求与架构师交接)、[T-006](../TASKS.md#t-006写回用户确认的七项规则)、[T-007](../TASKS.md#t-007调整为全手动候选需求)、[T-020](../TASKS.md#t-020更新内置默认菜单需求与验收) | 0.10，2026-11-02；当前正文指纹及本地提交定位见 T-020，旧 v0.9 指纹保留在 H-005 | draft | D-019 局部替代首次为空/不预填，新增一次性 8 项默认菜单、已有/同名数据保护和失败恢复验收；有效数量 12/12/32，D-016 其余约束和原菜单边界保留，设计/实现/独立测试待 T-021 至 T-023，无整份成果批准 |
| A-002 | [高德接口免费额度与本机菜单保存授权核实](amap-usage-assessment.md) | OPC artifacts | product-manager | [T-001](../TASKS.md#t-001产品需求首轮澄清) | 0.2，2026-10-16 | draft | D-016 后仅作历史核实记录，不再作为本版数据接口待办；未批准最终选型 |
| A-003 | [首版餐厅数据源可行性比较](data-source-feasibility.md) | OPC artifacts | product-manager | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-16 | draft | D-016 已取消本版数据源和林萃桥覆盖需求；原未验证事实保留为历史，不视为通过 |
| A-004 | [首版页面与交互方案](interface-plan.md) | OPC artifacts；治理附件 meal-wheel-preview.html | product-manager | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.1，2026-10-16 | draft | D-011/D-016 保留截图视觉方向；旧搜索和模拟数据仅作历史，v0.9 页面由架构师适配 |
| A-005 | [数据源与林萃桥覆盖验证记录](architecture-data-verification.md) | OPC artifacts；旧证据与 D-016 撤销说明 | architect | [T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.3，2026-10-20 | draft | 旧 V-01 至 V-04 已撤销，不再补名单；保留历史失败及接收前 Overture 成功取数，未作最终餐厅去重或选型 |
| A-006 | [首版系统架构与交互设计](system-architecture.md) | OPC artifacts；默认菜单一次性初始化、旧数据/恢复契约及12/12/32追溯；第9节保留旧PV/B证据 | architect | 当前[T-021](../TASKS.md#t-021补充默认菜单首次初始化技术契约)；历史[T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.6，2026-11-02；输入0e649fa0deb28677680d1b127cdc961c482828ae内A-001 v0.10/A-007 v0.4，产品只读基线6c9ef2b3135a0013fb0b06ebbe761b298c4b7260；本轮正文/提交定位见T-021 | draft | D-019范围；新/旧空库/异常分类、根内pending/complete、同次提交及幂等、删除不复活和DM-01～12已成契约，未实施或测试；原T-002完成/证据边界保留，无整份批准 |
| A-007 | [首版业务规则与数据源推进选项](product-decision-options.md) | OPC artifacts；D-019 默认菜单行为/验收映射、保留 v0.8 七项答复和旧范围撤销历史 | product-manager | [T-005](../TASKS.md#t-005整理架构反馈中的用户确认选项)、[T-006](../TASKS.md#t-006写回用户确认的七项规则)、[T-007](../TASKS.md#t-007调整为全手动候选需求)、[T-020](../TASKS.md#t-020更新内置默认菜单需求与验收) | 0.4，2026-11-02；当前指纹及本地提交定位见 T-020，旧 v0.3 指纹保留在 H-005 | draft | D-019 首次自动加载且已有菜单不覆盖，映射 A-001 v0.10 的 12/12/32；D-014 菜单规则和 D-016 其余边界保留，无须重复确认业务选择；旧数据验证不恢复，技术契约/实现/独立测试尚待后续 |
| A-008 | [当前首版账号与权限前置清单](accounts-access-checklist.md) | OPC artifacts；默认菜单当前准备/待验证边界，第1～5节保留旧账号/配置证据 | architect | 当前[T-021](../TASKS.md#t-021补充默认菜单首次初始化技术契约)；历史[T-002](../TASKS.md#t-002数据源可行性验证与首版界面方案) | 0.4，2026-11-02；输入及产品只读基线同A-006 v0.6，当前正文/提交定位见T-021 | draft | 本次无新用户业务确认、账号或权限前置；配置原字节保留，技术契约无阻塞，默认初始化/兼容/恢复实现和独立证据待T-022/T-023；旧测试不覆盖本次功能，无整份批准 |
| A-009 | [产品仓库初始化 README](https://github.com/yinzhongkai/eating-everyday/blob/8fa87fe4cb8f7f33aacf2842798a486150a6105e/README.md) | 产品仓库 git@github.com:yinzhongkai/eating-everyday.git；主机：本机；checkout：../eating-everyday；仓库内路径：README.md | project-manager | [T-009](../TASKS.md#t-009初始化产品仓库并配置独立工作区) | 8fa87fe4cb8f7f33aacf2842798a486150a6105e，main，2026-10-21 | draft | D-017 授权 README 初始化、推送和工作区基线登记；仓库仅完成初始化，无应用实现或验收 |
| A-010 | [全手动首版测试计划与第一阶段交接](core-test-plan.md) | OPC artifacts；27 项 AC 分层用例、边界/状态、故障矩阵、随机及接口观察约定；实际执行另见 A-012 | tester | [T-011](../TASKS.md#t-011准备验收用例并执行不依赖-appid-的测试)，供 T-010 读取 | 0.1，2026-10-21；第一阶段提交1f537859636de9f763e10a355ec20887da29e148，正文未改 | draft | 保留第一阶段设计版本；第二阶段已按固定候选实际执行，结果与缺陷见[报告](core-test-report.md)，不将设计当作验收通过 |
| A-011 | [核心实现与开发自测交接](developer-core-handoff.md) | OPC artifacts；本机产品git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；src/、test/historical-replay.test.js、docs/core-api.md、test/evidence/developer-fix-t011-002/及原工程 | developer | [T-010](../TASKS.md#t-010实现不依赖-appid-的核心逻辑与开发自测)，供T-011复测读取 | 0.3，2026-10-21；最新核心候选54b720fa920dfcda9a3db5fa0e42378be01f5535，父提交tester资产a82d86b；原候选保留历史；历史候选已包含于main，最终交付见A-015 | draft | 001由tester关闭；002及四类历史回放已开发修复，123项开发及原113项开发复核均通过、合并236通过，17份tester资产不变，待tester复测002；T-012未开始，无微信/真机验收 |
| A-012 | [核心独立测试报告与缺陷复测](core-test-report.md) | OPC artifacts；本机产品仓库git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；test/independent/与test/evidence/下三个tester证据目录 | tester | [T-011](../TASKS.md#t-011准备验收用例并执行不依赖-appid-的测试)，D/M/O后续由T-013接续 | 0.3，2026-10-21；通过的被测实现54b720fa920dfcda9a3db5fa0e42378be01f5535，新测试资产f2646b52f7e6746b2814076bfab99e91a5e95f07；旧版本/失败历史保留，治理提交按T-011/Git定位 | draft | 001/002均独立复测closed；新增21项及最终257/257（开发123＋独立134）通过，T-011当前L范围completed。真实wx/UI/工具/手机/离线D/M/O未执行，交T-013，不代表整体验收或方案批准 |
| A-013 | [原生小程序工程与平台复核交接](miniprogram-preview-handoff.md) | OPC artifacts；本机产品git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；src/default-menu.js、src/startup.js、src/repository.js、pages/index/及test/evidence/developer-t022-default-menu/ | developer | 当前[T-022](../TASKS.md#t-022实现首次自动加载内置默认菜单)，供T-023独立验证；T-012/T-014历史保留 | 0.4，2026-11-02；固定代码dcb22d3bdcec196e37b544805f79196de126f676，完整证据资产HEAD 33e3e64cd3e59d48adbd13d69622a82087468016，feature/default-menu未合入main | draft | D-019/12/12/32及DM-01～12；工作树/干净导出各372/372，新73项及隔离真实wx六组开发冒烟通过。173份原独立测试/历史证据及两配置原字节保持，现配置eecc5998…e645未提交；手机/实体离线/物理故障/降级兼容未新增验证。T-022开发交接完成，T-023待独立执行，不代表成果批准/可合入/发布，旧299/299保留历史 |
| A-014 | [小程序独立平台测试与缺陷复测](miniprogram-test-report.md) | OPC artifacts；本机产品git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday；test/independent/default-menu.acceptance.test.js及test/evidence/tester-t023-default-menu/resume-20261102/ | tester | 当前[T-023](../TASKS.md#t-023独立验证默认菜单及已有数据保护)；T-013/T-015及v0.5暂停历史保留 | 0.6，2026-11-02；固定代码dcb22d3bdcec196e37b544805f79196de126f676，暂停资产eb4f6838ecd17a69bf9690cd4abbf542eef6fe69，本次平台证据12649a797966299c4f23b6d4a4b9f0b3d2d1619e；未合入main | draft | T-023 completed，独立建议可合入，无新增产品缺陷。原129/129及两配置各501/501保留；12组真实wx、实际鼠标重试1次原ID完整8项/dirty=false、保存重载/抽取及删除重进通过。262原文件和两配置字节保持，仅本域清理剩余0且隔离工程关闭。手机/实体离线/物理故障等限制、旧暂停/权限失败保留，不等于整份批准或发布 |
| A-015 | [首版Git交付与配置修正记录](git-delivery-handoff.md) | OPC artifacts；产品git@github.com:yinzhongkai/eating-everyday.git，本机../eating-everyday，main；源码与test/evidence/ | project-manager | [T-016](../TASKS.md#t-016合入并推送本轮双仓库代码交付) | 0.4，2026-10-22；压缩后产品最终95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0，GitHub main已核对 | draft | 用户追加要求本轮每仓库一条单行提交；保留旧基线、归档标签和bundle，显式旧头lease同步。已将工程配置及配套测试/说明amend到原提交；主线与干净导出各299/299，本机字节和私有文件保留、工作树干净，旧专业结论仍按原版本适用，不包含微信提审或发布 |
| A-016 | [产品v0.1.0发布merge与标签记录](product-release-tag.md) | OPC artifacts；产品git@github.com:yinzhongkai/eating-everyday.git，本机../eating-everyday，main及附注v0.1.0 | project-manager | [T-017](../TASKS.md#t-017为成果物v010创建发布merge提交与标签) | 0.1，2026-10-22；发布merge 6c9ef2b3135a0013fb0b06ebbe761b298c4b7260，GitHub main/tag剥离目标已核对 | draft | 用户最新要求先打带merge commit的发布标签；双父及树一致、299/299、原子推送已实际完成。GitHub Release页面和新文档/最终独立复核未执行，不代表微信上线或整份成果批准 |
| A-017 | [OPC项目发布标签记录](opc-release-tag.md) | OPC artifacts；git@github.com:yinzhongkai/opc.git，项目主线project/eating-everyday，附注project/eating-everyday/v0.1.0 | project-manager | [T-018](../TASKS.md#t-018为opc当前项目快照创建发布merge与标签) | 0.3，2026-10-22；新标签固定最终发布merge且等于最新项目主线HEAD，确切提交通过已核对Git引用解析，不在同一提交自写自身哈希 | draft | 用户明确修正命名、最新头并要求合并旧发布/回执节点；所有回执先纳入发布merge，新tag/主线/旧名删除由原子推送及旧头lease保护，发布后不再追加commit。旧tag已作本机恢复备份，最终结果按远端核对；产品和框架保持 |
| A-018 | [产品标签更名与v1.0.1发布记录](product-release-v1.0.1.md) | OPC artifacts；产品git@github.com:yinzhongkai/eating-everyday.git，本机/GitHub main及附注v1.0.0/v1.0.1；OPC WORKSPACE锁定同一产品最终提交 | project-manager | [T-027](../TASKS.md#t-027调整产品标签并创建v101发布merge)、[T-028](../TASKS.md#t-028删除产品功能分支并同步主线及标签到远端)、[T-024](../TASKS.md#t-024按功能整理并合入opc默认菜单记录) | 0.3，2026-11-02；产品v1.0.1/main及OPC锁定版本均为3800e6dd8bc32b7d767bfc873a4b9ae57d10232b；v1.0.0仍为旧6c9ef2b；远端对象/目标已核对 | draft | 产品双父/单行message/501项、分支清理与远端同步已完成；用户恢复OPC后工作区采用及快进合入/目标检查完成，现有OPC标签保持；无GitHub Release页面或微信正式发布 |

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
