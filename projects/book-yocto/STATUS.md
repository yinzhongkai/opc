# 项目状态摘要

- 汇总日期与信息截至点：2026-11-20。用户明确“我校验完成了，感觉这一章的内容可以了”，第 4 章正文按 [D-016](DECISIONS.md) 定稿；本次刷新章节批准与发布收尾状态，其他任务沿用原记录。
- 维护人：project-manager（按 [PROJECT.md](PROJECT.md) 指定的协调记录维护人）。
- 当前阶段：P1/P2 完成，chapter 1–4 正文已定稿。T-016 in_progress，书稿与 meta-tiger 均已合入各自 main，WORKSPACE 两份 revision 已回写；OPC 项目合入随本轮五笔功能整理收口，生效边界见 T-016。配套 chapter4 标签及未结交接仍待处理，获确认正文见 D-016，计划范围不变。
- 任务进展：
  - T-001 书稿现状评估（writer）：completed，产出 [A-001](artifacts/A-001-书稿现状评估.md) v0.1（draft）。
  - T-002 起草全书打磨计划（project-manager）：completed，[A-002](artifacts/A-002-book-polishing-plan.md) 当前 v0.8（draft，保留 D-007/D-010/D-011/D-013 要求；在 QEMU 模型开发与 Yocto 集成验证之前加入 architect 平台设计、专业核对与用户确认，并作为 writer 蓝图输入；原起草任务不因此重新打开）。
  - T-003 按章节抽取待校验知识点清单（writer）：completed，产出 [A-003](artifacts/A-003-知识点校验清单.md) v0.1（draft）：86 条注记 + 跨章 X 系列 4 条。
  - T-004 修复 index.md（writer）：completed（2026-09-19）。
  - T-005 体例约定草案（writer）：completed，[A-004](artifacts/A-004-体例约定草案.md) 当前 v0.5 **approved**；v0.3 于 2026-09-25 经用户批准，v0.4 同日依据 D-009 增补全书 `-` 紧凑列表风格，v0.5 的后续批准见 T-013。
  - T-006 chapter 1 试点闭环（writer）：**completed（2026-09-25 用户确认定稿，定稿版本 Git 92ede7e，816 行）**——第 1 轮评审（R-1~R-5、U-1~U-16）、定稿前修订（E-1~E-3）、定稿通读意见（U-17~U-20）、T-009 实测不符项（N-1/N-2/O-1）全部处理并复核闭环；U-3 删节方案随定稿确认闭环。chapter 1 成为全书首章"每个运行结果有实测背书"的定稿，P2 流程试点完成，为 P3 定型；列表风格归一已由 D-009/T-010 闭环。
  - T-007 全书人物"老周"更名"达哥"（writer，D-008）：completed（2026-09-25）——18 文件 381 处逐文件提交（ae9052c…bc6fa78），PM 复核通过：书稿目录"老周"零残留，"达哥"405 处逐文件吻合，抽查语义中性。
  - T-008 真实环境运行核查 chapter 1 环境信息（reviewer）：completed（2026-09-23 容器实测）。
  - T-009 真实环境首次构建与启动核查 chapter 1 主线（reviewer）：completed（2026-09-25）——远程服务器 tiger 实测：local.conf 七项一致、续跑构建成功（4073 tasks 全过）、runqemu guest 逐项核对；[核查记录](artifacts/work-records/t009-chapter1-build-check.md) 登记 N-1/N-2 不符项与 O-1/O-2 观察项；PM 复核通过，N-1/N-2/O-1 已随 T-006 定稿修订处理。
  - T-010 task02 列表记号归一与顺手修订（writer，D-009）：completed（2026-09-26，PM 复核通过：20 项列表逐字一致归一、L135 输出保留正确、scarthgap 修正落实）。
  - T-011 chapter 2《读懂这个项目》修订与第 1 轮评审（writer/reviewer/用户）：**completed（2026-09-27 用户确认定稿，Git `cf9a71c`，631 行）**——U-1~U-18、R-1~R-6、T-012 实测不符项、四图 ASCII 呈现及全部针对性复核均已闭环。
  - T-012 chapter 2 命令、配置与输出真实环境核验（reviewer）：completed（2026-09-26 project-manager 复核通过）——11 组核验项覆盖本章命令、配置、输出与机制边界，登记 N-1~N-5/O-1；[核验记录](artifacts/work-records/t012-chapter2-env-check.md) 已提交，不符项转入 T-011。
  - T-013 补充硬件型号与等宽文本排版规则（writer）：completed——A-004 v0.5 于 2026-09-27 经用户明确批准，approved 状态与成果索引已在 Git `36c5e35` 回写并经 project-manager 复核。
  - T-014 chapter 3《搭起 meta-tiger 的骨架》修订与第 1 轮评审（writer/reviewer/project-manager/用户）：**completed（2026-10-08 用户最终确认）**——2026-10-03～2026-10-08 完成用户逐项审校、writer 统一修订、reviewer 针对性复核及 Docker 真实环境复验/补证；最终复核 `pass`。源码整理为 `9c745965...`，以 merge commit `f444790f...` 合入并推送 `main`；最终净差异仅 chapter 3 正文，215 insertions / 123 deletions。验证报告与日志按 D-012 保存在 OPC 工作记录，不进入书稿主线。
  - T-015 chapter 3 命令、配置与输出真实环境核验（reviewer）：completed（2026-09-27 project-manager 复核通过）——12 组核验 9 组通过、3 组局部不符；构建和两次 `yocto-check-layer` 成功，错误实验均恢复，远端 `meta-tiger` 提交为 `0e3da48`；N-1/N-2/O-1 已流转至 T-014。
  - T-016 chapter 4《让 QEMU 长出 tiger 这块板》修订与第 1 轮评审：in_progress（正文已定稿，书稿与工程均已合入 main；OPC 项目合入随本轮功能整理收口，配套标签及未结交接仍待处理）。
  - T-017 全书四仓库叙事迁移为 `meta-tiger` patch-only 模型：todo（2026-10-09 已登记；全文扫描命中前言、chapter 2–16、尾声、附录 B/C 共 19 个文件；等待 T-016 收口后再建立配对分支，不并行启动）。
