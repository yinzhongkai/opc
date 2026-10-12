# book-yocto

项目 ID：`book-yocto`

- 框架基线：`v0.0.4`（远端发布提交 `464ca52aafc6c73ff780a7120acd3bf63c9d31dd`，注释标签对象 `21d803d34cbd2f83daef822afe8bb0ef13f1b0d2`）。本次升级来源、恢复点和执行口径见下文“框架升级记录”。
- 历史框架基线：2026-10-08 将项目提交 rebase 到 `v0.0.3`（`6f79b98e219e0bbf8f7de45dd35ec54c2a6f1f07`）正式标签之上，采用角色/技能术语、项目历史工作流、会话消息边界和项目升级 rebase 规则；底层配置字段继续使用 `knowledge`。此前 `v0.0.2` 升级及归档记录继续有效。
- 历史提前采用记录（公共能力已纳入 v0.0.4）：2026-10-09 为处理 H-001，从框架集成分支 `opc-v0.0.4-dev` 提前采用 `multi-git` 独立成果物声明（原提交 `5a4f6137048ef195f473ddd05caef751e8b508f9`，集成 merge `23bdd9b442e89e47e49515b58ea8be54670b62cb`）及 SSH 主机 checkout 扩展（原提交 `60a30bb889b1b7b42b1644d685738fb1e444a62f`，集成 merge `921a11952946f21f1fbbb5392a5f44a61040192e`）；本项目采用提交为 `c62d87d4ebccae8200f413fba838cad4af7131d8`、`71af3dec61165a1123d81b5ac6760a9796ab8a63`。采用原因为书稿与 `tiger` 服务器上的 `meta-tiger` 是两个独立 Git 成果物，现行 `v0.0.3` 单仓库声明无法同时锁定；兼容风险是该能力尚未正式发布、集成线与项目提交尚未推送，后续升级正式版本时必须核对等价提交并去重。60 项框架测试、框架结构校验和差异检查均通过。
- 分支迁移：2026-09-28 从原 `book-yocto` 历史收敛为 `project/book-yocto` 的干净项目基线；迁移前完整历史由 `archive/2026-09-28/book-yocto` 保存，项目线中原有的重复 Yocto 框架提交不再作为权威来源。
- 历史提前采用记录（公共角色与技能已纳入 v0.0.4）：2026-10-11 按用户新增 QEMU 工程师的请求，从 `opc-v0.0.4-dev` 采用公共角色 `qemu-engineer` 与技能 `qemu-engineering`。原提交 `ea967cce2402517d7ee6027c1506ddb269710cf7` 已以 `--no-ff` 合入框架集成线（merge `846270fe2d67a87a01b8ad417e9c27da02a79e01`），项目采用提交为 `cb38bbf459b59df30d46c62c69ec4481ddf7d244`；采用原因是虚拟板和设备模型正确性需要独立专业分工。该能力尚未正式发布，本轮集成及项目提交仅保存在本机；后续升级正式框架版本时核对等价提交并去重。框架合并前后结构校验、60 项测试及差异检查通过，公共提交仅含角色、技能和索引。当前 T-016 开发分支继续保留，建员属于其工程启动准备，不另开下一轮项目开发。

## 建项信息

- 建立日期：2026-09-12。
- 用户请求来源：2026-09-12 超级管理员会话中，建项用户请求"创建一个名叫 book-yocto 的项目"，说明 `yocto/` 目录是其撰写的草稿，希望通过本项目使草稿达到出版程度。
- 当前阶段：P3 逐章修订与逐章真实环境核验（chapter 1–3 已定稿；A-005 v0.2 的 QEMU 与 Yocto 针对性复核均 pass，2026-10-11 用户明确批准，见 D-014；设计批准回写已提交并经 project-manager 核验，T-016 当前衔接 writer 的 chapter 4 蓝图。chapter 4 只做最小可启动 machine，功能随章节补充；工程实现与构建运行尚未开始）。

## 目标与约束

