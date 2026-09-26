# 项目任务

状态与登记权限见根 [项目运行协议](../../PROJECT_PROTOCOL.md)。

## T-001：起草首本书定位与全书目录草案
- 负责人：planner
- 状态：completed
- 授权来源与日期：用户于 2026-09-12 在 planner 会话直接指示“我们开始起草本书吧”
- 目标与范围：形成首本书（暂定《nRF54L15 与 Zephyr RTOS 实战：从芯片外设到低功耗蓝牙产品》）的图书设计与全书目录两份草案，落在 `artifacts/book/design.md` 与 `artifacts/book/outline.md`；不含章节蓝图、正文撰写与环境基线确认
- 输入与依赖：[PROJECT.md](PROJECT.md) 已确认目标与暂定书名；`nRF54L15_DK_资料/` 官方资料包（用户 2026-09-12 下载）；根图书成果模板 `templates/book/`
- 优先级：未设定
- 完成条件与确认方式：两份草案成文并登记成果索引后提交用户审阅；读者定位与目录经用户确认后登记 DECISIONS，草案在此之前保持 draft
- 进展：2026-09-12 任务登记并开始起草；同日完成两份草案并提交用户审阅
- 成果与验证证据：[design.md](artifacts/book/design.md)（A-001 v0.1）、[outline.md](artifacts/book/outline.md)（A-002 v0.1），均已登记成果索引
- 阻塞与下一位行动人：无；下一位行动人为用户（审阅两份草案并答复 design.md 中的开放问题）
- 更新日期：2026-09-12

## T-002：按用户确认修订图书设计与目录至 v0.2 并登记决定
- 负责人：planner
- 状态：completed
- 授权来源与日期：用户于 2026-09-13 在 planner 会话逐条确认 T-001 草案的全部开放问题
- 目标与范围：修订 A-001、A-002 至 v0.2 并按确认标记批准依据；登记决定 D-001～D-005；更新成果索引；不含章节蓝图编写
- 输入与依赖：T-001 成果 v0.1；用户 2026-09-13 确认记录；DK 硬件用户指南 v1.0.0、芯片数据手册 v1.0、Nordic 官方发布信息与 Zephyr 官方文档核实结果
- 优先级：未设定
- 完成条件与确认方式：两份成果更新至 v0.2 并登记批准决定；索引一致；框架校验通过
- 进展：2026-09-13 任务登记并当日完成
- 成果与验证证据：[design.md](artifacts/book/design.md)（A-001 v0.2，approved）、[outline.md](artifacts/book/outline.md)（A-002 v0.2，approved）；决定 D-001～D-005 见 [DECISIONS.md](DECISIONS.md)
- 阻塞与下一位行动人：无；下一位行动人为用户（决定是否进入第 1 章 ch-env-setup 蓝图阶段）
- 更新日期：2026-09-13

## T-003：起草第 1 章 ch-env-setup 章节蓝图
- 负责人：planner
- 状态：completed
- 授权来源与日期：用户于 2026-09-13 在 project-manager 会话指示“进入 ch-env-setup 蓝图阶段”，由 project-manager 按项目计划登记安排
- 目标与范围：为第 1 章 ch-env-setup（认识 nRF54L15 DK；搭建 NCS/Zephyr 开发环境；构建并烧录第一个程序上板运行）起草章节蓝图：复制 `templates/book/chapters/ch-001/plan.md` 至 `artifacts/book/chapters/ch-env-setup/plan.md` 并填写实际内容，含学习目标与前提、内容蓝图、示例/实验设计、易错点与交付条件；不含正文草稿、示例工程实现与 environment.md 环境基线确认
- 输入与依赖：A-001 v0.2 图书设计（approved）；A-002 v0.2 全书目录（approved，ch-env-setup 为建议的首个闭环章，前置 ch-intro）；D-001～D-005（读者画像、内容边界、目录、物料清单、验证基线与节奏）；`nRF54L15_DK_资料/` 官方资料包；根图书成果模板 `templates/book/`
- 优先级：未设定
- 完成条件与确认方式：蓝图成文并登记成果索引后提交用户审阅；检查重点为学习目标明确、前置依赖可获得、示例和验证要求有着落；经用户确认前蓝图保持 draft
- 进展：2026-09-13 planner 会话受理本任务并完成蓝图起草，登记成果索引后提交用户审阅
- 成果与验证证据：[plan.md](artifacts/book/chapters/ch-env-setup/plan.md)（A-003 v0.1，draft）
- 阻塞与下一位行动人：无阻塞；下一位行动人为用户（审阅蓝图）。关注项：environment.md 环境基线尚未确认（developer 职责，待立项），蓝图按 D-005 候选基线 NCS v3.4.0 起草并在文中标注
- 更新日期：2026-09-13

## T-004：起草第 1 章 ch-env-setup 正文草稿
- 负责人：writer
- 状态：completed
- 授权来源与日期：用户于 2026-09-19 在 project-manager 会话确认登记（“登记吧”），由 project-manager 按项目计划安排；2026-09-20 用户在 writer 会话指示“受理 T-004”
- 目标与范围：按已批准的 A-003 v0.1 蓝图起草第 1 章正文：复制 `templates/book/chapters/ch-001/text.md` 至 `artifacts/book/chapters/ch-env-setup/text.md` 并撰写草稿，覆盖蓝图约定的学习目标、内容结构与示例说明；不含示例工程实现（developer 职责）、不含审校与试读安排
- 输入与依赖：A-003 v0.1 蓝图（approved，D-006）；A-001 v0.2、A-002 v0.2；D-001～D-006；`nRF54L15_DK_资料/` 官方资料包；环境基线以 T-005 确认的 environment.md 为准——基线未确认前按 D-005 候选基线 NCS v3.4.0 撰写并标注待核位置
- 优先级：未设定
- 完成条件与确认方式：草稿成文并登记成果索引后提交用户审阅；是否安排任务内评审在受理时按蓝图交付条件明确；经用户确认前保持 draft
- 进展：2026-09-20 writer 会话受理并完成草稿 v0.1：按 A-003 v0.1 蓝图撰写，覆盖全部学习目标与 4 个实验（现象均标“预期”），按 D-005 候选基线 NCS v3.4.0 撰写并以【待核】标注待核位置；板级目标、blinky/hello_world 命令与烧录方式经 Zephyr 官方板级文档在线核实（2026-09-20）；发现 `nRF54L15_DK_资料/` 不在当前仓库克隆中，硬件事实取自已批准成果记录的核查结论并在正文“读者可见的限制”第 6 条说明。受理时明确：本任务完成条件为草稿成文并提交用户审阅，草稿阶段不安排任务内交叉评审；审校与试读安排按蓝图交付条件另行登记
- 成果与验证证据：[text.md](artifacts/book/chapters/ch-env-setup/text.md)（A-004 v0.1，draft），已登记成果索引
- 阻塞与下一位行动人：无阻塞；下一位行动人为用户（审阅 A-004 v0.1 草稿；欢迎顺带反馈是否开始实机照做）
- 更新日期：2026-09-20

