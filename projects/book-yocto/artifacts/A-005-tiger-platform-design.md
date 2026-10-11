# tiger 平台设计说明

```text
项目：book-yocto
成果 ID：A-005
负责人：architect
关联任务：T-016；跨章影响供 T-017 使用，T-017 尚未启动
版本：0.1
更新日期：2026-10-11
状态：draft
适用范围：chapter 4 最小平台设计候选；chapter 2、6–10、15 的接口与后续能力约束
来源及输入版本：T-016 平台设计前置安排（OPC ba4e22e7854dcb0fa09adeb688af9c0a14b45a07）；A-002 v0.8、A-003 v0.1、A-004 v0.5；D-001/D-007/D-010/D-011/D-013；书稿 f444790f352b46f699fd3d46198f8b9b9f089a62 的相关章节；自身及两个工程成员的当前 scope
批准依据：尚无；设计任务已授权，本文候选值、接口选择和验证路线尚待专业核对及用户确认
版本记录：2026-10-11 v0.1 首次候选稿，整理书稿约定、提出最小布局与启动路线、登记缺口和验证条件
```

## 1. 本版结论与使用条件

建议先实现一个可执行最小测试固件的 tiger ARMv8-A 平台：Cortex-A53 四核、1 GiB RAM、固件装载区、启动 SRAM、GIC、通用定时器、PL011 串口；PL031 RTC 可作为同阶段的复用设备核对。chapter 4 交付真实 machine patch、模型证据、Yocto 正式集成及清理临时 workspace 后的干净复验。NAND、完整 TF-A 启动链、U-Boot、Linux、watchdog、SPI NOR 和 EEPROM 分阶段接入，提前明确接口和保留地址。

本文的地址、中断编号、时钟与启动模式是 **architect 提出的候选设计**，不是从现有 tiger 实现提取的事实，也不是已经批准的实施基线。书稿中“已跑通”“已交付”的叙事和示意日志不能证明工程能力存在。本文实际完成的是资料定位、checkout 核对、设计整理和静态算术检查，未执行 QEMU、BitBake、固件或 guest 测试。

本章主要确认门槛是：实际配方与上游基线、最小地址/中断/时钟契约、固件执行及复位模式、可实际执行的 runqemu 输入。chapter 4 旧稿只有 machine 列表与 `-S` 暂停实例化，并明确把完整运行留给以后；这不足以直接满足当前 T-016/D-013 的 runqemu 验证要求。§5 给出补齐路线，不能未经核对就把启动要求降为静态参数展示。

## 2. 资料与版本证据

### 2.1 本轮实际核对

- OPC：`D:/opc-book-yocto`，当前为 `feature/book-yocto/t-016-chapter4-revision`，开始时 HEAD 为 `ba4e22e7854dcb0fa09adeb688af9c0a14b45a07`；工作树无未提交项，`project/book-yocto` 是其祖先。本轮未联网刷新远端引用。
- 书稿：`D:/book-yocto`，远端为 `git@github.com:yinzhongkai/book-yocto.git`，当前分支为 `feature/t-016-chapter4-revision`，HEAD 与本机 `main` 均为 `f444790f352b46f699fd3d46198f8b9b9f089a62`，与 WORKSPACE 锁定版本一致；工作树无未提交项。读取正文前已核对，未修改该仓库。
- `meta-tiger`：只读取 WORKSPACE 声明，`host: tiger`，checkout `/home/oops/workspace/meta-tiger`，目标 `master`，声明 revision 为 `0e3da48b34ef772532535a730a21878cc30ccf67`。本轮未 SSH、读取其文件或核对当前工作树；历史核对不能冒充本轮核对。
- 本机书稿仓库的文件扫描未取得 Poky 配方、`runqemu`、QEMU machine 源码、TF-A 平台源码或 tiger DTS。未取得资料保持待核实，不从书稿中的源码节选推导真实 checkout。
- 身份、角色与有效技能已刷新；与初始化时 OPC `737d1d78fdc858c60e81b8eace9f8c1ede59a804` 比较，根规则、角色、四项有效技能、DECISIONS、A-003/A-004 未变，复用已完整读取的正文。

### 2.2 书稿定位与读取范围

下表路径均为 `book-manuscript` 仓库内路径，统一对应提交 `f444790f352b46f699fd3d46198f8b9b9f089a62`。本轮先扫描章节标题与平台关键字，再读取下列相关正文区间；未声称全文审校全部章节。

