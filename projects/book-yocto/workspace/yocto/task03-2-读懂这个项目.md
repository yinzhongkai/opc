## 2 读懂这个项目

周二上午，阿凯一到工位就打开终端。昨天构建的 `core-image-minimal` 还在 `tmp/deploy/images/qemuarm64/` 里躺着。达哥从内网 wiki 上拉了一张图，发到阿凯屏幕上。

"跑起来了？"达哥问。

"嗯，昨晚 `runqemu` 进了 shell。"阿凯回答。

"好。现在你打开 `poky` 仓库，告诉我你看到了什么。"

阿凯把窗口切过去，从上到下念："`bitbake/`、`meta/`、`meta-poky/`、`meta-yocto-bsp/`……昨天翻过了。"

"光看目录没用。"达哥把架构图放大，"你接下来三个月做的所有事，都在这张图上。昨天你跑的是 `qemuarm64`——通用机器。今天我要你在这张图上找到 tiger 的每个零件，然后在 Poky 里指出它对应哪个目录、哪个 recipe。"

阿凯盯着图：**Cortex-A53**、**DDR**、**NAND Flash**、**SPI NOR Flash**、**UART**、**RTC**、**Watchdog**……旁边还挂着启动链的六个阶段。"从哪开始？"

"从硬件。CPU、内存、Flash、外设——序章给你讲过了，现在你去 `meta` 里找一个真实 MACHINE 的配置，看看硬件是怎么描述给 Yocto 项目的。然后顺着启动链走，最后告诉我 tiger 的应用程序应该放哪。"

阿凯脱口而出："放 `meta-tiger` 里？"

达哥没回答，把椅子转回去了。

### 2.1 tiger 硬件全貌的 Yocto 项目映射

阿凯先把达哥发来的架构图存到本地，又打开一个终端。他没有急着敲命令，而是把图上的每个硬件框都标上了序号：CPU、DDR、NAND Flash、SPI NOR Flash、UART、RTC、Watchdog、I2C EEPROM。

"这些框不是装饰，"他在心里提醒自己，"每个框在 Poky 里都得有人负责。"

`tiger` 和 `qemuarm64` 一样都是 ARMv8-A 64 位，先从最接近的参考 MACHINE 看起最省时间。阿凯先定位 `qemuarm64.conf` 的位置。

```bash
# 定位 qemuarm64 的 MACHINE 配置文件
find ~/workspace/poky -name "qemuarm64.conf"
```

输出：

```text
/home/<your-username>/workspace/poky/meta/conf/machine/qemuarm64.conf
```

```bash
# 查看参考 MACHINE 配置的内容（只展示关键行）
cat ~/workspace/poky/meta/conf/machine/qemuarm64.conf
```

输出（关键行）：

```bitbake
#@TYPE: Machine
#@NAME: QEMU ARMv8 machine

require conf/machine/include/arm/armv8a/tune-cortexa57.inc
require conf/machine/include/qemu.inc

KERNEL_IMAGETYPE = "Image"
UBOOT_MACHINE ?= "qemu_arm64_defconfig"
SERIAL_CONSOLES ?= "115200;ttyAMA0 115200;hvc0"

# For runqemu
QB_SYSTEM_NAME = "qemu-system-aarch64"
QB_MACHINE = "-machine virt"
QB_CPU = "-cpu cortex-a57"
# ... (省略 QEMU 启动参数)
```

阿凯把这几行抄在便签上，逐条问达哥。

"`KERNEL_IMAGETYPE = "Image"` 是 **内核镜像类型（KERNEL_IMAGETYPE）**，指定内核编译产物格式。`Image` 是 AArch64 的未压缩内核镜像，tiger 也会用这个格式。"达哥指着屏幕，"`tune-cortexa57.inc` 是体系调优入口，tiger 用 Cortex-A53，后续会换对应的 tune 文件。"

"`SERIAL_CONSOLES` 呢？"

"串口控制台。`115200;ttyAMA0` 表示波特率 115200，设备节点 `ttyAMA0`。QEMU 的 **PL011** 串口模型会对应这个节点。 tiger 的串口也是 PL011，到时候直接复用这个格式。`hvc0` 是 QEMU virtio console 的备用节点。"

