# chapter 4 章节蓝图：让 QEMU 长出 tiger 这块板

```text
项目：book-yocto
成果 ID：A-006
负责人：writer
关联任务：T-016 章节蓝图阶段；跨章影响供 T-017 使用，T-017 尚未启动
版本：0.1
更新日期：2026-10-11
状态：draft
适用范围：chapter 4 的教学结构、待实施实验、工程资产交接和正文回填；本版不含书稿正文或工程实现
来源及输入版本：T-016 章节蓝图安排（OPC 2fcec08417fbfb2362b6649dfd78c98571b46cbf）；A-005 v0.2 approved / D-014；A-002 v0.8 draft、A-003 v0.1 draft、A-004 v0.5 approved；D-001/D-007/D-010/D-011/D-013；书稿 f444790f352b46f699fd3d46198f8b9b9f089a62；当前 TEAM、writer 与工程成员 scope、WORKSPACE
设计固定版本：A-005 获批提交 08bdf9c08d6d8cbd8836df3771ffd28231470059，原 blob 26c5fdca1103f37ac3e752208c429231b8191eee；批准元信息回写 28bdc2bd0aea4c575dd286f7c71f3a5793dd5f0c，技术正文不变
批准依据：本蓝图尚无批准；D-014 批准的是 A-005 v0.2 设计，不是本蓝图、实现或章节定稿
版本记录：2026-10-11 v0.1 首次蓝图，按已批准最小启动设计组织九节教学、工程责任、证据及验收映射
稳定章节标识：书稿 yocto/task05-4-让QEMU长出tiger这块板.md；显示章号为第 4 章，不按文件中的 task05 推导章号
```

## 1. 输入效力与本轮读取

本章终点是：清理临时 devtool workspace 后，依靠锁定 Poky 与正式 `meta-tiger`，重新构建 native QEMU 和最小固件，经 `runqemu` 让 CPU0 实际执行固件、输出可追溯串口标记，并通过同进程宿主 reset 复验。以最小启动解释 patch 集成，不提前实现全平台。

- 已批准技术契约以 [A-005 v0.2](A-005-tiger-platform-design.md) 和 [D-014](../DECISIONS.md) 为准。本版引用其 §4/§5/§10，不增加 boot mode、CPU reset handler 或设备。A-005 正文保留的 draft/待确认描述属于获批前时点，当前效力按其批准元信息及 D-014 判定。
- writer 本轮刷新身份、角色、scope、四项有效技能及工作区声明；读取 T-016 蓝图安排、相关决定、A-002 v0.8、A-005 全文、两份工程来源报告及选定原始摘录。此前完整读取的 chapter 3/4、A-003/A-004 和公共协议经版本/内容未变核对复用；补读 chapter 2 结尾、chapter 5 开头、chapter 15 的 devtool 衔接段。
- 本机 OPC 开始 HEAD 为 `2fcec08417fbfb2362b6649dfd78c98571b46cbf`，分支 `feature/book-yocto/t-016-chapter4-revision`，工作树干净；书稿在 `feature/t-016-chapter4-revision`，HEAD 与 `main` 均为 `f444790f352b46f699fd3d46198f8b9b9f089a62`，工作树干净。未切换分支或修改书稿。
- 远端资料仅引用 [Yocto 来源报告](work-records/t016-a005-yocto-review.md)（固定 `dd15379ebea54b36dab7b5cc2638fd345e71553f`）和 [QEMU 来源报告](work-records/t016-a005-qemu-review.md)（固定 `1c8304446f8bf82fd46a56413a5fa5787e271ae6`），各自附有原始日志。writer 没有重新 SSH、联网、解析 BitBake、编译或运行 QEMU。设计针对性复核的两个 pass 见 T-016，不能用于宣称实现通过。
- 已有证据：Poky `77d1feb37e280733684ae8a9449fb031d5d7ff40`；QEMU 8.2.7 tar 包 SHA-256 `1f0604f296ab9acb4854c054764a1ba408643fc299bd54a6500cccfaaca65b55`；29 项通用加 2 项 native OE patches，已有应用序列文件 SHA-256 `3b7c0560451d78d8eae6512a4c21d77129eb3b0e1106512949709e8ef0eb8430`。已有构建环境是 `qemuarm64`，`meta-tiger` 基线仅有 layer 骨架；尚无 tiger 模型、fixture、machine 配置、bbappend 或启动结果。31 项名称及顺序引用报告/原始日志，不用新 tiger patch 覆盖它们。

所有下文工程文件、命令、输出位置和验收用例都是**待实施计划**。唯一完成的对象是本蓝图及本地文档检查。下一步由 project-manager 核对交接，再按已登记次序串行衔接工程成员。

## 2. 读者前提、学习目标与边界

