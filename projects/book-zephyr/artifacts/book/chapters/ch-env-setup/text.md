# 第 1 章 认识 nRF54L15 DK 与开发环境搭建

> 唯一当前正文。本版为第 1 轮问题修订后的必要复核稿（in_review），待 developer 复核 R1-001、R1-002 的处理结果后，再交用户完整通读；尚未最终批准。

- 项目：book-zephyr / 成果 ID：A-004 / 关联任务：T-004（初稿）、T-006（修订、技术复核与通读反馈）
- 负责人：writer / 版本：0.3 / 更新时间：2026-09-26
- 成果状态：in_review
- 适用范围：第 1 章正文草稿，覆盖已批准蓝图 A-003 v0.1 约定的学习目标、内容结构与示例说明；不含示例工程的独立仓库实现（本章直接使用 NCS/Zephyr 自带示例），不含 environment.md 环境基线确认（T-005，developer 职责）
- 来源及输入版本：[章节蓝图 A-003 v0.1](plan.md)（approved，批准依据 D-006）；[图书设计 A-001 v0.2](../../design.md)（approved）；[全书目录 A-002 v0.2](../../outline.md)（approved）；[环境基线 A-005 v0.3](../../environment.md)（approved，D-007）；决定 D-001～D-008；[官方资料包索引](../../../../workspace/nrf54l15-dk-docs/README.md)；初稿技术来源保留，2026-09-26 补核 NCS v3.4.0 固定版本的安装、编程、工具要求、west manifest、板级 runner 与 `SDK_VERSION`（见“来源与延伸阅读”）。A-001/A-003 的旧安装表述按 T-006、D-007 和 A-005 v0.3 修正，不修改其原稿；A-005 v0.3 的烧录工具与版本检查差异已在 T-006 登记协调，也不由本成果静默改写。
- 批准依据：尚无
- 版本记录：2026-09-20 v0.1 初稿，按 A-003 v0.1 蓝图撰写；软件基线按 D-005 候选基线 NCS v3.4.0 撰写，environment.md 基线确认前，涉及具体版本、路径与界面的位置均标注【待核】
- 版本记录：2026-09-26 v0.2 按 T-006 修订学习目标、1.2～1.4 节及图示/截图说明：改用 VS Code 扩展安装 SDK、配置终端并衔接构建烧录；核销“基线未确认”“资料包缺失”，更新来源，保留实际版本、安装与上板未执行及图像占位。新增烧录依赖和版本检查命令的待技术复核说明；作者自查后提交 developer 第 1 轮技术复核，尚无复核结论或用户通读反馈。
- 版本记录：2026-09-26 v0.3 按 developer 第 1 轮技术复核结论 `revise` 统一处理 R1-001、R1-002：明确默认烧录链为工具链捆绑的 nRF Util `device` 命令配合独立安装的 SEGGER J-Link，Programmer 改为可选图形工具，nRF Command Line Tools / nrfjprog 仅作显式选择 runner 时的备选；新增 nrfutil、device、设备枚举和 runner context 检查；删除 `west sdk-version`，分开核对 manifest 标签、`SDK_VERSION`、GCC、west、nRF Util 与 device。资料期望值和用户实际输出保持分离，全部安装、构建、烧录及上板结果仍未执行；现提交 developer 必要复核。
- 稳定章节标识：ch-env-setup（显示章号“第 1 章”以 [全书目录](../../outline.md) 为准）/ 交付用途：技术复核，后续通读按 D-008

## 本章要解决的问题

本章从 nRF54L15 DK 和一台 Windows 主机开始，不需要后续外设实验的全部物料。导读尚未成文时，可以直接按下面的资源清单准备。这一章要回答三个问题：这块板子上有什么？开发它的软件环境由哪些部分组成、怎么在 Windows 上从零装起来？以及最重要的——如何把第一个程序真正跑在芯片上？你也可以先完整阅读并反馈疑问，再另行安排安装和上板实验。

读完本章并跟着做完实验后，你应该能够：

1. 说出 nRF54L15 SoC 的主要组成——Cortex-M33 应用核、FLPR RISC-V 协处理器、1.5 MB RRAM、256 KB RAM、电源域的概念，以及主要外设各自负责什么——并在 DK 实物上指出主芯片、天线、按键与 LED、USB 口、电流测量头的位置；
2. 在 Windows 主机上从零安装开发环境（VS Code → nRF Connect 扩展包 → Install SDK 安装 NCS v3.4.0 及配套工具链），准备调试驱动和烧录工具，并从扩展的工具链终端进行检查；
3. 基于官方示例创建工程，完成“构建 → 烧录 → 上板观察”的完整循环（blinky：LED 闪烁）；
4. 修改示例参数（闪烁周期）后重新构建烧录，确认自己掌控这个循环；
5. 说出官方资料——数据手册、DK 用户指南、Rev 2 勘误表、NCS 在线文档、DevZone——各自的用途，并能按板卡版本找到对应的勘误。

**前提**：本书假设你熟练使用 C 语言和系统编程、熟悉驱动与体系结构的概念（例如寄存器、中断、控制器），但此前没有接触过单片机、RTOS 和蓝牙。C 语言与寄存器原理本书不再讲解；遇到 Zephyr 特有的概念（本章会遇到 devicetree、Kconfig 的名字）我们会借助你的 Linux 内核背景作对照——正式讲解在第 2 章，本章只要求“会用”。

**所需资源**：一台 Windows 主机（本章按尚未安装开发软件的起点讲解）；nRF54L15 DK 1.0.0（板上为 Rev 2 芯片）；一条 USB-C **数据线**；可访问 Nordic 官网、VS Code 下载页和扩展市场的网络。安装前检查磁盘余量与网络；下载体积、耗时、实际 Windows 版本和安装路径均【待核：用户安装后回填】，基线批准不代表这些数据已经测得。

