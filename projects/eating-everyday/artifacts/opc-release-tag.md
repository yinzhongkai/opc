# OPC项目发布标签记录

- 项目：eating-everyday（今天吃什么）；成果ID：A-017；负责人：project-manager。
- 版本：0.6，2026-11-02（Asia/Shanghai）；状态：draft；关联任务：T-018、T-029、T-030、[T-031](../TASKS.md#t-031压缩opc发布记录并保护强推当前版本)。
- 用户来源：本次明确“把opc仓库打一个release的tag 要求有merge commit”。当前克隆以eating-everyday项目身份工作，标签发布当前项目资料；不把项目内容合入公共框架main。

## 当前release结构（v0.6）

用户明确要求把45b72e4e9d63741a7562794a35871d2280735d08和8aa7e59dbc6f234151ae1d60af929664274d042c合并，允许强推，并指定release message格式参考d4e6598；授权见D-023/T-031。本次将两个相邻节点及本次必要历史说明压为一条普通release提交，唯一父为a4dc4a2a0ece4e88250fd17523b5244b352ecbf7，message为单行release(eating-everyday): project/eating-everyday/v1.1.0。

项目主线与附注project/eating-everyday/v1.1.0同时指向整理后的release提交；新SHA及tag对象由Git引用解析核对，不在同一提交正文自写。原45b72e4/8aa7e59不再作为当前主线/tag的祖先，旧发布内容与专业正文保留；本次仅同步必要当前说明，所有记录一并纳入，不在新release之后追加治理提交。

先完整保存旧主线/tag和两个旧节点，再在来源分支rebase/fixup，核对新树与准备树一致。按预期旧SHA更新本机主线与同名附注tag，使用旧远端头8aa7e59及旧tag对象5b7d7de2438ef87e15665fccd85f77b8a7f1087c的显式lease原子强推；远端变化即停止。before.bundle、旧附注、准备/验证/推送及最终读回均保存在.git/codex-backups/t031-opc-release-squash-20261102/。

仅本项目v1.1.0及项目主线受本次明确历史改写授权影响。OPC v1.0.0仍固定d4e6598，框架及其他标签不变；产品main/v1.1.0和WORKSPACE仍为3800e6d。本次验证结构/链接、diff、唯一父提交/单行message、旧节点非祖先、范围/专业正文及配置保护，不重跑未变产品测试，不创建GitHub Release页面。

## 2026-11-02更名记录（v0.5历史）

用户在T-030连续要求将OPC及产品的v1.0.1标签更名为v1.1.0，确认见D-022。本机和远端现有附注project/eating-everyday/v1.1.0，仍指向原发布快照45b72e4e9d63741a7562794a35871d2280735d08；旧project/eating-everyday/v1.0.1名称两边均已删除。project/eating-everyday/v1.0.0仍为d4e6598，其他OPC标签/框架main不变。

更名只改Git标签名称与附注内部名称，不移动目标或改写发布提交。新名称及旧名删除已原子推送，实际远端读回的新对象/目标与本机相同，所有无关引用不变。原tag对象d8745fbb1162a267e7b5ce03d96419e41ed11e55及完整历史已备份至.git/codex-backups/t030-version-tag-rename-20261102/，tag-verification.json保存实际新对象与前后引用。

本段属于更名后的后续治理回执，项目主线可因本回执前进；发布标签保持原45b72e4，不移动标签来纳入回执。WORKSPACE仍锁定产品3800e6d，产品新v1.1.0同样保留原3800e6d。回执通过T-030功能分支快进纳入/同步项目主线，最终结果见同目录final-verification.json；不创建GitHub Release页面或修改发布内容。

## 2026-11-02标签创建（v0.4历史）

用户在2026-11-02明确将project/eating-everyday/v0.1.0更名为project/eating-everyday/v1.0.0，同时新增project/eating-everyday/v1.0.1，授权及范围见[D-021](../DECISIONS.md#d-021opc项目标签更名并新增v101)。本次只调整本项目标签和必要治理记录；产品main/v1.0.1及WORKSPACE仍为3800e6dd8bc32b7d767bfc873a4b9ae57d10232b，框架main/其他标签不变。

| 标签 | 当前定位规则 |
|---|---|
| project/eating-everyday/v1.0.0 | 由旧v0.1.0更名，附注标签，保留原发布目标d4e65980fb30d4fe9a65248c277861823402eb9a；不纳入后续默认菜单资料。 |
| project/eating-everyday/v1.0.1 | 附注标签；本次全部治理记录先快进合入project/eating-everyday，再固定最终本机/远端项目主线HEAD。最终SHA由Git引用解析核对，不在同一提交自写自身哈希。 |
| project/eating-everyday/v0.1.0 | 旧名称从本机/远端移除；旧对象251e3b6cf04464cbc21aa24b57cf441b05ed2048及完整历史已备份。 |

本轮记录通过feature/eating-everyday/t-029-opc-version-tags形成单行普通治理提交，以--ff-only合入项目主线；tag说明均为单行。新tag创建后不追加治理commit，避免标签落在本次回执之前。已共享历史不改写，旧版本目标不移动。

项目主线、两个新tag及旧tag删除使用显式refspec和--atomic同步，各引用由预期旧头或新tag必须不存在的lease保护；核对项目主线为快进。最终实际提交、标签对象/剥离目标、其他引用保护及远端读回以.git/codex-backups/t029-opc-tags-20261102/final-verification.json和Git为准，验证后安全删除已合入的本轮feature。原恢复点before.bundle已通过git bundle verify，旧附注原文另存old.tag。

检查覆盖项目结构/本地链接、diff、目录边界、单行message、附注内部名称与目标一致、WORKSPACE和产品只读不变、其他标签/框架main/原.tmp保护。本次不重跑未变产品测试，不新增专业验收；按D-018不创建GitHub Release页面。

以下v0.1～v0.3为2026-10-22的原执行历史，其旧标签名称和“当前发布”仅在各自时点适用，现以本节及T-029为准。

## 2026-10-22发布结构（v0.3历史）

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
| 2026-11-02 | 0.4 | 按D-021将旧tag更名为project/eating-everyday/v1.0.0并保持原目标；全部记录先合入主线，再创建project/eating-everyday/v1.0.1固定最终HEAD，同步远端/旧名删除并保存实际引用校验。 |
| 2026-11-02 | 0.5 | 按D-022/T-030将project/eating-everyday/v1.0.1更名为v1.1.0并同步远端，保留原45b72e4目标；更名回执为后续治理提交，不纳入既有发布快照。 |
| 2026-11-02 | 0.6 | 按D-023/T-031将45b72e4和8aa7e59合为单条release，采用d4e6598同格式单行message，项目主线/tag同步指向新提交并按旧对象保护强推。 |