- 目标：把独立 `book-yocto` 仓库 `yocto/` 目录下已有的书稿草稿（task00–task21 共 22 个章节文件，Yocto / meta-tiger 主题）修订、完善至可出版的质量水平。
- 书稿位置变更：2026-09-13 经用户在 writer 会话确认，书稿从仓库根 `yocto/` 迁入 `projects/book-yocto/yocto/` 并纳入 Git（此前未跟踪）；2026-09-16 经用户在 project-manager 会话指示，再迁入 `projects/book-yocto/workspace/yocto/`。2026-09-28 经用户授权，将当前书稿和 Docker 环境以单一内容基线迁入独立仓库 `git@github.com:yinzhongkai/book-yocto.git`，书稿现位于其根目录下的 `yocto/`，初始基线为 `4bc485e`。旧修订历史、任务过程和核验证据仍由 OPC 管理；此前台账与成果中的旧路径均按迁移前位置理解。
- 范围与非目标：范围包含技术内容的完整实测验证——实现 tiger 平台代码并全链构建，回填全部 C-W/V 待验证项（[D-001](DECISIONS.md)，2026-09-18）；按 [D-013](DECISIONS.md)（2026-10-09）不再创建四个长期 fork，而以 Poky 配方锁定的上游源码为基线，在临时 devtool/BitBake workspace 中开发，最终 patch、`.bbappend`、`SRC_URI` 和配置进入 `meta-tiger`，并在清理 workspace 后从干净状态复验。从 chapter 2 起，每章均须对命令、配置、路径、构建/运行输出及依赖实际环境的技术结论执行逐章真实环境核验并保存证据（[D-010](DECISIONS.md)，2026-09-26）；术语与体例统一纳入范围（[D-005](DECISIONS.md)）。非目标：PDF/EPUB 等排版导出与出版渠道对接（[D-002](DECISIONS.md)）。
- 交付物与验收要求：主交付物为独立书稿仓库 `yocto/` 下的 Markdown 终稿（[D-002](DECISIONS.md)，2026-09-18）；`meta-tiger` 保存支撑书稿复现的长期工程 patch 与集成元数据，不以四个上游源码 fork 作为交付物。验收标准为书稿逐章评审通过、验证按 D-001/D-010/D-013 执行完毕、学习维度以建项用户通读校验代替外部试读（[D-003](DECISIONS.md)，2026-09-18）。
- 时间、资源与其他约束：无硬性截稿日期，按"先闭环一章"节奏推进（[D-004](DECISIONS.md)，2026-09-18）；完整实测需要可用构建环境、配方对应的上游源码版本和可写的 `meta-tiger` 工程工作区。`yocto-engineer` 已于 2026-09-27 加入团队；2026-10-09 已按 H-001 将 `tiger:/home/oops/workspace/meta-tiger` 纳入 WORKSPACE 并锁定完整基线，实际工作仍须逐次核对远端工作树、分支与 revision，声明本身不授予写入、切换或推送权限。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 确认与记录责任

- 默认最终确认人：建项用户，以本文件记录的请求来源识别；用户可明确指定其他确认人。
- 协调记录维护人：`project-manager`（2026-09-12 按建员约定指定；首名且唯一项目经理，用户无其他安排。用户可改指定其他在册成员）。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：未配置项目经理时，跨成员计划变化由用户确认；配置后按项目运行协议明确负责的项目经理及记录维护人。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享角色为准；记录整理不授予其他角色职责。

## 交付约定

- 正式治理成果位于 `artifacts/`；书稿位于 [WORKSPACE.yaml](WORKSPACE.yaml) 声明的 `book-manuscript` 仓库，修订脚本和核验证据位于 OPC 的 `artifacts/work-records/`。D-013 新增的长期工程资产落在同一声明中的远程 `meta-tiger` 仓库；各成果物仓库均先提交并验证源码修改，再更新 OPC 中各自的精确 revision、任务和索引证据。
- 用户保证当前仓库每次只有一个会话执行，前一会话完成或停止后再启动下一会话；框架不实现文件锁或自动调度。
- 每份文档默认维护一个当前版本，记录版本及批准依据；历史通过用户维护的 Git 追溯。
- 评审与批准要求在任务中明确。专业交叉评审由现有成员在原会话执行，不默认新建评审会话；用户保证受评版本可读取且不变，本轮全部评审者提交意见后作者再统一修订。采用根 [项目运行协议](../../PROJECT_PROTOCOL.md) 的评审、完成与状态规则。
- 默认在相关任务内记录轻量评审，需独立跟踪或正式报告时再拆分；具体已确认的验收要求不能自行降低。

## 成员配置记录

超级管理员建员或调整时记录实际日期、用户授权来源、涉及成员 ID、变更内容及配置路径；此处不重复维护成员当前字段。