| 来源 ID | 仓库内路径 | 已读取的相关区间 | 提取内容 |
|---|---|---|---|
| S-02 | `yocto/task03-2-读懂这个项目.md` | L1–278、L570–631，重点 §2.1/2.2/2.7 | 四核 A53、硬件清单、目标启动链、BSP 边界；SPI NOR 型号仍是候选 |
| S-04 | `yocto/task05-4-让QEMU长出tiger这块板.md` | L1–175、L436–675，重点 §4.1/4.5/4.6 | machine 命名、旧 patch 叙事、QEMU 配方目标、暂停冒烟与 runqemu 输入缺口 |
| S-06 | `yocto/task07-6-集成TF-A：让芯片_上电_.md` | L1–97、L438–534、L708–748 | BL1/BL2/BL31、`-bios`、`NEED_BL33=no`、FIP 与未交付的装载方案 |
| S-07 | `yocto/task08-7-集成U-Boot：让板子_会启动_.md` | L1–53、L409–490、L649–702、L732–802 | standalone U-Boot、两份 DTS、boot.cmd、CPU/RAM/QB 参数 |
| S-08 | `yocto/task09-8-集成LinuxKernel：让板子_活过来_.md` | L355–460、L707–755 | DTS 占位、Image/DTB 直通、串口及 PSCI 假设 |
| S-09 | `yocto/task10-9-NAND+UBI：把_硬盘_建好.md` | L198–262、L293–342、L433–530、L601–638 | NAND 几何、UBI 参数、三段三卷、控制器与后端占位 |
| S-10 | `yocto/task11-10-打通启动链：第一次开机成功.md` | L1–335、L392–437 | 原始 NAND 总装、BL2/FIP 偏移、装载地址、runqemu 全链和 PSCI |
| S-15 | `yocto/task16-15-修一个上游bug.md` | L1–55、L183–345 | watchdog 地址/时钟、示意寄存器、人为简化驱动及待核行为 |

### 2.3 依据的效力

| 内容 | 现有依据 | 本版处理 |
|---|---|---|
| 真实代码、全链构建与运行、C-W/V 回填 | D-001、D-010、D-013 confirmed | 保持验收目标；设计说明不能替代运行证据 |
| 上游版本由配方锁定，长期修改进入 `meta-tiger` | D-013 confirmed | 不创建四个长期 fork；临时 workspace 不是交付物 |
| Cortex-A53 四核、1 GiB RAM、PL011/PL031、目标启动链 | S-02/S-07 的文本要求 | 用作候选输入；章节定稿不证明设备已实现 |
| QEMU 8.2.7、TF-A 2.10.4、U-Boot 2024.04、Linux 6.6 | 相关草稿与 A-001 的版本描述 | 不是实际选中配方的证据；精确源码版本、已有 patch 与展开结果待工程核对 |
| NAND 2 KiB/128 KiB/512 MiB，watchdog 32768 Hz | S-09/S-15 与 A-003 C-W19/C-W30 | 条件性设计输入；不存在本轮芯片手册、模型或实测背书 |
| 文中“D5 拍板”、原 blueprint、四个 fork 的提交示例 | 草稿叙事/未取得的原工作流资料 | 不当作本项目已确认决定或真实提交；当前决定编号以 DECISIONS 为准 |

## 3. 能力阶段与最小资源

| 阶段 | 应具备的能力 | 本版实现/验证事实 |
|---|---|---|
| chapter 4 最小 machine | 注册 `tiger`；A53 四核对象、1 GiB RAM、ROM/SRAM、GIC、timer、PL011；可执行受控固件并复位再执行 | 未读取 tiger 工程源码，未验证；本版只是候选 |
| chapter 4 RTC 扩展 | 复用 PL031，确认时钟、IRQ、reset；若纳入 patch 则补模型验证 | 未实现/未测试；是否同阶段纳入待确认 |
| chapter 6/7 | TF-A 逐级装载与 standalone U-Boot；阶段边界及缺失 BL33 的真实终态 | 未验证；须补启动模式和 IO 契约 |
| chapter 8/9 | Image/DTB 直通、NAND 控制器与 guest 驱动、UBI/UBIFS 测试 | 未验证；按对应阶段接入 |
| chapter 10 | 固件启动、FIP/BL33、PSCI 多核、完整 NAND 布局至 rootfs | 未验证；不是 chapter 4 的完成声明 |
| 后续设备/教学实验 | watchdog、SPI NOR、I2C EEPROM；网络相关能力另核 A-003 C-W26/C-W29 | 未验证；不因保留地址就宣称具备能力 |

默认资源候选：TCG 执行，AArch64 Cortex-A53，四核，RAM 1024 MiB。首个实现只接受这组资源，其他 CPU、核数、RAM 大小及 KVM 支持须明确拒绝或另有经过测试的能力声明，不能静默接受后使用固定地址表。CPU0 执行，其余核在固件模式下保持待唤醒状态；具体 reset/power 状态由目标 QEMU 版本的 CPU 接口核对。