## T-005：确认 environment.md 开发环境基线
- 负责人：developer
- 状态：completed
- 授权来源与日期：用户于 2026-09-19 在 project-manager 会话确认登记（“登记吧”），由 project-manager 按项目计划安排
- 目标与范围：复制 `templates/book/environment.md` 至 `artifacts/book/environment.md`，确认并记录开发环境基线：Windows 主机从零安装路径、NCS 版本（候选 v3.4.0，基于 Zephyr 4.4）、工具链与烧录工具版本、DK 1.0.0（Rev 2 芯片）板卡与固件版本；并安排 D-004 电池 5 V 升压路径的实测验证；不含第 1 章示例工程开发
- 输入与依赖：D-005（验证基线、开发环境与节奏）、D-004（物料清单与电池验证项）、D-001（读者画像）；`nRF54L15_DK_资料/` 官方资料包；Nordic 与 Zephyr 官方发布信息
- 优先级：未设定
- 完成条件与确认方式：environment.md 成文、登记成果索引并经用户确认；确认前保持 draft
- 进展：2026-09-20 developer 会话受理，完成官方信息核查并起草 environment.md v0.1（draft），登记成果索引，提交用户审阅。核查发现并处理两处事实：Toolchain Manager 已弃用、不支持 NCS v3.0.0+，安装路径须走 nRF Connect for VS Code 扩展包；任务输入中的 `nRF54L15_DK_资料/` 官方资料包当前不在本仓库内，已在文中标注缺口并请用户确认位置。本成果初登记为 A-004，与 T-004 章节正文草稿（A-004，writer 会话同日提交）撞号，改登记为 A-005。2026-09-23 用户将资料包放入项目工作区，应用户要求目录定名 `workspace/nrf54l15-dk-docs/`（project-manager 抽查：数据手册 940 页、DK 用户指南 31 页、Rev 2 勘误表 27 页均可读；与本书无关的 Altium/Gerber 硬件设计包 zip、ngl_001 硬件设计指南与 PCA10156 原理图保留在本地但不纳入 Git），资料包位置缺口解除，environment.md 中相应缺口标注待 developer 修订时核销。2026-09-23 developer 按用户三条反馈修订为 v0.2：① 核销"官方资料包缺口"一节、改按 `workspace/nrf54l15-dk-docs/` 新路径引用并复核原文引用（抽查四份 PDF 页数与版本标识均与记载一致：数据手册 v1.0 共 940 页、DK 用户指南 v1.0.0 共 31 页、Rev 2 勘误表 v1.1 共 27 页、nan_047 量产烧录指南共 20 页）；② 安装路径与验证步骤保持"未执行/待回填"标注不变；③ 新增"IDE 路径评估"节——"编译器工具链 + Source Insight"经官方 `nrfutil sdk-manager` 与 `west` 命令行文档核查可行，构建烧录与扩展路径等价，调试改用 `west debug`（J-Link GDB），建议本书主线仍按 VS Code 扩展路径撰写，最终选择待用户决定。v0.2 再次提交用户确认。2026-09-23 用户与 developer 讨论主机方案（Windows vs Linux，含用户日常"Linux 编译 + Samba + Source Insight"工作流的适用性）后确认：本书主线为 Windows 主机 + VS Code + nRF Connect 扩展，Source Insight 作个人编辑器附注；developer 据此修订为 v0.3 并提交用户最终确认。2026-09-23 用户回复"确认"：developer 登记 D-007（批准 A-005 v0.3，含主机与 IDE 主线定案），成果与索引写回 approved，本任务完成
- 成果与验证证据：[environment.md](artifacts/book/environment.md)（A-005 v0.3，approved，批准依据 D-007）；核实来源：Nordic 官方博客 LTS 公告（2026-07-02）、Nordic 官方 nRF Connect for Desktop 下载页、Nordic DevZone，均 2026-09-20 核查；`workspace/nrf54l15-dk-docs/` 资料包原文与 `nrfutil sdk-manager`/`west` 官方命令行文档（2026-09-23 核查）
- 阻塞与下一位行动人：无阻塞；2026-09-23 用户审阅 v0.1 反馈三条（记录人 project-manager）：① "官方资料包缺口"一节已过时且路径有误——资料包已于当日入库 `workspace/nrf54l15-dk-docs/`（旧名 `nRF54L15_DK_资料/` 不再使用），修订时核销缺口并按新路径复核原文引用；② 用户尚未在主机实测安装，安装路径与验证步骤须保持"未执行/待回填"标注，不得以资料核查结论冒充实测结果；③ 用户询问 IDE 能否改用"编译器工具链 + Source Insight"方式（不用 VS Code + nRF Connect 扩展）——developer 评估可行性并修订 IDE 与安装路径条目：需覆盖 SDK 命令行安装途径（`nrfutil sdk-manager` 备选）、west 命令行构建烧录、调试手段变化，以及对第 1 章正文（A-004 按 VS Code 路径撰写）的影响。三条均已在 v0.2 处理（详见上文进展）；④ 2026-09-23 主机与 IDE 主线经讨论定案（Windows 主机 + VS Code + nRF Connect 扩展），已写入 v0.3；用户同日确认 v0.3，D-007 登记、A-005 转 approved，任务关闭。下一位行动人：无；后续事项——A-004 正文【待核】位置可依 A-005 v0.3 核销（writer 职责，另行安排）；安装与实测回填项在用户实际执行后按 A-005 约定回填
- 更新日期：2026-09-23

