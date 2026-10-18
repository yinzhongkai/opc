# T-016：Yocto 正式集成与系统验证

- 执行人：book-yocto / yocto-engineer；2026-10-18（Asia/Shanghai）。
- 状态：completed（本人正式集成、系统验证与交接完成）。干净来源重构建、真实 runqemu 冷启动/两次同进程宿主 reset、16 项模型矩阵和部署错误后的恢复均通过；T-016 整体、正文审校与批准仍由后续流程处理。
- 授权与输入：T-016 正式集成安排，OPC 起始 `ab33063b73e8147c1d1847f4538695a796accf6d`；A-005 v0.2 approved / D-014（获批技术提交 `08bdf9c08d6d8cbd8836df3771ffd28231470059`）；A-006 v0.1 draft 的继续实施授权（`16467c0a8c667d3eabd59f026ceee5bc72a6584d`）；D-010/D-013/D-015；QEMU 工程交接及 PM 2026-10-18 核对。
- 原始证据：[目录与复核方法](t016-yocto-system-raw/README.md)、[291 份文本的归档清单](t016-yocto-system-raw/handoff-2/manifest.json)、[本地完整性回执](t016-yocto-system-raw/verification.json)。日志包内保存原始文本，不以本报告摘录代替。

## 1. 精确版本与工程交付

OPC 使用 `feature/book-yocto/t-016-chapter4-revision`。书稿 `D:/book-yocto` 的 feature/main 均为 `9c74596591728c0f2d614e357059034551a42dca`，未改正文。工程主机为 SSH `tiger` / `oops`，仓库 `/home/oops/workspace/meta-tiger`，remote `git@github.com:yinzhongkai/meta-tiger.git`，复用 `feature/t-016-chapter4-revision`。

| 对象 | 本阶段输入与终态 |
|---|---|
| meta-tiger | 输入 `b303ae6d614f067e11d4cbba57feacaf84ee7e8e`；最终工程/测试头 `4dd7dfda8f2af2d59bdcfe6585418cf43eaa73e8`，干净 |
| 实际构建 metadata | `cd6c348836346004e801cb13f32ad3fcb12c1dfd`；此后只修改部署测试和工程 README，配方、machine、模型及固件源未变 |
| meta-tiger master | 始终为 `0e3da48b34ef772532535a730a21878cc30ccf67`；未推送、未合并、未删除 feature |
| Poky | `/home/oops/workspace/poky`，scarthgap `77d1feb37e280733684ae8a9449fb031d5d7ff40`，干净且未改 |
| 持久 QEMU 源码 | `/home/oops/workspace/sources/qemu-system-native`，devtool `7ed31149ac01a9d6bb3a682b9d768936c4315ed3`，干净且未改 |
| 正式 QEMU | `qemu-system-native_8.2.7.bb`，包 SHA-256 `1f0604f296ab9acb4854c054764a1ba408643fc299bd54a6500cccfaaca65b55` |

本人成果按模块形成以下普通提交；没有新增 merge commit：

| 提交 | 内容与仓库相对路径 |
|---|---|
| `fa5e5b03ac8e88810c442b73ea7e10bdc221e495` | 正式 append：`recipes-devtools/qemu/qemu-system-native_8.2.7.bbappend`，保留 OE patch 列表并追加 tiger |
| `baff3bcb5b441e2be0ea4db89214589ff4520d26` | `conf/machine/tiger.conf`、`recipes-test/tiger-minboot/tiger-minboot_0.1.bb`、`files/build-fixture.py` |
| `a9fc4e3e9610c961d4f8b2425dd2406f367f290e` | `tests/integration/test-runqemu.py`，正式部署资产的冷启动/同进程 reset |
| `4178e8dff3682ac6b53c9b35fc7adcc71ce27bfd` | 部署诊断脚本及 `tests/integration/README.md` |
| `cd6c348836346004e801cb13f32ad3fcb12c1dfd` | tiger 的 native QEMU 仅保留本章 TCG/串口所需路线，去除 SDL/virgl/epoxy/KVM |
| `4dd7dfda8f2af2d59bdcfe6585418cf43eaa73e8` | 隔离缺 kernel 用例的兜底搜索；记录锁定 runqemu 不传播子进程失败的行为 |

