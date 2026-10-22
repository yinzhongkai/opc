# 首版Git交付与配置修正记录

- 项目：eating-everyday（今天吃什么）
- 成果ID：A-015；负责人：project-manager；关联任务：[T-016](../TASKS.md#t-016合入并推送本轮双仓库代码交付)。
- 版本：0.1，2026-10-22（Asia/Shanghai）；状态：draft。
- 来源：用户授权配置修正、独立复测通过后合并并推送GitHub；T-014/T-015、A-013 v0.3、A-014 v0.4及A-006 v0.5。
- 范围：产品源码与OPC治理资料的Git交付，不包含正式账号办理、微信上传审核或公开上线，不替代专业报告和成果批准。

## 精确版本与已完成验证

| 项目 | 实际结果 |
|---|---|
| 产品仓库/本机位置 | git@github.com:yinzhongkai/eating-everyday.git；相对OPC根目录../eating-everyday。 |
| 交付main | [f642dcad85d86d4c1adf251fbd2dad6d8cc84065](https://github.com/yinzhongkai/eating-everyday/commit/f642dcad85d86d4c1adf251fbd2dad6d8cc84065)，从8fa87fe4cb8f7f33aacf2842798a486150a6105e以--ff-only合入；GitHub main已核对同SHA。 |
| 业务与配置修正 | 业务固定8e4472006f1d3cdff5b774052e01ce8db5a96453；测试修正ac9ae93596633919e527934b5c5f64c40112b49a；开发资产aa8c6d8、独立资产f642dcad。后续增量只涉及测试/证据，业务未变。 |
| 独立复测 | A-014 v0.4：保留本机配置与无.git干净导出各299/299；85/85独立配置检查，六类隔离错误均命中原工程保护。各类计数分列，不相加成一次验收。 |
| 目标分支验证 | 产品main再次运行node --test --test-reporter=tap test/*.test.js test/independent/*.test.js，退出0、299/299，失败/取消/跳过/todo均0。原始TAP在产品忽略目录.local/delivery-t016-20261022/main-299.tap；独立公开证据在test/evidence/tester-t015-appid/。 |
| OPC当前接手点 | e03c707f44738e0ddcb90f4708cf4bd5998b7595；目标project/eating-everyday，合入前目标a735031c37c09e75fb9478681aaf42010a82f378。当前在原feature同步交付引用，待OPC快进合入、验证及远端核对。 |

17个产品功能/修复/验证提交保留原SHA，未重写阶段证据依赖的历史；新增范围无merge commit。T-010至T-015各任务的原被测SHA和专业结论继续保留，最终交付字段引用上述main提交。旧T-013阶段的291/292与292/292是修正前事实，当前配置复测以A-014 v0.4为准。

## 本机配置保留

仓库默认project.config.json仍为touristappid；合法实际AppID在本机运行配置单独校验，不跳过工程入口、基础库、事件绑定、依赖、Unicode数据或打包排除保护。公开提交中未检出本机实际AppID；私有配置及.local内容保持忽略。

切换主线前精确备份并只暂存已授权保留的公开配置。Git stash恢复时规范化了换行，SHA保护立即中止后续步骤；确认只存在换行差异后恢复原始备份，再验证后才运行目标分支测试和推送。原公开配置SHA-256始终以最终原字节为准：97f3a1355a54a3c8875f6da96580357c1fbc3a639079e009acf0aa07c6bae2db；私有配置为988e8a38615d839bbe92a89bf5b255e6ccb7be5c218ab1b8bbc71593870ccff4。本次自身stash已清理，原始备份保留于产品忽略目录，未操作其他数据。

## 当前收尾与边界

WORKSPACE.revision在产品main真实合入、验证及远端核对后更新到f642dcad，repository/branch/driver/相对checkout均未改变。OPC尚待按--ff-only合入project/eating-everyday、验证推送；全部成功后才安全清理两个开发分支。本阶段没有开始新一轮开发。

正式小程序账号由用户另行准备。手机结果仍为用户自测，设备/微信版本、实际构建指纹、真实满配额、多设备及历史GUI原始附件缺口等按A-014/A-006保留。H-005/H-006等交接由原发起人核对，不因本次Git操作自动关闭；需求与架构等成果保持draft。

## 版本记录

| 日期 | 版本 | 变化 |
|---|---|---|
| 2026-10-22 | 0.1 | 记录配置修正与独立通过、产品main合入验证和远端核对；同步最终产品版本，OPC收尾继续于T-016。 |
