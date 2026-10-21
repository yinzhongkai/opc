# 小程序独立平台测试与阻塞缺陷报告

- 项目：eating-everyday（今天吃什么）
- 成果 ID：A-014
- 负责人：tester（测试工程师，兼职）
- 关联任务：[T-013](../TASKS.md#t-013验证小程序导入编译与预览闭环)；输入为 T-012 固定交接，不代写 developer 修复或任务状态。
- 版本：0.1
- 更新日期：2026-10-21（Asia/Shanghai）
- 状态：draft
- 适用范围：当前原生候选的独立源码/桌面复跑、真实开发者工具编译、PNG/按钮及抽取平台复现；真实菜单闭环和手机验收尚未完成。
- 授权及输入：用户既定串行授权、项目经理本轮通知；[A-001 v0.9](product-requirements.md)、[A-006 v0.4](system-architecture.md)、[A-010 v0.1](core-test-plan.md)、[A-012 v0.3](core-test-report.md)、[A-013 v0.1](miniprogram-preview-handoff.md)及现行决定。接收时刷新 tester 身份、scope、角色、testing/微信小程序测试正文；未变规则及历史按 Git 范围核对复用。
- OPC 接手版本：7d497ef99bd9a26566982ebb86c8af13ce6f84f7，feature/eating-everyday/t-002-readiness-review；目标 project/eating-everyday 的本轮合入前基线 a735031c37c09e75fb9478681aaf42010a82f378。
- 被测实现：本机产品 git@github.com:yinzhongkai/eating-everyday.git，checkout=`C:/Users/YinZh/Documents/workspace/eating-everyday`，feature/t-002-readiness-review，**f7ceaf791dd06e8749fa1754b6a0143d1193fb63**；测试期间未改生产。
- 测试资产：同一产品仓库 **f678942064fb9dc8fdf37ddf9a2257a8b459c1c0**。自上述实现起仅新增20份 tester 测试/证据；首次资产提交 c0dcb576647535aa8b82200ad80b4c03787af6c4，后续 f7581070b90b0d1d3d50615e364107a0f0b26da9 / f678942 只修正新 JPEG 的二进制保存及证据字节哈希，未重写历史或改变失败断言。
- 存储位置：治理报告在 OPC artifacts；执行代码、截图、日志在产品 test/independent/defect-t013-001.test.js、test/evidence/tester-t013-preflight/、test/evidence/tester-t013-platform/，以下均为产品仓库内路径。
- 主线交付：未合入或推送。产品 main/已有 origin/main/WORKSPACE.revision 仍为 8fa87fe4cb8f7f33aacf2842798a486150a6105e。
- 批准依据：无整份成果批准；测试成功子项不代表完整预览闭环、真机验收或正式发布。
- 版本记录：0.1 首轮独立平台执行，发现 BUG-T013-001 并按约定停止；原 T-011 completed 和001/002 closed历史不变。

**当前结论：BUG-T013-001 open，阻塞双项抽取，T-013 blocked。** 桌面原套件285/285通过；真实编译及最终PNG/中央按钮显示、单项实际点击通过；加入两项后实际点击中央按钮立即报抽取不可用，无旋转、无结果。未生成新预览码，旧码继续无效，不能交付为已验证可用预览。

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