## 4. 候选地址、中断与时钟契约

### 4.1 物理地址表

以下统一使用字节地址与半开区间 `[base, end)`。未启用的保留窗口不得伪造设备寄存器响应。所有数值为本版新提案，只有 S-10 的三处 U-Boot 地址与 S-15 的 watchdog 基址具有现有草稿来源。

| 区域/设备 | base | end（不包含） | 大小/用途 | 阶段与来源 |
|---|---|---|---|---|
| 启动 ROM | `0x00000000` | `0x00100000` | 1 MiB，测试固件/BL1 装载窗口 | chapter 4，候选；镜像超过窗口必须报错 |
| GIC distributor | `0x08000000` | `0x08010000` | 64 KiB，候选 GICv2 地址窗口 | chapter 4，实际模型映射长度待核 |
| GIC CPU interface | `0x08010000` | `0x08020000` | 64 KiB，CPU 中断接口窗口 | chapter 4，候选 |
| PL011 UART0 | `0x09000000` | `0x09001000` | 4 KiB，主串口 | chapter 4，候选 |
| PL031 RTC | `0x09010000` | `0x09011000` | 4 KiB | 候选同阶段复用 |
| SPI controller | `0x09020000` | `0x09021000` | 4 KiB 保留；控制器型号未选定 | 后续；不等同 NOR 芯片本身地址 |
| watchdog | `0x09030000` | `0x09031000` | 4 KiB 保留 | 后续；基址取 S-15，长度候选 |
| I2C controller | `0x09040000` | `0x09041000` | 4 KiB 保留；总线/EEPROM 地址待定 | 后续 |
| NAND controller | `0x09050000` | `0x09060000` | 64 KiB 保留；具体寄存器契约待选型 | chapter 9；不得据此填写虚构 compatible |
| 启动 SRAM | `0x0e000000` | `0x0e100000` | 1 MiB，BL2/早期栈工作空间候选 | 为 TF-A 保留；实际 linker 布局待核 |
| DDR RAM | `0x40000000` | `0x80000000` | 1 GiB，四核共享主存 | chapter 4，基址为候选，容量取 S-07 |

选择 GICv2 是减少首轮平台组合的候选取舍；不复制 qemuarm64 的 `virt`/GICv3/KVM 配置来证明 tiger 兼容。两名工程成员须核对锁定版本模型、CPU timer 接线和 guest 驱动是否支持该组合。

QEMU 中 RAM 从创建时可访问是模拟平台行为；TF-A 后续可准备映射和软件状态，不能据此声称真实 DDR 训练或硬件时序经过模拟/验证。低地址 ROM 与 SRAM 并非具备实际 SoC Boot ROM 算法的承诺。

### 4.2 RAM 使用与镜像边界

| 用途 | 候选范围 | 契约 |
|---|---|---|
| BL31/安全运行时预留 | `[0x40000000, 0x41000000)`，16 MiB | 后续 TF-A 确认占用；Linux DTS 应表达保留区，且与固件实际布局一致 |
| 脚本暂存 `loadaddr` | `[0x41000000, 0x42000000)`，16 MiB | 起点来自 S-10；检查 boot.scr 实际长度，执行完后生命周期结束 |
| 内核输入 `kernel_addr_r` | `[0x42000000, 0x46000000)`，64 MiB | 起点来自 S-10；镜像、运行时占用及解压/搬运范围须分别核对 |
| DTB `fdt_addr_r` | `[0x46000000, 0x46200000)`，2 MiB | 起点来自 S-10；校验总大小、对齐及后续 fixup 所需余量 |
| BL33/U-Boot 初始装载 | `[0x48000000, 0x4a000000)`，32 MiB | 新候选；link 地址、TF-A handoff 与 standalone 装载使用同一份定义 |

这些是分阶段装载预算，不是 Linux 永久划分的内存分区。U-Boot relocation、栈、malloc、内核输入及最终运行范围须有按阶段的占用图；不能仅靠“几个入口地址不同”证明不会覆盖。BL31 常驻区必须从普通世界可用 RAM 中排除。DTS `reserved-memory` 只表达软件保留，不能单独证明 QEMU 已实现安全世界访问隔离；若后续宣称隔离，须核对 address space 与安全访问控制。

### 4.3 中断拓扑

候选 GIC INTID 分配：UART0=32、RTC=33、watchdog=34、SPI controller=35、I2C controller=36、NAND controller=37。保留项尚不接线。设备外部中断拟用 level-high，须由实际模型核对 assert/deassert、屏蔽/清除语义；不能把这个拟定触发方式套到模型的所有内部事件。

