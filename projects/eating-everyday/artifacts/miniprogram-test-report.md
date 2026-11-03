# 小程序独立平台测试与缺陷复测报告

- 项目：eating-everyday（今天吃什么）
- 成果 ID：A-014
- 负责人：tester（测试工程师，兼职）
- 关联任务：当前[T-035](../TASKS.md#t-035独立验证启动恢复和本机菜单保护)；T-013/T-015/T-023及其独立结论保留原版本历史，不代写 developer 结果。
- 版本：0.8
- 更新日期：2026-11-03（Asia/Shanghai）
- 状态：draft
- 适用范围：当前为T-035修复后独立复测及剩余启动恢复验证。BUG-T035-001在固定5085636上closed，T-035 completed，声明范围内建议合入；第1～11节保留原专业历史及首轮失败，不扩为实体手机或正式发布验收，成果仍draft。
- v0.1授权及输入：用户既定串行授权、项目经理本轮通知；[A-001 v0.9](product-requirements.md)、[A-006 v0.4](system-architecture.md)、[A-010 v0.1](core-test-plan.md)、[A-012 v0.3](core-test-report.md)、[A-013 v0.1](miniprogram-preview-handoff.md)及现行决定。接收时刷新 tester 身份、scope、角色、testing/微信小程序测试正文；未变规则及历史按 Git 范围核对复用。
- v0.1 OPC接手版本：7d497ef99bd9a26566982ebb86c8af13ce6f84f7，feature/eating-everyday/t-002-readiness-review；目标 project/eating-everyday 的本轮合入前基线 a735031c37c09e75fb9478681aaf42010a82f378。
- v0.1被测实现：本机产品 git@github.com:yinzhongkai/eating-everyday.git，checkout=`C:/Users/YinZh/Documents/workspace/eating-everyday`，feature/t-002-readiness-review，**f7ceaf791dd06e8749fa1754b6a0143d1193fb63**；测试期间未改生产。
- v0.1测试资产：同一产品仓库 **f678942064fb9dc8fdf37ddf9a2257a8b459c1c0**。自上述实现起仅新增20份 tester 测试/证据；首次资产提交 c0dcb576647535aa8b82200ad80b4c03787af6c4，后续 f7581070b90b0d1d3d50615e364107a0f0b26da9 / f678942 只修正新 JPEG 的二进制保存及证据字节哈希，未重写历史或改变失败断言。
- v0.2接手及该轮版本：项目经理按用户既定恢复授权串行通知，developer已停止；OPC输入6eca7297c1de1a2c6cd32a6068c3e412d0b12318及A-013 v0.2，固定产品实现**8e4472006f1d3cdff5b774052e01ce8db5a96453**，新tester资产**d212b5908484c47906a3eb62f254b74556175a0e**（父为该实现）。本轮仅新增test/evidence/tester-retest-t013-001/16份文件；37份非test和原44份tester原始工作树指纹不变，旧Git内容不变。结果及证据限制见第6节。
- 存储位置：治理报告在 OPC artifacts；执行代码、截图、日志在产品 test/independent/defect-t013-001.test.js、test/evidence/tester-t013-preflight/、test/evidence/tester-t013-platform/，以下均为产品仓库内路径。
- 主线交付：未合入或推送。产品 main/已有 origin/main/WORKSPACE.revision 仍为 8fa87fe4cb8f7f33aacf2842798a486150a6105e。
- 批准依据：无整份成果批准；测试成功子项不代表完整预览闭环、真机验收或正式发布。
- v0.3接手与当前资产：OPC输入cb1107213c148e5ee006f5c5697e31eedbf87b90；原同后缀feature和目标基线未变。固定实现仍为8e4472006f1d3cdff5b774052e01ce8db5a96453，新测试资产**10f52d40bb8a62b04a8f84793e44117fe3c28e46**（父为d212b59），仅新增test/evidence/tester-t013-special22/49份文件。用户授权保留的既有project.config.json差异未提交且本轮字节未变，其他受控文件未变。
- 版本记录：0.1首轮发现BUG-T013-001并停止；0.2独立复测关闭001及预览交接；0.3补齐专项并纳入用户手机确认、清理隔离数据后完成T-013。原T-011 completed及其001/002 closed历史不变；0.4追加T-015两配置独立完整回归及可合入结论；0.5保留T-023桌面结果及用户暂停检查点；0.6按用户恢复授权完成真实按钮/平台差异及隔离清理，T-023 completed、建议合入，原结果不改写；0.7追加T-035独立复现BUG-T035-001，任务blocked、等待修复复测；0.8独立复测关闭001并完成T-035剩余桌面/隔离平台验证，声明范围内可合入。

**v0.1历史结论：f7ceaf上001 open、T-013 blocked；原285项通过与新增1项失败分别保留。** 第1～5节保持当时正文，不把旧失败改记为通过。

**v0.3历史结论：固定8e44720的本轮约定验证已完成，T-013为completed。** 手机扫码/基础操作、正常关闭重开加载、离线，以及本轮输入法/软键盘、大字/安全区和旋转时前后台均有用户自测正常的明确反馈；tester完成剩余D层专项、保留新日志/截图并清理全部隔离前缀。提交基线292/292；保留本机AppID配置差异的工作树291/292，唯一失败为touristappid约束，不改旧断言。设备/构建信息未提供，真实满配额、多设备及旧原始GUI附件缺口仍按限制记录。第1～6节是原阶段历史，第7节为本轮结论。

**v0.4历史结论：T-015独立复测通过，可合入，任务completed。** 保留本机配置工作树与无`.git`的固定候选导出分别完整299/299；另85/85配置探针及六类隔离缺陷拒绝检查通过。原211份受控文件与公共/私有配置字节未变，个人AppID未纳入新提交；新导出/探针目录已清理。详情及精确提交见第8节；合并推送由project-manager执行T-016。

**v0.5暂停检查点历史：用户要求等空闲时再测试，T-023当时保持in_progress并暂停。** 新独立129/129，保留配置工作树与固定导出各501/501（原372＋独立129），非通过计数均0；真实工具只完成隔离初始化失败/PENDING观察，重试按钮未点击、后续平台验证和隔离数据清理未执行。已执行范围未发现新产品缺陷，但本轮尚不宣称通过或可合入。详细固定版本、原始证据、未执行范围和恢复点见第9节；不自动检测空闲或定时续跑。

**v0.6历史结论：T-023约定范围独立验证完成，可合入，无新增产品缺陷。** 原桌面结果按未变版本保留；本次完成12组独立真实wx、实际鼠标重试/菜单重载/抽取、页面重进及删除不复活。onRetry恰1次、原pending菜单ID不变、唯一完整8项自动加载且dirty=false。新增夹具分批清理27＋7＋7个key，最终本次前缀剩余0、外域key删除0，隔离工程已关闭；262份原受控文件和两配置字节保持。新证据产品提交12649a797966299c4f23b6d4a4b9f0b3d2d1619e，详细层级、访问记录和限制见第10节。未合入/推送或批准整份成果。

**v0.7历史结论：T-035 blocked，暂不可合入。** 固定代码e3ce8cfecde344d3bd7c4c003ee427696fd9a877独立复现BUG-T035-001（P2/open）：B记忆写失败后，C加载时选择读失败；恢复IO后重试仍被旧B意图遮蔽，页面静默清警示但持久记忆仍A，新页面恢复A。控制器首轮2通过/1失败，实际页面handler确认0通过/1失败，是同一缺陷的两层证据。按发现缺陷后停止约定，未执行全套/干净导出/真实wx或UI。313份原文件和两配置字节不变；新产品测试证据48469cfa7594b127b0af549b9ecafe0698b47cc9。详见第11节，等待项目经理安排修复后复测。

**当前v0.8：T-035 completed，BUG-T035-001（P2）独立复测closed，声明范围内可合入。** 固定修复5085636ba8955b08fcf39c24cf84b0329ef89c6d，未改原四项4/4、新增独立31/31，工作树/干净导出全套各609/609。真实微信隔离域完成鼠标补记/抽取、旧v1升级/空库、菜单保存切换/改名删除/后台编辑与异常提示，以及工程窗口关闭重开恢复B；20条行为断言及1条清理记录、0运行异常。343原文件及两配置字节保持，累计仅清本前缀50 key、最终剩余0，隔离工程已关闭。本人新测试证据912d77154d1c3958680a6f01400d9218c688eafd，详见第12节；未合入/推送或新增实体手机、物理故障和发布结论。

## 1. 授权、环境和证据层次

最初用户 Esc 停止仍有效时，只执行源码/桌面预检；原始 run.json 的 `ideActions=none`描述该阶段，未倒写。随后项目经理转达用户明确答复“允许开发和测试恢复自动操作，完成编译与预览验证”，才恢复 CLI/官方 SDK/Computer Use。developer及其他成员均停止，项目经理只协调读取；没有跨会话发消息或唤醒开发。

Computer Use 第一次使用编译前旧窗口句柄失败；刷新后遇到 `Computer Use app approval timed out`。当时没有取得窗口截图或点击，未收到自动审批拒绝理由。项目经理随后转达用户“已经处理了审批提示”，重新选取唯一主窗口成功；后面的实际鼠标复现已完成。当前阻塞是产品缺陷，应用审批超时已解除，不要求用户重复授权。

| 环境 | 实际值及边界 |
|---|---|
| 主机/Node | Windows 11 10.0.26300，Node v24.19.0；桌面无新增依赖。 |
| 开发者工具 | 已安装、已登录的 Stable 2.02.2608080；现有服务端口；CLI auto退出0。未修改安全/隐私设置。 |
| 小程序 | 基础库3.17.4，原生WebView；现有被忽略私有AppID配置，未读出或修改AppSecret。 |
| 模拟器 | iPhone 12/13 (Pro)，屏幕390×844、视口390×753、DPR3。API报告的 iOS10.0.1、WeChat8.0.5是模拟字段，不能作为实体设备证明。 |
| SDK | 既有被忽略的官方 miniprogram-automator@0.13.0，无安装/升级。按本机对应类型与实现核对调用。 |
| 鼠标验证 | 使用computer-use技能，在真实1250×1000工具窗口截图上逐次观察、输入、点击和刷新。不是仅判断节点存在或SDK程序tap。 |
| 存储隔离 | 两份脚本各用新建隔离前缀；均在菜单写入前失败，清理0个key。独立脚本确认v1用户域前后相同；后续诊断、鼠标仅编辑未保存副本，无业务存储写入。 |

官方API网页在本轮读取失败；没有用网页错误推断API不可用。实际API成功值、适配器失败、页面错误和鼠标截图才是本缺陷依据。有限抽样也不作为均匀概率证明。

## 2. 执行记录

| 检查/命令 | 实际结果 | 原始证据 |
|---|---|---|
| node --test --test-reporter=tap test/*.test.js test/independent/*.test.js（新增缺陷用例前） | 退出0，285/285；失败/取消/跳过/todo均0。开发151＋原独立134，本成员复跑。 | tester-t013-preflight/baseline-285.tap、run.json |
| CLI auto --project 产品根目录 --auto-port 9420 --lang zh | 退出0；固定源码运行到ready/空列表；最终PNG可见。不是preview生成或手机通过。 | 平台run.json、simulator-empty.png及本轮工具调用记录 |
| 独立调用既有 test/native-smoke.js，设置WX_RUN_NATIVE_SMOKE=1和独立输出目录 | 0项提示、1项结果取得；随后SDK响应超时，整段failed。原kind字符串来自既有开发脚本，不冒充新独立用例设计。 | native-smoke.json；simulator-empty.png |
| 独立 controller-smoke.js | 空/单项通过，加入两项且PNG生成；直接等待随机Promise时SDK超时，整段failed；未到菜单保存。 | controller-smoke.json、two-candidates.png |
| 有限 random-diagnostic.js | 不等待长SDK Promise而轮询，真实API返回4字节；双项draw显示错误/无结果/rotation=0。之后SDK截图或收尾失败，原failed状态保留。 | random-diagnostic.json、diagnostic-two-before.png；不存在diagnostic-two-after截图 |
| Computer Use实际单项点击 | 输入“面馆”并点击加入、中央“开吃”，结果卡显示“面馆”，可见按钮确实响应。 | 本轮Computer Use真实窗口调用记录；另尝试capture-runtime.js保存SDK截图失败，没有actual-mouse-single文件，不虚构截图 |
| Computer Use实际双项点击 | 从空页经真实输入框/加入按钮加入“面馆”“饺子”，两色PNG和中央按钮可见。实际点击中心后立即出现抽取不可用提示，无旋转或结果，失败。 | actual-mouse-two-failure.jpg；对应工具调用记录 |
| failure-probe.js（在同一鼠标失败状态，只读定位） | adapter拒绝RANDOM_UNAVAILABLE / Invalid random bytes；真实API另一次成功返回[object ArrayBuffer]、length4、instanceof=false。前后候选2项、busy=false、resultOpen=false、rotation=0。 | failure-probe.json |
| node --test --test-reporter=tap test/independent/defect-t013-001.test.js | 退出1，0通过/1失败，取消/跳过/todo均0；跨realm合法4字节值被拒绝，保留原期望。 | defect-t013-001.tap、独立用例源码 |

所有SDK失败均保留，不转记为整段通过；未明确执行的后续脚本代码不算覆盖。285/285描述原套件；新增缺陷用例单独1/1失败，未声称新增后的完整套件通过。37份非test受保护文件与预检前后SHA-256相同，原24份tester文件与f2646b52逐字节相同。最终提交的15份平台证据文件哈希已与run.json逐一核对，JPEG原始SHA-256为52f0ee86ee2210a544ad4865af1c994837a629187ce30639f264b03a464327b1。

## 3. BUG-T013-001：真实wx随机值被执行域判定拒绝

| 字段 | 内容 |
|---|---|
| 状态/严重度 | open；阻塞当前首个可用预览。两项实际失败；3～24项未继续实测，不虚报全部次数。 |
| 需求与用例 | A-001 AC-03/04/09，A-010 TC-03/04/09、PV-04；多项应从当前快照产生有效结果并完成一致的转盘展示。 |
| 固定版本/位置 | f7ceaf791dd06e8749fa1754b6a0143d1193fb63，adapters/wx.js:38 的 `result.randomValues instanceof ArrayBuffer`。测试资产版本单独见元信息。 |
| 前置 | 上述真实开发工具/基础库，空当前副本，正常ready且PNG生成；不需要已保存菜单、网络餐厅、地址或权限输入。 |
| 最小步骤 | ①输入“面馆”点击加入；②输入“饺子”点击加入，等待两色转盘；③实际点击可见中央“开吃”。 |
| 预期 | 从两名称中抽中一个，旋转后显示其完整名称，当前候选不变。 |
| 实际 | 立即显示“当前暂时无法抽取，请稍后重试”；无旋转、无结果，rotation=0；候选原样保留，busy恢复false。 |
| 重现 | 状态诊断1次＋独立实际鼠标1次均取得同一页面失败；SDK超时步骤不另计为产品重现次数。单项不调用熵，因此能成功。 |
| 定位证据 | 真实wx成功回调的值tag=[object ArrayBuffer]、byteLength=4，但在当前执行域instanceof ArrayBuffer=false；实际WxRandom.bytes拒绝并返回Invalid random bytes。独立VM回归复现同一输入契约差异。 |
| 影响 | 当前真实环境下有效多项熵值被适配器拒绝，核心抽取无法进入动画/结果。原桌面同realm替身未暴露此差异；这不是先前001/002重开。 |
| 修复与复测边界 | 由developer处理宿主合法ArrayBuffer的兼容校验/转换，继续拒绝非法值和错误长度，不得降级不安全熵源或改变均匀算法。tester不改生产；新固定版本后先复测此例及实际鼠标双项，再回归相关输入/失败与旧套件。修复前不关闭。 |

## 4. A-010平台层覆盖现状

以下只描述本轮D/M/O；L层历史及当前285项复跑单列在A-012/第2节。通过某个子项不代表整项AC验收，M和O本轮均未执行。

| AC / TC | 本轮平台状态及剩余 |
|---|---|
| 03 | 名称输入/追加两项通过；参与实际多项抽取失败，001阻塞。 |
| 04 | 双项抽取失败；完整多项结果、指针最终对齐未通过。 |
| 05、06 | 多菜单保存/加载、原顺序恢复未执行；因001停止，不能用脚本未到达代码计为通过。 |
| 07 | 手机正常关闭再开留存未执行；SDK页面reLaunch不等于结束微信/小程序进程。 |
| 08 | 0/1/2项PNG、指针、按钮可见且真实单项按钮响应通过；12/24、长名、键盘、小屏、大字及手机触控未执行。 |
| 09 | 真实随机API自身成功子项通过，适配器/多项流程失败；统计均匀不在本轮平台结论中。 |
| 10 | 已测0项提示（SDK）、1项唯一结果（SDK及真实鼠标）通过；0项实际鼠标和手机未执行。 |
| 11 | 两个基本中文名称的真实输入通过；空白/trim、粘贴/输入法组合等平台分支未执行。 |
| 12、13 | 重复名称、删除/删光后再抽取的平台流程未执行。 |
| 14、15、16、17 | 管理/修改副本、覆盖另存、dirty三选项及错误切换的平台闭环未执行。 |
| 18 | 已能打开后断网闭环未执行；未把源码无业务网络依赖当O通过。 |
| 20 | 隔离脚本清理0key不是已保存测试菜单清除场景；本用例未执行。 |
| 25、27、28、29、30 | 24/25、菜单20/21字素与长候选、同名、空候选保存、多于20份菜单的平台边界未执行。原桌面结果不变。 |
| 31、32、35 | 真实/注入容量和读写失败、删除确认/保留副本、保存失败不切换等平台流程未执行。 |
| 36 | 加入第1/2项通过子项；13/24平台边界未执行。 |
| 37 | 最终原生页初始空、无定位/搜索入口观察通过；完整全手动闭环被001阻塞，未验收。 |

当前没有取得手机扫码、正常关闭重开、实体生命周期/安全区/字体键盘、真实wx菜单完整闭环或离线结果。无需为修复前的无效候选要求用户扫码；待双项及关键菜单闭环通过后再生成新码，给用户一个扫码验证步骤，完整手机验收可以继续单列。

## 5. 停止与串行接续

本轮按T-013约定在阻塞缺陷定位后停止业务扩测；最后界面保留双项错误供开发查看，没有继续GUI/CLI/SDK业务操作，不生成新码，不修改生产、需求、架构、他人任务/索引、STATUS或WORKSPACE，不合入、推送、发布、重写历史或删分支。

本报告、T-013和本人A-014索引完成本地治理保存后停止，由项目经理按既定授权串行安排developer修复并提交新精确候选；tester收到后先复测BUG-T013-001与受影响回归，再继续PNG动画/菜单/新预览和手机分层检查。当前不存在需要重复确认的恢复操作授权；审批已解除，不能用原Esc或AppID缺失解释当前失败。成果draft，T-013不completed。

## 6. v0.2独立复测与当前预览交接

### 固定版本及独立回归

本轮刷新tester登记/role/scope、testing及微信小程序测试正文；根规则、需求、架构、决定经b44d985以来Git范围比较未变，读取A-013 v0.2和本人T-013。两仓库接手时同后缀feature且干净；固定实现8e4472006f1d3cdff5b774052e01ce8db5a96453，父为原tester资产f678942。沿用用户已经恢复的自动操作授权、已处理的应用审批、既有私有AppID/端口/官方SDK，不改安全隐私设置或AppSecret。developer停止，未联系其他会话。

产品新资产d212b5908484c47906a3eb62f254b74556175a0e的test/evidence/tester-retest-t013-001/保存下列独立执行结果。环境仍为Windows11/Node24.19.0、开发工具2.02.2608080、基础库3.17.4、WebView、iPhone12/13(Pro)模拟器390×844/视口390×753/DPR3、SDK0.13.0；模拟iOS/微信版本不冒充手机环境。

| 独立命令 | 实际结果 |
|---|---|
| node --test --test-reporter=tap test/independent/defect-t013-001.test.js | 退出0，原失败断言未改，1/1通过。original-defect.tap。 |
| node --test --test-reporter=tap test/wx-random-realms.test.js test/independent/defect-t013-001.test.js | 退出0，7/7通过。related-realms.tap；包含开发新增案例，本成员复跑。 |
| node --test --test-reporter=tap test/*.test.js test/independent/*.test.js | 退出0，292/292（开发157＋独立135），失败/取消/跳过/todo均0。combined-292.tap；三次计数不叠加为独立覆盖总数。 |

desktop-run.json保留命令、时间及前后指纹；收尾run.json再次核对37份非test和44份原tester原始工作树字节未变，Git内容与f678942一致。未改src/draw.js、旧用例或业务代码。新16份资产暂存字节逐项与元数据SHA-256一致；原CLI的CRLF/空行只在本证据目录设例外，不改全局规则。

### 001关闭及实际平台操作

CLI重新编译退出0后，实际鼠标操作前断开SDK；每次按真实窗口状态输入/点击，SDK只作离散观察、夹具或截图。下表“工具摘录”均指platform-tool-excerpts.md及本聊天实际工具响应，原始附件缺口在下一小节单列，不能把观察与缺失文件混写。

| 范围 | 实际观察与边界 |
|---|---|
| BUG-T013-001 | 真实输入面馆/饺子、实际中心点击，看到旋转及饺子结果；记录index1、rotation1980、busy=false/error空，指针/完整结果/核心结果一致。结合原1/1通过，tester将001由open改为closed，限固定8e44720及该环境；原失败保留。 |
| 0/1/24/25、长名称 | 0项实际点击提示先加入；唯一152个UTF-16单元长名实际抽取，结果内部可滚动到末尾emoji与“终”，按钮可达。页面handler填24并拒绝25，实际GUI看到编号01～24，实际中心点击旋转并返回第一项长名，关闭卡片后顶部指针落01。填24/尝试25由程序handler执行，非逐项物理输入；不作有限样本均匀性证明。 |
| 保存/页面重进/原样加载 | 21个汉字菜单名实际输入后拒绝并保留副本；20字符保存成功。SDK reLaunch回空副本后，实际点列表/目标，原menuId及面馆、饺子顺序恢复、dirty=false；不是手机结束重开。 |
| 覆盖/另存/重命名 | 实际另存“独立晚餐”新ID/3项，原20字符菜单仍2项；稳定原生选项中实际覆盖保持ID、revision1→2；实际重命名“独立夜宵”保持ID、revision→3，其他菜单不变。 |
| dirty三分支 | 实际取消保留4项dirty副本/原关联；实际放弃恢复另一菜单2项，旧3项保存未覆盖；原2项添加汤后实际保存并切换，持久原菜单为面馆/饺子/汤，当前目标仍面馆/饺子/米饭。SDK对真实快照读取确认了差异。 |
| 写失败保护（注入） | 在真实wx仓储的storage.write前注入一次WRITE_FAILED，程序选择保存并切换；targetLoadCalls=0，四项dirty副本/当前关联/两份旧记录保留，错误“保存未完成，当前内容已保留”。选择/确认函数及写失败均为显式运行时夹具，结束恢复；不是实际存储满或真实系统故障。 |
| 删除/另存、21份菜单（控制器＋真实wx） | 程序确认删除当前菜单，原ID不可加载，候选保留、current=null/dirty=true；再保存获得新ID。通过页面handlers累计21份实际完整快照，无固定20份上限；原其他菜单不变。删除确认是夹具返回true，未计为实际鼠标原生确认通过。 |
| 字素/空覆盖/长名往返（控制器＋真实wx） | 20个家庭emoji字素改名成功，21拒绝且旧名不变；删光当前后空覆盖拒绝，旧3项记录保留；152单元候选完整保存、切走、加载恢复，随后实际鼠标单项结果检查如上。 |

当前AC映射：03/04/09/10/36/37取得上述抽取/数量子项；05/06/14/15/16/17取得所列菜单分支；08/25/27取得模拟器所列布局、容量与长名子项；29/30/31/32/35取得明确注明的控制器/真实wx或注入子项。11只实际输入基本中文，其余输入法/空白/粘贴分支未平台重测；12/28平台重复/同名组合未另跑。07手机留存、18离线、20同域清除后重建/手机清除，以及全部M层均未完成；隔离清理不能冒充这些验收。L层历史及本轮292复跑单列，不自动把27项AC全部判通过。

### 保留的失败、证据缺口与清理

一次原生弹层尚在动画中，鼠标实际落到“另存”，提前按“已覆盖”检查dirty=false而断言失败；随后按真实分支完成另存，重新观察稳定选项后才实际覆盖。该输入/期望偏差不当作新生产缺陷，也不把最初失败改成通过。批量控制器观察器在24秒限时退出1，异步任务继续；随后只读取得status=passed、8组事件及injectionsRestored=true。两次被遮挡窗口截图出现其他前台应用，均弃用，不作点击依据，激活实际微信窗口后恢复。

**执行工具重置导致长期观察器进程及内存中的平台JSON/PNG未落盘。** 目录中没有mouse-two-result/menu20-saved/dirty各分支/24-result等原观察器图片，也没有platform-session.json或原extended-result.json；不得用不存在的文件当证据。上述已执行观察按真实聊天工具响应整理为明确标注的摘录，保留方法、状态和数字，不能称完整原始平台附件。未重跑整段场景来覆盖该缺口；这是本阶段交接的审计限制。desktop TAP、旧失败文件、新清理/CLI记录及当前空页PNG实际存在。

重置后SDK连接失败的原始错误保存在platform-recovery.json；重新CLI auto连接同一候选后，只核对并清理确切前缀eating-everyday:tester-retest-t013-001:1792583020228:。cleanup.json记录移除54个测试key、剩余0；用户v1域清理前后均0key，SHA-256均为4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945。该哈希对照仅覆盖清理，不伪造丢失的整轮初始哈希。重新进入取得ready/PNG/0项/空菜单，after-cleanup-empty.png已实际查看。没有操作其他前缀或用户菜单。

### 当前新预览与用户入口

本成员为同一未改实现独立执行CLI preview，退出0，preview-info.json实际总包**111012字节（108.4KB）**，preview-cli.txt保留本机AppID脱敏后的命令输出。开发121174及旧83913是其他构建记录，不混用为本次大小。新码绝对路径：`C:/Users/YinZh/Documents/workspace/eating-everyday/.local/tester-t013-001/preview.jpg`，已查看、被Git忽略；二维码SHA-256见run.json。未宣称手机已扫码成功或已发布。

用户最少操作：

1. 微信开发者工具导入产品根目录`C:/Users/YinZh/Documents/workspace/eating-everyday`，沿用既有私有AppID配置。
2. 使用基础库3.17.4，点“编译”，查看首页中央“开吃”与候选入口。
3. 点“预览”生成该目录当前版本的新码，或直接打开上述本轮新码。
4. 用有权限的微信“扫一扫”；在手机加入两项后点“开吃”，保存一份菜单，再正常关闭重开并显式加载核对。扫码/设备结果按用户实际反馈另记。

当前“可导入并预览候选”的阶段目标已交接；因手机扫码/进程结束留存、手机键盘字体小屏安全区、真实容量/生命周期、打开后断网和部分原生分支未执行，且本轮原始GUI附件不完整，T-013仍in_progress，不整体completed。治理本地保存后停止，后续由用户反馈/项目经理安排接续；不自动唤醒、跨会话发送、修改他人记录、合并推送、删分支或发布。


## 7. v0.3专项收尾与手机用户自测（2026-10-22）

### 授权、固定版本和边界

项目经理按用户“把我的手机测试结果纳入报告，补齐剩余专项验证，完成T-013”的安排串行交接，输入为本任务两条2026-10-22手机反馈、A-010 v0.1、A-013 v0.2和本报告v0.2。接手/恢复时实际重读TEAM、自身成员/role/scope、testing与微信小程序测试正文、WORKSPACE及当前T-013；OPC HEAD保持cb11072、治理规则和相关输入未变，复用已完整读过且Git范围确认未变的历史。开发及其他成员停止，本成员未联系或唤醒其他会话。

发现产品既有project.config.json由开发者工具写入本机AppID/编译设置，按SESSION_PROTOCOL第6节第4项先协调；用户答复“保留配置，继续测试”。随后物理Esc中断，tester停止自动操作且未把当时空输入点击记通过；用户又明确“允许恢复自动操作，继续补齐剩余专项测试，把手机反馈纳入报告，清理隔离数据并完成本地提交，按实际结果收尾T-013”，才恢复。未修改或提交该公共配置、私有AppID、AppSecret、安全/隐私设置。

| 定位 | 本轮实际值 |
|---|---|
| 本机产品仓库 | git@github.com:yinzhongkai/eating-everyday.git；checkout=C:/Users/YinZh/Documents/workspace/eating-everyday；feature/t-002-readiness-review，目标main。 |
| 固定实现/接手资产 | 8e4472006f1d3cdff5b774052e01ce8db5a96453 / d212b5908484c47906a3eb62f254b74556175a0e。业务与旧测试未改。 |
| 新测试资产 | **10f52d40bb8a62b04a8f84793e44117fe3c28e46**；仅新增test/evidence/tester-t013-special22/49份，含README、14个成功阶段JSON、15张新截图、失败/诊断记录、TAP及字节清单。以下路径均相对该产品目录。 |
| 环境 | Windows11、Node24.19.0、微信开发者工具2.02.2608080、基础库3.17.4、WebView、既有官方SDK0.13.0；初期390×844/视口390×753/DPR3，后实际切为iPhone5模拟器320×568/视口320×504/DPR2，默认字号16。模拟iOS10.0.1/微信8.0.5不当实体手机信息。 |
| 配置/源码指纹 | scope-check.json：37份非test受控文件中，只有已授权保留的配置差异；该文件原始SHA-256始终97f3a1355a54a3c8875f6da96580357c1fbc3a639079e009acf0aa07c6bae2db。其他受控文件与接手Git版本一致；49个暂存blob逐项与保存字节一致，PNG/JPEG未被换行转换。 |
| 主线/发布 | main、既有origin/main、WORKSPACE.revision仍8fa87fe4cb8f7f33aacf2842798a486150a6105e；两仓库均仅本地feature提交，未合入、推送、删分支或发布。新预览未重建，沿用第6节对应未改实现；不将本轮测试证据提交冒充手机新构建。 |

### 手机反馈来源及判定

以下均为**用户自测确认，非tester亲自操作**，不生成或虚构手机截图。前两条来源为[T-013](../TASKS.md#t-013验证小程序导入编译与预览闭环)中项目经理2026-10-22原记录；第三条为本测试会话实际答复。关联最近交付的实现8e44720及d212b59测试资产，手机实际构建指纹未取得，不能声称已在手机核验Git哈希。

| 来源/询问上下文 | 用户实际答复 | 纳入结果与边界 |
|---|---|---|
| 项目经理记录手机基础操作 | “扫码打开、添加候选和转盘抽取正常” | M层基础打开、添加、触控抽取正常；不是tester执行。 |
| 项目经理请测“保存菜单 → 关闭重开 → 加载菜单，以及离线使用” | “测试了正常” | 手机保存、正常关闭重开后显式加载、离线使用正常；未提供断网方式、每个管理子分支或设备日志，不扩大为全部离线故障/设备覆盖。 |
| 本轮一次列出手机剩余三项：中文输入法和粘贴含emoji长名、软键盘下加入/菜单名确认；微信大字后名称和保存/切换/结果按钮可达及底部手势区；至少两项旋转期间切后台再回，无过期结果、候选不丢 | 用户先说“手机上默认测试过吧”；tester仅为澄清事实补问是否包括这三项，用户明确“刚才三项也已实测正常” | 这三项按用户明确自测确认记正常。第一句本身未被当成专项通过；采用其后明确答复。未另提供机型、系统、微信版本、字体具体档位或物理屏幕尺寸，不编造参数。 |

此前“没有任何手机结果”的当前结论已被本节替代，原第1～6节的历史未改。默认流程与新增专项分别留源，不能把用户确认写成tester亲自独立实测或多机型兼容结论。

### 新执行专项、预期与实际

本轮脚本special-session.js按阶段恢复前置并保存证据；真实鼠标动作前断开SDK，操作后离散观察、落盘。夹具明确标注，不把接口故障伪装成真实设备满配额。

| 范围/AC | 前置、步骤及预期 | 实际结果/证据与层级 |
|---|---|---|
| 输入/重复：03/11/12/27、BN-02 | 实际空提交、半/全角纯空白；全选后输入带首尾空白的中文emoji；真实wx.setClipboardData准备同名后Ctrl+V并提交。拒绝应保持列表，有效输入应trim且完整。 | save-input.json四步数量0/0/1/1，唯一名精确为“午餐面 👩🏽‍🍳”，重复提示且输入保留；actual-input-duplicate.png。D真实键鼠；手机IME/软键盘另由上表用户确认。 |
| 菜单名/相等：12/27/28、BN-03 | 空白、trim、重名新建/改名、自身原名、明确覆盖；A/a/全角Ａ/é/e+组合重音。拒绝不覆盖旧记录，允许值按原字符串保持区分。 | name-rules.json共8个检查点通过，7份菜单及7个候选符合预期；menu-name-rules.png。D控制器＋真实wx；20/21字素边界复用第6节，不重复计数。 |
| 原生删除：14/32 | 当前保存2项、dirty副本3项，另有其他菜单；选择删除由_choose夹具，原_confirm弹真实wx.showModal，实际鼠标分别取消/确认。 | native-check-cancel/confirm.json及PNG、native-delete-dialog.jpg；取消业务/持久态全部不变，确认仅删目标、留3项副本/current=null/dirty=true，另一份不变；另存新ID。确认是本轮真实鼠标，另存为控制器操作。 |
| 删除状态组合：13/14/29/32 | 当前/非当前、dirty真/假、最后一份/另有菜单及空副本，6种组合各自取消、确认、再保存。 | delete-matrix-fixture.json 6/6；空副本拒绝保存，旧ID不可加载，非空副本新存新ID，其他菜单不变。D控制器＋真实wx，选择/确认是夹具；不宣称六次实际鼠标弹框。 |
| 清除同域重建：20 | 真正保存两份菜单后只清11个同域测试键，reLaunch后接回同一前缀。应为空且不预填/找回。 | clear-rebuild.json及same-domain-cleared-empty.png：0候选/0菜单、无错误，用户域未变；D真实wx。不是手机卸载或清用户真实数据。 |
| 读写/容量/核验：05/17/31/35、FI-01/02/03/07/08/15 | 两份真实wx记录及dirty副本，分别注入读取、容量估算、写前、实际根已写后报错、根回读、目标读取失败，并恢复重试。 | storage.json共12检查点通过：读错不当空；容量/写前错误保留旧记录/dirty、目标load=0；根真实已落盘时回读确认一次；结果不明冻结操作，重试按实际receipt核验；保存已确认而目标读错时留当前已保存内容。storage-protection.png。其余FI细分复用A-012的L层，不伪装为平台逐项全量注入或物理满盘。 |
| 生命周期：04/09/15、ST-06/RN-06 | 双项真实动画时调用onHide，再onShow；另延迟随机端口，真实reLaunch卸载旧页，再送达旧熵。应取消过期结果并不改新页。 | lifecycle.json及lifecycle-fresh-page.png：隐藏后停止动画、无结果、候选保留；旧页alive=false、定时器清除，新页保持0项/无结果。隐藏/显示是处理器夹具，卸载是真实路由；手机动画切后台另由用户确认。 |
| 小屏/长名：08/25/27/36 | 实际机型320×568，夹具24项含长名；观察01～24、滚到底、实际点保存；受控熵0抽第一项长名。 | small-24-top/bottom、small-save-dialog、small-long-result的JSON/PNG实存并已查看：编号可辨，第24项和保存按钮可达，保存弹框/结果按钮在视口内；完整结果字符串保留，结果区有内部滚动条。本次未滚到结果尾，不重复冒充第6节已观察的长结果末尾；字体仍16，D大字设置未验证生效，手机大字/安全区由用户确认。 |

第6节有效的0/1/24/25、20/21字素、覆盖/另存/重命名、dirty三分支、21份菜单、长名保存加载和001关闭结果按其原层级复用，本轮新证据不伪造为当时丢失的原始GUI附件。27项有效AC未新增或恢复已撤销编号；不同层级、检查点和回归次数不相加成“独立通过总数”。

### 回归、工具失败和用例修正

| 命令/环境 | 实际结果及依据 |
|---|---|
| 当前工作树：node --test --test-reporter=tap test/*.test.js test/independent/*.test.js | **退出1，291/292**。combined-292.tap中唯一失败为miniprogram-package.test.js要求公共appid=“touristappid”，而用户已授权保留工具写入的本机AppID。未改生产或旧断言，不写成292通过。AppID文字按evidence-redactions.json脱敏，原/脱敏SHA保留；原输出在忽略的.local/tester-t013-special22/working-config-292-original.tap。 |
| 未改提交基线：git archive d212b59导出到产品.local新目录，执行相同命令 | **退出0，292/292**，失败/取消/跳过/todo均0；candidate-baseline-292.tap、baseline-export.json保存精确提交、153文件导出及tar SHA-256。导出目录验证后已删除。该结果限定Git候选，不冒充带本机配置差异的工作树通过。 |

native-delete.json保留SDK取消返回后等待未结束的observer timeout，随后改真实鼠标取得成功，不倒写初次失败。组合用例首轮误将非当前菜单删除后的内部generation也要求不变，前三组合有记录、第四失败；delete-matrix-initial-failure.json和原脚本保留。noncurrent-oracle-probe.json独立诊断仅generation变，业务与另一份记录不变；oracle-correction.json保留旧/新断言与需求依据，修正后6/6，不放宽候选、关联、基线、dirty和其他记录不变的业务要求。

后续SDK单次求值超时、IPv4连接失败和CLI恢复输出均保存，最终使用可轮询异步任务。sdk-loopback-probe.json证明同一端口IPv6连接可用，最后6/6及清理用ws://[::1]:9420完成；未改端口/安全设置。机型菜单一次坐标越界未执行、一次鼠标选择未生效，后用键盘切换并由API核验320宽。窗口捕获数次出现其他前台应用，图已弃用并停止该GUI路线；不把大字设置记为已在D层生效。以上工具/用例问题不当新生产缺陷，也不删原记录。

### 清理、完成判断及剩余限制

bootstrap记录开测前真实用户域0键及SHA-256；最终cleanup.json记录仅删除唯一前缀eating-everyday:tester-special-t013-22:1792638148174:及其子域共78键，剩余0。开测前后真实用户域均0键，SHA-256均4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945；after-cleanup.png显示重新进入空页。方法夹具均恢复或随真实重进销毁，未清用户真实菜单；临时基线导出也已清理，证据及忽略的脱敏前原日志保留。

**T-013按原约定范围标completed。** 导入/编译/预览证据和001独立闭环沿用有效历史；剩余输入、同名、原生删除、隔离清除、故障保护、生命周期及小屏D检查已取得本轮证据；手机基础/留存/离线和新增三专项有用户明确自测正常的反馈，未发现新增阻断生产缺陷。测试资产已本地提交10f52d4，报告/本人任务及A-014索引本地提交后tester停止，由项目经理按串行约定安排后续架构复核；不自动联系其他会话。

保留的局限包括：未取得手机机型/系统/微信版本/精确构建指纹，手机结论来自用户而非tester独立操作；不扩展为多机型、全部字号、极端长名或真实满配额/进程强杀/设备清库的覆盖。物理配额不以破坏用户数据方式制造；故障注入只证明其应用路径。第6节旧原始GUI附件丢失仍未补回，本轮新附件只支持本轮结果。上述限制不隐去、不记为失败或虚构通过；本次任务完成不批准需求/架构整份成果，不改T-002、STATUS、WORKSPACE、他人结论或发布状态。


## 8. v0.4 AppID配置独立复测与合入判断（T-015，2026-10-22）

### 8.1 授权、来源与固定版本

项目经理按用户“先安排开发和测试处理AppID配置导致的测试差异，复测通过后把代码合并并推送GitHub，我来准备正式小程序账号”的明确授权串行安排T-015；developer已提交T-014并停止。tester实际刷新自身登记/角色/scope、testing及微信小程序测试正文、WORKSPACE、当前任务和A-013 v0.3第6节，读取A-006 v0.5/A-008 v0.3及本报告历史。已读未变治理规则按Git范围核对复用；既有公共配置保留授权继续有效，无须重新协调，不更改其内容。

| 定位 | 本轮实际版本与边界 |
|---|---|
| OPC接手 | 5c5d2ac4c008ec8c96d1789ddac6cd967a2f212c；feature/eating-everyday/t-002-readiness-review；目标project/eating-everyday，合入前目标a735031c37c09e75fb9478681aaf42010a82f378。 |
| 产品仓库 | git@github.com:yinzhongkai/eating-everyday.git；本机C:/Users/YinZh/Documents/workspace/eating-everyday；feature/t-002-readiness-review，目标main。 |
| 固定候选/配置测试源码 | 接手与导出固定aa8c6d83946ca7ec0ebe420c408a9511799c77ea；测试源码修正ac9ae93596633919e527934b5c5f64c40112b49a，后续ebc4e7d/aa8c6d8仅开发证据。 |
| 业务实现 | 仍为8e4472006f1d3cdff5b774052e01ce8db5a96453。相对T-013资产10f52d4，作者源码仅改三份test文件，其他增量均为测试证据；本轮未改这些文件或业务。 |
| 新tester资产 | **f642dcad85d86d4c1adf251fbd2dad6d8cc84065**；仅新增test/evidence/tester-t015-appid/16份日志、脚本、元数据与清单，父为固定aa8c6d8。以下文件名均位于该产品目录。 |
| 执行环境 | Windows、Node v24.19.0、现有Git/Python，无新增依赖。本轮没有GUI、微信CLI/SDK、手机或存储域操作。 |

### 8.2 两配置完整回归分别通过

在保留用户实际运行配置的当前工作树、以及`git archive --format=tar aa8c6d83946ca7ec0ebe420c408a9511799c77ea`解出的新空目录，实际分别执行完全相同命令：

```text
node --test --test-reporter=tap test/*.test.js test/independent/*.test.js
```

| 环境与配置来源 | 预期 | 实际与原始证据 |
|---|---|---|
| 工作树：默认读取git:HEAD:project.config.json，本机运行配置直接读取既有文件 | 仓库默认仍为touristappid；合法本机AppID不阻断后续工程断言 | **299/299，退出0**；失败/取消/跳过/todo均0。working.tap、run.json；默认来源与实际本机配置分开验证。 |
| 固定候选新导出：不含.git/私有配置；默认读取snapshot:project.config.json | 使用自身快照占位配置，不借用父仓库HEAD | **299/299，退出0**；失败/取消/跳过/todo均0。clean-export.tap、run.json含tar SHA-256及来源核验。 |

两处四个工程包测试均有真实`ok`记录：默认可移植与打包、实际app/page和本机配置、可达本地JS依赖及Unicode等价、可见转盘及离屏canvas保护。不是只让AppID断言通过后提前结束。相对旧292项，作者新增默认来源1项及合法/非法等6项，共299项；两类执行不合计成一次598项验收。第7节历史291/292与292/292、原TAP和断言均保留。

### 8.3 独立反例与原工程保护有效性

源码复核确认：仓库来源只接受touristappid；运行来源只接受该占位值或符合`^wx[0-9a-f]{16}$`的原始字符串；未知来源先拒绝。根目录./、miniprogram编译类型、3.17.4基础库与六个具有正确folder/file类型的必需排除项继续受保护。本轮没有替作者修测试或放宽原断言。

独立脚本independent-checks.cjs单独执行，**85/85，退出0**（independent-checks.json）：覆盖合法占位/合成wx值、空白/非字符串/错误长度大小写/尾部换行及Unicode行分隔符、未知来源及错误消息不泄露输入；三类工程字段、全部六项排除缺失或错类型、错误容器均拒绝。嵌在产品仓库下但无.git的探针读出自身哨兵配置，缺失/坏JSON不从父Git回退；显式无效.git来源同样关闭失败。这些为独立辅助检查，不混入完整299的计数。

另只在临时导出内逐次注入六类错误，以`node --test --test-reporter=tap test/miniprogram-package.test.js`执行：

| 注入错误 | 预期被拒绝的位置 | 实际 |
|---|---|---|
| app.pages错入口 | 原实际app/page复合检查 | 退出1，3通过/1失败，目标拒绝。 |
| WXML不存在的事件handler | 同一复合检查的事件绑定 | 退出1，3/1，目标拒绝。 |
| 删除页面json文件 | 同一复合检查的完整页面文件 | 退出1，3/1，目标拒绝。 |
| 页面引入不存在的本地JS | 可达依赖检查，实际页面导入也失败 | 退出1，2/2，两个目标拒绝。 |
| 改写生成Unicode模块 | 原生成数据等价检查 | 退出1，3/1，目标拒绝。 |
| 移除可见转盘旋转样式 | 原可见转盘/渲染保护 | 退出1，3/1，目标拒绝。 |

mutation-*.tap是**预期拒绝**日志，不能误算为候选失败；均无skip。每项之后恢复原字节，最后将整个导出树逐文件与原tar内容比较一致，才删除新目录。六项都命中具体目标；没有修改实际工作树源码。

### 8.4 原始指纹、数据隔离与提交边界

before.json记录开测前全部211份受控文件SHA-256；run.json确认结束时全部相同，包括原测试/历史证据。公共project.config.json原始SHA-256前后均**97f3a1355a54a3c8875f6da96580357c1fbc3a639079e009acf0aa07c6bae2db**；私有project.private.config.json只按原始字节计算，前后均**988e8a38615d839bbe92a89bf5b255e6ccb7be5c218ab1b8bbc71593870ccff4**，未读取密钥字段。Git受控默认原始SHA-256仍76d9f0314ad94472f55acb4fc07f4dcb44388f736b6eae7a250526f99161613e，内容仍touristappid。

实际本机AppID只在内存中与作者变更blob、新证据和暂存blob比对，未输出实际值且未检出泄漏；私有配置保持忽略/未跟踪。新16份暂存内容逐一与原保存字节相等；manifest.json记录其余15份文件哈希与长度。现有公共配置未覆盖、暂存或提交，产品提交后仍仅它为已授权未提交差异。未接触用户菜单或真实存储，无需清除真实数据。

临时目录均为本轮新建于产品.local下的tester-t015随机唯一目录，解包前逐成员检查路径及文件类型，清理前核对绝对路径/父目录及本轮名称；run.json确认导出和来源探针均已删除。没有删除旧证据、本机私有文件或其他隔离域。

### 8.5 结论与下一位行动人

**T-015 completed；独立测试结论为可合入，无新增阻塞。** 两种配置均完整通过，非法输入和包保护仍有效，个人配置及历史证据保持。T-013已completed和手机用户自测正常结论继续有效；手机环境/构建未提供、多设备/真实满配额及旧GUI附件缺口仍按第7节限制保留。本轮不增加真机或微信正式发布声明，正式账号由用户另行准备，不构成本次Git交付前置。

tester仅维护本人T-015、本报告及A-014索引，产品/治理本地提交并核对后停止。两个目标分支和WORKSPACE仍保持既有基线；project-manager依据本次精确版本串行执行T-016合入/推送。tester不跨会话通知、不修改他人结论/STATUS/WORKSPACE，不合并、推送或清理分支。


## 9. v0.5 默认菜单独立验证暂停检查点（T-023，2026-11-02）

### 9.1 授权、固定输入与保护

依据TASKS开头用户授权的T-020→T-021→T-022→T-023串行安排及project-manager通知，tester在上游及项目经理停止写入后实际接收。完整刷新自身登记/scope、active角色及testing、wechat-miniprogram-testing正文；根公共协议经Git核对未变。完整读取A-001 v0.10/A-007 v0.4（12/12/32）、A-006 v0.6第10节DM-01～DM-12、A-008 v0.4第0节、A-013 v0.4第7节、D-019及上下游任务。旧摘要与旧299/299不作为本轮输入或通过依据。

| 定位 | 本次实际值 |
|---|---|
| OPC接手 | feature/eating-everyday/default-menu，f404764a8871097bef2f9b0b24d1ce99ccf6768d；目标project/eating-everyday仍8474050eb2ddf6b1a414f649c4ba242636ecd4e8。 |
| 产品仓库 | 本机C:/Users/YinZh/Documents/workspace/eating-everyday；origin=git@github.com:yinzhongkai/eating-everyday.git；feature/default-menu。 |
| 被测代码/接手资产 | 固定代码dcb22d3bdcec196e37b544805f79196de126f676；接手完整资产33e3e64cd3e59d48adbd13d69622a82087468016，相对固定代码仅developer证据，不改运行源码或测试。 |
| 本人暂停资产 | c5a3988c78734feb733b11c31cd70ffdaffb4b17及原始证据字节修正eb4f6838ecd17a69bf9690cd4abbf542eef6fe69；只新增本人独立测试及test/evidence/tester-t023-default-menu/，未改生产/作者测试或旧证据。治理提交以T-023/Git历史定位。 |
| 目标与配置 | main/WORKSPACE仍6c9ef2b3135a0013fb0b06ebbe761b298c4b7260，未合入。公共配置SHA-256 eecc59987ea459258c2bf0329b69d98e73e790dfa6110d60f28ed3645456e645、私有df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d均与开测前一致；公共既有差异未暂存提交，私有保持忽略。 |
| 实际环境 | Windows 11 10.0.26300，Node v24.22.1；真实DevTools基础库3.17.4、现有官方SDK0.13.0。390×753/DPR3及iPhone/iOS/微信字段为模拟器报告，不是实体手机参数。 |

开测前245份原受控文件逐原始字节建立protected-before.json，暂停时全部指纹相同，包括被测实现、作者测试、原tester资产和developer交接证据。证据manifest记录原始字节；本目录用局部.gitattributes避免Git换行转换改变证据。原始日志及中间文件均在产品仓库；本次未读取AppSecret或修改用户配置。

### 9.2 实际执行及独立覆盖

以下路径均相对产品仓库，当前证据目录为test/evidence/tester-t023-default-menu/。独立文件是test/independent/default-menu.acceptance.test.js，使用独立编写的AuditPort、固定期望及行为断言；未修改被测实现或作者测试取得通过。

| 已执行层级/命令 | 实际结果与证据 |
|---|---|
| node --test --test-reporter=tap test/independent/default-menu.acceptance.test.js | 最终129/129：49顶层＋80故障子测试；失败/取消/跳过/todo均0，independent.tap。初次123/123保留independent-first.tap，随后增加4项容量及2项旧块损坏检查，不改写初次结果。 |
| node --test --test-reporter=tap test/*.test.js test/independent/*.test.js；保留配置工作树 | 501/501，退出0；原372＋独立129；四类非通过均0，working.tap/desktop.json。原作者默认菜单与受影响菜单/抽取/存储/字符/原生工程测试全部实际执行。 |
| 同一完整命令；固定接手资产git archive导出 | 501/501，退出0；四类非通过均0，clean-export.tap/desktop.json。导出无.git/私有配置，仅额外复制与当前字节一致的独立测试文件，哈希和archive指纹均记录；两次运行不相加为1002个独立用例。 |
| 真实DevTools初始化失败观察 | native-before.json：页面注册前注入独立WxStorage前缀；首次content.write受控抛STORAGE_FULL后，ready=false、count=0、菜单数0、classification=PENDING，显示“空间不足，原菜单和当前内容已保留”，retryEvents=0。SDK截图retry-before.png已保存；Computer Use未取得窗口访问，未视检或点击，不宣称恢复成功。 |

| 风险组 | 独立桌面断言及范围 |
|---|---|
| DM-01/05 | 唯一“默认菜单”、8项原顺序、自动关联dirty=false、保存重载一致；16次受控索引0～7逐项及重复，抽取不写存储；重复调用coalesce及再次启动不强载。受控抽样用于路径断言，不单独证明等概率。 |
| DM-02/03/04 | 1份/4份旧菜单含同名不同内容、旧genesis有/无孤块、含删除回执的有效空索引；启动前后根及全部原数据字节相同、零写入、不播种、不强制加载。 |
| DM-06 | 编辑/覆盖/改名/另存及取消/确认删除，删最后一份仍留9项未保存副本且解除关联；0/1/24/25、空保存并切换失败、原8项快照保持。 |
| DM-07 | info/根/索引/内容读取失败，坏JSON/校验/旧活块、无根残块、本域未知key、未知schema/策略/状态及元数据组合；拒绝并保留全部原字节，零播种。 |
| DM-08/09 | 强制多内容/索引块，20个可观察准备/回读/根切换/可达核验边界×应用前后×普通失败/模型中断，共80子测试；半份不可加载，pending不可被普通CRUD绕过，新实例先全量核验再原menuId续接。另覆盖4类容量失败；根已落盘但反馈读失败时冻结编辑/写入/切换，持续核验失败仍冻结，解除故障后同启动链安全恢复且无额外写入。 |
| DM-10 | 结果丢失后用户覆盖/改名/删除，旧恢复快照不覆盖或复活；延迟根完成遇generation改变或页面失活不应用迟到自动加载。 |
| DM-11/12 | 仅全域缺失可播种、删根残块拒绝、普通写先入队保留用户菜单、根guard变更拒绝覆盖；4类CRUD拒绝保留初始化operationId，终态及回执在CRUD后仍保留。外域隔离实际平台补验尚未执行，不能据此声称本轮已实测用户域内容保持。 |

以上L层故障是端口模型及确定性注入，不是物理满配额、真实强杀/断电、跨进程CAS或实体离线结果。真实wx目前只完成表中的失败观察，未把开发者六组冒烟记作tester执行。

### 9.3 用户暂缓、窗口权限与剩余项

官方CLI最初被沙箱禁止写本机.cli运行文件，SDK初次连接本机9424端口亦失败；依正常工具流程取得最小权限后CLI及SDK连接成功，失败日志和脱敏CLI记录保留。它们不是产品缺陷。随后Computer Use选择本轮隔离窗口时明确返回“Computer Use was not approved to use 微信开发者工具”；没有实际点击，也未改用SDK去绕过被拒的同一UI动作。

用户随后明确“等空闲的时候，你在进行测试吧”。本成员立即停止后续测试，只保存既有结果、检查点及本地提交；不安排自动空闲检测或定时续跑。T-023保持in_progress（用户暂缓），当前未发现新产品缺陷，但平台覆盖尚未完成，**不提供本轮通过/可合入结论**。

待用户空闲后，先按协议刷新身份、两仓库、固定代码、配置指纹和隔离工程状态，再继续：

1. 在独立工程实际点击现有重试按钮，核对onRetry只触发一次、原pending menuId不变、唯一完整8项自动加载、dirty=false、真实渲染和保存重载；保存前后证据。当前retryEvents仍0，不能用预置脚本或SDK直接调用冒充点击。
2. 按风险补齐本轮独立真实wx/页面的旧同名及空库、CRUD/删除重启、读异常与结果不明恢复等差异；native-session.cjs的after/platform模式只是已准备，尚未执行。
3. 只清本次前缀eating-everyday:tester-t023:086867d1f4634ab686dd7ee08a8b577d:下隔离key并核对剩余0，不清用户菜单。当前pending隔离数据和.local/tester-t023/导出/临时工程保留待续，cleanup模式未执行，未宣称已清理或已关闭该工程。

未新增手机扫码、本次实体离线首次、物理故障、多设备及旧程序降级兼容证据；旧T-013用户手机结论不覆盖本次默认菜单。这些层级按实际差异记录，当前不新增未约定的全设备门槛或重复索取业务选择/账号。

### 9.4 提交与停止边界

本轮产品只保存本人新增独立测试/证据，OPC只改T-023、本报告和A-014索引；原第1～8节专业历史保持。未改STATUS/PROJECT/TEAM/WORKSPACE、需求/架构/作者交接或他人任务，未联系其他会话。两仓库仍停留本轮feature；不合入主线、推送、打移标签、创建GitHub Release或微信发布。用户准备好后在本会话继续，项目经理可只读核对暂停检查点，不能把本轮已通过子项当作任务完成或成果批准。


## 10. v0.6 恢复后独立平台验证完成（T-023，2026-11-02）

### 10.1 当前授权、版本与保护

用户在project-manager会话明确“现在电脑空闲了，通知测试工程师继续完成剩余的平台交互测试”，解除此前暂停；TASKS开头保存实际授权，项目经理提交后停止写入并通知本成员。tester实际刷新本人登记/role/scope、testing和wechat-miniprogram-testing完整正文、WORKSPACE及T-023/第9节；规则经Git比较未变，现行五份需求/技术输入LF指纹与已完整读取的版本一致，沿用12/12/32和DM-01～12。其余成员均已停止，未启动新成员或任务。

| 定位 | 本次实际版本 |
|---|---|
| OPC接手 | 7a786103680d55c530f091694461bb6b024dd3c9，feature/eating-everyday/default-menu；目标project/eating-everyday仍8474050eb2ddf6b1a414f649c4ba242636ecd4e8。 |
| 产品接手 | eb4f6838ecd17a69bf9690cd4abbf542eef6fe69，feature/default-menu；固定被测代码仍dcb22d3bdcec196e37b544805f79196de126f676。 |
| 本次新测试证据 | 产品12649a797966299c4f23b6d4a4b9f0b3d2d1619e，仅新增test/evidence/tester-t023-default-menu/resume-20261102/共25份文件；未改实现、作者测试或旧证据。OPC本人提交由T-023及Git历史定位，不自写自身哈希。 |
| 目标基线 | 产品main/WORKSPACE仍6c9ef2b3135a0013fb0b06ebbe761b298c4b7260；本轮未合入、推送或推进版本基线，标签固定。 |
| 配置保护 | 公共SHA-256 eecc59987ea459258c2bf0329b69d98e73e790dfa6110d60f28ed3645456e645；私有df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d，均与恢复前相同。公共既有差异未夹带提交，私有保持忽略。 |
| 环境 | Windows 11 10.0.26300、Node v24.22.1；实际视检DevTools Stable 2.02.2608080，基础库3.17.4、SDK0.13.0。模拟器390×753/DPR3，不据其iPhone/iOS/hostVersion字段声称实体设备环境。 |

resume-before.json记录恢复前262份受控文件指纹和五份输入LF指纹；结束核对全部原字节一致。临时工程src/adapters/ui与注入后的注册入口共17份运行文件也保持原指纹。仅隔离夹具在内存中恢复其已消费的一次性故障钩子，未修改磁盘中的被测代码。原129/129和两环境各501/501按固定版本保留，未无理由重跑，更不把这些计数说成新增平台结果。

### 10.2 真实按钮、渲染与保存重载

本次证据以下均位于产品test/evidence/tester-t023-default-menu/resume-20261102/。此前第9节及父目录暂停、失败和权限记录完整保留。

| 实际动作与观察 | 结果及原始证据 |
|---|---|
| 恢复前SDK只读核对 | 原页面ready=false/count=0、PENDING、菜单数0，原menuId为menu-mvywijca-0-vk6qjtp01bj、retryEvents=0；native-probe-*.json、resume-before.png。 |
| 用户授权官方CLI重新打开隔离工程 | 隔离目录及前缀先核对；重新打开后窗口为1118522。测试模块重新加载，pending未显式核验时显示BUSY及“重新读取”，符合契约。只移除已在暂停前消费、因模块重载而重新挂载的content.write单次故障wrapper，露出原生产WxStorage原型方法；根字节、menuId及retryEvents=0均未变。native-restore-hook-*.json、脱敏official-reopen-redacted.txt。此步骤不调用onRetry或改持久数据。 |
| Computer Use实际置前、视检并鼠标点击“重新读取” | 单次点击[869,396]，没有SDK直接调用或合成事件替代。native-after-*.json确认onRetry恰1次、原menuId不变、INITIALIZED/storeRevision=1、唯一一份“默认菜单”、8项完整原顺序、dirty=false且wheelReady=true；retry-after.png及实际窗口视检均显示真实转盘。 |
| 实际点击“切换/管理”及已保存默认菜单行 | 视检列表只有一份8项默认菜单，点击该行后完整恢复；native-ui-reload-*.json确认保存内容仍为原8项、同menuId/revision=1、dirty=false，retryEvents仍1；ui-reload.png。 |
| 实际点击“开吃”及关闭结果 | 视检转盘动画、指针及完整结果“西域美食”；native-ui-draw-*.json确认index=2、候选范围正确且保存根与重载前逐字一致，ui-draw.png。实际鼠标关闭结果。有限抽取仅验证功能/对齐，不单独证明等概率。 |
| SDK明确执行真实页面reLaunch | native-restart-*.json确认菜单仍唯一但当前count=0，不重复播种、不强制再加载；restart-manual.png。此动作如实记为SDK导航，不冒充物理点击或强杀重启。 |
| SDK明确操作实际页面Session后再reLaunch | 加载后增加“独立页面自选”、覆盖、改名为“独立页面改名”再确认删除；保存内容9项正确，删除后当前9项未保存副本保留且解除关联，菜单数0，complete终态保留；原初始化/恢复调用不改变根。重进后ready=true/count=0/menus=0/dirty=false、INITIALIZED/complete，不复活。native-page-delete-restart-*.json、page-after-delete.png、page-deleted-restart.png，最终空页面已实际视检。 |

默认菜单名称及顺序严格为：肯德基、麦当劳、西域美食、醉面、吉野家、阿香米线、喜家德、麻辣烫。测试页面在注册前就注入本次前缀，没有先访问用户域再切换。

### 10.3 独立真实wx差异与层级

native-platform-*.json的8组与platform-extra.json的4组均实际执行通过，0运行异常。使用本次前缀下独立子域；进入前构造隔离WxStorage，未读取用户域值。

| 组别 | 实际平台结果 |
|---|---|
| 1 首次/熵源 | 真实WxStorage上的Startup自动保存并加载唯一原8项、干净基线，真实wx随机结果属于当前候选，抽取前后保存字节不变。 |
| 2 编辑删除终态 | 真实wx覆盖/重命名/删除后保留9项未保存副本，complete未丢；重复初始化/恢复不复活或改根。 |
| 3 旧多菜单/同名 | “默认菜单”含用户自有不同内容，另有第二份菜单；启动前后全部原字节相同、不强制加载。 |
| 4 已提交有效空库 | 旧菜单删除后的空索引保持原字节，未播种。 |
| 5/6 格式/读错误 | 坏根不播种并保留原字节；在真实wx数据上注入读取失败，报READ_FAILED且原字节保持。 |
| 7 结果不明 | complete根已真实落盘，注入后续读失败得到STORAGE_UNCERTAIN并冻结；解除注入后显式核验/原意图恢复，同menuId、仍一份完整菜单。属于故障注入的应用路径，不是物理故障。 |
| 8 外域隔离 | 仅新建并核对本成员拥有的foreign sentinel；仓储测试不修改它，不读取其他用户域内容。 |
| 9/10 旧genesis | revision=0/index=null分别有/无旧准备孤块，真实wx读取均走legacy_empty，全部字节不变。 |
| 11 回调失败与缺失区别 | 真实wx成功列出已有key后，隔离代理注入getStorage fail回调；得到READ_FAILED、零写入、旧域字节不变，未将异常当空。 |
| 12 真实缺失/残块 | 真实适配读取不存在根返回undefined；存在本域孤块后不再是fresh，报STORAGE_CORRUPT，不清理或播种。 |

上述平台检查补齐第9.3节剩余的默认初始化/旧数据/异常/恢复差异，结合既有独立129项及受影响全套501项判断。桌面逐块/迟到/guard/保留ID证据继续有效，未把开发者六组冒烟或历史手机反馈当本人本次执行。

### 10.4 清理、访问过程与完成判断

平台夹具清理27个key、额外差异夹具7个key，两次均确认自己的子域剩余0，并核对原PENDING/menuId/事件计数未变后才进入按钮测试。最终native-cleanup-*.json仅删除本次前缀eating-everyday:tester-t023:086867d1f4634ab686dd7ee08a8b577d:下剩余7个key，**剩余0、非测试key删除0**。不清用户菜单/其他域、私有配置或原.tmp/。随后对隔离窗口执行CUA Alt+F4，最新窗口清单已无该隔离工程，仅留开发者工具项目列表；测试临时导出/工程文件保留在产品忽略目录供复现，不冒充已删除。

本次窗口审批曾超时；用户实际回复“已允许”。隔离窗口随后被判最小化，尝试读取主工程以寻找窗口切换入口时，被自动审批以“超出隔离授权、可能暴露私有应用内容”为由拒绝，未取得该窗口内容或输入。用户明确要求“要不你自己重新打开一下”后，才用官方CLI重新打开已验证的隔离目录并绑定新窗口，所有成功鼠标动作均在隔离工程中完成；未重试或绕过主窗口拒绝。ui-access-events.json保留这些过程，不把权限/夹具重载问题记为产品缺陷。

**结论：T-023已满足原完成条件，completed；独立结论为可合入，无新产品缺陷和当前测试阻塞。** 原v0.5暂停已由用户授权解除且剩余事项实际完成。合入建议限定代码dcb22d3bdcec196e37b544805f79196de126f676及本轮约定范围；成果仍draft，不等于批准整份成果或正式发布。

未新增实体手机扫码/离线首次、物理满配额/强杀/断电、多设备、旧程序降级写入兼容或跨进程CAS结论；SDK/模拟器及故障注入不冒充这些层级。既有手机反馈继续按原版本和来源保留。上述限制不被伪写为通过，也不机械新增未约定全设备门槛或重复索取业务选择/账号。

仅提交本人新增产品证据及OPC本任务/本报告/A-014索引，原报告第1～9节、旧证据和原受控文件保持。适当治理/范围/字节检查完成后本地提交并停止，不联系其他会话、不改STATUS/PROJECT/TEAM/WORKSPACE及他人记录、不合入/推送/打移标签/创建Release或微信发布。下一位由project-manager只读核对本次回执后按权限汇总协调，不把测试结论自动变成合入授权。


## 11. v0.7 启动恢复独立测试阻塞报告（T-035，2026-11-03）

### 11.1 实际接收、输入与精确版本

本成员依据D-024及project-manager按人类用户既有“安排对应成员开发”“直接给各成员发消息”授权发送的串行通知实际接收T-035；developer及项目经理均已完成写入并停止，未唤醒其他成员。本人TEAM登记、tester角色/scope、testing与wechat-miniprogram-testing完整刷新，根公共协议经与已读版本Git比较未变。按当前记录加载T-032～T-036、D-024、A-001 v0.11/A-007 v0.5、A-006 v0.7第11节/A-008 v0.5、A-013 v0.5第8节、产品docs/core-api.md适用入口和本轮开发证据README；13/13/40、SR-01～13为当前范围。A-007已读历史正文经比较未变，旧成果/决定仅按依赖追溯，旧默认菜单分支/候选未沿用。

| 定位 | 本次实际版本/边界 |
|---|---|
| OPC接手 | b8447b991ee56e99ee0053f4f96f353c2220b96f；feature/eating-everyday/t-032-startup-restore；目标project/eating-everyday仍2e241e6b489a65701ba2078640b2794cdabba429，原.tmp/保留。 |
| 产品接手/被测代码 | 本机C:/Users/YinZh/Documents/workspace/eating-everyday，origin为git@github.com:yinzhongkai/eating-everyday.git；feature/t-032-startup-restore。完整资产9ab18b3e283b72f576055547453fbcfa5b192bd3；固定代码/测试/说明e3ce8cfecde344d3bd7c4c003ee427696fd9a877。已自行核对差异仅developer-t034-startup-restore证据目录。 |
| 本人新测试资产 | 48469cfa7594b127b0af549b9ecafe0698b47cc9；新增test/independent/startup-restore.acceptance.test.js及test/evidence/tester-t035-startup-restore/，共9份文件。故意保留失败断言供修复/复测，不改产品取得通过。 |
| 目标基线 | 产品main及WORKSPACE仍3800e6dd8bc32b7d767bfc873a4b9ae57d10232b；本轮未合入或推送，未移动标签/改WORKSPACE或清理开发分支。 |
| 配置及原证据保护 | 公共SHA-256 27a614c100e150979107c9f648b67f757d81b586883c9eb89a46f2670c143bbf；私有df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d，前后相同。313份接手时受控文件逐字节相同，包括产品实现、作者测试、旧独立及历史证据；未读取私有凭据字段。 |
| 环境与方法 | 实际Windows 11 10.0.26300，Node v24.22.1。独立编写StoragePort、预期及场景，通过真实MenuController和createPageDefinition/handler执行；不是仅原StartupCoordinator，不复制开发断言，不将模型称为真实wx/模拟器UI/实体手机。 |

当前四份需求/技术正文LF SHA-256与T-032/T-033一致：A-001为989002414d6817db1b5539a4e27757972dac0408a0ee054741a0d18d1babb454，A-007为4864d15138e96d90d83ae5f17c728af6e13f5953ebcab8c30e51b034babffcdd，A-006为f31ca08f9af88c7ee989065ebb785d3cac2e77cec6f3339b50ad119cac21f511，A-008为0e6b9b5b0a350f257574b4a971850dbe486a2abd8d4e45b06a3e60ff984fd6ef。protected-before.json记录全部输入及文件指纹，run.json记录实际执行和未执行范围，manifest.json保存原始证据哈希。Node失败TAP中的诊断空格原样保留，仅证据目录通过局部属性声明日志空白，不清理或改写原日志。

### 11.2 已执行、失败和未执行

以下路径相对产品仓库，证据目录为test/evidence/tester-t035-startup-restore/。

| 独立检查 | 实际结果 |
|---|---|
| MenuController稳定ID及最新快照 | 通过：选择B、改名/覆盖保存后新建独立repo/Session/控制器，恢复B最新3项、现名和干净基线；恢复不改菜单或选择原字节。 |
| 单次记忆写失败的普通补记 | 通过：B加载后记忆写失败，再加未保存编辑；解除故障仅补记成功，原菜单根、编辑副本及dirty全部不变。 |
| 旧失败意图＋新不可用选择的组合 | 失败：B记忆写失败留下pending B；C成功加载但选择读失败；解除故障后retry返回superseded旧B，持久选择仍A，新控制器恢复A。日志independent-first.tap为三项首轮，2通过/1失败，退出1，取消/跳过/todo均0。 |
| 实际页面handler最小确认 | 失败：使用真实createPageDefinition、onLoad/onSelectMenu/onRetrySelection/onUnload，独立隔离Port；页面当前为C，重试后selectionWarning和error均空，持久仍A，新页面显示默认A的8项。page-minimal.tap仅定向执行该新增页面用例，0通过/1失败、退出1，取消/跳过/todo均0；不是第二个缺陷。 |
| 全套及干净导出回归 | **未执行**：按T-035明确“若缺陷则记录并停止”约定，已确认最小复现后停止，等待串行修复。新增页面用例后的四项合并命令也未重跑，不把两个调用合并成已执行的单次套件。 |
| 独立真实wx/DevTools/可见UI/手机 | **未执行**：未启动工具、分配真实测试域、生成预览或操作窗口，不能引用developer九组冒烟或559/559为独立结果。无实际用户菜单读取/清除，未创建真实wx测试key，物理域清理不适用。 |

新用例文件只覆盖已实际执行的部分风险；SR-03～09/11/13及其他组合未完整执行，SR-01/02与简单SR-10的通过不扩成整组/整体验收。实体手机、实体离线、物理满配额/强杀/断电、多设备、降级写入及多进程CAS均未新增验证，旧版本按原契约的通过和手机来源保留。

### 11.3 BUG-T035-001（P2/open）：旧失败意图遮蔽当前选择补记，警示被静默清除

依据：A-001 R-18/P-15、AC-49/50，A-006第11.4/11.6节及SR-10/SR-12。错误不造成菜单内容丢失；它使用户在C上重试后失去正确失败提示，下一次启动仍回到旧A，违反本轮启动选择恢复和当前意图授权契约，因此本轮暂不可合入。

**前置与最小步骤：**

1. 经应用控制器/实际页面启动，A已正常加载并记住（选择revision=1）；准备另两份保存菜单B、C，不改变已有记忆。
2. 加载B，在selection.write前注入明确失败。B加载成功，显示未记住，持久记忆仍A，pending意图为B。
3. 加载C，仅在selection.read注入短暂读取失败。菜单库健康、C加载成功，提示选择暂不可用。
4. 解除全部故障，执行当前菜单的记忆专用重试；再读选择，并创建真正新控制器/页面。

**预期：** 旧B意图已过期，不得遮蔽C；重试应安全重读并补记当前授权C，或继续如实提示未确认，不能默默清警示。确认成功后新启动恢复C，且不重存菜单/重置编辑。

**实际：** 控制器返回B的superseded，持久记忆仍A/revision=1。页面当前C但selectionWarning=''、error=''；新页面恢复默认A的8项。两层复现的菜单根及工作副本保持，证据包含全部合成菜单ID、pending/authority、重试结果及新页面结果。

**位置与建议：** src/selection.js:65在不可用读取时提前返回，旧pending B尚未在76行退休；126～134行retrySelection再取旧pending，133行目标不符直接superseded。src/menu-controller.js:145将其作为当前反馈，pages/index/index.js:50～54对该状态清warning，157～158行真实重试入口暴露此路径。建议developer在新关联即使读取暂不可用时也处理旧意图/授权退休，为当前目标保留可安全重试的上下文；过期旧请求不等于新目标已经记住。保留未知格式、结果不明冻结和菜单部分成功等既有保护；本成员未实施修复或改开发测试。

完整可复现说明、命令和建议在产品test/evidence/tester-t035-startup-restore/BUG-T035-001.md。新增独立文件含两个失败层，可在新固定修复候选上复跑，再做相邻失败/不可用/不明与授权回归、完整套件和隔离真实平台验证。

### 11.4 状态、提交与下一步

**T-035 blocked，未完成；BUG-T035-001未修复/复测关闭，独立建议暂不可合入。** 本成员按任务约定保存最小证据、更新本人报告/索引/任务、适当验证并本地单行提交后停止。由project-manager核对回执，串行安排developer修复并交新的精确版本，再通知tester复测；不由tester重开或改写T-034、代写他人结论或把T-036标完成。

产品只新增本人复现与证据；OPC只改T-035、A-014当前元信息/第11节及A-014索引行，原报告第1～10节与旧专业事实保留。不改STATUS/PROJECT/TEAM/WORKSPACE、需求/架构稿或作者交接，不联网、合入、推送、改tag、删除分支、创建GitHub Release或微信上传发布，不发消息或唤醒其他会话。产品仍本轮feature且干净，OPC保留原.tmp/；.local/tester-t035空目录保留，不碰其他中间资产。失败用例保留为失败，不虚报全套/平台通过。

## 12. v0.8 修复后独立复测与启动恢复验证（T-035，2026-11-03）

### 12.1 接续、版本和保护

project-manager按人类用户原功能/串行成员安排授权发送修复接续通知后停止，developer已交付并停止；tester实际刷新TEAM、members/tester.yaml、tester角色/scope和testing、wechat-miniprogram-testing全文。根公共规则与四份需求/技术正文经Git/内容指纹确认未变，复用已完整读正文；重读D-024、当前T-035复测安排、原BUG/本报告第11节、A-006第11节SR-01～13、A-013 v0.6第9节、修复README及实际接口。继续按A-001 v0.11/A-007 v0.5的13/13/40、A-006 v0.7/A-008 v0.5验证，不新增范围或使用旧候选判修复。

| 定位 | 本轮实际版本/证据 |
|---|---|
| OPC接手 | a7257eb7664fcbfee993a307ea495501ef663b02，feature/eating-everyday/t-032-startup-restore；目标project/eating-everyday仍2e241e6b489a65701ba2078640b2794cdabba429，仅原.tmp/未跟踪。 |
| 产品固定修复 | 本机C:/Users/YinZh/Documents/workspace/eating-everyday，origin匹配WORKSPACE；feature/t-032-startup-restore。代码/开发回归/说明5085636ba8955b08fcf39c24cf84b0329ef89c6d；接手完整资产ae61a36c59d95e24669010900b8d8b99b739cded，实际差异仅developer-fix-t035-001证据目录，工作树干净。 |
| 本人新独立资产 | 912d77154d1c3958680a6f01400d9218c688eafd，仅新增test/independent/startup-restore.risks.test.js及test/evidence/tester-t035-retest-001/，30份文件；前者31项独立测试，后者含旧版夹具、原始日志、SDK脚本/截图、清理与保护。 |
| 保护与基线 | 接手343份原受控文件逐字节保持，包含原四项独立断言、旧失败/历史证据、作者测试与实现。公共配置SHA-256仍27a614c100e150979107c9f648b67f757d81b586883c9eb89a46f2670c143bbf，私有仍df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d。产品main/v1.1.0和WORKSPACE仍3800e6dd8bc32b7d767bfc873a4b9ae57d10232b。 |
| 环境/导出 | Windows 11 10.0.26300、Node v24.22.1。固定修复git archive导出无.git/私有配置，仅加入本次新独立测试及其legacy-v1.json夹具后执行同全套命令。candidate-manifest.json记录archive、22份运行源码及两份独立增量SHA；临时隔离工程仅注册前注入唯一WxStorage前缀与临时projectname，已实际比对其余源码和配置保持。 |

产品本轮证据README、protected-before.json、run.json和manifest.json记录精确命令、环境、前后字节及原始文件哈希；提交后再次核对Git blob与manifest一致。原独立测试诊断里fixedCode=e3ce8cf是未改历史源码文字，实际修复被测版本由本节及manifest固定为5085636。

### 12.2 独立执行结果与SR追溯

| 实际调用 | 结果 |
|---|---|
| 原未改startup-restore.acceptance.test.js完整文件 | **4/4**、退出0，含原控制器/真实页面handler两层最小失败；original-four.tap。 |
| 新独立startup-restore.risks.test.js | **31/31**、退出0；independent-risks.tap。独立Port/夹具与预期，不复制开发断言。 |
| 工作树全套 | **609/609**、退出0；working-all.tap。命令node --test --test-reporter=tap test/*.test.js test/independent/*.test.js。 |
| 固定代码干净导出同全套 | **609/609**、退出0；clean-export-all.tap。 |

以上最终运行失败/取消/跳过/todo均0。完整609已包含原四项及新增31，不把不同调用重复相加。developer的19/19、双578/578和八组平台仅为输入，没有冒充独立结果。

| 契约风险 | 本人本轮实际覆盖 |
|---|---|
| SR-01/02 | 原独立四项验证新会话按稳定ID恢复最新保存内容/名称/基线及干净状态、恢复不改菜单原字节；新增后台启动及真实wx全新域另验证默认8项、唯一默认身份和持久选择一致。 |
| SR-03/04 | 从已发布3800e6d的真实旧MenuRepository生成四份v1数据，保留complete/初始化回执原结构：改名内置ID、无bootstrap用户同名默认、原列表首份、合法已使用空库；另测missing/null/已删ID、selection-only/未知额外key不播种、同名新身份不复活旧ID。 |
| SR-05/06 | 新建/另存/覆盖、当前与其他改名、历史create回放不倒退选择；dirty取消/放弃/保存切换、空/容量失败、A保存成功B加载失败的菜单/基线/记忆逐阶段保护。 |
| SR-07/08 | 取消/非目标/当前/最后删除、清选择失败、同名新ID及重启降级；删除保留本次副本、不复活；同页hide/show保留编辑与dirty，新页面只恢复保存快照。 |
| SR-09/10 | 菜单info/根/内容读错与坏校验不跳坏库/播种；选择坏值/未知格式/读错单独不可用，健康菜单可明确加载；记忆专用重试不改菜单和完整编辑状态。 |
| SR-11/12 | 同ID/不同ID×unavailable/failed/uncertain、显式旧intent、旧owner、新关联代次；写已应用但回调失败核验一次、未settled迟到写冻结冲突入口、not_committed补写原意图；disposed旧owner/迟到加载不写回、后台启动延后应用、持久guard冲突警示保留/解除后安全补记。 |
| SR-13及原回归 | 唯一selection白名单、其他未知key保护、旧v1、意图/guard/序列行为；609全套覆盖现行默认菜单DM、原保存/菜单/字符/随机和存储回归。未把有限抽样宣称概率证明。 |

### 12.3 独立真实微信平台及可见交互

实际DevTools Stable 2.02.2608080、基础库3.17.4、已安装SDK 0.13.0，模拟器390×753/DPR3。环境中的iPhone 12/13 (Pro)/iOS字段仅代表模拟器。隔离工程位于产品.local/tester-t035-retest/native-project；Page注册在任何onLoad前即注入唯一测试前缀，未先进入用户存储域再切换。native.json保存**20条已断言行为记录＋1条清理记录，运行异常0**：

1. 全新域A的固定8项及持久选择；真实页面handler加载B时在选择write前明确失败，持久仍A；加载C时仅选择read失败，C和新增未保存编辑保留。
2. 使用computer-use的实际鼠标点击可见“重试记住菜单”，onRetrySelection恰1次；C确认后警示才清除，每份菜单原字节及完整工作态均不变。reLaunch真正重建页面恢复C保存的1项、dirty=false；再次实际鼠标点击“开吃”，真实wx随机返回C保存菜。两次鼠标点与SDK tap、handler调用分开记录于computer-use.json。
3. 四份旧v1夹具分别置入本隔离域，经真实WxStorage和新页面验证改名内置ID优先、用户默认、首份降级与空库；每次恢复均不改菜单字节，再建页面仍一致。
4. 实际页面选择、另存对话确认、覆盖/改名、dirty取消/保存并切换、删除取消/确认；B跨页恢复、同页hide/show保留编辑、N另存/最新覆盖改名快照、C/D删除保留副本/清记忆且新页不复活、回到仍存在内置A。动作表/删除确认的**决策值由明确测试替身提供**，未声称物理点击这些弹层；菜单操作仍经真实页面handler及WxStorage。
5. 隔离选择坏值使启动进入明确手动模式，B仍可正常加载、坏值和健康菜单均保持；guard持续冲突时SDK点击实际按钮仍保留“尚未确认”提示，解除故障后SDK补记B成功。未将superseded当作记忆成功。
6. 通过官方CLI**实际关闭并重新打开本轮隔离工程窗口**，新运行时仍恢复记住的B，菜单/选择记录原字节保持；这与reLaunch及同页hide/show分别记录，不扩为实体手机进程留存。

before/after-mouse-retry、mouse-draw-result、guard-warning、project-reopened-b五份截图均已查看；guard警示保留及C补记后无警示、未保存标记/保存快照区别可见。原生源脚本native.cjs和准备/回归脚本prepare-and-regress.py可追溯，后续复现应使用新的空临时目录/前缀；CLI日志已脱敏AppID。

### 12.4 BUG关闭、失败历史及限制

**BUG-T035-001（P2）在5085636上独立复测closed。** 原控制器和真实页面两层失败现在均通过，真实WxStorage的A→B写失败→C读失败→鼠标补记已确认C且新页面恢复C。相邻未知格式/不明写/授权与编辑保护通过，本轮未发现新产品缺陷。原e3ce8cf失败日志、open记录及第11节作为历史完整保留，新关闭说明在产品BUG-T035-001-retest.md。

保留的测试/工具失败没有改写为通过：新风险首轮30通过/1失败是夹具等待同ID覆盖会省略的selection.write，故未触发目标load故障；仅修正新测试为直接注入load.read并断言实际触发，之后31/31和双609通过。native-connection-sandbox.json记录初次本机端口沙箱连接失败；native-initial-page-handle.json及初版脚本记录reLaunch后误用旧SDK page句柄，重新取当前页后完成剩余核对，并另以D重做有完整记录的取消/确认删除；native-reopen-connection.json记录自动化服务刚重开尚未就绪，有限连接轮询后新运行时留存验证通过。上述为工具/夹具问题，不是产品缺陷，不降低业务预期或改作者断言。

故障按Port/WxStorage具体阶段注入；实体手机/新预览码、实体离线、物理满配额/强杀/断电、多设备、旧写入者降级及多进程CAS本轮未新增验证。不明写单次Port效果/共享串行前提仍成立，桌面/模拟器不替代实体机或物理平台保证。旧手机反馈及旧专业结论保留原适用版本，成果draft不等于整份批准或发布验收。

### 12.5 清理、状态和交接

本轮重置夹具及最终收尾累计仅删除本唯一前缀**50 key**，最后一次19、最终剩余**0**、非测试域删除**0**。最终先onUnload再清前缀，随后关闭隔离工程，computer-use列表核对标题已不存在；没有读取或清除用户菜单值。保留产品.local临时导出及OPC原.tmp/，不清理其他资产。

**T-035 completed；本轮声明范围内建议合入固定修复5085636及独立资产912d771。** 测试接收/实现/原失败与新证据的精确链已保存，343原文件、两配置及主线基线保持。本成员仅更新T-035、本报告当前元信息/第12节和A-014索引行，结构/链接/空白及范围核对通过后本地单行提交并停止；原第1～11节及他人任务/成果不改。不发消息或唤醒成员、不改STATUS/PROJECT/TEAM/WORKSPACE、不合入/推送/移tag/删开发分支/创建Release或微信上传发布。由project-manager核对独立回执及最终版本后处理原T-036，tester不代写整合完成。
