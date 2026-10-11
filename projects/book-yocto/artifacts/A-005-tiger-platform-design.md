# tiger 平台设计说明

```text
项目：book-yocto
成果 ID：A-005
负责人：architect
关联任务：T-016；跨章影响供 T-017 使用，T-017 尚未启动
版本：0.2
更新日期：2026-10-11
状态：draft
适用范围：chapter 4 最小启动候选设计；chapter 2、6–10、15 的接口与后续门槛
来源及输入版本：T-016 统一修订安排（869929b5d4b5e0ff39f10c2fe1ddf6853f9a0445）；固定 A-005 v0.1（69ce591cd9e4ee310b80cff4663c20c776364cfd）；U-A005-01/02、Y-A005-01～05（dd15379ebea54b36dab7b5cc2638fd345e71553f）、QE-A005-01～06（1c8304446f8bf82fd46a56413a5fa5787e271ae6）及两份报告/选定原始证据；A-002 v0.8、A-003 v0.1、A-004 v0.5；D-001/D-007/D-010/D-011/D-013；书稿 f444790f352b46f699fd3d46198f8b9b9f089a62；当前成员 scope
批准依据：尚无；两项 revise 核对和用户意见已齐备，本版为统一修订稿，待针对性复核与用户确认；实施/运行未执行
版本记录：2026-10-11 v0.1 首次候选稿；同日 v0.2 统一处理 13 项 U/Y/QE 意见，收敛单核启动、固定固件/reset/部署契约、回填来源、调整 SPI NAND/watchdog 门槛。v0.1 由固定提交保留。
```

## 1. 本版结论与使用条件

第 4 章只交付能实际启动的最小 tiger machine：**TCG、AArch64 小端 Cortex-A53 CPU0、1 MiB ROM、1 GiB RAM、轮询 PL011 UART0**。固件从 ROM 地址 0 的 secure EL3h 入口执行，在 RAM 建立栈、初始化 data/BSS，经 UART0 输出可追溯标记；宿主 system_reset 后回到同一入口并再次执行。1 GiB 是与后续布局衔接的选择，不是串口启动的技术最低量。

本章不实例化 SRAM/GIC/RTC/SPI/NAND/watchdog，不提供 timer IRQ、guest 请求 reset、四核唤醒、BL31/PSCI、U-Boot 或 Linux 启动。预留布局服务后续演进，未启用窗口保持未映射，不建立假设备。四核及完整平台仍是全书最终目标，按首次使用章节补齐并实测。

选定 §5 的 raw 固件装载和 ARM loader 纯 firmware reset 注册路线，独立测试资产配方部署固件并生成 runqemu 输入。源码静态依据不等于新 tiger 实现或启动通过。本版仍待复核与用户确认；本章交付真实 patch、正式 meta-tiger 集成、清理临时 workspace 后干净重构建及 runqemu 运行证据，保留 D-001/D-010/D-013 和最终 C-W/V 回填目标。

## 2. 资料与版本证据

### 2.1 直接读取与引用的工程证据

architect 本轮直接完成：刷新协议/身份/角色/四项有效技能、WORKSPACE、T-016 和决定；读取固定 v0.1、全部 U/Y/QE 意见、两份报告和选定原始摘录；核对本机 OPC 在 feature/book-yocto/t-016-chapter4-revision、开始 HEAD `869929b5d4b5e0ff39f10c2fe1ddf6853f9a0445` 且干净、目标分支为祖先，v0.1 blob 为 `d7d474e4de48ad8e05ffd4d9893594214843f9be`；书稿在对应 feature 分支、HEAD `f444790f352b46f699fd3d46198f8b9b9f089a62` 与 WORKSPACE 一致且干净。本轮不 SSH、联网、展开 BitBake、构建或运行。根规则/角色/依赖未变正文经 Git 内容比较复用，有效技能完整重读。

以下远端事实由 **两位工程师于 2026-10-11 只读实查**，architect 读取其保存证据，未重做远端核验。引用版本分别为 Yocto `dd15379ebea54b36dab7b5cc2638fd345e71553f`、QEMU `1c8304446f8bf82fd46a56413a5fa5787e271ae6`。

| 证据 ID | 来源 | 支持范围与限制 |
|---|---|---|
| E-Y | [Yocto 报告](work-records/t016-a005-yocto-review.md)及[原始日志](work-records/t016-a005-yocto-review-raw.log) A/B/C | meta-tiger master@0e3da48b34ef772532535a730a21878cc30ccf67 干净且只有骨架；Poky scarthgap@77d1feb37e280733684ae8a9449fb031d5d7ff40 干净；build 仍为 qemuarm64，未展开 tiger provider/override |
| E-Q | [QEMU 报告](work-records/t016-a005-qemu-review.md)及[原始日志](work-records/t016-a005-qemu-review-raw.log) B/C2/E/F/G | 22 个选定源码/API 文件与锁定包逐字节相同；CPU/loader/reset/PL011/SSI 静态依据；未核全树、未重应用 OE patches、未运行 |
| E-P | E-Y §1/§2、日志 A/C；E-Q §1 | qemu-helper-native_1.0.bb → qemu-system-native_8.2.7.bb → qemu-native.inc → qemu.inc；源码包加 31 项 OE patches |
| E-R | E-Y §3、日志 C；E-Q §4、日志 E | runqemu/qemuboot 的 BIOS/none/native sysroot/nographic/路径规则；不是部署或启动实测 |

Poky 精确来源为 `scarthgap@77d1feb37e280733684ae8a9449fb031d5d7ff40`；辅助源码路径 `tiger:/home/oops/workspace/poky`，不自动更新 WORKSPACE 或授权写入。QEMU 来源为 `https://download.qemu.org/qemu-8.2.7.tar.xz`，实际包 SHA-256 **`1f0604f296ab9acb4854c054764a1ba408643fc299bd54a6500cccfaaca65b55`**，与配方一致。work 源码位于 `tiger:/home/oops/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7`，没有独立 QEMU Git HEAD。

