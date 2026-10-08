# 自由的皮皮虾：项目入口

项目 ID：`free-pipi-shrimp`。

遵循根目录 [AGENTS.md](../../AGENTS.md)。成员工作先执行 [会话初始化协议](../../SESSION_PROTOCOL.md)，会话创建和消息边界见 [SESSION_MESSAGING.md](../../SESSION_MESSAGING.md)，项目运行规则见 [PROJECT_PROTOCOL.md](../../PROJECT_PROTOCOL.md)，配置规范见 [CONFIG_SCHEMA.md](../../CONFIG_SCHEMA.md)。

- 目标、约束和确认责任：[PROJECT.md](PROJECT.md)。
- 成员索引与项目角色技能：[TEAM.yaml](TEAM.yaml)；成员身份和分工：[成员文件说明](members/README.md)。
- 当前任务与进展：[TASKS.md](TASKS.md)；总体摘要：[STATUS.md](STATUS.md)。
- 决定：[DECISIONS.md](DECISIONS.md)；行动请求：[HANDOFFS.md](HANDOFFS.md)。
- 成果：[artifacts/README.md](artifacts/README.md)。
- 工作区声明：[WORKSPACE.yaml](WORKSPACE.yaml)。

本入口只定位项目资料，不复制动态成员表、任务状态或身份解析规则。按根会话协议分层加载当前工作，任务内指向自己的评审安排也属于待办，无关历史按需读取。

超级管理员是根 [框架入口](../../SUPER_ADMIN.md)，不绑定项目成员。请求建项或增员时由用户进入该会话处理。只有用户或超级管理员可以创建成员会话；同项目成员可以互发消息，成员不得直接联系超级管理员或其他项目成员。
