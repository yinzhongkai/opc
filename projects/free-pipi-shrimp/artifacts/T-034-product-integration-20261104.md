# T-034 产品库第二站功能整理与main集成记录

项目free-pipi-shrimp；负责人project-manager；2026-11-04（Asia/Shanghai）；状态completed。

## 用户授权与集成范围

用户直接要求“把feature/t-023-station02-first-two-visual-assets分支的内容合并main分支，要求按功能进行压缩，并且只保留一行commit message”；此前同会话已授权两仓同步远程。本次按该授权整理产品共享功能分支，以原头归档、明确旧头lease和原子推送保护；main只作普通快进，不重写其既有历史。

本轮只合并产品库main，OPC在原feature回写精确产品基线和治理证据，不合入OPC稳定框架或项目长期分支，不删除分支；不修改角色scope、已采用图文、专业审校意见或实际发布状态。

## 精确版本

- 产品仓库：git@github.com:yinzhongkai/free-pipi-shrimp.git；本机C:/Users/YinZh/Documents/workspace/free-pipi-shrimp。
- 来源功能分支：feature/t-023-station02-first-two-visual-assets；整理前本地／远程头b938a81860945acc527b8198d554fb67a83d78e4，16个提交、无merge commit。
- 合入前main／origin/main：b708c850e1d38f6e10eb8c4fb91359f9afd205c9，是来源分支祖先。
- 最终main／origin/main及产品功能分支本地／远程：771c9d2be5427cfa59decfed53acf26dffa93066，由--ff-only合入，当前checkout为main。
- 原历史保留：注释归档标签archive/t-023-pre-squash-20261104，本地／远程均指向原b938a81860945acc527b8198d554fb67a83d78e4；全部16旧提交继续可达，既有成果及旧SHA引用不改写。
- OPC接收头：f86a6072a5eafebe5029bc02bd2036a5a216f869；本轮仍在feature/free-pipi-shrimp/t-023-station02-first-two-visual-assets回写，仅推进WORKSPACE.revision从b708c850e1d38f6e10eb8c4fb91359f9afd205c9至已验证的771c9d2be5427cfa59decfed53acf26dffa93066，其他配置字段不变。

## 按功能归并与单行信息

按以下8个原阶段最终tree构建单父功能提交，保留各模块原作者。T-024的8个文案／套版／修正／整合提交归为前两篇完整图文交付，T-032的2个运营版本归为全站采用版发布包；其他6项各为独立功能。不是使用merge --squash生成替代合并提交，进入main采用--ff-only。

| 功能顺序 | 原提交数 | 新提交 | 完整单行commit message |
|---|---:|---|---|
| 1 | 1 | `03b1d2ad1f81dae634882c5359b0369521e26fbd` | feat(T-023): add Emeishan first-two visual assets |
| 2 | 8 | `da0be8e22abb63ae510e7e121ec718cee736a337` | feat(T-024): deliver first-two illustrated Emeishan plogs |
| 3 | 1 | `96a7a59346545cb717317ffaf4cc9ab55137aed2` | feat(T-025): prepare first-two Emeishan operations handoff |
| 4 | 1 | `c2a7c4833ac185471592fadee5f00d6a3c8c8d80` | feat(T-027): draft remaining three Emeishan plogs |
| 5 | 1 | `14d4fdd613489d2e156e9d9a0a275bd02ad82d59` | feat(T-028): illustrate and typeset remaining Emeishan plogs |
| 6 | 1 | `b1596c8a9be6652e2656f546fa2d3848047d9e81` | fix(T-027): align AI disclosures with generated illustrations |
| 7 | 1 | `0624abdf8e6d37426a3bd0c6f700d5679db4b806` | feat(T-030): integrate remaining three reviewed Emeishan plogs |
| 8 | 2 | `771c9d2be5427cfa59decfed53acf26dffa93066` | feat(T-032): prepare adopted five-post Emeishan publishing package |

新范围b708c850e1d38f6e10eb8c4fb91359f9afd205c9..771c9d2be5427cfa59decfed53acf26dffa93066共8个单父提交、0个merge commit，每个完整%B正文恰好一行；8个功能检查点tree逐个等于其原阶段tree。

## 内容与目标验证

- 整理前／后／合并后最终Git tree完全一致：69c5a1f5a2c048bf850e7a46ab09526fa0218634。664个受跟踪文件的实际大小／SHA逐项相同，索引mode／object序列SHA-256仍为59bb3751dae0271b10ef3f412c3e09caca4dfe1db632f6a6d3264efdad5a2500，工作树干净，无新增业务文件或媒体。
- 检查前两篇v0.3整合、后三篇v0.1整合、T-032运营v0.1及v0.2四份当前业务清单，共153个文件条目的Git LF或LFS实体大小／SHA全部通过。
- 五组原公开字段与采用版逐项一致；40张不同正式PNG的原路径／顺序／大小／SHA／1080×1440通过，合计98101481字节；本轮25个已采用标签及5档确认计划与原输入一致，实际提交／公开等结果仍空。
- 在合并后的main实际运行完整tools/verify_asset_retention.py：passed，working_bytes_checked=true；179图片路径、125唯一LFS对象、289094702字节、83当前SVG和79图像依赖，精确LFS白名单及实体均通过，无新增唯一对象。
- checkout后对9份JSON按原快照恢复原工作区换行形式，对其规范化哈希与HEAD逐一确认相同后重记索引状态，最终零暂存／工作树差异；未产生额外内容提交，664份原工作区字节及索引均保持。
- 原阶段脚本含当时main／分支／QA前提，保留原文作为历史方法。本轮用合并不变性、当前清单／采用字段和完整素材检查验证，不冒称重跑历史1176等阶段计数。用户已要求删除的10个本地T-031 QA文件未恢复；历史验收记录按原时点保留。

## 远程与回写

推送前再次确认远程main仍b708c850e1d38f6e10eb8c4fb91359f9afd205c9、远程功能分支仍b938a81860945acc527b8198d554fb67a83d78e4；以atomic同时同步main、整理后功能分支和原头归档标签。main普通快进，只有产品功能分支以明确旧头--force-with-lease保护更新。推送后ls-remote实际回读main／功能分支均771c9d2be5427cfa59decfed53acf26dffa93066，归档标签解引用为b938a81860945acc527b8198d554fb67a83d78e4，标签对象也与本地一致；产品工作树干净。

本记录、TASKS／STATUS／索引和WORKSPACE基线在现有OPC feature本地单行功能提交保存并同步，不将其混入产品业务tree。两仓功能分支保留，待后续明确的OPC集成与清理流程；本轮未安排新开发轮次或实际账号发布。
