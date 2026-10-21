# 原生小程序工程与平台复核交接

- 项目：eating-everyday（今天吃什么）
- 成果 ID：A-013
- 负责人：developer（研发工程师）
- 关联任务：[T-012](../TASKS.md#t-012补齐原生小程序工程与开发工具预览)，供[T-013](../TASKS.md#t-013验证小程序导入编译与预览闭环)串行接收。
- 版本：0.1
- 更新日期：2026-10-21（Asia/Shanghai）
- 状态：draft
- 适用范围：完整原生工程、真实wx端口、页面与开发自测，以及准确的平台尝试/停止记录；最终显示与新预览尚未验证，T-012仍blocked，不是整体验收。
- 来源及输入：本次用户授权及项目经理T-012安排；[A-001 v0.9](product-requirements.md)、[A-007 v0.3](product-decision-options.md)、[A-006 v0.4](system-architecture.md)、[A-008 v0.2](accounts-access-checklist.md)、[A-010 v0.1](core-test-plan.md)、[A-011 v0.3](developer-core-handoff.md)、[A-012 v0.3第7节](core-test-report.md)，现行D-005/D-011/D-014/D-016/D-017。
- 存储位置：本治理正文在OPC artifacts；产品源码、测试、中间证据全部在本机产品仓库git@github.com:yinzhongkai/eating-everyday.git，checkout=../eating-everyday。
- 固定工程候选：**f7ceaf791dd06e8749fa1754b6a0143d1193fb63**，产品feature/t-002-readiness-review，父提交tester资产f2646b52f7e6746b2814076bfab99e91a5e95f07，继承已独立通过的核心实现54b720fa920dfcda9a3db5fa0e42378be01f5535。原历史与24份tester资产逐字节保留。
- 主线交付：未合入、未推送；产品main/已有origin/main/WORKSPACE.revision仍为8fa87fe4cb8f7f33aacf2842798a486150a6105e。OPC沿用feature/eating-everyday/t-002-readiness-review，目标project/eating-everyday及已有origin目标仍a735031c37c09e75fb9478681aaf42010a82f378。
- 批准依据：无整份成果批准；本次授权允许开发和本地候选交接，不代写tester平台结论，不将开发自测当作独立测试。
- 版本记录：0.1首次原生候选；最终工具复核因用户Esc停止，按项目经理后续安排仅完成独立的源码、桌面验证与本地提交。

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
