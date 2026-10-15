# T-016：最小 tiger 模型、固件与模型验证交接

- 执行人：`book-yocto / qemu-engineer`；2026-10-14 接收，2026-10-15 完成工程提交/模型验证，2026-10-16 治理交付（Asia/Shanghai）。
- 结论：**本阶段模型作者自测通过，最终矩阵 16/16**。已交付真实增量 patch、完整最小固件源及模型工具；冷启动和同进程两次宿主 reset 有 CPU、RAM、PL011、指令与串口证据。正式 metadata/fixture/helper、清理 workspace 后的干净 BitBake 重构建及 runqemu 验收由下一阶段完成。
- 授权：T-016“QEMU 实施阶段”，用户 2026-10-14“我大致看了下A-006可以继续执行”及既有直接协调/实施授权。输入为 A-005 v0.2 approved / D-014、A-006 v0.1（`16467c0a8c667d3eabd59f026ceee5bc72a6584d`，blob `39b27c85411e93af0bd1fe02cc48b45cb6b7ce63`）、D-013 及原工程来源证据。角色/scope/三项有效技能已刷新；本阶段没有启动其他成员。
- 原始证据：[目录与判读说明](t016-qemu-model-raw/README.md)、[原始文件与字节校验清单](t016-qemu-model-raw/manifest.json)、[控制端回读](t016-qemu-model-raw/controller.log)。失败日志和旧版脚本当时的错误 pass 标签保留，按本报告解释，不以其代替最终结果。

## 1. 仓库、来源与工作区

| 对象 | 实际版本及状态 |
|---|---|
| OPC | 起始 `0eede3b58c6f5a972b35c59eb5d329d5e55d5179`，`feature/book-yocto/t-016-chapter4-revision`，开始干净；本阶段只更新本人任务、交接报告/原始证据及索引 |
| 书稿 | `D:/book-yocto`，`feature/t-016-chapter4-revision` / main，`9c74596591728c0f2d614e357059034551a42dca`，未修改 |
| meta-tiger | `ssh tiger`，用户 oops，`/home/oops/workspace/meta-tiger`；开始 master 与 github/master 为干净的 `0e3da48b34ef772532535a730a21878cc30ccf67`；从此建立 `feature/t-016-chapter4-revision` |
| Poky | `/home/oops/workspace/poky`，干净 `scarthgap@77d1feb37e280733684ae8a9449fb031d5d7ff40` |
| 实际 recipe | `qemu-system-native_8.2.7.bb`，PN=qemu-system-native、BPN=qemu、PV=8.2.7，class-native；完整前后展开见原始 env 日志 |
| 源码与补丁 | qemu-8.2.7.tar.xz SHA-256 `1f0604f296ab9acb4854c054764a1ba408643fc299bd54a6500cccfaaca65b55`；原有 29 common + 2 native patches，序列文件 SHA-256 `3b7c0560451d78d8eae6512a4c21d77129eb3b0e1106512949709e8ef0eb8430`，未复制/替换这 31 项 |
| 临时 source | `/home/oops/workspace/build/workspace/sources/qemu-system-native`；devtool 直接支持该 native recipe，默认 devtool 分支和 class-native override 在提取时同指 `3a261f929a6154bdc315aabf4cb6c79c57f73652` |
| 开发基点 | 上述 patched source 比 devtool-base `041c1c65762b96bce77da424335150d654bf144a` 多 31 项补丁提交；这是本次提取的临时 Git 身份，不假定别次提取的 import SHA 相同 |
| 最终模型 source | 临时 devtool commit `7ed31149ac01a9d6bb3a682b9d768936c4315ed3`，只有一个新增模型提交；工作树干净 |
| 实际 B | `/home/oops/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7`，config-host.mak 的 SRC_PATH 指向上述 workspace source；不能沿用修改前的兄弟 build/ 目录 |