"`qemu.inc` 里还有什么？"阿凯又问。

"`qemu.inc` 是 QEMU 机器的公共包含文件。"达哥打开文件，"里面设了 **机器特性（MACHINE_FEATURES）**、**镜像格式（IMAGE_FSTYPES）** 和内核提供者。注意 `MACHINE_FEATURES` 只描述硬件能力，不替发行版决定软件策略。"

```bitbake
# 文件路径：~/workspace/poky/meta/conf/machine/include/qemu.inc（关键行）
MACHINE_FEATURES = "alsa bluetooth usbgadget screen vfat"
IMAGE_FSTYPES += "tar.bz2 ext4"
PREFERRED_PROVIDER_virtual/kernel ??= "linux-yocto"
```

> **💡 提示**：`KERNEL_IMAGETYPE` 和 `IMAGE_FSTYPES` 本章只点到为止，后续写 `tiger-aarch64.conf` 时会再展开。

阿凯点点头，在图上把 `SERIAL_CONSOLES`、`MACHINE_FEATURES` 两个词标在 CPU/UART 旁边。然后他开始找外设在 Poky 里的对应位置。

```bash
# 在 linux-yocto 配方中查找 tiger 外设相关的内核特性配置
# PL011 UART、PL031 RTC、m25p80 SPI NOR、at24 I2C EEPROM
find ~/workspace/poky/meta/recipes-kernel/linux/ -type f \( -name "*.scc" -o -name "*.cfg" \) | \
  xargs grep -l -E "pl011|pl031|m25p80|at24" 2>/dev/null
```

输出（示例，可能为空或仅匹配部分）：

```text
# 在 OE-Core 的元数据目录中，这些驱动关键字通常无直接匹配
# 它们随 linux-yocto 内核源码一起提供，位于下载后的内核源码树中
```

```bash
# 查找 NAND/UBI 用户空间工具对应的 recipe
find ~/workspace/poky/meta/recipes-devtools -name "mtd-utils*.bb"
```

输出：

```text
/home/<your-username>/workspace/poky/meta/recipes-devtools/mtd/mtd-utils_git.bb
```

> **💡 提示**：`mtd-utils` 在 Scarthgap 中使用基于 git 的版本命名 `mtd-utils_git.bb`，以你本地文件名为准。

"PL011 UART、**PL031** RTC、SPI NOR 的 **m25p80** 模型、**I2C EEPROM** 的 **at24** 模型，这些通用驱动都随 `linux-yocto` 内核源码一起提供。"达哥说，"NAND 和 **UBI/UBIFS** 相关的工具则由 `mtd-utils` 这个 recipe 提供。"

阿凯把结果填进一张表里，作为今天的第一个"地图坐标"。

**Fig-2-1 tiger 硬件框图（Yocto 项目映射版）**

| tiger 硬件组件 | 在 Poky/Yocto 项目中的对应位置 | 说明 |
|---|---|---|
| Cortex-A53 4 核 | `meta/conf/machine/include/arm/armv8a/tune-cortexa57.inc` | 体系调优入口（tiger 后续换 A53 对应 tune） |
| DDR | TF-A BL2（后续由 `meta-arm` 集成，后续章节引入） | 启动链中初始化内存 |
| NAND Flash + UBI | `meta/recipes-devtools/mtd/mtd-utils_git.bb` | UBI/UBIFS 工具集 |
| SPI NOR Flash | `linux-yocto` 内核源码中的 **m25p80** 驱动 | QEMU 设备模型对应 |
| I2C EEPROM | `linux-yocto` 内核源码中的 **at24** 驱动 | 板载 EEPROM |
| UART / PL011 | `linux-yocto` 内核源码中的 `pl011` 驱动 | `SERIAL_CONSOLES` 指定节点 |
| RTC / PL031 | `linux-yocto` 内核源码中的 `pl031` 驱动 | 由设备树绑定 |
| Watchdog | `linux-yocto` 内核源码中的看门狗驱动 | 板载异常恢复 |
| 内核设备树 | **内核设备树（KERNEL_DEVICETREE）** 变量（`tiger-aarch64.conf` 中设置） | 指定编译哪些 `.dtb` |