31 项 patch 的完整顺序引用 E-Y 日志 A：前 29 项为公共 SRC_URI，末两项为 class-native 追加；`.pc/applied-patches` SHA-256 为 `3b7c0560451d78d8eae6512a4c21d77129eb3b0e1106512949709e8ef0eb8430`。tiger patch 基于包加这组 OE 补丁，接在已有序列之后；不得覆盖 SRC_URI、漏掉 native 追加或以裸上游包替代基线。正式集成时保存实际展开顺序与 patch 应用结果。

Poky runqemu SHA-256 为 `d644b4763b968414649f948151d51ff84b1663218ffd4abae3b58d19e4ba7861`，qemuboot.bbclass 为 `87d8cc1bcb8db862ae9a80f53c685e61b750867060fb563804a92bf1dedde01f`。旧 log.do_patch/work/helper binary 只证明既有身份，不作为新 tiger 的干净构建或启动证据。

本轮按需直接读取原始日志的配方/patch 顺序、qemuboot 任务/字段、runqemu BIOS/none/native/serial 分支，以及 CPU reset/rvbar、ARM loader 注册、PL011 reset/clk、watchdog 可用性摘录。其余审查结论引用报告，不声称 architect 全文读取 22 份源码或所有日志。

### 2.2 书稿定位与读取范围

下表路径均为 `book-manuscript` 仓库内路径，统一对应提交 `f444790f352b46f699fd3d46198f8b9b9f089a62`。下列区间为 architect 在 v0.1 形成时实际读取，本轮保留定位，未重新全文审校这些章节。

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

| 内容 | 依据 | 本版处理 |
|---|---|---|
| 本章最小启动/按章补功能；NAND 走 SPI | T-016 U-A005-01/02 | 采用范围及接入方向，不等于用户批准全部候选值 |
| 真实代码、全链运行、C-W/V、正式 patch/干净复验 | D-001/D-010/D-013 confirmed | 保留最终目标与本章启动证据 |
| 最终四核 A53/后续设备 | S-02/S-07 | 本章仅 CPU0，首次使用前扩充 |
| Poky/QEMU hash/31 patches | E-P | 已有静态来源；新 tiger 最终 provider/override 尚未展开 |
| 未来 TF-A/meta-arm/U-Boot/Linux | E-Y §1/§5 | 当前未启用 meta-arm/TF-A；可读 U-Boot 2024.01 配方和 qemuarm64 Linux 6.6.151 不是 tiger 已选基线；不采用草稿 TF-A 2.10.4/U-Boot 2024.04 为工程事实 |
| NAND 几何/watchdog 32768 Hz | S-09/S-15、A-003 | 条件性后续输入，型号/手册/模型/实测未确认 |
| 原 glossary/blueprint、“D5”和 fork 示例提交 | 未取得旧资料/草稿叙事 | 不当作项目决定或真实提交，以 DECISIONS 为准 |

## 3. 能力阶段与最小资源

| 阶段 | 交付与首次使用门槛 | 当前状态 |
|---|---|---|
| chapter 4 | CPU0/A53/TCG、ROM/RAM、轮询 PL011；raw 固件冷启动/宿主 reset；正式 patch/fixture/qemuboot/runqemu/干净复验 | 本版设计已收敛，实施/运行未执行，待复核与确认 |
| chapter 6 首次逐级装载前 | BL1/BL2/BL31 来源/linker/IO，按需要加 SRAM；若首次 SPI NAND 读 BL2/FIP，先满足 §7.1 | 后续依赖，不要求 chapter 4 先实现 |
| chapter 7/8 首次 U-Boot/Linux 前 | 入口/EL/DTS/boot 参数，按首次使用加入 GIC/timer IRQ、PSCI/guest reset/多核唤醒 | 尚未选定/实现/实测，各自形成契约 |
| chapter 9/10 | SPI NAND 写擦/持久化、MTD/UBI/UBIFS/镜像/全链/A-B；四核目标在实际需要前形成停车/affinity/唤醒/reset 方案 | 纸面布局仍候选 |
| RTC/SPI NOR/I2C/网络/watchdog 首次使用前 | 模型/驱动/binding/clock/IRQ/reset、正常行为与教学实验 | 后续未实现，依对应章节确认 |

chapter 4 只允许 `-accel tcg -cpu cortex-a53 -smp 1 -m 1024`；machine 默认/最大 CPU 数为 1，RAM 对应 1024 MiB。其他 CPU/核数/RAM/加速器、kernel/initrd/DTB 输入明确报错，不静默接受固定布局之外的值。没有 CPU1–3 对象，不暗示 secondary 停车或 CPU_ON 服务。

A53 自带 EL2/EL3/generic timer 是 E-Q 源码事实，不等于已接 GIC/PPI 或提供 PSCI。本章异常/中断保持禁用，UART IRQ 未接线，仅覆盖轮询字符功能，不需要 DTS，也不把预留设备写成已存在节点。

## 4. 本章实例化与后续预留契约

### 4.1 本章要实现的地址窗口

统一用半开区间。下表是待复核的实施契约，尚不是已实现状态。

| 区域/设备 | base | end（不包含） | 大小/行为 |
|---|---|---|---|
| 启动 ROM | `0x00000000` | `0x00100000` | 1 MiB，可执行只读 MemoryRegion；raw 偏移 0 |
| PL011 UART0 | `0x09000000` | `0x09001000` | 4 KiB；上游模型；32 位对齐访问，轮询，IRQ 未接 |
| DDR RAM | `0x40000000` | `0x80000000` | 1 GiB；栈/data/BSS 可写，不模拟 DDR 训练 |

### 4.1a 后续预留（本章不实例化）

| 区域/设备 | base | end（不包含） | 首次使用前确认 |
|---|---|---|---|
| GIC distributor | `0x08000000` | `0x08010000` | GICv2 候选映射/安全分组/binding |
| GIC CPU interface | `0x08010000` | `0x08020000` | CPU/中断接线 |
| PL031 RTC | `0x09010000` | `0x09011000` | 计时源/IRQ/reset，移出本章 |
| SPI master | `0x09020000` | `0x09021000` | 主控与实际 CS 未选；芯片不另占 NAND MMIO |
| watchdog | `0x09030000` | `0x09031000` | S-15 基址输入，长度候选；模型/手册/驱动 |
| I2C master | `0x09040000` | `0x09041000` | 主控/EEPROM/地址/IRQ |
| 启动 SRAM | `0x0e000000` | `0x0e100000` | TF-A linker/BL2/早期栈 |

