# T-016：A-005 v0.1 第 1 轮 Yocto 设计核对证据

- 执行人：`book-yocto / yocto-engineer`；日期：2026-10-11（Asia/Shanghai）。
- 结论：**revise**。现有锁定源码支持继续采用最小测试固件候选 A；本轮只证明静态输入路线有依据，尚未证明固件部署或 tiger 启动成功。具体问题及作者回复入口在 [T-016 本人意见](../../TASKS.md#yocto-engineer-的意见)。
- 固定受评稿：[A-005 v0.1](../A-005-tiger-platform-design.md)，OPC commit `69ce591cd9e4ee310b80cff4663c20c776364cfd`，文件 Git blob `d7d474e4de48ad8e05ffd4d9893594214843f9be`；本轮不修改该稿。
- 授权与边界：T-016 第 1 轮专业核对、用户 U-A005-01/U-A005-02，以及 project-manager 的串行协调安排；现有 H-001 用户授权的 `ssh tiger` 环境用于只读核对。写入限本人意见及治理证据；不启动模型、固件、metadata 实现或构建，不建立远程工程分支。
- 原始证据：[只读命令与完整输出](t016-a005-yocto-review-raw.log)，A/B/C 三组均 exit 0。命令使用已存在的用户 SSH 配置和 known_hosts；初次默认沙箱调用因 SSH 配置上下文不可用未取得数据，随后按既有授权使用获准的只读访问。源码摘录和旧构建日志均保留其原有性质。

## 1. 工作区与实际选择链

| 对象 | 实际主机、路径与版本 | 本轮覆盖及限制 |
|---|---|---|
| OPC | 本机 `D:/opc-book-yocto`；`feature/book-yocto/t-016-chapter4-revision`；评审开始 HEAD `d57adc9539d3c0a0212fb0353e33aa170ca5381a` | 开始时干净；受评稿 blob 与固定提交一致 |
| 书稿 | 本机 `D:/book-yocto`；`feature/t-016-chapter4-revision`；`f444790f352b46f699fd3d46198f8b9b9f089a62`；`git@github.com:yinzhongkai/book-yocto.git` | 与 WORKSPACE 基线一致，干净；未写入 |
| meta-tiger | 主机 `tiger`，用户 `oops`；`/home/oops/workspace/meta-tiger`；`master`；`0e3da48b34ef772532535a730a21878cc30ccf67`；`git@github.com:yinzhongkai/meta-tiger.git` | 与 WORKSPACE 一致，干净；只有 layer 骨架，没有 machine、固件配方或 QEMU bbappend；未写入 |
| Poky | 同主机 `/home/oops/workspace/poky`；`scarthgap`；`77d1feb37e280733684ae8a9449fb031d5d7ff40`；`https://git.yoctoproject.org/poky.git` | 实际读取的辅助源码 checkout，干净；完整提交锁定本轮证据，不自动更新 WORKSPACE 或授权其写入 |
| 当前 build 配置 | `/home/oops/workspace/build/conf/{bblayers,local}.conf` | `MACHINE="qemuarm64"`、`DISTRO?="poky"`；启用 Poky 的 meta/meta-poky/meta-yocto-bsp 和 meta-tiger；不是 tiger 实施配置 |
| meta-arm / TF-A | `/home/oops/workspace/meta-arm` 不存在；当前 BBLAYERS 没有 meta-arm；在上述四个启用层内未找到 TF-A 配方 | 没有可报告的实际选中 meta-arm 提交或 TF-A 源码基线；不推断整个主机不存在其他 checkout。后续引入时另行锁定 |

QEMU 的一手选择链为：`qemu-helper-native_1.0.bb` 的依赖指向 `qemu-system-native`；当前启用层有 `qemu-system-native_8.2.7.bb`，其 `BPN="qemu"`、`require qemu-native.inc`；后者 `require qemu.inc` 并使用 native 类。`qemu.inc:21–71` 以 `https://download.qemu.org/qemu-8.2.7.tar.xz` 加 OE patch 形成源码基线。对当前四个启用层查找未发现 QEMU bbappend。

已存在的 `/home/oops/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/`、`temp/log.do_patch` 和源码 `.pc/applied-patches` 与这条链一致。本轮没有执行 BitBake 解析、`bitbake -e`、fetch、patch 或构建，因此以上是“当前文件与已存在任务产物相互印证”，不是重新展开所有环境 override 后的 tiger 构建结论。将来实际集成时必须保存最终配方选择及展开的 `SRC_URI`，不能仅凭本轮静态记录宣布未来选择已验证。

## 2. QEMU 精确源码与已有 patch

已解包、打过补丁的源码目录为：

`ssh tiger:/home/oops/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7`

这是源码包加配方补丁的来源，不以虚构的 QEMU Git HEAD 表达。全部 hash 均为本轮对现有文件计算的 SHA-256：

| 文件（除特别说明外相对 Poky） | SHA-256 |
|---|---|
| `/home/oops/workspace/build/downloads/qemu-8.2.7.tar.xz` | `1f0604f296ab9acb4854c054764a1ba408643fc299bd54a6500cccfaaca65b55`（与配方一致） |
| `meta/recipes-devtools/qemu/qemu.inc` | `ce75d5fccd7c588b0063408fd0c3f51ccd7e00f94029eea605964076b57edc33` |
| `scripts/runqemu` | `d644b4763b968414649f948151d51ff84b1663218ffd4abae3b58d19e4ba7861` |
| `meta/classes-recipe/qemuboot.bbclass` | `87d8cc1bcb8db862ae9a80f53c685e61b750867060fb563804a92bf1dedde01f` |
| 已打补丁源码 `.pc/applied-patches` | `3b7c0560451d78d8eae6512a4c21d77129eb3b0e1106512949709e8ef0eb8430` |

`.pc/applied-patches` 记录 **31 个补丁**，顺序在原始日志 A 中完整列出；前 29 个来自公共 `SRC_URI`，末尾两项 `0011-linux-user-workaround-for-missing-MAP_FIXED_NOREPLAC.patch`、`0012-linux-user-workaround-for-missing-MAP_SHARED_VALIDAT.patch` 来自 `class-native` 追加。原有 `log.do_patch` 有相应应用记录和 `patch_do_patch finished`，不是本次重跑结果。所有配方补丁内容均由上述 Poky 完整提交锁定。旧 work 源码目录未做全树差异核验，应用记录不能单独排除额外手工修改；本轮仅对下述接口文件记录 hash，正式重构建仍须从锁定来源重新取得并应用补丁。

本章新增 tiger patch 应对这一带 OE 补丁的基线生成、核对并集成，而不是只称“QEMU v8.2.7”。运行验证使用 `qemu-system-native` 的 system emulator；`qemu-native` 的 user emulation 不能替代它。未来 bbappend 的适用范围、新增 patch 顺序及清理 devtool workspace 后的选择证据须明确，避免覆盖或遗漏已有补丁。

## 3. 最小测试固件的部署与 runqemu 输入契约

以下是待 architect/QEMU 成员确认后实施的路线建议，**未创建配方、固件或 qemuboot 文件，未运行命令**。

1. 在 meta-tiger 管理独立的最小测试固件源与交叉编译配方，使用最终确认的入口、链接地址、栈/可写内存和 UART 约定；产出可核对的 ELF、map 与 raw binary。固件包含由 CPU 实际执行产生的确定启动标记，版本和 hash 随部署证据保存。
2. 经 deploy 任务发布有版本名的 raw binary 及配套 `<name>.qemuboot.conf` 到该 machine 的 `DEPLOY_DIR_IMAGE`；明确构建顺序和 `qemu-helper-native:do_addto_recipe_sysroot` 等依赖。配置生成应复用锁定版本的字段、`[config_bsp]` 格式和路径规则，纳入 BitBake 任务及签名，不能靠用户临时手写 INI 代替正式集成。
3. `qemuboot.bbclass:125–143` 依赖 image 命名和 `do_rootfs`/`do_image` 任务上下文，`190` 通过 `EXTRA_IMAGEDEPENDS` 增加原生工具。独立固件配方不能只 `inherit qemuboot` 就假定这些 image 依赖有效。实施时应在自身 deploy/生成任务中明确相同功能；具体 metadata 和任务图在实施阶段验证，本轮不宣称已经成立。
4. 使用生成的 qemuboot 文件作为 runqemu 的显式输入；计划命令形态为 `runqemu <已部署的版本化.qemuboot.conf> nographic nonetwork`。先核对变量、目录和 native 工具，再运行。`runqemu:372–378` 把位置参数 `.bin` 当作 kernel，固件 binary 不应直接放在这个位置。

| 配置/接口 | 此路线要求及实际依据 |
|---|---|
| Machine/CPU/内存 | `MACHINE` 与 BSP 一致；`QB_SYSTEM_NAME=qemu-system-aarch64`；`QB_MACHINE` 选择真正 tiger，CPU/SMP/RAM 与 QEMU 审定的最小契约一致；采用 TCG，不传 kvm。A-005 的 A53/4核/1GiB 尚是候选，本轮不批准这些型号/数量 |
| 无 Linux/rootfs | 同时明确 `QB_DEFAULT_KERNEL=none`、`QB_DEFAULT_FSTYPE=none`、`QB_ROOTFS=none`；`runqemu:757` 跳过 kernel，`679–683` 跳过 rootfs 查找，`1258` 跳过 rootfs 设备装配。仅设置 kernel=none 不够 |
| 固件传入 | `QB_DEFAULT_BIOS` 使用 deploy 目录内已存在的版本化 binary 名；`800–815` 取 basename 后拼接 `DEPLOY_DIR_IMAGE` 并检查存在，`1585–1586` 添加 `-bios`。machine 对 raw 镜像的映射、长度检查和入口由 QEMU 成员核对 |
| 配置格式和路径 | `[config_bsp]`；`934–942` 将键转为大写，并处理 `../` 和 `.`。明确有效的 native staging 目录及 bindir，避免缺失后触发 `bitbake -e` 的隐式回退；显式配置的 deploy 目录按 `validate_paths` 规则核对 |
| 设备参数 | 清空 `QB_RNG`、`QB_GRAPHICS`、`QB_OPT_APPEND`，使用 `QB_NET=none`/nonetwork，移除来自 qemuarm64 的 PCI、virtio、USB、磁盘和 virtio serial 参数。`qemuboot.bbclass:101/103` 有默认 virtio 设备；`qemuarm64.conf:15–30` 属于 virt 板，不能整份继承后仅改 machine |
| 串口 | nographic 路径由 `setup_serial` 添加 `-serial mon:stdio`，实际 UART0 要接对应 chardev；不重复追加另一个 stdio 串口。不假设有 virtio console，接线及 model reset 由 QEMU 成员确认 |
| 原生工具来源 | `qemuboot.bbclass:157–159` 指向 qemu-helper-native 的持久 recipe sysroot；`runqemu:1515–1534` 找不到原生二进制会尝试宿主 PATH。因此验证必须记录实际可执行文件绝对路径/hash，宿主 QEMU 回退不算 D-013 集成通过 |
| 宿主前置条件 | 本版本 `check_and_set:862–864` 在非 slirp/bridge 时仍检查 TUN，早于 nonetwork 的处理；`check_tun:113–120` 检查存在与可写。现有主机 `/dev/net/tun` 为 `crw-rw-rw-`，静态条件满足；本轮未改权限或建立网卡，也不由此声称启动通过 |

现有 helper sysroot 中找到：

`/home/oops/workspace/build/tmp/work/x86_64-linux/qemu-helper-native/1.0/recipe-sysroot-native/usr/bin/qemu-system-aarch64`

其当前 SHA-256 为 `d77b1af41dd5b8107e343f4a0893797b1754e8d2f98656af142805bd4f546534`。这只是现有二进制身份；未运行它，也不能证明其中包含尚不存在的 tiger patch。实施后的二进制需重新记录。

第 4 章启动证据至少应关联：固件源/配方/patch 精确版本，binary 与 qemuboot hash，清理 workspace 后的构建日志，最终展开参数及 native QEMU 绝对路径，CPU0 执行固件生成的串口标记，返回码与运行终态。可在固件标记前后记录约定的 CPU/EL/入口状态或调试证据，避免把宿主程序打印当作 guest 执行。中断、guest 请求 reset 和多核测试若仍纳入本章，先给出实际触发接口和固件增量；枚举 machine、`-S`、对象实例化、qemuarm64 启动日志均不满足本章启动判据。

## 4. SPI NAND：后续依赖及阶段门槛

U-A005-01 的接入关系应为 **SPI 主控 → CS → SPI NAND 芯片**。移除独立并行 NAND 控制器窗口和 IRQ，不移除 SPI 主控。A-005 §4.1/§4.3/§4.4/§6/§7.1/Q-07 须据此统一修订，SPI 主控窗口/IRQ/时钟、CS 和型号继续标候选。

锁定、已打补丁的 QEMU 8.2.7 源码有 `hw/ssi/pl022.c`：它创建 SSI bus、MMIO 和 IRQ；有 `hw/block/m25p80.c`，其 SPI_NOR 标识、SSI peripheral 类型和 CS 语义说明这是 NOR 模型；`hw/block/nand.c` 暴露 `nand_setpins(cle, ale, ...)`/setio/getio 等并行 NAND 接口。对该源码的 `hw` 和 `include/hw` 检索 `spi.?nand|spinand` 没有文本命中。**本轮未找到可直接使用的 SPI NAND 芯片模型**，不是完成了所有模型/API 的独立审查；也不能把 NOR 或并行 NAND 改一个名称就宣布支持 SPI NAND。QEMU 成员需审定 master/CS 接线、可复用范围及缺失芯片模型的工程量。

上述 QEMU 文件 SHA-256：PL022 `ec7946bf40de500d635875072b5d54b18cf789eda688a6c757d353e0ae0f5f53`；m25p80 `e7d41385d34bc541b9a522f4a554c15bf4603f829eea66ad49a83e99398cd1a7`；nand `92e870a4d6103c836128aae3b55f62fb47ffa755d65f92fe5cb060999993c00f`。

实际已存在 Linux checkout `/home/oops/workspace/build/tmp/work-shared/qemuarm64/kernel-source` 的 HEAD 为 `b96d2d770cab02b84c46606fa538ce027bee6b08`，干净；与 Poky `linux-yocto_6.6.bb:22` 的 qemuarm64 override 一致。这里的代码提供后续接口依据，**不是 tiger 驱动已配置或启动的证明**。其中：

- `drivers/mtd/nand/spi/core.c` 使用 `spi_mem_exec_op` 和 `spi_mem_supports_op`；SHA-256 `9f7ed72297997fd01fbdea56bc59120502545c9b53e9e77064354b0ec4374bcd`。
- `drivers/mtd/nand/spi/Kconfig` 的 `MTD_SPI_NAND` 依赖 `SPI_MASTER`，选择 `SPI_MEM`；不能只打开 NAND 核心而省略 master driver。
- `drivers/spi/Kconfig:789–798` 的 `SPI_PL022` 依赖 `ARM_AMBA`；已有 `spi-pl022.c` 注册 transfer_one_message，SHA-256 `427545b979c691c44b73bbfb90880402a91b2b6a27853568b6dd522a4bb019f0`。存在此驱动不保证候选板级 AMBA/DT/clock/CS 接线或 SPI memory 操作已兼容，需实际主控 binding 和所选芯片操作组合核对。

后续集成前必须明确：芯片型号与 ID、单/双/四线及命令/dummy/CS 要求、页缓存与 program/erase/status 行为、busy/reset、OOB/坏块标记、片内或宿主 ECC 责任和错误传播；主控驱动、AMBA/DT binding、时钟及 CS；TF-A 初始化 SPI 并读取 BL2/FIP 的平台 IO；U-Boot/Linux 对应驱动和一致分区表；后端数据/OOB 布局及镜像总装转换。512MiB 数据容量不等于后端文件长，候选 2KiB 页、128KiB 块、LEB 与 UBI 参数需在芯片确定后复核。

若 chapter 6 首次通过 SPI NAND 读取 BL2（候选偏移 `0x40000`）或 FIP（`0x100000`），所需 SPI 主控/芯片模型、TF-A 读取路径、坏块/ECC 处理和镜像布局就必须在该读取前就绪，不能把全部 Q-07 门槛推到 chapter 9/10。若先用明确的其他测试介质，则将该阶段的有限覆盖写清，再在首次 SPI NAND 集成前满足这些依赖。完整 UBI/A/B/持久化等仍按后续章节安排；不因本次用户意见提前实施，也不取消全书最终全链验证。

## 5. 后续组件基线及未覆盖范围

当前 Poky 的 `linux-yocto_6.6.bb` 为 `LINUX_VERSION=6.6.151`，qemuarm64 branch `v6.6/standard/qemuarm64`，machine revision 见上，kernel metadata revision `dd201eb6ec53179f45836d41610ca68c725d28e2`。Poky distro 偏好 `6.6%`，machine include 指向 linux-yocto；现配方 `COMPATIBLE_MACHINE` 未含 tiger。将来 tiger BSP 须单独确认兼容、branch/revision、配置和 patch，不把 qemuarm64 override 当作虎板最终基线。

可读取的 U-Boot 配方为 `meta/recipes-bsp/u-boot/u-boot_2024.01.bb`，包括 `u-boot-common.inc`/`u-boot.inc`；common 的 `SRCREV=866ca972d6c3cabeaf6dbac431e8e08bb30b3c8e`，来源 `git://source.denx.de/u-boot/u-boot.git;protocol=https;branch=master`，并有 common 的一项及版本配方的九项补丁。本轮只记录未来接口所需配方来源，没有执行 provider 选择展开、取得/核对 U-Boot 实际源码或编译。TF-A/meta-arm 未启用且没有已选源码可核查，不能把书稿或草稿版本号升级为工程事实。这些后续未选项不单独阻塞 chapter 4 的测试固件路线。

未覆盖：Q-02/Q-03 的 ARM/GIC/reset API 独立模型审查；实际新增固件、qemuboot 生成任务图与参数完整性；新 tiger patch 的应用/编译/清理 workspace 后复验；CPU、UART、IRQ、guest reset 或 runqemu 执行；SPI NAND 芯片型号/数据手册和模型实现；TF-A/U-Boot/Linux 虎板驱动、构建、BL2/FIP/全链启动；NAND/OOB/ECC/UBI/镜像及 watchdog 实测。现有 build 产物只作为既有环境的一手证据，不作为本轮新增验证结果。

补充一手文档为 [Yocto 5.0.20 QEMU 使用说明](https://docs.yoctoproject.org/5.0.20/dev-manual/qemu.html)、[Linux 6.6 SPI 说明](https://docs.kernel.org/6.6/spi/spi-summary.html) 和 [Linux v6.6 SPI NAND 核心](https://github.com/torvalds/linux/blob/v6.6/drivers/mtd/nand/spi/core.c)。它们只辅助说明通用接口；项目结论优先采用上面固定 checkout 的本地源码，不用网页版本替换工程基线。

## 6. 本轮治理文件校验

`python -B scripts/validate_framework.py` 返回 0：21 个角色、36 份技能、1 个实际项目、1 套项目模板、467 处本地链接校验通过；此工具不验证业务语义或启动结果。`git diff --cached --check` 返回 0。Python 对比修改前后的 TASKS，确认本人意见之前及 qemu-engineer 意见起至文件末尾完全一致；受评 A-005 的 Git blob 仍为 `d7d474e4de48ad8e05ffd4d9893594214843f9be`。命令及输出附于原始日志 D；既有规则对 `.log` 关闭 diff，保留原始输出中的源码行号/空白。

本轮意见提交后停止；project-manager 衔接 qemu-engineer 的专业核对，architect 待全部意见到齐后统一修订。此报告不代表设计获批或 chapter 4 实施已经开始。