"所以 tiger 不需要从零写所有驱动？"阿凯问。

"大部分通用外设，Poky 和 `meta-arm` 里已经有了。"达哥说，"我们要做的是：把 tiger 的地址、中断、引脚差异用设备树描述清楚，再用 **内核设备树（KERNEL_DEVICETREE）** 变量告诉内核要编译哪份设备树。"

### 2.2 启动链与构建产物对照

硬件坐标找完了，阿凯顺着达哥的架构图往上看启动链。序章已经讲过六个阶段：**BL1** / **Boot ROM** → **BL2 (TF-A)** → **BL31 (TF-A)** → **BL33 (U-Boot)** → Linux Kernel → **UBI rootfs**。他现在想知道："这六个阶段在 Yocto 项目构建产物里长什么样？"

他先回忆昨天 `qemuarm64` 的 deploy 目录。

```bash
# 查看 qemuarm64 的构建产物
ls $BUILDDIR/tmp/deploy/images/qemuarm64/
```

输出（关键文件）：

```text
Image
core-image-minimal-qemuarm64.rootfs-<时间戳>.ext4
# ... (省略)
```

"`qemuarm64` 只有 `Image` 和 rootfs，"阿凯自言自语，"没有 TF-A，没有 U-Boot。"

"对。"达哥凑过来，"QEMU 机器通常由 QEMU 自己模拟固件和 bootloader，直接加载内核。但 tiger 是真板子逻辑，必须走完整启动链。你现在要关心的是：Yocto 项目怎么决定用哪个 bootloader、用哪个 kernel。"

阿凯想起昨天达哥说 BitBake 里有"虚包"机制。他尝试查看当前 MACHINE 下的 kernel 提供者。

```bash
# 确保已初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 在当前 MACHINE=qemuarm64 的构建环境下，查看虚拟 kernel 的提供者
bitbake -e virtual/kernel | grep PREFERRED_PROVIDER_virtual/kernel
```

输出（示例）：

```text
PREFERRED_PROVIDER_virtual/kernel="linux-yocto"
```

"这就是 **优先提供者（PREFERRED_PROVIDER）** 机制。"达哥指着输出，"`virtual/kernel` 是个虚包名，BitBake 允许多个 recipe 声称自己能提供 kernel。`PREFERRED_PROVIDER` 决定最终用谁。`qemuarm64` 的 kernel 由 `linux-yocto` 提供。"

"那 bootloader 呢？"阿凯问。

"QEMU 机器通常不设 `PREFERRED_PROVIDER_virtual/bootloader`，因为 QEMU 直接加载内核，不需要真实 bootloader。"达哥说，"但 tiger 会设。以后我们会写 `PREFERRED_PROVIDER_virtual/bootloader = "u-boot-tiger"`，还会加 `PREFERRED_PROVIDER_virtual/trusted-firmware-a = "trusted-firmware-a"`。这些现在只是前瞻，你先知道有这回事。"

阿凯把启动链和产物对应起来，画了第二张表。

**Fig-2-2 启动链与 Yocto 项目构建产物对照**

| 启动阶段 | 负责组件 | Yocto 项目产物文件 | qemuarm64 是否有 |
|---|---|---|---|
| BL1 / Boot ROM | SoC 出厂固化 | 不由 Yocto 项目构建 | 由 QEMU 模拟 |
| BL2 (TF-A) | Trusted Firmware-A | `bl2.bin` / `flash.bin`（后续集成） | 无 |
| BL31 (TF-A) | Trusted Firmware-A | `bl31.bin`（后续集成） | 无 |
| BL33 (U-Boot) | U-Boot | `u-boot.bin` / `u-boot.dtb`（后续集成） | 无 |
| Linux Kernel | linux-yocto / linux-tiger | `Image`、`tiger-aarch64.dtb` | 有 `Image` |
| UBI rootfs | 镜像配方 | `*.ubifs` / `*.ubi`（后续配置） | `*.ext4` |

