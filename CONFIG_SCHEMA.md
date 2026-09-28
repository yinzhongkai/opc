# 配置规范（TEAM 版本 3；工作区版本 1）

本规范定义成员配置和项目工作区声明的结构与引用要求，供 [初始化协议](SESSION_PROTOCOL.md)、[项目运行协议](PROJECT_PROTOCOL.md) 和只读校验器共同使用。使用 UTF-8 编码，YAML 禁止重复键，不通过自定义 YAML 标签执行代码。

## 路径与 ID

实际项目位于 `projects/<project-id>/`，项目之间不共享成员身份。项目、成员、岗位和知识 ID 使用小写字母开头的小写字母、数字、短横线字符串，匹配 `^[a-z][a-z0-9-]*$`。

岗位、知识文件名必须等于其 ID 加 `.md`；项目 ID 必须等于项目目录名；成员 ID 在本项目内唯一，对应 `members/<member-id>.yaml`。`projects/` 不存在或为空均合法，其下每个非隐藏子目录都应是完整项目。

成员 ID 的默认命名规则是：首名且通常唯一的岗位成员直接使用岗位 ID，例如 `project-manager`；不为可能出现的后续成员预先追加 `-01`。确有多人同岗、岗位 ID 已被当前成员或历史记录使用，或需要表达稳定分工时，可追加有意义的职责后缀，必要时再使用编号。已有合法成员 ID 不因命名风格变化自动重命名，避免破坏任务、决定、交接和成果中的引用。

超级管理员是根 [SUPER_ADMIN.md](SUPER_ADMIN.md) 定义的框架入口，不是项目岗位或成员。项目不配置管理员账号、管理员成员 ID 或 `managedBy`，也不为超级管理员配置 roleKnowledge。

空白模板位于 `templates/project/`。模板中的 `{{project_id}}`、`{{project_name}}` 供复制时替换，不能作为真实项目身份或工作区路径。

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

例如 TEAM 的 `members: [project-manager]` 对应文件 `members/project-manager.yaml`，其内容形如：

```yaml
id: project-manager
role: project-manager
scope:
  - 在已确认范围内协调本项目计划、依赖和风险
```

这是配置样例，不代表任何实际成员。新会话必须实际读取对应文件才能绑定；索引有效不代表平台会话已经创建或初始化。

## 项目岗位知识

`roleKnowledge` 的每个键必须指向 active 岗位，各知识 ID 必须存在且为 active。允许为暂未配置成员的 active 岗位预置补充知识。有效集合按基础列表在前、补充列表在后、首次出现顺序去重；重复知识 ID 不改变加载结果。

复制起点见 [空白 TEAM.yaml](templates/project/TEAM.yaml)。项目占位符仅在模板路径下有效。

## WORKSPACE.yaml

`projects/<project-id>/WORKSPACE.yaml` 声明业务工作区位于何处、由什么工具管理以及项目记录对应的精确源码版本。它是 OPC 的统一声明入口，不是自动执行器；读取配置不授权克隆、联网、切换提交、覆盖本地修改或操作框架目录之外的文件。

所有项目都必须提供该文件并使用工作区配置版本 1。框架不再提供 `projects/<project-id>/workspace/` 内嵌目录，也不为缺失配置建立兼容回退；尚未确定源码或工作资料位置时使用 `driver: none`，不能用隐式目录代替明确边界。

顶层只允许两个字段：

| 字段 | 必需 | 类型与要求 |
|---|---|---|
| `schemaVersion` | 是 | 整数 `1` |
| `workspace` | 是 | 工作区映射 |

`workspace` 的公共字段只有 `driver`。`driver` 必须是 `none`、`git`、`submodule` 或 `repo`；除 `none` 外，各驱动必须提供 `checkout`。checkout 使用 `/` 的可移植相对路径，不接受盘符或绝对路径。`submodule` 必须位于当前 `projects/<project-id>/` 内；`git` 和 `repo` 必须使用框架根目录的同级路径，例如 `../space-rhythm`，不能继续向更高目录逃逸。配置只声明默认位置，本机另有布局时由用户或平台在实际操作中明确指定，不把机器专有绝对路径写回共享配置。

### none

适用于尚无源码仓库，或只使用 OPC 的任务、决定、状态和成果记录而不需要日常工作区的项目：

```yaml
schemaVersion: 1
workspace:
  driver: none
```