删除 v0.1 独立并行 NAND MMIO/IRQ，SPI NAND 走 SPI master 的真实 CS，见 §7.1。若 CS 需新板级 GPIO/寄存器，其地址/驱动/reset 随后续选型登记，不能暗藏成已实现接口。未启用窗口保持未映射。ROM 仅外部装载固件，不证明模拟 SoC 从 SPI NAND 读 BL1 的 Boot ROM 算法。

### 4.2 RAM 使用与镜像边界

本章测试固件运行预算：

| 用途 | 范围 | 约束 |
|---|---|---|
| data/BSS | `[0x40000000, 0x40010000)`，64 KiB | .data VMA 在 RAM、LMA 在 ROM；每次入口复制 data/清 BSS，链接 ASSERT 防溢出 |
| 栈 | `[0x40010000, 0x40020000)`，64 KiB | SP_EL3=`0x40020000`，16 字节对齐、向下增长，与 data/BSS 分开 |

测试固件临时占用不形成未来 Linux 保留区。输出标记后改变测试 data/BSS 哨兵并进入循环，便于 reset 后检验再初始化；不要求 RAM 全清零。下面只保留后续组件的候选预算，本章不加载这些镜像，生命周期不与测试固件同时：

| 用途 | 候选范围 | 契约 |
|---|---|---|
| BL31/安全运行时预留 | `[0x40000000, 0x41000000)`，16 MiB | 后续 TF-A 确认占用；Linux DTS 应表达保留区，且与固件实际布局一致 |
| 脚本暂存 `loadaddr` | `[0x41000000, 0x42000000)`，16 MiB | 起点来自 S-10；检查 boot.scr 实际长度，执行完后生命周期结束 |
| 内核输入 `kernel_addr_r` | `[0x42000000, 0x46000000)`，64 MiB | 起点来自 S-10；镜像、运行时占用及解压/搬运范围须分别核对 |
| DTB `fdt_addr_r` | `[0x46000000, 0x46200000)`，2 MiB | 起点来自 S-10；校验总大小、对齐及后续 fixup 所需余量 |
| BL33/U-Boot 初始装载 | `[0x48000000, 0x4a000000)`，32 MiB | 新候选；link 地址、TF-A handoff 与 standalone 装载使用同一份定义 |

这些是分阶段装载预算，不是 Linux 永久划分的内存分区。U-Boot relocation、栈、malloc、内核输入及最终运行范围须有按阶段的占用图；不能仅靠“几个入口地址不同”证明不会覆盖。BL31 常驻区必须从普通世界可用 RAM 中排除。DTS `reserved-memory` 只表达软件保留，不能单独证明 QEMU 已实现安全世界访问隔离；若后续宣称隔离，须核对 address space 与安全访问控制。


### 4.3 后续中断候选

本章 UART 轮询，不实例化 GIC、不接 UART IRQ/timer PPI。未来候选 INTID：UART0=32、RTC=33、watchdog=34、SPI master=35、I2C master=36；没有独立 NAND IRQ或虚构芯片中断脚。level-high 仅候选，随实际模型确认。

E-Q §6 给出 GICv2 和 timer INTID 26/27/29/30 的源码参考，未完成逐寄存器审查。后续分别记录 INTID、GIC GPIO 下标、DTS SPI/PPI 索引/flags、安全分组与每核接线。UART INTID 32 的 DTS SPI 索引候选为 0，不能再加 32；PPI 与外部 SPI 转换分开核对。实际 handler/清除/撤销/reset 需 guest 测试，QTest 不替代。

### 4.4 时钟与复位职责

PL011 **只有一个模型输入 clk**：固定 24 MHz 源在 realize 前通过 qdev_connect_clock_in 连接，由 machine 持有，生命周期覆盖 reset。后续 DTS uartclk/apb_pclk 名称不等于两个模型输入。

CPU cntfrq 独立设为候选 24 MHz，在 realize 前配置，不能由 UART 时钟推导；本章不使用 timer IRQ。后续固件计时/PPI/handler 另核。watchdog 32768 Hz 仍仅 S-15 输入，RTC/SPI/I2C 时钟随模型确认。

系统 reset 按 §5.2 已选 ARM loader 注册路线：CPU 回 rvbar=0/secure EL3h，PL011 设备回调复位 FIFO/寄存器/屏蔽，ROM 保持，固件重新初始化自身 RAM；RAM 整体不承诺清零。宿主 system_reset 属本章验证，guest 请求 reset 留待真实 PSCI/板级接口/watchdog 具备后另测。后续 SPI NAND 持久数据、CS reset 未选中/芯片状态与 watchdog 复位传播另设门槛；本章没有复位原因寄存器。

## 5. 固件、reset 与 runqemu 实施契约

### 5.1 唯一装载路线与 CPU 入口

本章仅接受 raw 测试固件 BIOS：**MachineState.firmware → BIOS 路径解析 → tiger 实际 ROM MemoryRegion → load_image_mr**。优先传显式部署绝对路径；若支持 BIOS 搜索路径，按锁定 API 解析并记录最终文件。先建立 ROM，再装载到偏移 0，入口为 0；不猜 ELF 入口或根据文件名选择 BL1/U-Boot，不建立三种 boot-mode 属性。kernel/initrd/DTB 输入事先拒绝。

E-Q §3/原始 E 的 loader.c:143–163 表明 loader 可接受空文件，因此 tiger 额外要求 **0 < raw 长度 ≤ 0x100000**。缺少 BIOS、不可读、空文件、超长或实际装载失败均在 CPU 执行前报错。恰好 1 MiB 合法，1 MiB+1 拒绝；长度合法只支持边界判据，不证明内容或启动有效，负向用例在实施时生成并留返回码。

CPU0 保留 has_el2=true/has_el3=true，rvbar=0、start-powered-off=false；realize 前接 memory 与 secure-memory 到同一 system-memory 视图，并配置 CPU/频率属性。reset 初态为 AArch64 小端 secure EL3h/PC=0，依据 E-Q cpu.c:302–313/1572–1589 及 A53 属性，不复制 virt 关闭 EL3 的设置。同一视图不承诺安全隔离；本章没有 BL31，PSCI disabled，不调用 SMC/HVC 作 guest reset 或唤醒。

