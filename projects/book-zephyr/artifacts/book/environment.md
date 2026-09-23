# 技术与实验环境基线

- 项目 / 成果 ID / 关联任务：book-zephyr / A-005 / T-005
- 负责人 / 版本 / 更新时间：developer / 0.2 / 2026-09-23
- 成果状态：draft
- 适用范围：首本书全部章节的开发主机、NCS/Zephyr 软件基线、工具链与烧录工具、nRF54L15 DK 板卡基线，以及 D-004 电池 5 V 升压路径的实测验证安排；不含各章示例工程特有的配置（在对应示例与记录中补充）
- 来源及输入版本：T-005 授权（用户 2026-09-19 在 project-manager 会话确认登记）；D-001（读者画像）、D-004（物料清单与电池验证项）、D-005（验证基线、开发环境与节奏）；A-001 v0.2 图书设计（approved）；Nordic 官方发布信息与下载页核实结果（2026-09-20，见文中标注）；`workspace/nrf54l15-dk-docs/` 官方资料包（用户 2026-09-12 下载，2026-09-23 入库项目工作区，见"官方资料包"节）
- 批准依据：尚无（draft，待用户确认）
- 版本记录：2026-09-20 v0.1 初版：确认软件与板卡目标基线，登记电池升压实测计划；主机尚未安装任何开发软件，"实际安装结果"列待用户安装后回填。2026-09-23 v0.2：按用户三条反馈修订——① 核销"官方资料包缺口"一节并改按 `workspace/nrf54l15-dk-docs/` 新路径引用（旧名 `nRF54L15_DK_资料/` 不再使用），复核原文引用；② 复核确认安装路径与验证步骤全部保持"未执行/待回填"标注，未以资料核查冒充实测；③ 新增"IDE 路径评估"节：命令行工具链 + Source Insight 可行（官方 `nrfutil sdk-manager` / `west` 路径），建议本书主线仍按 VS Code 扩展路径，最终选择待用户决定

## 证据分级说明

本文区分三类事实，不互相冒充：

1. **官方资料核查**：developer 对 Nordic 官方发布信息、下载页与公开文档的在线核查结论（2026-09-20，来源逐条标注），以及对项目工作区内官方资料包原文的核查（2026-09-23，见"官方资料包"节）；
2. **用户已确认实物**：用户 2026-09-13 在 planner 会话确认、记录于 D-005 的实物信息（丝印、芯片修订号）；
3. **未执行/待回填**：主机安装、首次构建烧录、电池实测等尚未执行的项目，如实标注，不以资料结论代替执行结果。

## 环境组成

