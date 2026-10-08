# book-yocto

项目 ID：`book-yocto`

- 框架基线：`v0.0.2`（提交 `267278b`）。2026-09-28 在发布标签纳入声明式工作区功能后重新升级；项目分支中等价的提前回灌提交已在重放时去重，Yocto 工程岗位与知识、分支治理策略及声明式工作区功能均由正式发布基线接管。首次升级前分支头由 `archive/2026-09-28/pre-v0.0.2-project-book-yocto` 保存，本次重放前分支头由 `archive/2026-09-28/project-book-yocto-before-v0.0.2-retag-upgrade` 保存。
- 分支迁移：2026-09-28 从原 `book-yocto` 历史收敛为 `project/book-yocto` 的干净项目基线；迁移前完整历史由 `archive/2026-09-28/book-yocto` 保存，项目线中原有的重复 Yocto 框架提交不再作为权威来源。

## 建项信息

- 建立日期：2026-09-12。
- 用户请求来源：2026-09-12 超级管理员会话中，建项用户请求"创建一个名叫 book-yocto 的项目"，说明 `yocto/` 目录是其撰写的草稿，希望通过本项目使草稿达到出版程度。
- 当前阶段：P3 逐章修订与逐章真实环境核验（chapter 2 已定稿，chapter 3 第 1 轮评审中）。

## 目标与约束

- 目标：把独立 `book-yocto` 仓库 `yocto/` 目录下已有的书稿草稿（task00–task21 共 22 个章节文件，Yocto / meta-tiger 主题）修订、完善至可出版的质量水平。
- 书稿位置变更：2026-09-13 经用户在 writer 会话确认，书稿从仓库根 `yocto/` 迁入 `projects/book-yocto/yocto/` 并纳入 Git（此前未跟踪）；2026-09-16 经用户在 project-manager 会话指示，再迁入 `projects/book-yocto/workspace/yocto/`。2026-09-28 经用户授权，将当前书稿和 Docker 环境以单一内容基线迁入独立仓库 `git@github.com:yinzhongkai/book-yocto.git`，书稿现位于其根目录下的 `yocto/`，初始基线为 `4bc485e`。旧修订历史、任务过程和核验证据仍由 OPC 管理；此前台账与成果中的旧路径均按迁移前位置理解。
- 范围与非目标：范围包含技术内容的完整实测验证——实际创建四个开发态仓库并全链构建，回填全部 C-W/V 待验证项（[D-001](DECISIONS.md)，2026-09-18 确认候选 A）；从 chapter 2 起，每章均须对命令、配置、路径、构建/运行输出及依赖实际环境的技术结论执行逐章真实环境核验并保存证据（[D-010](DECISIONS.md)，2026-09-26）；术语与体例统一纳入范围（[D-005](DECISIONS.md)）。非目标：PDF/EPUB 等排版导出与出版渠道对接（[D-002](DECISIONS.md)）。
- 交付物与验收要求：交付物为独立仓库 `yocto/` 下的 Markdown 终稿（[D-002](DECISIONS.md)，2026-09-18）；验收标准为书稿逐章评审通过、验证按 D-001 与 D-010 执行完毕、学习维度以建项用户通读校验代替外部试读（[D-003](DECISIONS.md)，2026-09-18；[D-010](DECISIONS.md)，2026-09-26）。
- 时间、资源与其他约束：无硬性截稿日期，按"先闭环一章"节奏推进（[D-004](DECISIONS.md)，2026-09-18）；完整实测需要可用构建环境与四个开发态仓库的工程创建。`yocto-engineer` 已于 2026-09-27 加入团队，但 P0.5 工程任务与仓库责任尚未正式指派。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 确认与记录责任

- 默认最终确认人：建项用户，以本文件记录的请求来源识别；用户可明确指定其他确认人。
- 协调记录维护人：`project-manager`（2026-09-12 按建员约定指定；首名且唯一项目经理，用户无其他安排。用户可改指定其他在册成员）。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：未配置项目经理时，跨成员计划变化由用户确认；配置后按项目运行协议明确负责的项目经理及记录维护人。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享岗位为准；记录整理不授予其他岗位职责。

## 交付约定

- 正式成果位于 `artifacts/`；书稿与配套环境位于 [WORKSPACE.yaml](WORKSPACE.yaml) 声明的独立 Git 仓库，修订脚本和核验证据位于 OPC 的 `artifacts/work-records/`。先在工作区提交并验证源码修改，再更新 OPC 中的精确 revision 与任务证据。
- 用户保证当前仓库每次只有一个会话执行，前一会话完成或停止后再启动下一会话；框架不实现文件锁或自动调度。
- 每份文档默认维护一个当前版本，记录版本及批准依据；历史通过用户维护的 Git 追溯。
- 评审与批准要求在任务中明确。专业交叉评审由现有成员在原会话执行，不默认新建评审会话；用户保证受评版本可读取且不变，本轮全部评审者提交意见后作者再统一修订。采用根 [项目运行协议](../../PROJECT_PROTOCOL.md) 的评审、完成与状态规则。
- 默认在相关任务内记录轻量评审，需独立跟踪或正式报告时再拆分；具体已确认的验收要求不能自行降低。

## 成员配置记录

超级管理员建员或调整时记录实际日期、用户授权来源、涉及成员 ID、变更内容及配置路径；此处不重复维护成员当前字段。

- 2026-09-12：用户在本超级管理员会话确认按建议的 3 人方案创建成员。新建 `project-manager`（岗位 project-manager），配置路径 [members/project-manager.yaml](members/project-manager.yaml)；TEAM 登记成员索引并为该岗位补充知识 `book-production`；按建员约定指定其为协调记录维护人。记录人：框架超级管理员。
- 2026-09-12：同一授权下新建 `writer`（岗位 writer），配置路径 [members/writer.yaml](members/writer.yaml)；TEAM 登记成员索引并为该岗位补充知识 `book-production, reader-feedback, book-planning`。记录人：框架超级管理员。
- 2026-09-12：同一授权下新建 `reviewer`（岗位 reviewer），配置路径 [members/reviewer.yaml](members/reviewer.yaml)；TEAM 登记成员索引并为该岗位补充知识 `book-production, book-review, reader-feedback, technical-book-validation`。记录人：框架超级管理员。
- 2026-09-27：用户在本超级管理员会话要求增加一名专门进行 Yocto 开发的成员，并在 OPC 框架中增加相应知识和岗位。新增公共知识 `yocto-engineering`、公共岗位 `yocto-engineer`，新建成员 `yocto-engineer`，配置路径 [members/yocto-engineer.yaml](members/yocto-engineer.yaml)；TEAM 登记成员索引并为该岗位补充知识 `technical-book-validation`。记录人：框架超级管理员。

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