读者已完成 chapter 1 的 Yocto 构建环境准备、chapter 2 的全书工程目标理解及 chapter 3 的 `meta-tiger` layer 骨架与 `bblayers.conf` 登记，能读 shell 命令、Git diff、简单 C/汇编和 BitBake 变量。实验需要可用的 AArch64 交叉工具链及 Yocto native 构建环境；具体路径、资源和宿主条件由真实核验报告回填。不能要求读者先有 TF-A、U-Boot、Linux 或 rootfs 来完成本章。

读完本章，读者应能：

1. 区分 QEMU 的 `-machine tiger`、Yocto 的测试配置 `MACHINE=tiger` 与后续完整 BSP 的 `tiger-aarch64`，解释本章单核和预留能力的关系。
2. 用最少 devtool 命令取得配方锁定的可修改源码，读懂真实 tiger patch 如何创建 CPU、映射 ROM/RAM、连接 PL011、装载 raw 固件并注册 reset。
3. 解释 raw 偏移 0、ROM VMA/data LMA/RAM VMA、入口/栈/data/BSS 的对应关系，指出串口字符为何不足以单独证明正确 UART 配置。
4. 把真实 patch 交给 `.bbappend`/`SRC_URI`，用展开结果和日志判断适用配方、已有/新增 patch 顺序，清理 workspace 后证明正式集成仍能重建。
5. 使用版本化 fixture 生成的 `.qemuboot.conf` 启动，核对实际 native QEMU、BIOS 与参数；通过 CPU/guest MMIO/串口及宿主 reset 证据判断最小启动成功。

本章只简释 `devtool status`、`devtool modify` 的使用，以及收尾所需的 `devtool reset`；workspace 内部机制、多配方开发和 `devtool finish` 的系统讲解留给 chapter 15。QEMU C 代码按初始化链讲解，完整可运行文件由工程资产提供，不把伪代码包装为完整 patch。

### 2.1 本章必须保持的最小契约

| 项目 | 本章设计基线 | 教学和验收边界 |
|---|---|---|
| CPU/运行方式 | TCG、AArch64 小端 Cortex-A53 CPU0，默认/最大 CPU 数 1，`-smp 1 -m 1024` | 其他 CPU/核数/RAM/accelerator 明确拒绝；不声称 SMP/KVM 可用 |
| ROM | `[0x00000000, 0x00100000)`，1 MiB，可执行只读；raw 入口 0 | 严格要求 `0 < raw 长度 ≤ 0x100000`；缺失/不可读/空/超长/加载失败在 CPU 执行前拒绝；1 MiB 合法、1 MiB+1 拒绝，合法长度不等于有效启动 |
| UART0 | PL011 `[0x09000000, 0x09001000)`，4 KiB，32 位对齐 MMIO，轮询 | IRQ 不接；单个 clk 固定 24 MHz，在 realize 前连接；不验收 IRQ/线速 |
| RAM | `[0x40000000, 0x80000000)`，1 GiB | 是后续兼容选择，不是最小固件必需容量；reset 不要求全部 RAM 为零 |
| 固件 RAM 使用 | data/BSS `[0x40000000, 0x40010000)`；栈 `[0x40010000, 0x40020000)`，初始 SP_EL3=`0x40020000` | 每次入口复制 data/清 BSS，栈 16 字节对齐；用链接 ASSERT 与实际状态对账 |
| 冷启动/reset | has_el2/has_el3 保留 true，rvbar=0，CPU0 非 powered-off，secure EL3h/PC=0；PSCI disabled | memory/secure-memory 共用系统内存视图，不承诺安全隔离；cntfrq 独立候选 24 MHz，不代表 timer IRQ |
| 装载与生命周期 | `MachineState.firmware` → BIOS 解析 → 实际 ROM → `load_image_mr`；纯 firmware `arm_load_kernel` 只注册一次 | machine 持有零初始化 `arm_boot_info`；先拒绝 kernel/initrd/DTB；不叠加板级 CPU reset handler，不在每次 reset 重调 loader |

最小固件保持 MMU/cache 未开启，屏蔽异常/中断，不依赖 libc、动态链接、Linux ABI、DTB、PSCI 或宿主预填 RAM。PL011 的正常设备 reset 与 CPU reset 各守其责任。预留窗口不映射、不创建假设备。

纯 firmware loader 的 `arm_boot_info` 设置 `firmware_loaded=true`、`is_linux=false`、PSCI disabled，无 DTB/secondary hook；在 CPU/ROM/UART 建立及 BIOS 成功后调用一次，相关对象全程有效。固件编译核对禁用 builtin、栈保护、PIE 等隐式要求；text/rodata 在 ROM，data 初值 LMA 留在 ROM、VMA/BSS 在 RAM。UART 初始化按关闭 UART→屏蔽并清中断→8N1/分频/控制位→使能 UART/TX→轮询 TXFF→写 DR 的顺序实施，并对照锁定源码读回配置。