资源核对时主机 8 个逻辑 CPU、30 GiB RAM、工作分区约 647 GiB 可用。使用既有下载/工具链及授权的 devtool/BitBake 临时目录，没有建立独立 QEMU fork。devtool 自动登记 workspace layer/externalsrc；MACHINE 仍为 **qemuarm64**。local.conf 未作人工变更，workspace override 留给后续正式集成/清理流程；本阶段不把它说成干净重构建。

## 2. 工程交付版本

仓库为 `git@github.com:yinzhongkai/meta-tiger.git`，主机/checkout 见上表。以下均为 **feature 阶段本地提交，尚未合入 master 或推送**；WORKSPACE 的最终 revision 未推进。

| 模块 | 提交 | 仓库内路径 |
|---|---|---|
| QEMU 增量 | `0e73cd7faacff219fa5de0a7365743ce31402d73` | `recipes-devtools/qemu/files/0001-arm-add-minimal-tiger-machine.patch` |
| 固件与模型工具（交接头） | `b303ae6d614f067e11d4cbba57feacaf84ee7e8e` | `recipes-test/tiger-minboot/files/{start.S,main.c,tiger-minboot.ld,COPYING.MIT}`；`tests/model/{build-minboot.py,test-tiger.py,README.md}` |

Patch SHA-256：`5437db1b7a95d656840d3ef0e590b84c889a891aed378b0813e1d4b989b94797`。它只新增/修改 `hw/arm/tiger.c`、`hw/arm/Kconfig`、`hw/arm/meson.build`，应用在原有 31 项之后。对 patched base 的两份原文件及空的新文件位置执行 git apply --check、实际 apply，并逐字节比对三文件与实测 source，均通过。checkpatch（--no-tree --no-signoff）为 **0 errors、0 warnings**。模型 GPL-2.0-or-later，固件/工具 MIT，patch 包含 Upstream-Status。

导出的标准 format-patch 邮件上下文/尾部空行保留原样，外层 Git 空白检查对该邮件文件使用 `core.whitespace=-blank-at-eol,-blank-at-eof`；内部代码已由 checkpatch/应用检查核对。固件与脚本使用默认 git diff --cached --check，通过。两个工程模块为普通线性提交，没有本轮新增 merge commit；未合并/删除分支。

实现与批准契约一致：TCG 单核 A53；只映射 1 MiB ROM、1 GiB RAM 和 PL011；IRQ 未接；单 clk 24 MHz 在 realize 前连接，CPU cntfrq 独立显式配置；strict raw BIOS 读取成功且长度为 1～1048576 字节才建立 CPU。CPU 保留 EL2/EL3、rvbar=0、PSCI disabled；machine 持有 boot_info，纯 firmware arm_load_kernel 初始化时只调用一次。ROM 初始化采用 NULL migration owner，避免将 MachineState 强转为 DeviceState。没有新增 GIC/SRAM/SPI/NAND/watchdog 或额外 boot mode。

## 3. 实际编译与执行身份

实际执行：

```sh
source /home/oops/workspace/poky/oe-init-build-env /home/oops/workspace/build
bitbake -e qemu-system-native
devtool status
devtool modify qemu-system-native
devtool build qemu-system-native
```

首轮开发构建返回 0，1224 任务全部成功，1215 项不需重跑；本轮真实执行 configure/compile/install/populate_sysroot。ROM owner 修复后再次构建返回 0，1221 项不需重跑，compile/install/populate_sysroot 实际重跑。完整任务输出见 [构建原始日志](t016-qemu-model-raw/bitbake-tasks.log)。已有依赖缓存命中按原样记录，不称为清缓存构建。

固件由 `tests/model/build-minboot.py` 使用已有 Yocto AArch64 GCC **13.4.0** 编译，编译器实际路径为：

`/home/oops/workspace/build/tmp/work/cortexa57-poky-linux/libedit/20230828-3.1/recipe-sysroot-native/usr/bin/aarch64-poky-linux/aarch64-poky-linux-gcc`

