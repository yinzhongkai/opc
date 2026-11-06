# 原生小程序工程与平台复核交接

- 项目：eating-everyday（今天吃什么）
- 成果 ID：A-013
- 负责人：developer（研发工程师）
- 关联任务：当前[T-041](../TASKS.md#t-041实现首页好友转发与朋友圈分享)，供[T-042](../TASKS.md#t-042独立验证首页分享及既有菜单行为)串行接收；原任务事实保留各历史节。
- 版本：0.8
- 更新日期：2026-11-06（Asia/Shanghai；本轮2026-11-05开始）
- 状态：draft
- 适用范围：T-041重开后的分享静态图解析兼容修复与开发交接；第1～10节原事实保持，第11节为当前固定候选。已完成本机真实预览/取消及关闭重开开发验证，不包含T-042独立结论、实体手机收卡或实际外发验收。
- 来源及输入：project-manager依用户“继续找对应的成员完成测试任务吧”和[D-027](../DECISIONS.md#d-027先建feature分支再实现首页分享)明确重开T-041，串行通知本成员。OPC接手22c61fe62e1c899f994fd2bf7d3f39340ab75c62，产品939d55f07322afebe889943dbae9445dd9c1439d；输入A-014 v0.10第14节及tester最小复现，原v0.7交接保留第10节。本人配置/角色/scope及三技能已完整刷新，根规则/原需求架构比较未变；[A-001 v0.11](product-requirements.md)、[A-006 v0.7](system-architecture.md)的数据保护与恢复契约不改。
- 存储位置：本治理正文在OPC artifacts；产品源码、测试、中间证据全部在本机产品仓库git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday。
- 固定工程候选：代码/回归/说明 **a647d1f04acc7d8022699baa0a5c54af1856c4ee**，父为接手资产939d55f；完整研发证据 **e280b9559d119c6e66cfdc01b42e4d9d4929c522**，仅追加test/evidence/developer-t041-image-20261105/88文件。旧fb27d41/cc9c191按第10节保留；新代码不改其历史结论。
- 主线交付：仍在原配对feature，仅本地提交。WORKSPACE锁定38b1e69350ad2b4cf43c3124a3635f4a0b9c47a0、OPC任务基线1970f68a均为当前feature祖先；本克隆本机main仍6c9ef2b3135a0013fb0b06ebbe761b298c4b7260、project/eating-everyday仍d4e65980fb30d4fe9a65248c277861823402eb9a，按T-040协调保持。本轮只查本机引用，未联网核远端、切换/拉取/改写或合入主线；候选不是已合入基线。
- 批准依据：D-027/T-041授权本成员实现、自测和有界本地交接；无整份成果批准，不代写T-042独立结论或执行合入、推送、发布、WORKSPACE推进及实际外发。
- 版本记录：0.1首次原生候选及Esc停止历史；0.2随机跨域修复与开发预览；0.3配置测试适配及双环境结果；0.4默认菜单一次性初始化/兼容/恢复；0.5启动恢复首轮开发验证；0.6旧意图/补记修复历史；0.7首页分享实现、13项新回归及工具启动失败限制；0.8包内图解析有界对照、本地静态文件兼容、双652与真实固定候选预览/取消/重开及精确清理，待tester独立复测。

以下第1～10节保留v0.1～v0.7历史事实；**当前T-041以第11节v0.8为准**。旧输入SHA、旧启动方式、Esc及缺陷待复测状态均限于原执行时点；后续独立事实以其原任务/A-014对应版本为准，不倒写旧失败或把旧通过扩为新候选已独立验收。

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

## 6. v0.3：T-014仓库默认与本机配置测试适配

### 接收与最小变更

项目经理已完成安排且其他成员停止，本成员独占执行。重新读取本人配置/scope、角色与三项有效技能、WORKSPACE、T-014及相关输入；未变规则经Git范围核对复用。OPC接手fafe2d8e71d7e5442715c35e25cbde0d36957fdb干净，产品接手10f52d40bb8a62b04a8f84793e44117fe3c28e46，唯一已协调差异为用户授权保留的project.config.json。当前分支仍为本轮同后缀feature，未创建新轮次。

只改test/miniprogram-package.test.js，并新增test/support/project-config.js、test/project-config.test.js。Git检出从只读git show HEAD:project.config.json检查受控默认占位AppID；无.git的干净导出检查导出文件，不向上寻找父Git。工作树运行配置接受touristappid或wx＋16位小写十六进制格式，拒绝空白/填充、错误前缀/长度/字符、非字符串；仅检查格式，不替代账号授权/发布资格。错误消息不回显账户输入。

两来源都执行根目录、compileType、固定基础库和带folder/file类型的打包排除保护，包含本机私有配置排除；不因本机AppID不同跳过整个复合用例。原app/page/事件/JS可达依赖/Unicode等值/可视转盘检查继续运行。新增一项默认来源实查及六项合法/非法/来源/打包/元数据测试，共7项；没有任意AppID放行、测试skip或业务代码变更。Git检出使用现有Git进行只读来源审计，导出套件只需Node，无npm依赖增加。

### 双配置开发验证

固定被测测试修正提交为ac9ae93596633919e527934b5c5f64c40112b49a。以下均为developer执行；独立合入结论交T-015：

| 配置来源/阶段 | 实际结果 | 产品证据 |
|---|---|---|
| 修正前本机文件 | 输入10f52d4完整套件退出1，291/292，唯一AppID固定断言失败 | before-working-redacted.tap、before.json；原始日志仅在忽略的.local/t014/ |
| 修正后保留本机配置工作树 | HEAD=ac9ae935；默认来源为git HEAD，实际文件仍为原授权配置；完整套件299/299、退出0 | working.tap、run.json |
| 修正后干净Git导出 | git archive ac9ae935；无.git/私有配置，默认文件仍touristappid；完整套件299/299、退出0 | clean-export.tap、run.json中archive SHA/来源；新导出目录已安全清理 |

两类完整命令相同：`node --test --test-reporter=tap test/*.test.js test/independent/*.test.js`，Node v24.19.0；失败/取消/跳过/todo均0。原A-014的291/292与提交292/292仍是其当时结果，没有倒写为新通过。产品证据位置为test/evidence/developer-t014-appid/，含脱敏TAP、配置/私有文件前后SHA、源码绑定和T-015最少步骤。

用户project.config.json原始SHA-256前后均**97f3a1355a54a3c8875f6da96580357c1fbc3a639079e009acf0aa07c6bae2db**，从未改写、暂存或提交；私有配置指纹前后相同。业务src/adapters/pages/ui原始指纹不变；受控公共配置与输入Git版本一致。公开日志脱敏，个人AppID不入提交。失败TAP保留报告缩进，证据首次空白检查提示两处blank-at-eol后，仅在本证据目录加*.tap格式属性并追加提交，保留日志SHA与旧失败，不屏蔽测试失败或放宽源码检查。

### 固定交接、最少复测与停止

修正提交ac9ae93596633919e527934b5c5f64c40112b49a；原始证据ebc4e7d70ee2450dd1bbd98eda9ef55dc8feb129，当前完整资产HEAD aa8c6d83946ca7ec0ebe420c408a9511799c77ea。追加提交仅测试证据/格式说明，不改被测三份测试源码、业务或公共配置。

下一位tester先核对当前资产HEAD及本机配置SHA，然后在保留该配置的工作树执行上述完整命令，再将固定候选git archive到产品.local/内新的空目录，在导出目录执行相同命令；两处应299/299。读取test/project-config.test.js的合法/非法与打包用例，不复制本机配置覆盖导出默认；导出/复测后再核对本机原SHA。具体PowerShell步骤见产品证据README，无需GUI、正式账号或手机重复测试。T-015接收时另记当前资产HEAD，源码SHA与日志被测ac9ae935分列。

T-014按本次开发完成条件completed，T-015仍由tester独立执行、T-016仅项目经理在通过后推进。本成员只改本人T-014/A-013/索引，不改STATUS/PROJECT/WORKSPACE、其他任务或他人专业报告；不合并、推送、重写历史、删分支、微信上传或发布，不联系/唤醒其他会话。治理提交后停止，产品工作树仅保留已授权的project.config.json差异，不能称其完全干净；OPC保存后干净。

## 7. v0.4：T-022默认菜单实现与开发交接（2026-11-02）

### 固定版本与实际范围

代码候选为产品**dcb22d3bdcec196e37b544805f79196de126f676**，完整证据资产HEAD为**33e3e64cd3e59d48adbd13d69622a82087468016**；后者只增加本人成果目录。基线main/WORKSPACE保持6c9ef2b3135a0013fb0b06ebbe761b298c4b7260，当前feature/default-menu尚未合入；产品v0.1.0不包含本功能。OPC在同后缀feature仅改本人T-022、本成果及A-013索引，提交通过接手19005a6b、任务ID和Git历史定位。

已按A-001 v0.10/A-007 v0.4、A-006 v0.6第10节、A-008 v0.4第0节及D-019落实12/12/32和DM-01～DM-12，未降低数据保护条件。默认名“默认菜单”，8项顺序固定为：肯德基、麦当劳、西域美食、醉面、吉野家、阿香米线、喜家德、麻辣烫。无新账号、定位、联网取数、费用或用户业务选择。

| 契约 | 实现与可观察入口 |
|---|---|
| 首次及旧数据分类 | repository.ensureDefaultMenu在原串行队列内全量验证根/索引/全部菜单，成功枚举本域key且根及本域全空才为FRESH。旧无标记genesis（含孤块）、有效空索引及任意旧菜单只读兼容，不写标记、不追加、不覆盖同名菜单。无根残块/未知key、读失败、坏校验及未知格式拒绝。 |
| 一次性与原意图 | src/default-menu.js固定d019-v1/8项与保留operationId。pending在revision=0/index=null保存原menuId；完整菜单、回执及complete在revision=1一次根切换提交。标记参与原checksum，原菜单/索引schemaVersion仍1。pending期间CRUD拒绝BUSY；普通命令不得使用初始化保留ID。 |
| 提交/恢复 | 内容和索引逐块回读，容量及根guard后切换并全量确认；反馈报错但核验完整则按实际成功。结果不明冻结写入/切换；recoverDefaultMenu只读全量核验后才允许原menuId续接，不把历史回执当当前菜单加载。所有CRUD保留终态及初始化回执，编辑/重命名/删除和空库均不复活种子。 |
| 原生启动/重试 | StartupCoordinator锁住空且无关联/dirty的Session，重复调用与重复onLoad coalesce。仅本次确认created或同一未失效启动链的安全恢复填充保存基线，dirty=false；已有数据和之后重进仍手动加载。失活/代次变化不应用迟到结果。onRetry在ready=false时先恢复/分类/续接，不能只刷新列表后绕过pending。 |
| 既有流程 | WorkingSession构造仍为空，原菜单CRUD/dirty切换/删除留副本、候选24项、字素校验及随机拒绝采样继续执行。旧手动页面及位图0/1场景改用合法旧空根；真正首次单独覆盖8项，不跳过旧保护或改tester断言。最小使用说明在产品README及docs/core-api.md的T-022节。 |

### 当前实际开发验证

以下全部由developer执行，不代替T-023独立验证：

| 环境/范围 | 结果及证据（产品仓库内） |
|---|---|
| Windows、Node v24.22.1；保留本机配置工作树 | 完整命令node --test --test-reporter=tap test/*.test.js test/independent/*.test.js，**372/372**，失败/取消/跳过/todo均0；test/evidence/developer-t022-default-menu/working.tap。不是沿用历史299/299。 |
| 同固定候选git archive干净导出，无.git/私有配置 | 同一完整命令**372/372**，四类非通过计数均0；clean-export.tap、candidate-manifest.json。代码候选与证据HEAD的运行源码/测试一致。 |
| 新默认菜单测试 | **73项顶层检查**，另在用例内逐内容/索引块循环写入及回读前后失败/中断。DM-01～06覆盖唯一8项/顺序/1⁄8、旧/同名/两类空库、重复进入、CRUD/删后不复活及0/1/24/25；DM-07～12覆盖读取/格式/残块、容量/各提交阶段、结果不明冻结及核验、迟到结果、全清与删根区别、队列/guard/保留ID和元数据保留。新源为test/default-menu.test.js、test/default-menu-page.test.js，后者为回调级wx替身。 |
| 真实开发工具：基础库3.17.4、自动化SDK0.13.0 | 固定候选临时工程在页面注册前注入独立WxStorage前缀，业务模块及页面逻辑不改。六组开发冒烟通过、0运行异常：首次自动8项/菜单名/dirty=false且PNG就绪，真实wx随机，reLaunch不强载，覆盖/改名/删除及旧意图不复活，删最后一份后重进空库，真实WxStorage上注入内容失败后原menuId续接。native.json、native-default-menu.png（已实际查看）、native-cli-redacted.txt。模拟器390×753、DPR3及其iOS字段不作为实体手机信息。 |
| 隔离/配置/旧证据 | 未清空用户菜单；原生工程只清理本次前缀10个key，验证后关闭临时工程。173份原独立测试及历史证据逐原始字节核对不变；公共配置前后eecc59987ea459258c2bf0329b69d98e73e790dfa6110d60f28ed3645456e645，私有配置前后df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d，两者均未暂存/提交。run.json记录环境、命令、数量和保护结果。 |

开发期间的三组失败保留为historical-*.tap并在run.json解释：五处异步拒绝误用同步断言后改为await rejects；三项旧位图手动场景改用合法旧空根；一次过严的诊断inspect与旧外域key清理契约冲突后恢复兼容，严格本域检查仍在初始化/恢复入口。没有修改tester资产或旧失败记录取绿。原生脚本启动阶段曾修正本机SDK路径并在沙箱连接被拒后依工具流程取得本机连接权限，成功运行证据以native.json为准，不把连接失败当产品功能结论。

### T-023最少输入、限制与停止点

tester先核对产品feature/default-menu的完整资产HEAD 33e3e64cd3e59d48adbd13d69622a82087468016、固定代码dcb22d3bdcec196e37b544805f79196de126f676与本节配置指纹；读取现行四份需求/技术输入、DM-01～12、产品docs/core-api.md及本轮证据README，再在独立隔离数据上执行新场景和受影响回归。真实首次从构造WxStorage时就使用独立前缀，不先运行用户域再切前缀，也不清用户真实菜单。可先运行两份新测试，再运行上述完整命令；独立结论、问题及复测由tester自行提供。

手机扫码/本次预览码、实体离线首次、物理满配额/强杀/断电、多设备、旧程序降级写入兼容与多进程CAS未新增验证。真实工具的动作通过SDK/控制器调用，未作为物理鼠标/手机触控验收；内容故障为注入，不是物理磁盘满。原生重试入口已有回调级页面测试，真实工具本轮故障使用启动协调器直接调用，T-023按风险补充按钮路径及实际平台差异。没有把这些未覆盖项写成通过，也没有索取新的账号或重复业务确认。

T-022已满足实现、自测、固定候选及开发交接条件，当前无剩余研发阻塞，成果仍draft，T-023尚待独立执行。两仓库本地提交本人内容并核对后停止，由project-manager确认停止后通知tester；本成员不发消息、不启动他人，不合入主线/推送/打移标签/创建GitHub Release/微信发布，不推进WORKSPACE、不清理原.tmp/、私有配置或开发分支。PROJECT/STATUS及索引顶部旧摘要由原维护人后续同步。

## 8. v0.5：T-034启动恢复实现与开发交接（2026-11-03）

### 固定输入、代码与交付边界

本轮接手OPC 42aeb7235a1c6070afaba695f334b0b2cd73d9a9，产品基线3800e6dd8bc32b7d767bfc873a4b9ae57d10232b。T-032提交2cf3714163a84e5b0fa68c93cc62a16bc7f87f46、T-033提交b47f6081af7392d4c4fc4d52624ca73678781dbb，均经项目经理核对完成、停止后才实际接收。本成员按SESSION_PROTOCOL刷新身份/角色/scope与software-engineering、frontend-development、wechat-miniprogram-development三项有效技能，读取WORKSPACE和相关需求/技术正文；不启动或联系其他会话。

输入UTF-8/LF归一化正文SHA-256为：A-001 v0.11 `989002414d6817db1b5539a4e27757972dac0408a0ee054741a0d18d1babb454`，A-007 v0.5 `4864d15138e96d90d83ae5f17c728af6e13f5953ebcab8c30e51b034babffcdd`，A-006 v0.7 `f31ca08f9af88c7ee989065ebb785d3cac2e77cec6f3339b50ad119cac21f511`，A-008 v0.5 `0e6b9b5b0a350f257574b4a971850dbe486a2abd8d4e45b06a3e60ff984fd6ef`。落实D-024、13项需求/13条规则/40项验收及SR-01～13，不追加用户业务确认。

固定代码/测试/最小说明为产品 **e3ce8cfecde344d3bd7c4c003ee427696fd9a877**；完整资产HEAD **9ab18b3e283b72f576055547453fbcfa5b192bd3**，后者仅增加 `test/evidence/developer-t034-startup-restore/`。本机仓库 `git@github.com:yinzhongkai/eating-everyday.git`、checkout `../eating-everyday`、分支 `feature/t-032-startup-restore`；OPC配对分支 `feature/eating-everyday/t-032-startup-restore`。产品main、v1.1.0和WORKSPACE仍3800e6d，OPC目标project/eating-everyday仍2e241e6b489a65701ba2078640b2794cdabba429；未合入、推送、发布、移动标签或删除分支，未联网刷新远端。

### 实现与风险处理

| 契约 | 本轮实际实现 |
|---|---|
| SR-01～04 启动/升级 | 新增MenuController作为原生页面实际入口，先复用既有pending/complete初始化，再核验当前菜单库及选择，加载最新保存快照。优先有效记忆ID；无/空/悬空选择按内置bootstrap ID（改名仍成立）、trim同名“默认菜单”、原索引首份回退。合法旧空库及删空保持空，不补种或加载历史回执。 |
| SR-05～08 菜单操作 | 成功关联/创建/覆盖/另存才记稳定ID；改名不改ID。dirty保存A再切B按阶段执行，A成功与B读取失败分别保留；选择不明停在已完成阶段。删除仅按实际记忆ID清null，保留有效其他选择及当前未保存副本，记忆重试不重复菜单保存/删除或重载。旧回执重放不制造选择事件，也不能丢弃新dirty内容。 |
| SR-09～10 故障分离 | 必需菜单读错/损坏阻断启动，不能当不存在；单独选择读错/坏校验/未知schema不覆盖raw，健康菜单仍可手动选。菜单事实与选择状态独立返回，页面明确显示“菜单已保存，启动选择未能记住”及专用重试入口。 |
| SR-11 写入顺序/不明状态 | SelectionStore与菜单写共享repository串行队列，容量/原raw与revision guard/写后回读。超时保留真实write settled状态并冻结冲突持久动作；旧值或更高值不能释放仍可能落盘的旧写。完整唯一拟写记录且Port声明singleWriteEffect才可提前确认；WxStorage单次调用setStorage且无内部重试。不是多进程CAS、跨key事务或物理断电保证。 |
| SR-12 生命周期/授权 | sessionEpoch、choiceEpoch、Session editGeneration及owner控制适用性。新关联即使同ID且无需IO也撤销旧补记授权；旧owner不能撤销新owner意图，unload后的回调/恢复不能回放旧工作副本。普通同页hide/show保留编辑；后台尚未应用的启动保持waiting，onShow重新读取再应用。 |
| SR-13 兼容 | 仅新增精确逻辑key `ee/selection`，沿用WxStorage原前缀和pack/unpack。v1 menu-selection记录含revision/baseRevision/menuId或null/operationId，校验严格；原根/索引/内容/bootstrap/receipt格式不变，既有非空menuId不加新格式限制。无根但有选择不是全新，其他未知key仍拒绝。 |

代码主要在 `src/selection.js`、`src/menu-controller.js`、`src/repository.js`、`src/session.js`、`adapters/wx.js`、`pages/index/`。原 `StartupCoordinator` 保留为低层bootstrap原语供旧调用及原独立测试使用，不再是原生页面应用入口；直接WorkingSession/repository CRUD仍只管菜单，不把检查读写当用户选择。产品 `docs/core-api.md` 与README说明当前接口、阶段结果、恢复条件和最少复测。

### 开发验证与原始证据

以下均由developer执行，不代替T-035独立设计/执行或合入批准。

| 环境/范围 | 实际结果与证据 |
|---|---|
| Windows、Node v24.22.1，保留本机配置工作树 | 完整命令 `node --test --test-reporter=tap test/*.test.js test/independent/*.test.js`，**559/559**，失败/取消/跳过/todo均0；产品本轮目录working.tap。 |
| 固定代码候选git archive干净导出 | 无.git/私有配置，同一完整命令 **559/559**，四类非通过计数均0；clean-export.tap，archive SHA-256 `74d101667ff78d94f0c3826d9e8572b000894ee30735df06c9388c09ed6dd02c`及22份运行文件SHA在candidate-manifest.json。 |
| 新专项及旧保护 | 两份startup-restore测试新增 **58个顶层检查**，覆盖上述SR-01～13；页面专项为回调级wx替身。原default-menu-page中四项开发者旧重进预期按新需求调整；原独立测试/历史证据 **229份** 原始字节完全保持，未删用例、skip或改tester结论。 |
| 真实微信开发工具、基础库3.17.4、SDK0.13.0 | **九组**隔离冒烟通过、0运行异常：默认8项及选择、SDK点击真实wx抽取、另存9项B后reLaunch恢复、切回A后重建恢复、未保存编辑不跨重建、真实保存handler中注入记忆失败的部分成功提示、实际重试handler只补记且菜单根不变、补记后重建恢复、真实WxStorage旧complete根无选择/已改名内置ID升级。native.json逐步记录。 |
| 视觉、隔离和清理 | 三张PNG已实际查看：默认8项、恢复9项B、部分成功及补记入口。页面注册前即注入独立前缀；临时工程只更改注册注入/工程名，正式AppID保留。最终仅移除本前缀13个key，非测试删除0，临时工程已关闭；CLI日志脱敏。未读取/清空实际用户菜单。 |
| 配置及固定候选保护 | 公共原始SHA-256前后均 `27a614c100e150979107c9f648b67f757d81b586883c9eb89a46f2670c143bbf`，私有前后均 `df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d`，两份均未暂存/改写；证据资产相对e3ce8cf只含本轮目录。 |

本轮产品证据目录README含SR覆盖/复现/限制，run.json含环境、计数和保护结果，保留三组historical失败及阶段通过TAP：旧页面预期4项失败、初版显式flag及同ID新关联后旧重试权限2项失败、B读取故障注入阶段错误1项fixture失败。authority退休与回归已修复；fixture改为在A选择写后才注入B读失败，不弱化断言。没有重写旧失败为通过。

首轮真实工具已通过前两步，SDK page.callMethod随后返回 `this.onCloseResult is not a function`，该方法实际存在；原native-first-failed.json及脚本保留，当轮清理4个本域key。脚本改为mini.evaluate调用当前页面handler，在同一已清理测试前缀重跑九步通过，产品候选未改。转盘为SDK按钮tap，其他动作由真实handler/控制器调用，不作为物理鼠标/手机触控验收。模拟器390×753/DPR3及其iOS字段不是实体手机环境，选择写失败为注入，不是物理满配额。

### T-035最少接收与停止点

tester由project-manager确认本成员停止后串行接续，先核对资产HEAD 9ab18b3e283b72f576055547453fbcfa5b192bd3、代码候选e3ce8cfecde344d3bd7c4c003ee427696fd9a877及本节配置指纹，读取现行需求/契约、docs/core-api与本轮证据README，自行设计SR-01～13独立风险用例。可先执行两份新专项，再执行上述完整命令及新的隔离真实平台路径；不复制测试配置覆盖正式配置，不清用户实际菜单，测试域必须在任何onLoad前注入。

本轮未新增实体手机/预览码、实体离线、物理满配额/强杀/断电、多设备、旧写入者降级及多进程CAS验证；单次Port写效果声明不证明这些条件。现无剩余研发阻塞，T-034实现/开发自测/固定候选/交接完成，成果仍draft，T-035尚待独立执行。两仓库提交本人内容并核对后停止，仅维护T-034/A-013/本人索引行；STATUS/PROJECT/索引顶部摘要由原维护人后续同步，不改WORKSPACE/T-035/A-014或旧H项，不发消息/唤醒成员，不执行合入、推送、移tag或发布，不清原.tmp/、私有配置及开发分支。

## 9. v0.6：BUG-T035-001修复开发交接（2026-11-03）

### 重开依据与精确候选

project-manager依据T-035独立缺陷明确重开T-034后串行通知，发送方及tester均已停止。本次实际接手OPC **fbddd9c508813293e6942a786f5309762c9b47e0**，产品 **48469cfa7594b127b0af549b9ecafe0698b47cc9**；已核对其相对首轮9ab18b3只增9份tester复现/证据，工作树干净，没有覆盖独立新增资产。TEAM、developer角色/scope和三项有效技能完整刷新，根公共协议及原需求/技术正文经Git比较未变；实际读取A-014 v0.7第11节、产品BUG说明和独立测试、当前任务及受影响实现。仍按D-024、13/13/40及SR-01～13执行，不扩展业务。

当前固定修复代码/新增开发回归/接口说明为 **5085636ba8955b08fcf39c24cf84b0329ef89c6d**；完整证据HEAD为 **ae61a36c59d95e24669010900b8d8b99b739cded**，后者只增加 `test/evidence/developer-fix-t035-001/`。仓库/本机checkout及配对feature分支与第8节相同；main/v1.1.0和WORKSPACE仍3800e6dd8bc32b7d767bfc873a4b9ae57d10232b，OPC目标仍2e241e6b489a65701ba2078640b2794cdabba429。全部仅本地单行提交，不合入/推送/移tag/发布或删分支，未联网刷新远端。

### 两层复现、原因及修复

在任何实现修改前，分别运行独立控制器 `--test-name-pattern="T035 SR10/12"` 与实际页面handler `--test-name-pattern="T035 page"`，文件均为 `test/independent/startup-restore.acceptance.test.js`。各0通过/1失败、退出1，确认同一BUG：A已记住，B记忆写明确失败，C加载成功但选择读失败；IO恢复后的重试取旧B并返回superseded，当前警示被清除，持久及新页面仍为A。原始before-controller.tap、before-page.tap保存于本轮产品证据目录，原独立文件/失败资产不改写。

原因是SelectionStore先读取选择，只有读取成功后才清旧pending；新C读失败提前返回时遗留旧B。retry随后把旧B的退休结果反馈给当前C，页面原先对superseded清warning。此次只改 `src/selection.js`、`pages/index/index.js`，新增 `test/selection-retry-repair.test.js` 并在 `docs/core-api.md`补说明：

- 新关联在任何选择IO前退休已结束的旧补记意图，即使新读取不可用也不会遮蔽当前授权。原uncertain检查仍在此前，尚可能落盘的物理写保持pending/冻结，不盲清或放行。
- 当前重试不带显式旧intent时以当前仍授权目标为准。旧意图的authority/目标不符不再阻断当前补记；已安全结束的raw/版本冲突至多新建一次意图重读，仍执行目标存在性、原raw/版本guard和写后核验。显式旧请求、旧owner或不匹配目标继续superseded；恢复不明状态前后均核对当前上下文，不能借旧目标已提交确认新目标成功。
- 页面对superseded明确提示“启动选择尚未确认，请重新选择菜单或重试记住菜单”。持续unavailable/failed/uncertain或guard冲突不会清警示；当前选择确认后才清除。补记始终不重存菜单、不加载或回放快照、不改变候选/基线/dirty。

### 当前开发验证与保护

以下是developer执行结果，BUG仍由tester独立复测关闭，T-035保持blocked/暂不可合入，不改其原报告或专业结论。

| 范围 | 实际结果及证据 |
|---|---|
| 原两层失败复现 | 修改前各1项失败、退出1；before-controller.tap、before-page.tap。控制器及页面均确认旧B/当前C/持久A，不把首轮开发559通过改写为独立通过。 |
| 新风险回归＋原独立文件 | 新增 **15个顶层检查**；与原独立四项合跑 **19/19**。覆盖同/不同ID×unavailable/failed/uncertain、未知格式、显式旧意图、已结束版本冲突、未settled写冻结、旧owner、不明恢复目标/authority、迟到读取及两组实际页面连续失败/guard反馈。targeted.tap；原独立文件原字节不变。 |
| Windows、Node v24.22.1工作树全套 | 完整命令 `node --test --test-reporter=tap test/*.test.js test/independent/*.test.js`，**578/578**，失败/取消/跳过/todo均0；working.tap。 |
| 同固定候选git archive干净导出 | 无.git/私有配置，相同命令 **578/578**，四类非通过计数均0；clean-export.tap。archive SHA-256为 `8b5f48ad960a5b2f34f4d4f78ccd9564b256412813566577650fd6bc9a063b9e`，22份运行文件SHA在candidate-manifest.json。 |
| 真实微信工具、基础库3.17.4、SDK0.13.0 | 新隔离工程 **八组通过、0运行异常**：A及B/C夹具；B实际handler选择写前失败；C选择读失败退休B并保留编辑；SDK点补记时持续读错警示保留；解故障后SDK再点确认C且全部菜单/完整编辑状态不变；reLaunch恢复C保存1项且真实wx抽取正确；guard冲突时返回superseded、SDK重试仍保留警示；解除后SDK补记B、新页面恢复B。native.json逐步记录。 |
| 视觉和隔离 | 三张截图已实际查看：当前C不可用提示/未保存标记、新页面恢复C、superseded保留提示。注册前即注入独立前缀，仅临时工程名/注册注入不同，正式AppID保留。只清本前缀8个key，非测试删除0；隔离工程已关闭，CLI日志脱敏；未读/清用户实际菜单。 |
| 原文件/配置保护 | 接手时原独立测试/历史证据 **260份** 原始字节不变，含tester本次失败资产；清单protected-before.json。公共配置前后SHA仍 `27a614c100e150979107c9f648b67f757d81b586883c9eb89a46f2670c143bbf`，私有仍 `df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d`，均未改写/暂存或读取私有凭据字段。 |

实际命令、环境、数量及阶段62/62、18/18记录在产品本轮README/run.json；最终19/19和双578/578对应齐备后的修复候选。原独立测试诊断保留初始fixedCode=e3ce8cf文字，没有为新候选修改其源码；实际被测修复由本节及manifest固定为5085636。SDK按钮tap不同于物理鼠标/手机触控；选菜单/编辑夹具通过mini.evaluate调用真实页面handler，存储故障按阶段注入，不是物理故障。模拟器390×753/DPR3及iOS字段不作为实体手机环境。

### 复测接收与停止

tester由project-manager核对本交付并确认本成员停止后串行接续，先核对代码5085636与完整资产ae61a36精确SHA、差异仅本轮证据目录及两配置指纹，读取原BUG、现行需求/契约和产品修复README，再独立复跑两层失败和相邻风险，继续完成T-035原定剩余全套及隔离真实平台检查。BUG-T035-001仍open，独立关闭/解除blocked和合入建议由tester给出，本成员不代写。

未新增实体手机/预览码、实体离线、物理满配额/强杀/断电、多设备、旧写入者降级及多进程CAS结论；原不明写冻结、未知格式及Port单次写前提保持。T-034本次重开修复已满足实现、开发复现/回归、固定新候选和交接条件，成果仍draft。两仓库本地提交本人范围并核对后停止，不联系/唤醒其他成员，不改T-035/A-014、STATUS/PROJECT/索引顶部摘要、WORKSPACE或旧H项；原.tmp/、私有配置和开发分支保留，不合入/推送/发布。

## 10. v0.7：T-041首页分享开发交接（2026-11-05）

### 接收、版本与实现

按project-manager依用户既定授权及D-027发送的串行通知实际接收；发送方已停止。已重读根AGENTS/SESSION_PROTOCOL、TEAM、自身developer角色/scope与software-engineering、frontend-development、wechat-miniprogram-development三项技能，根公共规则经Git比较未变，读取D-027/T-040～T-042、当前项目、索引及本人交接。产品代码缺少两个分享回调的原定位保留为输入，不改写为本轮测试结果。

接手OPC **fc5cc8b78dd2c05bf5de2e7483dfa1d5b031e447**，产品main/v1.2.0 **38b1e69350ad2b4cf43c3124a3635f4a0b9c47a0**；已确认配对分支及WORKSPACE。固定代码/开发回归/说明 **fb27d41c598a52425f3401c1cbd17b4ecc2129fd**，完整证据HEAD **cc9c1912996035ddfa3a2b39e7f1cd062312e7f6**，后者只增加 `test/evidence/developer-t041-share/`。当前产品feature/t-040-share、OPC feature/eating-everyday/t-040-share仅本地提交；两主线/v1.2.0/WORKSPACE保持本节基线，未合入/推送/打tag或发布。

实际页面定义新增onShareAppMessage/onShareTimeline，标题固定“今天吃什么”；好友path固定 `/pages/index/index`，朋友圈使用当前首页并显式 `query:''`。两份代码包静态转盘PNG分别5:4及1:1，不使用当前页面截图，不拼菜单/选择记忆/抽中结果，无shareTicket或参数导入，不从截图账号名改产品名称。接收方普通首页仍读取自身保存菜单。

onLoad在独立于菜单读取的步骤请求shareAppMessage/shareTimeline；无API、同步异常或异步失败不阻止启动，只做一次好友入口回退。隐藏/卸载后的旧失败回调不更新菜单。分享回调是纯元数据返回，不保存/加载或修改工作态。场景1154跳过分享菜单API，原菜单/恢复/抽取行为保持。变更为pages/index/index.js、两份assets PNG、新test/sharing.test.js及README/docs/sharing.md，无新增业务依赖或云服务。

### 官方现行接口核实

2026-11-05直接读取三份微信官方正文，均HTTP 200，链接/HTML与提取正文SHA保存在产品official-interfaces.json；最初Web读取器不可达后按工具权限流程通过只读HTTPS取得原文。不是只靠旧类型声明。

- [Page分享回调](https://developers.weixin.qq.com/miniprogram/dev/reference/api/Page.html)明确好友完整path、静态PNG/JPG和朋友圈query/image返回；朋友圈不接受自定义path。静态分享图避免默认截图携带本机内容。
- [朋友圈专页](https://developers.weixin.qq.com/miniprogram/dev/framework/open-ability/share-timeline.html)现行编辑日期2026-06-16，支持Android及微信8.0.24以上iOS，基础库2.11.3起；页面同时具备好友与朋友圈处理后，入口由客户端支持情况决定。单页模式存储与普通模式不共用，原生底部“前往小程序”进入普通模式后才使用接收方原菜单，不做跨模式复制。
- [showShareMenu](https://developers.weixin.qq.com/miniprogram/dev/api/share/wx.showShareMenu.html)允许同时请求两个入口，朋友圈不能脱离好友入口；参数说明仍有旧Beta/Android字样，代码没有据此硬编码排除iOS。单页场景禁用的API不调用。

这些版本条件是官方文档范围，不是已实测所有手机或账号能力；产品docs/sharing.md说明单页差异和最少验证入口。

### 开发检查、保护和平台待补项

| 范围 | 本轮实际结果 |
|---|---|
| 新页面/集成专项 | **13/13**，test/sharing.test.js；真实页面定义接入固定元数据/PNG打包、回调纯度、缺失/同步及异步失败/迟到API、单页场景、普通接收方自身菜单、分享准备/取消/前后台/未保存编辑及迟到抽取。wx为回调级替身；不是手机分享或真实单页模式。 |
| Windows、Node v24.22.1工作树全套 | `node --test --test-reporter=tap test/*.test.js test/independent/*.test.js`，**622/622**，失败/取消/跳过/todo均0；working.tap。 |
| 同固定候选git archive干净导出 | 无.git/私有配置，同命令 **622/622**，四类非通过计数均0；clean-export.tap，archive及24份运行文件SHA在candidate-manifest.json。 |
| 静态分享图 | 两份实际查看，几何绘图与原配色一致，不含菜单名称/结果/个人信息。实际生成器随产品本轮证据保存，未新增图片网络服务。 |
| 原独立/历史证据 | 接手 **310份** 原始字节不变，含旧独立断言和原失败/通过资产；protected-before.json可逐项复核。 |
| 配置保护 | 公共原始SHA始终 `eecc59987ea459258c2bf0329b69d98e73e790dfa6110d60f28ed3645456e645`，私有始终 `df846e395e410139062950e08f81795908fc694a5755b96e0970551f11aee63d`；公共过滤blob与索引始终 `c2c297e3db63062e72a6ef0476e02a611914dbba`。原已协调CRLF导致的M仍携带，未格式化、暂存或提交配置，不能把产品树称为完全干净。 |

**真实分享菜单、卡片预览、取消路径及真实wx行为本轮未验证通过。** 本机Stable 2.02.2608080隔离工程从Page注册前即注入新前缀，正式AppID/其余配置来自Git候选，仅临时工程名/注册注入不同，不复制私有配置。官方auto两次在Fetching AppID permissions阶段ECONNRESET/TLS建连失败，虽CLI退出0，原文明确失败。通过已安装工具和官方open打开隔离工程后，唯一标题窗口显示“模拟器启动失败 / Error: simulator launch failed”，不是首页或分享菜单；在已打开工程启用auto仍同错。未改分享代码的v1.2.0隔离对照auto也在同一AppID/TLS阶段失败，证明该自动化启动故障并非分享代码特有；UI模拟器失败的具体原因未进一步诊断，不冒称产品通过或新增业务缺陷。

原失败CLI、open/close及对照日志脱敏保留；native.json标为setup_unavailable，ui-observation.json记录实际观察层级。只关闭本隔离工程，窗口清单已确认该标题消失、对照工程未打开。没有自动化运行时，未通过API枚举/清理测试key，也未核实本测试前缀剩余数；可能的页面执行已从注册前隔离，前缀留待环境恢复后核对，没有清空任何用户缓存或菜单。配置目标基础库3.17.4不写成已实际查询到的运行版本。

### T-042接收与停止

本次按实现、开发自测、官方核验及可执行平台尝试交付，T-041研发完成；真实平台待补项明确留存，不等于分享已独立通过或正式手机可用。tester由project-manager核对精确候选/本成员停止后串行接收，读取D-027/T-041、docs/sharing.md、本轮证据README和既有菜单契约，独立设计参数/旧API/生命周期及回归。先核对代码fb27d41与资产cc9c191的完整SHA及差异仅本目录，再跑新专项和全套；工具启动恢复后须补真实菜单/静态卡片/取消与数据留存，使用新的独立前缀，任何实际发送/发布前停止。

本轮未做好友/群外发、朋友圈发布、真实iOS/Android收卡/单页模式或账号权限验收。两仓库只提交本人范围后停止，不联系/唤醒tester或其他成员，不代写T-042/A-014，不改STATUS/PROJECT/索引顶部摘要/WORKSPACE、旧专业正文或.tmp/，不合入/推送/打tag/GitHub Release/微信上传提审发布。原独立T-035及旧BUG事实由原报告对应版本保留，不由本次分享交接改写。

## 11. v0.8：T-041分享图解析兼容修复（2026-11-06）

### 实际接收与固定版本

本轮2026-11-05接收、2026-11-06完成（Asia/Shanghai），依据T-041下项目经理的明确重开安排，发送方及其他成员已停止。OPC接手22c61fe62e1c899f994fd2bf7d3f39340ab75c62，产品接手939d55f07322afebe889943dbae9445dd9c1439d，仍使用原配对t-040-share。本人TEAM登记、配置/角色/scope及三项有效技能完整刷新；根规则、原需求/架构经Git比较未变，补读A-014 v0.10第14节、tester续测README/OBS/截图、原分享说明及相关契约。未重新授权、改登录/安全设置或安装SDK。

代码/开发回归/说明固定为 **a647d1f04acc7d8022699baa0a5c54af1856c4ee**，父为939d55f；完整证据 **e280b9559d119c6e66cfdc01b42e4d9d4929c522**，后者仅追加产品`test/evidence/developer-t041-image-20261105/`的88份文件。两条提交信息为含T-041的单行。本文及TASKS保留第10节原fb27d41/cc9c191开发结论，不移动标签或把新候选冒充主线采用版本。

### 图片解析链与最小修复

新隔离工程中实际鼠标打开原好友预览，标题正确而图破。只把`/assets/share-home.png`换为页面相对`../../assets/share-home.png`、实际getImageInfo返回的`assets/share-home.png`，均仍破；三次PNG原字节不变。原两图PNG结构/CRC有效，getImageInfo成功不当预览通过。随后将**同一PNG原样复制为本地用户文件**，好友和朋友圈预览均显示；真实FS逐字节比较18,987/18,143字节一致。证据为原始/relative/canonical截图、image-paths、local-bytes与local-file两预览，旧TLS失败不作为本次图片归因。

因果边界是本机环境的包内`imageUrl`到分享预览的资源解析兼容：本地文件形式能显示同一图片，不能据此泛称全部手机路径错误或已证明工具内部通用缺陷。未重编码图片、去掉图片、替换成当前页面截图或引入网络托管。

产品只改页面接入、新增`ui/share-images.js`与5项开发测试并补分享说明。`onLoad`准备两个固定命名的本地静态图，回调只返回已准备路径；图片仍来自包内中性图，不携带候选、选择、结果或用户参数。文件API缺失/抛错、缓存不可写/容量不足时按图回退包内路径，不阻塞菜单启动，也不承诺该失败环境图片已显示；1154不请求文件写入。每次新页重建固定两文件，不逐次分享累积缓存。实际工具`http://usr`是微信本地文件映射，不是外部图片服务。

### 固定候选实际验证

| 范围 | 本轮结果与边界 |
|---|---|
| 开发完整套件 | Node v24.22.1，工作树与固定a647d1f的无.git/私有配置干净导出各**652/652**；fail/cancel/skip/todo均0。新增5项已包含，旧647、原13项及多次执行不叠加。原独立断言未改，本次执行者为developer。 |
| 新测试重点 | 返回本地文件与原图逐字节一致、回调无额外IO/私有菜单；缺失文件能力/抛错/容量失败时启动及按图回退；新页固定两文件恢复；1154文件请求数0。替身故障不冒称真实物理满配额。 |
| 实际环境 | 已安装wechatide 0.3.11、工具2.02.2608080、实际库3.17.4，模拟器390×753/DPR3。状态equal、登录有效、CLI无需token。新导出只注入页面注册前独立WxStorage前缀与临时projectname，不复制私有配置；固定helper/页面业务正文无诊断替换。 |
| 真正好友/朋友圈预览 | 固定候选实际鼠标打开两预览，标题与中性转盘图均正常；`fixed-friend.png`、`fixed-timeline.png`由官方simulator_screenshot保存并实际查看。只取消/X，未发送、发布或进入朋友圈单页链接。 |
| 两取消保护 | 真实handler准备9项已保存菜单、10项dirty副本。两取消后候选含ID/顺序、菜单身份、基线、dirty、phase/result/storage状态及持久字节均保持。既有onHide使generation由4变5以失效抽取；该计数器未冒称不变。 |
| 真正关闭重开 | 旧窗口4524796消失后新窗口4459596、新页面恢复同一menuId的9项保存内容、dirty=false、存储字节一致，无旧fixture属性；未保存第10项不留存。新窗口好友图仍显示，取消后数据保持，`fixed-reopened-friend.png`及原生快照可核对。 |

`verification.json`验证上述快照、运行时字节/两处临时注入和文件保护；`manifest.json`固定87份证据文件哈希（本清单本身另计，目录共88）。原始失败均保留：首次运行时查询超时由一次currentPage重试恢复；桌面输入曾被ChatGPT遮挡，用户把唯一隔离窗口置前台后才继续。新包装器help参数错误、跨仓库Git读取信任范围错误分别保留并按真实接口/各自cwd解决，未改全局trust。初版新离线核对器将generation与业务态一并判相等而失败，保留`verify-first.py`/失败说明；修正为业务各字段全等并单独断言真实generation递增，未改产品或原测试造绿。这些不是新增产品缺陷或独立通过。

### 保护、精确清理与后续

接手475份受控文件中，仅预期页面和分享说明改变，另新增helper与开发测试；所有原独立断言、失败/通过证据、源PNG及其他实现保持。公共配置SHA-256 `27a614c100e150979107c9f648b67f757d81b586883c9eb89a46f2670c143bbf`、私有`d5db685f26b2319769eb494e7dc43ff31bcb5a9f0ae0aa626ac22892e944299a`均保持，不回写旧环境指纹。

本轮control精确域`eating-everyday:developer-t041-image:10c00a036ae443788d4ba25ab9111764:`为4→0；fixed域`eating-everyday:developer-t041-fixed:3758b1dc538b4512a605030b9c2f6180:`为6→0，baseline为0，非测试key删除数均0。两份唯一对照文件已清；fixed加载前两固定缓存文件明确不存在，本轮新建后与源图全字节一致才清除，并核实文件不存在。没有读取/清除用户菜单值，旧C盘不可访问域余数仍未知。本次隔离工程最终关闭、标题已从新窗口清单消失；忽略目录中的复现工程保留。

**T-041本次研发completed，A-013仍draft；T-042/OBS-T042-001不由developer关闭。** project-manager核对固定代码a647d1f、证据e280b95及停止回执后，再串行通知tester独立复测两图、取消、真正重开和文件能力失败边界。未执行实体iOS/Android收卡、实际外发/朋友圈发布、真实单页/前往小程序跳转、物理故障或发布验收。原长期分支差异按T-040保留，无合入、推送、改tag/WORKSPACE或删除分支；本成员仅本地提交T-041/A-013/本人索引行后停止，不联系或启动其他会话。
