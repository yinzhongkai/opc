# T-043 OPC提交按功能压缩

2026-11-04（Asia/Shanghai）；project-manager；本地整理completed，远端同步待SSH认证恢复。

按用户要求，把保留基线c7ad181之后至cb47ef7的58条提交压为11条功能提交。每条完整commit message只有一行，无正文。另用1条独立治理提交记录本次整理及来源查询表。

## 仓库与版本

- 本机：`C:\Users\YinZh\Documents\workspace\opc-free-pipi-shrimp`；远端`git@github.com:yinzhongkai/opc.git`。
- 分支：`feature/free-pipi-shrimp/t-023-station02-first-two-visual-assets`。
- 保留基线：`c7ad18166b0e2c4fca152c3c9a24fdea3110af75`，对应项目长期分支，未修改。
- 原功能头：`cb47ef73d5fd98438f3f5ef7348fe7d4b292dc95`。
- 整理后功能头：`16dcd268127635c15b227ec7e0c84149d1a54fd4`；最终tree仍`05a3d885c1622f5927c8da77a104587e6f3b8389`。
- 独立治理提交只增加本轮回执、索引及任务状态；不改业务成果，不将功能分支合入项目长期分支。

## 功能划分

| 功能 | 原提交数 | 新提交 | 单行说明 |
| --- | ---: | --- | --- |
| 前两篇视觉资源与评审 | 4 | a881c60 | feat(T-023): register and review station02 first-two visual assets |
| 前两篇文案、套版与审校 | 19 | 4d0e3d7 | feat(T-024): prepare reviewed first-two copy and illustrated drafts |
| 前两篇运营与首站发布补记 | 3 | 77803f6 | feat(T-025): record first-two operations checks and station01 publication |
| 后三篇规划 | 4 | 2805cb6 | feat(T-026): plan remaining station02 stories and serial delivery |
| 后三篇文案、视觉与说明 | 6 | 0efcb5a | feat(T-027,T-028): deliver remaining-three copy, visuals and disclosures |
| 审校、整合与五篇验收 | 6 | aa583f2 | feat(T-029,T-030,T-031): review, integrate and accept five-post candidates |
| 运营总包、采用与发布安排 | 5 | 2cda9f9 | feat(T-032,T-033): prepare operations package and confirm publication plan |
| 产品主线集成与基线登记 | 1 | 8a0a6cc | chore(T-034): record product integration and workspace baseline |
| 本地素材清理与首站归档 | 2 | f5a1da1 | chore(T-035,T-036): clean local assets and archive station01 editing files |
| 远端素材迁移与历史恢复 | 5 | 1481fda | chore(T-037,T-038,T-039): migrate final assets and restore browsable history |
| 引用及旧隔离副本清理 | 3 | 16dcd26 | chore(T-040,T-041,T-042): clean product refs and obsolete OPC work copy |

## 验证与保全

在独立副本执行交互式rebase及fixup，再导入新Git对象，按原头SHA条件更新当前功能引用。每条功能提交的tree与原组最后一条提交的tree逐一相同，58条来源完整覆盖且无重复；最终tree与旧cb47ef7相同，无新增merge。

实际工作目录没有执行checkout或reset。176份跟踪文件、17份未跟踪temp原字节及当前索引tree保持；之后只修改本次治理记录。其他本地分支、标签及跟踪引用保持，当前产品main／WORKSPACE仍`5061a76052e19d393988c1932f92912d60c9e745`，产品跟踪文件原字节保持。独立副本及当前仓库fsck通过，框架／链接校验通过。

## 原始快照与查询

原始源历史已保存在本地自包含Git bundle，独立克隆及fsck验证通过：

- `C:\Users\YinZh\Documents\workspace\opc-free-pipi-shrimp\.git\history-consolidation-t043-20261104\before-consolidation.bundle`。
- 大小1384005字节，SHA-256为`2f7b68ed422ec53e3c84d6a962d96bf925bad816f314f600ea72e274ead0a6f1`。

[来源查询表](T-043-opc-commit-map-20261104.tsv)与[结构化回执](T-043-opc-functional-consolidation-20261104.json)提供全部58条旧OPC SHA到11个功能提交的归属。该映射是多对一聚合；只有原组末尾的tree与新功能tree相同，组内较早提交的精确快照必须从bundle恢复后读取，不能把新功能SHA冒充原评审时点。既有任务正文里的旧OPC SHA保留历史含义，产品SHA及其T-039映射不变。

需要读取旧时点时，可在另一个空目录从该bundle克隆镜像，再执行`git show <旧OPC SHA>:<路径>`。原提交的作者、日期、完整说明和原始快照都在备份中保留。本轮未新增远端备份标签或分支。

## 远端同步

执行前读取的真实远端功能头为`8b2f5e7c75677dc930bf4a26aa8b3207d57e6501`，项目基线仍c7ad181。已使用该旧头的显式force-with-lease及原子推送尝试同步，GitHub返回`Permission denied (publickey)`，尚未同步成功。

认证恢复后应重新核对远端旧头，再保护性更新本功能分支；若旧头变化则停止核对，不能无条件force。当前本地结果及备份已保存。本机.git/history-consolidation-t043-20261104/还保留计划、原工作文件哈希、rebase日志和推送回执；临时rebase副本已删除。