采用 freestanding、general-regs-only、无 libc/动态链接/PIE/stack protector 等明确 flags；完整 argv、工具身份与 ELF 检查见 [固件原始输出](t016-qemu-model-raw/firmware-inspection.log)。这是本阶段独立模型 fixture 编译，尚不是正式 target 配方的 deploy 结果。

| 对象 | 实际路径/内容 | SHA-256 |
|---|---|---|
| 新 native QEMU | B 下 `qemu-system-aarch64`，报告版本 8.2.7 | `4bd5db20de2c77e4df878019f097200ff170f9b180062f2c869e9abe7e8af528` |
| raw | `/home/oops/workspace/build/t016-qemu/fixture-1/tiger-minboot.bin`，752 字节 | `2b67c2bfb9d2fcbf3fd1880a1c1bf3ff09ece73b8bcbf5235116477fd2714376` |
| ELF | 同目录 tiger-minboot.elf，132448 字节 | `b80e02248b16f19121e123beae2aee26f74454fdaaf54bac9cb45e2de76edf57` |
| manifest | 同目录 manifest.json，关联源文件、flags、map/symbol/disassembly | `e8e70e40ba10f828d3be575725f42c3b9279c16c44392f9dec39cc69ee73884b` |
| 最终测试脚本 | meta-tiger 的 tests/model/test-tiger.py；与通过测试时内容相同 | `5f3cb82d80e99e3ed4b7efca704f16dbfb95ef26ac8fcd9aa13db6b29b5f1b73` |

ELF _start/entry=0、firmware_main=0xc0；data LMA=0x2e8，RAM data 为 [0x40000000,0x40000008)，BSS 为 [0x40000008,0x40000030)，栈为 [0x40010000,0x40020000)。链接 ASSERT 和实际 ELF/symbol/raw 范围核对通过；raw 没有 RAM VMA 造成的巨大空洞，没有未解析符号或 INTERP 段。

Guest 实际输出（最终 run-3；同一进程共三次）：

```text
TIGER-CH4 FW=sha256:7fbb005bd53a3f3fa2ed0a933181e50db7a6bff97cab78ce9429557ccb9818d7 CPU0 EL3 INIT=OK UART0 CFG=OK ROM=RO
```

Build ID 来源于命名源文件内容与 flags，未递归嵌入 raw hash。工程提交后再次核对其源文件及脚本 hash 与已通过的测试输入一致。

## 4. 最终模型矩阵与观察

最终实际调用：

```sh
python3 /home/oops/workspace/meta-tiger/tests/model/test-tiger.py --qemu /home/oops/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7/qemu-system-aarch64 --fixture /home/oops/workspace/build/t016-qemu/fixture-1 --out /home/oops/workspace/build/t016-qemu/model-run-3
```

该调用返回 **0，16/16 pass**；复验应按工具要求使用新的输出目录。全量 argv/QMP/GDB/trace/状态/退出码见 [最终运行原始日志](t016-qemu-model-raw/model-run-3.log)。

| 用例 | 实际结果 |
|---|---|
| cold-and-two-warm-resets | 同一 QEMU PID **80887**，cold + 两次宿主 reset，三条 guest 标记；最后 QMP quit，进程返回 0 |
| exact-1MiB | 有效 fixture 补齐至恰好 1048576 字节，长度接受且该内容实际启动，QMP quit 返回 0；不推断任意合法长度内容都能启动 |
| missing-bios-option / nonexistent-bios | 退出 1，分别为要求非空 BIOS、找不到 BIOS 的对应诊断 |
| empty-bios / too-large-bios | 退出 1，严格长度/可读 raw 诊断；超长为 1048577 字节 |
| unreadable-bios / load-failure-directory | 退出 1，权限不可读/目录加载失败对应诊断；临时权限随后恢复 |
| wrong-cpu / wrong-smp | cortex-a57、两核在 CPU 执行前由框架/板级约束拒绝，退出 1 |
| wrong-ram / ram-hotplug | 512 MiB、1024 MiB+maxmem=2G/slots=1 拒绝，退出 1 |
| wrong-accelerator | qtest 进入非 TCG 拒绝路径，退出 1；未把其他宿主加速器逐一运行当作已覆盖 |
| kernel-input / initrd-input / dtb-input | 退出 1；kernel/DTB 为板级拒绝，单独 initrd 由 QEMU “仅允许与 kernel 同用”规则提前拒绝 |