"现在明白为什么 `qemuarm64` 昨天一跑就通了吗？"达哥问，"因为 QEMU 替它吃了前半段启动链。 tiger 不行，前半段得我们自己一片片拼。"

### 2.3 软件栈分层与归属决策

硬件和启动链都串起来了，阿凯的注意力落到最上面一层：应用程序。达哥刚才没回答的问题还在他脑子里转。

"达哥，"阿凯转过身，"tiger 的网关应用程序、Web 配置界面——这些也写成 recipe 放进 `meta-tiger` 吗？"

达哥没有正面回答："你昨天构建的 `core-image-minimal` 里有什么？"

"BusyBox、一些基础工具、内核、根文件系统……没有应用程序。"阿凯说。

"那你想想，如果我把 Web 后端、数据库、业务逻辑全塞进 `meta-tiger`，三个月后应用团队要更新一个功能，怎么办？"

阿凯迟疑了一下："重新 `bitbake`？"

"重新 `bitbake` 意味着整个 rootfs 重新构建、重新验证、重新出 license 清单。应用团队只是想改一个 API 响应。"达哥顿了顿，"`meta-tiger` 只放 BSP 层的东西——让板子能启动、能跑内核、有基础文件系统。应用程序是另一回事。"

阿凯追问："那 OTA 升级怎么做？"

"分成两条线。"达哥在纸上画了两条竖线，"BSP 更新走全量固件升级，周期长、验证重；应用更新走容器或独立包升级，轻量、快速。把它们分开，才能各按各的节奏走。"

"开源合规呢？"

"`meta-tiger` 里的组件越少，license 清单越干净。业务应用如果是自研闭源或第三方闭源 **软件开发工具包（SDK）**，本来就不该混进 BSP 的 license 审计范围。"达哥把笔放下，"长期维护也是同理。三年后别人接手 `meta-tiger`，他应该看到的是一块板子的 BSP，而不是你们公司的业务史。"

阿凯挠了挠头，把决策规则记进本子：

- **进 `meta-tiger`**：bootloader、kernel、设备树、基础驱动、根文件系统骨架、工具链配置、Distro 策略。
- **不进 `meta-tiger`**：业务应用、Web 服务、UI 逻辑、产品配置、第三方闭源 SDK。

> **💡 提示**：可以把 BSP 想象成"房子的地基和水电"，应用是"装修和家具"。地基出问题整栋房子都危险，但换一盏灯不应该敲承重墙。

**Fig-2-3 软件栈三层归属图**

```text
┌─────────────────────────────────────────────────────────────┐
│  Application 层（不进 meta-tiger）                            │
│  网关应用 / Web 配置 / 业务 SDK                               │
├─────────────────────────────────────────────────────────────┤
│  OS / BSP 层（进 meta-tiger）                                 │
│  Linux Kernel / 设备树 / 根文件系统 / DISTRO 策略             │
├─────────────────────────────────────────────────────────────┤
│  Firmware / Bootloader 层（进 meta-tiger）                    │
│  TF-A (BL2/BL31) / U-Boot (BL33)                            │
└─────────────────────────────────────────────────────────────┘
```

### 2.4 层（Layer）叠加机制

理解了软件栈归属，阿凯回头再看昨天那三个 layer。task 02 里 `bitbake-layers show-layers` 输出过：

```text
layer                path                                                                   priority
========================================================================================================
core                 /home/<your-username>/workspace/poky/meta                                5
yocto                /home/<your-username>/workspace/poky/meta-poky                          5
yoctobsp             /home/<your-username>/workspace/poky/meta-yocto-bsp                     5
```

> **💡 提示**：第一列是各 layer 的 `BBFILE_COLLECTIONS` 名称（`core`/`yocto`/`yoctobsp`），不是目录名。这三个名称由对应 layer 的 `layer.conf` 中 `BBFILE_COLLECTIONS` 变量固定声明，不会随本地路径变化。

"它们为什么需要三个？"阿凯问。

"因为职责不同。"达哥说，"`meta` 是 OE-Core，提供通用 recipe、class 和默认策略；`meta-poky` 是 Poky 发行版，声明 Distro 策略；`meta-yocto-bsp` 是官方参考 BSP，提供 MACHINE 配置。三者叠加，才凑出一个能构建 `qemuarm64` 的环境。"