原 QEMU patch、四份固件源/许可及 `tests/model/` 与输入 b303ae6 完全相同。产品代码及中间工程资产均在上述工程仓库/授权构建树，OPC 只保存治理和文本证据。WORKSPACE 正式 revision 未推进。

## 2. override 解除、源码保留与隔离重构建

解除覆盖前备份原 build 的 local.conf/bblayers.conf、workspace append 和 `.devtool_md5`，保存持久 source 的 HEAD/status/全部 refs/bundle hash。读取锁定 `scripts/lib/devtool/standard.py` 的实现后，在原构建环境执行 `devtool reset -n qemu-system-native`：不 clean、不传 `--remove-work/-r`。override 消失，原 source 目录外置，不被 reset 删除。

结束时再次核对：source HEAD/status/全部 refs 与开始时逐项一致；`archive/t016-minimal-model-20261017` 在位；全 refs 恢复包 `.git/opc-retention/20261017/qemu-system-native-all-refs.bundle` SHA-256 仍为 `c1561b554b272d0a6b847fab6fb5bbcd3707bcc5060d5353713f55279878fd6e`。旧 `/home/oops/workspace/build/workspace/sources/qemu-system-native` 兼容链接仍指向同一源码。原 build 的下载、无关 work 和用户资料保留。

新建 `/home/oops/workspace/build-t016-yocto`，使用全新 TMPDIR，bblayers 不含 workspace layer。共享原 `/home/oops/workspace/build/downloads`；普通依赖允许用原通用 sstate，但 qemu-system-native/qemu-helper-native/tiger-minboot 使用本任务初始为空的 `sstate-t016-critical`，禁用 sstate mirrors。首轮实际摘要 Wanted 476 / Local 2 / Mirrors 0 / Missed 474 / Current 0。

首轮于 21:25:07 开始，完成正式 do_patch 和 fixture 编译后等待 native 图形依赖 LLVM。23:19:45/23:20:12 对已核对 PID/cwd/cmdline 的本任务 BitBake 客户端两次 SIGINT，BitBake 终止剩余 LLVM 任务并返回 1。随后收敛 tiger 的 native PACKAGECONFIG，在同一新构建树继续；不是把第二轮描述为另一个空 TMPDIR。此前已执行的正式 patch/fixture 任务和后续新 configure/compile/helper/deploy 日志共同证明来源；未借旧 externalsrc work 或旧 QEMU/helper。

第二轮 23:21:49–23:31:00，9 分 11 秒，1021 个任务中 1007 个无须重跑，其余全部成功，build-2.rc=0。实际 PACKAGECONFIG 为 fdt/alsa/pie/slirp/png，QEMU_TARGETS=aarch64；图中已无 LLVM/Mesa/SDL/virgl/epoxy。关键任务均有本构建树的真实 run/log：QEMU patch/configure/compile/install/populate_sysroot，helper install/populate_sysroot/addto_recipe_sysroot，fixture compile/generate_qemuboot/deploy。

构建有两项 uninative 告警：其安装/重定位命令返回 127，BitBake 禁用 uninative 后继续并完成构建。这轮不宣称 uninative 正常或跨宿主缓存可复用；没有为本章改动 Poky/宿主 SDK。完整告警与 native 编译命令保留。没有以宿主 PATH 中的 QEMU 代替 helper。

## 3. 正式 patch、任务图与部署配置

`bitbake-layers show-appends` 确认 append 匹配 system-native 8.2.7；实际 `bitbake -e` 无 EXTERNALSRC/EXTERNALSRC_BUILD。SRC_URI 与新 source 的 `.pc/applied-patches` 均为原 31 项后追加 `0001-arm-add-minimal-tiger-machine.patch`；前 31 项序列 SHA-256 仍为 `3b7c0560451d78d8eae6512a4c21d77129eb3b0e1106512949709e8ef0eb8430`，tiger patch SHA-256 `5437db1b7a95d656840d3ef0e590b84c889a891aed378b0813e1d4b989b94797`。正式解包应用后的 tiger.c/Kconfig/meson.build 与持久 source 对应文件逐字节相同；不重新导出已有 OE patches。

