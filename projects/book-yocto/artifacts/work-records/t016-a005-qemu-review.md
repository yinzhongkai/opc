# T-016：A-005 v0.1 第 1 轮 QEMU 设计核对证据

- 执行人：`book-yocto / qemu-engineer`；日期：2026-10-11（Asia/Shanghai）。
- 结论：**revise**。单核 A53、ROM/RAM、轮询 PL011 的最小固件路线有锁定源码依据；A-005 尚须明确范围、加载与复位契约、SPI NAND 阶段依赖。本轮没有实现或运行模型，不表示设计获批。
- 固定受评稿：[A-005 v0.1](../A-005-tiger-platform-design.md)，OPC `69ce591cd9e4ee310b80cff4663c20c776364cfd`，Git blob `d7d474e4de48ad8e05ffd4d9893594214843f9be`。意见与后续作者回复入口：[T-016 本人意见](../../TASKS.md#qemu-engineer-的意见)。
- 输入：U-A005-01/U-A005-02、Y-A005-01～05、[Yocto 报告](t016-a005-yocto-review.md)及其原始证据、D-010/D-013、当前身份/角色/有效技能与 WORKSPACE。依 project-manager 的既有串行安排执行。
- 边界：已授权 `ssh tiger` 的只读源码核对；写入仅本人意见及治理证据。没有远端写入、克隆、checkout、构建、QEMU 运行、工程分支、模型/配方实现或书稿修改。
- 原始证据：[命令及完整输出](t016-a005-qemu-review-raw.log)，A/B/C2/D1/D2/E/F/G 均 exit 0。初次默认沙箱 SSH 未取得配置，随后使用获准的只读访问。初次过长 C 摘录被工具截断，未纳入证据；关键正文已由 C2/E/F 完整补读。源文件摘录含原始行号，不把静态源码或既有任务记录当作运行结果。

## 1. 工作区与精确来源

| 对象 | 实际基线 | 状态及效力 |
|---|---|---|
| OPC | 本机 `D:/opc-book-yocto`，`feature/book-yocto/t-016-chapter4-revision`，评审开始 `e92399c65f93900ca801c459d297c713ac9a980d` | 开始干净；只修改本人评审 |
| 书稿 | 本机 `D:/book-yocto`，`feature/t-016-chapter4-revision`，`f444790f352b46f699fd3d46198f8b9b9f089a62` | 与 WORKSPACE 一致，干净；未写入 |
| meta-tiger | `tiger` / `oops`，`/home/oops/workspace/meta-tiger`，`master@0e3da48b34ef772532535a730a21878cc30ccf67` | 与 WORKSPACE 一致，开始及末次核对均干净 |
| Poky | 同主机 `/home/oops/workspace/poky`，`scarthgap@77d1feb37e280733684ae8a9449fb031d5d7ff40` | 开始及末次核对均干净；只读辅助源码，不更新 WORKSPACE |
| QEMU | `/home/oops/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7` | 既有 work 源码；没有独立 Git HEAD，来源是源码包及 OE patches |

实际配方选择链及展开限制沿用 Y-A005-01 的证据：`qemu-system-native_8.2.7.bb → qemu-native.inc → qemu.inc`。本轮复算 `downloads/qemu-8.2.7.tar.xz` SHA-256 为 `1f0604f296ab9acb4854c054764a1ba408643fc299bd54a6500cccfaaca65b55`；`.pc/applied-patches` 为 `3b7c0560451d78d8eae6512a4c21d77129eb3b0e1106512949709e8ef0eb8430`，31 项顺序与 Yocto 记录一致。当前 build 仍为 qemuarm64，meta-tiger 仅骨架，不能据此宣布 tiger 已选中或新 patch 已集成。

对本轮读取的 **22 个选定源码/API 文件**，将 work 文件逐字节与该源码包内成员比较，全部相同；文件 SHA-256 见原始日志 B/E/G。覆盖 ARM CPU/boot、virt/sbsa/realview/stellaris、PL011、PL022/SSI、NOR/并行 NAND、loader/IRQ/clock API、看门狗候选及 timer INTID 定义。Poky QEMU patch 头检索未发现触及 B 组 14 个文件的条目。此局部核对排除了这些文件的额外内容差异，**未验证全树或重新应用全部补丁**。未来 tiger patch 仍应基于锁定包加 31 项 OE patches，按 D-013 正式集成并在清理开发 workspace 后重构建。

## 2. 第 4 章最小模型建议（QE-A005-01）

以下是交由 architect 统一修订的候选契约，不是本轮批准或实现。

| 资源 | 最小建议 | 边界 |
|---|---|---|
| CPU | TCG，AArch64 小端，`cortex-a53`，仅 CPU0，`-smp 1` | machine 的默认/允许 CPU 数与输入一致；四核演进另设阶段 |
| ROM | 保留 `[0x00000000,0x00100000)`，1 MiB，可执行只读区 | raw 固件偏移 0 为入口指令；不是 SPI NAND/真实 BootROM 读取协议 |
| RAM | 可保留 DDR `[0x40000000,0x80000000)`，1 GiB | 1 GiB 是布局兼容选择，不是串口启动的技术最低量；`-m` 与实际映射须一致 |
| UART0 | PL011，`[0x09000000,0x09001000)`，轮询，`serial_hd(0)` | 暂不连 GIC；中断未覆盖须明示 |
| 固件可写区 | 在上述 RAM 内明确栈、data/BSS 和边界 | 栈 16 字节对齐；由固件初始化，不能写 ROM |

`target/arm/cpu64.c:655–710/750` 注册 A53 并包含 AArch64、EL2、EL3、generic timer。这些 CPU 自带能力不等于板级 GIC、timer IRQ 或 PSCI 服务已经提供。PL011 的未接 IRQ 可留空，`hw/core/irq.c:39–44` 对空 IRQ 安全返回；测试固件关闭/不使用中断、仅轮询 UART。因此 SRAM、GIC、RTC、SPI/NAND、watchdog 和四核上线均不必作为本章启动门槛。未来地址可以预留，但不要把预留区实例化为假设备，也不要把未实现的节点加入本章 guest 描述。

本章保留 ROM/UART/DDR 基址有利于后续演进；TF-A SRAM 使用、BL2/FIP、U-Boot standalone 和 direct-linux 的入口/DTS/寄存器传参仍为后续契约，不需要本章先实现三种 boot 模式。

## 3. 固件加载、入口与复位（QE-A005-02/03）

`-bios` 只给 machine 传入固件文件名，并不会替新 machine 自动建立 ROM 或入口。锁定 `hw/arm/virt.c:1251–1280` 示范读取 `MachineState.firmware`、解析 BIOS 路径并向实际 MemoryRegion 调用 `load_image_mr`。tiger 应明确选择自己的 ROM，不能照搬 virt 的 CFI flash 设备或整套初始化。

`hw/core/loader.c:143–163` 限制目标为 RAM/ROM，读取失败或超过 MemoryRegion 长度时返回负值，但空文件返回 0。因此本章应要求 **0 < binary 长度 ≤ 1 MiB**，缺失、不可读、空文件、超长均在运行 CPU 前显式报错；ELF/map 的 `_start` 和入口地址为 0，raw 首条指令对应同一入口。仅 raw 长度合法不能证明入口正确，须保留源、链接脚本、ELF/map 与 binary hash 的关联。

`target/arm/cpu.c:302–313` 在 reset 时进入最高可用 EL 并将 `rvbar_prop` 采样到 PC；QOM 属性名为 **`rvbar`**（1572–1575）。A53 默认 `has_el2/has_el3=true`（1377–1382），故保留 EL3 时 CPU0 从 secure EL3h、PC=0 开始。应在 realize 前设置/核对这些属性、CPU 的 `memory` 与所需 `secure-memory` 连接；`virt.c:2181–2187` 会按其参数关闭 EL3/EL2，不能直接复制后继续声称 EL3 入口。此最小路线不承诺安全隔离或已安装 BL31。固件建立自身栈/可写区、维持中断禁用；不得使用 Linux 启动寄存器约定或假定 PSCI 已可调用。

**CPU 创建不等于系统复位接线成立。** `hw/arm/boot.c:1204–1220` 明确为各 CPU 注册 `do_cpu_reset`；其 685–723 调用 `cpu_reset`。可以采用该锁定版本的 `arm_load_kernel` 纯 firmware 路径（有效生命周期的 boot_info、没有 kernel/DTB、标明固件已载入），也可以显式注册等效 CPU reset handler；应选定一种且避免重复注册。1150–1202/1236–1239 的纯 firmware 路径保留 `env->boot_info=NULL`，reset 使用 CPU 的 rvbar；混合 kernel/firmware 路径另需 fw_cfg，不属于本章。

验收应拆分为：

1. 首次冷启动：CPU0 从入口执行、固件初始化 RAM 中的运行区并经 UART 输出一次确定标记。
2. **模型系统复位**：通过宿主 QMP/HMP `system_reset` 触发，核对 CPU 返回同一入口/EL、UART FIFO/屏蔽状态复位及固件再次执行。PL011 注册 reset 回调（`pl011.c:493–517`）。不默认清零全部 RAM，固件须重新初始化自己的 data/BSS；ROM 内容及再次入口在实际测试中核对。
3. **guest 请求 reset**：后续选定可用 PSCI/板级寄存器/watchdog 接口后验证；本章不假定 BL31 或 QEMU SMC/HVC 服务。
4. 四核、GIC/timer IRQ：在真实唤醒/中断契约具备后验证，不与最低串口启动合并。

若坚持本章创建 4 核，则需显式设置并测试 CPU1–3 的 `start-powered-off`、affinity、reset 和唤醒责任，不能只填 `-smp 4`。boot.c:1273–1292 在 EL3 firmware 情况会禁用 SMC PSCI，自动停车 secondary 的代码只在 emulated PSCI 启用时执行；因此不能从 virt/direct-linux 推出 EL3 firmware 的 secondary 自动停车或可用 CPU_ON。建议本章先单核，后续再引入四核，仍保留全书最终目标。

## 4. UART 与真实执行证据（QE-A005-04）

`hw/char/pl011.c:36–48` 给出创建、chardev、realize、4 KiB MMIO 和 IRQ 接法；`virt.c:2274` 使用 `serial_hd(0)`。锁定 `poky/scripts/runqemu:1481–1505` 的 nographic 分支添加 `-serial mon:stdio`。按 Y-A005-03 清除继承的 virtio/PCI/额外串口参数，使 **UART0 接第一个实际 serial backend**，记录最终参数，避免已有 `-serial` 选项改变序号或重复占用 stdio。

建议固件用对齐的 32 位寄存器访问（PL011 的实现访问宽度为 4 字节，381–387）、轮询 UARTFR、正确配置 UART 后写 UARTDR。254–270 的 DR 写最终调用 `qemu_chr_fe_write_all`，但模型没有强制检查 TX enable，所以单看输出不能证明控制位、真实波特率或中断正确。PL011 仅有一个模型时钟输入 `clk`（479–480）；如保留 24 MHz 契约，连接真实固定时钟源并按 `qdev_connect_clock_in` 要求在 realize 前完成（头文件 69–79）。DTS 的 `uartclk/apb_pclk` 名称不代表有两个模型 clock 输入。若本章只覆盖字符功能，应明确未验证波特率时序；generic timer 的 `cntfrq` 是另一独立 CPU 属性，不能由 UART 时钟自动推得。

实施时证据应关联：tiger patch/metadata 与固件源精确版本、ELF/map/raw hash、生成 qemuboot hash、实际 native QEMU 路径/hash、TCG/单核/ROM/RAM/串口最终参数，以及完整日志、运行终态和退出方式。标记由 guest 固件代码在约定 MMIO 写出，至少结合入口/EL 的寄存器或调试证据与 UART 写路径证明实际执行；宿主初始化打印、machine 枚举、`-S` 暂停或对象创建不算启动通过。随后按 D-013 清理 devtool workspace 后重构建及重验。以上均为待实施的判据，本轮未生成输出样例或宣称实测。

## 5. SPI NAND 可复用范围与首次读取门槛（QE-A005-05）

落实 U-A005-01：采用 **SPI master → CS → SPI NAND**；删除独立并行 NAND 控制器窗口 `0x09050000` 及其专用 IRQ 37，保留候选 SPI 窗口/IRQ。SPI 芯片通常由 CS 选择，不能据此断言不需要 SPI 主控或沿用并行 NAND IRQ。

| 锁定模型/API | 可复用范围 | 尚缺契约 |
|---|---|---|
| `hw/ssi/pl022.c` | 已有 MMIO SPI master、FIFO、IRQ、`ssi` bus 和 `ssi_transfer` | **没有 CS 输出**；忽略实际线速/clock format，DMA 未实现，不能承诺双/四线及真实时序 |
| `hw/ssi/ssi.c`、`include/hw/ssi/ssi.h` | SSI peripheral、transfer、set_cs、极性、`SSI_GPIO_CS` | `cs` 属性是唯一 bus 地址，不能代替片选电平切换 |
| `hw/block/m25p80.c` | NOR 的 SSI peripheral、CS 回调、块后端/迁移结构可作实现参考 | 这是 SPI NOR；不是 page-cache/OOB/ECC 的 SPI NAND |
| `hw/block/nand.c` | 并行 NAND 几何/存储实现可作后端参考 | `nand_setpins(cle,ale,...)`、setio/getio 是并行接口；直接挂 SSI 不成立 |

PL022 全文件核对见 D1：284–293 创建 MMIO/IRQ/SSI bus，73–116 在 FIFO 处理中调用 `ssi_transfer`；没有 CS GPIO 或 sysbus CS 输出。SSI.c:30–55/112 的 `cs_index` 检查与属性只选择总线地址；72–108 的 GPIO 电平、极性和 `set_cs` 回调才承担事务边界。`stellaris.c:1250–1264` 演示板级 GPIO 接入 `SSI_GPIO_CS` 并初始化为未选中，说明 CS 必须实际接线；它不构成虎板接线已完成的证据。

如果选 PL022，architect 需同时选定 guest 可控制的 GPIO/片选来源、复位未选中电平和 TF-A/U-Boot/Linux 对应操作；或者另选确有 CS 接口的现成 master 并重新核对。芯片命令采用单线 8 bit 是否足够，须按最终型号与所需操作确定。不能仅 `ssi_create_peripheral(...,cs=0)` 就宣称可用，NOR/SPI NAND 的命令边界可能在 CS 释放时结束。

在锁定源码 `hw`/`include` 的 .c/.h 内检索 `spi.?nand|spinand` 无命中，未识别到可直接复用的 SPI NAND 模型。本轮结论是**需要补充芯片实现或取得并审查外部实现**，不是在本轮实现，也不是全世界没有可用模型。若采用新 SSI 芯片模型，至少覆盖所选 ID、reset、get/set feature/status、page-read 到缓存及 cache-read、write-enable、program-load/execute、erase、busy/error 与 CS 边界；精确 opcode/dummy/地址/OOB/ECC/坏块和后端持久化布局按数据手册确定。既有 SSI、BlockBackend 和 NOR/并行 NAND 结构可减少基础设施工作，不能替代芯片协议与错误语义审查。

**阶段门槛应设在首次从 SPI NAND 读取 BL2/FIP 之前**，可能早于 chapter 9/10。须具备 master/真实 CS、所选芯片最小读取协议及 reset/status、后端镜像布局和所需 ECC/坏块处理、TF-A 平台初始化/IO 读取，并实际验证读取字节与目标镜像相同。若先采用其他测试介质，则写清有限覆盖；完整 program/erase/UBI/A/B/持久化按相应章节补齐。512 MiB、2 KiB 页、128 KiB 块和分区表仍为待型号确认的候选，不由本轮静态算术批准。

## 6. 后续兼容约束与覆盖限制（QE-A005-06）

- **GIC/timer（Q-02/Q-06）**：`virt.c:716–756/789–851` 有 GICv2 revision/num-cpu/num-irq、MMIO 及 CPU IRQ/FIQ/VIRQ/VFIQ/各核 PPI 接线参考；`num-irq` 含 32 个内部 INTID，GPIO 下标不等于所有情况下的 INTID。`include/hw/arm/bsa.h:27–31` 给出 timer INTID 26/27/29/30。后续需明确外设 SPI 的 INTID 与 GIC 输入转换、每核 PPI、EL/安全分组、`cntfrq` 和固件 handler；本轮未逐寄存器审核 GIC 或证明 IRQ 运行。
- **TF-A/U-Boot/direct-linux（Q-05/Q-06）**：沿用 Yocto 的“TF-A/meta-arm 未启用，未来源码尚未锁定”边界。ROM/SRAM/DDR 地址保留需避免后续链接区重叠；真正 BL31 PSCI provider/conduit/affinity 与 Linux entry/DTS 契约在对应阶段取得源码后确认。本轮不批准 BL2/BL31/BL33 入口或三种 boot 模式。
- **watchdog（Q-08）**：在 hw/include 的 .c/.h 检索 SP805，仅命中 vexpress 两处注释，其中 383 行明确 `not modelled`；watchdog 目录没有 SP805 模型。当前存在 CMSDK APB watchdog 和 SBSA generic watchdog，F/G 保存其来源与 hash；它们不能凭名称直接视为 SP805 已兼容。后续需选择/适配模型并核对寄存器、时钟、LOAD=0、两级超时/IRQ/reset、Linux 驱动及教学 bug 边界；不提前实现，也不制造“上游 SP805 bug”结论。
- **存储/其他设备（Q-07/Q-09/Q-10）**：ECC/OOB/坏块/UBI/A/B/环境持久化、SPI NOR/EEPROM 型号及驱动、网卡模型与 guest 网络路径仍未验证。slirp 参数不产生 guest 网卡；RTC 不属于本章最低门槛。

已覆盖：固定稿及既有意见、声明工作区基线、选定 QEMU 源码来源一致性、CPU/固件加载/reset/PL011 API 的静态核对、四核/GIC/timer/guest reset 的阶段拆分、SPI master/CS/芯片模型可用性及首次读取依赖、后续 watchdog 可用性缺口。

未覆盖：全源码树一致性、BitBake 最终选择展开、tiger patch/固件/配方/qemuboot 实现、构建或 native emulator 执行、guest CPU/UART/reset/IRQ/SMP 实测；具体 SPI NAND 数据手册与实现、TF-A/U-Boot/Linux 平台源码/驱动/全链验证，所有后续存储/watchdog 实测。未覆盖项按本章或后续 gate 处理，不以既有 qemuarm64 产物替代 tiger 结果。

## 7. 治理交付与后续

本轮只提交 TASKS 的 qemu-engineer 意见、本报告及原始日志。受评稿和其他成员意见保持原样；作者处理回复及复核留待本人负责者实际填写。校验结果在原始日志 V 中追加：框架结构/链接、diff 空白、固定 blob 与写入边界检查；这些检查不验证模型或启动。

提交后本成员停止，等待 project-manager 串行衔接 architect 汇总两名工程成员意见并统一修订。后续必要专业复核及用户确认形成 chapter 4 实施基线；本轮不代替批准或启动实施。

实际校验：框架校验返回 0，覆盖 21 个角色、36 份技能、1 个项目、1 套模板及 475 处本地链接；Python 对比确认固定 A-005 blob 不变、TASKS 本人意见之前与 T-017 起至文件末尾内容不变；书稿基线及干净状态复核通过。暂存区仅本轮三份治理文件，最终 diff 空白检查结果见原始日志 V。