通用定时器候选保留 INTID 26/27/29/30，具体 timer 输出、PPI 与安全/非安全世界对应关系须回到锁定 QEMU 和 guest binding 核对。INTID、QEMU 接口编号与 DTS 编号分开记录：例如候选 UART INTID 32 对应 GIC SPI 索引 0，而不是在 DTS 的 SPI 索引字段再写 32。最终 DTS 还须明确类型与 flags，经过模型核对后才生成。

四核分别连接 GIC CPU 接口和各自 timer PPI；设备 SPI 接到共同 distributor。验证须观察 GIC 待决/撤销和 guest handler，QTest 寄存器读写不能代替 CPU 实际进入中断处理。

### 4.4 时钟与复位

候选固定时钟：系统/通用 timer 24 MHz；PL011 UART 时钟与 APB 时钟各 24 MHz；watchdog 后续独立 32768 Hz（S-15 文本输入）。PL031 的计时源/宿主与虚拟时间选择依据上游模型核对，不把“秒计时”误填成未经核对的模型输入接口。SPI/I2C/NAND 控制器时钟待选型，不复制 UART 值充数。TF-A timer 初始化、DTS clock provider 和模型输入必须引用同一份最终数值。

chapter 4 的系统复位契约候选：停止并清理本轮 timer/IRQ 状态，设备进入对应上游模型 reset 值，CPU0 重新进入启动入口，其余核回到待唤醒状态；ROM 保持，RAM 在 warm reset 时不承诺清零，测试固件必须自行初始化。冷启动与 warm reset 分别验证，不能以重新启动 QEMU 进程替代 guest 请求的系统复位。后续 NAND 数据应在普通复位后保留，watchdog 复位须进入相同系统复位链。复位原因寄存器尚未设计，不宣称可读取原因。

## 5. 启动模式及 chapter 4 的 runqemu 缺口

### 5.1 显式启动模式候选

建议把装载模式作为显式 machine 契约，避免根据文件名猜 `bl1.bin` 和 `u-boot.bin` 的入口。以下 `firmware`、`standalone-uboot`、`direct-linux` 是设计名称，**不是已经存在的 QEMU 属性或可复制执行的命令**；属性拼写、默认值、错误信息及 `-bios`/`-kernel` 行为由模型实现核对后确定。

| 模式 | 输入/装载 | 初始执行状态候选 | 后续服务与验证范围 |
|---|---|---|---|
| `firmware`（建议默认） | `-bios` 对应启动 ROM base `0x0`；chapter 4 用最小测试固件，chapter 6 起可用 BL1 | CPU0 AArch64 secure EL3，reset PC `0x0`；其余核待唤醒 | chapter 4 测固件执行、UART、reset；TF-A 后续提供 PSCI，QEMU 不同时假扮另一份 PSCI |
| `standalone-uboot` | 原始 U-Boot binary 装入候选 `0x48000000`；固件 link 地址及内嵌控制 DTS须匹配 | CPU0 non-secure EL2；secondary 不自动进入任意镜像入口 | 验 U-Boot 命令行；不证明 TF-A handoff、完整 SMP 或 NAND 启动 |
| `direct-linux` | Image、显式 DTB、bootargs；装载位置/寄存器协议按锁定版本 ARM loader 与 Linux 入口核对 | 推荐 non-secure EL2 路线，具体寄存器、MMU/cache 状态待版本依据 | chapter 8/9 可用；若启用 QEMU 提供的 PSCI，须与 DTS 匹配并区别于固件模式的 BL31 PSCI |

不同模式输入混用应明确报错；无镜像的 `-S` 只用于实例化，恢复执行前必须具备有效入口内容。不要让未装载的 ROM/RAM 零内容表现成成功启动。固件模式与直接内核模式中 secondary CPU 的唤醒提供者分别核对；S-07/S-08 的“没有 BL31 所以一定单核”只是旧稿假设，不能覆盖实际 QEMU loader/PSCI 行为。

### 5.2 runqemu 候选路线

首选候选 A：给 chapter 4 提供 **最小测试固件及测试启动元数据**，通过实际 `qemu-system-native` 产物和 runqemu 执行 CPU0 UART 标记/复位测试。该固件是模型验证输入，不冒充 TF-A、Linux 或量产 rootfs；将来由工程成员按已确认契约制作，长期测试资产随 `meta-tiger` 管理。本轮只设计其用途。

Yocto 成员须先从锁定版本确认 runqemu 能消费的 qemuboot 配置、部署输入与固件路径：禁用默认 `-kernel` 的方式、是否要求 rootfs、`QB_DEFAULT_BIOS` 或追加参数的实际路径处理、helper sysroot 中使用的二进制，以及普通 `QB_*` 默认 virtio/PCI/网络参数是否会误注入 tiger。S-10 的 `QB_DEFAULT_KERNEL="none"` 也明确待验证，不能照抄即认为路线成立。不能仅手写一份无法被 runqemu 接受的 INI 文件充作验证。

