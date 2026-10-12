# 项目成果索引

本项目采用根 [可选图书成果模板](../../../templates/book/README.md)。按 [D-011](../DECISIONS.md)，图书设计、章节规划、蓝图和验证基线保存在 OPC 的 [writing/](writing/README.md)；书稿、配图和官方参考资料位于 [WORKSPACE.yaml](../WORKSPACE.yaml) 声明的独立仓库。本文件统一维护成果 ID、版本、状态、批准依据、当前路径及外部内容的固定 revision 链接。

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与正文链接 | 负责人 | 关联任务 | 版本 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|
| A-001 | [图书设计（读者定位与内容取舍）](writing/design.md) | planner | T-001、T-002 | 0.2 | approved | D-001、D-002、D-005 |
| A-002 | [全书目录与依赖](writing/outline.md) | planner | T-001、T-002 | 0.2 | approved | D-003、D-004 |
| A-003 | [第 1 章蓝图（ch-env-setup）](writing/chapters/ch-env-setup/plan.md) | planner | T-003 | 0.1 | approved | D-006 |
| A-004 | [第 1 章正文（ch-env-setup，用户反馈硬件图修订复核稿）](https://github.com/yinzhongkai/book-zephyr/blob/08092e0a5af5d2895e44911b7282bb3b29d3b64e/zephyr/01-认识nRF54L15-DK与开发环境搭建.md) | writer | T-004、T-006 | 0.6 | in_review | 尚无 |
| A-005 | [技术与实验环境基线](writing/environment.md) | developer | T-005 | 0.3 | approved | D-007 |

## D-011 资料迁回与 T-007 书稿整理

按用户确认的 D-011，四份写作资料已从制品库 writing/ 迁回 OPC artifacts/writing/，上表已同步其当前正文位置。设计、目录与蓝图逐字节不变，环境基线仅修正资料包及证据的位置说明和链接；版本、负责人及原批准状态不变。

按 [D-012](../DECISIONS.md)，内容仓库本地与远端 main 均为唯一初始 commit `08092e0a5af5d2895e44911b7282bb3b29d3b64e`。当前 11 个文件齐全，书稿、图片、5 份独立 PDF、硬件 ZIP 及根 README 的原始哈希不变；参考资料索引仅修正旧历史恢复说明。参考目录目前 51,888,310 字节，18 个本地文件链接有效；旧原理图仍完整保存在 ZIP 的 39 个文件中。.git 从 58,693,332 降至 50,925,289 字节（减少 13.235%），完整性检查通过。当前路径映射如下。

| 成果或附件 | 所在仓库 | 当前路径 |
|---|---|---|
| A-001 图书设计 | OPC | `projects/book-zephyr/artifacts/writing/design.md` |
| A-002 全书目录 | OPC | `projects/book-zephyr/artifacts/writing/outline.md` |
| A-003 第 1 章蓝图 | OPC | `projects/book-zephyr/artifacts/writing/chapters/ch-env-setup/plan.md` |
| A-004 第 1 章正文 | book-zephyr | `zephyr/01-认识nRF54L15-DK与开发环境搭建.md` |
| A-005 环境基线 | OPC | `projects/book-zephyr/artifacts/writing/environment.md` |
| 第 1 章配图 | book-zephyr | `zephyr/images/ch-env-setup/figure-1-1-nrf54l15-dk-front.png` |

当前书稿入口为 [第 1 章编号正文](https://github.com/yinzhongkai/book-zephyr/blob/08092e0a5af5d2895e44911b7282bb3b29d3b64e/zephyr/01-认识nRF54L15-DK与开发环境搭建.md)，已推送远端；上表 A-004 的当前位置已同步，专业版本与批准状态不变。2026-10-11，框架超级管理员按用户授权核对当前成果路径，并将 WORKSPACE 锁定到新初始提交 08092e0a5af5d2895e44911b7282bb3b29d3b64e；OPC 集成与 [H-001](../HANDOFFS.md) 的关闭证据见 [T-007](../TASKS.md)。原目录整理提交 85f693201bc05ea92c94615ae1a7e0e111f7d21d、5e99dec114f3138bf6b551902bd76c291e9057cb、23e94fc954f7c17dff4e1b3725832c35c9196644 等已归入 D-012 的仓库外恢复包；旧输入和评审记录的路径与精确 SHA 保留，历史外部链接需从该恢复包读取。

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