| 对象 | 版本、修订号或实际配置 | 核对依据 | 覆盖章节 | 未知与限制 |
|---|---|---|---|---|
| 开发主机 | Windows（版本未细分）；尚未安装任何开发软件 | D-005（用户确认，2026-09-13） | 全书，第 1 章从零安装写起 | 安装后的实际 Windows 版本、用户名路径待回填；其他系统最多作附注 |
| nRF Connect SDK（NCS） | **v3.4.0**（目标基线；Nordic 首个 LTS，5 年补丁支持；基于 Zephyr 4.4，含 Mbed TLS 4.1.0、TF-M 2.3.0） | 官方资料核查：Nordic 官方博客（2026-07-02 发布 LTS 公告；2026-07-01 发布） | 全书所有构建与示例 | 实际安装版本待第 1 章安装后回填；若安装前 Nordic 发布 v3.4.x 补丁，是否跟进在基线变更节处理 |
| Zephyr RTOS | 4.4（随 NCS v3.4.0 锁定，非 Zephyr LTS；API 与 Zephyr 下一 LTS 4.6 兼容） | 官方资料核查：同上 Nordic 官方博客 | 全书 | 不跨版本套用结论；引用 Zephyr 文档时以 NCS v3.4.0 自带文档为准 |
| 编译工具链 | Zephyr SDK 1.0.1 / GCC 14.3.0（随 NCS v3.4.0 预打包工具链） | 官方资料核查：Nordic DevZone 用户环境报告（NCS 3.4.0 LTS 配套）与公开版本分析一致 | 全书 | 精确版本以安装后 `west sdk-version` 与工具链目录实际内容回填为准 |
| SDK 安装方式 | **nRF Connect for VS Code 扩展包**（Install SDK，预打包 SDK+工具链，默认安装到 `C:\ncs`）；命令行备选 `nrfutil sdk-manager` | 官方资料核查：Nordic 官方 nRF Connect for Desktop 下载页明确 **Toolchain Manager 已弃用、不支持 NCS v3.0.0 及以后版本**，改用 VS Code 扩展或命令行 | 第 1 章安装路径 | 这是对 A-001 v0.2 中"nRF Connect for Desktop / Toolchain Manager / VS Code 路径"表述的实测前修正：NCS v3.4.0 不能走 Toolchain Manager，正文须按 VS Code 扩展路径写 |
| IDE | 主线：Visual Studio Code + nRF Connect for VS Code 扩展包（版本随安装时市场最新）；备选：Source Insight 等任意编辑器 + 命令行工具链（可行性评估见"IDE 路径评估"节，最终选择待用户决定） | 官方资料核查：Nordic 官方安装文档路径；备选路径经官方 `nrfutil sdk-manager` / `west` 命令行文档核查 | 第 1 章及全书 | 扩展具体版本号安装后回填；备选路径若成为本书主线将影响 A-004 第 1 章正文写法 |
| 烧录与桌面工具 | nRF Connect for Desktop（Programmer 应用）；nRF Command Line Tools（含 nrfjprog 与 SEGGER J-Link 驱动） | 官方资料核查：Nordic 官方下载页（2026-09-20） | 第 1 章烧录、后续章节目志与电流测量 | 具体版本安装后回填；J-Link 驱动须随 nRF Command Line Tools 安装，不可跳过 |
| 开发板 | nRF54L15 DK **1.0.0**（实物丝印 PCA10156 1.0.0）；板上 **Rev 2** 芯片（nRF54L15-QFAAC00）；勘误表对应 Rev 2 Errata v1.1 | 用户已确认实物（D-005，2026-09-13）；与资料包中 DK 用户指南 v1.0.0、Rev 2 勘误表 v1.1 记载一致（2026-09-23 按新路径复核） | 全书硬件实验 | 板上 J-Link OB 调试器固件版本首次连接时核对并回填；早期 PDK（0.8.x）与 DK 0.9.x 不构成本书基线 |
| 板级构建目标 | `nrf54l15dk/nrf54l15/cpuapp`（Zephyr 板级标识，以 NCS v3.4.0 板级文档为准） | 官方资料核查：Zephyr/NCS 板级命名惯例；最终标识在第 1 章首次构建时以 `west boards` 实际输出回填 | 全书示例构建命令 | 未执行：首次构建前不作最终引用 |
| 外接实验物料 | 按 D-004 清单：ICM-42688-P ×2、DRV2605L 带电平转换 ×1、LRA 3–5 颗、磷酸铁锂充放一体模块 ×2、3.2 V 约 2000 mAh 软包 ×2；用户已有线材，无逻辑分析仪 | D-004（用户确认，2026-09-13） | 第 6、7、8、9、15、18 章 | 物料到货与验货状态待用户报告；协议教学用日志与寄存器，不依赖逻辑分析仪 |
| 测量手段 | DK 板载电流测量能力（nRF Connect for Desktop / Power Profiler 类工具路径）；无独立仪器 | D-005、A-001 v0.2 | 第 15 章低功耗、电池验证 | 无仪器测量项不承诺数值精度；缺仪器的测量计划保留并标注 |

## 准备与复现条件

### 从零安装路径（Windows，第 1 章主线）