阿凯先验证哪个 recipe 由哪个 layer 提供。

```bash
# 确保已初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 查看 core-image-minimal 由哪个 layer 提供（默认输出：层名 + 版本号）
bitbake-layers show-recipes core-image-minimal
```

输出（关键行，示例）：

```text
=== Matching recipes: ===
core-image-minimal:
  meta                 1.0
```

"默认输出只显示层名和版本号。"达哥说，"想看完整路径，加 `-f`。"

```bash
# 查看 core-image-minimal 的完整文件路径
bitbake-layers show-recipes -f core-image-minimal
```

输出（关键行，示例）：

```text
=== Matching recipes: ===
core-image-minimal:
  /home/<your-username>/workspace/poky/meta/recipes-core/images/core-image-minimal.bb
```

> **💡 提示**：`-f` 模式只输出文件路径，不输出层名 + 版本。以你本地实际输出为准。

"`core-image-minimal` 来自 `meta` 层。"阿凯说。

"对。基础镜像、基础包组一般在 OE-Core。`meta-poky` 更多是做发行版微调，比如默认启用哪些特性、用哪个 init 系统。"

达哥让阿凯再看 `DISTRO_FEATURES` 和 `MACHINE_FEATURES` 的叠加结果，解释两者的区别：`DISTRO_FEATURES` 是 **发行版特性（DISTRO_FEATURES）**，由 DISTRO 配置声明系统级策略，回答"这个发行版选择支持什么"。

```bash
# 确保已初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 查看叠加后的 DISTRO 特性和 MACHINE 特性
bitbake -e core-image-minimal | grep -E "^(DISTRO_FEATURES|MACHINE_FEATURES)="
```

输出（示例，具体值因版本略有不同，以本地实际输出为准）：

```text
DISTRO_FEATURES="acl alsa bluetooth debuginfod ext2 ipv4 ipv6 pcmcia usbgadget usbhost wifi xattr nfs zeroconf pci 3g nfc x11 vfat seccomp opengl ptest multiarch wayland vulkan sysvinit pulseaudio gobject-introspection-data ldconfig"
MACHINE_FEATURES="alsa bluetooth usbgadget screen vfat"
```

"注意看，`DISTRO_FEATURES` 比 `MACHINE_FEATURES` 长很多。"达哥指着输出，"`MACHINE_FEATURES` 只回答'这块板子有什么'，`DISTRO_FEATURES` 回答'这个发行版选择支持什么'。两者合并起来，才决定最终编译哪些东西。"

"那如果两个 layer 都提供了同名的 recipe，BitBake 选哪个？"

"看 **层优先级变量（BBFILE_PRIORITY）** 和 `BBLAYERS` 的顺序。"达哥打开 `bblayers.conf`，"`BBFILE_PRIORITY` 数值越大优先级越高；同优先级时，`BBLAYERS` 中排得越靠后的层优先级越高。这个规则只适用于'同名 recipe 出现在不同层'的场景，普通变量冲突另有规则。"

```bitbake
# 文件路径：~/workspace/build/conf/bblayers.conf（当前内容）
BBLAYERS ?= " \
  /home/<your-username>/workspace/poky/meta \
  /home/<your-username>/workspace/poky/meta-poky \
  /home/<your-username>/workspace/poky/meta-yocto-bsp \
  "
```

> **💡 提示**：写入 `bblayers.conf` 时，将 `/home/<your-username>` 替换为你主机的真实绝对路径。

阿凯把 layer 叠加画成第四张图。

**Fig-2-4 Layer 叠加示意图**

| 层级（从上到下） | Layer | 主要职责 | 覆盖/扩展对象 |
|---|---|---|---|
| 最底层 | `meta` | OE-Core：通用 recipe、class、默认变量 | 通用基础 |
| 中间层 | `meta-poky` | Poky 发行版：DISTRO 策略、默认镜像微调 | `meta` 的默认策略 |
| 中间层 | `meta-yocto-bsp` | 官方参考 BSP：qemuarm64 等 MACHINE 配置 | `meta` 的 MACHINE 默认 |
| 最上层（待添加） | `meta-tiger` | tiger 专用 BSP：tiger-aarch64 配置、启动链集成 | 覆盖/扩展下层默认 MACHINE |

