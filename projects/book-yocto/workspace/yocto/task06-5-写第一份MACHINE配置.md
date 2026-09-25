## 5 写第一份 MACHINE 配置

周五上午，阿凯工位。终端历史里还留着昨天冒烟跑通的那条命令——从 qemu-system-native 的 sysroot 里起出来的 `qemu-system-aarch64 -machine tiger -display none -S`，QEMU 已经认识 tiger 这块板了。chapter 4 的 tag 也打完了。

达哥端着杯子过来："QEMU 准备好了，但 Yocto 还不知道我们要给谁构建。今天写 `tiger-aarch64.conf`，让 MACHINE 这一层立起来——尽管我们还没有内核、没有 bootloader。"

"没有内核也能写 MACHINE 配置？"阿凯愣了一下，"构建不会直接炸吗？"

"会炸。"达哥答得很干脆，"今天就让它炸。炸出来的报错，就是你要学会读的东西。MACHINE 配置立没立住，判断标准不是构建通过——是报错精确地只剩'缺内核'这一件事。"

阿凯还想问，达哥摆摆手，给了三个路标："先想清楚 MACHINE 这一层管什么、不管什么；再搞清楚 BitBake 怎么找到这个文件；然后一行行写，每写一行都要能回答'这行回答什么问题'。"

"写完呢？"

"写完你自己会想做一件事。"达哥走了两步又回头，"到时候那个'一切正常'，别急着信——先查。"

### 5.1 MACHINE、DISTRO、IMAGE：三张桌子各管什么

#### 5.1.1 把三张桌子合拢成一张图

chapter 2 读项目的时候，阿凯分别见过这三层：`qemuarm64.conf` 是 Machine 配置，`poky.conf` 是 Distro 配置，`core-image-minimal.bb` 是镜像配方。当时是分开读的，今天动笔之前，得先把它们合拢成一张图。

达哥打了个比方：这三层像三张桌子，各管一摊事，谁也别往别人桌上伸手。

- **MACHINE 这张桌子**管"硬件事实"：CPU 是什么架构、串口在哪、板子有什么外设。桌上的变量是 `MACHINE_FEATURES`、`KERNEL_IMAGETYPE`、`SERIAL_CONSOLES`，以及今天的主角——调优参数。
- **DISTRO 这张桌子**管"软件策略"：init 系统选谁、启用哪些系统级特性。桌上的变量是 `DISTRO_FEATURES`。
- **IMAGE 这张桌子**管"镜像里装什么"：桌上的变量是 `IMAGE_INSTALL`、`IMAGE_FEATURES`。

**Fig-5-1 三层配置模型：变量各回各家**

```text
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  MACHINE 配置    │  │  Distro 配置     │  │  镜像配方        │
│ tiger-aarch64   │  │ poky.conf       │  │ core-image-     │
│     .conf       │  │ （今天不动）      │  │ minimal.bb      │
├─────────────────┤  ├─────────────────┤  ├─────────────────┤
│ MACHINE_FEATURES│  │ DISTRO_FEATURES │  │ IMAGE_INSTALL   │
│ TUNE 变量族      │  │ （系统策略）      │  │ IMAGE_FEATURES  │
│ KERNEL_IMAGETYPE│  │                 │  │ IMAGE_FSTYPES*  │
│ SERIAL_CONSOLES │  │                 │  │                 │
│ QB_* 变量族      │  │                 │  │                 │
└────────┬────────┘  └────────┬────────┘  └────────┬────────┘
         │                    │                    │
         └────────────┬───────┴────────────────────┘
                      ▼
              BitBake 解析合并
              （bitbake -e 可见最终值）
```

* `IMAGE_FSTYPES` 由 MACHINE 配置给出主要取值，最终落在镜像产物上，跨两张桌子，所以标了星号。

#### 5.1.2 用 bitbake -e 验证三桌不越界

光说不算，验证一下。当前构建环境还是 `MACHINE = "qemuarm64"`（chapter 1 起就没动过），三个变量各查一次最终值。

```bash
# 初始化构建环境（每次新开终端都需要）
cd ~/workspace/poky
source oe-init-build-env ../build

# 三张桌子各看一个变量的最终合并值
bitbake -e core-image-minimal | grep -E "^(MACHINE_FEATURES|DISTRO_FEATURES|IMAGE_INSTALL)="
```

输出（关键行，以本地实际输出为准）：

```text
MACHINE_FEATURES="alsa bluetooth usbgadget screen vfat"
DISTRO_FEATURES="acl alsa bluetooth debuginfod ext2 ipv4 ipv6 pcmcia usbgadget usbhost wifi xattr nfs zeroconf pci 3g nfc x11 vfat seccomp opengl ptest multiarch wayland vulkan sysvinit pulseaudio gobject-introspection-data ldconfig"
IMAGE_INSTALL="packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"
```

`MACHINE_FEATURES` 来自 `qemuarm64.conf` 和它 require 的 `qemu.inc`（chapter 2 读过），`DISTRO_FEATURES` 来自 `poky.conf`，`IMAGE_INSTALL` 来自 `core-image-minimal.bb`——最终合并值里，每个变量都只有自己那张桌子写进去的内容，没有谁越界。

> **⚠️ 注意**：越界的样子 chapter 2 的踩坑实录已经演示过——在 machine 配置里塞 `DISTRO_FEATURES += "systemd"`，BitBake 不拦你，系统策略就这么被硬件配置悄悄改了。那不是假设，是真踩过的坑。本章在 `tiger-aarch64.conf` 里写每一行之前，先问自己一句：这行回答的是硬件问题还是软件策略问题？后者不属于这个文件。

#### 5.1.3 今天只动 MACHINE 这张桌子

"所以今天的活很清楚。"达哥说，"DISTRO 用现成的 poky，镜像用现成的 core-image-minimal，你只写 MACHINE 这一层。等 chapter 11 做 tiger 自己的 DISTRO 时，才轮到第二张桌子。"

阿凯在本子上写下今天要回答的问题清单：CPU 调优（TUNE）、内核产物格式、串口、机器能力、镜像格式、runqemu 参数、内核提供者。七个问题，一个文件。