SPI master/实际 CS/SPI NAND、SRAM/GIC/timer IRQ/四核、guest reset、RTC、watchdog、完整 TF-A/U-Boot/Linux 链均属后续能力，见 §8 的首次使用门槛。本章串口成功也不证明这些能力。

## 3. 九节内容蓝图与正文回填位置

沿用显示节号 4.1–4.9，按新主线调整内容。以下节名为作者修订方案，尚未写入书稿；其他章节指向旧节义的链接只登记供 T-017 处理。

| 节 | 核心问题与讲解顺序 | 例子/图表及回填对象 | 读者检验理解 |
|---|---|---|---|
| 4.1 从配方锁定的源码开始 | 承接 layer 骨架；说明本章成功标准与后续边界；核对 provider/PV/SRC_URI；最少 devtool 取得源码 | 图 4-1：锁定基线→临时 workspace→真实修改→正式 patch→清理→重建→启动；E-01/E-02 的真实命令与定位输出 | 为什么旧 `qemuarm64` 二进制或另一个 QEMU tag 不能当作 tiger 修改基线？ |
| 4.2 用真实 patch 建立最小 machine | 注册/构建入口→CPU0→ROM/RAM→时钟/串口→固件装载→一次性 reset 注册 | 表 4-1：已实现三窗口与未实现能力；按真实 patch 文件名/函数截取关键段，E-03；完整文件位置及提交 | 仅 `-machine help` 或 `-S` 成功能证明什么？CPU 从哪里取第一条指令？ |
| 4.3 让 CPU 执行最小固件 | raw 与 ELF 的角色；入口/栈/data/BSS；轮询 UART 的初始化与 guest 标记；暖复位哨兵 | 图 4-2：ROM LMA 与 RAM VMA；E-04 的源、ELF/map/raw 对账，完整固件文件引用 | data 初值为何在 ROM，而运行地址在 RAM？为何 reset 后再次初始化？ |
| 4.4 把 patch 留在 meta-tiger | `.bbappend` 匹配、`FILESEXTRAPATHS`、`SRC_URI`、真实 patch 顺序、许可/`Upstream-Status` | 正式 layer 目录树、bbappend/patch 必要全文或完整资产引用；E-05 展开/应用日志 | 如何判断 append 确实作用于 system emulator？哪些字段决定补丁次序？ |
| 4.5 生成固件与 runqemu 输入 | 独立测试 machine 与 fixture；编译→生成配置→deploy；helper native 工具与路径；为什么没有 image/rootfs | 图 4-3：fixture 任务依赖；配置表/真实部署目录，E-06；完整配方及生成器引用 | 为什么本 fixture 不继承 `qemuboot.bbclass`？conf/BIOS/helper 如何关联？ |
| 4.6 清理 workspace，再从正式 layer 构建 | 保存真实修改/patch/metadata→reset→证实 override 消失→排除旧产物→干净重构建 | E-07 的清理前后 status、provider/SRC_URI、执行任务和 hash；精确命令与真实耗时按日志回填 | workspace 删除后为何仍应成功？怎样排除旧 work/sstate/旧 helper 的假成功？ |
| 4.7 用 runqemu 验证启动和宿主 reset | 显式 conf 启动→核对最终程序/参数→入口及 guest 标记→暂停/reset/观测/cont→第二标记 | E-08/E-09 的原始证据摘录，控制顺序、终态与覆盖限制；此节成为本章实际成功终点 | 宿主打印与 guest MMIO 如何区分？第二标记为何还需要 reset 状态证据？ |
| 4.8 根据证据排查问题 | 未生效 append/遗漏 patch、错误原生 QEMU、BIOS 输入错误、重复 stdio、错误部署路径；区分预期失败与主线失败 | E-10 的负向矩阵；若保留错误注入示例，必须有真实注入/恢复/复验记录 | 怎样从日志判断是构建、配置、加载还是固件执行问题？ |
| 4.9 小结与下一章边界 | 复述可重复的完整最小实验；列出已验收和未启用能力；连接后续 BSP/启动链与 chapter 15 | 简短成功清单与后续依赖表，不附内部任务编号或章末 Git tag 流程 | 本章能承诺什么？第一次读 SPI NAND 前还缺哪些工程资产？ |

篇幅预算以讲解正文约 7,000–9,000 汉字为首轮目标，完整源码另按真实资产长度计：4.1/4.2/4.3 约 35%，4.4/4.5 约 30%，4.6/4.7 约 25%，4.8/4.9 约 10%。优先保留复现链、真实 patch 和证据；压缩重复历史争论、虚构 commit 故事和与本章无关的设备代码。故事人物可保留用于提出问题，不能虚构其已完成的工程经历或测量结果。