- 2026-09-12：用户在本超级管理员会话确认按建议的 3 人方案创建成员。新建 `project-manager`（角色 project-manager），配置路径 [members/project-manager.yaml](members/project-manager.yaml)；TEAM 登记成员索引并为该角色补充技能 `book-production`；按建员约定指定其为协调记录维护人。记录人：框架超级管理员。
- 2026-09-12：同一授权下新建 `writer`（角色 writer），配置路径 [members/writer.yaml](members/writer.yaml)；TEAM 登记成员索引并为该角色补充技能 `book-production, reader-feedback, book-planning`。记录人：框架超级管理员。
- 2026-09-12：同一授权下新建 `reviewer`（角色 reviewer），配置路径 [members/reviewer.yaml](members/reviewer.yaml)；TEAM 登记成员索引并为该角色补充技能 `book-production, book-review, reader-feedback, technical-book-validation`。记录人：框架超级管理员。
- 2026-09-27：用户在本超级管理员会话要求增加一名专门进行 Yocto 开发的成员，并在 OPC 框架中增加相应技能和角色。新增公共技能 `yocto-engineering`、公共角色 `yocto-engineer`，新建成员 `yocto-engineer`，配置路径 [members/yocto-engineer.yaml](members/yocto-engineer.yaml)；TEAM 登记成员索引并为该角色补充技能 `technical-book-validation`。记录人：框架超级管理员。
- 2026-10-11：用户在本超级管理员会话明确要求新增 QEMU 工程师并创建会话、完成只读初始化，主责 tiger 模型代码与正确性，现有 Yocto 工程师继续承担 patch 集成、BitBake 构建和系统验证。新建 `qemu-engineer`（角色 qemu-engineer），配置路径 [members/qemu-engineer.yaml](members/qemu-engineer.yaml)；TEAM 登记成员并补充 `technical-book-validation`，有效技能为 `software-engineering, qemu-engineering, technical-book-validation`。同步细化 [members/yocto-engineer.yaml](members/yocto-engineer.yaml) 的 QEMU 集成与模型问题交接边界，角色和技能集合不变。遵循 D-013，上游版本由 Poky 配方锁定，长期 patch 进入既有 `meta-tiger`；WORKSPACE 及成果物基线不变。T-016 的具体任务分工交由项目经理后续调整，见 [H-002](HANDOFFS.md)；本次配置不代表业务开发已启动。记录人：框架超级管理员。

- 2026-10-11：用户在 project-manager 会话明确要求“按你的建议新增加成员吧”，授权新增负责 tiger 平台设计的系统架构师。本轮按超级管理员入口处理有界成员配置维护，原 project-manager 成员身份不变。复用已有 active 角色 `architect`，新建首名该角色成员 `architect`，配置路径 [members/architect.yaml](members/architect.yaml)；TEAM 登记并补充 `qemu-engineering, yocto-engineering, technical-book-validation`，有效技能为 `system-design, qemu-engineering, yocto-engineering, technical-book-validation`。主责跨 QEMU/TF-A/U-Boot/Linux 的平台设计、接口与启动链契约及设计风险；writer 保持章节教学设计和写作职责，两个工程成员保持模型开发与集成验证职责。公共角色、技能正文和 WORKSPACE 基线均未修改；建员属于 T-016 启动准备，不开始下一轮开发。本次完成成员配置，不创建或唤醒平台会话、不代写初始化或专业设计成果；任务和计划衔接见 [H-003](HANDOFFS.md)。记录人：框架超级管理员（本轮用户明确授权的有界配置维护）。

## 框架升级记录

- 2026-10-12：用户在超级管理员会话要求“先同步下最新代码，然后将项目升级到 v0.0.4 框架”。OPC、独立书稿及 SSH 主机 `tiger` 上的 `meta-tiger` 已刷新远端引用；升级前两个本机仓库及远程工程仓库工作树均干净。书稿保持 `feature/t-016-chapter4-revision`、`main@f444790f352b46f699fd3d46198f8b9b9f089a62`，工程保持 `master@0e3da48b34ef772532535a730a21878cc30ccf67`；本次框架升级不推进成果物版本。
- 发布来源：远端 `origin` 的 `refs/tags/v0.0.4` 指向上述发布提交，第一父提交为 `v0.0.3`，发布文件树与远端 `main` 一致。本机此前同名注释标签对象 `f529a7a3b0249a1d0abe658f5908b56910a66e65` 指向 `869f04626d0110185c508e9cc46759375df24f9e`，文件树与远端发布相同；旧对象保存在 `refs/tags/archive/2026-10-12/v0.0.4-before-sync`，本机同名标签保持原指向。升级与祖先检查使用单独获取的远端标签引用 `refs/upgrades/book-yocto/v0.0.4-remote-2026-10-12` 及上述精确发布提交；本机和远端发布标签均不改写。
- 恢复点与旧远程头：`project/book-yocto@f3d8c2ff882e0542f86650829d99feb0dc523729` 保存为 `archive/2026-10-12/book-yocto-before-v0.0.4`；活动分支 `feature/book-yocto/t-016-chapter4-revision@b262ea9c0935d662a42ec0e4ded0c8cb4d24fa58` 保存为 `archive/2026-10-12/book-yocto-t-016-before-v0.0.4`。项目独有提交使用 rebase 迁移；T-016 活动提交继续基于新项目基线，不合入长期分支，不改变任务状态、成员分工或成果批准。
- 重放与流程口径：已提前采用的 `multi-git`、SSH checkout 和 QEMU 角色/技能属于 `v0.0.4` 正式框架，活动分支重放时按等价内容去重；此前采用记录及旧 SHA 保留为历史依据。后续 feature/bugfix 按当前根 [分支策略](../../BRANCH_STRATEGY.md) 使用 `--ff-only` 合入并记录合入前目标提交与合入后的最终提交；框架发布继续使用 `--no-ff` 发布合并。已完成任务的旧合并证据不追改。
- 记录人：框架超级管理员。本次为用户授权的框架基线迁移，不构成专业业务任务安排。

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