### 5.2 已选系统 reset 路线与生命周期

**选用锁定 arm_load_kernel 的纯 firmware 路线，不另注册板级 CPU reset handler。** tiger machine state 持有零初始化 arm_boot_info，生命周期覆盖整个 machine，禁止传栈上临时结构。ROM/CPU/UART 建立、BIOS 装载成功后只调用一次：

- firmware_loaded=true；psci_conduit=QEMU_PSCI_CONDUIT_DISABLED、is_linux=false；无 DTB 回调/生成物、无 secondary hook。ram_size/loader_start 记录本版 RAM，入口由 ROM/rvbar 决定。
- MachineState 的 kernel/initrd/DTB 输入已经拒绝。函数会复制这些字段，不能仅清 boot_info 来掩盖命令行输入。
- E-Q 原始 C2/E 的 boot.c:1204–1239 为 CPU 注册 do_cpu_reset，进入 arm_setup_firmware_boot；无 kernel/DTB 时 env->boot_info 保持 NULL，reset 先 cpu_reset，再使用 rvbar，不另改 PC。
- 注册责任归 machine 初始化；冷启动与后续 system_reset 由 QEMU reset 链执行，每次 reset 不重建对象、不重调 arm_load_kernel、不重复注册。machine/boot_info/ROM/clock 全程有效；本章不支持 CPU 热插拔、运行中销毁/重建 machine，未来扩充该生命周期时再审取消注册/释放。
- PL011 用设备 reset 回调，不通过 CPU handler 代清 UART。guest 自己“再打印一次”不能替代模型系统 reset 验证。

实施时暂停 CPU，再发宿主 QMP/HMP system_reset，在固件恢复前读取 PC/EL、UART FIFO/屏蔽/寄存器 reset 值并对照锁定源码，再 cont 取得第二标记和 data/BSS 初始化成功。保存完整控制顺序、状态和日志。guest 请求 reset、四核停车/CPU_ON、timer IRQ 以后分别给出真实触发接口再测。

### 5.3 raw 测试固件资产

计划资产 tiger-minboot，首版固件候选 0.1，独立于设计 v0.2。QEMU 工程师主责启动汇编/最小轮询代码/链接脚本及模型交接、guest 证据；Yocto 工程师主责编译/部署配方/qemuboot/正式集成验证。以下文件是实施方案，**目前未创建**：

- meta-tiger/recipes-test/tiger-minboot/files/ 下的 start.S、main.c、tiger-minboot.ld 和许可；上层配方 tiger-minboot_0.1.bb。AArch64 freestanding，无 libc/动态链接/Linux ABI 依赖；工具链 flags 核对禁用 builtin、栈保护、PIE 等隐式要求。
- ELF _start/入口与 ROM VMA=0，text/rodata 在 ROM；data 初值 LMA 紧随 ROM 内容，VMA/BSS 在 §4.2 RAM。链接 ASSERT 防 ROM/data-BSS 溢出；objcopy 按装载布局生成 raw，避免 RAM VMA 造成巨大空洞；保留 ELF/map/脚本/raw 对账。
- 入口屏蔽异常/中断，建立 16 字节对齐 SP_EL3；每次复制 data、清 BSS 并验证初值；保持 MMU/cache 未开启，不依赖未定义通用寄存器、DTS、PSCI 或宿主预填 RAM。
- UART 使用 32 位对齐 MMIO：关闭 UART、关闭中断屏蔽并清待决，配置 8N1/轮询发送分频及控制位，再使能 UART/TX，检查 UARTFR.TXFF 后写 UARTDR。24 MHz/115200 的候选整数/小数分频为 13/1，寄存器位和顺序在实现时对照锁定 PL011 并读回；不据字符输出宣布波特率时序或控制位已经验证。
- guest 输出确定标记，预期格式示例（非实测日志）：`TIGER-CH4 FW=<source-build-id> CPU0 EL3 INIT=OK UART0`。build-id 来源于版本化输入，不编造提交、不递归嵌入 raw 自身 hash；部署 manifest 另记源/配方提交及 ELF/map/raw SHA-256。成功后改变测试 data/BSS 哨兵并循环，使 warm reset 的再初始化可观察。

E-Q 表明 PL011 DR 输出未强制检查 TX enable，因此字符输出不能单独证明正确 UART 配置、IRQ 或线速。结合入口/CurrentEL、链接关系和真实 guest MMIO trace/调试证据判定执行，本章没有中断或性能验收。

### 5.4 独立部署配方、任务图与字段

独立 target fixture 配方 **inherit deploy，自行实现版本化配置生成任务**；不继承需要 image/rootfs 上下文的 qemuboot 类。E-Y 原始 C 的类任务位于 do_rootfs/do_image 之间，使用 IMAGE_NAME/IMGDEPLOYDIR，本章不为它创建 Linux image。

计划任务图，未解析/运行：

```text
版本化源/许可 → do_fetch/unpack → do_compile（ELF/map/raw、链接/长度检查）
                                      ↓
qemu-helper-native:do_addto_recipe_sysroot → do_generate_qemuboot
   ↑ DEPENDS 含 qemu-system-native 等          ↓
                                  do_deploy（整套资产） → do_build
```

do_generate_qemuboot 在 do_compile 后、do_deploy 前，显式依赖 qemu-helper-native:do_addto_recipe_sysroot；helper 的 DEPENDS 获取 qemu-system-native 等工具并形成持久 sysroot。do_deploy 在 do_build 前发布 raw、ELF/map、manifest、conf。Yocto 工程师用 bitbake -e/-g 及任务日志核对实际依赖图，不能把 RDEPENDS 当成全部构建依赖或保留旧 native QEMU。

生成任务用 ConfigParser 写 [config_bsp]，按已展开变量生成下表字段，字段与生成器输入纳入 vardeps，源/配方纳入签名。先写 DEPLOYDIR，由 deploy class/sstate 发布到 DEPLOY_DIR_IMAGE；不临时手写公共 deploy INI，不依赖工作目录残留。raw/conf 共用版本名 tiger-minboot-0.1-tiger，分别后缀 .bin/.qemuboot.conf，版本调整同步 manifest。