备选候选 B：若 A 在锁定 runqemu 中不成立，使用明确版本的最小 Linux/DTB 测试输入。须先评估是否需要提前形成 chapter 5/8 的部分支持、编译和设备树工作；不能用 `virt` DTB 或 qemuarm64 的启动日志代替 tiger 的系统运行。此路线对阶段范围和教学次序有影响，交用户确认后才能采用。

本版推荐先核对 A，避免 chapter 4 同时承担后续全链实现。A/B 均未通过可行性核对；若都不能满足既有任务条件，则保持启动缺口并协调范围，不能只以 `-machine help`、`-S` 或 runqemu 报错将本章标为完成。`nographic` 所指的 chardev 接线必须绑定 UART0；尚无 NIC 的 tiger 不自动承诺 `slirp` 可用。

## 6. 跨组件契约

| 交接面 | 候选契约及必须补齐的内容 | 证据/影响 |
|---|---|---|
| QEMU → 测试固件/BL1 | ROM 入口、初始 EL/security、CPU0/secondary 状态、UART、timer、reset；镜像长度检查 | S-04/S-06；chapter 4/6；实际模型接口待核 |
| BL1 → BL2 | BL2 来源、NAND/备用介质、偏移、加载 SRAM 地址、入口、镜像大小和读取失败终态 | S-06/S-10；`-bios bl1.bin` 只提供 BL1，不能凭此得到 BL2 |
| BL2 → BL31/BL33 | 读取 FIP、校验及实际包内容；BL31 常驻地址、BL33 link/load/entry、参数/EL；不得额外加载一份已在 FIP 中的 U-Boot | S-06/S-07/S-10；TF-A IO driver、linker 和生成物待核 |
| 无 BL33 的阶段运行 | 明确缺少 BL33 时允许的构建配置、真实终态与成功条件 | S-06 `NEED_BL33=no` 及“准备移交后停住”未实测；不能要求虚构日志 |
| BL31 → U-Boot/内核 PSCI | PSCI 版本、conduit、CPU affinity、CPU_ON/复位入口；固件模式只认 BL31 服务；DTS 同步 | S-10；启动四核对象不等于四核 Linux online |
| U-Boot → Linux | Image/DTB 实际部署名和 `/boot/` 路径；booti 参数；输入镜像不覆盖 U-Boot runtime/BL31；`console=ttyAMA0,115200` | S-07/S-08/S-10；S-10 的 `ttyS0` 是故障示例，不能作为最终契约 |
| U-Boot DTS ↔ Linux DTS ↔ QEMU | 分别维护组件 DTS，但 CPU affinity、地址、IRQ、clock、RAM/保留区来自同一平台表；各自校验 binding | S-07/S-08；`tigersemicon,tiger` 和 vendor 目录为候选，未验证已有 binding |
| Linux → NAND/UBI rootfs | 控制器驱动内建、真实兼容串、三段 MTD 名称、UBI 卷名、root 参数；不能用空框 DTS 替代驱动 | S-09/S-10；C-W17–C-W21 |
| 工程 → Yocto 集成 | 实际配方基线+已带 patch+新增 patch 顺序、相关 `.bbappend`/`SRC_URI` 与配置、清理 workspace 后复验 | D-013；配方文件名/PV 不能代替下载校验和或源码提交 |

BL2/FIP IO 候选：沿用 S-10 的 NAND 启动介质，由真实控制器访问约定偏移，并由 TF-A 平台 IO 实现读取；不让 QEMU 把 BL2/FIP 自动塞进 RAM 来伪装固件加载成功。chapter 6 的验证若先使用其他明确测试介质，须标注仅覆盖分段启动，并与后续 NAND 全链区分；具体介质/参数尚未选定。

TF-A 的 BL1 位置、SRAM、EL3 常驻区和 BL33 load/entry 必须由最终平台 linker 配置对账。U-Boot standalone 的控制 DTS 是否内嵌、是否另需 DTS输入，应以真实 `OF_EMBED`/`OF_SEPARATE` 和 binary 布局核对；只出现 `u-boot.bin` 文件不能证明 `-bios` 可跑。

## 7. NAND、分区与镜像约束

### 7.1 几何与控制器

沿用 S-09 的候选几何：页数据 2048 B、64 页/块、擦除块 131072 B、数据容量 536870912 B（512 MiB、4096 块）。页 OOB 大小、ECC、子页、芯片 ID、坏块标记及 QEMU 后端数据格式均未核实。控制器地址窗口见 §4，仅是保留空间；控制器型号/寄存器/中断、QEMU NAND 芯片模型与 TF-A/U-Boot/Linux 驱动组合待选型。