### 5.2 BitBake 怎么找到 tiger-aarch64.conf

#### 5.2.1 从 `MACHINE = "tiger-aarch64"` 出发

动笔之前先回答一个机制问题：在 `local.conf` 里写下 `MACHINE = "tiger-aarch64"` 之后，BitBake 去哪儿找对应的配置文件？

答案是约定加搜索路径。BitBake 拿到 `MACHINE` 的值后，会沿 BBPATH（chapter 3 写 `layer.conf` 时引入的 BitBake 搜索路径）逐个目录查找 `conf/machine/<MACHINE 名>.conf`——`BBPATH .= ":${LAYERDIR}"` 把每个 layer 自己的顶层目录追加进搜索路径。`meta-tiger` 注册进构建之后，`~/workspace/meta-tiger` 就在 `BBPATH` 里，所以 `~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf` 这个位置天然能被找到。

**Fig-5-2 MACHINE 配置的查找链**

```text
local.conf: MACHINE = "tiger-aarch64"
        │
        ▼
BitBake 沿 BBPATH 逐目录查找 conf/machine/tiger-aarch64.conf
        │
        ├─ poky/meta/conf/machine/          （qemuarm64.conf 等在这里）
        ├─ poky/meta-poky/conf/machine/     （没有）
        ├─ poky/meta-yocto-bsp/conf/machine/（没有）
        └─ meta-tiger/conf/machine/         ← 我们要写的文件放这里
        │
        ▼
找到后解析该文件；文件里的 require 再沿 BBPATH 找被包含文件
```

> **💡 提示**：路径里的 `conf/machine/` 是硬约定，不能改名换位置。BSP layer 的 machine 配置都长在这个位置，翻任何一家厂商的 layer 都一样。

#### 5.2.2 require 与 include：找不到，报不报错

machine 配置几乎不会单文件写完——CPU 调优、QEMU 公共参数这类内容通常放在公共包含文件里，用包含指令引进来。BitBake 有两条 **文件包含指令（require / include）**：`require` 找不到目标文件时立即报错，`include` 找不到时静默跳过。

区别就在"找不到"这一种情况下，但用法因此不同：

- 公共调优文件、公共 inc 文件——没有它这个配置就不成立，用 `require`，让它找不到就喊出来。
- 可选的本地覆盖文件（比如厂商预留的 `machine-extra.conf`）——有就叠加、没有也正常，才用 `include`。

本章只用 `require`。chapter 2 读 `qemuarm64.conf` 时见过的那两行 `require conf/machine/include/...`，走的就是这个机制：沿 `BBPATH` 找到被包含文件，原地展开。

#### 5.2.3 先炸一次：conf 不存在时的报错

机制讲完了，达哥却给了个奇怪的第一步："先把 `local.conf` 的 `MACHINE` 改成 `tiger-aarch64`，直接构建。"

"文件还没写呢。"

"对，就是要看没写的时候它说什么。"

按规范，这个修改标注为**替换**：

```bitbake
# 文件路径：~/workspace/build/conf/local.conf
# 将 MACHINE = "qemuarm64" 一行替换为（DL_DIR、SSTATE_DIR 等其余行保持不变）：
MACHINE = "tiger-aarch64"
```

然后构建：

```bash
# 初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# MACHINE 配置文件还不存在，直接构建（这是"预期中的错误"）
bitbake core-image-minimal
```

输出（关键行）：

```text
ERROR:  OE-core's config sanity checker detected a potential misconfiguration.
    Either fix the cause of this error or at your own risk disable the checker (see sanity.conf).
    Following is the list of potential problems / advisories:

    MACHINE=tiger-aarch64 is invalid. Please set a valid MACHINE in your local.conf, environment or other configuration file.
```

这条报错值得读仔细：构建一个任务都没跑，就死在了**配置体检**阶段——BitBake 启动时会先检查基本配置是否自洽，`MACHINE` 指向一个沿 `BBPATH` 找不到的文件，直接判 invalid。这正好给 5.2.1 的查找链做了个反向证明：不是"先解析再看在不在"，而是"找不到就连解析的资格都没有"。

"所以 `conf/machine/tiger-aarch64.conf` 这个文件，名字和路径都不是我们选的。"阿凯在查找链图上画了个圈，"是 `MACHINE` 的值加约定推出来的。"

"现在可以写它了。"达哥说。

### 5.3 给 Cortex-A53 找对调优参数：TUNE

#### 5.3.1 达哥只给一句话

文件的第一行写什么，达哥没直接说。他只留下一句："tiger 的 CPU 是 Cortex-A53。你去找 cortexa53 的 tune 文件，看完告诉我里面定义了什么。"

阿凯自己摸索。chapter 2 读 `qemuarm64.conf` 时见过它 require 的是 `tune-cortexa57.inc`，那 cortexa53 的对应文件应该也在同一个目录家族里。

```bash
# 在 OE-Core 的 machine 包含文件目录里找 cortexa53 的 tune 文件
find ~/workspace/poky/meta/conf/machine/include -name "*cortexa53*"
```

输出（以本地实际输出为准）：

```text
/home/<your-username>/workspace/poky/meta/conf/machine/include/arm/armv8a/tune-cortexa53.inc
```

找到了，打开看内容：

```bash
# 查看 cortexa53 tune 文件的关键定义
cat ~/workspace/poky/meta/conf/machine/include/arm/armv8a/tune-cortexa53.inc
```

输出（关键行，以本地实际输出为准）：

```text
# ... (省略文件头注释)
DEFAULTTUNE ?= "cortexa53"

TUNEVALID[cortexa53] = "Enable Cortex-A53 specific processor optimizations"
TUNE_CCARGS .= "${@bb.utils.contains('TUNE_FEATURES', 'cortexa53', ' -mcpu=cortex-a53', '', d)}"

require conf/machine/include/arm/arch-armv8a.inc
# ... (省略 TUNE_FEATURES 变体与 PACKAGE_EXTRA_ARCHS 等配套行)
TUNE_FEATURES:tune-cortexa53 = "aarch64 crc cortexa53"
```

