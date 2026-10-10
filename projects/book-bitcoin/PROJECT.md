# book-bitcoin

项目 ID：`book-bitcoin`

- 框架基线：`v0.0.3`（发布提交 `6f79b98e219e0bbf8f7de45dd35ec54c2a6f1f07`）。
- 项目长期分支：`project/book-bitcoin`；建项开发分支：`feature/book-bitcoin/project-bootstrap`。

## 建项信息

- 建立日期：2026-10-11。
- 用户请求来源：建项用户在本超级管理员会话指定管理目标为 book-bitcoin；2026-10-11 说明该项目类似 book-yocto，书稿已经完成，需要进一步审核达到可出版的标准，并明确选择“先创建空团队，稍后指定成员”。
- 当前阶段：出版前审核筹备，首批成员已配置，待明确书稿位置、受评版本与审核安排。
- 建项参考：读取本克隆 `origin/project/book-yocto` 的项目入口配置与成员分工（参考提交 `f3d8c2ff882e0542f86650829d99feb0dc523729`），采用独立项目记录与工作区声明的组织方式；本项目范围、验收要求与工作区另行确认。

## 目标与约束

- 目标：对已完成的 book-bitcoin 书稿进一步审核，并在必要修订与复核后达到可出版的质量水平。
- 现有书稿：用户说明已经完成；超级管理员尚未读取书稿，当前不作内容质量或出版就绪结论。
- 范围与非目标：已明确以现有书稿的出版前审核为工作方向；读者定位、具体审校范围、修订安排、排版导出与出版渠道工作待确认。
- 书稿位置与版本：待用户提供 book-bitcoin 书稿的本地路径或仓库信息及受评版本；[WORKSPACE.yaml](WORKSPACE.yaml) 暂使用 `none`，配置工作区后再执行依赖书稿的工作。
- 交付物与验收要求：质量目标为可出版；具体交付格式、审校覆盖、验收条件和出版方要求待用户或后续获授权成员明确。最终批准按本项目确认责任执行。
- 时间、资源与其他约束：未设定。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

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
