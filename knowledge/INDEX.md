# 专业知识目录

本目录保存专业领域知识、工作方法和判断依据。每项知识使用一份普通 Markdown 文档，直接放在 `knowledge/` 下。

| 知识 ID | 名称 |
|---|---|
| [backend-development](backend-development.md) | 后端开发 |
| [critical-review](critical-review.md) | 独立评审 |
| [frontend-development](frontend-development.md) | 前端开发 |
| [planning](planning.md) | 方案规划 |
| [project-management](project-management.md) | 项目管理 |
| [requirements-analysis](requirements-analysis.md) | 需求分析 |
| [research](research.md) | 资料研究与事实核查 |
| [software-development-basics](software-development-basics.md) | 软件研发基础 |
| [software-engineering](software-engineering.md) | 软件工程 |
| [system-design](system-design.md) | 系统设计 |
| [team-management](team-management.md) | 团队组织与成员维护 |
| [testing](testing.md) | 测试设计与执行 |
| [travel-planning](travel-planning.md) | 旅行规划 |
| [writing](writing.md) | 长篇写作与编辑 |

## 文件约定

一个知识条目对应 `knowledge/<id>.md`，字段与有效引用统一见 [CONFIG_SCHEMA.md](../CONFIG_SCHEMA.md)。必需文件头为：

```yaml
---
id: frontend-development
name: 前端开发
status: active
---
```

正文说明专业知识、方法、适用条件与判断依据。保持文件平铺，需要新增条目时直接增加另一个 Markdown 文件并更新本索引。知识 ID 使用小写字母、数字与短横线，保持引用稳定。

知识不定义成员身份、不分配任务、不授予权限。具体技术栈、旅行日期、读者和预算由项目文档提供；补充知识是为了支撑岗位职责，不把另一岗位的职责一并带入。

## 引用和加载

共享岗位以 `knowledge` 定义基础要求。项目 TEAM 以 `roleKnowledge` 为相应岗位补充知识，成员文件只记录身份和分工。初始化按[协议](../SESSION_PROTOCOL.md)将二者合并去重，并完整读取对应正文。框架级 [超级管理员](../SUPER_ADMIN.md)直接加载 `team-management`，不使用项目岗位知识组合。

专业知识可以被多个岗位、多个项目复用。同项目同岗位的成员加载相同组合，分工差异放在成员 scope。索引不代替知识正文；这些文档也不负责安装工具或提供执行环境。

## 维护

知识状态为 draft、active、deprecated。新会话要求有效集合中的知识全部存在且 active。

修改或退役正文前，检查当前仓库共享岗位的基础引用、全部项目的补充引用及超级管理员入口的直接引用，再确定受影响的会话。退役前迁移仍在使用的引用并保留历史文件。文档更新不代表其他会话已经读取，也不会自动同步到其他克隆；成员开始新任务时按初始化协议刷新当前可访问的资料。