术语首次出现解释 native system emulator、raw/ELF、VMA/LMA、MMIO、fixture、deploy、qemuboot 和宿主 system_reset；已有 layer/recipe/append 只做必要回顾。新增术语及统一译法待正文修订时同步全书术语记录，不在本蓝图改动 A-003。章/节使用 H1/H2/H3；硬件型号、代码性标识、真实输出空白和作者等宽图表执行 A-004 v0.5。旧“图”中的纯比较表改为表，不复制错位空格。

## 4. 工程交接资产与责任

以下均为**计划位置，当前未创建**；产品代码、构建资产与其工程中间文件只落授权 `meta-tiger`/临时开发工作区。OPC 留报告、索引和原始证据，不代替工程仓库。meta-tiger 声明基线为 `0e3da48b34ef772532535a730a21878cc30ccf67`（主机 `tiger`，`/home/oops/workspace/meta-tiger`）；实际访问/写入前由执行成员核对授权、checkout、洁净状态和版本。

| 交接包 | qemu-engineer 主责 | yocto-engineer 主责 | 本章需要拿到的对象 |
|---|---|---|---|
| QEMU 模型与 patch | 基于准确 tar+31 OE patches 的 tiger 模型、构建/注册修改；CPU/ROM/RAM/UART/clock/loader/reset；代码正确性及模型测试；形成真实新增 patch 和应用顺序 | 核对实际 recipe/provider/override；把最终 patch、bbappend/SRC_URI 集成到正式 layer，保留 OE patches | `recipes-devtools/qemu/` 下按实际目标匹配的 bbappend、patch 文件及 hash；源基线、提取基点、工程提交；不预设虚构 patch 数量/commit |
| tiger-minboot 源 | `recipes-test/tiger-minboot/files/start.S`、`main.c`、`tiger-minboot.ld`、许可；入口/对齐/初始化/轮询 MMIO/标记/哨兵；源与模型联测 | 从同一版本源编译，核对工具链/flags、ELF/map/raw 及 manifest 的一致性 | 完整源、链接脚本、许可，源/构建 ID 生成规则；避免 raw 自身 hash 递归嵌入标记 |
| fixture 构建/部署 | 交付 freestanding 假设、链接 ASSERT、有效镜像/边界测试需求 | `recipes-test/tiger-minboot/tiger-minboot_0.1.bb`，编译/生成 conf/deploy 任务及依赖、vardeps/签名、持久 helper sysroot；deploy/sstate 正式发布 | ELF/map/raw/manifest/conf 一套版本化产物；工具路径与 flags；任务图和日志；不能以手工拷入公共 deploy 目录替代 |
| 最小测试 machine | 提供 CPU、RAM、串口和拒绝项契约；核对最终 QEMU 参数 | 计划 `conf/machine/tiger.conf`：独立 AArch64 tuning、最小 fixture 使用的字段；不整份继承 qemuarm64/virt | 实际 MACHINE/tuning/provider 展开及 conf 生成输入；本章不会交付完整 Linux BSP |
| 模型验收 | 登记/只读 ROM/非法参数/BIOS 边界、CPU/EL/RAM/MMIO、同进程宿主 reset；控制手段和模型状态 | 在正式重构建的二进制上串行复验并关联部署资产/参数 | 模型命令、脚本位置、原始日志/返回码、寄存器/trace 来源、实际结论与未覆盖项 |
| 系统集成 | 说明模型/固件 patch 接口，处理模型问题；不能代写系统 pass | 最少 devtool 实际路线、patch 正式集成、workspace 清理、干净 BitBake/fixture/helper 重构建及 runqemu；逐章环境核验 | 从工程提交到实际执行文件的完整证据链；失败与修复/重跑记录，不只选取成功字符 |

工程次序按 T-016：蓝图交付后先由 project-manager 核对，再衔接 qemu-engineer；其获授权并复核基线后建立远端 `feature/t-016-chapter4-revision`，完成模型/固件源与测试交接并停止。yocto-engineer 随后复用同一任务分支串行集成，完成正式重构建与启动验证并停止。writer 再据真实工程证据校准正文；architect 处理设计契约问题，reviewer 执行独立审校。蓝图不发送通知、不创建工程分支、不把工程分工当作工程成员已接收或验收。

### 4.1 fixture 与启动配置的验收输入

保留 A-005 §5.4 的独立 target 配方路线：`inherit deploy`，自定义版本化 ConfigParser 生成 `[config_bsp]`，不继承依赖 image/rootfs 任务的 `qemuboot.bbclass`。`do_generate_qemuboot` 在 `do_compile` 后、`do_deploy` 前，显式依赖 `qemu-helper-native:do_addto_recipe_sysroot`；helper 经其 DEPENDS 获得 `qemu-system-native` 等，`do_deploy` 在 `do_build` 前。实际图与签名必须重新解析证明。