阿凯逐行读完，汇报给达哥："开头 `DEFAULTTUNE ?= "cortexa53"`，弱赋值——意思是没人点名调优集时默认用 cortexa53 这一套，我们 require 这个文件就等于点了名，这行先混个脸熟。`TUNEVALID[cortexa53]` 像是给这个调优集登记一句说明文字。第三行我看懂一半：`${@...}` 这种写法 chapter 2 读 `core-image-minimal` 时扫到过，当时没人讲——按字面猜是内嵌了一段 Python：'如果 TUNE_FEATURES 里有 cortexa53，交叉编译参数就追加 -mcpu=cortex-a53'。往下 require 了 `arch-armv8a.inc`，ARMv8-A 架构的公共定义。最后 `TUNE_FEATURES:tune-cortexa53` 不绕弯，直接写死三个特性标记：aarch64、crc、cortexa53。"

"猜得对。"达哥说，"`${@...}` 就是内联 Python 展开，`bb.utils.contains` 是查'某个特性在不在列表里'的常用工具函数——在，返回前一个串；不在，返回空。这行你的读法成立。"他顿了顿，"漏了一个问题。`require` 套 `require`，链有多长？"

"两层。我的 conf require tune 文件，tune 文件 require 架构公共文件——都在 `BBPATH` 上，所以写相对路径就行。"阿凯说，"而且按 5.2.2 的说法，这条链上每一环都必须用 `require`：哪一环找不到，配置就不成立，应该当场报错。"

达哥点头："调优这条线你摸到门了。下面说它为什么重要。"

#### 5.3.2 TUNE：一个变量族，一条因果链

**体系调优（TUNE）** 是 BitBake 的一个变量族——`TUNE_FEATURES`、`TUNE_CCARGS` 等一组以 TUNE 开头的变量，控制交叉编译的目标架构优化选项：用什么指令集、针对哪个微架构调度、浮点 ABI 怎么选。tiger 是 Cortex-A53，对应的就是刚才那个文件定义的这一套。

TUNE 之所以要写进 machine 配置的第一段，是因为它牵着一条很长的因果链：

```text
TUNE_FEATURES 变化
   → 交叉编译器 flags 变化（-mcpu 等）
      → 所有 target 配方的任务签名变化
         → Sstate 缓存大面积失效
            → glibc、gcc 等工具链级组件全盘重编
```

换句话说，**TUNE 是工具链级变量**：改它一行的代价，是整条构建从头再来。平时无感，是因为没人会去动它；真动错了，感受非常直观——5.6 的踩坑实录会专门演示一次，这里先把因果链立住。

> **⚠️ 注意**：TUNE 写错的危险在于它不报错。require 了一个不对但存在的 tune 文件，构建照样往下走，只是编译参数悄悄变了、缓存悄悄全废。查证法只有一个：改动前后各跑一次 `bitbake -e ... | grep ^TUNE_FEATURES=` 对比。

#### 5.3.3 落下第一行：require

创建文件，写下第一段：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（新建文件，写入以下内容）
# meta-tiger/conf/machine/tiger-aarch64.conf
# tiger 开发板（ARM Cortex-A53）的 Yocto MACHINE 配置

# ---- 调优：这块板子的 CPU 是什么 ----
# 引入 Cortex-A53 调优参数（决定交叉编译的 -mcpu 等 flags；改错代价见 5.6.1）
require conf/machine/include/arm/armv8a/tune-cortexa53.inc
```

写完立即验证——这是本章的节奏：写一段，`bitbake -e` 验一段。顺便说一句，刚才 5.2.3 的报错此刻应该已经消失，因为文件存在了、第一行能解析了。

```bash
# 验证 TUNE 变量族已随 require 生效
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e core-image-minimal | grep -E "^(TUNE_FEATURES|TUNE_ARCH)="
```

输出（关键行，以本地实际输出为准）：

```text
TUNE_FEATURES="aarch64 crc cortexa53"
TUNE_ARCH="aarch64"
```

`tune-cortexa53.inc` 这条 require 链两层展开后，目标架构锁定为 AArch64，特性集合里带上了 `cortexa53`。第一行落地。

### 5.4 一行行写出 tiger-aarch64.conf

#### 5.4.1 内核镜像与串口：这行回答"内核产物长什么样、控制台从哪进"

第二段回答两个问题。内核编译出来是什么格式？chapter 2 已经定过：AArch64 用未压缩的 `Image`，写进 `KERNEL_IMAGETYPE`。开机后控制台从哪个串口进？chapter 2 在 `qemuarm64.conf` 里见过 `SERIAL_CONSOLES`，这里正式落地——**串口控制台（SERIAL_CONSOLES）**，声明目标设备的串口控制台列表，格式是 `"波特率;设备路径"`，它会影响根文件系统里 `/etc/inittab` 或 systemd 的串口登录配置。

tiger 的串口是 PL011，对应设备节点 `ttyAMA0`（chapter 2 读项目时确认过），波特率按项目惯例 115200：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接上文追加）

# ---- 内核与串口 ----
# 内核编译产物格式：AArch64 为未压缩的 Image
KERNEL_IMAGETYPE = "Image"
# 串口控制台：tiger 串口为 PL011（设备节点 ttyAMA0），波特率 115200
SERIAL_CONSOLES = "115200;ttyAMA0"
```

验证：

```bash
# 验证内核镜像类型与串口配置已被解析
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e core-image-minimal | grep -E "^(KERNEL_IMAGETYPE|SERIAL_CONSOLES)="
```

输出：

```text
KERNEL_IMAGETYPE="Image"
SERIAL_CONSOLES="115200;ttyAMA0"
```

#### 5.4.2 机器能力与镜像格式：这行回答"板子有什么、产物打成什么包"