- 团队状态：六名成员登记不变。用户本章校验与定稿确认已完成，project-manager 维护批准版本和发布收尾记录，各成员历史评审保留原版本与范围。
- 后续章节修订参考：[第 4 章人工校审记录](artifacts/work-records/t016-chapter4-manual-review-style.md)已整理本轮明确的正文、示例、命令输出和排版偏好，结合 A-004 使用。
- 开发仓库状态：书稿已普通推送 `main@619c4efcd57a08818572f360b61b97abe2462ed0`，包含 2 个单行说明功能提交，原书稿 feature 已清理，本机恢复点仍保存旧头 `f6b45b536158570813442841bcff27da052d1c93`；第 4 章批准正文未变，第 3 章补注当前工程版本。meta-tiger 的 `main@79d9696c1d37fcbf65724eb05a253b3168d2eab7` 保持不变，原工程 feature 和 master 已清理，旧历史由本机恢复点保留。OPC 原 54 笔开发历史保留，本轮以 5 个功能提交收口到项目分支；两份 WORKSPACE revision、tiger 主机和路径不变，chapter4 标签及未结交接继续由 T-016 跟踪。
- 关键成果：第 4 章正文已获用户确认定稿（D-016）；A-004 v0.5 approved、A-002 v0.8 draft、A-005 v0.2 approved / D-014、A-006 v0.1 draft。蓝图和各阶段报告保留其原版本及当时状态。当前工程构建与手工复位证据见[工程回执](artifacts/work-records/remove-tests-minboot-result.json)和[监控器日志](artifacts/work-records/manual-monitor-minboot-20261120.log)。
- 阻塞、待确认事项及下一位行动人：
  - **chapter 3 无未完成阻塞**：用户确认、专业复核、真实环境验证、源码合并和远端推送均已完成。
  - **平台设计确认及回写已完成**：用户批准 A-005 v0.2（D-014），architect 元信息/索引回写已核验，技术正文保持不变；SPI NAND/CS/watchdog 等后续依赖仍在首次使用前补齐，不纳入本章最小实现。
  - **第 4 章定稿及发布收尾**：正文已获用户确认。T-016 跟踪成果物提交、合并、配套 chapter4 标签和 WORKSPACE 最终版本回写，PM-C4-01 随发布收尾。
  - **H-002/H-003 待发起人核对关闭**：两项分工与计划交接均已接收并有处理记录，状态为 accepted，发起人核对关闭待办理。
  - **跨章一致性已登记、不并行启动**：T-017 覆盖 19 个受影响文件，依赖 T-016 的真实 QEMU patch 范式和最终章节版本；T-016 完成前不建立下一轮 feature 分支。
  - **用户持续事项**：继续全书通读校验；A-003 中原四仓库依赖和 chapter 8 的 X-1 将在对应修订轮统一处理。
- 通读期协作约定（2026-09-13 用户确认，2026-09-19 D-007 补充）：通读中发现的问题按类型分流，拿不准的一律先汇集到 project-manager 会话分类、登记并跟踪到关闭；**受评章节的用户通读意见在 writer 统一修订前提交，与成员评审意见一并处理**。

本文件是摘要，原始事实以 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md) 和 [成果索引](artifacts/README.md) 为准。汇总后注明实际信息范围，过期摘要不能覆盖原始记录。
