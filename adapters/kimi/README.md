# Kimi Work 使用方式

根据所用模式，以项目文件、附件或可访问目录提供公共资料。本模板不假定 Kimi Work 可直接读写电脑上的目录。

初始化一个成员需要：

- 根目录 `AGENTS.md`、`SESSION_PROTOCOL.md`。
- 目标项目的七个共享文件与相关产物，尤其是包含 `roleKnowledge` 和成员名单的 `TEAM.yaml`。
- 成员的 `roles/<role-id>.md`。
- 该岗位基础知识与当前项目补充知识合并后的全部 `knowledge/<id>.md`。

例如，`space-rhythm` 的成员 `organization-manager-01` 使用岗位 `organization-manager`，需要基础知识 `team-management.md`，没有项目补充知识。这是可直接读取的普通 Markdown 资料。

组织管理员另需两个共享索引和候选定义；检查公共定义变更影响时，需要相关的其他项目成员表和岗位配置。资料不能访问时，报告未能检查的影响范围。

支持项目公共指令时使用 [PROJECT_INSTRUCTIONS.md](PROJECT_INSTRUCTIONS.md)；否则将其中内容放在新会话首条消息，再追加项目与成员声明。

无法写文件时，输出路径和建议写回内容，明确说明尚未保存，交由用户或有权限的会话写回。知识文档提供领域方法，实际文件访问与写回能力以当前 Kimi Work 模式为准。