`MACHINE_FEATURES` chapter 2 学过：只陈述硬件事实，不替 Distro 做软件决策。对照 tiger 的外设清单逐项过一遍：PL011 串口——`serial`；PL031 实时时钟——`rtc`；没有屏幕、没有声卡、没有蓝牙，对照 `qemuarm64` 那串 `alsa bluetooth usbgadget screen vfat`，该删的都删；`vfat` 保留——将来的启动分区用 FAT 格式。`ext2` 不是从 qemuarm64 抄来的，是新加的一项：它声明这块板具备 ext2 系文件系统能力，下面 `IMAGE_FSTYPES` 要出的 ext4 镜像就走在这条线上，chapter 9 做 NAND 之前的启动分区也会用到。

`IMAGE_FSTYPES` 同理，先出最通用的两种包：`tar.bz2`（根文件系统打包）和 `ext4`（可挂载、可烧写的通用磁盘格式）。

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接上文追加）

# ---- 机器能力与镜像格式 ----
# 只陈述硬件事实：串口、RTC；不替 Distro 决定软件策略
MACHINE_FEATURES = "ext2 rtc serial vfat"
# 镜像输出先用通用格式；UBI 格式等 chapter 9 NAND 建好再加
IMAGE_FSTYPES += "tar.bz2 ext4"
```

> **📖 深入阅读**：tiger 的根文件系统最终要进 NAND Flash，走 UBI/UBIFS，对应 `IMAGE_FSTYPES` 里的 `ubi` 以及 `MKUBIFS_ARGS`、`UBINIZE_ARGS` 一组参数。这些等 chapter 9 把 NAND 建好再写——现在写了也没有对应的存储可烧，属于提前作答。

验证：

```bash
# 验证机器能力与镜像格式
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e core-image-minimal | grep -E "^(MACHINE_FEATURES|IMAGE_FSTYPES)="
```

输出（关键行）：

```text
MACHINE_FEATURES="ext2 rtc serial vfat"
IMAGE_FSTYPES=" tar.bz2 ext4"
```

"咦，`IMAGE_FSTYPES` 前面怎么多了个空格？"阿凯盯着输出，"而且我记得 `bitbake.conf` 里有个默认的 `tar.gz`，怎么没了？"

"两个观察都对，而且是同一个原因。"达哥说，"`bitbake.conf` 里那句 `IMAGE_FSTYPES ?= "tar.gz"` 是弱赋值——'没人设过才生效'。可我们的 `+=` 一动手，变量就算'已设置'了，`?=` 不再补位，`tar.gz` 就这么被顶掉了。前导那个空格，正是 `+=` 往空值上追加时留下的痕迹。"他补了一句，"弱赋值这个命门先混个印象，5.5.2 写占位行时还会再见面。"

#### 5.4.3 接上 runqemu：QB_* 落地

下一段是昨天的伏笔回收。chapter 4 的 4.6 节讲透过整条数据流：MACHINE 配置里的 `QB_*` 变量 → 构建镜像时落盘成 `qemuboot.conf` → runqemu 读它拼 QEMU 命令行。昨天 `runqemu tiger-aarch64` 跑不通，缺的拼图之一就是这里的 `QB_*` 取值。机制不再重讲，只落地配置行：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接上文追加）

# ---- runqemu 参数（机制见 chapter 4 的 4.6 节） ----
# QEMU 二进制与 machine 类型：Yocto 侧 tiger-aarch64 → QEMU 侧 -machine tiger
QB_SYSTEM_NAME = "qemu-system-aarch64"
QB_MACHINE = "-machine tiger"
# CPU 型号与板卡资源：Cortex-A53，4 核，内存按产品规格 1 GiB
QB_CPU = "-cpu cortex-a53"
QB_SMP ?= "-smp 4"
QB_MEM ?= "-m 1024"
# 内核命令行追加控制台参数；默认内核产物为 Image
QB_KERNEL_CMDLINE_APPEND = "console=ttyAMA0"
QB_DEFAULT_KERNEL = "Image"
```

逐行的回答：`QB_SYSTEM_NAME`——用哪个 QEMU 二进制；`QB_MACHINE`——`-M` 参数是什么，这一行就是 chapter 4 说的"命名约定"被亲手接上的地方：`tiger-aarch64` 映射到 `-machine tiger`；`QB_CPU` / `QB_SMP` / `QB_MEM`——CPU 型号、核数、内存，对照 tiger 的 Cortex-A53 4 核规格；`QB_KERNEL_CMDLINE_APPEND`——内核命令行追加 `console=ttyAMA0`，和上面的 `SERIAL_CONSOLES` 指向同一个串口，两边必须一致；`QB_DEFAULT_KERNEL`——runqemu 默认加载的内核文件名。存储相关的 `QB_ROOTFS_OPT` 等 chapter 9 存储方案定了再补。

注意 `QB_SMP` 和 `QB_MEM` 这两行用的是 `?=` 而不是 `=`——弱赋值：给出默认值，同时允许 `local.conf` 之类的地方覆盖。三档赋值（`=` / `?=` / `??=`）的完整语义 5.5.2 一次说清，这里先记住一句：弱赋值就是"可被覆盖的默认值"。

验证 `QB_MACHINE` 这一行的合龙：

```bash
# 验证 QB_* 已被解析（重点看 QB_MACHINE）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e core-image-minimal | grep -E "^(QB_SYSTEM_NAME|QB_MACHINE|QB_CPU|QB_MEM)="
```

输出：

```text
QB_SYSTEM_NAME="qemu-system-aarch64"
QB_MACHINE="-machine tiger"
QB_CPU="-cpu cortex-a53"
QB_MEM="-m 1024"
```

#### 5.4.4 写一段验一段

到这里文件已有四段。把"写一段验一段"的节奏固定下来，是因为 machine 配置的每个错误都会流向很远的地方——TUNE 错了重编全盘，串口错了开机没控制台，QB 错了 runqemu 拼错命令行。`bitbake -e` 查一次十秒钟，是全程最便宜的检查点。

当前文件的逐行职责一览：