路径策略：发布后 conf 与 BIOS 同目录，DEPLOY_DIR_IMAGE 为 "."，BIOS 仅填 basename。从**最终 deploy 位置**计算 helper sysroot 的相对路径，使以 "../" 开头并匹配 runqemu 展开规则；不能从临时 DEPLOYDIR 计算错层级。若采用绝对路径，明确绑定该构建树、检查存在，不宣称跨机器可搬移。实施前复核 validate_paths 所需全部字段，配置/路径缺失不通过，不用隐式 bitbake 回退掩盖部署错误。

| 字段 | 计划值/生成要求 |
|---|---|
| MACHINE / TUNE_ARCH | tiger / aarch64；machine 独立 tuning，不整份继承 qemuarm64/virt |
| QB_SYSTEM_NAME | qemu-system-aarch64 |
| QB_MACHINE / QB_CPU / QB_SMP / QB_MEM | -machine tiger -accel tcg / -cpu cortex-a53 / -smp 1 / -m 1024 |
| QB_DEFAULT_KERNEL / QB_DEFAULT_FSTYPE / QB_ROOTFS | 三者均字面 none，不由 image/kernel 变量替代 |
| QB_DEFAULT_BIOS | tiger-minboot-0.1-tiger.bin，实际同目录发布文件 |
| DEPLOY_DIR_IMAGE | "."，按 conf 位置展开，部署后检查 |
| STAGING_BINDIR_NATIVE | 按 BASE_WORKDIR/BUILD_SYS/qemu-helper-native/1.0/recipe-sysroot-native/usr/bin 计算，helper 任务已完成 |
| STAGING_DIR_NATIVE / STAGING_DIR_HOST | native 为 helper sysroot 根；host 为实际 target recipe sysroot；按最终 conf 位置编码并检查存在/validate_paths，不虚构目录 |
| QB_NET | none，运行参数同时 nonetwork |
| QB_RNG / QB_GRAPHICS / QB_OPT_APPEND | 空，不注入 virtio RNG/GPU、USB、PCI 或额外 device |
| QB_ROOTFS_OPT / QB_NETWORK_DEVICE / QB_TAP_OPT / QB_TCPSERIAL_OPT | 空，不注入磁盘/网卡/virtio console |
| QB_SERIAL_OPT / SERIAL_CONSOLES | 空，由 nographic 提供唯一第一 serial backend，无额外 stdio |
| QB_CPU_KVM / QB_KERNEL_CMDLINE_APPEND | 空，不选择 KVM 或额外 kernel 参数 |
| QB_KERNEL_CMDLINE | none，无 kernel 时不生成 -append，仍核对最终参数 |

E-R 分支：runqemu:757 跳过 kernel，679–683/1258 跳过 rootfs；800–815 将 BIOS basename 拼到 deploy 目录并检查存在，1585–1586 加 -bios；934–942 展开 "../"/"."；nographic 的 setup_serial 添加 -serial mon:stdio。tiger UART0 接 serial_hd(0)，此前没有其他 -serial/chardev stdio；不再追加 QEMU -nographic 导致重复 stdio。显示/monitor 参数以真实展开记录为准。

计划命令（授权/checkout 重核后才执行，本轮未执行）：

```sh
bitbake -e qemu-system-native
bitbake -e tiger-minboot
bitbake -g tiger-minboot
bitbake tiger-minboot
runqemu /home/oops/workspace/build/tmp/deploy/images/tiger/tiger-minboot-0.1-tiger.qemuboot.conf nographic nonetwork
```

deploy 路径按既有 build 默认 TMPDIR 给出计划值，实施保存实际展开值，不因本文出现就认为文件存在。runqemu 位置参数 .bin 会当 kernel，故只传生成 .qemuboot.conf。

启动前检查 STAGING_BINDIR_NATIVE/qemu-system-aarch64 存在且可执行，记录绝对路径/hash和最终进程参数；若退到宿主 PATH QEMU，集成失败。旧 helper 路径/hash见 E-Y §3，不作新 tiger 证据。最终参数须包含本版 CPU/TCG/内存/BIOS/serial、-nic none，不含 kernel/rootfs/virt/virtio/PCI/USB/网络设备或重复串口。

锁定 runqemu 在 nonetwork 处理前仍查 /dev/net/tun；E-Y 记录当时存在且可写，本人未重查。实施前重核，失败按宿主条件记录，不靠 slirp 伪称板上有网卡。

清理/reset 临时 devtool workspace 后，仅用锁定 Poky + 正式 meta-tiger patch/metadata 重构建、重生成资产/helper sysroot并重复 runqemu/模型证据。保存 provider/override、已有/新增 patch 顺序、任务图、命令/返回码、各 hash、完整日志/退出终态；缓存、旧 work 或手工启动不代替干净复验。

## 6. 跨组件契约与后续启动阶段

| 交接面 | 契约/首次使用门槛 | 阶段与依据 |
|---|---|---|
| QEMU → 本章固件 | CPU0/ROM/EL3/rvbar、RAM 运行区、PL011/clk、ARM loader reset；source/ELF/map/raw 对账 | chapter 4，§4/§5、E-Q；无 TF-A/BL31/PSCI |
| 工程 → Yocto | 锁定包+31 OE patches+tiger patches，真实 bbappend/SRC_URI/fixture 配方、任务图、干净重验 | chapter 4，E-P/E-R/D-013；实施展开 provider/override |
| QEMU → BL1 → BL2 | BL1 入口/secure memory、SRAM linker、介质/offset/load/entry/长度及失败终态 | chapter 6 前；-bios BL1 只供 BL1，不能自动得到 BL2 |
| SPI master/CS/芯片 → TF-A IO | §7.1 主控/片选/读取字节/镜像/ECC/坏块、初始化/IO | 首次 SPI NAND 读 BL2/FIP 前，可能 chapter 6 |
| BL2 → BL31/BL33 | FIP 内容/link/load/entry/参数/EL，无 BL33 的真实终态；不重复装已有 FIP 的 U-Boot | chapter 6/7，TF-A/meta-arm/NEED_BL33 未锁定 |
| BL31/loader → 多核/guest reset | provider/版本/conduit/affinity、停车/reset/唤醒，BL31 与 direct-linux QEMU 服务分开 | 首次使用前，不能假定 EL3 firmware 自动停车或有 CPU_ON |
| U-Boot → Linux | Image/DTB/路径/booti、阶段内存占用、console=ttyAMA0,115200、root 卷 | chapter 7–10，linker/relocation 待核 |
| QEMU ↔ 两份 DTS | CPU/内存/GIC/PPI/SPI/CS/clock/保留区来自最终表，各自校验 binding | chapter 7/8，候选 vendor/compatible 不证明已有驱动 |
| Linux → SPI NAND/UBI | SPI master/CS、SPI_MEM/MTD_SPI_NAND/芯片与内建驱动、三段 MTD/三卷/参数 | 首次使用前，qemuarm64 Linux 只是接口依据 |

