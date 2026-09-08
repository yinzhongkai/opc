# space-rhythm（太空律动）

项目 ID：`space-rhythm`

## 建项信息

- 建立日期：2026-09-07。
- 用户请求来源：本会话用户于 2026-09-07 明确请求“帮我创建 space-rhythm 项目”。
- 建项记录人：框架超级管理员，依据上述建项授权。
- 当前阶段：筹备。

## 目标与约束

- 目标：待用户明确业务目标；本次仅建立项目协作资料，不预设功能或技术方案。
- 范围与非目标：待确认。
- 交付物与验收要求：待确认。
- 时间、资源与其他约束：未设定。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 确认与记录责任

- 默认最终确认人：提出上述建项请求的本会话用户；后续可由用户明确指定其他确认人。
- 协调记录维护人：[project-manager-01](members/project-manager-01.yaml)，按首名项目经理的默认建员约定指定，依据已确认内容整理协调记录。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：由上述项目经理在已授权范围内维护团队计划；其会话尚未运行时，跨成员计划变化仍由用户确认。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享岗位为准；记录整理不授予其他岗位职责。

## 交付约定

- 正式成果位于 `artifacts/`，软件源码或外部资料位置按实际需要另行确定。
- 用户保证当前仓库每次只有一个会话执行，前一会话完成或停止后再启动下一会话；框架不实现文件锁或自动调度。
- 每份文档默认维护一个当前版本，记录版本及批准依据；历史通过用户维护的 Git 追溯。
- 评审与批准要求在任务中明确。专业交叉评审由现有成员在原会话执行，不默认新建评审会话；用户保证受评版本可读取且不变，本轮全部评审者提交意见后作者再统一修订。采用根 [项目运行协议](../../PROJECT_PROTOCOL.md) 的评审、完成与状态规则。
- 默认在相关任务内记录轻量评审，需独立跟踪或正式报告时再拆分；具体已确认的验收要求不能自行降低。

## 成员配置记录

2026-09-07 建项时未指定首批成员，因此建立空团队；没有创建成员文件或启动成员会话。后续建员或调整时记录实际日期、用户授权来源、涉及成员 ID、变更内容及配置路径，此处不重复维护成员当前字段。

### 2026-09-07：新增项目经理

- 授权来源：本会话用户明确请求“帮我在 space-rhythm 项目中加一个项目成员，岗位是项目经理”。
- 配置变更：创建 [project-manager-01](members/project-manager-01.yaml)，并登记到 [TEAM.yaml](TEAM.yaml)。
- 分工依据：本次未进一步限定分工，采用共享项目经理岗位的共同职责作为初始范围；具体分工以成员文件为准，基础知识沿用岗位配置，未添加项目补充知识。
- 责任配置：该成员为首名项目经理，原协调记录维护人待指定，按根项目运行协议指定其承担协调记录维护；默认最终确认人仍为建项用户。
- 记录人：框架超级管理员。本次仅创建成员配置，不代表已创建或初始化成员会话，也不登记未经指派的业务任务。

### 2026-09-07：新增逆向分析方向研发工程师

- 授权来源：本会话用户明确请求“帮我在 space-rhythm 项目中加一个项目成员，岗位是研发工程师，具体内容是做linux & window 系统下可执行文件的逆向分析工作”；配置中将系统名称规范为 Linux 与 Windows。
- 配置变更：创建 [developer-reverse-01](members/developer-reverse-01.yaml)，复用共享 [developer 岗位](../../roles/developer.md)，并登记到 [TEAM.yaml](TEAM.yaml)。具体分工以成员文件为准。
- 知识配置：基础知识沿用 [software-engineering](../../knowledge/software-engineering.md)，新增 [reverse-engineering](../../knowledge/reverse-engineering.md) 并加入本项目 developer 的岗位补充知识，有效集合依次为 software-engineering、reverse-engineering。
- 影响范围：当前仅新增研发成员使用此补充组合；本项目后续同岗位成员也会加载该组合，以各自 scope 区分分工。未修改共享岗位的基础配置、项目经理分工或协调记录维护责任。
- 记录人：框架超级管理员。本次仅建立成员与知识配置，不代表已创建或初始化成员会话、安装工具或开始分析样本；具体样本、分析目标与完成条件由后续实际任务明确，不登记未经指派的业务任务。

### 2026-09-07：新增产品经理

- 授权来源：本会话用户明确请求“帮我在 space-rhythm 项目中加一个项目成员，岗位是产品经理”。
- 配置变更：创建 [product-manager-01](members/product-manager-01.yaml)，复用共享 [product-manager 岗位](../../roles/product-manager.md)，并登记到 [TEAM.yaml](TEAM.yaml)。
- 分工依据：本次未指定细分方向，采用共享产品经理岗位的共同职责作为初始范围；具体分工以成员文件为准，基础知识沿用 [requirements-analysis](../../knowledge/requirements-analysis.md)，未添加项目补充知识。
- 责任配置：协调记录维护人仍为 [project-manager-01](members/project-manager-01.yaml)，默认最终确认人及其他成员分工不变。
- 记录人：框架超级管理员。本次仅创建成员配置，不代表已创建或初始化成员会话，也不登记未经指派的产品任务。

### 2026-09-07：新增系统架构师