| 配置行 | 回答的问题 |
|--------|-----------|
| `require ... tune-cortexa53.inc` | CPU 是什么？编译参数怎么调？ |
| `KERNEL_IMAGETYPE = "Image"` | 内核产物是什么格式？ |
| `SERIAL_CONSOLES = "115200;ttyAMA0"` | 控制台从哪个串口进？ |
| `MACHINE_FEATURES = "ext2 rtc serial vfat"` | 这块板子有什么？ |
| `IMAGE_FSTYPES += "tar.bz2 ext4"` | 镜像打成什么包？ |
| `QB_SYSTEM_NAME` / `QB_MACHINE` 等 | runqemu 怎么启动这块板？ |

还差最后一个问题没回答：内核谁来编译？

### 5.5 还差一个提供者：PREFERRED_PROVIDER 与 PREFERRED_VERSION 占位

#### 5.5.1 阿凯主动构建：预期的报错没有来

"写完了。"阿凯活动了一下手腕，"跑一次试试？"

"试。"达哥这次很痛快，"先别写任何 PROVIDER 相关的行。也别真跑——加 `-n` 干跑，让它只解析排队不执行。你先说说，你预期看到什么。"

"预期看到报错。"阿凯胸有成竹——`core-image-minimal` 要内核，poky 自带那几个 `linux-yocto` 却不一定认这块新板子。光凭印象不行，他当场把它们的兼容名单翻出来核实：

```bash
# 查看 poky 自带内核配方的 COMPATIBLE_MACHINE 兼容名单
cd ~/workspace/poky/meta/recipes-kernel/linux
grep -n "COMPATIBLE_MACHINE" linux-yocto*.bb
```

输出（关键行，以本地实际输出为准）：

```text
linux-yocto-dev.bb:47:COMPATIBLE_MACHINE = "^(qemuarmv5|qemuarm|qemuarm64|qemux86|qemuppc|qemumips|qemumips64|qemux86-64|qemuriscv32|qemuriscv64|qemuloongarch64)$"
linux-yocto-rt_6.6.bb:37:COMPATIBLE_MACHINE = "^(qemux86|qemux86-64|qemuarm|qemuarmv5|qemuarm64|qemuppc|qemumips)$"
linux-yocto-tiny_6.6.bb:28:COMPATIBLE_MACHINE = "^(qemux86|qemux86-64|qemuarm64|qemuarm|qemuarmv5)$"
linux-yocto_6.6.bb:56:COMPATIBLE_MACHINE = "^(qemuarm|qemuarmv5|qemuarm64|qemux86|qemuppc|qemuppc64|qemumips|qemumips64|qemux86-64|qemuriscv64|qemuriscv32|qemuloongarch64)$"
```

四份配方，四张名单，清一色 qemu 系正则——没有一张带 `tiger-aarch64`。"没人提供内核，它总该喊出来吧。"

```bash
# 首次以 MACHINE=tiger-aarch64 干跑构建计划（-n：只解析和排计划，不执行任务）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -n core-image-minimal
```

输出（关键行，以本地实际输出为准）：

```text
NOTE: Tasks Summary: Attempted 3415 tasks of which 0 didn't need to be rerun and all succeeded.
```

"……没报错。"阿凯把终端往上翻了好几屏，"三千多个任务，全部通过计划。内核不是没人提供吗？"

达哥不问"哪里错了"，问的是另一句："计划里内核是谁？"

"内核不是没人提供吗？"

"没人**报错**，不等于没人**上岗**。"达哥说，"`virtual/kernel` 是个虚包，计划排到它的时候，总得落到一个具体配方头上。别猜，查——看那个配方的名字。"

```bash
# 查 virtual/kernel 实际落到哪个配方头上（看该配方的 PN）
bitbake -e virtual/kernel | grep ^PN=
```

输出：

```text
PN="linux-dummy"
```

`linux-dummy`？阿凯从没听过这个名字。先把它的配方文件找出来：

```bash
# 在 OE-Core 里定位 linux-dummy 的配方文件
find ~/workspace/poky/meta -name "linux-dummy.bb"
```

输出（以本地实际输出为准）：

```text
/home/<your-username>/workspace/poky/meta/recipes-kernel/linux/linux-dummy.bb
```

他打开文件逐行读完，背上有点凉：这是一份**空壳内核配方**——不下载源码、不打补丁、不编译任何镜像，唯一正经事就是声明自己能提供 `virtual/kernel`，给"不需要真内核"的构建（比如纯容器根文件系统）兜底。更要命的是它**不设 `COMPATIBLE_MACHINE`**——不设防，所以对任何 MACHINE 都"兼容"，永远不会被跳过。

"现在你明白我为什么让你先干跑了。"达哥说，"今天这个配置要是真跑完，镜像里的'内核'就是它——一个什么都不包含的空壳。报错一个不出现，假话一句不缺。"

阿凯把这句话记在了本子最顶上：**不报错的构建，比报错的构建更危险。**linux-dummy 本身没做错什么，它生来就是兜底用的；错的是我们——内核提供者这件事，我们一个字都还没声明。

"怎么破？"达哥问。

"显式认领。"阿凯说，"把 `virtual/kernel` 从 linux-dummy 手里拿走，写死成 `linux-tiger`。"

"写下来。"

#### 5.5.2 自己得出结论，写下占位行

这里还有一个时序问题要说透：`linux-tiger.bb` 这个配方**还不存在**——chapter 8 才会写它。今天写的是"占位"：先声明意图，让配置结构完整，顺便把 linux-dummy 从岗位上顶下来。

占位用 `??=` 弱赋值。`=` 是硬赋值，`?=` 是"没人设过才生效"，`??=` 更弱——解析全程最后兜底，任何地方（比如将来某个 distro 配置或本地覆盖文件）设过值就用别人的。声明默认意图、又给别人留覆盖余地，占位行用它最合适。

同时引入本章最后一个新变量：**优先版本（PREFERRED_VERSION）**，锁定配方的首选版本号。tiger 内核锁定 Linux 6.6 LTS（长期支持版本，版本锁定见全书版本表），写成 `6.6%` 的通配形式，将来 `linux-tiger_6.6.x.bb` 的任何小版本都匹配。

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接上文追加，文件至此完整）