fixture 为独立 target recipe（MIT），不继承 image/qemuboot 类，不构建 kernel/rootfs。独立 tiger machine 使用 Cortex-A53 AArch64 tuning，不整份继承 qemuarm64/virt。实际任务图的三条边：

    tiger-minboot.do_generate_qemuboot -> tiger-minboot.do_compile
    tiger-minboot.do_generate_qemuboot -> qemu-helper-native.do_addto_recipe_sysroot
    tiger-minboot.do_deploy -> tiger-minboot.do_generate_qemuboot

`do_generate_qemuboot` 的实际 sigdata 经 `bitbake-dumpsig` 展开，包含全部生成所用 QB 字段、SERIAL_CONSOLES、MACHINE、TUNE_ARCH、DEPLOY_DIR_IMAGE、BASE_WORKDIR、BUILD_SYS、RECIPE_SYSROOT 等依赖。完整依赖和值保存在 `generate-signature-2.log`；不是仅检查配方中写有 vardeps。

发布目录为 `/home/oops/workspace/build-t016-yocto/tmp/deploy/images/tiger`，实际资产基名 `tiger-minboot-0.1-tiger`：.bin/.elf/.map/.qemuboot.conf，及 -manifest.json/-commands.json/-readelf.txt/-nm.txt/-disassembly.txt/-native-identity.json。由 compile→generate→deploy.bbclass 正式发布，没有手写公共 deploy 文件。

| 配置项 | 实际值/核对 |
|---|---|
| MACHINE / TUNE_ARCH | tiger / aarch64 |
| QB_DEFAULT_KERNEL / QB_DEFAULT_FSTYPE / QB_ROOTFS | 全部为字面 `none` |
| QB_DEFAULT_BIOS / DEPLOY_DIR_IMAGE | `tiger-minboot-0.1-tiger.bin` / `.` |
| QB_MACHINE / CPU / SMP / MEM | `-machine tiger -accel tcg` / `-cpu cortex-a53` / `-smp 1` / `-m 1024` |
| native bindir | `../../../work/x86_64-linux/qemu-helper-native/1.0/recipe-sysroot-native/usr/bin`，相对最终 conf 目录计算并验证存在 |
| target sysroot | `../../../work/tiger-poky-linux/tiger-minboot/0.1/recipe-sysroot` |
| 额外设备参数 | RNG/graphics/rootfs/network/tap/serial 等相应字段清空；最终 argv 无 kernel/rootfs/virtio/PCI/USB/网络设备或 KVM |

配置/身份记录绑定本构建树；搬移或换树后须重新生成并核对，不宣称已验证跨树部署缓存或可搬移性。锁定 runqemu 即使 nonetwork 仍先检查 TUN；本轮 `/dev/net/tun` 存在且可写。

## 4. 工具链和资产身份

fixture 由本轮构建的 AArch64 GCC 13.4.0 编译。真实 GCC 路径为 `/home/oops/workspace/build-t016-yocto/tmp/work/tiger-poky-linux/tiger-minboot/0.1/recipe-sysroot-native/usr/bin/aarch64-poky-linux/aarch64-poky-linux-gcc`，SHA-256 `3234173045d548835cce7550f86434620cdb9dfe6ce2d08f4ce9d97186364d0c`；objcopy/readelf/gcc-nm/objdump 路径及 hash 同样保存于 asset-audit.json。manifest 保存真实 CC 参数（含 tuning/security/sysroot）、版本、声明 flags、四份源 hash 和每条命令。

声明 freestanding flags 为 `-O2 -Wall -Wextra -Werror -march=armv8-a -mgeneral-regs-only -mstrict-align -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -fno-pic -fno-unwind-tables -fno-asynchronous-unwind-tables -nostdlib -nostartfiles -static -no-pie`。本轮 raw 为 760 字节（模型作者旧独立产物为 752 字节），不要求不同编译命令生成同一 binary。source-build-id 由四份源和声明 flags 得出；实际产物身份使用下列 hash，不把 source-build-id 当作编译器或 binary 的唯一指纹。