后续可扩充 BL1、standalone U-Boot 或 direct-linux，本章不实现三种模式。每次先锁来源、模式/EL/入口/装载/测试边界，再改模型/组件/DTS；不根据同一个 -bios 文件名猜入口，不把 QEMU 预塞 BL2/FIP 到 RAM 当 TF-A IO 成功。

chapter 6 若先用其他明确测试介质，记录仅覆盖该介质，全书 SPI NAND 链仍补齐 §7.1。U-Boot OF_EMBED/OF_SEPARATE/控制 DTS/binary，TF-A linker/BL31 常驻区，以及 direct-linux 参数/PSCI 在首次使用前核对。缺少 BL31 不必然等于实际 QEMU direct-linux 单核，结论取决于真实模式和服务。

## 7. SPI NAND、分区与镜像约束

### 7.1 接入、选型责任及首次读取门槛

接入方向为 **SPI master → 实际 CS 电平 → SPI NAND**，不设独立并行 NAND MMIO/IRQ。master/片选来源/芯片仍为后续候选，用户无需在本章批准未知型号。qemu-engineer 主责主控复用/CS/芯片实现或外部实现审查；yocto-engineer 主责 guest driver/binding、TF-A/U-Boot/Linux IO 与镜像/构建集成；architect 汇总接口契约，project-manager 按首次使用阶段安排。本轮不启动这些实现。

| 锁定源码对象 | 已知可复用范围 | 尚缺内容 |
|---|---|---|
| PL022 | SSI bus/MMIO/IRQ/FIFO/ssi_transfer | 没有实际 CS 输出；DMA/单双四线/线速能力须分别核对 |
| SSI API | cs 为 bus 地址；GPIO 电平/极性/set_cs 表达事务边界 | cs=0 不等于选中电平或释放事务 |
| m25p80 | SPI NOR，可参考 SSI/后端结构 | 改名不能得到 SPI NAND 页缓存/OOB/ECC 协议 |
| nand.c | 并行 setpins/setio/getio，可评估后端复用 | 不能直接挂 SSI 或声称已有 SPI NAND |
| SPI NAND 模型 | E-Q/E-Y 在选定锁定树检索未找到可直接用模型 | 需补充实现或取得外部实现并审查；不宣称世界上没有模型 |

若选 PL022，同时选 guest 可控制 GPIO/片选来源，明确 CS index、active-low、reset 初态未选中及 assert/transfer/deassert 的驱动路径；否则另选有真实 CS 的 master 并核对。CS 提供者每次 reset 都要恢复未选中，不能只在进程启动时设置一次。新增 GPIO/MMIO/IRQ/clock/binding 随后续选型版本登记，必要时调整 §4 SPI 窗口，不作为本章隐藏实现。

芯片需审查许可/版本/正式 patch，按最终手册锁定 ID/容量、opcode/地址/dummy/单双四线、reset/get-set-feature/status、page-read/cache-read、write-enable/program-load-execute/erase、busy/error、CS 边界、ECC/OOB/坏块/后端持久化；不以固定返回成功的桩通过。现有 Linux 接口依赖 SPI_MASTER、SPI_MEM、MTD_SPI_NAND；PL022 driver 的 AMBA/DT/clock/CS 和 spi-mem 组合需实查，驱动文件存在不等于 tiger 已配置。

**首次从 SPI NAND 读 BL2/FIP 前**，至少具备 master/真实 CS、芯片 reset/status/最小读取模型、后端数据/镜像布局与所需 ECC/坏块处理、TF-A SPI 初始化/IO，并实测读出字节等于输入镜像。此门槛可能早于 chapter 9。完整写擦/UBI/A-B/持久化继续分阶段，最小只读成功不代表完整芯片完成。若先用其他测试介质，记录有限覆盖。

候选几何仍为页数据 2048 B、64 页/块、擦除块 131072 B、数据容量 536870912 B（512 MiB/4096 块）；OOB/ECC/ID/坏块及后端格式未知。下述布局只在这些几何/工具条件成立时保留，选型变化重算。逻辑容量不等于后端长度，512 MiB 纯数据 dd 仅在后端确实接受此线性格式时采用，否则定义 OOB/ECC 转换；擦除态/错误/持久化实测。

### 7.2 候选逻辑数据偏移表

下表继承 S-09/S-10 的纸面布局，偏移位于 NAND **逻辑数据区**，不是 CPU MMIO 地址，也不一定等于含 OOB 后端文件偏移。

| 区域 | `[offset, end)` | 容量 | 接口要求 |
|---|---|---|---|
| bootloader 总区 | `[0x00000000, 0x00400000)` | 4 MiB / 32 块 | 固件布局与下面子区一致 |
| BL1 子区 | `[0x00000000, 0x00040000)` | 256 KiB / 2 块 | QEMU 外部装入的 BL1 与镜像内 BL1 须为同一版本；当前不能据此声称模拟了 Boot ROM 从 NAND 读取 BL1 |
| BL2 子区 | `[0x00040000, 0x00080000)` | 256 KiB / 2 块 | BL1 实际读取偏移及长度检查 |
| 保留子区 | `[0x00080000, 0x00100000)` | 512 KiB / 4 块 | 不作为未知镜像的隐式落点 |
| FIP 子区 | `[0x00100000, 0x00400000)` | 3 MiB / 24 块 | 检查 FIP 长度，明确包含 BL31/BL33 的阶段版本 |
| env | `[0x00400000, 0x00500000)` | 1 MiB / 8 块 | U-Boot 环境保存、CRC/默认值/坏块/冗余策略待设计验证 |
| nand0 | `[0x00500000, 0x20000000)` | 507 MiB / 4056 块 | label 为 `nand0`，U-Boot、Linux、root 参数一致 |