## T-006：修订第 1 章正文、技术复核与用户通读反馈闭环
- 负责人：writer
- 状态：in_review
- 授权来源与日期：用户于 2026-09-26 在 project-manager 会话明确要求登记第 1 章正文修订任务，由 writer 按 A-005 v0.3 修正安装流程及过时说明，安排 developer 对修订稿技术复核，形成可读稿后交用户完整通读并按反馈修订；由 project-manager 据此登记。
- 目标与范围：在 A-004 v0.1 上修订唯一当前正文，覆盖全章涉及的安装顺序、工具职责、工具链终端入口、构建烧录衔接、相关图示/截图说明与来源；按 A-005 v0.3 和 D-007 对齐 Windows + VS Code + nRF Connect 扩展主线，替换 Toolchain Manager 旧步骤。更新资料包路径并核销已解决的“资料包缺失”“基线未确认”说明，逐项区分可核销与仍需实测的【待核】项；完成技术复核、用户通读及反馈修订。本任务不含独立示例工程开发、替用户上板实验或全章最终定稿批准，也不授权 writer 改写 planner/developer 的成果。
- 输入与依赖：[A-004 v0.1 正文](artifacts/book/chapters/ch-env-setup/text.md)（draft）；[A-005 v0.3 环境基线](artifacts/book/environment.md)（approved，D-007）；[A-003 v0.1 蓝图](artifacts/book/chapters/ch-env-setup/plan.md)（approved，D-006）；[A-001 v0.2 图书设计](artifacts/book/design.md)、[A-002 v0.2 目录](artifacts/book/outline.md)；D-001～D-008；[官方资料包](workspace/nrf54l15-dk-docs/README.md)。A-001/A-003 的旧安装表述按 D-007 和 A-005 v0.3 对齐；D-007 对旧正文“无需改动”的描述不替代实际正文核对。发现基线自身疑点时记录证据交 developer/project-manager 协调，不以获批代替技术核查，不自行变更已确认基线。
- 优先级：本章可读稿交付的前置工作；未设日期期限。
- 完成条件与确认方式：① 修订稿与成果索引版本、关联任务及状态一致，记录改动、来源与剩余限制；② developer 对明确版本完成下述技术复核，问题经 writer 处理并由 developer 对受影响内容复核，无未解决的主线阻断问题；③ 形成明确版本的可读稿并交用户完整通读，记录用户实际通读完成及反馈（无意见也须实际确认）；④ writer 逐项回复并修订，涉及技术内容的改动由 developer 再复核，用户确认通读反馈已处理；全部满足后方可 completed。可读稿交付或本任务完成均不自动把 A-004 标为 approved；实机验证与章节定稿仍按原约定另行满足。
- 进展：2026-09-26 project-manager 登记任务及技术复核安排；同日 writer 完成 A-004 v0.2、v0.3 的技术问题修订，developer 对 v0.3 的必要复核结论为 `pass`。writer 按 D-008 交用户通读后，依据 F-001 完成全章书稿化编辑并形成 v0.4。用户继续通读时明确要求 F-002“先直接使用截图”；writer 已将官方用户指南 Figure 2 截图纳入正文，按图中标注修正 LED 与 J3 USB-C 接口事实，形成 A-004 v0.5，现提交 developer 做限定技术复核。用户通读在复核完成前暂停。
- 成果与验证证据：[A-004 v0.5](artifacts/book/chapters/ch-env-setup/text.md)（in_review，尚未批准），输入为 A-004 v0.4、F-002、Nordic《nRF54L15 DK Hardware User Guide v1.0.0》Figure 2 及既有依赖；正式图片位于 `artifacts/book/chapters/ch-env-setup/images/figure-1-1-nrf54l15-dk-front.png`，成果索引已同步为 v0.5。v0.5 规范化 SHA-256、处理范围、版权边界及复核安排见下方记录。
- 阻塞与下一位行动人：无输入阻塞；下一位行动人为 developer，对 A-004 v0.5 的 Figure 2 图片引用、LED 0–3 与 J3 USB-C 接口表述及直接影响处做限定技术复核。复核通过前不重新交用户完整通读。安装、构建、设备枚举、烧录与上板仍未执行；截图用于当前内部稿件不表示已取得正式出版转载许可，出版前须确认许可或替换为自有实物照片；A-005 v0.3 的差异继续交 developer/project-manager 协调。
- 更新日期：2026-09-26