raw/conf 共用 `tiger-minboot-0.1-tiger` 版本名，后缀为 `.bin`/`.qemuboot.conf`；ELF/map/manifest 同批发布。配置从 DEPLOYDIR 经 deploy/sstate 进入 DEPLOY_DIR_IMAGE。版本变更同步源标记、配置和 manifest，不把设计版本 0.2 当作固件 PV。

| 字段组 | 必须生成并核对的值 |
|---|---|
| machine/架构/程序 | `MACHINE=tiger`、`TUNE_ARCH=aarch64`、`QB_SYSTEM_NAME=qemu-system-aarch64` |
| QEMU 最小参数 | `QB_MACHINE=-machine tiger -accel tcg`、`QB_CPU=-cpu cortex-a53`、`QB_SMP=-smp 1`、`QB_MEM=-m 1024` |
| 无 kernel/rootfs | `QB_DEFAULT_KERNEL`、`QB_DEFAULT_FSTYPE`、`QB_ROOTFS` 均为字面 `none`；`QB_KERNEL_CMDLINE=none`，最终无 `-kernel`/`-initrd`/`-dtb`/`-append` |
| BIOS/部署 | `QB_DEFAULT_BIOS=tiger-minboot-0.1-tiger.bin`，与 conf 同目录；`DEPLOY_DIR_IMAGE=.`，按最终 conf 位置展开 |
| native/target 路径 | `STAGING_BINDIR_NATIVE` 指向 helper 的 `recipe-sysroot-native/usr/bin`；`STAGING_DIR_NATIVE` 指向该 helper 根，`STAGING_DIR_HOST` 为实际 target recipe sysroot。按最终部署 conf 位置计算 `../` 相对路径并检查，不能从临时 DEPLOYDIR 算错层级；绝对路径须声明绑定构建树 |
| 无网络/额外设备 | `QB_NET=none`，运行也传 `nonetwork`；`QB_RNG/QB_GRAPHICS/QB_OPT_APPEND/QB_ROOTFS_OPT/QB_NETWORK_DEVICE/QB_TAP_OPT/QB_TCPSERIAL_OPT` 为空 |
| serial/无 KVM | `QB_SERIAL_OPT/SERIAL_CONSOLES/QB_CPU_KVM/QB_KERNEL_CMDLINE_APPEND` 为空；`nographic` 生成的第一项 `-serial mon:stdio` 接 UART0 的 `serial_hd(0)`，不再追加 QEMU `-nographic` 或第二个 stdio |

`runqemu` 脚本允许补出的第二项 `-serial null` 只是空 backend，不等于模型有第二个 UART，也不等于重复 stdio；记录实际参数并核对此分支。最终必须使用新构建的 helper sysroot 内 native QEMU，含本版 CPU/TCG/RAM/BIOS/serial 和 `-nic none`，不夹带 virt/virtio/PCI/USB/网卡/磁盘。锁定脚本在 nonetwork 处理前仍检查 `/dev/net/tun`，工程执行前重新核对宿主条件；路径或条件失败如实登记，不能用宿主 PATH QEMU 或 slirp 掩盖问题。

## 5. 待实施命令与输出回填清单

本节命令是工程规划，不是实际执行记录，也不是可原样运行的完整读者教程。带尖括号的是等待工程核定的参数。最终正文必须替换为执行成员在受控版本验证过的精确命令、环境初始化、工作目录、返回码和日志摘录；读者无需猜参数或补缺失文件。