"所以 `meta-tiger` 会加在最上面，专门覆盖下层的默认 MACHINE 配置。"阿凯说。

"没错。"达哥点头，"它只负责 tiger 这台机器与众不同的部分， common 的东西留给下层。"

### 2.5 认识镜像配方与 IMAGE_INSTALL

最后一块拼图是镜像本身。阿凯昨天构建的 `core-image-minimal` 为什么只有 BusyBox 和基础工具？他打开镜像配方的源码。

```bash
# 查看核心最小镜像的配方
cat ~/workspace/poky/meta/recipes-core/images/core-image-minimal.bb
```

输出：

```bitbake
SUMMARY = "A small image just capable of allowing a device to boot."

IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

LICENSE = "MIT"

inherit core-image

IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE:append = "${@bb.utils.contains(\"DISTRO_FEATURES\", \"systemd\", \" + 4096\", \"\", d)}"
```

"这就是 **镜像配方（Image Recipe）**。"达哥说，"它和普通 recipe 最大的区别是：普通 recipe 描述怎么编译一个软件包，image recipe 描述'这个镜像里要装哪些包'。它通过继承 `core-image`（来自 `image.bbclass`）获得打包根文件系统的能力。"

"`IMAGE_INSTALL` 就是安装列表？"阿凯问。

"对。`IMAGE_INSTALL` 是 **镜像安装列表（IMAGE_INSTALL）**，声明最终 rootfs 里要包含哪些包。`packagegroup-core-boot` 不是单个包，而是一个 **包组（Package Group）**。"

阿凯继续追踪这个包组。

```bash
# 查看 boot 包组的内容
cat ~/workspace/poky/meta/recipes-core/packagegroups/packagegroup-core-boot.bb
```

输出（关键行）：

```bitbake
SUMMARY = "Minimal boot requirements"
DESCRIPTION = "The minimal set of packages required to boot the system"

inherit packagegroup

# Distro can override the following VIRTUAL-RUNTIME providers:
VIRTUAL-RUNTIME_dev_manager ?= "udev"
VIRTUAL-RUNTIME_keymaps ?= "keymaps"

EFI_PROVIDER ??= "grub-efi"

SYSVINIT_SCRIPTS = "${@bb.utils.contains('MACHINE_FEATURES', 'rtc', '${VIRTUAL-RUNTIME_base-utils-hwclock}', '', d)} \
                    modutils-initscripts \
                    ${VIRTUAL-RUNTIME_initscripts} \
                   "

RDEPENDS:${PN} = "\
    base-files \
    base-passwd \
    ${VIRTUAL-RUNTIME_base-utils} \
    ${@bb.utils.contains(\"DISTRO_FEATURES\", \"sysvinit\", \"${SYSVINIT_SCRIPTS}\", \"\", d)} \
    ${@bb.utils.contains(\"MACHINE_FEATURES\", \"keyboard\", \"${VIRTUAL-RUNTIME_keymaps}\", \"\", d)} \
    ${@bb.utils.contains(\"MACHINE_FEATURES\", \"efi\", \"${EFI_PROVIDER} kernel\", \"\", d)} \
    netbase \
    ${VIRTUAL-RUNTIME_login_manager} \
    ${VIRTUAL-RUNTIME_init_manager} \
    ${VIRTUAL-RUNTIME_dev_manager} \
    ${VIRTUAL-RUNTIME_update-alternatives} \
    ${MACHINE_ESSENTIAL_EXTRA_RDEPENDS}"

RRECOMMENDS:${PN} = "\
    ${VIRTUAL-RUNTIME_base-utils-syslog} \
    ${MACHINE_ESSENTIAL_EXTRA_RRECOMMENDS} \
    ${@bb.utils.contains(\"DISTRO_FEATURES\", \"sysvinit\", \"init-ifupdown\", \"\", d)} \
    ${@bb.utils.contains(\"DISTRO_FEATURES\", \"sysvinit pni-names\", \"ifupdown\", \"\", d)} \
    "
```

