# 原生小程序工程与平台复核交接

- 项目：eating-everyday（今天吃什么）
- 成果 ID：A-013
- 负责人：developer（研发工程师）
- 关联任务：[T-012](../TASKS.md#t-012补齐原生小程序工程与开发工具预览)，供[T-013](../TASKS.md#t-013验证小程序导入编译与预览闭环)串行接收。
- 版本：0.2
- 更新日期：2026-10-21（Asia/Shanghai）
- 状态：draft
- 适用范围：原生工程及BUG-T013-001开发修复；真实双项鼠标抽取/指针/结果、关键保存加载和新预览已取得。T-012完成本地开发候选交接，001仍待tester独立复测，手机与整体验收未完成。
- 来源及输入：本次用户授权及项目经理T-012安排；[A-001 v0.9](product-requirements.md)、[A-007 v0.3](product-decision-options.md)、[A-006 v0.4](system-architecture.md)、[A-008 v0.2](accounts-access-checklist.md)、[A-010 v0.1](core-test-plan.md)、[A-011 v0.3](developer-core-handoff.md)、[A-012 v0.3第7节](core-test-report.md)，现行D-005/D-011/D-014/D-016/D-017；v0.2另输入[A-014 v0.1 BUG-T013-001](miniprogram-test-report.md)，用户已明确恢复开发/测试自动操作并处理审批。
- 存储位置：本治理正文在OPC artifacts；产品源码、测试、中间证据全部在本机产品仓库git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday。
- 固定工程候选：**8e4472006f1d3cdff5b774052e01ce8db5a96453**，产品feature/t-002-readiness-review，父提交为tester资产f678942064fb9dc8fdf37ddf9a2257a8b459c1c0。继承f7ceaf791dd06e8749fa1754b6a0143d1193fb63工程及已独立通过的核心54b720fa920dfcda9a3db5fa0e42378be01f5535；原历史/24份原tester字节保留，当前44份tester Git内容一致，无测试断言改写。
- 主线交付：未合入、未推送；产品main/已有origin/main/WORKSPACE.revision仍为8fa87fe4cb8f7f33aacf2842798a486150a6105e。OPC沿用feature/eating-everyday/t-002-readiness-review，目标project/eating-everyday及已有origin目标仍a735031c37c09e75fb9478681aaf42010a82f378。
- 批准依据：无整份成果批准；本次授权允许开发和本地候选交接，不代写tester平台结论，不将开发自测当作独立测试。
- 版本记录：0.1首次原生候选及Esc停止历史；0.2在用户恢复授权、tester实际发现001后修复宿主随机值兼容，完成真实双项/菜单冒烟与新版预览。

以下第1～4节保留v0.1当时的停止和未验证事实，**当前以第5节v0.2为准**；旧失败记录不改记为通过。

## 1. 工程与操作入口

导入根目录为本机 `C:\Users\YinZh\Documents\workspace\eating-everyday`，不是OPC仓库或src子目录。候选包含app.js/app.json/app.wxss、sitemap、project.config、pages/index的JS/WXML/WXSS/JSON、ui及wx适配。无需npm构建或第三方产品运行依赖。真实安装工具2.02.2608080；公共配置锁定实际曾运行的基础库3.17.4，原生WebView渲染。

产品README给出手动导入、编译、预览和CLI步骤；docs/miniprogram.md说明端口、状态、失败保护、停止点及后续验证脚本。个人测试号AppID只在本机被忽略的project.private.config.json，公共appid为touristappid；不取得AppSecret。本机私有配置已存在且曾被CLI正确读取。桌面Node适配、测试/证据、vendor、docs、examples、.local及私有配置均排除小程序打包。

用户Esc中止后，后续工具操作方式尚未确定；本轮不再连接GUI、CLI或automator，也不以别的方法重试同一电脑操作。用户若自行操作，先点击「编译」核对首页、中央按钮和两项抽取；确认后重新生成预览码。若由代理继续，须先有用户明确恢复答复。本交接不替用户或项目经理作出该答复。

## 2. 实现及规则保持

- 第一次候选为空，保存菜单不自动载入；名称首尾去空白、同名拒绝、最多24项。完整名称保留在列表/结果；菜单名按固定Unicode18.0.0/UAX29 rev49扩展字素簇计1～20，不把候选截成20字。
- 多份命名菜单、明确保存/覆盖/另存、原样加载、重命名、确认删除、dirty保存/放弃/取消切换；删除当前只去掉记录并保留未保存副本。操作ID/原版本/原候选快照保持核心回执契约。
- wx.getStorageInfo/getStorage/setStorage/removeStorage使用独立命名空间；只有成功枚举能证明不存在，读取失败、损坏或未知版本不清空。空间与回调异常由既有复制写入/提交核验/显式恢复处理，不自动退出保存。
- wx.getRandomValues只接受对应长度ArrayBuffer；仍用大端32位拒绝采样和32次预算，0/1边界、独立等概率及连续重复结果不变。Math.random只用于ID，不参与抽取。
- 真实模块漏载修正为顶层静态require及显式.js路径，并从等值JS表加载Unicode数据；Object.hasOwn改为宿主可用的原型调用。核心业务没有另改，全部既有回归通过。
- 最终转盘使用屏幕外Canvas生成本机PNG，由普通image旋转；中心原生button、固定指针与普通弹层避开可视Canvas层级。串行导出只接收最新快照，生成新图之前清除旧图并禁用抽取；出错可重绘。大于12项或名称较长时扇区按编号对应完整列表。
- 抽取只发生一次，动画把同一索引对齐指针；隐藏/卸载取消计时器并使代际失效，旧熵、动画或图片回调不覆盖新状态。存储不确定时阻止编辑/保存/切换直至核验。

## 3. 实际验证与证据

产品 `test/evidence/developer-t012/` 保存原始TAP、环境和源指纹、24份原tester对照、两份失败JSON及历史截图；源指纹在固定产品提交后复核一致。以下执行者均为developer：

| 范围 | 实际结果 | 证据 |
|---|---|---|
| 新增适配/页面/位图/工程检查 | 28/28通过 | new-adapter-page.tap |
| 全部开发桌面检查 | 151/151通过 | developer-all.tap，含853条官方分段语料 |
| 合并复跑原独立134项 | 285/285通过，无取消/跳过 | combined.tap；不是tester本轮独立验收 |
| 原tester资产 | 24份逐字节不变 | run.json，相对父提交f2646b52 |
| 第一次真实CLI导入/预览 | open/preview退出0，83913字节并生成旧码 | historical-preview-info.json及platform-transcript.md；随后发现模块漏载，旧码不可用 |
| 模块修复后真实runtime | 首页ready=true、0项、空菜单，无同一模块错 | 会话摘录；不是最终PNG方案的运行证据 |
| 真实随机接口单独检查 | 成功回调、4字节ArrayBuffer | platform-transcript.md；没有mock熵 |
| 原生烟雾尝试1、2 | 均失败，仅到单项；超时/节点null | native-attempt-1.json、native-attempt-2.json，保留原失败状态 |
| 中间画布UI | 中央按钮不可见 | historical-direct-canvas-empty.png；真实窗口也观察到相同问题 |
| 最终PNG方案/新预览/手机 | 未执行 | 用户Esc后未重新操作开发工具 |

真实模拟器曾配置iPhone12/13(Pro)、390×844、视口390×753、DPR3、基础库3.17.4；不是实体手机系统信息。SDK节点82×82及程序触发单项结果不能证明按钮可见可触达；没有将其升格为完整UI通过。最后一次auto返回成功也没有完整运行结论。

桌面复跑命令（在产品根目录，不操作开发工具）：

```powershell
node --test test/*.test.js
node --test test/*.test.js test/independent/*.test.js
```

可选test/native-smoke.js默认退出且不被桌面glob运行。修改后的脚本会等待图片/节点、使用新建隔离存储域并只清理自身key；只有用户恢复工具操作后才可执行，最终脚本尚无成功平台记录。SDK固定0.13.0安装在被忽略的.local/automation，不增加产品运行依赖。

## 4. 剩余项与停止边界

T-012因最终平台复核未完成标blocked，不能以285项桌面通过替代完成条件。当前准确剩余项：最终源码重新编译、PNG导出/显示、中央按钮可见及触达、指针/持续旋转/结果卡与命名菜单弹层、真实wx保存加载及失败恢复闭环、重新生成可用预览码；手机扫码、关闭重开留存、键盘/小屏/大字/安全区、平台已能打开后的断网及实体生命周期/容量边界另行实测。

本轮已完成可审查的本地工程提交和本人治理交接，两仓库保留当前feature；未改需求/架构/决定、tester的T-011/T-013/A-012、项目STATUS、WORKSPACE基线或共享角色技能。未合并、推送、发布、删除分支、创建或联系其他会话。保存治理后停止，由项目经理按用户后续答复和既定串行安排接续；不得把测试接收当作用户恢复工具操作。

## 5. v0.2：BUG-T013-001修复与首个可用预览候选

### 接收、授权与固定版本

项目经理按用户既定授权串行通知，tester实际停止后接收。OPC接手b44d985033c1f174decaad3b37edbcc49e16835b，产品接手f678942064fb9dc8fdf37ddf9a2257a8b459c1c0，同后缀feature且干净；不回退新20份tester资产。身份/scope、研发角色及三项技能已重读；规则/需求/架构/决定经Git比较未变，完整读取A-014及T-012/T-013。

用户已明确“允许开发和测试恢复自动操作，完成编译与预览验证”，又确认已处理审批。原Esc及审批超时已解除，本次沿用既有CLI/SDK/computer-use及私有AppID，不更改安全/隐私设置，不读写AppSecret。不是仅以测试接收推定恢复授权。

### 修复内容与开发回归

原独立跨执行域回归先实际复现退出1，失败另存。WxRandom用ArrayBuffer内建byteLength getter检查实际类型/长度，既接受合法跨执行域宿主值，也拒绝toStringTag/length伪造；再复制为当前域自有Uint8Array，满足下游核心端口。数组/typed view/共享/分离buffer、错误长度、非法请求和API失败仍拒绝。src/draw.js与输入逐字节相同，不改等概率拒绝采样、32次预算或引入随机回退。

新增6项开发回归；原缺陷1项＋新6项7/7，全部开发157/157，开发复跑合并292/292（原独立135），无失败/取消/跳过。官方Unicode853条仍匹配。产品test/evidence/developer-fix-t013-001/保存before-fix/cross-realm-fix/developer-all/combined TAP、真实状态与图片、源码指纹和44份tester资产对照。44份Git blob内容一致；只有新缺陷JS工作树原本CRLF而Git为LF，记录原始SHA并按Git规范化校验，没有改写该文件；原24份另按历史原始字节复核一致。

### 真实工具与用户入口

| 开发检查 | 本轮实际结果 |
|---|---|
| 环境/编译 | Windows11、Node24.19.0；工具Stable2.02.2608080、基础库3.17.4，原生WebView；CLI auto退出0。 |
| 真实跨域随机链路 | raw ArrayBuffer：4字节、instanceof=false；实际WxRandom输出：Uint8Array、length4、当前域instanceof=true。核心实际双项抽取成功。 |
| 鼠标转盘/结果 | 实际输入面馆/饺子、点击可见中央开吃，观察旋转与完整面馆结果卡。捕获结果index0/rotation1800，指针与结果对应；见mouse-two-result.json/png。不是只判断节点存在或SDK tap。 |
| 保存/重新进入/加载 | 新建隔离域，实际鼠标保存命名“开发隔离午餐”；SDK reLaunch回空副本后连接同一域，再实际点菜单列表及目标。原menuId/面馆、饺子顺序恢复，dirty=false；见menu-saved及menu-loaded-cleanup JSON/PNG。 |
| 用户数据 | 只清理隔离域3个key；用户v1域前后0key且SHA256一致，没有覆盖用户菜单。reLaunch不是手机结束/重启微信。 |
| 新版预览 | CLI preview退出0，包**121174字节（118.3KB）**；原始preview-info.json已保存。本机**.local/t013-001/preview.jpg**为新二维码，被Git忽略。旧83913字节码继续无效。 |

实际导入根目录仍为本机 `C:\Users\YinZh\Documents\workspace\eating-everyday`，README给出完整手动步骤。用户可以用有权限的微信扫一扫**上述新码**；手机是否成功仍以实际扫码结果记录，不预先宣称通过。个人AppID仍只在被忽略的project.private.config.json，没有AppSecret、业务登录或云端菜单改动。

证据保存有过一次时序偏差：写入项目内证据后页面已重新编译，首次SDK图为空页，另存sdk-after-probe-empty.png，不将其当结果图；后续先读取/捕获再落盘，正确结果和保存/加载图均已查看。持续SDK连接影响鼠标操作的试验被中止，没有整段通过记录；后续鼠标动作前断开SDK。旧SDK失败及A-014原失败不改为成功。

### 交接与停止

固定产品修复提交8e4472006f1d3cdff5b774052e01ce8db5a96453，父为f678942064fb9dc8fdf37ddf9a2257a8b459c1c0；运行源码指纹、截图提交字节及24份原tester原始字节均复核。产品及OPC本地feature保持，主线/WORKSPACE基线不变，不合并/推送/删分支或发布。

T-012按“完整工程＋开发自测＋实际可用首个预览候选”完成本次开发交接，不代表BUG-T013-001由tester关闭或T-013完成。A-014/T-013原open/blocked由tester维护；本会话停止，由项目经理串行通知tester先独立复测001与真实双项，再继续菜单平台分支及新预览。本人不联系/唤醒其他会话、不改STATUS/WORKSPACE或他人报告。

手机扫码、正常关闭重开留存、完整原生覆盖/另存/重命名/删除/dirty选择分支、键盘/小屏/大字/安全区、实体生命周期与真实容量边界、已打开后的断网仍待T-013分层执行；不扩展首版需求，不等待全部机型才交当前预览。