| 顺序/正文位置 | 待实施命令或动作 | 必须回填的输出/判据 |
|---|---|---|
| P-01 / 4.1 | 核对主机/路径/授权、各仓库 `git status`/`rev-parse`，初始化实际 Yocto 环境；`bitbake -e qemu-system-native` | Poky/meta-tiger/构建目录精确版本与状态；recipe/provider/PV/SRC_URI/override；与已知 8.2.7+31 OE patches 比较，若变化先协调 |
| P-02 / 4.1 | `devtool status`；`devtool modify <确认可修改的 QEMU 配方名>` | 实际命令、devtool 对 native/变体的支持及选择、源码路径、基点和有效补丁。system emulator 目标仍是 `qemu-system-native`，不能误用仅 user emulation 的 `qemu-native`；若需基础配方开发，须证明源码和最终 native 构建仍匹配锁定基线 |
| P-03 / 4.2–4.4 | 实现真实模型/固件源、构建联测；对 tiger 增量 `git diff`/按实际基点 `git format-patch` 导出至授权工程位置 | 实际源提交、patch 内容/文件名/hash/顺序；仅导出新增修改，不把已有 OE patches 再复制一遍。patch 导出基点与整洁历史由工程成员核定 |
| P-04 / 4.4–4.5 | 正式集成 bbappend、patch、测试 machine、fixture 配方；`bitbake-layers show-appends`；`bitbake -e qemu-system-native`；`bitbake -e tiger-minboot`；`bitbake -g tiger-minboot` | 目标 append 确实生效、完整 SRC_URI/应用顺序、MACHINE/tuning、生成字段、helper/生成/deploy 依赖、任务签名及 staging 路径。不能仅凭文件名或 layer priority 宣告成功 |
| P-05 / 4.5 | `bitbake tiger-minboot` 及所需依赖构建；检查部署资产 | 真正执行的编译/生成/deploy 任务，工具链/flags、ELF/map/raw 范围与 hash、conf 和 manifest，一组可追溯版本；首次开发成功尚未通过干净复验 |
| P-06 / 4.6 | 先确认增量已进工程 Git，再 `devtool reset <实际修改的配方名>`、`devtool status`；按工程成员验证的清理/独立构建目录方案排除旧 work/sstate/helper，再仅用正式层重构建 | 外部源码 override 已消失；真实执行 fetch/patch/compile/生成/deploy/helper sysroot，而非只命中旧缓存；清理对象、保留下载缓存条件、命令/返回码、日志及新产物 hash。不能用 reset 一个命令自动证明“干净” |
| P-07 / 4.7 | 对实际部署 conf 执行下列 runqemu 计划命令；记录启动及退出 | 可执行文件绝对路径/hash、完整最终 argv、实际 BIOS路径/hash、配置 hash；CPU0 入口/EL、guest MMIO 和标记；循环运行的受控退出方式/返回码，不能把仍运行写成退出 0 |
| P-08 / 4.7 | 同进程经 QMP/HMP 暂停→`system_reset`→恢复前状态读取→`cont` | monitor/调试工具版本、完整控制顺序及状态；PC=0/secure EL3h、UART reset 值/FIFO/屏蔽状态对照；第二标记与哨兵再初始化。具体状态读取命令由 QEMU 成员核定，避免读寄存器的副作用干扰判据 |
| P-09 / 4.8 | 执行参数/BIOS/ROM 负向矩阵；若设计教学错误注入，完成注入→预期失败→恢复→主线重跑 | 每例输入、执行文件/配置版本、返回码/错误及 CPU 未执行证明；教学注入单独标记。预期拒绝是负向通过，正常主线失败不能标通过 |

以下路径来自已有构建目录，只是 A-005 给出的**待实施计划值**；文件尚未产生，实际 TMPDIR/deploy 路径由工程核验后回填：

```sh
bitbake tiger-minboot
runqemu /home/oops/workspace/build/tmp/deploy/images/tiger/tiger-minboot-0.1-tiger.qemuboot.conf nographic nonetwork
```

只把显式 `.qemuboot.conf` 传给 `runqemu`，不把 `.bin` 位置参数误当 BIOS。预计 guest 标记格式为 `TIGER-CH4 FW=<source-build-id> CPU0 EL3 INIT=OK UART0`，**这是格式示例，不是实测输出**；最终采用真实固件实际生成的文字，manifest 单独关联源/配方提交和 ELF/map/raw hash。

## 6. 可验证对象、原始证据与验收映射

以下 E 项全部未实施。A-005 的 V 编号用于设计验收映射，本蓝图不回写其状态。静态来源已确认不等于新 tiger 的 V-04～V-07 已通过。

