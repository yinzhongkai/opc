# 产品发布与标签更名记录

- 项目：eating-everyday（今天吃什么）；成果ID：A-018；负责人：project-manager。
- 版本：0.9，2026-11-06（Asia/Shanghai）；状态：draft；关联任务：[T-045](../TASKS.md#t-045采用产品v130并整理发布opc项目标签)、[T-044](../TASKS.md#t-044整理产品分享功能并创建v130发布与标签)、T-038、T-037、T-027、T-028、T-024、T-030、[T-031](../TASKS.md#t-031压缩opc发布记录并保护强推当前版本)。
- 当前授权及边界：D-029/T-045已将WORKSPACE采用产品main/v1.3.0最终571802a，并本地整理发布OPC对应项目版本；本轮产品只读，不修改代码/配置/引用或重跑未变671项。本次实际核对产品远端main及v1.3.0也为571802a，非本任务执行的推送；OPC本地发布见A-017 v0.8。
- T-044授权时点（历史）：D-028/T-044按用户明确要求先将产品feature按功能压缩、合入main，并按之前格式创建v1.3.0；当次仅本地产品交付，OPC feature保存必要回执，WORKSPACE暂留旧基线待后续采用，不推送或删除分支。
- T-038同步时点授权及边界（历史）：2026-11-03用户明确将产品main和v1.2.0同步远端，T-038已原子完成并读回；当时未授权OPC远端同步、GitHub Release页面或微信发布。
- T-037准备时点授权及边界：D-025/T-037沿用T-027产品双父发布merge和附注tag，v1.2.0为本轮功能增量默认命名，非用户逐字指定号码；仅本地Git发布，未推送或微信发布。成果仍draft。
- 历史来源及批准边界：D-020及T-028的产品标签/发布/远端同步已完成。用户随后恢复OPC并要求先核对WORKSPACE，T-024已采用本次最终产品版本并快进合入项目主线。成果仍draft，无整份批准或微信正式发布结论。
- 存储位置：治理记录在OPC；产品仓库git@github.com:yinzhongkai/eating-everyday.git，本机checkout=../eating-everyday。

## T-045工作区采用与当前产品核对

用户后续明确先更新WORKSPACE并整理发布OPC。已只改revision从38b1e693到571802a6e4697bb5d61fd508e902dc1746c54c00，实际本机产品main/v1.3.0及实时远端同目标，附注对象仍f1ae71c4cb3383533cbc339becfb2e519b05f123。产品626受控文件、两份本机配置和所有引用按T-045 before.json逐项保持，不改变产品发布或专业结论。OPC将相关治理记录按功能整理、快进项目主线，再以完整准备创建本地双父project/eating-everyday/v1.3.0，具体结构/恢复点见A-017 v0.8和T-045日志。

产品README和下文T-044的“仅本地/暂缓WORKSPACE”文字保留其原执行时点；当前采用和产品远端事实以上段为准。本次没有产品推送、代码更改、微信或实体机验收，也没有重跑已通过且未变的671项；只完成用户本次授权的工作区版本采用和OPC本地交付。

## v1.3.0本地功能整理与发布（T-044历史）

2026-11-06用户明确先处理产品feature，将commit按功能压缩后合入main，再打v1.3.0，格式沿用之前且commit message只一行。T-044在现有分享配对feature上收尾，不开新业务轮次；四名成员均已核对停止。发布前本机、origin及实时远端main均38b1e69350ad2b4cf43c3124a3635f4a0b9c47a0，原feature及实时远端为fdf64604db125569efd8e131f46c29922fa9a5d8，远端v1.3.0不存在。

### 按功能整理与原始版本恢复

| 分组 | 整理后提交 | 范围 |
|---|---|---|
| 分享实现与开发回归 | 29a07826b9f2fe4792ab68c409477af12d77343e | 8路径：页面、静态图、ui/share-images.js、两份开发测试及README/分享说明；最终实现blob不改。 |
| 开发验证证据 | dda797143ec8e98aa71438e3fdb62bd932423050 | 107份developer-t041分享与图片修复证据，保留原失败/对照/平台输出。 |
| 独立验证与证据 | 167951a83f522582c5a2103a31f7aadccafd0402 | 140路径：两份独立源码及138份tester-t042原始/续测/复测资产。 |

原7条整理为上述3条线性提交。第三条tree为6c083977dbf345e13beeaf12c8b3abeb776a18bb，与原fdf6460严格相同；原实现、原始证据及失败历史没有为整理而更改。原受测a647d1f及过程SHA继续作为对应历史时点引用，不冒称整理后main祖先。修改本地feature引用前已验证完整before.bundle，含原main、feature及旧tag可达历史；远端feature未改写，无强推。功能整理通过后使用--ff-only合入main。

### 发布提交与附注标签

| 项目 | 实际结果 |
|---|---|
| 发布第一父/功能合入后main | 167951a83f522582c5a2103a31f7aadccafd0402。 |
| README准备/第二父 | 93ed96beab2ce722289387c8c4dd924e9f05cc82，源feature/t-040-share，只增README当前v1.3.0说明并保留旧发布历史。 |
| release/main | 571802a6e4697bb5d61fd508e902dc1746c54c00；恰有上述两个不同父，message为release(eating-everyday): v1.3.0。 |
| 发布树 | 5be2e88a700d3c9bad9e60587eae519d395b36e2，与README准备版本相同；相对原fdf6460仅README变化。 |
| 附注v1.3.0 | 对象f1ae71c4cb3383533cbc339becfb2e519b05f123；内部tag名称v1.3.0，说明Release v1.3.0，剥离目标等于release/main。 |

新增范围共3条功能、1条README准备和1条发布merge，message全部单行，tag说明也单行。--no-ff仅用于本次沿用的发布节点，第一父固定为已经快进合入的功能头，不改变普通功能合并规则。原main/v1.2.0以前的共享历史未改写，v1.0.0/v1.1.0/v1.2.0对象及目标保持。

### 验证、配置与边界

Node v24.22.1，完整命令node --test --test-reporter=tap test/*.test.js test/independent/*.test.js。整理后的工作树、无.git/私有配置干净导出、功能合入main、release/main分别671/671，fail/cancel/skip/todo均0；这是同一套用例在规定阶段的验证，不能累加。diff检查、原树一致、两父/发布树、单行message及附注对象/指向通过。此次没有重新操作微信或补实体机验收，A-013 v0.8/A-014 v0.11保留原专业结论和未覆盖边界。

当前公共配置原始SHA-256为eecc59987ea459258c2bf0329b69d98e73e790dfa6110d60f28ed3645456e645，私有为a5312f8c43cbae297edf96440cd70ce5fd95e3c3b945a2aa016c2fafde142fc0，整理、切换、合入和发布前后原字节相同；公共M只是已协调CRLF表示差异，过滤blob始终c2c297e3db63062e72a6ef0476e02a611914dbba。未格式化或提交配置，不宣称工作树完全无M；package不改。

产品.git/codex-backups/t044-product-v1.3.0-20261106/保存before.bundle、before.json、原两配置、functional-build.json、functional-verification.json、integrated-main.json、prepared.json、release-verification.json及四阶段原始TAP。OPC对应.git目录保存本次登记patch、专业正文/WORKSPACE原指纹和最终校验。原受评及资产SHA可从bundle恢复，不依赖改写后的本地feature或创建新的远端归档tag。

本次仅本地产品main/v1.3.0；远端main/feature及旧tag保持，无推送、分支删除、GitHub Release或微信上传提审发布。产品当前checkout已为571802a；按用户“先产品”安排，OPC只在现有feature保存治理回执，不合项目主线、不推进WORKSPACE旧锁定38b1e69、不打OPC新标签。后续采用须记录该实际产品版本，不能用旧WORKSPACE或另一克隆旧长期分支说明否认本次本地交付。两feature均保留，等待用户后续安排。

## v1.2.0远端同步（历史）

T-038依据用户明确指令，仅以两个显式refspec同步产品main与v1.2.0。推送前main为3800e6dd8bc32b7d767bfc873a4b9ae57d10232b，新tag不存在；已验证其为本机main祖先。使用--atomic及明确旧main/新tag不存在的lease保护，禁用followTags，实际main快进3800e6d..38b1e69、新tag首次创建、退出0，没有历史改写。

读回全部远端heads/tags：main及v1.2.0目标均38b1e69350ad2b4cf43c3124a3635f4a0b9c47a0，附注对象fadaed19cb4261f76dcb73f369d97b5c1b97c596，与本机一致；其他引用包括v1.0.0/v1.1.0原对象/目标不变。本机origin/main已同步，产品工作树干净、文件/公共和私有配置及本机标签均不改，沿用T-037同一发布版本609/609，不重复测试。

恢复与实证位于产品.git/codex-backups/t038-sync-product-v1.2.0-20261103/：before.bundle已验证，before.json保留原远端快照，push-result.json保存精确命令/输出，remote-verification.json保存全量读回和保护检查。OPC回执仅本地快进及安全清理，WORKSPACE已为同一SHA无需再改，不推OPC或打OPC新tag。当前正文及产品README中的T-037“未推送”说明限准备时点，以下保留历史，不代表T-038仍待执行；不再改发布内容以免tag和main偏离。

## v1.2.0本地发布准备（T-037历史）

| 项目 | 实际结果 |
|---|---|
| 发布前main/第一父 | 84db035b85a30e8825273f5ec7ebd2247ddd03c7，T-036已验证的启动恢复交付。 |
| README准备/第二父 | 23b8d52a2d14971efec201ab61189108be39a52d，源feature/t-037-release-v1-2-0；只更新README发布说明。 |
| release merge/main | 38b1e69350ad2b4cf43c3124a3635f4a0b9c47a0；恰有上述两个不同父提交，message单行为release(eating-everyday): v1.2.0。 |
| 新附注标签 | v1.2.0，对象fadaed19cb4261f76dcb73f369d97b5c1b97c596，剥离目标等于release/main，单行说明Release v1.2.0。 |
| 发布树及内容 | 1e8714955ba20e2fa90e067cc3f379c88adaf36c，与README准备版本树相同；相对发布前main仅README变化。 |
| 发布验证 | 新release/main实际609/609，失败/取消/跳过/todo均0；diff通过，原实现/测试/证据、package和公共/私有配置不变。 |
| 旧标签及范围 | v1.0.0/v1.1.0原对象及目标保持；不重写main历史，不新增OPC项目tag，不创建GitHub Release页面或执行微信上传发布。 |
| 本地/远端 | 本机main及新v1.2.0已建立；未推送，远端main仍为发布前已同步3800e6d，新tag在远端尚不存在。 |

本次--no-ff仅为用户「按之前的提交逻辑」明确沿用的产品发布例外，OPC记录及WORKSPACE仍按普通--ff-only采用新实际发布SHA。独立通过与限制继续按A-014 v0.8，未新增实体手机、实体离线或物理故障结论；本次609是发布版本的桌面复验，不冒称新的独立平台或微信验收。

两仓库.git/codex-backups/t037-release-v1.2.0-20261103/保留已验证before.bundle和原refs/tag/配置指纹；产品另有release-before-validation.json、release-main.tap、product-verification.json。最终OPC采用、引用、旧tag和安全清理结果由Git及final-verification.json核对，OPC自身SHA不写入同一提交正文。下述v1.1.0及更早内容均保留原时点历史。

## v1.1.0标签更名（历史）

T-030按用户最新明确要求将产品v1.0.1更名为附注v1.1.0，仍指向发布merge/main 3800e6dd8bc32b7d767bfc873a4b9ae57d10232b。本机及远端旧名均已移除，新对象/剥离目标已实际读回一致；v1.0.0及其他引用不变。仅更名，不改产品文件、配置、package版本、原发布merge及其message，WORKSPACE无须变更。

完整恢复点、原tag对象43b32337887b8a44a55739e22d4415f011ee523e/原文、原子推送及远端快照保存在产品.git/codex-backups/t030-version-tag-rename-20261102/。OPC对应标签当时保留45b72e4；后续T-031按用户明确要求压缩OPC两条记录，OPC主线及同名v1.1.0改指整理后的release。产品main/tag/文件/配置及WORKSPACE不受该OPC历史整理影响，仍为3800e6d。以下表格及T-027/T-028的v1.0.1名称属于原执行历史，不表示旧名称仍存在。

## 首次发布标签与提交（历史）

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
| 2026-11-02 | 0.4 | 按T-030将产品v1.0.1更名为v1.1.0并同步远端，main/发布merge仍3800e6d，文件与配置不变。 |
| 2026-11-02 | 0.5 | 说明T-031仅压缩OPC发布记录并同步其tag，产品main/v1.1.0、文件/配置及WORKSPACE均不变。 |
| 2026-11-03 | 0.6 | T-037按既有逻辑创建产品v1.2.0双父release与附注tag，README单独准备，release/main609/609，旧tag/配置/业务证据保持；仅本地，OPC采用实际新发布SHA。 |
| 2026-11-03 | 0.7 | T-038按用户明确请求原子同步产品main和v1.2.0，读回全部远端对象/目标一致，旧tag及其他引用/配置/产品文件保持；OPC回执仅本地，未创建GitHub Release或微信发布。 |
| 2026-11-06 | 0.8 | T-044/D-028将分享原7条整理为3组并保留原树，先快进main再以README准备创建双父release及附注v1.3.0；5条新增message/附注说明单行，四阶段各671/671，原证据/配置/旧tag保持。本次仅产品本地交付，OPC仍在feature保存回执、WORKSPACE暂留旧基线，未推送或清理分支。 |
| 2026-11-06 | 0.9 | T-045/D-029接续用户授权，将WORKSPACE采用产品最终571802a并核对本机/实时远端main/tag一致；产品626文件/两配置/全部引用只读保持，旧本地准备说明保留历史。OPC本地整理发布见A-017 v0.8，未执行产品推送或新增产品测试。 |
