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
