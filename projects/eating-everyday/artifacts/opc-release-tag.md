# OPC项目发布标签记录

- 项目：eating-everyday（今天吃什么）；成果ID：A-017；负责人：project-manager。
- 版本：0.8，2026-11-06（Asia/Shanghai）；状态：draft；关联任务：[T-045](../TASKS.md#t-045采用产品v130并整理发布opc项目标签)、T-039、T-018、T-029、T-030、[T-031](../TASKS.md#t-031压缩opc发布记录并保护强推当前版本)。
- 当前用户来源：2026-11-06用户明确先更新WORKSPACE，再按功能压缩OPC feature并合入project/eating-everyday，按之前格式打project/eating-everyday/v1.3.0，commit message只有一行，见D-029/T-045；仅本地交付，不推送或创建GitHub Release页面。
- T-039来源（历史）：2026-11-03先要求OPC按之前逻辑整理，随后明确选择「与产品一致：保留 release merge，并打 project/eating-everyday/v1.2.0」，见D-026/T-039；当次先在本地完成。
- 初始用户来源（历史）：本次明确“把opc仓库打一个release的tag 要求有merge commit”。当前克隆以eating-everyday项目身份工作，标签发布当前项目资料；不把项目内容合入公共框架main。

## 当前release结构（v0.8）

先完成用户要求的WORKSPACE版本更新：仅revision改为产品main/v1.3.0已验证最终571802a6e4697bb5d61fd508e902dc1746c54c00，driver/仓库/main/checkout保持。单独工作区提交a0a2c947后形成计划源快照947085bd3e507da77d8c94da7c0f89ca1621a820；其15条包含原feature的13条过程及本轮2条准备。随后整理为5组，最终树72a9d92e1351fb70f56ca826fe71cd27f0f8b7c9严格等于源快照，无专业正文或历史证据损失。

| 功能分组 | 整理后提交与范围 |
|---|---|
| 范围/决定/协调记录 | d6c1bd6d5342a90b51a9f4b6bf06f5c61882e184；DECISIONS、PROJECT、STATUS、TASKS及共享索引，保留原跨成员事实。 |
| 研发交接 | 126e63035deb3eae2f8f460ac25b12cb0159980f；A-013原最终正文。 |
| 独立报告 | 6b0eb8572b97b524bf62d95e5790857fb9857390；A-014原最终正文及原失败/复测结论。 |
| 产品发布记录 | 17fe058576e6c4b42d4da248aef3d8176d060ea0；A-018产品v1.3.0交付。 |
| 工作区采用 | 1506c020dd104dced5abec24e335c2dc172481f0；WORKSPACE锁定已验证产品最终版本。 |

5组均为单行线性提交，原树核验后--ff-only合入project/eating-everyday，再进行本次完整发布准备；原共享项目主线1970f68之前的历史不改写。原feature ef48fd5及受评/交付过程SHA由before.bundle/source.bundle保留，不误认新主线祖先，也没有强推共享feature。

| 当前发布约束 | 定位及核验 |
|---|---|
| 标签名称/说明 | 附注project/eating-everyday/v1.3.0，说明单行Release project/eating-everyday/v1.3.0。 |
| 第一父 | 1506c020dd104dced5abec24e335c2dc172481f0，5组已快进合入的功能头。 |
| 第二父 | 当前feature/eating-everyday/t-040-share的完整发布准备提交，含T-045及A-017/A-018与当前摘要；实际SHA由最终merge的第二父及本机日志定位。 |
| release | 真正--no-ff双父merge，message单行release(eating-everyday): project/eating-everyday/v1.3.0；不在本提交正文中自写最终SHA。 |
| 树与最终引用 | release树等于完整准备树；附注tag内部名称/对象/剥离目标正确，目标等于最终项目主线HEAD；tag之后不追加治理commit。 |
| 产品/旧refs | 产品本机/实时远端main及v1.3.0已核对571802a，本次仅只读核对；626受控文件/两配置/所有引用、六份OPC专业正文、旧tag及框架线保持。 |

工作区/功能整合前后结构、链接、diff和原树一致检查已通过；最终发布与准备版本范围、两父/树/tag、单行message及旧引用/产品保护结果保存于.git/codex-backups/t045-opc-v1.3.0-20261106/release-verification.json与final-verification.json。恢复点还有已验证before.bundle/source.bundle、before/source.json及functional-build/functional-verification/integrated-main.json。本轮只做治理/Git，沿用产品原671/671及原实体设备/外发限制，不重复执行或冒称新测试通过。

本次发布--no-ff为用户D-029明确例外，不修改公共策略或把项目资料合入框架main。所有本次回执均先进入准备版本，最终项目HEAD/tag从Git原文核对；不推送远端、不删除分支、不创建GitHub Release或执行微信上传提审发布。原.tmp/、两个feature和不可访问旧C盘域的历史未知范围保持；下文为旧发布时点，不能覆盖本轮工作区采用和v1.3.0本地结构。

## v0.7 release结构（历史）

本轮先将远端已发布2e241e6b489a65701ba2078640b2794cdabba429之后尚未推送的9条记录整理为需求/决定、架构、实现与独立验证、交付及产品发布记录、WORKSPACE采用5组单行功能提交。整理后主线为70c0d37792bd22494a4c67766ac8e858a0f5424d，文件树仍38ba31c0adccf30d64165ae10167b1a26fe2f510，与原e4d3f7a5518233a8ae366ee427ed76d382a1a931完全一致；原9条及旧受评SHA由完整before.bundle保留，不误认新主线祖先。

| 当前发布约束 | 记录与核验方式 |
|---|---|
| 发布名称 | 新附注project/eating-everyday/v1.2.0，仅本地，不移动旧tag。 |
| 第一父提交 | 70c0d37792bd22494a4c67766ac8e858a0f5424d，为整理后的项目主线。 |
| 第二父提交 | feature/eating-everyday/t-039-opc-release-v1-2-0的完整发布准备版本，恰为第一父之后一条本次说明提交；发布后从merge父列表及本机日志核对实际SHA。 |
| release commit | 使用--no-ff创建恰有两个不同父的真正merge；message单行release(eating-everyday): project/eating-everyday/v1.2.0。 |
| 树与tag | release树等于准备版本树；附注tag内部名称/单行说明正确，剥离目标等于项目主线最新HEAD，所有本次治理回执都在准备版本中，tag后不追加commit。 |
| 产品与WORKSPACE | 仍锁定已远端同步产品main/v1.2.0的38b1e69350ad2b4cf43c3124a3635f4a0b9c47a0，产品仓库及两配置只读不变；六份专业正文逐blob保持，不重跑未变609项。 |
| 旧引用和远端 | 已发布project/eating-everyday/v1.1.0仍指向2e241e6，v1.0.0及框架/其他项目全部引用保持；本次不推送，未来项目主线仍可从原2e241e6正常快进。 |

恢复和证据位于.git/codex-backups/t039-opc-release-v1.2.0-20261103/：before.bundle已验证，before.json固定原引用、专业正文、WORKSPACE及产品只读指纹；functional-build/functional-verification记录5组实际SHA和树等价，registered-task.patch保存登记改动。首次本地update-ref事务因Windows文本换行解析失败且未改引用，明确LF字节重试后事务通过，已核对登记改动保持。发布双父、准备树/tag对象和旧引用/产品保护/安全清理结果以release-verification.json及final-verification.json和Git引用为准，不在同一提交写自身SHA。

本次项目发布merge为用户明确例外，不修改公共框架策略或把项目资料合入框架main。所有新message和tag说明均单行；本轮只进行OPC治理/Git操作，无新增业务实现、独立测试或微信发布，产品已通过的609及实体手机等限制保留原范围。下述v0.6及更早为历史阶段，不描述当前发布结构。

## 2026-11-02单父release整理（v0.6历史）

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
| 2026-11-03 | 0.7 | D-026/T-039将9条未推送记录按5组功能整理，保持原树，再按用户选择创建真正双父release及附注project/eating-everyday/v1.2.0；旧tag、产品和WORKSPACE保持，仅本地，tag固定最新项目主线。 |
| 2026-11-06 | 0.8 | D-029/T-045先采用产品main/v1.3.0最终571802a，再将原13条及工作区/计划准备按5组整理并保持源树，快进项目主线，以完整准备和功能头构成双父release/附注project/eating-everyday/v1.3.0；message/说明单行，产品/专业正文/旧refs保持，仅本地，不在tag后追加提交。 |