全部 14 项预期拒绝都匹配具体诊断、退出码 1，且无 guest 指令或串口输出；信号崩溃不能通过。

2026-10-16 补充核对 CPU 属性 aarch64=off：锁定 TCG 框架在执行前明确拒绝，退出 1（[最终核对日志](t016-qemu-model-raw/final-state.log)）。此项在上述 16 例矩阵之外，未修改模型/固件；不将其他 CPU 属性或 KVM 实测泛化为已覆盖。

冷启动及每次 RESET 事件后、cont 前，PC=0，PSTATE 显示 EL3h；QOM 的 rvbar=0、has_el2/has_el3=true、cntfrq=24000000、start-powered-off=false、psci-conduit=0，CPU 数始终为 1。UART reset 读回 FR=0x90、IBRD/FBRD/LCRH/IMSC/RIS/MIS=0、CR=0x300；未读 DR，避免 FIFO 副作用。设备/窗口树只含本章 ROM/RAM/PL011。

每次 GDB 在 **firmware_main 第一条指令前** 断住，观察 PC=0xc0、SP=0x40020000、data=0x1122334455667788、BSS=0；证明启动汇编已经实际复制/清零。C 代码观察到 CurrentEL=12、CPU0、MMU/cache 未开，函数栈 SP=0x4001fff0。输出前 data/BSS 改为 0xdadadadadadadada / 0xb0b0b0b0b0b0b0b0；reset 后、startup 再执行前两者仍保留，之后又在 main 入口恢复初值。由此区分模型 reset 与 RAM 全清零，也排除仅再打印一次的替代判据。

Guest 使用真正的 store 指令尝试改 ROM，读回及整段 1 MiB ROM dump 均保持原内容；warm reset 后亦一致。UART 初始化寄存器分别读回 13/1、LCRH=0x60、CR=0x101、IMSC=0；实际 PL011 trace 记录 clk=24000000hz 及 guest 写入。trace 的 115244 是该模型的整数计算值，不是串行线速测量。本阶段不验收真实波特率时序、IRQ 或性能。

## 5. 失败、修复与重跑

1. **旧 B 路径误选**：首次沿用 devtool 修改前的兄弟 build/ 目录，二进制 SHA-256 为 `53f28628d0a2ddcc24b36d55cdfe5bb89fece1cb4fa47da1d13ade249cb6b5e5`，machine help 无 tiger，预检返回 1，尚未调用模型矩阵。根据新 config-host.mak 的 SRC_PATH 和后续 bitbake -e 纠正 B；该旧文件未被冒充实测产物。
2. **run-1 新模型 owner 错误**：当时新二进制为 `2462e8c386ad0c8068655f973148727344cd5307256701009b5d1a7bce41e23e`。新增 tiger 代码把 MachineState 作为 memory_region_init_rom 的 owner，触发 DEVICE 类型检查并 abort。锁定 system/memory.c:3603–3610 明确要求 NULL 或 DeviceState；改为 NULL owner 后重构建。这是本次新增代码错误，不是上游 QEMU bug。
3. **run-1 负向误判已撤销**：旧测试只看非零返回，原始 results 中的 14 个 pass 含 empty/too-large/directory 三个 -6 崩溃，不能算预期拒绝通过。收紧为退出 1 + 对应诊断 + 无执行证据，保留原脚本快照及全部旧输出，不修改旧日志的标签。
4. **run-2 trace 采集位置错误**：修复模型后，cold/两次 warm reset、GDB/哨兵/UART/ROM 和受控退出实际完成，但测试收尾从 stderr 找 trace，整体返回 1。实际 -D 将 PL011 log backend 和指令 trace 都写到 instructions.log。按真实落点修正，并补核 cold/warm 的 CPU 属性及每次 UART 配置的 24 MHz trace；run-3 使用相同模型/固件二进制，最终 16/16。
5. **patch 邮件格式检查**：第一次外层 Git 检查把标准邮件尾空行列为格式问题，未提交；使用专用邮件空白选项保留原始 format-patch 字节，内部 patch 风格/应用内容检查独立通过。source 最终修复与导出为临时提交 7ed31149a，meta-tiger 两模块提交见 §2。

