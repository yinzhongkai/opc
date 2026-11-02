# 产品标签更名与v1.0.1发布记录

- 项目：eating-everyday（今天吃什么）；成果ID：A-018；负责人：project-manager。
- 版本：0.3，2026-11-02（Asia/Shanghai）；状态：draft；关联任务：[T-027](../TASKS.md#t-027调整产品标签并创建v101发布merge)、[T-028](../TASKS.md#t-028删除产品功能分支并同步主线及标签到远端)、[T-024](../TASKS.md#t-024按功能整理并合入opc默认菜单记录)。
- 来源及批准边界：D-020及T-028的产品标签/发布/远端同步已完成。用户随后恢复OPC并要求先核对WORKSPACE，T-024已采用本次最终产品版本并快进合入项目主线。成果仍draft，无整份批准或微信正式发布结论。
- 存储位置：治理记录在OPC；产品仓库git@github.com:yinzhongkai/eating-everyday.git，本机checkout=../eating-everyday。

## 标签与提交

| 项目 | 已核对结果 |
|---|---|
| 旧发布提交 | 6c9ef2b3135a0013fb0b06ebbe761b298c4b7260，不移动。 |
| 标签更名（本机/远端） | v0.1.0 → v1.0.0；两边旧名称引用均已移除，新名称为附注标签，目标仍为上述旧发布提交。 |
| 旧标签对象 | d12b4b9c30f198400ca6fa15d2a36b4b19c94ef7，原始对象与完整历史已备份。 |
| v1.0.0标签对象 | c9b959b9fe77931109688e4de0bf8af89ce0243b，内部tag名称及单行说明Release v1.0.0已核对。 |
| 发布前main | 8ad2ae35f8a9da39c1c9202c072b9045c9edc4b4，已包含默认菜单及正式AppID。 |
| 发布准备/source | a63531b35e462f3f815529a786ac067891ecb7f0，原feature/default-menu；只更新README标签及发布说明。该分支现已安全删除，提交由发布merge第二父留存。 |
| 发布merge/main | 3800e6dd8bc32b7d767bfc873a4b9ae57d10232b；单行message为release(T-027): v1.0.1。 |
| 第一父提交 | 8ad2ae35f8a9da39c1c9202c072b9045c9edc4b4，即发布前main。 |
| 第二父提交 | a63531b35e462f3f815529a786ac067891ecb7f0，即发布准备版本；两个父提交不同。 |
| v1.0.1标签对象 | 43b32337887b8a44a55739e22d4415f011ee523e；附注、内部名称及单行说明Release v1.0.1已核对，剥离目标等于发布merge/main。 |
| 发布树 | cacb8fac4dd779add01690415bbf00b29802306d；发布merge与准备版本完全一致，相比发布前main仅README变化。 |

按用户明确要求使用git merge --no-ff，作为本次产品发布对普通feature快进规则的例外，不改变公共框架策略。准备提交和发布merge的message均只有一行，没有改写已合入的main历史。v1.0.0只更名、不纳入后续功能；v1.0.1固定当前发布树。

## 验证与恢复点

Windows、Node v24.22.1。在发布merge/main实际执行node --test --test-reporter=tap test/*.test.js test/independent/*.test.js，501/501，失败、取消、跳过、todo均0；git diff --check通过。

本轮未改实现、测试、package版本、公共/私有工程配置或历史证据；仅README说明与Git发布元数据变化。两个配置原始SHA-256与操作前一致，产品工作树干净。没有重新运行微信工具、访问菜单域或新增手机/物理故障验收；T-023独立结论与限制保留。

产品本机.git/codex-backups/t027-release-v1.0.1-20261102/保存original.bundle（git bundle verify通过）、原v0.1.0.tag、before/prepared/release-before-validation/final-verification.json和release-main.tap。旧tag名称/对象可从该完整bundle恢复，不依赖已删除的本机引用。

## 远端及OPC边界

T-027时仅本机完成，远端main为6c9ef2b，v0.1.0对象为d12b4b9，两个新tag不存在。随后用户在T-028明确要求删除feature及同步远端，已用git branch -d删除已合入的本机feature；远端操作前后均不存在feature。

T-028使用显式refspec及--atomic同步main、两个新tag及旧tag删除，分别用预期旧头/不存在新标签的lease保护。main从6c9ef2b快进到3800e6d，无提交历史改写；推送退出0。实际读回远端全部heads/tags：main=3800e6dd8bc32b7d767bfc873a4b9ae57d10232b；两个tag对象及剥离目标与上表完全一致，v0.1.0不存在。产品本机main/origin/main一致，仅保留main分支，工作树干净。完整bundle、前后引用、推送命令/输出及验证报告保存于产品.git/codex-backups/t028-sync-remote-20261102/；本次未改产品文件、配置、发布merge或tag对象，沿用同一版本的501/501，不重复执行未变测试。

用户明确恢复OPC后，WORKSPACE.revision已从旧6c9ef2b推进到3800e6dd8bc32b7d767bfc873a4b9ae57d10232b，地址/驱动/main/相对checkout保持，实际产品本机和远端再次核对一致。OPC按功能整理后以--ff-only合入project/eating-everyday，合入前后结构/链接、目录边界、专业正文、无新增merge及版本引用检查通过；最终回执继续快进纳入，提交均单行。OPC自身最终版本由任务、来源分支、合入前目标8474050e及Git历史定位，不在同一提交自写哈希。

项目主线同步及已合入功能分支安全清理按T-024交付步骤执行，最终本机/远端引用与清理校验保存在OPC本机.git/codex-backups/t024-final-integration-20261102/final-verification.json；原历史和tag已备份为同目录完整original.bundle。OPC project/eating-everyday/v0.1.0、框架main及全部标签保持，不新增发布或启动下一轮。

按D-018不创建GitHub Release页面；未执行微信上传、提审或正式发布。

## 版本记录

| 日期 | 版本 | 变化 |
|---|---|---|
| 2026-11-02 | 0.1 | 记录本地标签更名、v1.0.1双父发布merge、单行message、501项main复验及未推送/OPC暂缓状态。 |
| 2026-11-02 | 0.2 | 按追加授权安全删除产品feature，原子同步main/全部tag及旧tag删除，实际读回对象/目标一致；产品干净，OPC仍暂缓。 |
| 2026-11-02 | 0.3 | 用户恢复OPC，WORKSPACE锁定产品最终发布，按功能整理/快进合入及目标检查完成，登记最终交付的引用和清理验证位置。 |
