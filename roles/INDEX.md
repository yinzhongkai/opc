# 岗位目录

岗位按共同职责划分，技术方向和业务领域通过项目岗位补充知识表达。每个文件包含职责、边界、产出与基础知识引用。

| 岗位 ID | 名称 | 基础知识 |
|---|---|---|
| [organization-manager](organization-manager.md) | 组织管理员 | `team-management` |
| [project-manager](project-manager.md) | 项目经理 | `project-management` |
| [product-manager](product-manager.md) | 产品经理 | `requirements-analysis` |
| [researcher](researcher.md) | 研究员 | `research` |
| [planner](planner.md) | 规划师 | `planning` |
| [architect](architect.md) | 系统架构师 | `system-design` |
| [developer](developer.md) | 研发工程师 | `software-engineering` |
| [tester](tester.md) | 测试工程师 | `testing` |
| [writer](writer.md) | 作者 | `writing` |
| [reviewer](reviewer.md) | 独立评审者 | `critical-review` |

## 配置规则

文件头必需字段为 `id`、`name`、`status`、`knowledge`。ID 与文件名一致，knowledge 为至少一个知识 ID 的字符串列表，来源为根目录 `knowledge/<id>.md`。

本项目的 TEAM.yaml 使用 `roleKnowledge` 为岗位补充知识。有效知识是基础与补充列表按首次出现顺序合并去重的结果；项目补充不会删掉基础要求或覆盖职责。成员只填写一个 role 和自己的 scope，不直接配置知识。

前后端成员复用 developer；软件与旅行项目经理复用 project-manager；旅行和图书规划成员复用 planner。仅当共同职责、边界或产出确实不同，才考虑新增岗位。

## 维护

状态为 draft、active、deprecated。新会话使用 active 岗位及有效知识。修改共享岗位的基础知识影响所有引用项目；仅本项目需要某项知识时修改本项目 roleKnowledge。

退役岗位前迁移 active 成员，并整理对应项目补充配置；保留岗位文件及历史成员记录供查阅。不要为了停用一个成员而删除共享岗位。
