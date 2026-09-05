# 太空律动：项目入口

项目 ID：`space-rhythm`。显示名称：太空律动。

遵循根目录 [AGENTS.md](../../AGENTS.md)。项目基本信息在 [PROJECT.md](PROJECT.md)，成员配置在 [TEAM.yaml](TEAM.yaml)，任务和状态在 [TASKS.md](TASKS.md) 与 [STATUS.md](STATUS.md)。

当前配置 `organization-manager-01`（组织管理员）和 `product-manager-01`（产品经理）。共享岗位位于 `../../roles/<role-id>.md`，知识位于 `../../knowledge/<knowledge-id>.md`。有效知识按岗位基础 `knowledge`、项目 `roleKnowledge` 的顺序合并去重；当前无项目补充知识，组织管理员使用基础知识 `team-management`，产品经理使用基础知识 `requirements-analysis`。

用户声明成员身份后，须执行根目录 `SESSION_PROTOCOL.md`。2026-09-05 核对时该文件缺失，已记入状态与待办；补齐前不能宣称完成标准成员初始化。维护项目配置不等于绑定成员身份。

用户已要求依据此前讨论整理 V1 PRD 并绑定本项目；成果 `SR-A001` 已归档到 `artifacts/`，当前为 Draft，批准前不作为实施基线。已被 PRD 吸收的重复项目简介和产品经理拆分纪要已清理；保留的技术说明与逆向报告可用于追溯，但不自动成为项目决定或任务依据。

当前配置、决定和资料纳入规则见 [PROJECT.md](PROJECT.md) 与 [DECISIONS.md](DECISIONS.md)。修改共享文件前重新读取，保留无关更改。

## 产品经理会话启动消息

以下为成员启动消息，待上述初始化协议补齐后使用；本次增员仅更新配置，尚未创建或初始化成员会话。

```text
项目：space-rhythm
成员：product-manager-01
请执行会话初始化，按根目录 SESSION_PROTOCOL.md 读取成员记录、岗位基础知识、项目补充知识和项目资料，汇报岗位、分工、实际读取的文件与当前任务。
加载项目成果 SR-A001 及其任务/决定状态；此前撤销的业务任务不自动恢复，Draft 不视为已批准范围。
```