| 证据包 | 待验证对象/通过条件 | 主责及 A-005 映射 | 正文位置 |
|---|---|---|---|
| E-01 基线 | 精确 Poky/包校验/31 OE patches/meta-tiger 基线，实际 native provider 和 override 展开；工程版本与输入一致或已协调 | Yocto，V-04；工程 checkout 记录参照 V-01 | 4.1 |
| E-02 workspace | 可修改源码实际来自锁定基线；记录 devtool 命令/路径/基点，最终能脱离外部源码 override | QEMU 开发、Yocto 核对，V-04/V-07 | 4.1、4.6 |
| E-03 模型 | 编译与注册；三窗口/ROM 只读；CPU/加速器/核数/内存及 kernel/initrd/DTB 非法输入拒绝；clock/reset 生命周期无重复注册 | QEMU，V-03 的范围落实、V-05 | 4.2、4.8 |
| E-04 固件 | 源/ELF/map/raw 对账；ROM/data/BSS/栈 ASSERT 与实际范围；从 PC=0/secure EL3h 执行、data/BSS 初始化及 32 位 MMIO UART 标记，配置位读回 | QEMU 源与执行、Yocto 编译资产，V-06 | 4.3、4.5、4.7 |
| E-05 正式 patch | 可读真实增量、许可/Upstream-Status、bbappend 匹配和实际 SRC_URI/应用序列；保留已有 OE patches，不能以 layer priority 直接推出所有应用先后 | QEMU patch、Yocto 集成，V-04/V-07 | 4.4 |
| E-06 fixture | deploy 配方/生成器/依赖图/vardeps/签名、版本化发布、有效 staging 路径及 hash、最终无 kernel/rootfs 的参数契约 | Yocto，V-07 | 4.5 |
| E-07 干净重构建 | workspace/override 清除，只有锁定 Poky+正式 meta-tiger，证实新 patch/compile/fixture/helper 执行；源→任务→部署→执行文件闭环 | Yocto，V-07 | 4.6 |
| E-08 冷启动 | 干净重构建的新 native QEMU/BIOS/conf；最终参数符合设计；CPU入口/EL/guest MMIO证据与可追溯标记相互印证；明确停止终态 | Yocto 系统验证，QEMU 提供模型判据；V-06/V-07 | 4.7 |
| E-09 宿主 reset | 同进程先观察固件改变哨兵，再暂停/reset，在 cont 前确认 PC/EL/UART reset 状态，cont 后第二标记和 data/BSS 再初始化；ROM保留，RAM不要求全零 | QEMU 模型测试、Yocto 正式集成复验，V-06R/V-07 | 4.7 |
| E-10 负向/恢复 | 缺 BIOS、不可读、空文件、1 MiB、1 MiB+1、加载失败；参数拒绝/ROM 写入无效；每例有版本、返回码和状态；合法长度例不冒充启动通过；演示恢复后主线仍通过 | QEMU V-05；Yocto 对实际部署/参数错误复验 V-07 | 4.8 |

冷启动和 reset 需用真实 guest 指令/地址、寄存器/trace 或调试记录关联 UART 写入。`-machine help`、`-S`、对象创建、宿主 `printf`、已有 `qemuarm64` 日志只作有限辅助。PL011 的 DR 输出本身不强制证明 TX enable/正确分频，所以 UART 初始化寄存器与执行证据要一起提交；13/1 分频仅是 24 MHz/115200 的设计候选，不宣称线速实测。

证据报告在 OPC `artifacts/work-records/` 按实际执行形成，由执行成员保存：日期/主机/环境/工作目录；各仓库精确提交与洁净状态；命令、输入、执行文件路径/hash、配置和 patch hash；原始 stdout/stderr/控制交互、返回码或仍运行/受控退出终态；预期/实际、结论、未覆盖项；失败→修复提交→重跑对应关系。索引引用工程文件的主机、仓库、路径和精确版本，不只写浮动分支名。

模型测试报告与 Yocto 系统报告分别归属执行成员。原始日志保留完整失败和成功，不用作者整理的片段替代；正文只摘必要真实输出，注明省略/替换路径等处理。工程测试脚本及二进制归工程成果物仓库，OPC 不存这些产品资产。

## 7. 作者易错点、自查与定稿出口

- 原章从假定 `qemu-tiger` 仓库生成虚构 patch/commit 的路线改为配方锁定源码与真实工程增量；不沿用“成功运行了这三份补丁”等未经证实故事。`.bbappend` 适配真实 recipe 文件与版本，不把 PN/PV 的口头说明当作匹配和展开证据。
- 原章将 `BBFILE_PRIORITY` 直接等同于多个 append 中所有 patch 的应用次序，修订必须依据本基线解析结果解释 override、SRC_URI 和实际 patch 日志；不再复制未经运行的错误 patch 输出。
- 原章 `-S` 暂停冒烟、runqemu 暂时失败留到后章的结尾，改为本章已有最小固件/配置、实际 cold boot 和 host reset；调试中的暂停仍可用，但不能充当成功终点。
- raw 转换避免 RAM VMA 造成巨大空洞；空文件即使 loader API 接受也必须由 tiger 拒绝；1 MiB 边界例只验证长度。链接断言、PC/EL、RAM初始化和 UART 配置证据缺一时写明覆盖缺口。
- 不复制 virt 的关闭 EL3、PSCI、DTB、设备/网卡默认参数；不重复 CPU reset 注册或 stdio；不把第二个空 serial backend 描述为板上第二 UART。
- A-003 中旧 fork 假设及“前几章无 C-W/V”不能免除 D-010 的真实环境核验。本章全部可执行步骤形成实际证据后才校准稳定正文，未完成项留在治理记录，不把待填结果写成既成事实。
- 正文按 D-011 清理读者可见的内部 `task NN` 和章末 Git tag 流程，文件名/内部稳定 ID 可以保留；有实际用途的上游版本标识仍准确说明。A-004 的格式核对不能替代技术验收。