# ---- 内核提供者占位 ----
# virtual/kernel 由 linux-tiger 提供；该配方 chapter 8 才出现，先用弱赋值占位，
# 让认领意图显式落在配置里（配方到位后无需再动本文件）
PREFERRED_PROVIDER_virtual/kernel ??= "linux-tiger"
# 锁定内核大版本为 6.6 LTS（版本锁定见全书版本表）
PREFERRED_VERSION_linux-tiger ??= "6.6%"
```

`tiger-aarch64.conf` 至此完整。全文件回顾：

```bitbake
# meta-tiger/conf/machine/tiger-aarch64.conf
# tiger 开发板（ARM Cortex-A53）的 Yocto MACHINE 配置

# ---- 调优：这块板子的 CPU 是什么 ----
# 引入 Cortex-A53 调优参数（决定交叉编译的 -mcpu 等 flags；改错代价见 5.6.1）
require conf/machine/include/arm/armv8a/tune-cortexa53.inc

# ---- 内核与串口 ----
# 内核编译产物格式：AArch64 为未压缩的 Image
KERNEL_IMAGETYPE = "Image"
# 串口控制台：tiger 串口为 PL011（设备节点 ttyAMA0），波特率 115200
SERIAL_CONSOLES = "115200;ttyAMA0"

# ---- 机器能力与镜像格式 ----
# 只陈述硬件事实：串口、RTC；不替 Distro 决定软件策略
MACHINE_FEATURES = "ext2 rtc serial vfat"
# 镜像输出先用通用格式；UBI 格式等 chapter 9 NAND 建好再加
IMAGE_FSTYPES += "tar.bz2 ext4"

# ---- runqemu 参数（机制见 chapter 4 的 4.6 节） ----
# QEMU 二进制与 machine 类型：Yocto 侧 tiger-aarch64 → QEMU 侧 -machine tiger
QB_SYSTEM_NAME = "qemu-system-aarch64"
QB_MACHINE = "-machine tiger"
# CPU 型号与板卡资源：Cortex-A53，4 核，内存按产品规格 1 GiB
QB_CPU = "-cpu cortex-a53"
QB_SMP ?= "-smp 4"
QB_MEM ?= "-m 1024"
# 内核命令行追加控制台参数；默认内核产物为 Image
QB_KERNEL_CMDLINE_APPEND = "console=ttyAMA0"
QB_DEFAULT_KERNEL = "Image"

# ---- 内核提供者占位 ----
# virtual/kernel 由 linux-tiger 提供；该配方 chapter 8 才出现，先用弱赋值占位，
# 让认领意图显式落在配置里（配方到位后无需再动本文件）
PREFERRED_PROVIDER_virtual/kernel ??= "linux-tiger"
# 锁定内核大版本为 6.6 LTS（版本锁定见全书版本表）
PREFERRED_VERSION_linux-tiger ??= "6.6%"
```

"等下。"阿凯指着最后两行，"`linux-tiger` 的配方还不存在。linux-dummy 被顶掉之后，`virtual/kernel` 岂不是又没人提供了？这回该炸了吧。"

"再跑一次，这次来真的——跟刚才那个'一切正常'对比着看。"达哥说。

#### 5.5.3 成功判据：报错出现，且只剩"缺内核"一件事

```bash
# 占位行写入后正式构建（这是"预期中的错误"，与 5.5.1 的"一切正常"对比着看）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake core-image-minimal
```

输出（关键行）：

```text
ERROR: Nothing PROVIDES 'virtual/kernel'
linux-yocto PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-rt PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-tiny PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-upstream PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-dev PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-dummy PROVIDES virtual/kernel but was skipped: PREFERRED_PROVIDER_virtual/kernel set to linux-tiger, not linux-dummy
```

报错终于来了。一行主消息，加六条 skipped 提示，信息都在里面。阿凯逐行读：

1. **第一行**：有个叫 `virtual/kernel` 的东西，没有任何配方提供它。`virtual/` 前缀——chapter 2 查过的虚包机制，"内核"这个功能名，需要某个具体配方来认领。
2. **中间五行**：poky 自带的五个内核候选——`linux-yocto`、`linux-yocto-rt`、`linux-yocto-tiny`、`linux-yocto-upstream`（linux-yocto 6.6 的纯上游 stable 变体）、`linux-yocto-dev`（这个才是跟踪上游最新开发的）——个个都 `PROVIDES virtual/kernel`，但它们的 `COMPATIBLE_MACHINE` 兼容名单里都没有 `tiger-aarch64`，整份配方被跳过（skipped）。
3. **最后一行**：`linux-dummy` 的死因和前五个不同——它不是不兼容，而是被我们的占位行顶掉了：`PREFERRED_PROVIDER_virtual/kernel` 已经指向 `linux-tiger`，轮不到它上岗。

这六条提示行来自解析期的跳过名单，两次构建里出现的顺序可能不一样——顺序没有语义，读的是**行集**：六条候选，两个死因，全都写在纸面上，没有一个静默通过。

"等下。"阿凯拿这份名单对着刚才的 grep 输出数了一遍，"多出一个 `linux-yocto-upstream`——刚才翻配方文件的时候，明明只有四份 `linux-yocto*.bb`。"

"它没有自己的 `.bb` 文件。"达哥一句话带过，"它是 `linux-yocto` 的一个变体，由原配方里一行 `BBCLASSEXTEND` 在解析时派生出来——所以按文件名 grep 找不到它，但候选名单里它确实独立占位。机制先不展开，记住它从哪来就行。"

"它还是没直接说'请去写 PREFERRED_PROVIDER'。"阿凯说，"新人看到第一行 `Nothing PROVIDES 'virtual/kernel'`，第一反应还是去搜 virtual/kernel 是什么——那就迷路了。"

"但至少现在，报错里有一行把答案的方向露出来了。"达哥指了指 linux-dummy 那一行，"死因收敛成一个：`linux-tiger` 的配方还没写。去确认一下认领声明本身生效了没有——注意，从现在开始 `bitbake -e core-image-minimal` 这条路断了，解析会撞上缺内核，什么都查不出来。查全局变量，用不带目标的 `bitbake -e`。"

阿凯查了：

```bash
# 查 virtual/kernel 的认领声明指向谁（占位行是否生效的直接证据；
# 必须用无目标的全局环境查询——带着 core-image-minimal 会因缺内核解析失败）
bitbake -e | grep "^PREFERRED_PROVIDER_virtual/kernel"
```

输出：

```text
PREFERRED_PROVIDER_virtual/kernel="linux-tiger"
```

"认领者已经写死在配置里了。"阿凯说，"和 5.5.1 比一比：那时候是零行报错加一个假内核悄悄上岗；现在是七行报错，每条死因都摆在明处，缺的只剩 chapter 8 的配方。报错从'不出现'变成'出现且死因唯一'——这才是进展。"

本章的成功判据至此全部达成：

1. 5.3 到 5.4 的逐段验证里（彼时占位行未写、`bitbake -e core-image-minimal` 尚能解析），TUNE、内核类型、串口、机器能力、镜像格式、QB 参数全部正确读出；
2. `bitbake core-image-minimal` 的报错只剩 `virtual/kernel` 一项，且死因唯一——认领者已在配置中（`linux-tiger`），缺的只是 chapter 8 的配方。

> **🔥 重要**：此刻构建不通是**设计使然**，不是失败。MACHINE 层的职责是把硬件事实说清楚，让后续每一章（TF-A、U-Boot、内核、存储）都往一个已经立住的框架里填。判据从来不是"今天能出镜像"，更不是"没有报错"——5.5.1 已经演示过，"没有报错"恰恰是 linux-dummy 在替你说谎。判据是：报错出现、读得懂、且只剩今天明知缺的那一件。

阿凯把读这类问题的套路记了下来——先查 `bitbake -e virtual/kernel` 看内核当前是谁（看到 linux-dummy 就要警惕），再用无目标的 `bitbake -e` 查 `PREFERRED_PROVIDER` 指向谁，最后确认指向的配方存不存在。三步，5.6 还会再用一次。

### 5.6 踩坑实录

这两件事发生在 5.3 到 5.5 之间——你前面看到的正确文件和干净的验证输出，都是改回来之后的样子。

#### 5.6.1 踩坑 1：require 错了 tune 文件，整个工具链悄悄重编

写 5.3 那行 require 时，阿凯的手比脑子快：刚读过 `qemuarm64.conf`，凭印象把 cortexa57 敲了进去——`require conf/machine/include/arm/armv8a/tune-cortexa57.inc`。文件存在，`require` 不报错，`bitbake -e` 也能解析。什么都没炸，所以他根本没意识到出事了。

我们来重演这个错误。探针用 glibc 这个工具链级组件——tune 的变化第一站就流到它身上。先看改错之前的基线：

```bash
# 改错之前（基线）：确认 glibc 继承的编译参数、特性集与包架构
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e glibc | grep -E "^(TUNE_CCARGS|TUNE_FEATURES|PACKAGE_ARCH)="
```

输出：

```text
TUNE_CCARGS=" -mcpu=cortex-a53+crc -mbranch-protection=standard"
TUNE_FEATURES="aarch64 crc cortexa53"
PACKAGE_ARCH="cortexa53"
```

```bash
# 错误示范：把 require 行改为 cortexa57（请勿模仿），改完后再次查询
sed -i 's/tune-cortexa53.inc/tune-cortexa57.inc/' \
    ~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf
