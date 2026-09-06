# 太空律动：项目入口

项目 ID：`space-rhythm`。显示名称：太空律动。

遵循根目录 [AGENTS.md](../../AGENTS.md)。项目基本信息在 [PROJECT.md](PROJECT.md)，成员配置在 [TEAM.yaml](TEAM.yaml)，任务和状态在 [TASKS.md](TASKS.md) 与 [STATUS.md](STATUS.md)。

共享岗位位于 `../../roles/<role-id>.md`，知识位于 `../../knowledge/<knowledge-id>.md`。有效知识按岗位基础 `knowledge`、项目 `roleKnowledge` 的顺序合并去重；当前无项目补充知识，成员及基础知识如下：

| 成员 ID | 岗位 | 基础知识 |
|---|---|---|
| organization-manager-01 | organization-manager（组织管理员） | team-management |
| project-manager-01 | project-manager（项目经理） | project-management |
| product-manager-01 | product-manager（产品经理） | requirements-analysis |
| architect-01 | architect（系统架构师） | system-design |

用户声明成员身份后，须执行根目录 `SESSION_PROTOCOL.md`。2026-09-06 核对时该文件仍缺失，已记入状态与待办；补齐前不能宣称完成标准成员初始化。维护项目配置不等于绑定成员身份。

当前交付文档统一见 [artifacts/](artifacts/README.md)：`SR-A001` PRD、`SR-A002` TRD，以及保留独立算法实现逻辑的 `SR-A003` 技术说明。用户明确要求不保留中间草稿、过程报告和备份；后续直接维护各文档当前版本，有独立技术价值的内容可单独形成成果。PRD/TRD 的范围评审状态仍按文档及项目决定执行。

当前配置、决定和资料纳入规则见 [PROJECT.md](PROJECT.md) 与 [DECISIONS.md](DECISIONS.md)。修改共享文件前重新读取，保留无关更改。

## 产品经理会话启动消息

其他成员的启动消息也见本文件相应章节。

以下为成员启动消息，待上述初始化协议补齐后使用；本次增员仅更新配置，尚未创建或初始化成员会话。

```text
项目：space-rhythm
成员：product-manager-01
请执行会话初始化，按根目录 SESSION_PROTOCOL.md 读取成员记录、岗位基础知识、项目补充知识和项目资料，汇报岗位、分工、实际读取的文件与当前任务。
加载项目成果 SR-A001 及其任务/决定状态；此前撤销的业务任务不自动恢复，Draft 不视为已批准范围。
```

## 系统架构师会话启动消息

待上述初始化协议补齐后使用。本次新增成员配置，尚未创建或初始化架构师会话，也未登记已接收的业务任务。

```text
项目：space-rhythm
成员：architect-01
请执行会话初始化，按根目录 SESSION_PROTOCOL.md 读取成员记录、architect 岗位、基础知识 system-design、项目补充知识和项目资料，汇报岗位、分工、实际读取的文件与当前任务。
职责是将产品需求转换为可实现、可验证的技术需求，建立需求追溯，定义模块、接口、数据模型、质量要求及验证条件，并与 product-manager-01 澄清需求歧义。
读取 SR-A001 及其任务和决定状态，区分 Draft、已确认需求与技术假设；此前撤销的任务不自动恢复。
```

## 项目经理会话启动消息

待上述初始化协议补齐后使用。本次仅增加成员配置及运营分工，尚未创建或初始化项目经理会话。

```text
项目：space-rhythm
成员：project-manager-01
请执行会话初始化，按根目录 SESSION_PROTOCOL.md 读取成员记录、project-manager 岗位、基础知识 project-management、项目补充知识和项目资料，汇报岗位、分工、实际读取的文件与当前任务。
负责项目日常运营：计划与里程碑、进度、任务安排、跨成员协调、资源需求、阻塞与风险跟踪、项目状态和阶段总结；成员与知识配置需求提交 organization-manager-01。
核对 TASKS.md、STATUS.md、DECISIONS.md、HANDOFFS.md 及 SR-A001 的状态，依据已确认目标和实际证据组织后续计划；Draft 不视为已批准范围，已撤销任务不自动恢复。
```