## 理解与实践

### 1.1 这块板子是什么

#### 1.1.1 先摸一遍实物

把 DK 拿在手里，先建立空间印象。以下是板上需要认识的位置（对照图 1-1）：

> 【图 1-1 占位】DK 布局标注图——基于 DK 硬件用户指南（v1.0.0）第 1 章**自制重绘**，标注：主芯片 nRF54L15、2.4 GHz 天线区、按键 Button 0–3、LED 1–4、两个 USB-C 口、电流测量头 P6、外接引脚排座、电源开关。待绘制。

- **主芯片 nRF54L15**：板子中央那枚 QFN 封装的芯片，整块板子的主角。你写的程序最终就运行在它内部。
- **天线区**：板子边缘的 PCB 天线区域，2.4 GHz 射频信号从这里进出。蓝牙实验（第 11 章起）之前只需要知道它在哪里、不要用手遮挡即可。
- **按键与 LED**：4 个用户按键（Button 0–3）和 4 个用户 LED（LED 1–4）。它们是全书前半部分最主要的“输入输出设备”——本章的第一个程序就是让 LED 闪起来。
- **USB-C 口**：DK 上有 USB 口用于连接电脑。它不仅供电，还承担着烧录、调试和串口日志三重职责——这些都由板载调试器完成（见 1.1.3）。本章连接的是板载调试器一侧的 USB 口【待核：实物 USB 口丝印标注以 DK 用户指南 v1.0.0 第 1 章为准，用户实机照做时确认】。
- **电流测量头 P6**：一组排针，用于给芯片供电并串联电流测量。第 9 章做低功耗实验时会用到它，本章只需找到它的位置。
- **外接引脚排座**：板子两侧把芯片的主要 GPIO 引了出来，第 6 章起外接传感器和振动马达时会用到。

#### 1.1.2 打开芯片：nRF54L15 架构导览

现在把视野从板子缩到芯片内部。nRF54L15 是一颗多核、多电源域的 SoC，我们先建立一张“地图”（对照图 1-2），各部件只用一句话说明职责，细节留到对应章节展开。

> 【图 1-2 占位】nRF54L15 SoC 架构示意图——基于芯片数据手册（v1.0）框图**自制重绘**，标注：Cortex-M33 应用核、FLPR 协处理器、RRAM、RAM、2.4 GHz 射频、电源域划分、主要外设。待绘制。

- **Cortex-M33 应用核（128 MHz）**：主处理器，你的应用程序默认运行在这里。它与你在 Linux 世界熟悉的“应用 CPU”角色相同——只不过这颗芯片没有 MMU，不存在用户态/内核态的地址空间隔离，程序直接运行在物理地址上。理解 RTOS 时我们会反复回到这个差异（第 3 章）。
- **FLPR RISC-V 协处理器**：一颗 128 MHz 的 RISC-V 小核，可以分担时序敏感的外设任务。本章只需要建立“芯片里还有一个协处理器”的印象；如何给它编程是进阶篇 ch-flpr-adv 的内容。
- **1.5 MB RRAM（阻变存储器）**：芯片的主存储器，相当于你熟悉的 Flash——程序烧录后就存放在这里。nRF54L15 用 RRAM 取代传统 NOR Flash，这在 Zephyr 里仍按 Flash 设备管理，第 16 章讲分区布局时会展开。
- **256 KB RAM**：运行时的内存。所有线程栈、堆、全局变量都在这 256 KB 里周转——没有交换分区可言，内存规划是嵌入式开发的日常（第 10 章）。
- **2.4 GHz 射频**：支持蓝牙低功耗（BLE）等 2.4 GHz 无线协议。本书主线是 BLE，从第 11 章开始正式使用。
- **电源域**：芯片内部划分了多个可以独立上下电的区域，这是 nRF54L15 低功耗设计的物理基础。本章只需知道“芯片能分区断电”这个概念，第 9 章结合电流测量展开。
- **主要外设清单**（各一句话，后面都有专章）：
  - GPIO / GPIOTE——引脚电平与事件，第 4 章；
  - UARTE——带 DMA 的串口，日志与调试的命脉，第 5 章；
  - TIMER / PWM——定时与波形输出，第 6 章；
  - TWIM（I2C）/ SPIM（SPI）——两条主力外设总线，第 7、8 章；
  - SAADC——模数转换，第 9 章；
  - RADIO、CRACEN（硬件加密）、GRTC（全局 RTC）、WDT（看门狗）等——随用随讲。

> 顺手澄清一个容易误导的传闻：nRF54L15 **没有** I3C 控制器——本书规划阶段曾对 940 页芯片数据手册（v1.0）全文检索，I3C/MIPI 零命中（I3C 是 nRF54H 系列的特性）。所以网上涉及 nRF54 的 I3C 讨论不适用于这块芯片，本书也不涉及 I3C。

#### 1.1.3 板载调试器：你的烧录器、调试器和串口线

DK 上除了 nRF54L15，还有一位隐形助手：**板载调试器**（J-Link OB，“OB”即 On-Board）。它在 USB 口与目标芯片之间，一片抵三件传统装备：烧录器、在线调试器（类似你熟悉的 JTAG）、USB 转串口。后面的 `west flash` 命令、第 5 章的串口日志、以及可选实验 ex-hello-uart 里看到的终端输出，都经由它完成。第一次连接 DK 时，电脑上的 Nordic 工具可能提示给板载调试器升级固件——这是正常现象，按提示操作即可【待核：出厂固件是否提示升级以匹配 NCS v3.4.0，待用户实机照做时验证】。