| 对象 | 字节数 / SHA-256 |
|---|---|
| raw | 760 / `092517d50dfac35b0697f3ec4819e3a3cac351d433a6e537ca4ffea52e7210cb` |
| ELF | 132448 / `560a718b0c4275fa74bc46f68aa1395368e3961b2d9a2a9665b927f0d67ae901` |
| map | 3939 / `fac70c4e94d999a427f093e42535562352320a4f02dcba8876ea95af21be885b` |
| manifest | `1a478c87019ffc1eb89ba1473864bc6fd61c50094f6672fb04d0f3b8669fda07` |
| qemuboot.conf | `b1218db0a43435e2893340cb5f7117f90d20b51435e2964fe3b232d1996508c0` |
| 实际 helper QEMU | `1f05c31da60eb777e59b07f5075a143a07281b0d5b5473fac2b763a93bfdb002` |
| 锁定 runqemu 脚本 | `d644b4763b968414649f948151d51ff84b1663218ffd4abae3b58d19e4ba7861` |

ELF 无 INTERP/未定义符号，入口 0；data VMA `[0x40000000,0x40000008)`，LMA 0x2f0；BSS `[0x40000008,0x40000030)`，栈 `[0x40010000,0x40020000)`。link ASSERT、readelf/map/nm/disassembly/raw 大小和源文件对账均通过。manifest.source_revision=b303ae6 指四份未变固件源，不冒充完整 metadata 最终头。

helper QEMU 绝对路径为 `/home/oops/workspace/build-t016-yocto/tmp/work/x86_64-linux/qemu-helper-native/1.0/recipe-sysroot-native/usr/bin/qemu-system-aarch64`。运行时 `/proc/<pid>/exe` 的路径和 hash 与生成 identity 一致；build 目录未安装 binary 的 hash 为 `37373f8cd717cf873976625e27571b8b06867b37ba987f2c7dd7cc551f8bffa5`，分别记录安装前/后的身份，不错误要求二者相同。

## 5. 真实 runqemu、复位和模型矩阵

测试实际调用锁定 `poky/scripts/runqemu <正式版本化 conf> nographic nonetwork`，并加 QMP/GDB/指令/PL011 trace 调试参数。冷启动 runqemu-2 的 QEMU PID 997465；恢复 runqemu-recovery-3 的 PID 997810。每轮各自同一进程内完成冷启动和两次 system_reset，最后 QMP quit、launcher 返回 0，结束核对两 PID 均已退出。

最终 argv 的生产参数为 `-nic none -machine tiger -accel tcg -cpu cortex-a53 -smp 1 -m 1024 -serial mon:stdio -serial null -nographic -bios <正式 raw>`；实际完整 argv 含调试端口和路径，分别保存在 state.json/launcher-argv.json。没有 -device/-netdev/-drive/-kernel/-initrd/-dtb/-append/-enable-kvm；第二个 null backend 不代表第二个 UART。仅以该锁定启动器的实际接线为准，不再额外追加 stdio 参数。

两轮均验证：

- `info mtree` 为 ROM/RAM/PL011 三窗口，无新增 GIC/RTC/SPI/SRAM；CPU 仅 1 个，rvbar=0、EL2/EL3=true、cntfrq=24000000、start-powered-off=false、PSCI conduit=0。
- 首次执行及每次 reset 后、cont 前均为 PC=0 / secure EL3h；UART FR=0x90、IBRD/FBRD/LCR_H/IMSC/RIS/MIS=0、CR=0x300。暂停→system_reset→RESET event→读取状态→恢复的完整 QMP 交互留存。
- GDB 在 `firmware_main=0xc0` 第一条指令处观察 SP=0x40020000、data=0x1122334455667788、BSS=0；实际运行后的 EL=12（EL3）、MPIDR CPU0、MMU/cache 位和栈对齐范围符合固件契约。
- 固件将 data/BSS 写成 DADA/B0 哨兵；宿主 reset 后 RAM 哨兵仍在，启动代码再次恢复 data/BSS 初值，再运行到下一条 guest 标记。不是假设 RAM 被系统 reset 全清零。
- PL011 分频/控制读回 13/1/0x60/0x101、IMSC=0，指令与 PL011 MMIO trace 关联 guest 输出，24 MHz clock trace 达标；不据此宣称线速或 IRQ 已测。
- 每次运行后整段 1 MiB ROM 与 raw 补零逐字节一致；三次 ROM hash 都为 `32908382bdd7ee2b53b511c7ea5baa1d6c4964c0bf93520a741f96b6f29035ab`，guest 写入没有改变 ROM。