除 `driver` 外不接受其他 workspace 字段。`none` 不创建隐式目录，也不授权把草稿、源码或中间产物直接放进项目记录目录；项目开始需要工作区时，先经用户确认改为 `git`、`submodule` 或 `repo`。

### git

适用于 `space-rhythm` 一类具有独立源码、构建、测试和发布生命周期的产品仓库，是代码型项目的推荐模式：

```yaml
schemaVersion: 1
workspace:
  driver: git
  repository: git@example.com:team/example.git
  branch: main
  revision: 0123456789abcdef0123456789abcdef01234567
  checkout: ../example
```

`repository` 和 `branch` 必须是非空字符串；`revision` 必须是完整的 40 或 64 位小写 Git 提交哈希。`branch` 表示日常演进线，`revision` 才是 OPC 项目记录所对应的可复现基线。升级源码基线时先验证目标提交，再在同一项目变更中更新 `revision`、相关任务或状态记录；不能只跟踪可移动分支而宣称版本已锁定。

### submodule

适用于必须嵌入项目目录、又需要由上层 Git 精确锁定少量依赖仓库的场景：

```yaml
schemaVersion: 1
workspace:
  driver: submodule
  repository: git@example.com:team/dependency.git
  revision: 0123456789abcdef0123456789abcdef01234567
  checkout: projects/example/source
```

`repository` 为非空字符串，`revision` 使用完整提交哈希。配置必须与 `.gitmodules` 和实际 gitlink 一致；只读校验器检查声明结构，不读取 Git 对象或联网验证远端。产品主源码默认不使用该模式，以免把独立产品生命周期重新耦合到 OPC 项目分支。

### repo

适用于内核、Bootloader、Yocto Layer、系统组件和应用等多个 Git 仓库共同组成一个产品的场景：

```yaml
schemaVersion: 1
workspace:
  driver: repo
  manifestRepository: https://example.com/platform/manifest.git
  manifestRevision: release-1.0
  manifestFile: manifests/default.xml
  checkout: ../example
```

`manifestRepository`、`manifestRevision` 和 `manifestFile` 均为非空字符串；Manifest 文件必须是仓库内不含 `..` 的相对 `.xml` 路径。项目发布时应把 Manifest 固定到可复核版本，并确保其中各仓库 revision 满足发布可复现要求。校验器不安装或调用 Google Repo，也不验证外部 Manifest 内容。

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

新建实际项目及空白模板均包含 `AGENTS.md`、`PROJECT.md`、`TEAM.yaml`、`WORKSPACE.yaml`、`members/README.md`、`TASKS.md`、`STATUS.md`、`DECISIONS.md`、`HANDOFFS.md` 和 `artifacts/README.md`。成员日常读写与中间产物落点由 WORKSPACE 声明；正式成果仍入 `artifacts/`。项目模板不再包含 `workspace/` 目录。空白模板使用空 members 列表，不预置真实成员文件。没有任务、决定或交接时明确写“暂无”，不要把格式样例登记为真实记录。

Markdown 记录字段和状态由 [PROJECT_PROTOCOL.md](PROJECT_PROTOCOL.md) 定义。初始化时还须人工核对 PROJECT 中的确认人、协调记录维护人和任务事实；校验器不把 Markdown 的业务语义当作已验证事实。

## 校验与变更

运行 `python scripts/validate_framework.py` 检查当前根目录，或使用 `--root <框架根目录>` 指定另一份框架。只读检查报告路径与问题，不自动修复配置。

校验覆盖文件存在性、YAML 类型和重复键、ID、岗位及知识状态、成员索引与文件对应、岗位/知识引用、工作区驱动字段和可移植路径，以及普通 Markdown 本地链接。它不读取 Git 对象，不安装或调用 Submodule/Repo，不访问外部仓库，也不验证 checkout 当前提交。外部链接、自然语言理解、业务正确性、授权真实性、任务依赖环和并发写入不在本版自动校验范围内。

修改公共定义先检查当前仓库全部项目的基础和补充引用，以及超级管理员入口对 `team-management` 的引用；修改项目组合检查该项目相应岗位的全部成员。退役定义前迁移在用引用，保留历史定义与工作记录。版本 1、2 不能直接作为版本 3 使用，见 [迁移说明](MIGRATIONS.md)，校验器不自动迁移。