本阶段出口为版本化蓝图、本人索引/阶段记录及本地静态自查。当前没有需要 writer 擅自改变 A-005 的设计冲突。devtool 可修改配方、最终编译 flags、native/helper 路径、清理方案、UART 状态读取方法和真实 patch 名称均是工程核定/回填项，不能凭本蓝图宣称已解决；若实施发现与获批契约冲突，工程成员记录证据交 project-manager/architect 协调，不静默改变契约。

章节定稿仍需 E-01～E-10 的主线实际通过、工程证据校准后的唯一稳定受评正文、相关成员技术审校与用户通读意见到齐后统一修订、必要复核及用户明确确认。模型测试不代替独立审校，蓝图 draft 不等于 T-016 整体完成；合并、精确版本回写与分支清理由后续任务流程执行。

## 8. 前后章衔接与首次使用门槛

以下只登记影响，不启动 T-017，不修改其他正文或扩大本章工程实现。

| 连接点 | 本章交出的能力/需调整的叙述 | 首次使用前的后续门槛 |
|---|---|---|
| chapter 2/3 → chapter 4 | 完整平台清单是最终目标；本章承接已登记 layer，选独立最小测试 `MACHINE=tiger`；chapter 3 的 collection/BBFILE_PATTERN 使用当前 `tiger` 口径 | 保留 chapter 3 已定稿基线，不回写旧 `meta-tiger` collection 或另一个默认分支假设 |
| chapter 4 → chapter 5 | 本章为 fixture 提供最小 `tiger.conf`，不声称完整 BSP；旧 chapter 5 以 `-S`/章末 tag 结束上一章、首次创建 `tiger-aarch64.conf` 的开场需要衔接 | 后续完整 BSP 的 `MACHINE=tiger-aarch64` 与测试 `tiger` 显式区分，具体复用/演进经工程事实和设计核对；不能擅自把 A-005 的 MACHINE 改名 |
| chapter 6 的固件/首次存储读 | 本章只运行 tiger-minboot raw，未实现 BL1/BL2/BL31/TF-A IO；不把测试 BIOS 自动称为真实启动 ROM/量产链 | 首次 BL2/FIP 读取前若采用 SPI NAND，必须先实现/核对 SPI master→真实 CS→SPI NAND、reset/ID/字节读取、TF-A IO、几何/ECC/坏块与镜像边界（V-09R），不能拖到 chapter 9 才解决 |
| chapter 7/8/10 | 本章无 U-Boot/Linux/DTB/rootfs、SRAM/GIC/timer IRQ、PSCI/四核/guest reset | 首次使用前补齐各固件/模型/驱动接口、选定源与分段实测；全链按 V-08 验收，本章串口日志不能代替 |
| chapter 9/10 存储 | SPI NAND 不能当作 SPI NOR 或并行 NAND；本章不映射预留窗口 | 完整 program/erase/持久化/OOB/ECC/UBI/env/A-B/恢复按 V-09；候选 2 KiB/128 KiB/512 MiB 不当作已选芯片几何 |
| chapter 15 | 本章只用 devtool 最少取源码与清理命令；正式 workspace/finish 教程留本章之后 | watchdog 模型/驱动/时钟/reset 必须先有真实依据（V-10）；不能宣称锁定 QEMU 已有 SP805。正常版本与教学 bug 注入分别版本化、可恢复、实测 |
| 全书及附录 | 长期资产统一为上游源码+正式 meta-tiger patch；不新增四个长期 fork | T-017 依已登记 19 文件范围处理源码仓库图、命令/文件/章节引用和附录；继续等待 T-016 完成与验证/合并/清理门槛 |

PL022 的 SSI 连接或 cs 索引不自动等于真实片选电平，锁定源码里没有直接可用 SPI NAND 的依据不能靠 NOR 模型替代；具体型号、CS 接线、后端格式和驱动留给后续工程选型/实现。watchdog 的 SP805/CMSDK/SBSA 也不能无证据视为兼容。上述后续未实施项不单独阻塞 chapter 4 的已批准最小路线，但必须在首次使用前闭环。

## 9. 本轮交付与后续行动

writer 交付 A-006 v0.1 draft，并维护成果索引和 T-016 本人蓝图阶段实际记录。只完成教学/交接规划与本地文档自查，没有书稿改稿、产品源码、真实 patch、fixture 产物、BitBake 运行或新 tiger 运行结果；A-005 技术正文和原评审/复核意见保持原样。

本地提交后 writer 停止。project-manager 核对本蓝图版本、工程分工/证据/验收映射及当前仓库状态，再按 T-016 衔接 qemu-engineer。后续工程输入到齐后才继续正文修订，主线失败按 D-010 阻断定稿；本蓝图不请求绕过工程验证或提前开始下一轮。
