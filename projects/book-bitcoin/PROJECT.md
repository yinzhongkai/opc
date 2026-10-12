# book-bitcoin

项目 ID：`book-bitcoin`

- 框架基线：`v0.0.4`（发布提交 `464ca52aafc6c73ff780a7120acd3bf63c9d31dd`）。
- 项目长期分支：`project/book-bitcoin`；建项开发分支：`feature/book-bitcoin/project-bootstrap`。

## 建项信息

- 建立日期：2026-10-11。
- 用户请求来源：建项用户在本超级管理员会话指定管理目标为 book-bitcoin；2026-10-11 说明该项目类似 book-yocto，书稿已经完成，需要进一步审核达到可出版的标准，并明确选择“先创建空团队，稍后指定成员”。
- 当前阶段：出版前审核筹备，T-001 盘点与计划已合入项目分支；T-002 的标准和配套边界已获用户确认，下游可开始领域核查与资料整理。
- 建项参考：读取本克隆 `origin/project/book-yocto` 的项目入口配置与成员分工（参考提交 `f3d8c2ff882e0542f86650829d99feb0dc523729`），采用独立项目记录与工作区声明的组织方式；本项目范围、验收要求与工作区另行确认。

## 目标与约束

- 目标：对已完成的 book-bitcoin 书稿进一步审核，并在必要修订与复核后达到可出版的质量水平。
- 现有书稿：用户说明已经完成；2026-10-11 超级管理员核对了输入目录、文件数量及内容指纹，尚未进行内容审校，当前不作内容质量或出版就绪结论。
- 范围与非目标：已明确以现有书稿的出版前审核为工作方向；按 [D-004](DECISIONS.md) 沿用书稿现有读者定位（有 1–3 年经验、能读 C/C++ 并使用 Linux 的开发者）。全书盘点与审核 / 修订安排见 [A-001](artifacts/book/content-inventory.md)、[A-002](artifacts/book/review-plan.md)；按 D-005 审核全 13 篇，配套在现有产品仓库提供，附录包含完整参考解答，排版导出与出版渠道待确认。
- 书稿位置与版本：依据 [D-001 至 D-003](DECISIONS.md)，原稿从本机输入目录 `C:\Users\YinZh\Downloads\books-bitcoin\content\bitcoin` 复制到独立仓库 `git@github.com:yinzhongkai/book-bitcoin.git` 的 `bitcoin/`；默认本机检出为 `D:\book-bitcoin`，日常分支为 `main`，当前提交为 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`。[WORKSPACE.yaml](WORKSPACE.yaml) 已声明 `git` 工作区与精确版本，该版本已推送并核对远端 `main`。
- 交付物与验收要求：质量目标为可出版；采用已确认的 [A-003 1.0](artifacts/book/review-criteria.md) S-01–S-15 和 D-005 配套边界。配套开发与运行由用户安排执行者，T-006 运行前落实具体人选与环境；成书格式和出版方要求仍待明确，最终书稿批准按本项目确认责任执行。
- 时间、资源与其他约束：未设定。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 工作区来源登记

- 2026-10-11 首次来源登记：建项用户在本超级管理员会话提供上述书稿目录和 Git 仓库地址；首次登记仅完成只读核对与项目记录登记。
- 输入目录核对：包含 13 个 Markdown 文件，编号 task00–task12，总计 649322 字节。下载目录 `books-bitcoin` 无 Git 元数据；导入前的原稿通过下述指纹定位，导入后采用 WORKSPACE 中的精确 Git 提交。
- 输入指纹：`f2e40d8709796b61912171b10401ae07a6892c90c83cd7fa1c3b81f0c7679087`（SHA-256）。按完整路径排序，每行以相对路径、TAB、文件 SHA-256 小写组成，行尾 LF，整份清单采用 UTF-8 后计算摘要；仅用于核对本次输入，不代替 Git revision 或审校结论。
- 初次仓库核对：只读查询 `git ls-remote --symref git@github.com:yinzhongkai/book-bitcoin.git HEAD refs/heads/*` 成功，未返回 HEAD 或分支；当时本机默认候选检出 `D:\book-bitcoin` 尚不存在。后续克隆确认目标为空仓库。
- 早期候选：原先提出将原稿放在 `content/bitcoin/`；后续用户要求目录结构参考 book-yocto，按 [D-001](DECISIONS.md) 初始采用根目录 `bitcoin/` 和 `docker/`，保留原下载目录；现按 [D-002](DECISIONS.md) 移除 `docker/` 和 `.gitattributes`，保留书稿与导航。
- 当前恢复点：独立仓库当前 `main` 及远端 `main` 均为整理后的提交 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`，OPC 的 WORKSPACE 锁定该版本。原下载目录在初始导入时保留，2026-10-12 本轮核对时该路径已不存在；现有 13 篇书稿与 Git 内容、A-001 盘点摘要及上述原稿指纹一致。早期初始与目录清理提交仅保留于本机 Git 恢复记录，不属于当前远端分支历史。

## 工作区配置记录

### 初始导入

- 2026-10-11 用户授权：建项用户在本会话明确要求“克隆到 D:\book-bitcoin，目录结构参考 book-yocto”，承接此前建立本地 main 初始提交和配置工作区的方案，确认结果见 [D-001](DECISIONS.md)。记录人：框架超级管理员。
- 实施范围：克隆用户指定仓库，复制 13 篇原稿至 `bitcoin/`，从原输入包的 `resource/dockerfile` 复制环境定义至 `docker/dockerfile`，新增 `README.md`、`bitcoin/index.md` 和保持原稿字节的 `.gitattributes`；未复制输入包中的其他项目资料。
- 内容基线：本地 `main` 初始提交 `b3e147f539c3ba298e6be6fcaf0d16c3e79b7bdd`；WORKSPACE 从 `none` 改为 `git`，默认检出声明为可移植路径 `../book-bitcoin`。工作区输入位置见 [成果索引](artifacts/README.md)。
- 验证证据：13 篇原稿及原始 Dockerfile 的源文件、本地副本与 Git 暂存内容逐字节一致；共 17 个预期文件，新增导航中的 15 处本地链接有效；环境定义未执行构建或书稿实验验证。
- OPC 提交方式：本轮开发分支为 `feature/book-bitcoin/workspace-bootstrap`，按本会话已采用的线性提交偏好快进合入 `project/book-bitcoin`，不增加 merge commit。
- 初始导入完成时状态：原稿基线已就绪，书稿质量、实验结果及出版批准仍待获授权成员按任务处理；当时只建立本地版本，尚未推送。

### 按用户要求精简仓库

- 2026-10-11 用户授权：建项用户在本会话要求“docker目录不需要啊”，随后补充“gitattributes 也直接去掉吧”，确认结果见 [D-002](DECISIONS.md)。记录人：框架超级管理员。
- 实施范围：从独立仓库删除 `docker/dockerfile` 与 `.gitattributes`，移除空 `docker/` 目录，更新仓库说明；原稿、原下载目录与文件索引保留。
- 目录清理完成时基线：本地 `main` 提交 `2f47631c7f41d3585bf25242b037cb92e988d0f1`，当时 WORKSPACE 和[工作区输入索引](artifacts/README.md)同步引用该精确版本；后续版本整理见下节。
- 验证证据：13 篇原稿的源文件、本地副本与 Git 内容逐字节一致；剩余 15 个预期文件，导航中的 14 处本地链接有效；两个删除目标已不存在。
- 提交方式与状态：成果物仓库使用 `bugfix/remove-docker`，OPC 使用 `bugfix/book-bitcoin/remove-docker`，分别快进更新 `main` 和 `project/book-bitcoin`，不增加 merge commit；本轮仅保存本地版本，尚未推送，不构成内容审校或出版批准。

### 远端同步与版本核对

- 2026-10-11 用户授权：建项用户在本会话要求“同步到远程仓库吧”，范围为书稿仓库 `main` 与 OPC 的 `project/book-bitcoin`；确认记录见 [D-003](DECISIONS.md)。记录人：框架超级管理员。
- 推送前事实：现有书稿 `main` 已经通过本地 reset/amend 整理为 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`；本轮未执行该历史整理。与原声明版本 `2f47631c7f41d3585bf25242b037cb92e988d0f1` 的文件树完全一致，均为 `2f173ea03ae48e07accc2542fb60c7aaa82b574f`；两个目标远端分支当时均不存在。
- 书稿同步证据：已将现有 `main` 普通推送至 `git@github.com:yinzhongkai/book-bitcoin.git`，重新读取远端 `refs/heads/main` 确认提交为 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`，并建立本地跟踪关系。
- 配置与项目记录同步：WORKSPACE、状态摘要及当前输入索引改为引用现有精确提交；在 `bugfix/book-bitcoin/remote-sync` 更新本轮记录，按本会话线性提交偏好快进更新 `project/book-bitcoin`，再按用户授权推送该项目分支。无成果物内容修改，不创建成果物开发分支；不推送框架主线或其他项目分支。

## 框架升级记录

- 2026-10-12 用户授权：建项用户在本超级管理员会话要求“先同步下最新代码，然后将项目升级到v0.0.4框架”，见 [D-006](DECISIONS.md)。记录人：框架超级管理员。
- 同步与基线：先获取 OPC 与书稿仓库的最新远程引用，核对两仓库工作树干净；OPC `main` 已为 `v0.0.4` 发布提交，书稿本地 / 远端 `main` 与 WORKSPACE 均为 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`。
- 升级前项目远程头：`a16c646edafcf8924f3347671c032104524d3baa`；原框架基线为 `v0.0.3`（`6f79b98e219e0bbf8f7de45dd35ec54c2a6f1f07`）。恢复标签 `archive/book-bitcoin-pre-v0.0.4-20261012` 保留旧项目线及审批记录所引用的精确提交，随新项目线一同推送。
- 迁移方式：按根 [分支策略](../../BRANCH_STRATEGY.md) 第 6 节，将 10 笔项目独有提交 rebase 到 `v0.0.4`；迁移无冲突，逐笔核对项目目录树完全一致。基线更新前的新项目头为 `491ce2cb9a5775c36396b9ee3009d941a41f0e03`；本次以普通基线提交收口，不新增升级 merge commit。没有实际存在的活动 feature / bugfix 分支；TASKS 中尚未创建的计划分支及任务状态保持原记录。
- 配置与兼容性：保留四名成员、角色、scope、有效技能和 WORKSPACE 配置；版本 3 TEAM 与版本 1 git 工作区兼容新版。本项目后续开发使用新版 `--ff-only` 流程；历史任务证据按 [迁移说明](../../MIGRATIONS.md) 保留，开发任务启动时再登记实际 OPC 合入前目标提交。
- 验证证据：`v0.0.4` 发布版与升级项目分别通过全部 60 项回归测试（`python -B -m unittest discover -s scripts -p 'test_*.py'`）；升级项目通过 `python -B scripts/validate_framework.py`，核对 22 个角色、37 份技能、1 个实际项目、1 套模板和 449 处本地链接。目录边界、双仓库版本与 `git diff --check` 均通过；仅更新本项目四份治理记录，A-001 至 A-003 正文、TASKS、成员及 WORKSPACE 内容保持不变。13 篇书稿共 649322 字节，与 Git 基线、已登记原稿摘要和输入指纹一致；原下载目录本轮不可读取，不作新的源目录逐字节核对声明。
- 推送保护：推送前重新获取远程引用，确认项目旧远程头及不可变发布标签未变化；仅以显式旧 SHA 的 `--force-with-lease` 原子更新本项目分支与恢复标签，不更新框架主线或书稿历史。
- 后续会话：成员开始下一项工作前按会话协议刷新 `v0.0.4` 规则、项目基线和关联资料；其他克隆须显式切换到新项目历史，不把旧项目头再次合回。本轮不触发成员会话或分配业务任务。

## 工作方法

- 采用框架已有的[图书生产与统稿](../../knowledge/book-production.md)、[图书审校与一致性检查](../../knowledge/book-review.md)及[图书成果模板入口](../../templates/book/README.md)作为工作参考；项目补充技能以 [TEAM.yaml](TEAM.yaml) 为准，与角色基础技能合并加载。
- 后续审校从现有书稿评估开始，逐章评审、作者统一修订、相关成员复核与全书统稿按已确认任务推进。包含代码、命令或实验时，再按实际内容确认[技术书验证](../../knowledge/technical-book-validation.md)的适用范围。
- 读者反馈方式与成书输出要求待确认，不将书稿完成、评审完成或结构校验通过记为出版批准。

## 确认与记录责任

- 默认最终确认人：建项用户，以本文件记录的请求来源识别；用户可明确指定其他确认人。
- 协调记录维护人：`project-manager`（2026-10-11 按建员约定指定；首名且唯一项目经理，用户无其他安排。用户可改指定其他在册成员）。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：未配置项目经理时，跨成员计划变化由用户确认；配置后按项目运行协议明确负责的项目经理及记录维护人。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享角色为准；记录整理不授予其他角色职责。

## 交付约定

- 需求、方案、计划、评审、验收等治理与协作成果位于 OPC 的 `artifacts/`；产品代码、可直接交付的业务内容及其中间文件位于 [WORKSPACE.yaml](WORKSPACE.yaml) 声明的成果物仓库。默认 `none` 不创建工作目录；独立产品源码优先使用外部 git。`artifacts/README.md` 同时索引两类成果，并为工作区成果记录仓库、路径和精确版本。声明本身不授权联网或修改外部仓库。
- 用户保证当前仓库每次只有一个会话执行，前一会话完成或停止后再启动下一会话；框架不实现文件锁或自动调度。
- 每份文档默认维护一个当前版本，记录版本及批准依据；历史通过用户维护的 Git 追溯。
- 评审与批准要求在任务中明确。专业交叉评审由现有成员在原会话执行，不默认新建评审会话；用户保证受评版本可读取且不变，本轮全部评审者提交意见后作者再统一修订。采用根 [项目运行协议](../../PROJECT_PROTOCOL.md) 的评审、完成与状态规则。
- 默认在相关任务内记录轻量评审，需独立跟踪或正式报告时再拆分；具体已确认的验收要求不能自行降低。

## 成员配置记录

- 2026-10-11：依据建项用户在本会话明确选择“先创建空团队，稍后指定成员”，建立版本 3 空团队（`members: []`，`roleKnowledge: {}`），配置路径 [TEAM.yaml](TEAM.yaml)；协调记录维护人待指定。未创建成员文档或成员会话。记录人：框架超级管理员。
- 2026-10-11：建项用户在本超级管理员会话确认此前四人建议，要求“先创建这4个成员”。在 `feature/book-bitcoin/team-bootstrap` 配置成员并登记 TEAM；仅创建成员配置与角色技能组合，成员会话和具体业务任务尚未创建。记录人：框架超级管理员。
- 2026-10-11：同一授权下新建 `project-manager`（角色 project-manager），配置路径 [members/project-manager.yaml](members/project-manager.yaml)，补充技能 `book-production`；按首名项目经理默认约定指定其为协调记录维护人。记录人：框架超级管理员。
- 2026-10-11：同一授权下新建 `writer`（角色 writer），配置路径 [members/writer.yaml](members/writer.yaml)，补充技能 `book-production, book-review, reader-feedback`；分工为已有书稿修订与全书统稿。记录人：框架超级管理员。
- 2026-10-11：同一授权下新建 `reviewer`（角色 reviewer），配置路径 [members/reviewer.yaml](members/reviewer.yaml)，补充技能 `book-production, book-review, reader-feedback, technical-book-validation`；分工为独立审校与修订复核。记录人：框架超级管理员。
- 2026-10-11：同一授权下新建 `researcher`（角色 researcher），配置路径 [members/researcher.yaml](members/researcher.yaml)，补充技能 `book-production, book-review, technical-book-validation`；分工为比特币领域事实、协议及来源核查，具体正文以成员文件为准。记录人：框架超级管理员。

后续超级管理员建员或调整时记录实际日期、用户授权来源、涉及成员 ID、变更内容及配置路径；此处不重复维护成员当前字段。

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