优先核对锁定上游中能复用的 NAND 芯片模型与有 guest 驱动的控制器组合。只有芯片模型不等于有可访问的板级 NAND 控制器。若没有可复用组合，新增控制器及 guest driver 是明确的后续工程依赖，需用户/项目经理在相应阶段安排，不能用一个占位 compatible 或始终返回成功的 MMIO 桩解决。

逻辑数据容量与后端文件长度分别记录。512 MiB 纯数据镜像直接 `dd` 的流程，仅在后端确实接受不含 OOB 的线性数据布局时成立；若存储页包含 OOB/ECC 或需要专门转换，必须改总装格式和长度检查。擦除态 `0xFF`、编程/擦除/坏块行为须实测，不从镜像大小推导几何正确。

### 7.2 候选逻辑数据偏移表

下表继承 S-09/S-10 的纸面布局，偏移位于 NAND **逻辑数据区**，不是 CPU MMIO 地址，也不一定等于含 OOB 后端文件偏移。

| 区域 | `[offset, end)` | 容量 | 接口要求 |
|---|---|---|---|
| bootloader 总区 | `[0x00000000, 0x00400000)` | 4 MiB / 32 块 | 固件布局与下面子区一致 |
| BL1 子区 | `[0x00000000, 0x00040000)` | 256 KiB / 2 块 | QEMU 外部装入的 BL1 与镜像内 BL1须为同一版本；当前不能据此声称模拟了 Boot ROM 从 NAND 读取 BL1 |
| BL2 子区 | `[0x00040000, 0x00080000)` | 256 KiB / 2 块 | BL1 实际读取偏移及长度检查 |
| 保留子区 | `[0x00080000, 0x00100000)` | 512 KiB / 4 块 | 不作为未知镜像的隐式落点 |
| FIP 子区 | `[0x00100000, 0x00400000)` | 3 MiB / 24 块 | 检查 FIP 长度，明确包含 BL31/BL33 的阶段版本 |
| env | `[0x00400000, 0x00500000)` | 1 MiB / 8 块 | U-Boot 环境保存、CRC/默认值/坏块/冗余策略待设计验证 |
| nand0 | `[0x00500000, 0x20000000)` | 507 MiB / 4056 块 | label 为 `nand0`，U-Boot、Linux、root 参数一致 |

静态算术确认：全部边界按 `0x20000` 对齐，bootloader 子区合计 32 块，三大区合计 4096 块。总装还须按每个子区检查输入长度；仅检查最终文件为 512 MiB 会漏掉超大 FIP 覆盖 env 的错误。

### 7.3 UBI/UBIFS 与 A/B

在“页 2048、VID offset 2048、数据 offset 4096”的条件下，候选 LEB 为 `131072 - 4096 = 126976 B`。S-09 的 `-m 2048 -p 131072 -s 2048 -O 2048` 与 `mkfs.ubifs -e 126976` 保持为待模型和工具验证的组合。可用块数不能直接把 S-09 的 `4056 - 2 - 80 = 3974` 当成实际终态：坏块、UBI内部预留和配置会改变结果，须使用真实 attach 日志与工具结果对账；`-c 3968` 同样待核。

S-10 候选 `rootfs_a`/`rootfs_b` 各 64 MiB，按 126976 B/LEB 向上取整各为 529 LEB；`data` 使用剩余空间，只有它拟带 autoresize。镜像需实际包含 `/boot/Image`、`/boot/tiger.dtb`、`/boot/boot.scr`，并核对文件系统余量及镜像最小需求；529 LEB 的算术成立不证明文件能装下。

启动接口候选：`bootpart=a|b` → `bootvol=rootfs_a|rootfs_b`；`ubi part nand0`；内核参数为 `ubi.mtd=nand0 root=ubi0:${bootvol} rootfstype=ubifs`，串口统一为 `ttyAMA0`。U-Boot MTD 环境表与 Linux fixed-partitions 都需要同一份区间，组件分别表达不等于可以各拍一张表。

S-10 固定先从 `rootfs_a` 读取 boot.scr，仅对“两卷工厂内容一致且 A 卷可读”的测试前提成立；A 卷损坏时不能据此保证 B 回滚。建议后续选择策略：先确定 bootpart，再从所选卷读取脚本；若仍保留固定 A 路线，须明确不覆盖 A 损坏的场景。完整 OTA 成功标记、原子 env 切换与断电回滚不在本轮实现范围，不能从双卷存在就宣布安全升级已实现。

## 8. watchdog、SPI NOR 与 EEPROM 的后续约束

### 8.1 watchdog