bitbake -e glibc | grep -E "^(TUNE_CCARGS|TUNE_FEATURES|PACKAGE_ARCH)="
```

输出：

```text
TUNE_CCARGS=" -mcpu=cortex-a57+crc -mbranch-protection=standard"
TUNE_FEATURES="aarch64 crc cortexa57"
PACKAGE_ARCH="cortexa57"
```

一个词的差别，三个变量连动：编译 flags 变了（`-mcpu=cortex-a53` 变成 `-mcpu=cortex-a57`），特性标记变了，连包架构名都从 `cortexa53` 变成了 `cortexa57`。

"变量变了，可它还是不报错。"阿凯对比着两组输出，"真构建起来会怎样？"

"按 5.3.2 的因果链推一遍。"达哥说，"Sstate 缓存按任务签名索引，签名由这些变量参与计算。现在 flags、特性集、包架构全变了——所有 target 配方的任务签名跟着全变，chapter 1 起编出来的 glibc、gcc-cross 那批产物，对这个配置来说一夜之间全成了'别人编的东西'，一件都认不出来，只能从头重编。这在真实构建里就是几小时的代价——而且从头到尾，没有任何一行报错告诉你为什么突然变慢。"

中断演示，改回正确的 tune 文件：

```bash
# 修正：require 改回 cortexa53（这是必须完成的收尾，不能停留在错误状态）
sed -i 's/tune-cortexa57.inc/tune-cortexa53.inc/' \
    ~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf

# 验证变量族复原
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e glibc | grep -E "^(TUNE_FEATURES|PACKAGE_ARCH)="
```

输出：

```text
TUNE_FEATURES="aarch64 crc cortexa53"
PACKAGE_ARCH="cortexa53"
```

变量族复原，任务签名随之复原——原有缓存重新对得上号，一切回到正轨。

教训两条。一是 **TUNE 是工具链级变量**，动它之前先想清楚代价是"全盘重编"，排查"为什么突然变慢"时第一个查它。二是查证手法固定：改前改后各跑一次 `bitbake -e`，对比 `TUNE_CCARGS`、`PACKAGE_ARCH` 这一组变量，一个词的变化都无处遁形。这条命令值得贴在显示器边上。

#### 5.6.2 踩坑 2：注释掉占位行，报错消失——linux-dummy 回来了

第二个坑是个对照实验。5.5.2 写完占位行后，阿凯想知道那两行到底是不是非写不可——达哥听见了，说："想知道就注释掉，再构建一次。这次你先告诉我，你预期看到什么。"

阿凯的预期：按 5.5.1 的结论，占位行没了，linux-dummy 就没人顶了——报错会消失，假内核重新上岗，干跑又会"一切正常"。验证：

```bash
# 对照实验：注释掉认领行（错误示范状态，实验完必须恢复）
sed -i 's/^PREFERRED_PROVIDER_virtual\/kernel/#PREFERRED_PROVIDER_virtual\/kernel/' \
    ~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf

# 重新干跑构建计划
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -n core-image-minimal
```

输出（关键行，以本地实际输出为准）：

```text
NOTE: Tasks Summary: Attempted 3415 tasks of which 0 didn't need to be rerun and all succeeded.
```

报错果然消失了。再查一次内核是谁，坐实上岗者：

```bash
# 查 virtual/kernel 现在又落到谁头上
bitbake -e virtual/kernel | grep ^PN=
```

输出：

```text
PN="linux-dummy"
```

预判全中。这个对照实验把 5.5 的结论钉死了：**占位行不是写给报错的装饰品，它是一个开关**——写上它，"静默说谎"变成"显式报错"；拿掉它，报错消失，但问题不是被解决了，而是被藏回了 linux-dummy 的空壳里。报错消失从来不等于问题解决，在 Yocto 里有时甚至相反。

读这类问题固定三步，5.5.3 记过一次，这里钉死：

1. `bitbake -e virtual/kernel | grep ^PN=`——查内核当前是谁，看到 `linux-dummy` 就要警惕：它意味着没人显式认领；
2. `bitbake -e | grep "^PREFERRED_PROVIDER_virtual/kernel"`——查认领声明指向谁（注意不带构建目标，带着 `core-image-minimal` 会解析失败）；
3. 确认指向的配方存不存在——存在是环境问题，不存在就是该写配方或该写占位。

> **💡 提示**：这类报错还有另一种形态：当环境里同时存在多个都能提供 `virtual/kernel` 的配方、又没人写 `PREFERRED_PROVIDER` 时，BitBake 会把候选配方列成一张清单让你选。形态不同，读法相同——清单再长，答案都是那一行 `PREFERRED_PROVIDER`。

恢复占位行，实验收尾：

```bash
# 恢复：取消注释（必须完成的收尾）
sed -i 's/^#PREFERRED_PROVIDER_virtual\/kernel/PREFERRED_PROVIDER_virtual\/kernel/' \
    ~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf

# 确认占位行已恢复
grep "^PREFERRED_" ~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf
```

输出：

```text
PREFERRED_PROVIDER_virtual/kernel ??= "linux-tiger"
PREFERRED_VERSION_linux-tiger ??= "6.6%"
```

"那两行不是写给报错的。"阿凯合上本子，"注释掉、写回去，报错来了又走——它们是写给 chapter 8 的，也是写给读配置的人的：意图落在纸面上，假话才没有藏身的地方。"

达哥嗯了一声，算是给这一天收了尾。

### 5.7 本章小结

一天结束，达哥早上留的三个路标全部走完：

- **5.1**：三层配置各管一摊——MACHINE 管硬件事实，DISTRO 管软件策略，IMAGE 管装什么；本章只动 MACHINE 这张桌子。
- **5.2**：`MACHINE = "tiger-aarch64"` + `conf/machine/` 硬约定 + `BBPATH` 搜索，推出文件的唯一合法位置；`require` 找不到就报错，`include` 静默跳过；conf 不存在时构建死在 sanity 体检阶段。
- **5.3**：TUNE 是工具链级变量族，`require` 一个 tune 文件就锁定了整条交叉编译参数链；改错不报错，但代价是全盘重编。
- **5.4**：tiger-aarch64.conf 逐段落地，每行都能回答"这行回答什么问题"；`QB_MACHINE = "-machine tiger"` 把 chapter 4 的命名约定亲手接上。
- **5.5**：内核提供者的真相——不写认领行，linux-dummy 静默顶岗、连报错都没有；`PREFERRED_PROVIDER_virtual/kernel ??= "linux-tiger"` 把"静默说谎"掰成"显式报错"，死因收敛到"缺 chapter 8 的配方"一件；构建不通是设计使然。
- **5.6**：两个坑互为镜像——TUNE 改错"不报错但全盘签名作废"，占位行拿掉"报错消失但假内核回归"；查证都靠 `bitbake -e`。

本章产出清单：

- `meta-tiger` 新增 `conf/machine/tiger-aarch64.conf`：Yocto 侧正式认识 tiger-aarch64 这台机器。
- 一套可复用的查证节奏：**写一段配置，`bitbake -e` 验一段**。

提交并打 tag。本章唯一被修改的仓库是 `meta-tiger`：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Add tiger-aarch64 machine configuration"
git tag chapter5
```

> **⚠️ 注意**：tag 归属延续前两章的惯例——`meta-tiger` 打 `chapter5`；`poky` 保持原样不打 tag；`qemu-tiger` 本章未涉及。`build/conf/local.conf` 里 `MACHINE` 的修改属于本地构建环境，不进任何仓库。

后续任务清单：

- **task 07 / chapter 6**：集成 TF-A——启动链的 BL2/BL31 要进构建了。今天学会的"配方提供某产物 + 配置声明使用谁"这套模式，TF-A 会立刻再用一遍；而 TUNE 与工具链的关系也会有个有趣的对照——TF-A 是裸机代码，不走 target 工具链。
- 远景：task 08 集成 U-Boot，task 09 写出 `linux-tiger.bb`，把今天占位的 `PREFERRED_PROVIDER` 变成真配方，5.5 的报错到那时才真正清零。

达哥下班前路过，扫了一眼屏幕上的 tag："MACHINE 立住了。下周一开始往启动链上走——板子该'上电'了。"

---

**延伸阅读**

1. Yocto BSP 开发手册，"Board Support Packages (BSP)" 章节（MACHINE 配置的组织与约定）：https://docs.yoctoproject.org/5.0/bsp-guide/
2. Yocto 变量术语表，MACHINE / MACHINE_FEATURES / SERIAL_CONSOLES / KERNEL_IMAGETYPE / IMAGE_FSTYPES / PREFERRED_PROVIDER / PREFERRED_VERSION / QB_* 条目：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
3. BitBake 用户手册，require / include 指令与弱赋值（`?=` / `??=`）语法：https://docs.yoctoproject.org/bitbake/2.8/
