# 产品v0.1.0发布merge与标签记录

- 项目：eating-everyday（今天吃什么）；成果ID：A-016；负责人：project-manager。
- 版本：0.1，2026-10-22（Asia/Shanghai）；状态：draft；关联任务：[T-017](../TASKS.md#t-017为成果物v010创建发布merge提交与标签)。
- 来源：用户最新明确要求“先给成果物仓库打一个release的tag 要求有merge commit”，调整先前先更新文档、最终独立复核再发布的顺序；本次只先固定已有产品代码的Git发布点。

## 实际版本与提交结构

| 项目 | 实际结果 |
|---|---|
| 成果物仓库 | git@github.com:yinzhongkai/eating-everyday.git；本机checkout为相对OPC根目录../eating-everyday。 |
| 发布前main/源码基线 | 95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0 |
| 发布准备提交 | fe5045769e83044a0b75185dd2c72a329de39b01，source为feature/t-017-release-v0-1-0；空提交仅记录版本准备，不改文件。 |
| 发布merge/main | [6c9ef2b3135a0013fb0b06ebbe761b298c4b7260](https://github.com/yinzhongkai/eating-everyday/commit/6c9ef2b3135a0013fb0b06ebbe761b298c4b7260)；message为`release(T-017): v0.1.0`，只有一行。 |
| 第一父提交 | 95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0（合入前main）。 |
| 第二父提交 | fe5045769e83044a0b75185dd2c72a329de39b01（版本准备提交）；恰好两个不同父提交。 |
| 附注标签 | [v0.1.0](https://github.com/yinzhongkai/eating-everyday/tree/v0.1.0)，tag对象d12b4b9c30f198400ca6fa15d2a36b4b19c94ef7，剥离目标为上述发布merge。 |
| 文件树 | a7cf6d7d3218a13b5fb35e2a7d80ebbfa9f488f2；发布merge、版本准备和原95173d4的tree完全一致，`git diff 95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0 6c9ef2b3135a0013fb0b06ebbe761b298c4b7260 --name-only`为空。 |

按用户本次明确要求，产品发布以`git merge --no-ff`形成实际merge commit，作为普通feature默认快进规则的本次发布例外；没有修改公共框架策略，OPC项目记录继续--ff-only合入。版本准备与merge的提交说明均为单行；该新增发布结构是用户最新要求，不再把新增范围称为一条普通提交。

## 实际验证与远端核对

在产品main发布merge上执行`node --test --test-reporter=tap test/*.test.js test/independent/*.test.js`，299/299通过、退出0，失败/取消/跳过/todo均0。目标树与原已交付、主线及干净导出各299通过的95173d4相同；本次结果由project-manager实际运行，不冒充新增tester独立复核或微信正式验收。

main与标签通过`git push --atomic origin main:main refs/tags/v0.1.0:refs/tags/v0.1.0`同步，未强推。实际读回GitHub main=6c9ef2b3135a0013fb0b06ebbe761b298c4b7260、v0.1.0对象=d12b4b9c30f198400ca6fa15d2a36b4b19c94ef7、剥离目标=6c9ef2b3135a0013fb0b06ebbe761b298c4b7260；已确认tag类型为tag、release为两父commit。

产品必要中间证据在已声明工作区忽略目录`.local/t017-release-v0.1.0/`：`release.json`记录父提交/树/tag对象，`remote-verified.json`保存读回引用，`release-main-299.tap`保存原始目标分支结果。配置工作树原始SHA-256保持97f3a1355a54a3c8875f6da96580357c1fbc3a639079e009acf0aa07c6bae2db，私有配置保持988e8a38615d839bbe92a89bf5b255e6ccb7be5c218ab1b8bbc71593870ccff4且被忽略；没有修改源代码、配置或原始测试证据。新发布main工作树干净。

## 交付引用与后续边界

WORKSPACE仅推进revision至6c9ef2b3135a0013fb0b06ebbe761b298c4b7260，repository/branch/driver及相对checkout不变；OPC同步该真实产品版本并验证后，快进合入项目主线、推送并安全清理本轮两个临时分支，最终结果由Git引用及会话回执确认，正文不写自身OPC哈希。

当前仅创建并推送了Git标签，GitHub Release页面尚未创建。README与发布说明更新、最终独立复核尚未执行，本轮未向developer/tester发送任务；既有手机来源与设备/附件等限制仍按A-014保留，正式小程序账号由用户准备。v0.1.0固定当前文件树，后续文档或代码不能通过移动标签纳入；后续接续需区分该已冻结版本和新变更。没有执行微信上传、提审、发布或账号办理，不据Git标签认定微信正式可上线。

## 版本记录

| 日期 | 版本 | 变化 |
|---|---|---|
| 2026-10-22 | 0.1 | 按用户最新顺序记录产品发布merge、附注v0.1.0、299项实际结果及GitHub原子推送核对；同步精确工作区版本，保留尚未执行的文档/独立复核/Release页面边界。 |
