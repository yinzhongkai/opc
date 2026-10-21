# 首版Git交付与配置修正记录

- 项目：eating-everyday（今天吃什么）
- 成果ID：A-015；负责人：project-manager；关联任务：[T-016](../TASKS.md#t-016合入并推送本轮双仓库代码交付)。
- 版本：0.4，2026-10-22（Asia/Shanghai）；状态：draft。
- 来源：用户授权配置修正、独立复测通过后合并并推送GitHub；T-014/T-015、A-013 v0.3、A-014 v0.4及A-006 v0.5。
- 范围：产品源码与OPC治理资料的Git交付，不包含正式账号办理、微信上传审核或公开上线，不替代专业报告和成果批准。

## 原始交付与已完成验证（压缩前历史）

| 项目 | 实际结果 |
|---|---|
| 产品仓库/本机位置 | git@github.com:yinzhongkai/eating-everyday.git；相对OPC根目录../eating-everyday。 |
| 交付main | [f642dcad85d86d4c1adf251fbd2dad6d8cc84065](https://github.com/yinzhongkai/eating-everyday/commit/f642dcad85d86d4c1adf251fbd2dad6d8cc84065)，从8fa87fe4cb8f7f33aacf2842798a486150a6105e以--ff-only合入；GitHub main已核对同SHA。 |
| 业务与配置修正 | 业务固定8e4472006f1d3cdff5b774052e01ce8db5a96453；测试修正ac9ae93596633919e527934b5c5f64c40112b49a；开发资产aa8c6d8、独立资产f642dcad。后续增量只涉及测试/证据，业务未变。 |
| 独立复测 | A-014 v0.4：保留本机配置与无.git干净导出各299/299；85/85独立配置检查，六类隔离错误均命中原工程保护。各类计数分列，不相加成一次验收。 |
| 目标分支验证 | 产品main再次运行node --test --test-reporter=tap test/*.test.js test/independent/*.test.js，退出0、299/299，失败/取消/跳过/todo均0。原始TAP在产品忽略目录.local/delivery-t016-20261022/main-299.tap；独立公开证据在test/evidence/tester-t015-appid/。 |
| OPC项目主线 | 从a735031c37c09e75fb9478681aaf42010a82f378快进合入集成检查点1b95cd3dbfa961501ea22d4bee51b5910bc46591，目标分支校验和584处本地链接、空白/项目范围及交付引用检查通过，GitHub project/eating-everyday已核对同SHA。当前收尾记录的最终提交以Git历史及远端分支核对定位，不在正文自引用。 |

原始交付时17个产品功能/修复/验证提交保留原SHA，新增范围无merge commit。随后用户要求本轮每仓库压成一条，当前交付及旧SHA的归档入口见下一节。T-010至T-015各任务的原被测SHA和专业结论继续作为历史保留；旧T-013阶段的291/292与292/292是修正前事实，当前配置复测以A-014 v0.4为准。

## 首次压缩结果（配置纳入前历史）

2026-10-22用户追加要求“把commit message按功能压缩整理下，要求只保留一条commit message”。本次只整理刚交付的首版范围：产品17条、OPC项目27条各归并为一条普通提交，提交说明仅一行，更早基线保留。该明确要求作为本轮已共享历史改写的授权；不使用merge --squash或无条件强推。

| 仓库 | 压缩前与当前版本 |
|---|---|
| 产品main | 原头f642dcad85d86d4c1adf251fbd2dad6d8cc84065 → 当前[44ef58be1fa4f7bfd2d372dabefed7f79ea96008](https://github.com/yinzhongkai/eating-everyday/commit/44ef58be1fa4f7bfd2d372dabefed7f79ea96008)；父提交仍为8fa87fe4cb8f7f33aacf2842798a486150a6105e，本轮范围恰好一条提交。提交说明：`feat(T-010..T-015): 完成原生小程序转盘与本机菜单管理`。 |
| OPC project/eating-everyday | 原头9e24dba1047edfb5f82389e7871645ce83c57a9f；27条压为以a735031c37c09e75fb9478681aaf42010a82f378为父提交的一条治理提交，再将当前引用与本节说明纳入同一提交。最终头由Git历史及远端核对定位，不在自身正文自引用。提交说明：`docs(T-002,T-010..T-016): 整理首版交付与验证记录`。 |

两仓库均先保存并推送轻量标签`archive/eating-everyday/t016-before-squash-20261022`：产品标签指向旧f642dcad，OPC标签指向旧9e24dba；[产品归档](https://github.com/yinzhongkai/eating-everyday/tree/archive/eating-everyday/t016-before-squash-20261022)和[OPC归档](https://github.com/yinzhongkai/opc/tree/archive/eating-everyday/t016-before-squash-20261022)保留原测试/评审SHA的可达历史。本机另存已校验Git bundle：产品`.local/history-squash-t016-20261022/old-history.bundle`，OPC`.git/codex-backups/t016-squash-20261022/old-history.bundle`。历史报告和原始证据不改写，也不再把旧SHA称为当前主线祖先。

交互式rebase压缩后先核对各仓库原文件树一致，再仅更新OPC的当前版本引用及本次追溯说明。产品新旧tree均为`4f4ff1a6cc1b5437a8041fef976084a4f3f35a0c`；当前main完整测试重跑299/299，失败/取消/跳过/todo均0，原始TAP在产品忽略目录`.local/history-squash-t016-20261022/product-299.tap`。本机公开和私有配置原字节均保持，私有文件、个人AppID和中间文件未纳入提交。

推送仅使用显式旧头保护：产品`--force-with-lease=refs/heads/main:f642dcad85d86d4c1adf251fbd2dad6d8cc84065`已执行并读回新头相等；OPC仅允许以`--force-with-lease=refs/heads/project/eating-everyday:9e24dba1047edfb5f82389e7871645ce83c57a9f`同步，远端变化即停止。OPC结构及584处本地链接、空白和项目范围检查通过；当前产品引用一致，本轮范围每仓库各一条普通提交，说明只一行。最终同步结果以实际远端读回和会话回执为准。其他克隆接续前须核对新的主线历史。

## 纳入工程配置后的当前交付

用户在首次压缩后明确要求“那把 project.config.json 也合入刚才那条 commit，保持一条提交，再同步到 GitHub。”本次据此替代原来不提交实际AppID、保留一处本机差异的收尾约定；纳入当前测试号配置和工具设置，私有配置与密钥不提交。

产品main从首次压缩头44ef58be1fa4f7bfd2d372dabefed7f79ea96008 amend为[95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0](https://github.com/yinzhongkai/eating-everyday/commit/95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0)，父提交仍为8fa87fe4cb8f7f33aacf2842798a486150a6105e，单行message保持原文，本轮范围仍一条普通提交。相对首次压缩版本仅改动project.config.json、README.md、test/support/project-config.js、test/project-config.test.js及test/miniprogram-package.test.js；业务实现和历史证据不改，新树已不与f642dcad相同。

既有测试原来限定受控配置为touristappid，故本次配套调整为受控/本机配置均接受占位符或格式合法的wx AppID；保留来源校验、非法类型/空值/空白/大小写/长度/字符拒绝，补齐末尾换行拒绝，断言不回显账号值。工程根、基础库、页面、事件、依赖、Unicode一致性和打包排除保护继续执行；README仅同步配置导入说明。原T-014/T-015的专业报告及85项独立检查仍仅代表其原被测版本，不冒充本次新增独立验收。

amend后产品main和新git archive固定提交导出（无.git和私有配置）分别完整299/299通过，退出0，失败/取消/跳过/todo均0；由project-manager实际执行收尾验证。原始TAP在产品忽略目录.local/t016-config-amend-20261022/main-299.tap及clean-export-299.tap。公开配置工作树原始SHA-256仍为97f3a1355a54a3c8875f6da96580357c1fbc3a639079e009acf0aa07c6bae2db；Git沿既有LF规范存储，blob SHA-256为4e4b7ceb6fca79fc699b161f69ada2bb22eec994f0087bb45e4114a5a57ef835，JSON内容与用户原配置相同。私有配置SHA988e8a38…cff4不变，保持忽略，未跟踪；检查未发现工程配置非空密钥字段。产品工作树已干净。

产品以--force-with-lease=refs/heads/main:44ef58be1fa4f7bfd2d372dabefed7f79ea96008同步并核对GitHub main为95173d4e。两仓库改写前另保存本机refs/codex-backups/t016-before-config-20261022及已校验bundle：产品.local/t016-config-amend-20261022/before-config.bundle，OPC.git/codex-backups/t016-config-amend-20261022/before-config.bundle；原归档标签不移动。OPC当前引用和本节纳入原治理提交，最终版本由Git历史和远端核对定位，只允许以旧头96bd69a2e0672d94a1b4da4cc7c0745550b68874的显式lease同步。

## 本机配置保留（配置纳入前历史）

仓库默认project.config.json仍为touristappid；合法实际AppID在本机运行配置单独校验，不跳过工程入口、基础库、事件绑定、依赖、Unicode数据或打包排除保护。公开提交中未检出本机实际AppID；私有配置及.local内容保持忽略。

切换主线前精确备份并只暂存已授权保留的公开配置。Git stash恢复时规范化了换行，SHA保护立即中止后续步骤；确认只存在换行差异后恢复原始备份，再验证后才运行目标分支测试和推送。原公开配置SHA-256始终以最终原字节为准：97f3a1355a54a3c8875f6da96580357c1fbc3a639079e009acf0aa07c6bae2db；私有配置为988e8a38615d839bbe92a89bf5b255e6ccb7be5c218ab1b8bbc71593870ccff4。本次自身stash已清理，原始备份保留于产品忽略目录，未操作其他数据。

## 当前收尾与边界

WORKSPACE.revision在工程配置纳入、重测及远端核对后更新到95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0，repository/branch/driver/相对checkout均未改变。原两仓库feature已经安全删除。本次历史整理后的OPC范围仍仅含当前项目原有13个路径，本次OPC追加修改只涉及当前交付引用和追溯说明，产品工程配置及配套校验/说明已按用户授权amend，无框架变更或新增merge commit；按用户本次授权以显式旧头lease同步。本阶段没有开始新一轮业务开发。

正式小程序账号由用户另行准备。手机结果仍为用户自测，设备/微信版本、实际构建指纹、真实满配额、多设备及历史GUI原始附件缺口等按A-014/A-006保留。H-005/H-006等交接由原发起人核对，不因本次Git操作自动关闭；需求与架构等成果保持draft。

## 版本记录

| 日期 | 版本 | 变化 |
|---|---|---|
| 2026-10-22 | 0.1 | 记录配置修正与独立通过、产品main合入验证和远端核对；同步最终产品版本，OPC收尾继续于T-016。 |
| 2026-10-22 | 0.2 | 补记OPC项目主线快进合入、目标分支验证与GitHub核对，T-016完成；沿原分支同步完成记录并执行最终安全清理。 |
| 2026-10-22 | 0.3 | 按用户要求将本轮每仓库提交各压成一条单行说明，保留旧头归档标签和bundle；核对产品文件树一致、299/299重跑和GitHub新头，更新当前交付引用并记录显式旧头保护同步方式。 |
| 2026-10-22 | 0.4 | 按用户要求将工程配置及必要校验/README纳入原产品提交，保持本轮一条单行message；主线与干净导出各299/299，更新当前产品版本，私有配置保持忽略。 |
