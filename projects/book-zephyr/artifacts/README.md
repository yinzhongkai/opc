# 项目成果索引

本项目采用根 [可选图书成果模板](../../../templates/book/README.md)。图书内容位于 [WORKSPACE.yaml](../WORKSPACE.yaml) 锁定的独立仓库 `book/`，本文件在 OPC 中维护成果 ID、版本、状态、批准依据和固定 revision 链接。

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与正文链接 | 负责人 | 关联任务 | 版本 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|
| A-001 | [图书设计（读者定位与内容取舍）](https://github.com/yinzhongkai/book-zephyr/blob/695ec76e54639c9faf9462b7b043c644b2db9ec2/book/design.md) | planner | T-001、T-002 | 0.2 | approved | D-001、D-002、D-005 |
| A-002 | [全书目录与依赖](https://github.com/yinzhongkai/book-zephyr/blob/695ec76e54639c9faf9462b7b043c644b2db9ec2/book/outline.md) | planner | T-001、T-002 | 0.2 | approved | D-003、D-004 |
| A-003 | [第 1 章蓝图（ch-env-setup）](https://github.com/yinzhongkai/book-zephyr/blob/695ec76e54639c9faf9462b7b043c644b2db9ec2/book/chapters/ch-env-setup/plan.md) | planner | T-003 | 0.1 | approved | D-006 |
| A-004 | [第 1 章正文（ch-env-setup，用户反馈硬件图修订复核稿）](https://github.com/yinzhongkai/book-zephyr/blob/695ec76e54639c9faf9462b7b043c644b2db9ec2/book/chapters/ch-env-setup/text.md) | writer | T-004、T-006 | 0.6 | in_review | 尚无 |
| A-005 | [技术与实验环境基线](https://github.com/yinzhongkai/book-zephyr/blob/695ec76e54639c9faf9462b7b043c644b2db9ec2/book/environment.md) | developer | T-005 | 0.3 | approved | D-007 |

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
批准依据：尚无
版本记录：<日期、版本、主要变化和替代关系>
```

任务完成不自动批准成果。重要交接或批准须引用实际版本及可用的 Git 提交或内容摘要；修改批准内容时更新版本并重新说明评审状态。