DK 的 VDD（IO 电平）默认为 1.8 V。本章不连接任何外接模块，无需调整；等第 7 章接传感器之前，我们才会用到配套的 Board Configurator 工具检查电平设置。

### 1.2 开发环境长什么样

动手安装之前，先看一张“要装哪些东西、各自管什么”的地图（对照图 1-3）。嵌入式工具链名词繁多，先把角色分清，安装时就不会迷失。

图 1-3 开发环境组成示意（作者自制，表示工具职责，不是实机截图）：

```text
Windows 主机
├─ VS Code + nRF Connect 扩展包：安装 SDK、编辑、构建与调试入口
│  ├─ Install SDK → NCS v3.4.0 源码 + 配套工具链
│  └─ nRF Connect 工具链终端 → west → 构建 / 默认 nrfutil runner
│     └─ 工具链捆绑 nRF Util + device 命令
├─ SEGGER J-Link 软件 + Windows USB 驱动 → 板载调试器 → nRF54L15
└─ 可选：nRF Connect for Desktop → Programmer（独立图形烧录界面）
```

- **Zephyr**：一个开源 RTOS（实时操作系统），由 Linux 基金会托管。它提供内核（线程、调度、同步、中断、定时）、设备驱动模型和大量协议栈。角色上可以对标你熟悉的 Linux 内核——只不过它面向没有 MMU 的微控制器，镜像通常只有几十到几百 KB。
- **NCS（nRF Connect SDK）**：Nordic 在 Zephyr 之上维护的官方 SDK，包含 Nordic 平台支持、协议栈、MCUboot 和示例。本书按已批准的 A-005 v0.3 使用 **NCS v3.4.0**，其中记录的 Zephyr 基线为 4.4；实际安装版本仍待回填。日常说“装环境”，需要同时具备 SDK 源码和与它匹配的工具链。
- **west**：Zephyr 生态的命令行“大管家”，负责多仓库管理，并通过扩展命令衔接构建、烧录和调试。本章用 `west --version`、`west topdir` 和 `west boards` 检查命令、工作区与板列表，再用 `west build` 构建、`west flash` 调用烧录后端。对本书板级目标，默认 flash runner 是 **nrfutil**。
- **工具链**：编译器、链接器、构建工具，以及 NCS v3.4.0 工具链捆绑的 nRF Util 与 `device` 命令。A-005 v0.3 记录的目标是 Zephyr SDK 1.0.1 / GCC 14.3.0；源码、SDK、编译器、west、nRF Util 与 `device` 各自有独立版本，必须分别核对，不能从其中一条输出推断整套环境已匹配。
- **默认烧录链**：`west flash` 默认选择 nrfutil runner，由工具链内的 nRF Util `device` 命令驱动烧录流程，并通过独立安装的 SEGGER J-Link 软件和 Windows USB 驱动访问 DK 的板载调试器。NCS v3.4.0 的板级文件先载入 nrfutil、再载入 jlink，因此默认项是 nrfutil；实际 runner 仍要对具体构建目录执行 `west flash --context` 核对。
- **nRF Connect for Desktop / Programmer**：独立的图形化工具箱与可选烧录界面，不是本章默认 CLI 烧录链的前置条件，也不承担本书版本的 SDK 安装入口。**Toolchain Manager 从 NCS v3.0.0 起不再提供新版本 SDK 和工具链安装**，不能用它安装 v3.4.0。[官方迁移说明](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/releases_and_maturity/migration/migration_guide_3.0.rst)
- **nRF Command Line Tools / nrfjprog（备选）**：该工具包已经归档；只有显式选择 `west flash -r nrfjprog` 时才需要。不要为了默认的 `west flash` 主线安装它，也不要让其中附带的旧 J-Link 覆盖已经安装的匹配版本。
- **VS Code 与 nRF Connect 扩展包**：本书的开发主线，负责 SDK 安装及日常编辑、构建和调试。下面的 west 命令在扩展配置的终端中运行，用于说明每一步的输入与产物；它们不要求改用另一套 IDE。Source Insight 可供个人浏览编辑代码，不替代 SDK、工具链或终端环境。

还有两个名词本章只点名、不展开：**devicetree**（硬件描述，可借助你的 Linux 设备树经验理解，但使用方式存在差异）和 **Kconfig**（功能配置，同样源自 Linux 内核）。第 2 章剖析 blinky 工程时正式讲解；**DFU**（固件升级，本书第 16 章介绍 BLE 空中升级）则留到后面。

一个容易混淆的概念是**板级目标**（board target）：构建时告诉 west“为哪块板子的哪个核构建”。本书统一使用 `nrf54l15dk/nrf54l15/cpuapp`——读作“nRF54L15 DK 板 / nRF54L15 芯片 / 应用核”。写错一个字符，构建系统就会报错或生成错误的镜像，这是新手最常见的失败之一（Zephyr 官方板级文档亦以完整板级目标名演示构建，见“来源与延伸阅读”）。

### 1.3 动手安装（Windows 从零）

主线为 **VS Code → nRF Connect 扩展包 → SDK 与配套工具链（含 nRF Util + device）→ SEGGER J-Link 软件与 Windows 驱动 → 工具链终端检查**。以下步骤以 A-005 v0.3 的已批准主机、IDE 和 SDK 基线为输入，并按 T-006 的固定版本技术复核结果纠正烧录工具关系；本项目尚未执行安装。各步“预期成功现象”是检查目标，不是已发生的结果。

安装目录采用不含空格和中文的路径，例如 `C:\ncs`；SDK 的实际路径需在安装时记录。界面可能随扩展版本变化，找不到入口时记录扩展版本与界面，不回退到 Toolchain Manager，也不因列表默认推荐其他 SDK 而自行更换 v3.4.0 基线。

#### 1.3.1 第一步：安装 VS Code 与扩展包

