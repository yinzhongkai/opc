# Codex 使用方式

让各成员会话能够访问模板根目录与目标项目文件，首条消息声明项目和成员：

```text
项目：space-rhythm
成员：organization-manager-01
请读取 SESSION_PROTOCOL.md 并初始化。
若当前位于项目子目录，协议位于 ../../SESSION_PROTOCOL.md。
```

当前唯一成员的岗位是 `organization-manager`。初始化回执应列出基础知识 `team-management`，项目补充知识为空，并说明当前成员负责团队、岗位和知识配置及初始化缺口跟踪，具体以项目成员表为准。

## 入口

Codex 从项目根目录到工作目录读取适用的 `AGENTS.md`；找不到项目根目录时只检查当前目录。本模板的项目入口显式指向根目录协议，方便从根目录或项目子目录开展工作。见 [官方 AGENTS.md 文档](https://learn.chatgpt.com/zh-Hans/docs/agent-configuration/agents-md)。

公共协议负责按“成员 → 岗位基础知识 + 项目岗位补充知识”读取 `knowledge/` 中的普通文档。

## 共享资料

核对初始化回执的项目、成员、岗位、知识来源与实际文件路径。你负责创建和归档会话；组织管理员维护成员及项目岗位补充知识，成员直接更新自己的任务进展，项目经理维护任务计划并汇总项目状态。
