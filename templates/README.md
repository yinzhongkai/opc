# 空白项目模板

`project/` 是建项时使用的模板，不是实际项目，不能据此初始化成员或执行示例任务。实际项目目录保持在 `projects/<project-id>/`。

按 [项目运行协议](../PROJECT_PROTOCOL.md) 接收用户建项请求后，将 `project/` 的全部文件复制到尚不存在的项目目录。替换所有 `{{project_id}}` 和 `{{project_name}}`，填写实际日期、用户来源、目标及首批成员；不要覆盖已有目录。

模板采用与真实项目相同的两级深度，链接 `../../AGENTS.md` 等根文件时无需调整。若使用其他目录结构，应先调整公共协议和链接，不直接套用本模板。

模板默认空团队，不配置超级管理员或项目经理。超级管理员是根框架入口，成员在用户提出需求后分别创建配置文件并登记。最终确认人和协调记录维护人的默认规则见 PROJECT；尚无成员时维护人待指定。任务、决定、交接和成果索引初始为空，代码块中的记录样式仅供填写时参考。

| 文件 | 内容 |
|---|---|
| [AGENTS.md](project/AGENTS.md) | 项目入口 |
| [PROJECT.md](project/PROJECT.md) | 名称、目标、约束和确认责任 |
| [TEAM.yaml](project/TEAM.yaml) | 版本 3 的成员 ID 索引与项目岗位知识 |
| [members/README.md](project/members/README.md) | 成员文件格式与创建、绑定说明 |
| [TASKS.md](project/TASKS.md) | 任务记录样式 |
| [STATUS.md](project/STATUS.md) | 状态摘要起点 |
| [DECISIONS.md](project/DECISIONS.md) | 决定记录样式 |
| [HANDOFFS.md](project/HANDOFFS.md) | 交接记录样式 |
| [artifacts/README.md](project/artifacts/README.md) | 成果索引与元信息样式 |

建项后检查占位符、配置与本地链接，输出成员启动消息。结构检查通过不代表成员会话已经初始化或示例任务已经登记。
