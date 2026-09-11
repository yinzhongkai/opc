# 可选图书成果模板（第一版）

本目录是图书项目的成果模板，不是第二套项目模板、真实书稿或平台原生 Skill。通用项目仍只从 [空白项目模板](../README.md) 建立；不自动建员、不自动派任务、不启动写作或发布。

方法入口：[图书生产与统稿](../../knowledge/book-production.md)、[图书规划](../../knowledge/book-planning.md)、[写作](../../knowledge/writing.md)、[图书审校](../../knowledge/book-review.md)、[读者反馈](../../knowledge/reader-feedback.md)。技术书按需使用 [实验验证](../../knowledge/technical-book-validation.md)。身份、评审、批准与交接仍遵循 [项目运行协议](../../PROJECT_PROTOCOL.md)，不在模板里重定义状态机。

## 采用方式

1. 用户确认采用图书工作方式后，在实际项目 PROJECT 记录适用方法、书稿位置、读者反馈方式、确认范围与记录责任。已存在的项目局部更新，不重新复制空白项目。
2. 超级管理员按授权为相应岗位合并补充知识，保留已有组合和成员。知识可以预置给尚无成员的岗位；是否创建成员仍以用户要求为准。
3. 有效成员受理具体工作后，仅复制需要的成果模板到 `projects/<project-id>/artifacts/book/`。本 README 留在模板目录，不复制到书稿中；项目入口链接本页即可。无实际工作时不创建空章节和伪任务。
4. 将 `<待填写>` 等说明替换为实际信息，未知写明待确认、缺口和来源。登记实际成果及元信息，依据角色职责确定负责人；模板行不是事实或验收结果。
5. 每章只保留一份当前正文；保存蓝图和反馈证据，评审引用 TASKS 原始记录。复制后核对链接，运行框架校验；校验不代表内容正确或书稿获批。

## 岗位补充知识建议

以下只是合并片段，不是完整 TEAM；按任务选用相关岗位，不要求全部创建。通用组合不包含硬件实验要求：

```yaml
roleKnowledge:
  project-manager: [book-production]
  planner: [writing, book-production, book-planning]
  writer: [book-production, reader-feedback]
  reviewer: [book-production, book-review, reader-feedback]
  researcher: [book-production, book-review]
  developer: [book-production]
  tester: [book-production, reader-feedback]
```

技术书可以将下列片段逐岗位追加、去重，不覆盖上面的组合或项目原有知识；不适用的岗位不必配置：

```yaml
roleKnowledge:
  planner: [technical-book-validation]
  writer: [technical-book-validation]
  reviewer: [technical-book-validation]
  researcher: [technical-book-validation]
  developer: [technical-book-validation]
  tester: [technical-book-validation]
```

协调由项目经理或用户按公共协议安排；规划师负责目录和蓝图，作者负责正文及统稿，专业成员提供本职范围的实现或核查。独立评审者、研究员和测试工程师按实际需要配置；没有实际参与者时不伪造对应评审或执行记录。

## 模板与落点

以下路径相对于实际项目的 `artifacts/book/`。`chapters/ch-001/` 是模板占位目录，可复制为实际稳定章节标识；`ch-001` 不是固定“第一章”，显示章号以目录为准。

| 模板 | 作用与建议维护人 |
|---|---|
| [design.md](design.md) | 规划师维护读者与内容设计；项目目标、资源和确认责任只引用 PROJECT，不维护第二份 |
| [outline.md](outline.md) | 规划师维护阅读顺序、章号、稳定标识和依赖；不复制完成状态 |
| [conventions.md](conventions.md) | 按项目指定的作者或规划师维护写作体例 |
| [glossary.md](glossary.md) | 指定作者维护术语定义和首次正式讲解位置 |
| [sources.md](sources.md) | 指定研究员或作者维护来源；章节引用相应条目 |
| [environment.md](environment.md) | 技术书由指定专业成员维护环境基线；非技术书可不采用 |
| [plan.md](chapters/ch-001/plan.md) | 规划师维护章节蓝图 |
| [text.md](chapters/ch-001/text.md) | 作者维护唯一当前正文 |
| [record.md](chapters/ch-001/record.md) | 指定章节记录维护人整理读者反馈和证据索引；专业结论保留实际提供人，不代签 |
| [examples/README.md](examples/README.md) | 技术书配套示例索引，由对应研发成员维护 |
| [evidence/README.md](evidence/README.md) | 原始证据与共享边界说明，采集者或获授权记录人维护 |
| [releases/README.md](releases/README.md) | 每次成书输出的材料清单样式，不代表已生成或已发布 |

同层与章节内链接复制后仍有效。不要把本页指向根知识的 `../../knowledge/` 链接原样带入更深的项目目录；项目入口采用符合实际深度的链接。表中维护人是建议，不替代成员 scope 或任务安排。

## 成果与记录的归属

正式成果按根协议登记项目、成果 ID、任务、负责人、版本、时间、范围、来源及批准依据。下列模板包含元信息填写位置；作为同一成果附件的示例或证据，通过主成果与索引明确归属、版本和路径，不要求每条日志单独批准。

章节记录保存读者原话、验证结果、版本影响与相关记录链接，不承载第二套评审或批准。TASKS 管工作与原始评审，DECISIONS 管批准，artifacts/README 管成果索引，STATUS 仅按截至时间分别汇总书稿、验证与学习，不以新进度表替代这些原始记录。

## 第一版的边界

目前提供方法、模板和配置组合，不提供自动建书、自动调度、自动正文生成、自动发布或专用书稿语义校验。已有校验器检查知识引用与普通本地文件链接，不检查章节依赖环、文内锚点、实验真实性、学习效果或出版质量。后续工具基于实际缺口另行实施。
