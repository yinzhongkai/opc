# 配置规范（版本 3）

本规范定义配置的结构与引用要求，供 [初始化协议](SESSION_PROTOCOL.md)、[项目运行协议](PROJECT_PROTOCOL.md) 和只读校验器共同使用。使用 UTF-8 编码，YAML 禁止重复键，不通过自定义 YAML 标签执行代码。

## 路径与 ID

实际项目位于 `projects/<project-id>/`，项目之间不共享成员身份。项目、成员、岗位和知识 ID 使用小写字母开头的小写字母、数字、短横线字符串，匹配 `^[a-z][a-z0-9-]*$`。

岗位、知识文件名必须等于其 ID 加 `.md`；项目 ID 必须等于项目目录名；成员 ID 在本项目内唯一，对应 `members/<member-id>.yaml`。`projects/` 不存在或为空均合法，其下每个非隐藏子目录都应是完整项目。

超级管理员是根 [SUPER_ADMIN.md](SUPER_ADMIN.md) 定义的框架入口，不是项目岗位或成员。项目不配置管理员账号、管理员成员 ID 或 `managedBy`，也不为超级管理员配置 roleKnowledge。

空白模板位于 `templates/project/`。模板中的 `{{project_id}}`、`{{project_name}}` 供复制时替换，不能作为真实项目身份。

## TEAM.yaml

| 字段 | 必需 | 类型与要求 |
|---|---|---|
| `schemaVersion` | 是 | 整数 `3` |
| `project` | 是 | 项目 ID |
| `members` | 是 | 不重复的成员 ID 字符串列表，可为 `[]`；每项定位 `members/<id>.yaml` |
| `roleKnowledge` | 否 | 岗位 ID 到知识 ID 列表的映射；省略为 `{}`，各列表可为空 |

上述字段之外的 TEAM 顶层字段属于配置错误，旧版 `managedBy` 不再有效。`null` 不等于省略，例如 `roleKnowledge: null`、`members: null` 无效。字符串不能代替列表，成员列表不能嵌入旧版成员映射。

TEAM 是在册成员索引。成员文件是身份与分工的唯一事实来源；TEAM 不重复这些字段。配置文件只接受 `.yaml` 扩展名；文件名与内部 id 必须一致，引用不能通过路径或符号链接越出项目 members 目录。未登记的非隐藏 `.yaml` / `.yml` 文件、缺失文件和重复 ID 都属于错误，不能被会话自行认领。其他资料不能作为成员配置使用。

## 成员文件

每个 `projects/<project-id>/members/<member-id>.yaml` 使用 YAML 映射，仅包含下列三个必需字段：

| 字段 | 类型与要求 |
|---|---|
| `id` | 成员 ID，项目内唯一 |
| `role` | 单个岗位 ID |
| `scope` | 至少包含一个非空字符串的列表，描述具体职责范围或产出 |

成员文件不配置个人知识、平台会话 ID 或会话标题，也不接受上述三个字段之外的配置。在册成员必须引用 active 项目岗位；不能绑定框架入口 `super-admin`。岗位或知识的状态属于公共定义有效性，与成员配置分开。

例如 TEAM 的 `members: [project-manager-01]` 对应文件 `members/project-manager-01.yaml`，其内容形如：

```yaml
id: project-manager-01
role: project-manager
scope:
  - 在已确认范围内协调本项目计划、依赖和风险
```

这是配置样例，不代表任何实际成员。新会话必须实际读取对应文件才能绑定；索引有效不代表平台会话已经创建或初始化。

## 项目岗位知识

`roleKnowledge` 的每个键必须指向 active 岗位，各知识 ID 必须存在且为 active。允许为暂未配置成员的 active 岗位预置补充知识。有效集合按基础列表在前、补充列表在后、首次出现顺序去重；重复知识 ID 不改变加载结果。

复制起点见 [空白 TEAM.yaml](templates/project/TEAM.yaml)。项目占位符仅在模板路径下有效。

## 岗位文件

`roles/<role-id>.md` 使用 YAML 文件头，后接完整职责、边界和主要产出：

```yaml
---
id: developer
name: 研发工程师
status: active
knowledge: [software-engineering]
---
```

必需字段为 `id`、非空 `name`、`status`、非空知识 ID 列表 `knowledge`。状态为 `draft`、`active` 或 `deprecated`。所有基础知识引用的文件必须存在；active 岗位的基础知识必须全部 active。

岗位可以增加说明性元数据，但不能用它覆盖公共身份或授权规则。职责是否与某成员的 scope 一致需要人工判断，结构检查不能代替这种判断。

## 知识文件

`knowledge/<knowledge-id>.md` 的必需文件头为 `id`、非空 `name`、`status`，状态为 `draft`、`active` 或 `deprecated`。正文保存方法、适用条件和判断依据，可以增加说明性元数据。

知识正文中的链接属于参考资料，不自动形成递归加载列表或授予职责。有效知识只由岗位基础列表和项目补充列表决定；完成任务确实需要的参考资料再按需读取。

## 项目共享文件

实际项目及空白模板均包含 `AGENTS.md`、`PROJECT.md`、`TEAM.yaml`、`members/README.md`、`TASKS.md`、`STATUS.md`、`DECISIONS.md`、`HANDOFFS.md` 和 `artifacts/README.md`。空白模板使用空 members 列表，不预置真实成员文件。没有任务、决定或交接时明确写“暂无”，不要把格式样例登记为真实记录。

Markdown 记录字段和状态由 [PROJECT_PROTOCOL.md](PROJECT_PROTOCOL.md) 定义。初始化时还须人工核对 PROJECT 中的确认人、协调记录维护人和任务事实；校验器不把 Markdown 的业务语义当作已验证事实。

## 校验与变更

运行 `python scripts/validate_framework.py` 检查当前根目录，或使用 `--root <框架根目录>` 指定另一份框架。只读检查报告路径与问题，不自动修复配置。

校验覆盖文件存在性、YAML 类型和重复键、ID、岗位及知识状态、成员索引与文件对应、岗位/知识引用，以及普通 Markdown 本地链接。外部链接、自然语言理解、业务正确性、授权真实性、任务依赖环和并发写入不在本版自动校验范围内。

修改公共定义先检查当前仓库全部项目的基础和补充引用，以及超级管理员入口对 `team-management` 的引用；修改项目组合检查该项目相应岗位的全部成员。退役定义前迁移在用引用，保留历史定义与工作记录。版本 1、2 不能直接作为版本 3 使用，见 [迁移说明](MIGRATIONS.md)，校验器不自动迁移。
