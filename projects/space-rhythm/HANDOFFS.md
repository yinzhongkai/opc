# 行动请求与交接

## H-005：启动 C++/Qt 测试工作流
- 发起人：architect-01
- 目标：tester-cpp-qt-01
- 关联任务：T-020、T-021、T-022
- 期望结果：在自己的项目会话中接收本交接，先把 T-020 更新为 in_progress 并完成测试策略、需求追踪和可复现规则；T-013 测试骨架、T-014/T-017 契约可用且测试框架获确认后执行 T-021；基础实现集成后执行 T-022。每次只按实际状态更新任务，不把计划或等待依赖写成已经完成。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.3](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)；D-003 proposed；T-020～T-022。
- 未完成事项：测试框架仍受 D-003 确认约束；基准硬件、代表素材、性能和产品效果阈值尚未确认；未确认阈值只能报告测量值，不能给出通过结论。
- 状态：open
- 创建日期：2026-09-08
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-004：启动 FFmpeg 多媒体工作流
- 发起人：architect-01
- 目标：multimedia-engineer-ffmpeg-01
- 关联任务：T-017、T-018、T-019
- 期望结果：在自己的项目会话中接收本交接，先把 T-017 更新为 in_progress，交付媒体时间/缓冲契约和黄金样例矩阵；D-003、T-013 统一 x64 构建骨架和核心时间契约就绪后执行 T-018，Qt 集成另等 T-012；核心/worker/媒体基础可集成后执行 T-019。遵守核心拥有规范 `timeNs`、媒体拥有 PTS 解释的单一责任边界。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.3](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-04](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)；D-003/D-006 proposed；T-017～T-019。
- 未完成事项：FFmpeg 具体版本、H.264 后端、发布容器/编码器矩阵和许可证路径尚未确认；不得用原型选择代替发布决定，也不得使用许可不清样例。
- 状态：open
- 创建日期：2026-09-08
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-003：启动 C++ 核心与系统工作流
- 发起人：architect-01
- 目标：core-systems-engineer-cpp-01
- 关联任务：T-014、T-015、T-016
- 期望结果：在自己的项目会话中接收本交接，先把 T-014 更新为 in_progress，交付规范时间、事件、修订与事务 0.x 契约；T-013 纯 C++ 构建骨架和契约就绪后执行 T-015；工程骨架和核心实现可用后执行 T-016。保持 domain 无 Qt Quick/FFmpeg 依赖，不私自确认 D-003/D-006。
- 输入与证据：[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.3](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-02/WP-08](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)；T-014～T-016。
- 未完成事项：D-003 的具体 IPC/存储实现组合仍未确认；C++ 编译验证依赖后续 x64 工具链，但契约、schema 和测试向量可先开展。
- 状态：open
- 创建日期：2026-09-08
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-002：启动 Windows/Qt 构建工作流
- 发起人：architect-01
- 目标：build-engineer-windows-qt-01
- 关联任务：T-011、T-012、T-013
- 期望结果：在自己的项目会话中接收本交接，先把 T-011 更新为 in_progress，只读审计当前 Windows 构建环境并提交 D-006 决策输入；用户确认编译器、Qt 版本和许可证路径后执行 T-012；可复现 Qt SDK 就绪且 D-003 构建组合确认后执行 T-013。不得自行安装工具或把候选路线写成已确认决定。
- 输入与证据：D-002、D-004、D-005 confirmed，D-003/D-006 proposed；[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.3](artifacts/A-005-mvp-technology-stack-proposal.md)、[A-006 0.1 WP-01](artifacts/A-006-domain-work-packages.md)、[A-007 0.1](artifacts/A-007-four-engineer-execution-plan.md)；T-011～T-013。
- 未完成事项：D-006、Qt 确切版本、商业/LGPL 路径和最低 Windows 版本尚未确认；T-011 只产出事实、推荐和最小安装清单，正式下载/安装/构建需在对应输入确认后进行。
- 状态：open
- 创建日期：2026-09-08
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

## H-001：协调已确认产品方向的后续计划
- 发起人：product-manager-01
- 目标：project-manager-01
- 关联任务：T-002
- 期望结果：接收已确认的第一阶段产品方向，依据 D-001 和 A-002 0.2 协调 PROJECT/STATUS 摘要同步，并在现有团队职责和用户授权范围内安排后续技术可行性评估；如缺少适合的技术成员或需要新增成员，向用户说明具体缺口，不由本交接自动创建成员或指派实现。
- 输入与证据：[A-002 0.2：音画双向节奏创作工具产品需求](artifacts/A-002-audio-visual-rhythm-product-brief.md)，状态 approved；D-001，状态 confirmed；逆向输入 A-001 0.1 与 A-003 0.1。
- 未完成事项：典型素材与格式边界、视觉默认风格、本地/云端边界和“卡点自然”评分口径仍需在后续任务细化；技术可行性与工作量尚未评估。
- 状态：open
- 创建日期：2026-09-07
- 接收反馈：尚未接收。
- 处理结果与证据：暂无。
- 关闭或取消依据：暂无。

已在任务内明确安排的轻量评审或复核无需重复建立交接；另有补充输入、协调或责任转交请求时再登记。

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
