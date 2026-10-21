# 核心实现与开发自测交接

- 项目：eating-everyday（今天吃什么）
- 成果 ID：A-011
- 负责人：developer（研发工程师）
- 关联任务：[T-010](../TASKS.md#t-010实现不依赖-appid-的核心逻辑与开发自测)；接续 [T-011](../TASKS.md#t-011准备验收用例并执行不依赖-appid-的测试) 第二阶段。
- 版本：0.3
- 更新日期：2026-10-21（Asia/Shanghai）
- 状态：draft
- 适用范围：不依赖 AppID 的领域、接口替身和开发自测交付；不是完整小程序、独立测试或真机验收。
- 来源及输入：[A-001 v0.9](product-requirements.md)、[A-007 v0.3](product-decision-options.md)、[A-006 v0.4](system-architecture.md)、[A-008 v0.2](accounts-access-checklist.md)、[A-010 v0.1](core-test-plan.md)，现行 D-005/D-011/D-014/D-016/D-017。实际接手 OPC 为 1f537859636de9f763e10a355ec20887da29e148；A-010 第一阶段已提交且 tester 已停止。
- 存储位置：本治理交接在 OPC artifacts；产品代码、示例、自测及证据均在本机产品仓库 git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday（相对 OPC 根目录）。
- 固定开发候选：feature/t-002-readiness-review，**54b720fa920dfcda9a3db5fa0e42378be01f5535**；父提交为tester资产a82d86b11a3daebcacd94e34af353b326e1e6d89，之前c4cb8bb/3fb016d的实现及证据保留历史。OPC沿用feature/eating-everyday/t-002-readiness-review。
- 主线交付：未合入、未推送；main、已有 origin/main 和 WORKSPACE.revision 保持 8fa87fe4cb8f7f33aacf2842798a486150a6105e。以上开发候选不能填作已合入最终版本。未重新联网核验 Git 远端。
- 批准依据：T-010 用户授权及原完成条件允许核心开发交接；无整份成果批准，不代写 T-011 结果或 T-002 完成。

- v0.2修复输入：[A-012 v0.1 独立报告与 BUG-T011-001](core-test-report.md)；OPC接手 ce5375a050ee064381857b0bcee68c19a56aba35。用户明确授权“需要修复和复测时继续按开发、测试串行推进”，developer 在 tester 停止后按该授权重开并完成修复交接。

- v0.3输入：[A-012 v0.2](core-test-report.md)第6节及T-010项目经理重开安排，实际接手OPC为9ccd252091368c2a356f1c282a1246fa998e1cf2。001已由tester独立关闭；本轮仅修复002并检查四类公开历史回放，不执行T-012平台接入。

以下第1～5节保留v0.1初次交付历史，第6节保留v0.2的001开发修复及当时待复测事实；**当前v0.3以第7节为准**，不把历史状态、数量或旧候选当作当前结论。

## 1. 实际实现及工程选择

采用 CommonJS JavaScript，零第三方运行包；Node.js 24.19.0 上直接运行。package.json/package-lock.json 锁定最小工程与无外部包的依赖图，dependencies.lock.json 固定 Unicode 官方原文件及生成表的 SHA-256。无需 npm 安装，当前环境未提供 npm 命令，不把未运行的安装步骤写成成功。

| 行为 | 产品仓库内路径及交付 |
|---|---|
| 候选 | src/policy.js、src/session.js：trim、非空、同列表逐字唯一、增删及0～24项；允许删除最后项，满额不挤掉旧项；候选名称不套菜单20字限制。 |
| 菜单 | src/repository.js：新建/另存、明确覆盖、重命名、加载、删除；不同菜单唯一名、非空快照、无固定菜单份数上限、完整名称和顺序。 |
| 编辑态 | src/session.js：工作副本与保存基线隔离，dirty 按有序名称比较；当前菜单删除成功后留未保存副本、清关联，后续另存新ID；切换三选项，失败不丢副本。 |
| 字符 | src/graphemes.js、src/unicode-data.json：固定 UAX #29 修订49 / Unicode 18.0.0 默认扩展字素簇规则；组合、ZWJ及20/21边界，不依赖宿主 Intl。 |
| 随机 | src/draw.js：可注入4字节 RandomPort，大端无符号32位、拒绝采样、预算32次；0/1项无熵调用，错误不出伪结果；代次失效及冲突命令锁。 |
| 存储 | src/repository.js、src/codec.js：不可变内容块和索引链、固定根、逐阶段回读、幂等回执、菜单revision、显式不明状态恢复和活块保护清理。 |
| 桌面入口 | src/index.js 导出核心；adapters/node-random.js 使用 Node 内置 crypto；examples/core-demo.js 使用显式输入及隔离内存，不给首次业务列表预填数据。 |
| 测试观察 | test/support/ports.js、test/support/fixture.js：隔离 StoragePort、随机序列、调用轨迹、before/after/延迟/回读替换和中断重建。 |

字符依据已只读核对 [UAX #29 修订49](https://www.unicode.org/reports/tr29/tr29-49.html) 及 [Unicode 18.0.0 官方一致性用例](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/GraphemeBreakTest.txt)；官方原文件和许可保存在 vendor/unicode-18.0.0。对应规则与853条独立分界预期匹配，未用被测计数器自己生成预期。

## 2. 接入命令和观察面

在 C:/Users/YinZh/Documents/workspace/eating-everyday 根目录、候选 c4cb8bb34d53efee92b512f0394ad0656dc9d531 上执行：

```powershell
node --test test/*.test.js
node examples/core-demo.js 面馆 饺子
node examples/core-demo.js
```

完整 TAP 复现命令为 node --test --test-reporter=tap test/*.test.js。演示传入名称才添加，未传参数从空列表退出；使用内存域，不承诺跨CLI进程留存，也不调用微信接口。

| A-010 第7节观察约定 | 交付入口与映射 |
|---|---|
| 创建、重建与注入 | require('./src') 的 MenuRepository、WorkingSession；仓储可注入 StoragePort、idFactory、clock、count、chunkBytes，Session 可注入 RandomPort/retryBudget；固定测试存储快照可重建。 |
| 候选 | session.addCandidate(raw)、removeCandidate(candidateId)；同步状态/明确错误，失败不变。 |
| 新建/覆盖/另存 | session.createMenu(name,options)、overwriteMenu(currentMenuId,options)；返回 operationId/storeRevision/menuId/snapshot/replayed。低层 repo 同名接口显式传候选名称数组；除新建外必须传目标 expectedRevision。 |
| 列表/加载 | session.listMenus()/loadMenu(id)、repo.inspect()/loadMenu(id)；列表含ID/名称/revision/数量，完整快照含 orderedCandidates（ID与完整名称）；返回和工作副本均深隔离。 |
| 重命名/删除 | session.renameMenu(id,name,options)、deleteMenu(id,{confirmed,...options})；当前菜单使用保存基线版本，过期冲突不被源记录预读掩盖。 |
| 切换及保存失败 | session.switchMenu(id,{choice:'save'/'discard'/'cancel',name?,...options})；保存确认前目标load=0，保存成功但目标读失败保留源已存事实。 |
| 抽取/过期 | session.draw()/invalidate()；候选固定快照、drawId/generation及索引/完整名，旧代次不显示，不改菜单。 |
| 工作状态 | session.state 深拷贝：candidates、currentMenuId、baselineRevision、baselineCandidates、dirty、phase、generation、result、storageStatus。 |
| 存储/错误/恢复 | repo.inspect()；session.recoverStorage()；storage.trace/data/snapshot()/hook；CoreError.code/details。实际键和阶段均可枚举。 |

产品 docs/core-api.md 详细记录参数、同步/异步错误、版本和幂等语义、端口签名及 A-010 对应；tester 可自行实现 Port 或独立断言，不必复用开发断言。此接口命名是 A-010 允许的具体映射，没有改变业务规则或新增批准门槛。

## 3. 存储故障与随机证据

首次写入先建立可验证的0版本空根，防止首次保存准备失败后无法区分空库与孤块；合法首次读取不写入。随后 content.write/readback → index.write/readback → capacity.info/root.guard → root.write/readback → commit.verify.*。内容/索引均可分块，默认每块128KiB，测试使用2048字节触发多块；没有把全部菜单硬塞单个key或设置20份上限。

准备失败不替换旧根；根提交回调异常但读回完整新版本时按实际成功处理；读回旧版本则明确未提交；不可判定时为 STORAGE_UNCERTAIN，保留编辑态，禁止继续写入/切换，再显式 recover。保存已确认但目标加载失败不假称全部回滚。cleanup(keys) 跳过有效根及全部可达块，清理错误不撤销已确认保存。首次空根、缺根有块、损坏和未知schema分别处理，不自动清库或回退复活已删菜单。

测试注入覆盖内容首/中/末块、部分生效、索引和根、真实替身容量不足、回读缺失/不符/失败、元信息失败、提交不明、各阶段模拟停止、幂等及过期版本、损坏快照、孤块清理与保存后加载失败。SimulatedCrash 是停止旧实例后从内存字节快照重建的模型，不是微信进程断电证据。

随机以32次为有限拒绝预算，x大端、B允许等于2^32。N=2/3/24边界、拒绝域与重试次数、熵错误/无效长度、0/1不调用、允许重复、代次失效均有实际断言。N=2～24用独立BigInt算术核对接受域每余数原像数量；均匀和独立仍依赖真实熵源假设，不用有限频数证明等概率。Node crypto 仅为桌面适配，真实 wx 熵源尚未覆盖。

## 4. 实际自测结果与计划范围

- 环境：Windows 11 10.0.26300、AMD64、Node v24.19.0；2026-10-21 执行。业务核心运行无需 Python、npm、AppID 或 wx；Python 3.12.3 仅用于本次文件/证据整理。
- 开发自测：93/93通过，失败/取消/跳过/todo均为0；官方853条字符一致性样本包含在独立语料测试中，不虚增为853个产品验收通过项。
- 提交前保存完整 TAP、环境和源码 SHA-256；提交后在上述固定候选复跑同一套93项再次全部通过，源指纹匹配且产品工作树干净，main未变。
- 两次桌面演示（显式候选、无候选）退出码均0。日志和测试仅使用隔离内存，没有用户真实菜单或凭据。
- 产品证据位置：test/evidence/developer.tap、test/evidence/developer-run.json、test/evidence/demo.log；自测源码见 test/*.test.js，版本均为上述候选提交。
- Git检查：自有源码统一LF，暂存字节与证据指纹一致；Unicode官方原文件保持SHA-256原字节，其原有行尾空格仅在vendor路径设定空白例外；产品 diff --check 通过。
- 治理检查：python -X utf8 -B scripts/validate_framework.py 通过（22个角色、39份技能、1个实际项目、537处本地链接），OPC diff --check 通过；逐段比较确认 T-010 之外的任务正文（包括 T-011）未变。框架检查不代替产品测试或批准。

| A-010 范围 | 实际开发自测证据及限制 |
|---|---|
| TC-03/05/06/11/12/13/14/15/16/17/25/27/28/29/30/31/32/35/36，BN-01～05、ST-01～05 | domain/storage/unicode 自测覆盖相应L逻辑；20/21字素、141字及多块长名称、21份菜单、快照/dirty/当前删除/切换均有断言。 |
| TC-04/09/10、ST-06、RN-01～07 | random 自测与精确算术、锁和迟到回调；无实际微信熵源及页面动画证据。 |
| FI-01～15 | storage 自测分阶段 before/after/回读/停止/恢复，范围详见第3节和产品接口说明。 |
| TC-07 | L重建与恢复通过；M正常关闭后菜单留存未执行。 |
| TC-18/20/37 | L无网络依赖、初始空候选、隔离清空和重建通过；平台打开、真实配置/调用/界面和离线未执行。 |
| TC-08及所有D/M/O部分 | 无产品通过结论；页面、工具、微信、设备和断网用例待后续工程及环境。 |

早期88项矩阵运行的9个失败是开发自测夹具错误：8项把改名/删除的源记录预读误计成切换目标加载，1项在单候选N上以不存在下标构造重复数据；已改正观察范围和夹具，原业务断言保留。代码自查另补过期基线、首次保存恢复和异步返回phase保护及回归。最终93项及独立官方分界预期均真实执行，无未解决的核心自测失败；本记录不冒充 tester 的独立检查。

## 5. 当前限制及下一位行动人

没有发现必须改变现行业务规则的输入冲突。以下实现边界已明确提供给 tester：

1. 当前是桌面可测试核心，不含小程序 app/page 工程、wx StoragePort/RandomPort、动画或界面；基础库兼容、IME/键盘、小屏/大字、实际手机留存、离线和真实平台故障均未执行。AppID、工具和设备由用户准备，不阻塞 T-011 的当前L阶段。
2. 仓储以同存储域单个 repository 串行写为前提；root.guard 能发现已发生的变化，不能充当跨进程CAS。未承诺跨key事务或微信断电原子性。真实Port计量、可读语义和单key条件须平台集成时验证。
3. 保留持久幂等回执，暂不压缩；孤块只显式清理。历史回执、复制准备和孤块消耗实际容量；没有菜单固定业务上限，也没有无限空间承诺。大量菜单/极端名称的性能与资源界限未实测。
4. 校验值检测意外损坏，不作加密/防恶意篡改保证。根/引用损坏时停止覆盖，不静默清库；是否需要恢复工具应另按实际缺陷或任务安排。
5. 工作区发布基线与开发候选分列。产品main和OPC长期分支均未合入；WORKSPACE、他人测试计划/记录、决定和协调摘要未由本成员改写。

T-010 原完成条件为核心开发交接，本次已达到；T-011 第二阶段尚未执行，T-002 整体及成果批准由相应负责人维护。developer 固定候选、保存本交接和本人任务/索引后停止，由用户授权的串行通知再让 tester 在精确候选上执行并记录独立结果。不启动或联系其他会话；后续缺陷修复待 tester 停止后按安排处理。

## 6. v0.2：BUG-T011-001 修复交接

本次只修复公开 Session 删除接口的原意图回放。已在未改生产源码的 bcbcf7c7ae9ddf5480dac45d121afd60b2c88431 上运行原独立缺陷用例，实际退出1、MENU_NOT_FOUND，与A-012一致。原因是Session先load已删除来源，尚未进入仓储的持久回执判断；未观察到数据丢失。

当前产品候选为 **3fb016d22ba69f0046afdc6f0e88329477e911cf**。src/session.js 的 deleteMenu 在显式传入 expectedRevision 时，将原 menuId/operationId/expectedRevision 直接交给仓储，先核对持久回执，再由仓储处理目标存在性和版本。不捕获并吞掉所有MENU_NOT_FOUND，不重新推算原意图版本；未给版本的首次操作仍使用当前基线或读取实时目标。确认/取消、BUSY、不明状态锁、旧副本、其他菜单和持久化协议保持原语义。

同意图返回原 storeRevision 且 replayed=true，不产生新提交；更换目标/原版本仍REVISION_CONFLICT，新操作删除不存在记录仍MENU_NOT_FOUND，取消不写入。新增 test/delete-replay.test.js 的10项开发回归覆盖当前/非当前删除重试、重建后回放与新副本保护、错误意图/版本/不存在目标、默认版本及过期基线、删除结果不明的已/未提交恢复，以及公开新建/覆盖/重命名正常回放。

| 本次开发实际复核命令（产品仓库根目录） | 结果 | 新证据路径 |
|---|---|---|
| node --test --test-reporter=tap test/*.test.js | 103/103通过；含原93项和新增10项，官方853条分界语料仍在原语料测试中。 | test/evidence/developer-fix-t011-001/developer.tap |
| node --test --test-reporter=tap test/independent/core.acceptance.test.js | 原59项均通过，原独立用例/断言未改。 | test/evidence/developer-fix-t011-001/independent.tap |
| node --test --test-reporter=tap test/independent/defect-t011-001.test.js | 原缺陷用例1/1通过；同参数公开重试返回原回执。 | test/evidence/developer-fix-t011-001/defect.tap |
| node --test --test-reporter=tap test/*.test.js test/independent/*.test.js | 在上述已固定提交上合并复跑163/163通过，失败/取消/跳过/todo均0。 | 本次固定版本复核回执；前三组完整TAP在上述候选内。 |

环境仍为Windows 11 10.0.26300、Node v24.19.0，无AppID/wx/依赖安装。本次运行元数据、源码指纹及八份原tester文件指纹见 test/evidence/developer-fix-t011-001/run.json。独立测试源码与 test/evidence/tester-stage2/ 的原失败日志、run.json和目录规则均与bcbcf7c7逐字节相同；初始开发证据也未覆盖。本次只增加自己的复核目录，TAP空白例外限定该目录。

产品提交只改 src/session.js、README、接口说明，并新增本成员回归和复核证据；核心仓储、业务规则、依赖锁、Unicode数据、tester源码与原日志未改。提交后源指纹匹配、工作树干净、main仍为8fa87fe4cb8f7f33aacf2842798a486150a6105e；未合入、推送、改写历史或删分支，WORKSPACE不变。

以上60项原独立用例是 **developer执行的开发复核**，不冒充tester实际复测或新的独立验收。BUG-T011-001的open状态及T-011 blocked/未覆盖表仍由tester维护，本成员未改A-012、T-011或其索引。T-010按原核心修复交接条件完成；tester需在本会话停止后通过授权的串行通知，在新候选上实际复测并继续A-012列出的剩余L场景。本次开发103项不自动补齐tester的未覆盖范围。微信适配、页面/动画、工具、手机留存及离线仍未实现/执行，不等待AppID进行本轮纯逻辑复测；无新的批准或发布结论。

## 7. v0.3：BUG-T011-002及四类历史回放修复

本轮由项目经理依据用户明确授权安排，tester及项目经理停止后接手。原产品HEAD为测试资产a82d86b11a3daebcacd94e34af353b326e1e6d89，生产源码对应3fb016d。先运行原defect-t011-002.test.js，实际退出1、MENU_NOT_FOUND；原始新复现输出单独保存在test/evidence/developer-fix-t011-002/before-fix.tap，没有覆盖tester日志。

新固定候选为 **54b720fa920dfcda9a3db5fa0e42378be01f5535**，父提交为a82d86b11a3daebcacd94e34af353b326e1e6d89。保持既有持久化schema及旧回执签名兼容；修改限于Session/Repository的回放路径、接口说明及本成员回归/证据：

- 重命名携带原expectedRevision时，直接交仓储先核验完整原意图回执，不再预读取可能已删除的来源。未给版本的首次操作仍使用当前基线或实时目标，新的不存在目标仍拒绝。
- Repository新增同队列只读replayWrite，和真正写命令共用_signature/_receipt，精确比较命令、目标/名称、有序候选及原版本。命中才回放，变意图或版本冲突仍拒绝；读失败不降级成新写入，不修改旧回执或数据格式。
- 新建/覆盖先检查完整回执，再检查新写入的当前状态限制；无回执的新覆盖仍须是已加载当前目标，新保存仍须等于当前有序候选。历史回放不清dirty、不换基线/关联、不改变generation或当前结果、不复活目标、无额外持久提交。
- 新建/覆盖新增可选options.candidateNames以显式保留原请求的候选快照。未提供时仍采样当前副本；当前内容已变时复用旧operationId属于不同实际意图，应报REVISION_CONFLICT，不以只比请求ID掩盖变更。重试可保留首次快照，或从首次结果snapshot.orderedCandidates映射displayName取得；目标、原菜单名/版本及operationId也须保留。该参数只表达原技术请求，不允许新命令保存与当前副本不同的隐藏内容。详见产品docs/core-api.md“保留完整历史意图”。

新增test/historical-replay.test.js共20项开发回归：四类操作在后续修改/删除后的回放、重建后的不同dirty当前菜单、错误命令/目标/名称/顺序/版本、新写入目标和内容保护、隐式内容变化与显式原快照的区分、回执读取失败及异步输入捕获。原test/delete-replay.test.js及全部tester用例不改。

| 实际开发复核命令（产品仓库根目录） | 结果 | 证据路径（同候选内） |
|---|---|---|
| node --test --test-reporter=tap test/*.test.js | 123/123通过（原103+新增20）。 | test/evidence/developer-fix-t011-002/developer.tap |
| node --test --test-reporter=tap test/independent/*.test.js | 原113/113通过，含001/002原断言及52项剩余场景；这是developer复核。 | test/evidence/developer-fix-t011-002/independent.tap |
| node --test --test-reporter=tap test/*.test.js test/independent/*.test.js | 提交前236/236通过；上述固定提交上再次236/236通过，失败/取消/跳过/todo均0。 | test/evidence/developer-fix-t011-002/combined.tap；固定提交复跑回执。 |

Windows 11 10.0.26300、Node v24.19.0，无AppID、微信或新增运行依赖。运行元数据、源码SHA-256及17份tester文件SHA-256见test/evidence/developer-fix-t011-002/run.json；这17份文件（独立测试、两个tester证据目录及原失败/夹具输出）与a82d86b逐字节一致。原001开发复核和原始开发证据同样保留。本次源码指纹与固定提交匹配，产品工作树干净，main/origin/main与WORKSPACE仍为8fa87fe4cb8f7f33aacf2842798a486150a6105e；不合入、推送、改写历史或删分支。

本次236项由developer执行，不代写tester独立复测。001的closed来自A-012 v0.2原记录；002的open、T-011 blocked、其原报告和索引未改。下一步由项目经理在本会话停止后串行通知tester，在新候选复测002、回归四类历史回放和错误意图/副本保护，并按变更影响补测。T-010本轮核心修复交接完成；T-012/T-013仍待后续通知，未提前执行平台接入。规模/性能及未穷举的交叉依然不冒称全覆盖；真实wx、页面/动画、工具编译和手机留存/离线未验证。

## 版本记录

| 日期 | 版本 | 变化 | 状态 |
|---|---|---|---|
| 2026-10-21 | 0.1 | T-010核心实现、固定候选、93项开发自测及853条官方分界检查；接口/阶段/计划映射与未覆盖项交接。 | draft |
| 2026-10-21 | 0.2 | BUG-T011-001公开删除重试修复；103项开发及原60项独立用例的开发复核通过，固定新候选，原失败资产保留，待tester独立复测。 | draft |
| 2026-10-21 | 0.3 | 002及四类历史回放修复，新增20项；开发123和原独立113项复核通过、合并236项通过，固定新候选，待tester独立复测。 | draft |