静态算术确认：全部边界按 `0x20000` 对齐，bootloader 子区合计 32 块，三大区合计 4096 块。总装还须按每个子区检查输入长度；仅检查最终文件为 512 MiB 会漏掉超大 FIP 覆盖 env 的错误。

### 7.3 UBI/UBIFS 与 A/B

在“页 2048、VID offset 2048、数据 offset 4096”的条件下，候选 LEB 为 `131072 - 4096 = 126976 B`。S-09 的 `-m 2048 -p 131072 -s 2048 -O 2048` 与 `mkfs.ubifs -e 126976` 保持为待模型和工具验证的组合。可用块数不能直接把 S-09 的 `4056 - 2 - 80 = 3974` 当成实际终态：坏块、UBI 内部预留和配置会改变结果，须使用真实 attach 日志与工具结果对账；`-c 3968` 同样待核。

S-10 候选 `rootfs_a`/`rootfs_b` 各 64 MiB，按 126976 B/LEB 向上取整各为 529 LEB；`data` 使用剩余空间，只有它拟带 autoresize。镜像需实际包含 `/boot/Image`、`/boot/tiger.dtb`、`/boot/boot.scr`，并核对文件系统余量及镜像最小需求；529 LEB 的算术成立不证明文件能装下。

启动接口候选：`bootpart=a|b` → `bootvol=rootfs_a|rootfs_b`；`ubi part nand0`；内核参数为 `ubi.mtd=nand0 root=ubi0:${bootvol} rootfstype=ubifs`，串口统一为 `ttyAMA0`。U-Boot MTD 环境表与 Linux fixed-partitions 都需要同一份区间，组件分别表达不等于可以各拍一张表。

S-10 固定先从 `rootfs_a` 读取 boot.scr，仅对“两卷工厂内容一致且 A 卷可读”的测试前提成立；A 卷损坏时不能据此保证 B 回滚。建议后续选择策略：先确定 bootpart，再从所选卷读取脚本；若仍保留固定 A 路线，须明确不覆盖 A 损坏的场景。完整 OTA 成功标记、原子 env 切换与断电回滚不在本轮实现范围，不能从双卷存在就宣布安全升级已实现。

## 8. watchdog、SPI NOR 与 EEPROM 的后续约束

### 8.1 watchdog

S-15 输入为 MMIO `0x09030000`、32768 Hz、32 位计数器、`arm,sp805`、`/dev/watchdog`；草稿同时规定“load=0 即停用”、使用简化驱动换算，并留下真实 SP805 TRM 待查项。寄存器地址接近 SP805 不足以证明行为兼容。

E-Q §6/F/G 的锁定树检索只找到 vexpress 注释（含 not modelled），未找到可直接实例化的 SP805。v0.1 的“优先复用 SP805 模型”缺少依据，本版撤回。已有 CMSDK APB/SBSA generic watchdog 仅是后续审查候选，由 qemu-engineer 核对兼容/适配或新建模型、yocto-engineer 核对驱动/配置、architect 汇总后在首次使用前确认。型号、寄存器、时钟、LOAD=0、两级超时/IRQ/reset、喂狗/锁/使能与系统复位传播均按选定手册/模型/驱动对账。S-15 列出的偏移 `0x000/0x004/0x008/0x00c/0xc00` 与解锁值只是待核清单，不作为已核硬件规范。特别核对 timeout 与实际计数周期的换算，是否分多阶段，以及 load=0 的真实行为；不为复现草稿故事而更改正常模型使其无条件“兼容”。

静态算术：`60 × 32768 = 0x001e0000`、`131072 × 32768 = 0x1_0000_0000`，u32 截断确为 0，但这里只证明草稿的简化表达式会溢出，不证明真实驱动使用此表达式。S-15 两个示意读数的差 `0x001df2c0 - 0x001d6e40 = 0x8480 = 33920`，对应 32768 Hz 下 1.03515625 秒，不能当作精确一秒减少 32768 tick 的测量证据。

chapter 15 的教学 bug 若保留，应作为有明确注入点、独立版本和恢复方法的实验修改，区别于正常平台驱动；是否采用及如何保持“真实上游”叙述准确，由用户在后续章节确认。复位测试使用可控制的 QEMU 虚拟时间，覆盖临界点、喂狗、屏蔽/清除、重复使能、warm reset 后状态；不能靠宿主等待 131072 秒判断模型正确。

### 8.2 SPI NOR / I2C EEPROM

S-02 将 M25P80 留为候选芯片，S-04 旧 patch 清单与 S-08 却已有 `m25p80`/at24 叙述。本版仅保留控制器窗口，不确认具体 NOR 容量、chip-select、JEDEC ID 或 EEPROM 型号、容量、I2C slave 地址。先核对上游控制器、芯片模型与 guest binding，再同步 QEMU 装配、DTS 和驱动配置；不能从驱动名称推断板上芯片存在，也不能照抄未核的 Kconfig 符号。它们在首次使用前形成设备/驱动/接线契约和实测证据，本章不实例化，不阻塞最小 UART 固件路线。


## 9. 问题状态与后续门槛

本表是候选设计的确认入口，不代写专业复核或 confirmed 决定；“已修订”只表示作者处理，运行仍未执行。