1. 从 [VS Code 官方下载页](https://code.visualstudio.com/download) 下载 Windows 安装程序，按主机情况选择用户或系统安装。
2. 打开扩展市场，安装 Nordic Semiconductor 发布的 **nRF Connect for VS Code Extension Pack**，完成其依赖扩展安装。
3. 打开左侧 nRF Connect 视图，进入 Welcome 页。

**预期成功现象**：扩展视图可以打开，首次安装时可看到 SDK 安装入口。记录 VS Code 与扩展版本；若扩展尚未启用，先处理安装提示，再继续。

> 【截图 1-1 占位】VS Code 的 nRF Connect Welcome 页及 Install SDK 入口；待用户实机采集，不以示意图冒充截图。

#### 1.3.2 第二步：通过 Install SDK 安装 NCS v3.4.0

1. 在 Welcome 页选择 **Install SDK**，下载区域按网络条件选择。
2. 选择 **nRF Connect SDK**，再选择 **v3.4.0** 的 SDK 与工具链组合。仅安装工具链不会自动补齐 SDK 源码。
3. 按基线使用 `C:\ncs` 作为安装根目录，记录安装器实际显示的 SDK 路径；等待完成通知，并查看 Output 面板中的安装日志。

**预期成功现象**：扩展可管理已安装的 v3.4.0 SDK 与对应工具链，源码目录存在。安装页细节、安装耗时与最终路径【待核：用户安装后回填】。若没有 v3.4.0 或安装失败，保留完整提示交技术核查，不将安装成功写入记录。[SDK 首次安装官方说明](https://docs.nordicsemi.com/r/bundle/nrf-connect-vscode/page/get_started/quick_setup.html/installing-sdk-and-toolchain-for-the-first-time?contentId=El7l02bgw~Jiewa~98anOw)

下文以 `C:\ncs\v3.4.0` 为 SDK 路径示例；如果实际路径不同，后续命令和编辑路径要一起替换。目录树为示意，未经本机安装验证：

```text
C:\ncs\
├── toolchains\     ← 配套工具链（具体子目录名待安装后记录）
└── v3.4.0\         ← 本文示例中的 west 工作区根目录
    ├── .west\      ← west 工作区信息
    ├── zephyr\     ← Zephyr 源码与官方示例
    ├── nrf\        ← Nordic 组件与示例
    ├── bootloader\ ← MCUboot 等组件
    └── ...
```

SDK 源码目录和工具链目录用途不同；选择终端时两者的版本必须匹配。这个多仓库工作区由 west 管理，第 2 章再展开。

> 【截图 1-2 占位】扩展中 v3.4.0 SDK 与配套工具链安装完成的界面；待用户实机采集。

#### 1.3.3 第三步：准备默认烧录链

NCS v3.4.0 的默认命令行路径是 **工具链捆绑的 nRF Util `device` 命令 + SEGGER J-Link 软件和 Windows USB 驱动**。按下面的职责关系准备，不把多个烧录工具混成同一项：

1. 按 [NCS v3.4.0 固定版本安装文档](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/installation/install_ncs.rst) 的 prerequisite 安装匹配的 **SEGGER J-Link Software and Documentation Pack**；在 Windows 上同时安装 SEGGER USB Driver for J-Link。记录实际安装版本；如果安装程序要求重启，完成后重新打开 VS Code 与工具链终端。
2. 不为默认烧录链另装一套 nRF Util。自 NCS v3.1.0 起，`device` 命令随 NCS 工具链 bundle 提供；1.3.2 的 v3.4.0 SDK 与工具链组合应同时带来锁定的 nRF Util 和 `device` 版本。下一节用命令确认实际可调用版本。
3. **Programmer 是可选图形工具**。需要图形化识别或烧录时，可以安装 [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop) 及 Programmer；未安装 Programmer 不阻止本章使用默认的 `west flash` 命令链。
4. **nRF Command Line Tools / nrfjprog 仅作备选**。只有明确执行 `west flash -r nrfjprog` 时才需要已归档的 nRF Command Line Tools。本章不选择该 runner，日常命令也不加 `-r nrfjprog`。

> **与 A-005 v0.3 的协调记录**：A-005 v0.3 已批准的 Windows、VS Code、NCS v3.4.0 与工具链基线继续生效；其中把 Programmer 和 nRF Command Line Tools / nrfjprog 写入主线、并把 J-Link 驱动绑定到该归档工具包的表述，与 NCS v3.4.0 固定版本资料和 developer 的 R1-001 结论不同。T-006 已登记该差异，交 developer/project-manager 协调 A-005 的后续版本；A-004 先采用可操作的固定版本路径，不改写 A-005 原文或其批准记录。

**预期成功现象**：匹配的 J-Link 软件与 Windows 驱动安装完成，工具链终端可以调用捆绑的 nRF Util 与 `device` 命令；连接 DK 后 `nrfutil device list` 能枚举目标设备。当前以上均为**未执行**，J-Link、nRF Util、`device` 的实际版本，设备识别结果和日志均待用户回填。

#### 1.3.4 实验 ex-env-toolchain-check：找到正确终端并检查环境

> **实验性质**：用户实机执行；当前未执行。以下为命令及预期检查点，不能替代后续构建、烧录和上板验证。

1. 在 VS Code 中按 `Ctrl+Shift+P` 打开命令面板，执行 **`nRF Connect: Create Shell Terminal`**；也可从 nRF Connect 的 Welcome 页选择 **Open terminal**。终端可能沿用上次使用的 SDK 和工具链，须通过 **Pick Toolchain and SDK for nRF Connect Terminal** 选择 v3.4.0 及匹配工具链，再核对终端显示的版本。入口名称据当前官方文档，实际界面与扩展版本【待核：用户回填】。[终端入口官方说明](https://docs.nordicsemi.com/r/bundle/nrf-connect-vscode/page/guides/extension_nrfconnect_profile.html/terminal-for-the-last-used-toolchain-and-sdk?contentId=IwzFNwle4i0Ds0KBOspH_A)
2. 在该终端进入 SDK 工作区根目录。下文命令以 Windows PowerShell 为例；路径不同应先替换，不要在普通终端或串口会话中直接照抄：

   ```powershell
   cd C:\ncs\v3.4.0
   git -C nrf describe --tags --exact-match HEAD
   Get-Content zephyr\SDK_VERSION
   arm-zephyr-eabi-gcc --version
   west --version
   west topdir
   west boards | findstr nrf54l15dk
   nrfutil --version
   nrfutil device --version
   ```

3. 先把**资料期望值**单独记下，不与实际输出合并：

   | 对象 | 本书基线的资料期望值 | 能证明什么 |
   |---|---|---|
   | NCS manifest 仓库 | `nrf` 的精确标签为 `v3.4.0` | 当前 manifest 源码身份 |
   | Zephyr SDK | `zephyr\SDK_VERSION` 内容为 `1.0.1` | 被该 Zephyr 修订记录的 SDK 版本 |
   | GCC | 目标版本为 `14.3.0` | 工具链中的实际编译器版本；还要结合首次构建日志中的 `Found toolchain` 路径 |
   | west | 记录 `west --version` 的实际值 | 只证明 west 自身版本 |
   | nRF Util / `device` | 由 v3.4.0 工具链 bundle 提供并锁定；分别记录两条版本命令的实际值 | 只证明当前终端调用到的 nRF Util 主程序和 `device` 命令版本 |

   `west sdk-version` 不是本基线中已声明的 west 命令，本版将它删除，不再把它作为待试命令或成功判据。`west --version`、`nrfutil --version` 和 `nrfutil device --version` 也不能互相替代。
4. 再保存**用户实际输出**：终端所选 SDK/工具链、SDK 路径、上述每条命令的完整输出，以及扩展安装信息。若 `git ... --exact-match` 因本地仓库没有标签而失败，或任何版本与期望不同，不要自行改写期望值；追加 `git -C nrf rev-parse HEAD`，保留路径与完整错误交技术核查。
5. 连接 DK 后执行设备枚举：

   ```powershell
   nrfutil device list
   ```

   **预期检查点**：输出中出现当前 DK 的设备记录；序列号、COM 口、名称和字段格式以用户实际输出为准。没有枚举到设备时，先保留完整输出并检查数据线、J-Link 软件/驱动和板卡供电，不能据此断言是哪一项失败。

`west topdir` 应指向所选 SDK 工作区，板列表应出现 `nrf54l15dk` 相关项。列表中有板名、版本命令符合期望或设备能够枚举，都不等于完整目标已经构建或烧录通过；应用核目标与构建使用的实际编译器还要在 1.4 节核实。

> 【截图 1-3 占位】nRF Connect 工具链终端、所选版本与实际检查输出；待用户实机采集。这里的 shell 终端用于执行 west；1.4.3 的串口终端用于接收板子输出，二者不同。

只有上述检查实际通过后，才继续构建示例。将资料期望值与真实版本、路径、报错及界面差异分栏保存并反馈，由 developer 按环境基线约定回填；T-005 已完成基线确认，不表示这台主机已安装或实验已通过。

### 1.4 第一个程序：blinky 上板

环境就位，现在把第一个程序跑上芯片。我们选用 Zephyr 官方示例 **blinky**——它是嵌入式世界的“Hello, World”：让一个 LED 周期性闪烁。这一步的目标不是理解代码（第 2 章剖析），而是**完整走一遍“创建工程 → 构建 → 烧录 → 观察”的循环**。

#### 1.4.1 实验 ex-blinky-first：完整的构建—烧录—观察循环

> **实验性质**：用户实机执行。下述界面、输出与现象均为**预期**（资料核查级；blinky 对本板级目标的支持经 Zephyr 官方板级文档核实，文档明确列出该板 LED 支持与构建示例）。

**第 1 步：选择 SDK 自带示例并创建构建目录**

沿用 1.3.4 节的 nRF Connect 工具链终端，确认选择的是 v3.4.0。下列命令使用 SDK 自带源码，不另外复制工程；先进入实际 SDK 根目录，示例路径不同则一并替换：

```text
cd C:\ncs\v3.4.0
west build -b nrf54l15dk/nrf54l15/cpuapp zephyr\samples\basic\blinky -d C:\ncs\build\blinky-first
```

逐段解释：

- `west build`：调用构建系统；
- `-b nrf54l15dk/nrf54l15/cpuapp`：板级目标（回忆 1.2 节的读法），**务必原样输入**；
- `zephyr\samples\basic\blinky`：示例源码相对 SDK 根目录的路径；
- `-d C:\ncs\build\blinky-first`：把构建产物放到 SDK 目录之外的独立目录，保持 SDK 源码干净。

**预期现象**：命令滚动输出 CMake 配置与编译日志，最终提示构建成功，产物位于 `C:\ncs\build\blinky-first\zephyr\` 下（`zephyr.elf` 等文件）。保存 CMake 日志中的 `Found toolchain` 路径与版本，和 1.3.4 的 `arm-zephyr-eabi-gcc --version` 输出对照；这两项实际记录尚未产生。

> 扩展也可以通过 “Create a new application” → “Copy a sample” 复制 blinky，再添加构建配置。但复制后的源码与构建目录会不同，不能混用本节直接构建 SDK 自带示例的路径。本轮使用扩展配置的终端执行上述命令，保持源码、构建目录与后续改参位置一致；GUI 字段和命令在所选基线中的实际行为仍待验证。

**第 2 步：连接 DK**

用 USB-C **数据线**把 DK 的调试器 USB 口连到电脑。第一次连接时：

- nRF Connect for Desktop 可能提示**板载调试器固件升级**，按提示完成即可（这是正常流程，不是故障）【待核：出厂固件行为待实机验证】；
- Windows 会识别出 J-Link 相关设备。

在同一工具链终端再次执行 `nrfutil device list`，确认本次连接的 DK 可以枚举；保存设备记录，但不要把序列号写入公开书稿。

> 易错点：务必使用**数据线**。只能充电的 USB 线是“电脑完全不认识板子”的最常见原因。

**第 3 步：烧录**

先确认第 1 步实际构建成功、第 2 步识别到目标板，并已解决 1.3.3 节的烧录后端依赖。`-d` 必须指向刚才成功构建的目录。先只查看这个构建目录记录的 runner 上下文：

```text
west flash -d C:\ncs\build\blinky-first --context
```

**预期检查点**：输出的 available runners 包含 `nrfutil`，default runner 是 `nrfutil`。如果默认项或可用项不同，保存完整输出并停止烧录，先核对板级目标、构建目录和工具链；不要为绕过检查而直接改用另一 runner。

上下文符合预期后，再在同一工具链终端执行：

```text
west flash -d C:\ncs\build\blinky-first
```

**预期现象**：west 使用默认 nrfutil runner，经 nRF Util `device` 命令与 J-Link 访问 DK，擦除需要更新的 RRAM 区域、写入镜像并复位芯片；日志提示编程完成【待核：runner 上下文、调用链与烧录日志的具体措辞均以实机为准；固定版本板级文件还将 J-Link 列为可用 runner】。

> 如果出现 `readback protection`（读保护）报错，先保存完整日志核对原因。官方板级文档列有 `--recover` 恢复选项；它会擦除设备内容，不是日常烧录必选项。确需恢复且已确认目标板内容可擦除时，命令还须带本例构建目录：`west flash -d C:\ncs\build\blinky-first --recover`。本项目尚未执行该操作。

**第 4 步：观察**

烧录完成后程序自动运行。

**预期现象**：DK 上的 LED 按固定节奏闪烁（blinky 默认每秒切换一次亮灭，即约亮 1 秒、灭 1 秒）【待核：具体是哪一颗 LED（LED1）以实机观察为准；蓝图预期为 LED1 约 1 Hz】。

如果实际构建、烧录成功，且观察到符合预期的闪烁，就可以把本次 **源码 → 镜像 → 芯片 → 可观察行为** 记录为用户执行成功。记录对应版本、命令、日志与现象；仅阅读到这里或只看到编译成功，都不代表上板验证已经完成。

#### 1.4.2 实验 ex-blinky-modify：确认你掌控这个循环

> **实验性质**：用户实机执行。预期现象为**预期**。

只做一次不算掌控。现在改一个参数，重走一遍循环。

1. 用 VS Code（或任意编辑器）打开 `C:\ncs\v3.4.0\zephyr\samples\basic\blinky\src\main.c`；
2. 找到定义闪烁间隔的宏 `SLEEP_TIME_MS`（默认值为 `1000`，单位毫秒），把它改成 `100`；
3. 在 1.3.4 节的工具链终端重新进入 SDK 根目录，再构建并烧录（路径按实际安装位置替换）：

   ```text
   cd C:\ncs\v3.4.0
   west build -b nrf54l15dk/nrf54l15/cpuapp zephyr\samples\basic\blinky -d C:\ncs\build\blinky-first
   west flash -d C:\ncs\build\blinky-first
   ```

**预期现象**：LED 闪烁明显变快（约每秒亮灭切换 10 次）。如果频率没有变化，检查：是否保存了文件？是否对同一个构建目录重新构建？烧录是否真的完成？

> 建议：改动 SDK 目录内的示例文件只是本章的权宜之计（方便起见直接在原示例上改）。从第 2 章开始，我们会把工程复制到独立目录再修改——这也是为什么上面的构建产物放在了 SDK 之外。

#### 1.4.3 可选实验 ex-hello-uart：认识板载调试器的虚拟串口

> **实验性质**：用户实机执行，可选。预期输出为**预期**（输出格式经 Zephyr 官方板级文档的 hello_world 示例核实）。

板载调试器还提供了一路 USB 虚拟串口——目标芯片的 `printk`/日志经它送到电脑。提前认识它，第 5 章讲日志时就有了直观印象。

1. 在 1.3.4 节的工具链终端构建并烧录 hello_world 示例（同样须先完成烧录依赖检查）：

   ```text
   cd C:\ncs\v3.4.0
   west build -b nrf54l15dk/nrf54l15/cpuapp zephyr\samples\hello_world -d C:\ncs\build\hello-uart
   west flash -d C:\ncs\build\hello-uart
   ```

2. 打开串口终端（例如扩展包中的 nRF Terminal 的串口连接功能），选择 DK 枚举出的 COM 口，参数 115200 8N1【待核：实际 COM 口、波特率及流控设置以基线配置和实机为准】。这里不是执行 west 的 shell 终端；
3. 按一下 DK 上的复位键（或重新上电）。

**预期现象**：终端输出类似 `Hello World! nrf54l15dk/nrf54l15/cpuapp` 的一行文本——程序名与板级目标名都在里面。

### 1.5 出问题去哪查：官方资料地图

最后，把“弹药库”交给你。嵌入式开发查资料是常态，关键是知道**哪类问题查哪份资料**：

| 资料 | 管什么 | 什么时候翻它 |
|---|---|---|
| **nRF54L15 芯片数据手册**（v1.0） | 芯片的权威定义：寄存器、电气参数、外设行为 | 写驱动、查引脚、确认硬件能力（如“有没有 I3C”） |
| **nRF54L15 DK 用户指南**（v1.0.0） | 这块**板子**的硬件：跳线、接口、供电、电流测量 | 接线、改供电、用 P6 测电流 |
| **勘误表（Errata）** | 芯片特定修订版的已知硬件缺陷与规避方法 | 现象与手册不符时，先查勘误再怀疑代码 |
| **NCS 在线文档**（docs.nordicsemi.com） | 安装指南、API 文档、示例说明、版本发布说明 | 装环境、查 API、看示例 |
| **Zephyr 官方文档**（docs.zephyrproject.org） | Zephyr 内核与驱动 API、板级支持列表、west 手册 | 查 Zephyr 层的一切 |
| **Nordic DevZone** | Nordic 官方技术社区，工程师在线答疑 | 资料查不到的疑难杂症 |

**演示：按板卡版本查勘误。** 勘误表是按芯片修订版（revision）发布的，查错版本可能误导排查。本书的 DK 1.0.0 上焊接的是 **Rev 2** 芯片（丝印 nRF54L15-QFAAC00），对应的勘误表是 **Rev 2 Errata v1.1**。查法：在 Nordic 官网进入 nRF54L15 产品页 → Downloads/文档区 → Errata，选择与你芯片修订版一致的文件。芯片修订版可以从芯片丝印或 DK 用户指南中确认。

本项目已保存的资料见 [workspace/nrf54l15-dk-docs/README.md](../../../../workspace/nrf54l15-dk-docs/README.md)：[数据手册 v1.0](../../../../workspace/nrf54l15-dk-docs/nRF54L15_nRF54L10_nRF54L05_Datasheet_v1.0.pdf)、[DK 用户指南 v1.0.0](../../../../workspace/nrf54l15-dk-docs/nRF54L15_DK_HW_User_Guide_v1.0.0.pdf)、[Rev 2 勘误表 v1.1](../../../../workspace/nrf54l15-dk-docs/nRF54L15_Rev_2_Errata_v1.1.pdf)。查询时先核对文件版本，在线最新版不能自动替代本书基线。

把这张地图存好。从下一章开始，每章的“来源与延伸阅读”都会告诉你该章结论来自哪份资料的哪个位置——这也是本书的写作约定：事实性主张必有可追溯来源。

## 读者可见的限制

本节列出本版技术复核稿的限制与证据边界。developer 对 v0.3 的必要复核完成前，本版不作为交给用户完整通读的可读稿；安装和实验实际通过前，也不把它标为已验证的实验稿：

1. **目标基线已批准，实际环境待验证**。A-005 v0.3 已由 D-007 批准，旧“基线未确认”标记核销；具体安装版本、路径和构建烧录结果仍待回填。基线变更时重新评估相关步骤和证据。
2. **安装与终端步骤尚未实机执行**。本轮使用 NCS v3.4.0 固定版本源码文档核对安装、默认 runner 与版本文件；VS Code 扩展界面仍按其当前文档描述，会随扩展版本变化。入口、界面、磁盘占用、J-Link/nRF Util/device/west 实际版本、GCC 路径及基线匹配仍需用户执行后核对，具体限制见各处【待核】及 1.3.3～1.3.4。
3. **图像仍有缺口**。图 1-1、1-2 尚待绘制，图 1-3 已改为自制文本示意；截图 1-1～1-3 待用户实机采集。它们尚未满足最终图文交付要求。
4. **全部实验现象均为“预期”**。ex-env-toolchain-check、ex-blinky-first、ex-blinky-modify、ex-hello-uart 均未实机执行；预期现象的依据已在各实验处注明（官方文档核查）。你照做后的实际结果将以“用户执行”记录，预期与实际不一致时按反馈流程修订。
5. **板载调试器固件升级提示未验证**。出厂 DK 连接 NCS v3.4.0 环境时是否提示升级，待实机确认。
6. **资料包已可访问，全文技术审校尚未完成**。资料包现位于项目 `workspace/nrf54l15-dk-docs/`；本轮读取索引并核对相关 PDF 文件存在，核销“资料包缺失”。这不等于逐页重查硬件事实；硬件细节仍沿用 A-001/A-003 的来源记录，本轮未重新核对 PDF 原文、引脚或图号。
7. **第 1 轮问题已修订，尚待 developer 必要复核**。developer 对 v0.2 的结论为 `revise`；本版 v0.3 已按 R1-001 明确默认 nrfutil/device + J-Link 烧录链，并按 R1-002 删除 `west sdk-version`、拆分版本检查。A-005 v0.3 的差异已在 T-006 登记协调，未改写已批准成果。developer 对 v0.3 的相关命令、预期值、失败分支和边界措辞复核完成前，不进入 D-008 的用户完整通读；通读完成也不等于实验通过或章节获批。

## 回顾与自检

读完后先检查理解；实际完成实验后，再检查操作结果。以下是学习目标，不是本项目已通过的记录：

- 建立了 nRF54L15 的架构地图（M33 应用核 + FLPR 协处理器、RRAM/RAM、电源域、外设清单），并认识了 DK 实物上的关键位置；
- 分清了 Zephyr、NCS、west、工具链、IDE 扩展各自的角色，并在 Windows 上从零装好了 NCS v3.4.0 环境；
- 用 blinky 走通了“构建 → 烧录 → 观察”循环，并通过修改闪烁周期确认了自己对它的掌控；
- 拿到了官方资料地图，知道哪类问题查哪份资料、如何按芯片修订版查勘误。

**自检**（按解释、操作、变式三个层次，请如实记录自己的表现，不必全部做到）：

- 解释：不看 1.2 节，说出 NCS 与 Zephyr 是什么关系？west 在其中管什么？`nrf54l15dk/nrf54l15/cpuapp` 三段各指什么？
- 解释：数据手册、DK 用户指南、勘误表分别回答哪类问题？为什么查勘误要先确认芯片修订版？
- 操作：从 nRF Connect 工具链终端执行 1.3.4 的检查，说出各命令能验证什么、不能证明什么；未执行就记录未执行。
- 变式：把 blinky 的闪烁改成“快闪 5 次、停 2 秒”的节奏（提示：需要改动 main.c 里的循环结构而不只是宏）。做不到没关系——第 2 章剖析完工程结构后回来再试。
- 变式：在不查 1.5 节表格的情况下，回答“LED 不闪”时你会按什么顺序排查？（参考顺序：USB 是否数据线 → 板级目标是否写对 → 构建是否成功 → 烧录日志是否完成 → 勘误表。）

## 来源与延伸阅读

本章关键主张的来源（写作约定：事实性主张可追溯；标注“资料核查级”的结论未经实机执行）：

- nRF54L15 SoC 组成（M33 + FLPR、1.5 MB RRAM、256 KB RAM、外设清单）：芯片数据手册 v1.0，经 A-001 v0.2 与 A-003 v0.1 记录核查（资料核查级）；板级硬件特性另经 Zephyr 官方板级文档 nRF54L15 DK 页核实（2026-09-20）：<https://docs.zephyrproject.org/latest/boards/nordic/nrf54l15dk/doc/index.html>。
- “无 I3C 控制器”：数据手册 v1.0 全文检索（I3C/MIPI 零命中），检索过程与结论记录于决定 D-002。
- NCS v3.4.0 为 Nordic 首个 LTS、基于 Zephyr 4.4：Nordic 官方博客，经 A-001 v0.2 与 D-005 记录（资料核查级）。
- 安装与终端流程：A-005 v0.3、D-007、[NCS v3.4.0 固定版本安装文档](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/installation/install_ncs.rst)，以及 1.3.4 链接的 VS Code 扩展终端文档（writer 于 2026-09-26 在线读取，资料核查级）。固定版本文档确认扩展的 Install SDK 路径、独立 J-Link prerequisite，以及扩展随工具链提供部分 nRF Util 命令；纠正 v0.1 对 Toolchain Manager 旧流程“仍然有效”的表述。
- 默认烧录链与备选 runner：[NCS v3.4.0 编程文档](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/app_dev/programming.rst) 说明 NCS v3.0.0 起 `west flash` 默认使用 nRF Util，v3.1.0 起 `device` 命令进入工具链 bundle，`west flash --context` 可显示 available/default runner；nRF Command Line Tools 仅在显式 `-r nrfjprog` 时需要。nRF54L15 DK 的[固定版本 board.cmake](https://github.com/nrfconnect/sdk-zephyr/blob/ncs-v3.4.0/boards/nordic/nrf54l15dk/board.cmake) 先载入 nrfutil、再载入 jlink，支持本章的默认项判断（资料核查级）。A-005 v0.3 的不同表述保留原文，差异与协调方式记录于 T-006 R1-001。
- 版本分层检查：[NCS v3.4.0 发布说明](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/doc/nrf/releases_and_maturity/releases/release-notes-3.4.0.rst) 给出 manifest 标签 `v3.4.0` 及 Zephyr SDK 1.0.1；[固定版本 west manifest](https://github.com/nrfconnect/sdk-nrf/blob/v3.4.0/west.yml) 将 Zephyr 锁定为 `ncs-v3.4.0`；该 Zephyr 修订的 [`SDK_VERSION`](https://github.com/nrfconnect/sdk-zephyr/blob/ncs-v3.4.0/SDK_VERSION) 内容为 `1.0.1`。GCC 14.3.0 目标沿用 A-005 v0.3 与 developer 的 R1-002 复核记录；实际编译器、west、nRF Util 和 `device` 版本仍须分别由用户输出回填。
- `west topdir` 的工作区检查用途：[Zephyr west 内置命令文档](https://docs.zephyrproject.org/latest/develop/west/built-in.html#other-built-in-commands)（2026-09-26 查阅）；输出不能证明工作区内所有仓库的版本或构建结果。
- 板级目标 `nrf54l15dk/nrf54l15/cpuapp`、blinky/hello_world 构建与烧录命令、readback protection 与 `west flash --recover`：[NCS v3.4.0 锁定的 Zephyr 板级文档](https://github.com/nrfconnect/sdk-zephyr/blob/ncs-v3.4.0/boards/nordic/nrf54l15dk/doc/index.rst)（资料核查级）；hello_world 终端输出格式沿用初稿的同页来源。本版命令与边界措辞仍待 developer 对 v0.3 必要复核。
- DK 部件位置（按键/LED/USB/P6/排座）、勘误表对应关系（Rev 2 Errata v1.1）、板载调试器与 VDD 1.8 V 默认值：DK 硬件用户指南 v1.0.0，经 A-001 v0.2 与 A-003 v0.1 记录核查（资料核查级）。
- 易错点（安装路径、数据线、调试器固件升级提示）：Nordic 官方安装文档、DK 用户指南与社区常见问题，经 A-003 v0.1 记录（资料核查级）。
- 延伸阅读：Nordic DevAcademy 免费课程（academy.nordicsemi.com，nRF Connect SDK 基础课程）可作为本章的并行学习材料；本书不复制其课程结构。

下一章（第 2 章 ch-build-system）将剖析本章创建的 blinky 工程：CMake 如何组织构建、devicetree 如何描述硬件、Kconfig 如何裁剪功能——届时请带上你的 Linux 内核设备树与 Kconfig 经验。
