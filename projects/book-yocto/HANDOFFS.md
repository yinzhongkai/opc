# 行动请求与交接

普通进展直接写 [TASKS.md](TASKS.md)，接收和关闭规则见根 [项目运行协议](../../PROJECT_PROTOCOL.md)。已在任务内明确安排的轻量评审或复核无需重复建立交接；另有补充输入、协调或责任转交请求时再登记。

## H-001：补齐 `meta-tiger` 工作区边界并启动 T-016 执行会话
- 发起人：project-manager
- 目标：用户
- 关联任务：T-016；后续 T-017
- 期望结果：①请用户转交超级管理员，根据 D-013 和现有独立书稿仓库事实，为 `meta-tiger` 补齐符合框架规则的 WORKSPACE/多成果物边界、checkout、默认分支和完整精确 revision；不得覆盖现有书稿基线，也不得在未声明目录中开始工程写入。②超级管理员完成并校验配置、当前 project-manager 会话停止后，由用户创建或启动已登记成员 `book-yocto / writer` 的当前有效会话，先执行 T-016 的章节蓝图与作者修订；writer 交付可实施蓝图后，再由用户按串行约定启动 `book-yocto / yocto-engineer` 完成 QEMU patch、`meta-tiger` 集成和真实环境核验。
- 输入与证据：D-013；T-016；A-002 v0.6；书稿基线 `git@github.com:yinzhongkai/book-yocto.git@f444790f352b46f699fd3d46198f8b9b9f089a62`；用户指定通过 SSH 目标 `tiger` 使用服务器上的 `meta-tiger`。2026-10-09 只读核对得到主机 `tiger`、用户 `oops`、checkout `/home/oops/workspace/meta-tiger`、远端 `git@github.com:yinzhongkai/meta-tiger.git`、远端 HEAD `refs/heads/master`、完整 revision `0e3da48b34ef772532535a730a21878cc30ccf67`，工作树为 `master...github/master` 且无未提交项。writer 与 yocto-engineer 的历史会话均可在归档列表定位，但不属于当前有效会话。
- 未完成事项：H-001 范围内无；进入 writer 会话执行 T-016，以及后续进入 yocto-engineer 会话实施 QEMU patch，均是任务后续行动，不属于本交接未完成配置。T-016 正文、QEMU patch、干净重构建和 `runqemu` 证据尚未产生，不因 H-001 关闭而改变。
- 状态：closed
- 创建日期：2026-10-09
- 接收反馈：2026-10-09 用户将 H-001 转交超级管理员处理，并明确要求使用 `ssh tiger` 上的 `meta-tiger` 环境；超级管理员已接收并执行。用户随后向 project-manager 报告“H-001 已执行完成”，并进一步确认已有 writer 会话。
- 处理结果与证据：`WORKSPACE.yaml` 已改为 `multi-git`，保留书稿 `main@f444790f352b46f699fd3d46198f8b9b9f089a62`，新增 `meta-tiger` 的 `host: tiger`、`master@0e3da48b34ef772532535a730a21878cc30ccf67` 和 `/home/oops/workspace/meta-tiger`。为使远程绝对 checkout 合规，项目提前采用 `opc-v0.0.4-dev` 的 multi-git 与 SSH checkout 公共能力；来源、风险和验证见 PROJECT。2026-10-09 project-manager 复核：结构校验通过（421 处本地链接），从 `scripts/` 运行 60 项测试全部通过；SSH 终态仍为干净 `master...github/master`、完整 HEAD `0e3da48b34ef772532535a730a21878cc30ccf67`、远端 `git@github.com:yinzhongkai/meta-tiger.git`。writer `01a0f27d-a698-7690-ba93-08b74425bd3f` 与 yocto-engineer `01a0f286-a9e6-7633-9b80-8bfb6d34c782` 的会话均可定位；当前归档状态符合项目由用户控制的单会话串行执行方式，不要求在 project-manager 会话运行时同时激活。
- 关闭或取消依据：2026-10-09 发起人 project-manager 已核对 WORKSPACE、框架校验、60 项测试、SSH 终态和成员会话存在性；用户确认 H-001 已执行完成且 writer 会话存在。期望的工作区边界与可继续执行的成员会话条件均已满足，H-001 关闭。