每轮实际输出三次以下完整标记（这里仅摘一条，原文未改）：

    TIGER-CH4 FW=sha256:7fbb005bd53a3f3fa2ed0a933181e50db7a6bff97cab78ce9429557ccb9818d7 CPU0 EL3 INIT=OK UART0 CFG=OK ROM=RO

`tests/model/test-tiger.py` 在正式 helper 和正式发布 raw/ELF/manifest 的只读 alias view 上复验，16/16 pass、脚本返回 0。包括冷启动/两次暖 reset、恰好 1 MiB 的实际启动，以及 14 项拒绝：缺 BIOS 选项、路径不存在、空、超长、不可读、目录加载失败、错误 CPU/SMP/RAM/内存热插拔/加速器、kernel/initrd/DTB。拒绝例分别要求对应诊断、QEMU 退出 1、指令 trace 为零及无 guest 输出。使用原模型工具，未为适配结果修改 QEMU 模型或旧测试。

## 6. 失败、诊断与恢复

| 轮次 | 实际结果与处理 |
|---|---|
| parse-1 | 在 source oe-init-build-env 前启用 nounset，BBSERVER 未定义，返回 1；未进入配方构建。改为非 nounset 后 parse-2 返回 0 |
| 只读控制探针 | 最初假设 devtool/reset.py 路径不存在，改读 standard.py；另有本机 PowerShell heredoc 语法错误及停止后查不到旧 PID 的探针，均未改工程，原输出在补充日志 |
| build-1 / system-1 | 去除无关图形依赖前的受控中断，分别返回 1；该系统脚本未进入资产/CPU 验证。LLVM 完整任务日志和停止回执保留 |
| build-2 | 全部所需任务成功，返回 0；uninative 两项告警如 §2，未隐藏 |
| system-2 | 资产检查、runqemu 冷启动/复位和 16 项模型矩阵成功；部署诊断第二例失败，整轮脚本返回 1，恢复步骤尚未执行 |
| system-3 | 修复部署测试后 3/3 预期诊断通过，实际恢复主线再冷启动/两次 reset，最终返回 0 |

deployment-2 的缺 BIOS 例符合预期返回 1。缺 kernel 例因 KERNEL_IMAGETYPE 为空，锁定 runqemu 的兜底 glob 选中了复制的 qemuboot.conf 并传入 -kernel；QEMU 正确报 unsupported，但启动器只记录子进程错误且返回 0。该次 `pass=false` 和 traceback 原样保留，不改记通过。

4dd7dfd 仅修正测试隔离：私有配置加不可能匹配的 KERNEL_IMAGETYPE，缺 BIOS/缺 kernel 两例各以明确 not found、launcher=1、无 guest 输出通过；另加显式 unexpected-kernel，确认 QEMU unsupported 诊断、零字节指令 trace、无标记，同时把已知 launcher=0 记录为局限而非启动成功。这里没有修改或声称修复上游 runqemu；直接 QEMU 的 kernel 拒绝/退出 1 另由模型矩阵证明。恢复例重新使用未改的正式 conf/BIOS/helper，三条标记和复位证据再次通过。

## 7. A-006 E-01～E-10 与正文回填