1. 安装 Visual Studio Code（系统或用户安装均可，安装路径避免中文与空格）。
2. 在 VS Code 扩展市场安装 **nRF Connect for VS Code Extension Pack**。
3. 通过扩展的 Welcome 页 **Install SDK**，选择 **nRF Connect SDK v3.4.0**（预打包 SDK + 工具链），默认安装到 `C:\ncs`；下载区域可按网络情况选择 Global 或 Mainland China。
4. 安装 **nRF Connect for Desktop**，并在其中安装 **Programmer** 应用（用于烧录与后续电流测量相关工具）。
5. 安装 **nRF Command Line Tools**（Windows x86-64），安装过程中**不要跳过 SEGGER J-Link 驱动**；安装完成后重启使环境变量生效。
6. 验证检查（每项以实际输出为准，记录于第 1 章记录）：
   - 工具链终端中 `west --version`、`west sdk-version` 可执行且版本符合本基线；
   - `west boards | findstr nrf54l15dk` 可见板级目标；
   - 针对 `nrf54l15dk/nrf54l15/cpuapp` 构建 NCS 自带示例（如 `hello_world`）成功；
   - DK 经 USB 连接后被 Programmer 识别，烧录示例成功，串口日志可见预期输出。

> 上述步骤 1–6 当前均为**未执行**：主机尚未安装任何软件（D-005）。第 1 章正文按此路径撰写，实际执行由用户照做，结果以"用户执行/用户报告"记录；developer 据此回填实际版本并处理差异。

### IDE 路径评估（2026-09-23 用户反馈③）

用户询问能否不用 VS Code + nRF Connect 扩展，改用"编译器工具链 + Source Insight"方式。评估结论：**可行，为官方支持路径，但与本书主线写法存在取舍**。

- **SDK 命令行安装途径（官方支持）**：`nrfutil sdk-manager install --ncs-version v3.4.0` 安装 SDK+工具链；`nrfutil sdk-manager toolchain launch --ncs-version v3.4.0 --terminal` 启动配好环境的工具链终端。注意 nrfutil 须从 Nordic 官网下载最新版，不可用 pip 安装（pip 上是过时的 v5.2.0）。
- **构建与烧录**：工具链终端内 `west build -b nrf54l15dk/nrf54l15/cpuapp`、`west flash` 与扩展路径完全等价——扩展底层同样调用 west，构建系统与产物无差异。
- **Source Insight 的角色**：纯代码编辑与导航工具，不参与构建配置；Zephyr 工程由 CMake/west 管理，不依赖任何 IDE 工程文件，因此编辑器可自由替换。
- **调试手段变化**：扩展的图形化调试（nRF Debug）不再可用；改用 `west debug` 命令行 GDB（经 J-Link GDB Server，Zephyr SDK 自带 arm-zephyr-eabi-gdb），或 SEGGER Ozone 图形调试器（备选，未验证）。
- **失去的能力**：扩展的构建/烧录 GUI 集成、Kconfig 与 devicetree 可视化辅助、nRF Terminal 串口（可用任意串口工具替代）。对首次接触 MCU/RTOS 的学习曲线，这些辅助有实际价值。
- **对本书的影响与建议**：Nordic 官方文档、DevAcademy 课程与排错资料均以 VS Code 扩展路径为主，读者（无 MCU/RTOS 经验，D-001）照做时参考资料最多，**建议本书主线仍按 VS Code 扩展路径撰写**；用户个人日常可并行使用 Source Insight 浏览代码，二者不冲突。若用户决定本书改用命令行主线，A-004 第 1 章正文（按 VS Code 路径撰写）的安装、构建、烧录小节需由 writer 改写，影响另行登记。**最终选择待用户决定，确认前本基线主线维持 VS Code 扩展路径不变。**
- **执行状态**：以上为官方资料与命令行文档核查结论（2026-09-23），`nrfutil sdk-manager` 安装与 `west debug` 均**未在本机执行**；若用户选定备选路径，相应步骤同样标"未执行/待回填"，由用户照做后回填。