## H-002：新增 QEMU 成员后的 T-016 分工刷新
- 发起人：框架超级管理员；授权来源为 2026-10-11 用户在本管理员会话新增 QEMU 工程师的明确请求
- 目标：project-manager
- 关联任务：T-016；后续章节依赖及 T-017 按实际影响核对
- 期望结果：在用户后续按串行约定启动项目经理会话时，刷新 TEAM 与两个工程成员配置，调整 T-016 的具体任务分工、输入、完成条件和交接次序，使 qemu-engineer 主责 QEMU 模型代码与正确性，yocto-engineer 主责 patch 集成、BitBake 构建和系统验证；核对相关计划和摘要中的旧口径。保持 D-013 的上游基线与 meta-tiger patch 模型，业务开发另行串行启动。
- 输入与证据：[TEAM.yaml](TEAM.yaml)、[QEMU 成员配置](members/qemu-engineer.yaml)、[Yocto 成员配置](members/yocto-engineer.yaml)、[PROJECT 成员配置及公共定义采用记录](PROJECT.md)、[公共 QEMU 角色](../../roles/qemu-engineer.md)、[公共 QEMU 技能](../../knowledge/qemu-engineering.md)、D-013 与现有 T-016。用户已确认职责分界；新成员的具体任务安排须由项目经理写回任务记录，不把 scope 当作已开始执行的任务。
- 未完成事项：分工刷新已完成，待发起人核对结果并关闭；Yocto 成员开始后续工作前须按 SESSION_PROTOCOL 刷新已调整的 scope。蓝图、工程、正文和评审仍由用户后续串行启动，交接收尾不阻塞 writer 准备蓝图。
- 状态：accepted
- 创建日期：2026-10-11
- 接收反馈：2026-10-11 用户报告管理员已创建 QEMU 工程师并询问下一步；project-manager 刷新协议、TEAM、自身身份与有效技能，实际读取两个工程成员配置、新 QEMU 角色及技能后接收本交接，按既有 T-016 授权安排职责及顺序。
- 处理结果与证据：已更新 [T-016/T-017](TASKS.md)、[A-002 v0.7](artifacts/A-002-book-polishing-plan.md)、[STATUS](STATUS.md) 和 [成果索引](artifacts/README.md)。T-016 明确 writer 蓝图 → qemu-engineer 模型/测试/patch → yocto-engineer 集成/干净构建/系统验证 → writer 稳定稿 → 评审和用户定稿；输入、完成条件及下一位行动人同步更新。T-017 仅补充专业事实核对分工，仍等待 T-016 收口。D-013、工作区基线与验收标准不变，T-016 仍为 todo，未启动其他会话或工程操作。
- 关闭或取消依据：尚未关闭；按 PROJECT_PROTOCOL 由发起人核对结果后关闭，接收方不代替发起人结案。

## H-003：新增系统架构师后的平台设计与 T-016 计划衔接
- 发起人：框架超级管理员；授权来源为 2026-10-11 用户在 project-manager 会话明确要求“按你的建议新增加成员吧”，本轮仅处理有界成员配置维护，原项目经理身份不变
- 目标：project-manager
- 关联任务：T-016；T-017 及后续章节依赖按实际影响核对
- 期望结果：建员配置校验通过后按 SESSION_PROTOCOL 刷新，依据用户采用的增员建议，把平台设计基线及必要的专业核对、用户确认作为章节蓝图和工程实施的输入，登记 architect 的实际任务、输入、完成条件和阶段次序，并同步 A-002、STATUS 和成果索引。保持 D-013 和既定验收范围，区分本章最小平台设计与后续章节能力；方案未确认前不把假设写成实施基线。
- 输入与证据：[TEAM.yaml](TEAM.yaml)、[架构师成员配置](members/architect.yaml)、[PROJECT 成员配置记录](PROJECT.md)、[公共架构师角色](../../roles/architect.md)、[系统设计技能](../../knowledge/system-design.md)、两个工程成员 scope、A-002 v0.7、A-003 v0.1、D-013 和现有 T-016。新成员有效技能为 system-design、qemu-engineering、yocto-engineering、technical-book-validation；配置存在不等于已完成身份初始化或业务方案。
- 未完成事项：任务与计划尚未按新增 architect 刷新，平台设计说明、专业核对和用户确认尚未产生；新成员须在独立成员会话中实际只读初始化后才能开展已登记的工作。本次建员未创建会话，未启动业务设计或工程操作。
- 状态：open
- 创建日期：2026-10-11
- 接收反馈：尚未接收；当前本轮是有界配置维护，不代写后续项目经理计划刷新结果
- 处理结果与证据：已创建并登记 architect 的成员配置，正式任务安排和计划更新待目标成员实际处理
- 关闭或取消依据：尚未关闭

## 记录样式（不是真实交接）

```text
## H-001：交接主题
- 发起人：<member-id；框架管理员的配置请求注明“框架超级管理员”及授权来源>
- 目标：<一个明确的 member-id 或用户>
- 关联任务：<T-编号；管理员配置请求可写无并说明原因>
- 期望结果：<对方需要完成什么>
- 输入与证据：<成果链接、版本和必要上下文>
- 未完成事项：<缺失信息、约束或需决定的问题>
- 状态：open
- 创建日期：<实际日期>
- 接收反馈：尚未接收
- 处理结果与证据：暂无
- 关闭或取消依据：暂无
```

目标成员实际接收后记录 accepted；提供结果后，由发起人核对并关闭。写入交接不会自动启动其他会话，也不会自动改变任务负责人。

需要超级管理员行动时，目标填写用户，期望结果说明请用户转交框架管理员，不使用虚构的项目管理员成员 ID。管理员仅在配置工作授权内发起交接，不借此安排专业业务任务。