对应失败与重跑分别完整保存于 [run-1](t016-qemu-model-raw/model-run-1.log)、[run-2](t016-qemu-model-raw/model-run-2.log)、[run-3](t016-qemu-model-raw/model-run-3.log) 及控制端日志。失败源码/脚本快照与二进制产物位于授权临时目录；正式源码与 patch 由阶段工程提交定位。

## 6. 验收映射、未覆盖与下一位

| A-006 | 本阶段覆盖 | 后续责任 |
|---|---|---|
| E-02 | native devtool 提取、31 项基点/override、真实修改/导出通过 | 脱离 externalsrc 后仍可重建的部分由 Yocto E-07 完成 |
| E-03 | 模型编译/注册、三窗口/ROM 只读、参数拒绝、clock/reset 生命周期与重复 reset 检查通过 | 正式二进制复验 |
| E-04 | 完整固件源/许可、独立编译的 ELF/map/raw、CPU/EL/初始化/UART 模型证据通过 | 同版源的正式 target 配方、部署/manifest 链 |
| E-09 | 同进程两次宿主 reset、RESET 事件、恢复前后状态及重初始化通过 | helper/native 正式重构建后的系统复验 |
| E-10 | 本成员 ROM/BIOS/参数负向与受控退出范围通过 | 实际部署/路径/参数错误的系统复验；未编造教学故障注入 |

本阶段是模型作者自测，不替代 reviewer 独立审校、Yocto 系统验收或用户定稿。未执行正式 bbappend/SRC_URI 集成、MACHINE=tiger/fixture/helper 配方、qemuboot/deploy、devtool reset、排除旧 work/sstate/helper 的干净 BitBake 重构建、runqemu 或正文回填。也未实现 SPI NAND、TF-A/U-Boot/Linux 全链、GIC/IRQ/SMP/guest reset/watchdog，后续首次使用门槛继续有效。

工程 workspace 和分支保留且已提交；Yocto 应先核对 meta-tiger 的交接提交/输入 hash，再集成真实 patch 与 metadata，保存展开/任务图，按记录计划清理 override 并从干净状态复验。具体命令及模型判据在工程 `tests/model/README.md`。本成员完成治理校验/本地提交后停止，由 project-manager 核对后串行衔接；不代写 Yocto 接收或结论，不合并/删除分支，不启动 T-017。

## 7. 治理收口（2026-10-16）

固定 A-005/A-006、其他任务/原评审/作者回复与索引旧条目保持不变；本轮只写本人实施阶段、本人交接索引及报告/原始日志。九个日志包的字节数与 SHA-256、263 个来源记录及最终 16 项结果均已核对。框架结构/本地链接与差异检查通过，具体命令结果保存在原始证据的 local-governance.log；暂存后还核对 Git blob 与原始日志字节一致。最终远端 meta-tiger/Poky/devtool source 均为报告中的精确提交且干净，master 未推进，无遗留本阶段 QEMU 进程。书稿未变。OPC 本地提交后停止，最终提交号与洁净状态由本会话交付回复给出。
