# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与位置 | 存储位置 | 负责人 | 关联任务 | 版本/精确提交 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|---|
| A-001 | [笔记组织与工作区候选方案](A-001-note-organization-and-workspace.md) | OPC artifacts | project-manager | [T-001](../TASKS.md) | 0.1；提交按正文路径与 Git 历史定位 | draft | 尚无；D-001、D-002 为 proposed |
| A-002 | [阅读流程与逐书伙伴候选方案](A-002-reading-workflow-and-book-partners.md) | OPC artifacts | project-manager | [T-002](../TASKS.md) | 0.1；提交按正文路径与 Git 历史定位 | draft | 尚无；D-003、D-004、D-005 为输入，整体采用待 D-001、D-002 确认 |
| A-003 | 投资领域书目与阅读推荐；入口 `README.md` | 成果物仓库：`git@github.com:yinzhongkai/reading-notes.git`；本机 `C:/Users/YinZh/Documents/workspace/reading-notes` | book-recommender | [T-007](../TASKS.md#t-007按已确认结构整理首批投资书目) | 0.1；`66ac807098f9c169745e5e339431ec314df69044` | draft | D-008 已确认组织结构与七本收录范围；具体内容尚无另行批准，六本候选未选定 |

本轮流程、笔记组织与工作区讨论以 A-002 为候选；A-001 保留 T-001 的初始方案内容，不代表两份方案均已批准。

两份候选稿中的仓库待定描述反映形成时的信息；后续仓库地址已由 [D-006](../DECISIONS.md#d-006github-成果物仓库地址) 确认，初始化与工作区配置见 [H-001](../HANDOFFS.md#h-001成果物仓库初始化与工作区配置)。本次 Git 提交保存上述讨论记录，方案版本及批准状态保持不变。

OPC 治理与协作成果使用当前项目 `artifacts/` 内的正文链接；产品代码和直接业务交付物使用“成果物仓库 + 仓库内路径”，并记录其最终合并提交。可移动分支名不能代替精确提交。

## A-003 投资领域书目与阅读推荐

- 负责人：book-recommender；关联任务 T-007；版本 0.1；更新日期 2026-10-13（Asia/Shanghai）；状态 draft。
- 存储位置：本机 `C:/Users/YinZh/Documents/workspace/reading-notes`；成果物仓库 `git@github.com:yinzhongkai/reading-notes.git`；目标分支 main，精确合入提交 `66ac807098f9c169745e5e339431ec314df69044`。
- 适用范围：首批七本投资书目的事实档案、主题导航和基于用户接近读完《聪明的投资者》的后续阅读建议；其他领域按后续请求添加。
- 来源与批准范围：T-005/T-006 的聊天研究输入及 D-008 的结构、收录确认；工作区位置依据 D-010。D-008 不代表用户选定其余六本或阅读版本，内容保留 draft，不批准 A-001/A-002 的其他候选安排。
- 验证：10 份 UTF-8 文档、7 个唯一档案、56 处本地链接、索引与档案状态一致，合并前后通过；作者、原书名与参考版本的来源及核对日期写在业务正文。

仓库内文件：

- 总书目入口：`README.md`。
- 投资领域入口：`recommendations/investing/README.md`。
- 本次推荐记录：`recommendations/investing/after-intelligent-investor.md`。
- 单书档案：`books/b001-intelligent-investor/README.md`、`books/b002-most-important-thing/README.md`、`books/b003-one-up-on-wall-street/README.md`、`books/b004-essays-of-warren-buffett/README.md`、`books/b005-common-stocks-uncommon-profits/README.md`、`books/b006-random-walk-down-wall-street/README.md`、`books/b007-security-analysis/README.md`。

## 正文元信息样式（不是真实成果）

```text
项目：<project-id>
成果 ID：A-001
负责人：<member-id>
关联任务：<T-编号>
版本：0.1
更新日期：<实际日期>
状态：draft
适用范围：<本成果覆盖的内容和边界>
来源及输入版本：<授权来源、所依赖成果及版本>
存储位置：<OPC artifacts 路径 | 成果物仓库、仓库内路径与最终合并提交>
批准依据：尚无
版本记录：<日期、版本、主要变化和替代关系>
```

任务完成不自动批准成果。重要交接或批准须引用实际版本及可用的 Git 提交或内容摘要；修改批准内容时更新版本并重新说明评审状态。