- 授权来源：本会话用户明确请求“帮我在 space-rhythm 项目中加一个项目成员，岗位是系统架构师”。
- 配置变更：创建 [architect-01](members/architect-01.yaml)，复用共享 [architect 岗位](../../roles/architect.md)，并登记到 [TEAM.yaml](TEAM.yaml)。
- 分工依据：本次未指定细分方向，采用共享系统架构师岗位的共同职责作为初始范围；具体分工以成员文件为准，基础知识沿用 [system-design](../../knowledge/system-design.md)，未添加项目补充知识。
- 责任配置：协调记录维护人、默认最终确认人及其他成员分工不变。现有交接仍按其原目标处理，新增成员不会自动接收交接或业务任务。
- 记录人：框架超级管理员。本次仅创建成员配置，不代表已创建或初始化成员会话，也不登记未经指派的架构任务。

### 2026-09-08：新增 Windows/Qt 构建工程师

- 授权来源：本会话用户明确请求在 space-rhythm 项目中增加 Windows/Qt 构建工程师，负责 Windows x64 工具链、Qt 源码编译、CMake/Ninja、依赖 ABI 和 CI。
- 配置变更：新增共享 [build-engineer 岗位](../../roles/build-engineer.md)，创建 [build-engineer-windows-qt-01](members/build-engineer-windows-qt-01.yaml)，并登记到 [TEAM.yaml](TEAM.yaml)。该岗位具有独立的工具链、构建、依赖与 CI 职责，未复用会加载逆向分析知识的 developer 岗位。
- 知识配置：基础知识为 [software-engineering](../../knowledge/software-engineering.md)；新增 [windows-qt-build-engineering](../../knowledge/windows-qt-build-engineering.md) 作为本项目 build-engineer 的岗位补充知识，有效集合依次为 software-engineering、windows-qt-build-engineering。
- 分工依据：具体 scope 采用用户明确的 Windows x64、Qt 源码、CMake/Ninja、依赖 ABI 和 CI 范围，并与当前已确认的 Windows x64、Qt 源码构建基线保持一致。编译器路线等未确认事项仍按项目决定流程处理，本次建员不替代确认。
- 影响范围：新共享岗位可供当前克隆的其他项目后续按需使用；项目补充知识当前只影响 space-rhythm 的 build-engineer。现有成员的岗位、知识组合、任务和交接均不改变。
- 记录人：框架超级管理员。本次仅创建成员与知识配置，不代表已创建或初始化成员会话、安装构建工具、开始 Qt 编译或登记未经指派的业务任务。

### 2026-09-08：新增 C++ 核心/系统工程师

- 授权来源：本会话用户明确请求在 space-rhythm 项目中增加 C++ 核心/系统工程师，负责时间模型、事件时间线、撤销重做、事件融合、Worker/IPC 和项目存储骨架。
- 配置变更：新增共享 [core-systems-engineer 岗位](../../roles/core-systems-engineer.md)，创建 [core-systems-engineer-cpp-01](members/core-systems-engineer-cpp-01.yaml)，并登记到 [TEAM.yaml](TEAM.yaml)。该岗位负责核心基础能力的实现与验证，系统架构师继续负责架构契约和关键技术决定。
- 知识配置：基础知识为 [software-engineering](../../knowledge/software-engineering.md)；新增 [cpp-core-systems-engineering](../../knowledge/cpp-core-systems-engineering.md) 作为本项目 core-systems-engineer 的岗位补充知识，有效集合依次为 software-engineering、cpp-core-systems-engineering。
- 分工依据：具体 scope 采用用户明确的六项核心范围，并与现有时间、事件、Worker 和存储架构契约保持一致；C++ 标准版本、IPC 传输与序列化或存储介质等未确认细节仍按项目决定流程处理。
- 影响范围：新共享岗位可供当前克隆的其他项目后续按需使用；项目补充知识当前只影响 space-rhythm 的 core-systems-engineer。现有成员的岗位、知识组合、任务和交接均不改变。
- 记录人：框架超级管理员。本次仅创建成员与知识配置，不代表已创建或初始化成员会话、开始核心模块实现或登记未经指派的业务任务。

### 2026-09-08：新增 FFmpeg 多媒体工程师

- 授权来源：本会话用户明确请求在 space-rhythm 项目中增加 FFmpeg 多媒体工程师，负责媒体探测、解码、PTS/VFR 时间映射、音视频同步、代理文件和导出。
- 配置变更：新增共享 [multimedia-engineer 岗位](../../roles/multimedia-engineer.md)，创建 [multimedia-engineer-ffmpeg-01](members/multimedia-engineer-ffmpeg-01.yaml)，并登记到 [TEAM.yaml](TEAM.yaml)。该岗位负责媒体管线的实现与验证，未复用会加载逆向分析知识的 developer 岗位。
- 知识配置：基础知识为 [software-engineering](../../knowledge/software-engineering.md)；新增 [ffmpeg-media-engineering](../../knowledge/ffmpeg-media-engineering.md) 作为本项目 multimedia-engineer 的岗位补充知识，有效集合依次为 software-engineering、ffmpeg-media-engineering。
- 分工依据：具体 scope 采用用户明确的六项媒体范围，并与现有只读媒体输入、规范时间、VFR、代理、同步及导出事务约束保持一致；具体输入输出格式、编码器、质量、硬件加速和许可证等未确认项仍按项目决定流程处理。
- 影响范围：新共享岗位可供当前克隆的其他项目后续按需使用；项目补充知识当前只影响 space-rhythm 的 multimedia-engineer。现有成员的岗位、知识组合、任务和交接均不改变。
- 记录人：框架超级管理员。本次仅创建成员与知识配置，不代表已创建或初始化成员会话、开始媒体模块实现或登记未经指派的业务任务。

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