"包组把一堆相关包组织成一个逻辑组，方便 image recipe 批量引用。"达哥说，"`${VIRTUAL-RUNTIME_base-utils}` 通常指向 `busybox`，`${VIRTUAL-RUNTIME_dev_manager}` 指向设备管理器。注意 Scarthgap 版本用了很多 `VIRTUAL-RUNTIME_*` 虚包和条件依赖——`sysvinit` 相关的脚本只在 `DISTRO_FEATURES` 包含 `sysvinit` 时才加入，`efi` 相关组件只在 `MACHINE_FEATURES` 包含 `efi` 时才加入。"

阿凯用 `bitbake -e` 查看最终生效的 `IMAGE_INSTALL`。

```bash
# 确保已初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 查看最终 IMAGE_INSTALL 的构成
bitbake -e core-image-minimal | grep ^IMAGE_INSTALL=
```

输出（示例，具体值因版本略有不同，以本地 `bitbake -e` 实际结果为准）：

```text
IMAGE_INSTALL="packagegroup-core-boot "
```

> **💡 提示**：示例输出以本地 `bitbake -e` 实际结果为准，此处保留尾部空格仅供参考。

"所以 `core-image-minimal` 不是'一个配方编译出所有东西'，而是'声明要安装哪些包，BitBake 自动解决依赖'。"阿凯总结。

"正是。"达哥说，"等你写 `tiger-image-minimal.bb` 的时候，就在 `IMAGE_INSTALL` 里加上 tiger 需要的启动链工具、MTD 工具、调试工具，而不是把所有业务应用都塞进来。"

### 2.6 踩坑实录

#### 2.6.1 踩坑 1：把所有东西都塞进 `meta-tiger`

阿凯在 2.3 节已经跟达哥讨论过这个问题，但那个教训值得再落一次地。

公司上代 Buildroot 项目就是把所有东西混在一起——bootloader、kernel、Web 应用、配置脚本、产品 logo 全塞在一个 `package/` 目录里。结果是：OTA 升级包体积巨大，每次升级都包含未改动的应用程序；license 审计时手动梳理了据说整整两周。

> **🔥 重要**：`meta-tiger` 不是"项目大杂烩"。它的边界是 BSP：让板子启动、让内核跑起来、提供基础根文件系统。业务应用、Web 服务、第三方闭源 SDK 应该由应用团队独立维护，BSP 只提供运行环境。

#### 2.6.2 踩坑 2：混淆 MACHINE 配置和 DISTRO 配置

阿凯在查看 `qemuarm64.conf` 时，发现里面也有 `DISTRO_FEATURES` 相关的引用，误以为 MACHINE 配置可以覆盖发行版策略。他尝试在 MACHINE 配置里写 `DISTRO_FEATURES += "systemd"`，被达哥拦下。

"`MACHINE_FEATURES` 描述硬件特性，比如 `apm`、`usbhost`、`screen`。`DISTRO_FEATURES` 描述系统策略，比如 `systemd`、`pulseaudio`、`wayland`。"达哥解释，"两者在 BitBake 中合并为最终生效的 feature 集合，但职责边界清晰：MACHINE 不该替 DISTRO 做策略决定。"

阿凯不服气，真的在 `qemuarm64.conf` 末尾加了一行：

```bash
# 阿凯真的在 qemuarm64.conf 末尾加了一行 DISTRO_FEATURES += "systemd"
# 先展示他改的那一行
grep "DISTRO_FEATURES" ~/workspace/poky/meta/conf/machine/qemuarm64.conf
```

输出（假设阿凯添加的行）：

```text
# ... (省略原有内容)
DISTRO_FEATURES += "systemd"
```

然后跑 `bitbake -e` 验证：

```bash
# 确保已初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 查看 DISTRO_FEATURES 的变化
bitbake -e core-image-minimal | grep ^DISTRO_FEATURES=
```

输出（示例，具体值因版本略有不同）：

```text
DISTRO_FEATURES="... systemd ..."
```

