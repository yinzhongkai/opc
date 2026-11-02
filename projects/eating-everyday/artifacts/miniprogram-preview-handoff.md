# 原生小程序工程与平台复核交接

- 项目：eating-everyday（今天吃什么）
- 成果 ID：A-013
- 负责人：developer（研发工程师）
- 关联任务：当前[T-022](../TASKS.md#t-022实现首次自动加载内置默认菜单)，供[T-023](../TASKS.md#t-023独立验证默认菜单及已有数据保护)串行接收；T-012～T-016等已完成事实保留，不重开。
- 版本：0.4
- 更新日期：2026-11-02（Asia/Shanghai）
- 状态：draft
- 适用范围：T-022一次性默认菜单、旧菜单/空库及失败恢复保护，实现、开发自测与隔离真实wx冒烟；不包含本次独立或手机验收。第1～6节保留旧原生工程及配置测试历史，第7节为本轮当前交接。
- 来源及输入：2026-11-02用户D-019及TASKS开头串行派发授权，project-manager通知T-022；OPC接手19005a6b980a3bf6fbe8e4f7f6e704a46c36961f。完整读取[A-001 v0.10](product-requirements.md)、[A-007 v0.4](product-decision-options.md)、[A-006 v0.6第10节](system-architecture.md)、[A-008 v0.4第0节](accounts-access-checklist.md)及T-020/T-021实际结果、T-023要求；12/12/32与DM-01～DM-12为当前输入。原T-012/T-014的输入及结果保留各历史段落。
- 存储位置：本治理正文在OPC artifacts；产品源码、测试、中间证据全部在本机产品仓库git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday。
- 固定工程候选：代码/测试/最小说明提交**dcb22d3bdcec196e37b544805f79196de126f676**，父为产品发布基线6c9ef2b3135a0013fb0b06ebbe761b298c4b7260；完整证据资产HEAD **33e3e64cd3e59d48adbd13d69622a82087468016**，后者相对代码候选只增加test/evidence/developer-t022-default-menu/，运行代码与测试不变。
- 主线交付：本轮产品feature/default-menu与OPC feature/eating-everyday/default-menu仅本地提交，未合入/推送；产品main/WORKSPACE保持6c9ef2b3135a0013fb0b06ebbe761b298c4b7260，OPC目标project/eating-everyday为8474050eb2ddf6b1a414f649c4ba242636ecd4e8。既有产品v0.1.0和OPC项目发布标签保持固定，本次未联网核对远端；当前代码候选不是已合入交付基线。
- 批准依据：D-019确认功能范围、T-022授权本成员实现/开发交接；无整份成果批准，不代写T-023独立结论或执行后续合入、发布、WORKSPACE推进。
- 版本记录：0.1首次原生候选及Esc停止历史；0.2随机跨域修复与开发预览；0.3配置测试适配及双环境结果；0.4默认菜单一次性初始化/兼容/恢复及当前开发验证，交T-023独立接续。

以下第1～4节为v0.1、第5节为v0.2、第6节为v0.3的历史事实；**当前T-022以第7节v0.4为准**。旧输入SHA、首次为空、Esc、未验证手机和待关闭缺陷均限于原执行时点，不覆盖当前契约；既有专业测试事实以其原任务/A-014对应版本为准，不倒写旧失败。

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