### writer 修订与自查记录（2026-09-26，A-004 v0.2）
- 刷新范围：重读根规则与协议、TEAM 自身登记、writer 成员文件、岗位、全部四项有效知识；重读项目入口、PROJECT、STATUS、成果索引、TASKS、DECISIONS（D-001～D-008）、HANDOFFS；完整读取 A-001 v0.2、A-002 v0.2、A-003 v0.1、A-004 v0.1、A-005 v0.3 和资料包 README。身份、scope、知识集合未变；无未结交接。资料包目录已核对，未重新读取 PDF 原文或 Git 历史正文。
- 作者自查：全文检查学习目标、工具职责、安装顺序、终端入口、工作目录、构建目录与后续改参/串口步骤；旧安装流程不再作为操作指引。修正仅有“三条 west 命令”的旧概述；图 1-3 改为文本示意，三处截图占位说明同步新流程。来源与实验现象继续区分资料核查、预期和用户实测。本次为作者自查，不是 developer 独立技术复核。
- 已核销：① “环境基线尚未确认”——依据 D-007、A-005 v0.3；② “官方资料包不在当前克隆”——依据项目 workspace/nrf54l15-dk-docs/ 索引与实际文件清单；不据此宣称重新核对了全部硬件事实。
- 保留待核：实际 Windows/SDK/工具链/扩展版本、安装目录/耗时/体积、界面入口、USB 口与调试器固件、烧录日志、LED/串口现象、图 1-1/1-2 与实机截图；四项实验均未执行。未安装 SDK、未构建或烧录、未代填任何用户结果。
- W-001（作者提出，非 developer 结论）：正文 1.3.3、1.4 与 A-005 的烧录依赖需复核。2026-09-26 查阅 [Nordic 安装文档](https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/installation/install_ncs.html)（当前 3.4.99）列独立 J-Link 前置条件及扩展自带部分 nRF Util 命令；[Zephyr 板级文档](https://docs.zephyrproject.org/latest/boards/nordic/nrf54l15dk/doc/index.html) 的 runner 表列 nrfutil 为默认烧录工具。A-005 v0.3 则列 nRF Command Line Tools / nrfjprog 及其随附驱动。请 developer 核实 v3.4.0 下的安装组合、nrfutil 可调用条件和检查步骤，必要时协调基线修订；writer 未擅改 A-005 或固定版本。
- W-002（作者提出，非 developer 结论）：A-005 v0.3 的 `west sdk-version` 尚无本轮确认的命令依据；正文 1.3.4 明示待技术复核，未将其作为成功判据。请 developer 核对命令有效性、输出含义和精确工具版本的检查方式；本轮未检索到充分依据不等于已证明命令不存在。
- 版本适用性：本轮在线核查用于修正安装和终端入口；NCS latest 页面当前为 3.4.99，固定 3.4.0 安装页本轮未能读取。未把在线最新版推荐版本或命令替代为项目基线；v3.4.0 的命令行为继续交技术复核和用户实机验证。
- 校验：`python scripts/validate_framework.py` 通过；`git diff --check` 无差异格式错误。结构与链接检查不证明技术正确性。修改范围为 A-004 正文、本人成果索引条目及 T-006；T-004 保持 completed，D-008 的用户通读尚未发生。

### 第 1 轮技术复核安排：A-004 v0.2（已提交）
- 安排来源与日期：用户 2026-09-26 明确要求 developer 对修订稿做技术复核；project-manager 在 T-006 内登记。
- 受评正文：[A-004 正文](artifacts/book/chapters/ch-env-setup/text.md)；实际提交版本 **v0.2**，提交人 writer，日期 **2026-09-26**，状态 in_review。内容摘要：VS Code 扩展安装 SDK、扩展工具链终端、工作目录与构建烧录衔接、新资料包路径和已批准基线、未实测与待技术复核项。
- 内容校验：将正文按 UTF-8 解码、CRLF/CR 统一为 LF，保留其他字符及末尾换行，再编码为 UTF-8 计算 SHA-256：`93d7b704f466467ee1dbc3e66975b17f6080bae12763ca57ffbf2064bff5cb44`。本次未创建 Git 提交；developer 开始前核对版本与该摘要，不把不同内容视为同一受评版本。自此提交起，本轮意见全部提交前 writer 保持正文不变。
- 参与成员与范围：developer；检查安装顺序与软件依赖、终端环境入口、板级目标与命令、构建烧录衔接、资料路径及来源、A-005 v0.3 一致性，以及“资料核查/未执行/用户实测”的证据边界；检查主线可操作性与已知技术阻断项。不代替用户通读、实机运行或最终批准。
- 完成条件：developer 在本任务署名记录日期、受评版本、依据、未覆盖项与 pass/revise/blocked 结论；问题使用本轮唯一编号 R1-001 等，含位置、影响、建议与原稿责任人 writer。证据缺失须明示，不凭资料核查声称构建或上板通过。
- 修订与后续复核：本轮仅 developer 一名评审者，其意见全部提交后 writer 才统一修订。writer 逐项填写处理回复；developer 按本次授权复核技术问题及后续用户反馈引起的技术改动，逐轮追加明确版本、范围、结果和证据，保留旧轮记录，每轮复核期间正文保持不变。发现超出本任务范围的争议交 project-manager 或用户决定。
- 本轮进度与下一位行动人：developer 已于 2026-09-26 完成第 1 轮技术复核，结论为 `revise`，本轮全部意见已提交；下一位行动人为 writer，统一处理 R1-001、R1-002 并提交明确修订版本。问题处理及 developer 必要复核完成前，不交用户完整通读。

#### developer 的意见
- 日期、成员、受评版本与内容校验：2026-09-26，`developer`；A-004 v0.2（in_review，writer 于 2026-09-26 提交）。开始复核前重新计算正文 SHA-256 为 `93d7b704f466467ee1dbc3e66975b17f6080bae12763ca57ffbf2064bff5cb44`，与本任务登记值一致；本轮意见仅适用于该内容。
- 评审范围与依据：按 A-005 v0.3、D-007、D-008 和本任务安排，检查 1.2～1.5 节的安装顺序、软件依赖、nRF Connect 工具链终端入口、`nrf54l15dk/nrf54l15/cpuapp` 板级目标、blinky/hello_world 构建与 `west flash -d` 衔接、来源，以及“资料核查/预期/未执行/用户实测”的边界。主要依据为固定版本的 [NCS v3.4.0 安装文档](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/installation/install_ncs.rst)、[NCS v3.4.0 编程文档](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/app_dev/programming.rst)、[NCS v3.4.0 工具要求](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/installation/recommended_versions.rst)、[NCS v3.4.0 发布说明](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/releases_and_maturity/releases/release-notes-3.4.0.rst)、[NCS v3.4.0 的 west manifest](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/west.yml)，以及其锁定的 [Zephyr ncs-v3.4.0 板级文档](https://github.com/nrfconnect/sdk-zephyr/blob/ncs-v3.4.0/boards/nordic/nrf54l15dk/doc/index.rst)、[`board.cmake`](https://github.com/nrfconnect/sdk-zephyr/blob/ncs-v3.4.0/boards/nordic/nrf54l15dk/board.cmake) 和 [`SDK_VERSION`](https://github.com/nrfconnect/sdk-zephyr/blob/ncs-v3.4.0/SDK_VERSION)。
- 已确认项：VS Code 扩展的 Install SDK 主线、扩展配置的终端、SDK 工作区根目录、板级目标和示例路径关系成立；`west build ... -d C:\ncs\build\...` 与随后 `west flash -d` 指向同一构建目录，衔接正确。NCS v3.4.0 的 manifest 将 Zephyr 锁定为 `ncs-v3.4.0`，其板级文件先载入 `nrfutil` runner、再载入 `jlink` runner，故默认 runner 为 `nrfutil`。正文已把所有安装、构建、烧录、LED/串口现象标为预期、未执行或待用户回填，没有冒充实测。
- 未覆盖项：本机未安装 NCS v3.4.0，未执行扩展安装、终端命令、构建、设备枚举、烧录、串口或上板观察；未确认安装时的实际扩展 UI、路径、磁盘占用、J-Link/nRF Util 具体安装版本和输出措辞；未重新逐页核查硬件 PDF、绘制图片或检查用户可读性。本文档复核不构成用户通读、实机验证、独立测试或成果批准。
- 结论：`revise`。板级目标、构建/烧录目录衔接、来源与未实测标记可以保留；R1-001、R1-002 会直接影响读者判断环境是否具备烧录条件及如何核对版本，须由 writer 统一修订后交 developer 复核。当前证据足以给出修改办法，因此不是 `blocked`。

##### R1-001：明确默认烧录栈，移除 nRF Command Line Tools 的主线依赖歧义
- 对应作者问题：W-001。
- 位置：1.2 开发环境组成与工具链说明；1.3.3“准备调试与烧录工具”；1.4.1 的烧录前提与预期；“来源与延伸阅读”的工具依赖说明。
- 依据与判断：NCS v3.4.0 固定版本文档说明，自 NCS v3.0.0 起 Nordic 板卡的 `west flash` 默认 runner 为 nRF Util；自 v3.1.0 起 `nrfutil device` 随 NCS 工具链 bundle 提供。固定版本安装前提仍要求独立安装匹配的 SEGGER J-Link 软件，Windows 还列出 J-Link USB Driver；nRF54L15 DK 的 `board.cmake` 以 `nrfutil` 为默认、`jlink` 为备选。`nRF Command Line Tools/nrfjprog` 仅在显式执行 `west flash -r nrfjprog` 时需要，Programmer 是可选图形界面，不是默认 CLI 烧录链的前置条件。A-005 v0.3 将 nRF Command Line Tools/nrfjprog 写入主线，已与固定版本资料不一致，需另行协调基线后续修订；A-004 本轮应先把当前可操作路径说明准确。
- 影响：正文当前把 A-005 的旧工具项、J-Link、Programmer 和 nrfutil 并列为“尚待澄清”，读者无法据此完成主线前置检查，也可能误装已归档的 nRF Command Line Tools，或错误地把 Programmer 可用等同于 `west flash` 默认后端可用。
- 修改建议：原稿责任人 writer。将主线明确为“安装 NCS v3.4.0 SDK+匹配工具链（其中已含锁定的 nRF Util 及 `device` 命令）→ 安装固定版本要求的 SEGGER J-Link 软件/Windows 驱动 → 在工具链终端检查 `nrfutil --version`、`nrfutil device --version` → 连接 DK 后执行 `nrfutil device list` → 构建完成后执行 `west flash -d <构建目录> --context` 核对 available/default runner，再执行 `west flash -d <构建目录>`”。将 Programmer 标为可选图形工具；把 nRF Command Line Tools/nrfjprog 移到“显式选择 `-r nrfjprog` 才需要”的备选说明。J-Link 的确切安装版本与以上输出继续标为用户实机回填，不写成已通过。
- 作者处理回复：writer 于 2026-09-26 已在 **A-004 v0.3** 处理。修改位置：正文元信息与版本记录；1.2 的环境组成图、工具链、默认烧录链、Programmer 与 nrfjprog 说明；1.3 的主线顺序及 1.3.3；1.3.4 的 nRF Util/device 版本与设备枚举；1.4.1 的构建日志、设备复核和 `west flash -d ... --context`；“读者可见的限制”第 2、7 条及“来源与延伸阅读”。来源为本问题列出的 NCS v3.4.0 固定版本安装/编程文档、Zephyr `board.cmake` 和 developer 复核结论。A-005 v0.3 保持 approved 原文不动；其把 Programmer、nRF Command Line Tools/nrfjprog 与 J-Link 驱动写入主线的差异继续保留在本问题与正文协调说明中，交 developer/project-manager 安排 A-005 后续修订，A-004 先采用固定版本可操作路径。仍未实测：J-Link、nRF Util 和 `device` 的实际版本，`nrfutil device list` 输出，构建目录 runner context、默认 runner、烧录日志及板上现象；未写成通过。
- 复核：2026-09-26，`developer` 对规范化 SHA-256 为 `4240ef1304bbfe64d62ad5a5626be4bb693b07c11b0b68c008348ddf292a67cf` 的 A-004 v0.3 完成必要复核，结果 `pass`，R1-001 关闭。正文已把默认链写为工具链捆绑的 nRF Util / `device` + 独立 J-Link 软件与 Windows 驱动，将 Programmer 标为可选图形工具、nRF Command Line Tools / nrfjprog 限定为显式 `west flash -r nrfjprog` 的备选；已补充 `nrfutil --version`、`nrfutil device --version`、`nrfutil device list`、构建目录 `west flash -d ... --context`、异常时保存输出并停止烧录的分支。A-005 v0.3 的差异继续在正文和本问题中明示并交 developer/project-manager 协调，未静默改写已批准成果。J-Link、nRF Util / `device` 的实际版本、设备枚举、runner context、烧录日志及上板现象仍未实测，不属于本次文档复核的通过声明。

##### R1-002：删除 `west sdk-version`，按不同对象分别核对版本
- 对应作者问题：W-002。
- 位置：1.2 的工具链版本说明；1.3.4 第 3 步；“读者可见的限制”第 7 条；来源说明；同时影响 A-005 v0.3 中原 `west sdk-version` 检查项。
- 依据与判断：Zephyr 的 west 内置命令清单没有 `sdk-version`，NCS v3.4.0 的 west manifest 只从 Zephyr/NCS 项目导入已声明的扩展命令，本轮亦未在固定版本官方资料中找到该命令；不能把 `west sdk-version` 作为可用命令或保留为读者待试项。NCS v3.4.0 发布说明给出的源码身份是 manifest 仓库标签 `v3.4.0`；该版本锁定的 Zephyr `SDK_VERSION` 文件内容为 `1.0.1`，发布说明确认工具链基于 Zephyr SDK v1.0.1。`west --version` 只检查 west 本身，不能替代 SDK、编译器或烧录工具版本。
- 影响：若读者照 A-005 的原检查项执行，会遇到未知命令或误解输出；仅记录 VS Code 选择项和 `west --version` 也不足以证明源码、Zephyr SDK、GCC 与 nRF Util 属于同一基线。
- 修改建议：原稿责任人 writer。删除 `west sdk-version` 待核说法，改为分层记录：① 在 SDK 工作区根目录执行 `git -C nrf describe --tags --exact-match HEAD`，期望源码 manifest 标签为 `v3.4.0`；② `Get-Content zephyr\SDK_VERSION`，期望固定版本文件为 `1.0.1`；③ 在扩展配置的工具链终端执行 `arm-zephyr-eabi-gcc --version`，并结合首次 `west build` 的 CMake“Found toolchain”日志记录实际编译器路径与版本，目标 GCC 为 14.3.0；④ 另行记录 `west --version`、`nrfutil --version` 和 `nrfutil device --version`，不要把任何单条输出扩张为整套环境已匹配。所有期望值标明资料核查级，实际输出仍由用户安装后回填；若 Git 包不含标签或命令输出不同，保存路径和完整输出交技术核查。
- 作者处理回复：writer 于 2026-09-26 已在 **A-004 v0.3** 处理。修改位置：1.2 的工具链版本职责；1.3.4 第 2～5 步的命令、资料期望值表、实际输出记录与失败分支；1.4.1 的首次构建 `Found toolchain` 记录；“读者可见的限制”第 2、7 条及“来源与延伸阅读”。删除可执行步骤中的 `west sdk-version` 待核说法，改为分别执行 `git -C nrf describe --tags --exact-match HEAD`、`Get-Content zephyr\SDK_VERSION`、`arm-zephyr-eabi-gcc --version`、`west --version`、`nrfutil --version`、`nrfutil device --version`；资料期望值单列为 manifest `v3.4.0`、Zephyr SDK `1.0.1`、GCC `14.3.0`，用户实际输出另行保存，Git 标签缺失或结果不符时保留路径、提交号与完整输出交技术核查。来源为本问题列出的 v3.4.0 发布说明、west manifest、Zephyr `SDK_VERSION`、固定版工具要求和 developer 复核结论。A-005 v0.3 的原检查项不由 writer 改写，差异交 developer/project-manager 协调。上述命令及构建日志均未在用户主机执行，west、GCC、nRF Util、device 的实际版本仍待回填。
- 复核：2026-09-26，`developer` 对同一 A-004 v0.3 完成必要复核，结果 `pass`，R1-002 关闭。正文已删除 `west sdk-version` 的可执行步骤和待试判据，分别检查 manifest 精确标签、`zephyr\SDK_VERSION`、GCC、west、nRF Util 与 `device`，把 `v3.4.0`、`1.0.1`、`14.3.0` 标为资料期望值，把用户实际输出独立保存；Git 标签缺失或版本不符时追加提交号、保留路径与完整输出并交技术核查。A-005 v0.3 的原检查项只登记差异并交协调，未由 writer 改写。上述命令和首次构建 `Found toolchain` 日志均未在用户主机执行，本次复核不声称环境版本已经实测匹配。

### writer 第 1 轮问题统一修订与自查（2026-09-26，A-004 v0.3）
- 会话刷新：按 SESSION_PROTOCOL 完整重读根/项目规则、writer 身份与岗位、四项有效知识，刷新 STATUS、成果索引、TASKS、DECISIONS、HANDOFFS，并完整读取 A-001 v0.2、A-002 v0.2、A-003 v0.1、A-004 v0.2、A-005 v0.3 和资料包索引。writer 身份、scope 与知识集合未变，无待接收交接；开始修订前确认 A-004 v0.2 规范化 SHA-256 为 `93d7b704f466467ee1dbc3e66975b17f6080bae12763ca57ffbf2064bff5cb44`，与 developer 受评记录一致。
- 修订范围与来源：仅修改 writer 负责的 A-004、其成果索引条目及 T-006 作者记录；未修改 A-005 或 A-001/A-003。R1-001 依据 NCS v3.4.0 固定版安装/编程文档和 nRF54L15 DK `board.cmake`；R1-002 依据 v3.4.0 发布说明、west manifest、Zephyr `SDK_VERSION`、固定版工具要求及 developer 评审记录。A-005 v0.3 的差异按两条作者处理回复交 developer/project-manager 协调，不以本次正文修订替代其后续版本流程。
- 作者自查：逐项对照 R1-001、R1-002 检查 1.2～1.4、限制与来源；确认默认链、可选/备选工具、设备枚举、构建目录 runner context、分层版本命令和失败分支均写入正文，`west sdk-version` 不再作为可执行命令或待试判据；成果元信息与索引均为 v0.3 / in_review。`python scripts/validate_framework.py` 通过，`git diff --check` 无差异格式错误；结构与静态检查不证明命令已运行或硬件已通过。
- 仍未实测边界：未安装 NCS v3.4.0、VS Code 扩展、J-Link 或任何烧录工具；未执行文中版本命令、`nrfutil device list`、构建、`west flash --context`、烧录、串口或上板观察；实际 UI、路径、版本、输出、日志、设备标识、LED/串口现象和截图仍待用户回填。未代填安装、构建、烧录或上板结果。

### 第 1 轮问题修订后的必要复核：A-004 v0.3（已提交）
- 受评正文：[A-004 正文](artifacts/book/chapters/ch-env-setup/text.md)；版本 **v0.3**，提交人 writer，日期 **2026-09-26**，状态 in_review。复核范围限定为 R1-001、R1-002 的处理及其直接影响处，不自动重开 v0.2 已确认可保留的其他内容。
- 内容校验：将正文按 UTF-8 解码、CRLF/CR 统一为 LF，保留其他字符及末尾换行，再编码为 UTF-8 计算 SHA-256：`4240ef1304bbfe64d62ad5a5626be4bb693b07c11b0b68c008348ddf292a67cf`。developer 开始前核对版本与哈希，不把不同内容视为同一复核版本；复核完成前 writer 保持正文不变。
- 复核重点：① 默认 nrfutil/device + J-Link 链、Programmer 可选定位、nrfjprog 备选条件和 A-005 差异协调是否准确；② nrfutil/device 版本、设备枚举、`west flash -d <构建目录> --context` 的步骤与失败分支；③ manifest、`SDK_VERSION`、GCC、west、nRF Util、device 的分层检查、资料期望值与实际输出边界；④ 未实测标记与来源是否足以支持后续用户照读而不冒充运行结果。
- 当前进度与下一位行动人：writer 已完成修订、自查和提交；developer 已完成必要复核并在下方记录 `pass`，R1-001、R1-002 均已关闭。下一位行动人为 writer 按 D-008 交付 v0.3 供用户完整通读；若用户反馈引入技术改动，形成新版本并再次复核，不覆盖本轮记录。

#### developer 的必要复核记录
- 日期、成员、受评版本与内容校验：2026-09-26，`developer`；A-004 v0.3（in_review，writer 于 2026-09-26 提交）。开始复核前按 UTF-8 解码、CRLF/CR 统一为 LF、保留其他字符及末尾换行后重新编码计算 SHA-256，结果为 `4240ef1304bbfe64d62ad5a5626be4bb693b07c11b0b68c008348ddf292a67cf`，与提交记录一致；本结论仅适用于该内容。
- 复核范围与依据：范围限定为 R1-001、R1-002 及其直接影响处。依据 A-005 v0.3、D-007、D-008、T-006 作者处理回复，以及 NCS v3.4.0 固定版本安装文档、编程文档、工具要求、发布说明和 west manifest，连同其锁定的 Zephyr `ncs-v3.4.0` 板级文档、`board.cmake`、`SDK_VERSION`。核对结果为：默认 nrfutil / `device` + J-Link 链、Programmer 可选定位、nrfjprog 备选条件、版本与设备检查、构建目录 runner context 和失败分支均已写明；版本检查已按 manifest、Zephyr SDK、GCC、west、nRF Util、`device` 分层，并区分资料期望与用户实际输出；A-005 v0.3 的两处差异已登记协调且未静默改写。
- 未覆盖项：未安装或运行 NCS v3.4.0、VS Code 扩展、J-Link、nRF Util / `device`；未执行版本命令、设备枚举、构建、`west flash --context`、烧录、串口或上板观察；未确认实际 UI、路径、工具版本、输出措辞、日志、设备标识和 LED / 串口现象。未重开 v0.2 已确认可保留的其他正文，也未执行 D-008 的用户完整通读；A-005 v0.3 的后续修订仍由 developer/project-manager 另行协调。
- 结论：`pass`。R1-001、R1-002 对 A-004 v0.3 的处理均通过并关闭；未发现新的本轮问题。该结论只表示本次限定技术文档复核通过，不表示安装、构建、设备连接、烧录、上板或整章批准已经完成。技术问题处理及必要复核已完成，下一位行动人为 writer，可按 D-008 启动 A-004 v0.3 的用户完整通读；若后续反馈引入技术改动，须形成明确版本并交 developer 再复核。

### 用户通读与反馈处理
- 依据：D-008；技术问题处理并完成必要复核后，writer 提供完整可读稿的路径、版本、修订摘要、已知限制与反馈入口。
- 反馈方式：用户可直接在会话中指出章节/小节、读不懂的地方、顺序或详略问题、希望补充的例子/图示；无需另写正式报告，也不要求同步完成上板实验。
- 记录方式：writer 按用户实际消息记录日期、所读版本、通读是否完成、反馈原意与位置，并逐项记录处理办法、修订版本及结果；未采纳或有分歧的意见说明理由并交用户确认。重要改动交用户复读，技术改动由 developer 复核；无反馈不能推定已读完或认可。
- 当前状态：2026-09-26，writer 已交付 A-004 v0.3 供完整通读；用户提出 F-001 后该次通读中止。writer 形成 v0.4 并重新交付后，用户继续提出 F-002，明确要求先直接使用官方用户指南截图。writer 已形成 A-004 v0.5（规范化 SHA-256：`15fd8e12b3936d88844936a9ee1bc16245217f011b60f5f01c3951e1edc5ed6c`）并提交 developer 限定技术复核；复核完成前不恢复用户通读，也不记录 v0.4 或 v0.5 已读完。

#### F-001：正文呈现过多项目记录信息，书稿感不足
- 反馈来源与日期：用户，2026-09-26；通读 A-004 v0.3 时提出的首条反馈，原意为“第 1 章有很多冗余信息，比如版本号、知识引用和说明性文字，感觉更像知识记录，不像书稿内容”。
- 反馈性质与范围：编辑与读者体验问题，涉及全章的正文层次和信息取舍；重点检查成果/版本元信息、项目内部知识与决定引用、证据等级和待核说明、来源说明及重复解释是否侵入读者叙事。必要的操作版本、兼容条件、安全提示和真实限制仍须保留，但应以读者完成本章所需的信息为准，项目追踪记录应留在 TASKS、DECISIONS、成果元信息或简洁的章末来源中。
- 作者处理回复：writer 于 2026-09-26 在 **A-004 v0.4** 完成全章编辑。移出读者叙事的内容包括 v0.1～v0.3 版本历史、成果/任务/决定/知识引用、A-005 差异协调过程、developer 复核状态、证据等级标签、重复的“未执行/待回填”段落及冗长的内部来源说明；这些事实保留在本任务、决定、成果索引和 Git 历史中。正文将未实测边界集中为章首一则“实践说明”，把章末来源压缩为读者可直接使用的官方入口，并改写开篇、安装、实验、资料地图和回顾段落的语气与衔接。正文规范化内容由 42422 字节降至 30275 字节，减少 12147 字节（约 28.6%）。
- 保留理由：NCS v3.4.0、DK 1.0.0 / Rev 2、Zephyr SDK 1.0.1、GCC 14.3.0、完整板级目标、命令参数、nrfutil/device + J-Link 默认链、nrfjprog 备选条件、`--recover` 擦除警告和“实验尚未执行”边界会直接影响兼容性、复现或安全，继续保留；三处截图及两幅插图仍以简短待补标记保留。
- 自查与复核判断：逐节检查正文已无可见的 A-/T-/D-编号、developer 评语、内部知识引用、`【待核】` 和“资料核查级”等项目话语；命令代码块、路径、板级目标、版本期望、构建/烧录顺序和安全前提与 developer 已通过的 v0.3 保持一致。变化属于编辑性删减、移位和语气改写，没有改变技术步骤或结论，因此不新增 developer 复核；若后续修改技术内容，再按 T-006 交复核。`python scripts/validate_framework.py` 与 `git diff --check` 的结果见本轮提交前校验记录。
- 状态：已处理，修订版本为 A-004 v0.4；等待用户重新完整通读，不把 v0.3 的中断阅读记为完成，也不预填 v0.4 的反馈。

#### F-002：先直接使用官方 DK 正面布局截图，并修正图中硬件标注
- 反馈来源与日期：用户，2026-09-26；继续通读 A-004 v0.4 时明确要求“先直接使用截图”。输入图片由 project-manager 从 Nordic 官方《nRF54L15 DK Hardware User Guide v1.0.0》第 7 页 Figure 2 截取，源文件位于 `workspace/ch-env-setup-assets/figure-1-1-nrf54l15-dk-front-source.png`。
- 作者处理回复：writer 于 2026-09-26 在 **A-004 v0.5** 处理。将源图原样复制为正式章节资源 `artifacts/book/chapters/ch-env-setup/images/figure-1-1-nrf54l15-dk-front.png`，源图与正式图片的 SHA-256 均为 `3E4452EE81D56D71D6F4AC989D079A3E72A01404F1C9B3217562294676E1C5E7`；正文 1.1.1 用 Markdown 图片和图注替换图 1-1 占位，图注注明 Nordic Semiconductor、《nRF54L15 DK Hardware User Guide v1.0.0》与 Figure 2，未写入临时裁剪路径。
- 硬件事实修正：依据 Figure 2 可见标注，将用户 LED 从“LED 1–4”修正为 **LED 0–3**；将供电、烧录、调试和串口连接明确为 **J3 一个 USB-C 接口**，并同步修改 1.1.3 和 1.4.1 的连接表述。其余技术步骤、命令、版本基线和未实测边界保持不变，没有写成实机验证结果。
- 版权与出版边界：本轮按用户当前明确指示将官方截图用于项目内部稿件。官方文档版权页写明未经书面许可不得复制；当前使用不等于已获得正式出版转载许可。正式出版前须确认 Nordic 的书面许可，或将图 1-1 替换为本项目自有的 DK 实物照片，并重新核对标注与图注。
- 状态：正文与图片已形成 A-004 v0.5，等待 developer 对图片来源、LED 0–3、J3 USB-C 接口及直接影响处做限定技术复核；复核结论不表示转载许可、安装或上板验证通过。

### F-002 技术修订复核：A-004 v0.5（已提交）
- 受评版本与内容校验：[A-004 正文](artifacts/book/chapters/ch-env-setup/text.md) v0.5，状态 in_review。正文按 UTF-8 解码、CRLF/CR 统一为 LF、保留其他字符及末尾换行后重新编码计算 SHA-256：`15fd8e12b3936d88844936a9ee1bc16245217f011b60f5f01c3951e1edc5ed6c`。
- 复核范围：① 正式图片是否与官方 Figure 2 截图一致、链接和图注来源是否准确；② 1.1.1 的 LED 0–3 与 J3 USB-C 接口表述；③ 1.1.3 和 1.4.1 的 J3 连接表述；④ 上述局部修正是否引入相邻硬件事实错误。版权许可只核对边界是否已清楚记录，不要求 developer 代替出版授权判断。
- 未覆盖与边界：不重开 v0.3 已通过的安装、工具链、构建和烧录内容；不把截图、资料核对或本次复核视为实机观察；不声称已取得正式出版转载许可。
- 当前进度与下一位行动人：writer 已完成修改、自查和提交；下一位行动人为 developer，填写受评版本/哈希、依据、未覆盖项与 pass/revise/blocked 结论。复核完成前正文保持不变，不恢复用户通读。

## 记录样式（不是真实任务）

```text
## T-001：任务标题
- 负责人：<member-id>
- 状态：todo
- 授权来源与日期：<用户直接指派或项目经理安排的可核对依据>
- 目标与范围：<实际授权的结果和边界>
- 输入与依赖：<任务、决定、资料版本；无则写无>
- 优先级：未设定
- 完成条件与确认方式：<可检查的条件；是否需评审，实际确认人>
- 进展：尚未开始
- 成果与验证证据：暂无
- 阻塞与下一位行动人：无
- 更新日期：<实际日期>
```

普通进展更新对应任务。更换负责人、重新打开或取消任务时保留原因、日期和已有成果；历史编号不复用。

## 可选的任务内评审样式（不是真实评审）

需要轻量评审时，将以下小节放到相应任务下；不要求为每位评审者另建任务、交接或报告。使用条件和写入责任见根项目运行协议第 4.1 节。

```text
### 本轮评审：<受评成果 ID、版本、轮次>
- 受评正文：<路径>
- 安排来源与日期：<用户或有权项目经理的明确安排>
- 参与成员、各自范围与完成条件：<member-id 及对应要求>
- 本轮进度与下一位行动人：<依据实际意见记录，不代写结论>

#### <member-id> 的意见
- 日期、评审依据与未覆盖项：<实际信息>
- 结论：<pass / revise / blocked；未提交时写尚未评审>
- 问题：<本轮唯一编号、位置、依据与影响、建议、原稿责任人；无问题则写无>
- 作者处理回复：<作者填写，注明问题编号与修订版本>
- 复核：<实际复核人、问题编号、版本、结果及证据；未复核则写待复核>
```

评审者只更新自己的意见与复核，作者填写处理回复；本轮全部意见提交后才修改受评正文。后续轮次保留前轮记录，不覆盖旧结论。