S-15 输入为 MMIO `0x09030000`、32768 Hz、32 位计数器、`arm,sp805`、`/dev/watchdog`；草稿同时规定“load=0 即停用”、使用简化驱动换算，并留下真实 SP805 TRM 待查项。寄存器地址接近 SP805 不足以证明行为兼容。

推荐候选：优先复用锁定 QEMU 中的 SP805 模型，并以同版本 Linux 驱动和手册核对装载、计数、喂狗、锁、使能、IRQ及系统复位语义。S-15 列出的偏移 `0x000/0x004/0x008/0x00c/0xc00` 与解锁值只是待核清单，不作为已核硬件规范。特别核对 timeout 与实际计数周期的换算，是否分多阶段，以及 load=0 的真实行为；不为复现草稿故事而更改正常模型使其无条件“兼容”。

静态算术：`60 × 32768 = 0x001e0000`、`131072 × 32768 = 0x1_0000_0000`，u32 截断确为 0，但这里只证明草稿的简化表达式会溢出，不证明真实驱动使用此表达式。S-15 两个示意读数的差 `0x001df2c0 - 0x001d6e40 = 0x8480 = 33920`，对应 32768 Hz 下 1.03515625 秒，不能当作精确一秒减少 32768 tick 的测量证据。

chapter 15 的教学 bug 若保留，应作为有明确注入点、独立版本和恢复方法的实验修改，区别于正常平台驱动；是否采用及如何保持“真实上游”叙述准确，由用户在后续章节确认。复位测试使用可控制的 QEMU 虚拟时间，覆盖临界点、喂狗、屏蔽/清除、重复使能、warm reset 后状态；不能靠宿主等待 131072 秒判断模型正确。

### 8.2 SPI NOR / I2C EEPROM

S-02 将 M25P80 留为候选芯片，S-04 旧 patch 清单与 S-08 却已有 `m25p80`/at24 叙述。本版仅保留控制器窗口，不确认具体 NOR 容量、chip-select、JEDEC ID 或 EEPROM 型号、容量、I2C slave 地址。先核对上游控制器、芯片模型与 guest binding，再同步 QEMU 装配、DTS 和驱动配置；不能从驱动名称推断板上芯片存在，也不能照抄未核的 Kconfig 符号。它们不阻塞最小 UART 固件路线，但若进入本章 patch，就成为本章必须验证的设备。

## 9. 待确认问题与风险

以下是候选稿问题清单，不是新 confirmed 决定、已安排评审或已启动的新任务。核对责任说明用于 project-manager 后续登记实际安排。

| ID | 待确认内容/推荐方向 | 影响与确认门槛 |
|---|---|---|
| Q-01 | QEMU/Poky/meta-arm、TF-A/U-Boot/Linux 实际选中版本、源码校验/提交、已有 patch；由 Yocto 核对 | chapter 4 gate；不能以 PV 8.2.7/移动分支替代来源锁定 |
| Q-02 | §4 的 CPU、地址窗口、GICv2、INTID/PPI、24 MHz、UART/reset 契约；由 QEMU 核对模型接口 | chapter 4 gate；实际模型范围/接线与 DTS 不符须修订提案 |
| Q-03 | §5 显式 boot 模式、secure EL3 入口、secondary 初态、无镜像错误及 reset | chapter 4 gate；QEMU 核对锁定版本 ARM API；用户确认最终模式边界 |
| Q-04 | runqemu 候选 A 的真实部署输入/二进制/固件传递是否成立；必要时评估 B | chapter 4 gate；Yocto 核对及用户选择；不得用 `-S` 替代运行 |
| Q-05 | BL2/FIP IO、SRAM/linker、缺少 BL33 的真实终态、U-Boot standalone DTS/入口 | chapter 6/7 前 gate；影响 chapter 4 的 ROM及保留内存规划须本章先核兼容性 |
| Q-06 | firmware 与 direct-linux 的 PSCI 提供者/版本/conduit/affinity及多核、复位实现 | chapter 8/10；不得以旧稿的“单核”叙述约束实际证据 |
| Q-07 | NAND 控制器/guest驱动组合、ID/几何/OOB/ECC/坏块及后端格式 | chapter 9/10 前 gate；C-W17/C-W19，失败会重算全部布局/UBI参数 |
| Q-08 | 正常 SP805 模型与教学简化驱动分离；零装载/周期/复位契约 | chapter 15 前 gate；C-W30/V23/V32，不能制造假的上游 bug 结论 |
| Q-09 | 64 MiB 两卷预算、boot.scr 选卷、env 持久化/坏块/冗余、镜像长度防覆盖 | chapter 9/10；仅有静态表，回滚与实测能力待补 |
| Q-10 | SPI/I2C具体设备及网络依赖；本章是否纳入 RTC | 相应章节；A-003 C-W26/C-W29 的网络路径未解决，不能假设 slirp 等于有网卡 |