### 通用机制与厂商扩展的区分

- `west`、CMake、Kconfig、devicetree、Zephyr SDK 为 Zephyr 通用机制；NCS 的预打包 SDK+工具链、nRF Connect for VS Code 扩展、nRF Connect for Desktop、nrfjprog/nrfutil 为 Nordic 厂商工具；`nrf54l15dk` 板级定义与 DK 板载测量能力为平台特性。正文引用时分别标注，不跨来源混用版本结论。
- 凭据（如 Nordic 账号）不写入书稿；安装过程不要求登录的环节如实说明。

### 官方资料包

官方资料包已入库项目工作区：[`workspace/nrf54l15-dk-docs/`](../../workspace/nrf54l15-dk-docs/README.md)（用户 2026-09-12 下载，2026-09-23 放入；旧名 `nRF54L15_DK_资料/` 不再使用，历史记录中的旧名指同一目录）。与本书直接相关并已纳入 Git 的文档：

| 文档 | 用途 |
|---|---|
| `nRF54L15_nRF54L10_nRF54L05_Datasheet_v1.0.pdf`（芯片数据手册 v1.0，940 页） | 寄存器、电气参数、封装引脚的一手来源 |
| `nRF54L15_DK_HW_User_Guide_v1.0.0.pdf`（DK 硬件用户指南 v1.0.0，31 页） | 板载资源、供电、电流测量、调试接口的一手来源 |
| `nRF54L15_Rev_2_Errata_v1.1.pdf`（Rev 2 勘误表 v1.1，27 页） | 与板上 Rev 2 芯片对应的勘误依据 |
| `nan_047.pdf`（nRF54L 系列量产烧录指南） | 第 17 章产品化参考 |

原理图（`PCA10156_Schematic_And_PCB.pdf`）、完整硬件设计包 zip 与硬件设计指南 `ngl_001.pdf` 保留在本地同一目录但不纳入 Git（D-002 已确认不讲自行设计硬件，仅作参考）。正文引用资料包原文（页码、图表编号）时以上述路径文件为准；developer 已于 2026-09-23 复核本文件对资料包的引用，无失效引用。

## D-004 电池 5 V 升压路径实测安排

- **待验证项**（来自 A-001 v0.2）：磷酸铁锂充放一体模块 5 V 升压路径的实测表现，含小电流模式/轻载自动关机行为；P6 直连稳压备选方案是否引入留待第 18 章部署形态讨论时定。
- **前置条件**：模块与电池到货（用户采购，当前状态待用户报告）；收货先验电池鼓包与保护板。
- **计划步骤（未执行，执行者为用户，developer 提供步骤与预期并记录结果）**：
  1. 模块空载与轻载输出电压测量，观察小电流模式下是否自动关断及恢复条件；
  2. 模块经 DK 供电路径带载供电，观察 DK 运行示例时的供电稳定性；
  3. DK 进入低功耗状态（睡眠电流区间）时模块是否保持输出——这是小电流模式的关键验证点；
  4. 记录每次测量的接法、参数、观察值与结论（通过/失败/阻塞），证据按 `artifacts/book/evidence/` 边界存档。
- **安全约束**（写入正文可见位置）：磷酸铁锂充电截止电压 3.65 V，不过充过放；收货验鼓包，异常即停用；远离高温与短路；首次充电有人值守。
- **当前状态**：未执行，无实测结论；不以模块标称参数代替实测。

## 基线变更

- 本文是首版基线，无旧基线。后续变更（如 NCS 补丁版本跟进、工具链版本变化、板卡固件更新）在此节追加：旧基线与新基线、变更依据、影响章节与示例、需要重跑的验证证据。旧环境下的通过结论不自动覆盖新环境。
