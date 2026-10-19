# 今天吃什么

项目 ID：`eating-everyday`

## 建项信息

- 建立日期：2026-10-15。
- 用户请求来源：本会话建项用户，2026-10-15 明确请求“我想创建一个叫今天吃什么的项目”。
- 当前阶段：需求整理完成，进入架构设计与数据源验证交接阶段；T-001、T-003、T-004 已完成，T-002 已安排 architect 负责，状态为 todo，待实际接收 H-002。依据 D-010、D-011。
- 建项记录人：框架超级管理员。
- 框架基线：已发布标签 `v0.0.4`，提交 `464ca52aafc6c73ff780a7120acd3bf63c9d31dd`。
- 项目长期分支：`project/eating-everyday`。
- 建项开发分支（历史）：`feature/what-to-eat-today/project-bootstrap`，已完成合并与清理。
- 建项合入前目标提交：`464ca52aafc6c73ff780a7120acd3bf63c9d31dd`。

## 目标与约束

- 目标：先服务用户本人每天饭点不知道吃什么的场景，以微信小程序根据定位获取附近具体餐厅，通过转盘随机抽取一家；依据 [D-001](DECISIONS.md#d-001个人用餐随机决策的产品方向)。
- 范围与非目标：首版功能、操作规则及费用偏好已整理为 [A-001 v0.7](artifacts/product-requirements.md)，依据 D-001 至 D-009；包含 1/3/5 公里搜索、地图最多 12 家与总候选最多 24 项、等概率转盘、多份菜单及当前手机原样保存加载。长期免费为优先偏好，不要求跨设备恢复；TBD-03 至 TBD-06 继续核实，不自行删减已确认功能。
- 交付物与验收要求：A-001 v0.7 包含 15 项需求、9 条操作规则、26 项验收条件及 V-01 至 V-04 验证清单，作为设计输入，整份版本仍为 draft。A-002 至 A-004 为已有核实和界面方向参考；数据源、真实覆盖、系统设计及产品实现尚未通过验证或批准。
- 时间、资源与其他约束：未设定。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 确认与记录责任

- 默认最终确认人：本会话建项用户，以本文件记录的 2026-10-15 请求来源识别；用户可明确指定其他确认人。
- 协调记录维护人：[project-manager](members/project-manager.yaml)；按首名项目经理的默认建员约定明确。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：由 `project-manager` 在已确认目标和授权范围内协调；超出范围或尚未确认的事项交用户决定。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享角色为准；记录整理不授予其他角色职责。

## 交付约定

- 需求、方案、计划、评审、验收等治理与协作成果位于 OPC 的 `artifacts/`；产品代码、可直接交付的业务内容及其中间文件位于 [WORKSPACE.yaml](WORKSPACE.yaml) 声明的成果物仓库。当前工作区驱动为 `none`，尚未声明成果物仓库，工作区版本基线不适用；后续工作区形式和位置待用户确认。`none` 不创建工作目录；独立产品源码优先使用外部 git。`artifacts/README.md` 同时索引两类成果，并为工作区成果记录仓库、路径和精确版本。声明本身不授权联网或修改外部仓库。
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

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