未取得的原 glossary/blueprint、“D5”来源及 A-003 的旧四仓库口径按项目记录追踪；本文不恢复旧仓库任务、不修订这些成果。平台取舍经专业核对和用户确认后，由实际记录人登记决定/批准依据；本版不新增或代写决定。

## 10. 验证办法与当前覆盖

所有运行步骤都是后续验证计划。每项证据须记录实际执行者、日期、环境/源码/patch/二进制版本、输入、命令、预期、实际、返回码、原始日志及终态；模型测试与系统验证分开。

| ID | 对象/方法 | 判据 | 本轮状态 |
|---|---|---|---|
| V-01 | 本机 checkout/远端地址/branch/HEAD/祖先/工作树只读核对 | 与 WORKSPACE、T-016一致；不访问未授权远程 checkout | 通过；仅 OPC 和书稿本机范围 |
| V-02 | 章节来源/决定/成员 scope 核对 | 每条事实有来源，候选值不冒充已确认，未取得源码明确列出 | 通过；文档核对，不是专业评审 |
| V-03 | 静态地址/分区/容量与 watchdog 算术 | MMIO/RAM 主窗口不重叠；子区边界/容量自洽；明确阶段占用 | 通过；候选数字静态检查，不证明实际模型映射或运行安全 |
| V-04 | 锁定配方与源码核查 | 取得 Q-01 基线、选择链、已有 patch、API和 binding；回填精确证据 | 未执行；本轮未取得工程源码/配方 |
| V-05 | 模型注册/QTest与边界 | machine可枚举；访问宽度、端序、保留位、边界和IRQ assert/deassert符合最终契约；未启用窗口无假设备 | 未执行；QEMU 工程验证计划 |
| V-06 | CPU测试固件执行/中断/reset | CPU0真实执行并输出标记；timer IRQ进入guest handler；warm reset后重新执行，secondary状态符合模式 | 未执行；暂停实例化不能替代 |
| V-07 | 正式集成/清理workspace/干净构建 | 仅Poky+meta-tiger得到同版patch和二进制；runqemu实际命令选中`tiger`且测试输入执行 | 未执行；Yocto系统验证计划，Q-04 gate待解除 |
| V-08 | TF-A/U-Boot/Linux分段与全链 | 每个装载/入口/参数面有证据；PSCI、DTB、console、rootfs分别覆盖 | 未执行；按chapter 6–10阶段开展 |
| V-09 | NAND/UBI/镜像和负向用例 | 页/块/OOB/读写擦除/越界/坏块、两卷内容与attach、超大FIP阻断及恢复 | 未执行；不从对照组推断tiger结果 |
| V-10 | watchdog虚拟时间与系统复位 | 真实模型周期、喂狗/使能/临界/复位后状态和guest驱动一致 | 未执行；只完成简化算术核对 |

V-01/V-03 的实际命令为本机 `git status`、`rev-parse`、`remote -v`、`merge-base --is-ancestor` 与 Python 整数运算；本轮未生成或运行业务代码。基于本机缓存的远端跟踪引用不代表服务器当前头。远程 `meta-tiger`、实际 Poky checkout 及上游组件均无本轮核验覆盖。

## 11. 本次交付与后续衔接

本轮按 T-016 授权仅交付 A-005 v0.1 draft，并登记自身实际进展和成果索引；未修改书稿、工程代码、WORKSPACE 或他人成果。候选稿形成后停止改稿，保留本版本供后续组织核对。

下一位行动人为 project-manager：按此实际版本登记 QEMU 模型/接口核对、Yocto 配方/集成/启动核对及用户意见。当前没有代写任一工程成员的接收或评审结论。意见到齐后 architect 统一修订，必要复核与用户确认形成 chapter 4 实施基线；再由 writer 编制章节蓝图，并依既有串行顺序开展工程。

跨章影响已定位：chapter 2 的四仓库地图/平台能力状态；chapter 4 的源码与真实patch、最小启动输入；chapter 6/7 的固件IO与standalone入口；chapter 8 的DTS/直通/PSCI；chapter 9/10 的NAND后端、分区、镜像、卷与装载生命周期；chapter 15 的正常watchdog模型与教学实验。均留给对应任务和章节阶段处理，不在本轮提前改稿。

关联正式记录：[T-016](../TASKS.md)、[项目决定](../DECISIONS.md)、[工作区声明](../WORKSPACE.yaml)、[A-002计划](A-002-book-polishing-plan.md)、[A-003校验清单](A-003-知识点校验清单.md)、[成果索引](README.md)。