"你看，bitbake 不会报错，变量确实被改了。"达哥说，"但这不是' MACHINE 配置生效'，而是' MACHINE 越界替 DISTRO 做了决定'。三个月后你换了一个 Distro，`systemd` 还在，因为 MACHINE 里硬编码了。"

阿凯意识到问题，赶紧撤销修改：

```bash
# 撤销刚才的越界修改
sed -i '/DISTRO_FEATURES += "systemd"/d' ~/workspace/poky/meta/conf/machine/qemuarm64.conf
```

然后重新验证：

```bash
# 确保已初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 正确做法：把 init 系统策略留给 Distro 配置
bitbake -e core-image-minimal | grep ^DISTRO_FEATURES=
```

输出（示例，具体值因版本略有不同，以本地实际输出为准）：

```text
DISTRO_FEATURES="acl alsa bluetooth debuginfod ext2 ipv4 ipv6 pcmcia usbgadget usbhost wifi xattr nfs zeroconf pci 3g nfc x11 vfat seccomp opengl ptest multiarch wayland vulkan sysvinit pulseaudio gobject-introspection-data ldconfig"
```

> **⚠️ 注意**：写 `tiger-aarch64.conf` 时，只声明 tiger 这块板子有什么硬件；init 系统、网络策略、显示框架等属于 Distro 策略，留到 `tiger-distro.conf` 里决定。

### 2.7 本章小结

阿凯把今天学到的东西在脑子里过了一遍：

- **2.1**：tiger 的每个硬件组件在 Poky 里都有对应位置，通用外设大多已存在，重点是用 **内核设备树（KERNEL_DEVICETREE）** 把 tiger 的设备树指给内核。
- **2.2**：启动链的每个阶段都会物化为 `tmp/deploy/images/<MACHINE>/` 下的文件；`PREFERRED_PROVIDER` 决定 BitBake 用哪个 recipe 提供虚包。
- **2.3**：BSP 层和应用层必须分开，否则 OTA、合规、维护都会失控。
- **2.4**：Yocto 项目通过 layer 叠加组织元数据，`meta-tiger` 将覆盖/扩展下层默认 MACHINE 配置。
- **2.5**：镜像配方是声明式的，`IMAGE_INSTALL` 决定 rootfs 内容，包组用于批量组织相关包。

他把这些内容汇总成一张项目全景地图。

**Fig-2-5 项目全景地图**

```text
┌─────────────────────────────────────────────────────────────┐
│  Application 层（不进 meta-tiger）                            │
│  网关应用 / Web 配置 / 业务 SDK                               │
├─────────────────────────────────────────────────────────────┤
│  OS / BSP 层（进 meta-tiger）                                 │
│  Linux Kernel / 设备树 / 根文件系统 / DISTRO 策略             │
├─────────────────────────────────────────────────────────────┤
│  Firmware / Bootloader 层（进 meta-tiger）                    │
│  TF-A (BL2/BL31) / U-Boot (BL33)                            │
├─────────────────────────────────────────────────────────────┤
│  硬件：Cortex-A53 / DDR / NAND / SPI NOR / UART / RTC / ...  │
├─────────────────────────────────────────────────────────────┤
│  仓库组织：linux-tiger / qemu-tiger / tf-a-tiger /            │
│           u-boot-tiger（开发态）+ meta-tiger（集成态）        │
└─────────────────────────────────────────────────────────────┘
```

达哥看了一眼："地图有了。下一步就是动手搭 `meta-tiger` 这个 layer。"

后续任务清单：

- **task 04 / chapter 3**：创建 `meta-tiger` 的骨架，让 BitBake 认识它。
- **task 05 / chapter 4**：让 QEMU 长出 tiger 这块板。
- **task 06 / chapter 5**：写第一份 MACHINE 配置。

最后，阿凯在 `poky` 仓库里打了一个 tag，标记本章结束时的状态。

```bash
# 在 poky 仓库标记本章终点
cd ~/workspace/poky
git tag chapter2
```

> **💡 提示**：本章只"看"和"理解"，没有创建 `meta-tiger`、没有写 `tiger-aarch64.conf`、没有集成任何启动链组件。所有动手创建工作从下一章开始。