| ID | 本版处理/下一步 | 阶段与责任 |
|---|---|---|
| Q-01 | 已回填 Poky/package hash/31 patches，新 tiger provider/override/正式 patch 顺序待展开 | 本章实施 Yocto；未来 TF-A/meta-arm/U-Boot/Linux 首次使用前另锁 |
| Q-02 | CPU0/A53/TCG、ROM/RAM/PL011/单 clk；GIC/PPI/四核移出 | 本章 QEMU 针对性复核/用户确认；后续中断另核 |
| Q-03 | 唯一 raw/rvbar/EL3/ARM loader reset 注册及生命周期 | 本章 QEMU 复核、冷启动/边界/宿主 reset 实测；guest reset 后续 |
| Q-04 | 独立 fixture deploy/生成任务、none/helper sysroot/serial/路径 | 本章 Yocto 复核；实施解析任务图、干净 runqemu |
| Q-05 | BL1/BL2/FIP IO/linker、缺 BL33 终态/U-Boot 入口 | chapter 6/7 前工程锁定/IO，architect 汇总 |
| Q-06 | BL31 与 direct-linux PSCI 分开，停车/affinity/PPI/guest reset 各有前提 | 首次四核/中断/guest reset 前，各成员按 scope |
| Q-07 | 移除并行接口；SPI master/真实 CS/芯片选型或实现审查 | 首次 SPI NAND 读 BL2/FIP 前，§7.1，不阻塞本章 |
| Q-08 | 撤回已有 SP805 假设，正常模型/手册/驱动与教学实验另选 | watchdog 首次使用前，C-W30/V23/V32，不制造上游 bug |
| Q-09 | 纸面布局/卷/env/回滚仍候选，型号/OOB 变化重算 | chapter 9/10，读取所需镜像边界提前满足 |
| Q-10 | RTC/SPI NOR/I2C/网络在首次使用前；slirp 不提供网卡 | 对应章节，C-W26/C-W29 保留 |

本章确认仅覆盖最小启动，不要求先批准未知 SPI NAND 型号或完成后续平台；最终真实工程/完整实测没有取消。原 glossary/blueprint、“D5”、A-003 旧四仓库口径保持追踪，本轮不代改；批准由实际记录人注明版本/依据，不自标 approved。

## 10. 设计覆盖与实施验收计划

分开记录 architect 文档/静态检查、工程师锁定源码只读核对、后续真实构建/运行。证据须有执行者/日期/环境、源/patch/metadata/二进制版本、输入/命令/预期/实际/返回码/日志/终态，不以 qemuarm64 旧产物替代 tiger。

| ID | 判据 | 当前覆盖 |
|---|---|---|
| V-01 | 本机版本/分支/祖先/干净与固定 v0.1 blob | architect 本轮直接核对通过；远端引用 E-Y/E-Q，不是本人 SSH |
| V-02 | 13 项 U/Y/QE 回复及来源/阶段边界 | 作者文档检查，待专业复核和用户确认，不等于评审通过 |
| V-03 | 主/预留窗口、测试 RAM/后续预算分别不重叠；条件性分区/UBI/watchdog/分频算术 | 静态核对，不证明映射、运行安全、波特率或真实几何 |
| V-04 | 锁定来源/31 patches/API、新 tiger 展开 | E-Y/E-Q 覆盖现有文件及局部接口；新 provider/override/重应用/构建未执行 |
| V-05 | 注册/ROM 只读/参数拒绝，缺失/不可读/空/1MiB/1MiB+1 BIOS 边界 | 本章 QEMU 验收计划，未执行；合法长度与有效启动分开 |
| V-06 | source/link/ELF/map/raw、CPU0 PC=0/secure EL3h、RAM 初始化/栈、guest MMIO 确定标记 | 本章执行验收，未执行；枚举/-S/对象/宿主打印不能通过 |
| V-06R | 同进程暂停→宿主 system_reset→入口/EL/UART reset→cont→第二标记/data-BSS 再初始化 | 本章模型 reset，未执行；不要求 RAM 全零，不代表 guest reset |
| V-07 | fixture 任务图/生成部署/helper路径-hash/参数；正式 patch 清理 workspace 干净构建和 runqemu | 本章 Yocto 系统验收，未执行；宿主 PATH 回退不能通过 |
| V-08 | SRAM/GIC/timer IRQ/四核/PSCI、U-Boot/Linux/DTB/rootfs 分段/全链 | 首次使用阶段分别实施，本章未做 |
| V-09R | SPI master/CS reset/芯片读取/TF-A IO、BL2/FIP 字节相等/ECC/坏块/镜像边界 | 首次 SPI NAND 读取前，未执行 |
| V-09 | program/erase/持久化/OOB/ECC/UBI/两卷/env、超大 FIP 阻断/回滚/恢复 | chapter 9/10 完整验收，未执行 |
| V-10 | watchdog 模型/真实周期/喂狗/IRQ/reset/driver 与实验版本分开 | 首次使用前，未执行，只保留简化算术 |

UART 配置/读回与字符输出分别验收，本章不验证真实线速/IRQ/性能。reset 值在 CPU 恢复前读，避免固件重写掩盖设备 reset 错误。QEMU 主责模型/负向/guest 执行，Yocto 主责正式集成/任务图/干净 runqemu，证据关联同版资产。

## 11. 本次交付与后续衔接

本轮交付 A-005 **v0.2 draft**，在 T-016 对 U-A005-01/02、Y-A005-01～05、QE-A005-01～06 共 13 项逐项填写作者回复并更新自身进展/索引。原意见/revise 结论/报告/日志保留，v0.1 在固定提交留存。本版不是工程复核结论或获批基线；未实现固件/模型/metadata、未构建/运行、未 SSH、未改书稿/工作区配置。

稳定提交后停止改稿。下一位 project-manager 按新版本串行安排必要针对性复核，再交用户确认本章实施基线；未代写通过/接收/批准，不自行发跨会话消息。确认后 writer 编制蓝图，工程按既有次序实施/实测。

跨章影响：chapter 2 资产地图/能力状态；chapter 4 真实 patch/测试固件/启动；chapter 6 SPI NAND 首次读取/TF-A IO；chapter 7/8 入口/DTS/PSCI/阶段设备；chapter 9/10 芯片/后端/分区/卷/镜像/全链；chapter 15 watchdog 可用性/正常行为/教学实验。留对应章节及 T-017，本轮不提前改稿。

关联正式记录：[T-016](../TASKS.md)、[项目决定](../DECISIONS.md)、[工作区声明](../WORKSPACE.yaml)、[A-002计划](A-002-book-polishing-plan.md)、[A-003校验清单](A-003-知识点校验清单.md)、[成果索引](README.md)。
