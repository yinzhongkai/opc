# 今天吃什么

项目 ID：`eating-everyday`

## 建项信息

- 建立日期：2026-10-15。
- 用户请求来源：本会话建项用户，2026-10-15 明确请求“我想创建一个叫今天吃什么的项目”。
- 当前阶段：依据 D-016 改为全手动候选首版，需求 A-001 v0.9 和架构草案 A-006 v0.3 已同步；产品仓库已按 T-009/D-017 初始化 README 并登记工作区，应用代码尚未实现。T-002 仍由 architect 负责且为 blocked，小程序测试入口和真机验证继续待落实。
- 建项记录人：框架超级管理员。
- 框架基线：已发布标签 `v0.0.4`，提交 `464ca52aafc6c73ff780a7120acd3bf63c9d31dd`。
- 项目长期分支：`project/eating-everyday`。
- 建项开发分支（历史）：`feature/what-to-eat-today/project-bootstrap`，已完成合并与清理。
- 建项合入前目标提交：`464ca52aafc6c73ff780a7120acd3bf63c9d31dd`。

## 目标与约束

- 目标：服务用户本人每天饭点不知道吃什么的场景；用户在微信小程序自行输入候选名称，由转盘随机选出一个，并在当前手机保存和加载常用菜单。依据 [D-016](DECISIONS.md#d-016首版改为全手动候选并移除定位搜索)。
- 范围与非目标：首版以 [A-001 v0.9](artifacts/product-requirements.md)、[A-007 v0.3](artifacts/product-decision-options.md)、D-005、D-011、D-014 和 D-016 为准；保留名称输入/删除、最多 24 项、独立等概率转盘、多份命名菜单及本机原样保存加载。首次为空，不自动预填；不获取定位，不提供距离、附近搜索或餐厅数据接入，不再要求林萃桥名单、数据源费用或保存许可验证。长期免费为使用偏好，不要求跨设备或清除数据后恢复。
- 交付物与验收要求：A-001 v0.9 含 11 项有效需求、11 条规则、27 项验收条件；旧编号保留撤销映射，不当作验证通过。需求与选项文档仍为 draft，具体范围确认不等于架构批准或实现验收。A-002/A-003 仅作历史核实资料，A-004 保留视觉参考；A-005 v0.3 归档旧数据验证，A-006 v0.3 为全手动系统设计草案，A-008 v0.1 列明当前账号与权限前置，均保持 draft。需求与架构资料分别提交，平台准入、本机留存/失败恢复、可见字符和 24 项可读性仍须实际验证。
- 时间、资源与其他约束：未设定。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 确认与记录责任

- 默认最终确认人：本会话建项用户，以本文件记录的 2026-10-15 请求来源识别；用户可明确指定其他确认人。
- 协调记录维护人：[project-manager](members/project-manager.yaml)；按首名项目经理的默认建员约定明确。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：由 `project-manager` 在已确认目标和授权范围内协调；超出范围或尚未确认的事项交用户决定。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享角色为准；记录整理不授予其他角色职责。

## 交付约定

- 需求、方案、计划、评审、验收等治理与协作成果位于 OPC 的 `artifacts/`；产品代码、可直接交付的业务内容及其中间文件位于 [WORKSPACE.yaml](WORKSPACE.yaml) 声明的外部 Git 仓库。依据 D-017，产品仓库为 `git@github.com:yinzhongkai/eating-everyday.git`，日常主线为 `main`，checkout 使用相对 OPC 根目录的同级路径 `../eating-everyday`，当前精确基线为 `8fa87fe4cb8f7f33aacf2842798a486150a6105e`，仅包含初始化 README。`artifacts/README.md` 同时索引两类成果，并为工作区成果记录仓库、路径和精确版本；后续操作继续以对应任务授权和实际基线核对为准。
- 用户保证当前仓库每次只有一个会话执行，前一会话完成或停止后再启动下一会话；框架不实现文件锁或自动调度。
- 每份文档默认维护一个当前版本，记录版本及批准依据；历史通过用户维护的 Git 追溯。
- 评审与批准要求在任务中明确。专业交叉评审由现有成员在原会话执行，不默认新建评审会话；用户保证受评版本可读取且不变，本轮全部评审者提交意见后作者再统一修订。采用根 [项目运行协议](../../PROJECT_PROTOCOL.md) 的评审、完成与状态规则。
- 默认在相关任务内记录轻量评审，需独立跟踪或正式报告时再拆分；具体已确认的验收要求不能自行降低。

## 项目 ID 调整记录

### 2026-10-15：简化为 eating

- 授权来源：本会话建项用户明确要求将过长的原 ID 简化为 `eating`。
- ID 与目录：原 ID `what-to-eat-today`、原目录 `projects/what-to-eat-today/` 改为 `eating`、`projects/eating/`。
- 项目长期分支：原 `project/what-to-eat-today` 改名为 `project/eating`；既有建项提交保留。
- 本轮开发分支：`feature/eating/project-id-rename`；合入前目标提交：`3e4a1631ee1381beb6c073ea86b5d952a4df873c`。
- 中文名称：今天吃什么。
- 记录人：框架超级管理员。

### 2026-10-15：调整为 eating-everyday

- 授权来源：本会话建项用户确认“对，就叫 eating-everyday，帮我改一下”。
- ID 与目录：原 ID `eating`、原目录 `projects/eating/` 改为 `eating-everyday`、`projects/eating-everyday/`。
- 项目长期分支：原 `project/eating` 改名为 `project/eating-everyday`；既有建项与改名提交保留。
- 本轮开发分支：`feature/eating-everyday/project-id-rename`；合入前目标提交：`4ad59e1af906b64dd0807c2119817e502dda7e83`。
- 中文名称：今天吃什么。
- 记录人：框架超级管理员。

## 成员配置记录

### 2026-10-15：创建首批三名成员

- 用户授权来源：本会话用户依次要求“帮我给 eating-everyday 项目创建一个项目经理”“再创建一个产品经理”“再创建一个架构师”。
- 新建成员配置：[project-manager](members/project-manager.yaml)、[product-manager](members/product-manager.yaml)、[architect](members/architect.yaml)，并登记至 [TEAM.yaml](TEAM.yaml)。
- 初始分工依据：采用对应共享角色的共同职责，具体 scope 以成员配置为准；未设置项目角色补充技能。
- 责任配置：首名项目经理 `project-manager` 按项目运行协议第 1.1 节承担协调记录维护；默认最终确认人仍为建项用户。
- 本轮开发分支：`feature/eating-everyday/initial-team`；合入前目标提交：`0ab3b8a1dfedb072f7b0935200d18d41ee0a824a`。
- 会话与任务事实：本次创建成员配置，成员会话待分别初始化；暂无已分配业务任务。
- 记录人：框架超级管理员。

## 工作区配置记录

### 2026-10-21：初始化独立产品仓库并登记同级工作区

- 授权来源：用户提供产品仓库 SSH 地址，确认 OPC 同级目录和相对路径，并要求直接添加 README、提交推送到 main 后配置 WORKSPACE；完整确认见 D-017、执行见 T-009。
- 原配置为 driver: none；本次按用户明确授权进行有界配置维护，改为 driver: git，repository、main、同级 checkout 和精确 revision 以上述声明为准。原 project-manager 身份不变，团队、角色和技能配置未变。
- 产品主线初始化提交：8fa87fe4cb8f7f33aacf2842798a486150a6105e；本地 HEAD、origin/main 和 GitHub refs/heads/main 已核对一致，仅含 README.md，无应用代码。
- OPC 交付主线为 project/eating-everyday，本轮来源为 feature/eating-everyday/workspace-setup；用户同日追加授权按 --ff-only 合入、推送 GitHub 并删除 feature，Git 收尾记录见 T-009。产品仓库 main 的 README 提交与推送已经实际完成。

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