| 证据 | 本轮实际覆盖 | 可复核位置（日志包内原文件） |
|---|---|---|
| E-01 | 输入版本、实际 recipe/provider/PV、包 hash、SRC_URI、无 override | controller；qemu-env-1/2；asset-audit；stage-final-check |
| E-02 | 原开发来源承接 QEMU 报告；本轮 devtool reset -n、前后 source/refs/bundle 不变，外部 override 消失 | override-before/after、devtool-reset-source、source-before、override-and-dependency-check |
| E-03 | 正式编译/注册、三窗口、ROM 只读、CPU/参数拒绝、同进程多次 reset | critical task logs；model-2；runqemu-2/recovery-3 |
| E-04 | 正式 target 编译、工具/flags/source/ELF/map/raw 对账、入口/初始化/MMIO | deployed-text-identities；asset-audit；GDB/指令/PL011 trace |
| E-05 | 原 31 OE patches 后追加真实 tiger patch；新应用源与交接源逐字节比对 | formal-patch-check；show-appends；qemu-env；do_patch |
| E-06 | 正式 recipe、三依赖边、实际 sigdata/vardeps、版本化 deploy/路径/none 参数 | task-depends-2；generate-signature-2；generate/deploy log；发布 conf |
| E-07 | 新 TMPDIR/关键空缓存起步、reset 后仅正式层；完整关键任务链和工具/部署/执行身份 | build-1/2；关键 task logs；asset-audit；stage-final-check |
| E-08 | 实际 runqemu、新 helper/BIOS/conf、完整 argv、CPU0/EL3/guest 标记及受控退出 | runqemu-2；state、qmp、gdb、instructions、runqemu 日志 |
| E-09 | 每轮同进程两次 reset，恢复前 PC/EL/UART、RAM 哨兵保留再初始化、ROM 未变 | runqemu-2/recovery-3；model-2 |
| E-10 | 16 项模型矩阵、缺 BIOS/kernel 与 unexpected-kernel 部署诊断、实际恢复复验 | model-2；deployment-2（失败）；deployment-3；runqemu-recovery-3 |

可供 writer 回填的真实主线命令如下；实际完整脚本见 configuration-and-control 包，P-02/P-03 的原 devtool 修改/导出过程仍引用 [QEMU 阶段报告](t016-qemu-model-validation.md)，本轮不重复声称执行 modify：

    source /home/oops/workspace/poky/oe-init-build-env /home/oops/workspace/build
    devtool status
    devtool reset -n qemu-system-native
    source /home/oops/workspace/poky/oe-init-build-env /home/oops/workspace/build-t016-yocto
    bitbake-layers show-appends
    bitbake -e qemu-system-native
    bitbake -e tiger-minboot
    bitbake -g tiger-minboot
    bitbake tiger-minboot
    runqemu /home/oops/workspace/build-t016-yocto/tmp/deploy/images/tiger/tiger-minboot-0.1-tiger.qemuboot.conf nographic nonetwork

验收实际以 `tests/integration/test-runqemu.py` 在上述命令上附加 QMP/GDB/trace 参数执行；完整控制和命令 argv 均留原文。不要将最后一行无调试的等价交互入口写成本轮另一次独立实测。fixture-view 只是指向正式发布文件的测试别名，没有编译/拷贝新固件。

## 8. 交接边界与终态

本轮 E-01～E-10 的 chapter 4 最小范围已覆盖。未实施/未验收：SPI NAND、TF-A/BL1/BL2/BL31/U-Boot/Linux 全链、DTB/rootfs、SMP/GIC/timer IRQ、guest 请求 reset、watchdog、物理串口线速、其他宿主/跨构建树部署缓存。它们继续按 A-005/A-006 的首次使用门槛处理。本次为本人正式系统验证，不替代独立审校、用户批准或整章定稿。

本地框架结构、链接及差异空白检查通过；范围核对确认 TASKS 本人阶段外、索引本人新增区段外、A-005/A-006/WORKSPACE/STATUS/PROJECT/DECISIONS 与起始提交一致，书稿 HEAD/main 和工作树未变。本地 Git 范围脚本前两次分别因 NUL exclude 路径及沙盒用户/仓库 ownership 不同而退出 128；改用现有仓库 exclude 和仅对两个已授权工作区的单命令 safe.directory 后通过，未修改全局 Git 配置。原始检查及回执列在证据目录。

阶段报告、9 包 291 份原始文本、逐份完整性回执已落 OPC；任务与索引只更新本人阶段。结束时 meta-tiger/Poky/持久 source 均干净，模型/固件源/原 refs/bundle 与输入一致，测试进程受控退出。书稿、A-005/A-006、旧评审及 WORKSPACE 保持原样。工程未推送/合并，任务分支保留；完成治理校验和本地提交后本人停止，交 project-manager 核对，再按既定次序衔接 writer。
