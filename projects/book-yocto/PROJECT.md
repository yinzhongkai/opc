# book-yocto

项目 ID：`book-yocto`

## 建项信息

- 建立日期：2026-09-12。
- 用户请求来源：2026-09-12 超级管理员会话中，建项用户请求"创建一个名叫 book-yocto 的项目"，说明 `yocto/` 目录是其撰写的草稿，希望通过本项目使草稿达到出版程度。
- 当前阶段：P3 逐章修订与逐章真实环境核验（chapter 2 进行中）。

## 目标与约束

- 目标：把本项目 `workspace/yocto/` 目录下已有的书稿草稿（task00–task21 共 22 个章节文件，Yocto / meta-tiger 主题）修订、完善至可出版的质量水平。
- 书稿位置变更：2026-09-13 经用户在 writer 会话确认，书稿从仓库根 `yocto/` 迁入 `projects/book-yocto/yocto/` 并纳入 Git（此前未跟踪）；2026-09-16 经用户在 project-manager 会话指示，再迁入 `projects/book-yocto/workspace/yocto/`。此前台账与成果中"仓库根 yocto/""projects/book-yocto/yocto/"的表述均指迁移前位置，章节内容与行号未变。
- 范围与非目标：范围包含技术内容的完整实测验证——实际创建四个开发态仓库并全链构建，回填全部 C-W/V 待验证项（[D-001](DECISIONS.md)，2026-09-18 确认候选 A）；从 chapter 2 起，每章均须对命令、配置、路径、构建/运行输出及依赖实际环境的技术结论执行逐章真实环境核验并保存证据（[D-010](DECISIONS.md)，2026-09-26）；术语与体例统一纳入范围（[D-005](DECISIONS.md)）。非目标：PDF/EPUB 等排版导出与出版渠道对接（[D-002](DECISIONS.md)）。
- 交付物与验收要求：交付物为 `workspace/yocto/` 的 Markdown 终稿（[D-002](DECISIONS.md)，2026-09-18）；验收标准为书稿逐章评审通过、验证按 D-001 与 D-010 执行完毕、学习维度以建项用户通读校验代替外部试读（[D-003](DECISIONS.md)，2026-09-18；[D-010](DECISIONS.md)，2026-09-26）。
- 时间、资源与其他约束：无硬性截稿日期，按"先闭环一章"节奏推进（[D-004](DECISIONS.md)，2026-09-18）；完整实测需要可用构建环境与四个开发态仓库的工程创建，责任人待用户安排。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 确认与记录责任

- 默认最终确认人：建项用户，以本文件记录的请求来源识别；用户可明确指定其他确认人。
- 协调记录维护人：`project-manager`（2026-09-12 按建员约定指定；首名且唯一项目经理，用户无其他安排。用户可改指定其他在册成员）。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：未配置项目经理时，跨成员计划变化由用户确认；配置后按项目运行协议明确负责的项目经理及记录维护人。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享岗位为准；记录整理不授予其他岗位职责。

## 交付约定

- 正式成果位于 `artifacts/`，软件源码或外部资料位置按实际需要另行确定。
- 用户保证当前仓库每次只有一个会话执行，前一会话完成或停止后再启动下一会话；框架不实现文件锁或自动调度。
- 每份文档默认维护一个当前版本，记录版本及批准依据；历史通过用户维护的 Git 追溯。
- 评审与批准要求在任务中明确。专业交叉评审由现有成员在原会话执行，不默认新建评审会话；用户保证受评版本可读取且不变，本轮全部评审者提交意见后作者再统一修订。采用根 [项目运行协议](../../PROJECT_PROTOCOL.md) 的评审、完成与状态规则。
- 默认在相关任务内记录轻量评审，需独立跟踪或正式报告时再拆分；具体已确认的验收要求不能自行降低。

## 成员配置记录

超级管理员建员或调整时记录实际日期、用户授权来源、涉及成员 ID、变更内容及配置路径；此处不重复维护成员当前字段。

- 2026-09-12：用户在本超级管理员会话确认按建议的 3 人方案创建成员。新建 `project-manager`（岗位 project-manager），配置路径 [members/project-manager.yaml](members/project-manager.yaml)；TEAM 登记成员索引并为该岗位补充知识 `book-production`；按建员约定指定其为协调记录维护人。记录人：框架超级管理员。
- 2026-09-12：同一授权下新建 `writer`（岗位 writer），配置路径 [members/writer.yaml](members/writer.yaml)；TEAM 登记成员索引并为该岗位补充知识 `book-production, reader-feedback, book-planning`。记录人：框架超级管理员。
- 2026-09-12：同一授权下新建 `reviewer`（岗位 reviewer），配置路径 [members/reviewer.yaml](members/reviewer.yaml)；TEAM 登记成员索引并为该岗位补充知识 `book-production, book-review, reader-feedback, technical-book-validation`。记录人：框架超级管理员。

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
