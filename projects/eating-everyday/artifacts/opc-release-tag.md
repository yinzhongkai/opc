# OPC项目发布标签记录

- 项目：eating-everyday（今天吃什么）；成果ID：A-017；负责人：project-manager。
- 版本：0.3，2026-10-22（Asia/Shanghai）；状态：draft；关联任务：[T-018](../TASKS.md#t-018为opc当前项目快照创建发布merge与标签)。
- 用户来源：本次明确“把opc仓库打一个release的tag 要求有merge commit”。当前克隆以eating-everyday项目身份工作，标签发布当前项目资料；不把项目内容合入公共框架main。

## 当前发布结构（v0.3）

用户再次明确要求把fb7d4e2d2a1d06208540c0ee34f8e2f48da0cc26和17c859ea2dafebe99503e46e595ab391649bcf05合入相邻提交，不作为独立节点保留在Git历史。本次据此整理8012e2beb4ead9b19259ddf7b10d06366d58103b之后的五条发布提交：旧发布merge与无文件变化的准备提交不再保留，回执及命名修正合为一条完整资料提交，再生成最终双父发布merge。

最终发布merge第一父为8012e2beb4ead9b19259ddf7b10d06366d58103b，第二父为合并后的资料提交；第一父之前的交付历史保持。当前附注标签仍为project/eating-everyday/v0.1.0，其剥离目标必须等于本机及GitHub项目主线最新HEAD，恰为上述双父发布merge。当前资料提交保留原全部项目内容，仅同步本次历史整合和当前拓扑说明；产品、WORKSPACE及公共框架不改。最终提交以Git引用解析核对，不在自身正文写自身哈希；发布后不追加commit。

旧头、旧附注tag及使旧发布历史可见的本机恢复引用已保存到.git/codex-backups/t018-history-squash-20261022/before-squash.bundle并校验。新项目分支和tag仅以显式旧远端头及旧tag对象保护的--force-with-lease原子同步，远端变化即停止；同步验证后，仅清理这些已入bundle的过时本机恢复引用，不新增远端归档标签以重新暴露旧提交。保留bundle和Git自身恢复信息，不宣称永久抹除对象。实际结果以分支/tag祖先检查、远端读回和会话回执为准。

以下v0.1/v0.2包含的旧SHA、首次发布及命名修正记录仅为原执行时点历史；旧独立提交已由本次用户明确要求替代，不再用其父提交描述当前发布结构。

## 标签命名修正（v0.2历史）

用户随后明确要求“把tag命名成 project/eating-everyday/v0.1.0，另外该tag应该批向最新的提交”。本次以该明确授权替代旧名eating-everyday/v0.1.0，以及旧标签停留在回执之前的安排；旧标签对象3e86c7f32c97a2ba7e611fea4eb4ed73d1001966和原项目最新头fb7d4e2d2a1d06208540c0ee34f8e2f48da0cc26已先在本机恢复引用及.git/codex-backups/t018-opc-tag-correction-20261022/before-correction.bundle保留。

当前发布名为[project/eating-everyday/v0.1.0](https://github.com/yinzhongkai/opc/tree/project/eating-everyday/v0.1.0)。发布必须同时满足：该附注标签剥离后等于本机及GitHub项目主线project/eating-everyday的最新HEAD；该HEAD是具有两个不同父提交的发布merge，包含本次所有记录。第一父固定修正前最新主线fb7d4e2d2a1d06208540c0ee34f8e2f48da0cc26，第二父为feature/eating-everyday/t-018-opc-tag-correction的完整资料修正提交；merge文件树与该准备提交完全一致，仅改本项目当前发布记录。最终merge哈希通过标签及项目主线解析核对，遵守不在同一提交正文自写自身哈希的约定。

全部当前资料和回执先写入准备提交，再使用--no-ff形成最终发布merge并创建新附注标签；之后不再追加任何治理commit。新标签、项目主线更新及旧标签删除作为一次原子推送，分别使用已记录旧项目头、旧tag对象和新tag必须不存在的显式lease保护，远端变化立即停止。实际同步结果以本机/远端HEAD、tag对象及剥离目标检查和会话回执为准；完成后新标签固定最新发布点，旧名称从本机与GitHub删除。用户本次明确的命名与目标修正授权仅适用于该OPC项目标签，产品v0.1.0、框架main及既有框架版本标签保持。

本次版本修正的before.json、发布结构/校验日志及最终远端核对保存在.git/codex-backups/t018-opc-tag-correction-20261022/，不因补记再新增文件提交。以下v0.1表格和“标签后的回执”仅为首次操作的历史，不再描述当前标签名称与目标。

## 首次发布点（v0.1历史）

| 项目 | 实际结果 |
|---|---|
| OPC仓库/项目主线 | git@github.com:yinzhongkai/opc.git；project/eating-everyday。 |
| 发布前主线 | 8012e2beb4ead9b19259ddf7b10d06366d58103b。 |
| 发布准备提交 | eed44d0a15635cb378ef723b91bbfad453399c1d，来自feature/eating-everyday/t-018-opc-release-v0-1-0；只记录发布准备，文件树不变。 |
| 发布merge | [17c859ea2dafebe99503e46e595ab391649bcf05](https://github.com/yinzhongkai/opc/commit/17c859ea2dafebe99503e46e595ab391649bcf05)，message为`release(eating-everyday): v0.1.0 OPC snapshot`，只有一行。 |
| 两个父提交 | 第一父8012e2beb4ead9b19259ddf7b10d06366d58103b；第二父eed44d0a15635cb378ef723b91bbfad453399c1d；恰好两个不同父提交。 |
| 附注标签 | [eating-everyday/v0.1.0](https://github.com/yinzhongkai/opc/tree/eating-everyday/v0.1.0)，tag对象3e86c7f32c97a2ba7e611fea4eb4ed73d1001966，剥离后等于发布merge。 |
| 文件树 | 7d58278e5a1bdf80fd3649d8a4c9a7a227999d17；发布merge、准备提交和原主线8012e2b完全相同，发布前后文件差异为空。 |
| 产品锁定版本 | WORKSPACE继续指向6c9ef2b3135a0013fb0b06ebbe761b298c4b7260；产品main/v0.1.0及配置未改变。 |

现有v0.0.1～v0.0.4属于框架版本，因此当前项目使用命名空间eating-everyday/v0.1.0。按用户本次明确要求，项目发布以--no-ff保留实际merge；未改变根公共策略。框架main继续464ca52aafc6c73ff780a7120acd3bf63c9d31dd，四个现存框架版本tag对象已逐一核对与操作前本机/远端一致，没有将项目内容混入框架稳定分支。

## 实际验证和远端确认

在发布merge上执行`python -X utf8 -B scripts/validate_framework.py`，结构及590处本地链接通过；工具只读，不代表业务验收。发布前后`git diff --check`通过，两个父提交、相同tree及附注tag类型均经实际Git检查。

项目主线及新标签通过`git push --atomic`同步GitHub；实际读回项目主线=17c859ea2dafebe99503e46e595ab391649bcf05、tag对象=3e86c7f32c97a2ba7e611fea4eb4ed73d1001966、标签剥离目标=17c859ea2dafebe99503e46e595ab391649bcf05，已与本机核对一致，未强推。原始release.json、remote-verified.json、snapshot-validation.txt在本机`.git/codex-backups/t018-opc-release-v0.1.0/`保留。标签创建不代表GitHub Release页面已经创建，也不表示完成原README更新、最终独立复核或微信正式发布。

## 标签后的回执

本记录及T-018是在标签真实推送、核对成功后补记，因此不在已冻结标签文件树中。回执在原本轮分支上从发布merge继续，以普通治理提交快进同步project/eating-everyday、校验推送并安全清理本轮分支；项目主线可因回执前进，eating-everyday/v0.1.0固定指向上述merge且不移动。回执最终版本通过Git历史及远端项目主线核对定位，不在正文写自身哈希。

本次未修改任何产品文件或WORKSPACE声明，未唤醒developer/tester，未重跑或代写产品专业测试。产品v0.1.0和本项目标签均已冻结，后续文档或业务变更需形成新的提交，不通过移动标签纳入。原用户要求的README/发布说明、新最终独立复核与GitHub Release页面工作仍保留为后续，正式小程序账号由用户准备。

## 版本记录

| 日期 | 版本 | 变化 |
|---|---|---|
| 2026-10-22 | 0.1 | 据实际Git结果记录OPC项目发布merge、附注标签、590处链接校验与原子推送核对，保留框架/产品不变范围，并补记标签后的交付回执。 |
| 2026-10-22 | 0.2 | 按用户明确要求修正为project/eating-everyday/v0.1.0；全部回执先提交，标签固定项目主线最新发布merge，原子同步新标签/主线并按旧对象保护删除旧名，最终结果由Git引用核对。 |
| 2026-10-22 | 0.3 | 按用户要求折叠旧发布merge与回执，五条整理为一条资料提交和最终双父release merge；保留全部内容并同步当前拓扑，tag仍等于最新HEAD，旧历史仅留本机bundle恢复。 |
