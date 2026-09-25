## 7 集成 U-Boot：让板子"会启动"

周二早上，阿凯工位。白板上的启动链图还挂着，BL31 那一格旁边是达哥昨天下班前写的一行字：启动链，下半段已通电。再往下，BL33 那格还是空的。

达哥端着杯子过来，指着那个空格："昨天日志停在哪？"

"等 BL33。"阿凯答。

"今天把它填上。"达哥说，"U-Boot 这个东西规矩多，defconfig、dts、env、boot script，每一样都要管。"

他在白板边上写了三个路标："第一，先定配方从哪来——这次的问题和昨天一样，答案不一样。第二，把四样规矩逐个落位。第三，接线、回填、单跑。"

阿凯翻开昨天记的笔记。chapter 6 集成 TF-A，抄的是 meta-arm 的现成配方，他记得自己数过行数——"这次要抄多少行？"

达哥没接话，朝屏幕抬了抬下巴："去 poky 里翻。"

### 7.1 U-Boot 上车：BL33 这个坑谁来填

#### 7.1.1 昨天停在哪，今天从哪开始

昨天收工时的状态值得先摆清楚。deploy 目录里躺着 `bl1.bin`、`bl2.bin`、`bl31.bin` 和 `fip.bin`，QEMU `-bios bl1.bin` 的串口日志停在 BL31 那句"准备移交"——移交对象 BL33 不存在，日志就没有下文。今天的任务就是把 BL33 从 fip 包里的一个空位，变成 deploy 目录里的 `u-boot.bin`。

BL33 = U-Boot，chapter 2 就立过这个名字。U-Boot 在 tiger 上的职责，项目立项时定过：从 NAND 加载 kernel 和 dtb，把内核扶上台。但今天得把边界说细一点，免得收工时说不清做完没做完——"从 NAND 加载 kernel + dtb"这句话两端此刻都是空的：kernel 和 dtb 是 chapter 8 的产出，NAND 和 UBI 是 chapter 9 的产出。所以本章的成功标准不是"U-Boot 把内核拉起来"，而是 **U-Boot 自己能在 tiger 上跑进命令行**。加载命令本章照样写，但写成骨架，语义等后面两章就位才闭环。

"那 fip 呢？"阿凯问，"昨天那个包里 BL33 的位置空着，今天 U-Boot 出来了，要不要填回去？"

"不动。"达哥说，"TF-A 一行都不动。fip 那条路 chapter 10 打通启动链的时候才走。"

#### 7.1.2 正式认识 FIP，以及本章不动 TF-A 的边界

昨天 fip.bin 出现过好几次，一直没给正式名字，今天补上：**固件镜像包（Firmware Image Package，FIP）**——TF-A 的固件打包格式，把 BL31、BL33 这些后续阶段的固件打成一个包，交给 BL2 统一加载。昨天 `TFA_BUILD_TARGET = "bl1 bl2 bl31 fip"` 里那个 `fip`，就是让构建顺手打出这个包；包里现在只有 BL31，BL33 的位置靠 tf-a-tiger 平台代码里 `NEED_BL33=no` 这条约定空着。

"U-Boot 编出来了，为什么不顺手填进去？"阿凯追问。

"填进去谁加载它？"达哥反问，"BL2 从哪读 fip、BL31 怎么把 BL33 从包里请出来、往哪跳——这是启动链两级之间怎么握手的事，不是'文件存在'的事。今天 U-Boot 用 `-bios` 单跑就够了，握手留给 chapter 10。"

不过达哥让阿凯看了一眼 meta-arm 配方里备好的机关——这个旋钮 chapter 10 会用到，今天先混个脸熟：

```bitbake
# 文件路径：~/workspace/meta-arm/meta-arm/recipes-bsp/trusted-firmware-a/trusted-firmware-a.inc（关键行节选）
# U-boot support (set TFA_UBOOT to 1 to activate)
# When U-Boot support is activated BL33 is activated with u-boot.bin file
TFA_UBOOT ??= "0"

# ...（中间略）...

DEPENDS += " ${@bb.utils.contains('TFA_UBOOT', '1', 'u-boot', '', d)}"
do_compile[depends] += " ${@bb.utils.contains('TFA_UBOOT', '1', 'u-boot:do_deploy', '', d)}"
EXTRA_OEMAKE += "${@bb.utils.contains('TFA_UBOOT', '1', 'BL33=${DEPLOY_DIR_IMAGE}/u-boot.bin', '', d)}"
```

读法很直白：`TFA_UBOOT` 默认 `"0"`，什么都不发生；哪天设成 `"1"`，TF-A 配方就自动依赖 u-boot、等它的 do_deploy 完成，再把 `${DEPLOY_DIR_IMAGE}/u-boot.bin` 作为 BL33 递给 TF-A 的 Makefile。机关是原配方作者预埋的，chapter 10 我们只要拧旋钮。今天这行保持 `"0"`，`NEED_BL33=no` 的前提也保持——本章的边界就划在这里。

### 7.2 配方从哪来：这次不用借

#### 7.2.1 达哥同一问法，答案自己冒出来

"配方从哪来。"达哥把昨天的问题原样抛回来，"自己写一份，还是……？"

这次阿凯没等他卖关子。他照着昨天的思路先想了一遍候选：U-Boot 上游仓库在 denx.de，要不要像 TF-A 那样去翻 meta-arm？他刚打开浏览器书签，忽然停住了——U-Boot 这个东西太常见了，常见到 poky 自己就该带着。他直接翻本地源码树：

```bash
# 定位 poky 自带的 U-Boot 配方
ls -1 ~/workspace/poky/meta/recipes-bsp/u-boot/
```

输出（以本地实际输出为准）：

```text
files
libubootenv_0.3.5.bb
u-boot-common.inc
u-boot-configure.inc
u-boot-tools.inc
u-boot-tools_2024.01.bb
u-boot.inc
u-boot_2024.01.bb
```

"不用借。"阿凯抬起头，"OE-Core 自己就带着 U-Boot 配方，连 layer 都不用加。"

"昨天那套判据呢？"

"照样成立，而且更顺。"阿凯说，"构建逻辑复杂、上游维护良好——这次维护者就是 OE-Core 本家，复用优于自写，连'引入外部 layer'那一步都省了。昨天是借邻居家的，今天是用自家现货。"

#### 7.2.2 读三份文件：.bb、common.inc、u-boot.inc

老规矩，动笔之前先把原配方读明白。U-Boot 配方是三层结构：`.bb` 很薄，两个 `.inc` 一个管源码、一个管构建。

先看 `.bb`：

```bitbake
# 文件路径：~/workspace/poky/meta/recipes-bsp/u-boot/u-boot_2024.01.bb（全文）
require u-boot-common.inc
require u-boot.inc

DEPENDS += "bc-native dtc-native python3-pyelftools-native"

SRC_URI += "file://CVE-2024-57254.patch \
            file://CVE-2024-57255.patch \
            file://CVE-2024-57256.patch \
            file://CVE-2024-57257.patch \
            file://CVE-2024-57258-1.patch \
            file://CVE-2024-57258-2.patch \
            file://CVE-2024-57258-3.patch \
            file://CVE-2024-57259.patch \
            file://CVE-2024-42040.patch \
"
```

require 两个 inc，补三条构建依赖，然后是九条 CVE 补丁。这九条先记下数量，7.4 要逐条过一遍。

再看 `u-boot-common.inc`，源码获取在这里：

```bitbake
# 文件路径：~/workspace/poky/meta/recipes-bsp/u-boot/u-boot-common.inc（关键行节选）

# We use the revision in order to avoid having to fetch it from the
# repo during parse
SRCREV = "866ca972d6c3cabeaf6dbac431e8e08bb30b3c8e"

SRC_URI = "git://source.denx.de/u-boot/u-boot.git;protocol=https;branch=master \
           file://CVE-2025-24857.patch \
"

S = "${WORKDIR}/git"
B = "${WORKDIR}/build"
```

和 TF-A 配方对照着读，差异立刻显出来：**git 地址是硬编码在 `SRC_URI` 里的，没有 `SRC_URI_TRUSTED_FIRMWARE_A ?=` 那样的预留旋钮**。这直接决定 7.4 的接线手法要换。另外两条值得划线：一是 SRCREV 头顶那行注释——"我们用固定修订号，是为了避免解析期还得去仓库里取"——7.4 写 PV 时要用到这个设计意图；二是 common.inc 里还藏着第十条 CVE patch（CVE-2025-24857），加上 .bb 里那九条，整份配方背着十条 CVE 补丁。

最后是 `u-boot.inc`，三百多行的工程量——compile、install、deploy 全家桶都在里面。阿凯只摘今天用得上的几段：

```bitbake
# 文件路径：~/workspace/poky/meta/recipes-bsp/u-boot/u-boot.inc（关键行节选）
SUMMARY = "Universal Boot Loader for embedded devices"
PROVIDES = "virtual/bootloader"

PACKAGE_ARCH = "${MACHINE_ARCH}"

DEPENDS += "${@bb.utils.contains('UBOOT_ENV_SUFFIX', 'scr', 'u-boot-mkimage-native', '', d)}"

inherit uboot-config uboot-extlinux-config uboot-sign deploy python3native kernel-arch
```

第一眼看第二行：`PROVIDES = "virtual/bootloader"`。chapter 6 达哥说过"`virtual/bootloader` 有 OE-Core 里现成的 PROVIDES，chapter 7 你会用到真的"——就是这一行，虚包真的存在。第三段那个 `${@bb.utils.contains(...)}` 是个条件依赖开关：`UBOOT_ENV_SUFFIX` 等于 `scr` 时才把 `u-boot-mkimage-native` 拉进 DEPENDS——7.6 讲 boot script 时这条管道的主角。`inherit uboot-config` 引出的 class 是 7.3 的主角。

"抄多少行？"达哥又问了一遍。

"一行不用抄。"阿凯说，"但这次有件事不对劲。"他指着 `.bb` 文件名，"文件名写着 `u-boot_2024.01.bb`，SRCREV 指向的也是 2024.01 的提交——可我们全书版本表锁定的是 v2024.04，固件组的 u-boot-tiger fork 也是基于 v2024.04 拉的。**配方文件名是一个版本，我们要用的源码是另一个版本**，这名字不撒谎吗？"

#### 7.2.3 版本名实错位：本章要解决两次

达哥点点头，这次他给了正面回答："名字没撒谎，名字说的是'这份配方文件生在 2024.01 时代'。是我们把源码换成 2024.04 之后，名字才开始名不副实。"

他把这件事拆成了两次解决，阿凯记在本子上：

- **第一次在 bbappend 里**：配方文件名决定默认 PV——chapter 4 学过，PV 就是从文件名里剥出来的版本号。源码经 bbappend 切到 v2024.04 之后，如果不动 PV，deploy 产物名、包名全是 2024.01，内容却是 2024.04。所以 bbappend 里要显式写一行 `PV = "2024.04"`，把名实拉齐。7.4 落实。
- **第二次在 machine conf 里**：PREFERRED_VERSION 那行该写 `"2024.01%"` 还是 `"2024.04%"`？通配符该跟文件名，还是跟改写后的 PV？直觉会选后者——这次对不对、坑在哪边，7.7.3 用一次反向实验回答，那里讲透了再写。

"记住这个错位不是事故，是常态。"达哥补了一句，"发行版的配方文件版本和它实际能构建的源码版本，本来就靠 bbappend 这层隔开。你要做的不是消灭错位，是让错位在纸面上说得清。"

### 7.3 UBOOT_MACHINE：U-Boot 的"平台参数"

#### 7.3.1 正式引入：不设它，配方整份跳过

TF-A 那边平台参数叫 `TFA_PLATFORM`，U-Boot 这边对应的角色换个名字。阿凯对这个名字其实不陌生——chapter 2 读 qemuarm64.conf 时扫到过一行 `UBOOT_MACHINE ?= "qemu_arm64_defconfig"`，当时只混了个脸熟，今天正式引入：**U-Boot 配置名（UBOOT_MACHINE）**——BitBake 变量，指定 U-Boot 配方使用哪个 defconfig 构建。

它的强制性比 TFA_PLATFORM 更硬，硬在解析期。看 `u-boot.inc` inherit 的那个 class：

```python
# 文件路径：~/workspace/poky/meta/classes-recipe/uboot-config.bbclass（关键段节选）
python () {
    ubootmachine = d.getVar("UBOOT_MACHINE")
    ubootconfigflags = d.getVarFlags('UBOOT_CONFIG')
    # ...（中间略）...

    if not ubootmachine and not ubootconfig:
        FILE = os.path.basename(d.getVar("FILE"))
        bb.debug(1, "To build %s, see %s for instructions on \
                 setting up your machine config" % (recipename, FILE))
        raise bb.parse.SkipRecipe("Either UBOOT_MACHINE or UBOOT_CONFIG must be set in the %s machine configuration." % d.getVar("MACHINE"))
```

`UBOOT_MACHINE` 和 `UBOOT_CONFIG`（一套多配置并编的老机制，本书用不上）都不设，class 在解析期直接抛 `SkipRecipe`——整份配方跳过。眼下 tiger-aarch64.conf 里还没写 UBOOT_MACHINE，这个兜底正在生效，可以现场看一眼：

```bash
# 看 u-boot 配方当前对 tiger-aarch64 的状态
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-recipes u-boot
```

输出（以本地实际输出为准）：

```text
=== Matching recipes: ===
u-boot:
  meta                 2024.01 (skipped: Either UBOOT_MACHINE or UBOOT_CONFIG must be set in the tiger-aarch64 machine configuration.)
```

跳过发生在解析期，还意味着配方连被查询的资格都没有。用 chapter 5 的手法直接查变量试试：

```bash
# UBOOT_MACHINE 未设时，直接查 u-boot 的变量
bitbake -e u-boot
```

输出（关键行，以本地实际输出为准）：

```text
ERROR: Nothing PROVIDES 'u-boot'
u-boot was skipped: Either UBOOT_MACHINE or UBOOT_CONFIG must be set in the tiger-aarch64 machine configuration.
```

`Nothing PROVIDES 'u-boot'`——世界里有这份配方文件，但对这台机器而言它不存在。7.7 回填之后，这条查询才有内容可查。

这个兜底值得和 TF-A 的对照一下——同功不同构。TF-A 是配方作者在变量层面兜底：`COMPATIBLE_MACHINE ?= "invalid"`，不为具体机器特化就不兼容，跳过发生在机器兼容性检查那一层。U-Boot 是 class 在解析期兜底：变量没设，配方连"候选"都算不上。显示文案也跟着层级走：兼容性跳过的 skipped 后缀写 `incompatible with machine ... (not in COMPATIBLE_MACHINE)`（chapter 5 整镜报错里那批），SkipRecipe 则把抛出的原因整句带了出来——上面两条就是原文。两种写法对使用者的要求一样：**认领这台机器的责任在 machine 配置一侧**，配方自己不猜。

#### 7.3.2 参数链：从变量到 `make tiger_aarch64_defconfig`

UBOOT_MACHINE 的值怎么流进 U-Boot 的构建？和 chapter 6 追 TFA_PLATFORM 的手法一样，沿着变量链走。接收端在 `u-boot-configure.inc` 的 `do_configure`：

```bash
# 文件路径：~/workspace/poky/meta/recipes-bsp/u-boot/u-boot-configure.inc（关键段节选）
uboot_configure () {
    if [ -n "${UBOOT_MACHINE}" ]; then
        oe_runmake -C ${S} O=${B} ${UBOOT_MACHINE}
    else
        oe_runmake -C ${S} O=${B} oldconfig
    fi
    merge_config.sh -m .config ${@" ".join(find_cfgs(d))}
    cml1_do_configure
}
```

`oe_runmake -C ${S} O=${B} ${UBOOT_MACHINE}`——在源码目录里跑 `make`，目标就是 UBOOT_MACHINE 的值。我们写上 `tiger_aarch64_defconfig`，这一步实际执行的就是 `make tiger_aarch64_defconfig`：U-Boot 与内核共用同一套 Kbuild/Kconfig 构建体系，defconfig 就是它的输入格式——这一步就是把 `configs/tiger_aarch64_defconfig` 展开成 `.config`。和 chapter 6 那条链（TFA_PLATFORM → EXTRA_OEMAKE → `PLAT=tiger` → plat/tiger/ 目录）同构：**BitBake 变量只是信使，真正接信的是组件自己的构建系统**。

这里也正式给 defconfig 一个名字——它 chapter 2 之后露过几次面，今天要用它干活了：**默认配置（defconfig）**——内核或 U-Boot 的默认配置文件，一份"这个平台的 Kconfig 选项该开哪些"的清单，是 menuconfig 等一切配置改动的起点。

归属问题顺带裁定：UBOOT_MACHINE 写在哪？两边都有先例可查——qemuarm64.conf 里是 `UBOOT_MACHINE ?= "qemu_arm64_defconfig"`，meta-arm 的 juno.conf 里是 `UBOOT_MACHINE = "vexpress_aemv8a_juno_defconfig"`，都写在 machine conf 里。道理也顺：defconfig 选择是"这块板子长什么样"的陈述，和 KERNEL_IMAGETYPE、SERIAL_CONSOLES 一桌；bbappend 管的是"源码从哪来、版本叫什么"，不管平台身份。**UBOOT_MACHINE 落 machine conf**——7.7 回填时写。

> **💡 提示**：UBOOT_MACHINE 的名字写错（连字符/下划线笔误、大小写），do_configure 会报 `make: *** No rule to make target 'xxx_defconfig'`——和 chapter 6 平台名大小写那个坑同构：错在别人的构建系统里，Yocto 层的检查全过。取值去 U-Boot 源码树的 `configs/` 目录里对，不凭记忆写。

#### 7.3.3 defconfig 放哪个仓库：开发态纪律再说一遍

`tiger_aarch64_defconfig` 这个文件本身放哪？阿凯把问题摆出来：u-boot-tiger 仓库，还是 meta-tiger？

"你昨天怎么答 TF-A 的平台代码？"达哥问。

"`plat/tiger/` 住在 tf-a-tiger，meta-tiger 只做集成。"阿凯顺着这个句式往下推，"defconfig 是 tiger 板支持代码的一部分——它跟着板子支持走，住在 u-boot-tiger 仓库的 `configs/` 下，跟入口代码、设备树一起演进。功能开发在开发态仓库，这条纪律不换组件。"

"那如果集成的时候想临时拧一个配置项呢？"达哥问，"比如调试期想开个 CONFIG，又不想动 fork 仓库。"

阿凯翻出刚才 `do_configure` 节选里还没讲的那两行——`find_cfgs` 和 `merge_config.sh`。这是 Yocto 侧备好的另一条通道：往 SRC_URI 里追加 `.cfg` 片段文件，`do_configure` 会在 defconfig 展开之后用 `merge_config.sh` 把片段合并进 `.config`。**defconfig 管平台出厂形态，.cfg 片段管集成态微调**——正当通道，有明确分工。内核那边也有一套同名的 cfg 片段机制，chapter 8 会专门讲，两套机制形似而各有脾气，到时候再对照，今天先不混着讲。

<!-- 【待验证】defconfig 具体行依赖 u-boot-tiger 仓库（待验证清单 C-W1 项）；以下节选为按机制书写的示意，定稿时以仓库实测替换 -->
按这个分工，defconfig 的关键行（7.6 会回来细讲其中两行）长这样——来自 u-boot-tiger 仓库 `configs/tiger_aarch64_defconfig` 的节选（示意形态，以仓库实际内容为准）：

```text
# 文件路径：~/workspace/u-boot-tiger/configs/tiger_aarch64_defconfig（关键行节选，示意形态）
CONFIG_DEFAULT_DEVICE_TREE="tiger"
CONFIG_EXTRA_ENV_SETTINGS="bootcmd=run distro_bootcmd\0"
# ...（串口 PL011 115200、内存布局等平台项，以仓库实际内容为准）
```

### 7.4 编写 u-boot_%.bbappend：换源码，这次没旋钮

#### 7.4.1 没有旋钮的接线：SRC_URI:remove + SRC_URI:append

目录先落位，沿用 chapter 4 的约定——bbappend 旁边要有 `${PN}` 同名的目录放 `file://` 附件：

```bash
# 创建 bbappend 与 file:// 附件的落位目录
mkdir -p ~/workspace/meta-tiger/recipes-bsp/u-boot/u-boot
```

接线手法这次要换。chapter 6 的 TF-A 配方预留了 `SRC_URI_TRUSTED_FIRMWARE_A ?=` 旋钮，bbappend 只要覆盖旋钮变量，SRC_URI 的最终值就跟着变——那是原配方作者留的活口。7.2.2 读过，U-Boot 的 `u-boot-common.inc` 把 git 地址直接写死在 `SRC_URI` 里，没有活口。手法相应变成两刀：

- `SRC_URI:remove` 把原来那条 git 行从清单里摘掉——`:remove` 按条目字符串精确匹配，写法和原条目逐字符一致；
- `SRC_URI:append` 把 u-boot-tiger 的地址追加上去，分支名 `branch=main` 随条目走（这份配方没有 TF-A 配方那种独立的 SRCBRANCH 变量，分支处理随 remove/append 一并完成）。

"为什么不整体覆盖 SRC_URI？"阿凯自问自答，他翻着 7.2.2 的笔记——"因为清单里不止 git 行，还有十条 CVE patch。整体覆盖等于把它们一并误杀；`:remove` 只摘该摘的，剩下的原样留着。"他顿了顿，"不过——**剩下的真的都该留着吗**？那十条 patch 是 poky 给 2024.01 准备的，我们换的是 2024.04 的源码。"

这个问题问到了点子上，达哥却没收："patch 头部有答案，自己去翻。"

#### 7.4.2 换大版本时的旧 patch 清单：逐条重审

阿凯把十条 patch 的头部挨个翻出来。chapter 4 学过 patch 头部的 Upstream-Status 字段——标记补丁是否已提交上游及状态。这十条的头部长得像一个模子刻的，挑三条代表看个形态（读 patch 头部的真实读法就是先看形态、同形归类）：

```text
# patch 头部的 Upstream-Status 行（代表三条，逐字摘自 ~/workspace/poky/meta/recipes-bsp/u-boot/files/；
# 其余七条同形，十条提交号的全量清单见下方核对表）
# CVE-2024-57254.patch:  Upstream-Status: Backport [https://source.denx.de/u-boot/u-boot/-/commit/c8e929e5758999933f9e905049ef2bf3fe6b140d]
# CVE-2024-42040.patch:  Upstream-Status: Backport [https://source.denx.de/u-boot/u-boot/-/commit/81e5708cc2c865df606e49aed5415adb2a662171]
# CVE-2025-24857.patch:  Upstream-Status: Backport [https://source.denx.de/u-boot/u-boot/-/commit/87d85139a96a39429120cca838e739408ef971a2]
# ...（其余七条同形，头部均为 Upstream-Status: Backport [提交 URL]）
```

`Backport [<提交 URL>]`——每一条都是 poky 维护者从上游把修复**回移**到 2024.01 的，方括号里就是上游的原始提交号。核对依据现成，剩下的就是逐条回答两个问题：**①这个上游修复合进 v2024.04 了吗？②没合进去的话，这条按 2024.01 上下文写的补丁，还打得上 2024.04 吗？**

第一个问题，git 一句话能答——等 u-boot-tiger 就位（或任何一个带 v2024.04 tag 的 U-Boot clone），逐条查提交号在不在基线里：

```bash
# 逐条核对：该 CVE 的上游修复是否已在 v2024.04 基线中（在含 v2024.04 tag 的 U-Boot 仓库里执行）
cd ~/workspace/u-boot-tiger
git merge-base --is-ancestor c8e929e5758999933f9e905049ef2bf3fe6b140d v2024.04 \
    && echo "已在基线里，patch 可摘除" || echo "不在基线里，patch 仍需保留"
# ... 对其余九条的提交号重复同一动作
```

第二个问题，只有构建能答——do_patch 阶段补丁能不能干净落上，跑一遍 `bitbake u-boot` 全知道。

处置分三档，阿凯在便签上列好：

- **已上游合入** → 这条 patch 成了多余甚至有害（重复修复），在 bbappend 里 `SRC_URI:remove = "file://CVE-XXXX-XXXXX.patch"` 逐条摘除——`file://` 条目同样吃 `:remove`；
- **未合入、能打上** → 保留，不动；
- **未合入、打不上**（两个版本间上下文漂移，hunk 落不了地）→ 三选一：refresh 重定位补丁上下文；把上游修复 cherry-pick 进 u-boot-tiger 再从清单摘除；摘除并接受风险（必须在提交信息里写明白）。

<!-- 【待验证·阻塞级】以下核对结论依赖 u-boot-tiger 仓库就位后的实测（待验证清单 C-W2 项）：预判来自时间线——十条 CVE 的上游修复均晚于 v2024.04 tag（patch 头部 Date 实测：CVE-2024-5725x 系列的上游修复落在 2024 年 8 月，CVE-2024-42040 落在 2025 年 10 月，CVE-2025-24857 落在 2025 年 12 月——最早的也在 v2024.04 tag 之后），预判十条均未上游化、全部保留；真实风险在 do_patch 能否干净打上 2024.04。实测后回填本表与 bbappend 的 remove 行 -->
按时间线预判（修复都晚于 v2024.04 tag），核对表大概率长这样：

| CVE patch | 上游提交 | 在 v2024.04 基线？ | 处置 |
|-----------|---------|-------------------|------|
| CVE-2024-57254 | c8e929e5 | 预判：否 | 保留 |
| CVE-2024-57255 | 233945eb | 预判：否 | 保留 |
| CVE-2024-57256 | 35f75d2a | 预判：否 | 保留 |
| CVE-2024-57257 | 4f5cc096 | 预判：否 | 保留 |
| CVE-2024-57258-1 | 0a10b492 | 预判：否 | 保留 |
| CVE-2024-57258-2 | 8642b217 | 预判：否 | 保留 |
| CVE-2024-57258-3 | c17b2a05 | 预判：否 | 保留 |
| CVE-2024-57259 | 048d795b | 预判：否 | 保留 |
| CVE-2024-42040 | 81e5708c | 预判：否 | 保留 |
| CVE-2025-24857 | 87d85139 | 预判：否 | 保留 |

"要是预判错了呢？"阿凯问。

"预判错不错不重要，**动作不能省**。"达哥说，"换大版本后旧 patch 清单逐条重审，这是正常工序，不是出了事才补的救。跳过它的两种结局我画给你看——运气好，do_patch 安静通过，可某条其实上游已经修了，你背着一条空补丁而不自知；运气差，do_patch 直接报 context mismatch 把你拦下。**do_patch 失败是信号，不是意外**——它在告诉你清单和源码版本对不上了。"

> **⚠️ 注意**：这个重审动作同时是"remove 优于覆盖"的实证理由。整体覆盖 SRC_URI 会把仍需保留的 CVE patch 一并误杀；只有逐条 `:remove` 才能表达"这条不需要了、其余留着"。覆盖是最省事的写法，也是最说不清的写法。

#### 7.4.3 PV 改写与 SRCREV：名实相符的两行

7.2.3 说好的第一刀落在这里。`u-boot_2024.01.bb` 这个文件名决定了默认 PV 是 `2024.01`；bbappend 在配方选中之后与 `.bb` 拼接执行，写一行 `PV = "2024.04"`（无条件赋值），文件名派生的默认值就被覆盖。从此 deploy 产物名、包名、内部版本字符串都按 2024.04 走——名实相符。

SRCREV 照 chapter 6 的纪律锁提交哈希。一个对照细节：TF-A 那边叫 `SRCREV_tfa`，因为原配方 SRC_URI 条目带 `;name=tfa`；这份 U-Boot 配方是单一 git 源、没有 `name=` 后缀，修订号变量就叫光秃秃的 `SRCREV`——名字后缀跟着 SRC_URI 的 `name=` 走，没有 name 就没有后缀，同一套规矩的正反两例。

还有一行刻意不写的：`PV = "2024.04+git${SRCPV}"` 这类写法在别的配方里常见，这里不用。`u-boot-common.inc` 里 SRCREV 头顶那行注释说得很清楚——静态锁死修订号，就是为了解析期不用去仓库取版本信息；引入 SRCPV 会破坏这个设计，对本章也没有任何增益。原配方作者的设计意图，读懂了再决定跟不跟。

#### 7.4.4 bbappend 全文与验证

本章核心交付物全文：

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-bsp/u-boot/u-boot_%.bbappend
# tiger 平台的 U-Boot 集成：源码改指 u-boot-tiger 开发态仓库（固件组维护，
# 基于上游 v2024.04）；defconfig/dts 等平台代码在开发态仓库演进，本 layer 只做集成

# file:// 附件（boot.cmd）搜索路径，沿用 chapter 4 约定
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# 换源码：原配方（u-boot-common.inc）git 地址硬编码、无预留旋钮，
# :remove 摘除原 git 行（与 u-boot-common.inc 中条目逐字符一致），:append 追加 fork
SRC_URI:remove = "git://source.denx.de/u-boot/u-boot.git;protocol=https;branch=master"
SRC_URI:append = " git://<internal-git-server>/bsp/u-boot-tiger.git;protocol=ssh;branch=main"
# 锁定提交哈希，保证可重现构建；单一 git 源无 name= 后缀，变量名即 SRCREV
SRCREV = "0000000000000000000000000000000000000000"

# 名实相符：文件名派生 PV=2024.01，源码已切 v2024.04，显式改写覆盖默认值
PV = "2024.04"

# CVE patch 逐条重审结论（7.4.2）：10 条全部为 2024.01 回移，
# 预判全部保留（上游修复均晚于 v2024.04；do_patch 能否干净打上待仓库就位后实测回填）
# （若某条已上游合入，处置写法：SRC_URI:remove = "file://CVE-XXXX-XXXXX.patch"）

# boot script：UBOOT_ENV=boot + SUFFIX=scr 触发 u-boot.inc 的 mkimage 打包管道（7.6.2），
# boot.cmd 经 file:// 进 SRC_URI
SRC_URI:append = " file://boot.cmd"
UBOOT_ENV = "boot"
UBOOT_ENV_SUFFIX = "scr"
```

<!-- 【待验证】SRCREV 为全零占位（u-boot-tiger 仓库尚不存在，待验证清单 C-W6 项），定稿前替换为真实提交；CVE 注释为预判口径，随 C-W2 实测改为确定措辞 -->

和 TF-A 那份对照，少了什么？没有 `COMPATIBLE_MACHINE` 行——U-Boot 配方没有 invalid 兜底，认领靠 7.7 要写进 machine conf 的 UBOOT_MACHINE；也没有平台参数行——UBOOT_MACHINE 不落 bbappend，归属 7.3.2 已经裁定过。这份 bbappend 的职责就三件：源码改指、版本名实、env/script 接线。

验证全是旧艺。先看挂接：

```bash
# 检查 u-boot 的 bbappend 叠加关系
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-appends u-boot
```

输出（以本地实际输出为准）：

```text
=== Matched appended recipes ===
u-boot_2024.01.bb:
  /home/<your-username>/workspace/meta-arm/meta-arm/recipes-bsp/u-boot/u-boot_%.bbappend
  /home/<your-username>/workspace/meta-tiger/recipes-bsp/u-boot/u-boot_%.bbappend
```

比预想多出一行：meta-arm 自己也带一条 `u-boot_%.bbappend`。bbappend 是全局叠加——layer 在 BBLAYERS 里，它那条就对 tiger 同样生效（内容是 meta-arm 自己的平台杂项，与本章无关，不展开）；本章这条排在它后面叠加。

变量终值这次先不查——`bitbake -e u-boot` 得等 7.7 把 UBOOT_MACHINE 写进 machine conf 之后才有内容可查，现在跑只会吃到 7.3.1 现场见过的那个 `ERROR: Nothing PROVIDES 'u-boot'`。这笔账先记下，7.7.3 回填之后一次查全。

### 7.5 两份设备树：U-Boot 的 dts 不是内核的 dts

7.3.3 的 defconfig 节选里有一行 `CONFIG_DEFAULT_DEVICE_TREE="tiger"`，阿凯圈了出来："设备树？设备树不是 chapter 8 内核那边的事吗？"

"是两份。"达哥说，"今天先把这条界限划了，免得 chapter 8 打架。"

U-Boot 自己也要认硬件——串口在哪个地址、NAND 控制器什么型号，它打印日志、读启动介质都要用。所以 U-Boot 源码树里自带一套设备树，tiger 的那份住在 u-boot-tiger 仓库 `arch/arm/dts/tiger.dts`，defconfig 里 `CONFIG_DEFAULT_DEVICE_TREE` 指到它，编译时随二进制一起走。说细一点：dtb 是否真的编进 `u-boot.bin`，由 fork defconfig 的 OF_EMBED/OF_SEPARATE 选择决定——7.8 用 `-bios u-boot.bin` 单跑这条路要求 dtb 随二进制走，这一点已列入待验证清单 C-W1 的核对项。内核那份 dts 是 chapter 8 linux-tiger 仓库的事，两份文件描述的是同一块板子，但服务两个组件、各自演进——U-Boot 改它的不代表内核跟着改，反过来也一样。

"那 U-Boot 的 dtb 要不要也 deploy 出来？"阿凯问。

"`u-boot.inc` 里留着可选通道——`UBOOT_DTB`、`UBOOT_DTB_BINARY` 设了，do_deploy 会把编出的 dtb 一并拷进 deploy 目录。"达哥说，"tiger 要不要用这条通道，等 u-boot-tiger 的 defconfig 定了再定。今天记住一件事：**跟着 u-boot.bin 走的那份设备树是 U-Boot 自己带的，不来自内核仓库**——将来在真板上排查'U-Boot 认得出、内核认不出'这类问题，先想清楚自己在看哪一份。"

### 7.6 环境变量三条路线与 boot.scr

#### 7.6.1 出厂环境从哪来：三条路线

U-Boot 的"规矩"里，环境变量是最活的一样——启动时先干什么、从哪加载、加载到哪，都是环境变量说了算。这批变量的初始值有三条来路，阿凯按"出厂内置的程度"排了序：

1. **构建期烧进二进制**：**U-Boot 环境变量初始值（CONFIG_EXTRA_ENV_SETTINGS）**——U-Boot 的 Kconfig 符号，在 defconfig 里定义环境变量的出厂默认值，编进 u-boot.bin，上电就有，不依赖任何存储介质。7.3.3 节选里那行就是。构建时这套默认值还会被导出成一份文本产物 `u-boot-initial-env`（`u-boot.inc` 里 `UBOOT_INITIAL_ENV` 机制负责生成和 deploy），可以在主机上直接查看、也可以用工具写回设备。
2. **运行时从介质读文本**：`uEnv.txt`——启动时 U-Boot 从启动介质（FAT 分区等）上读一个纯文本文件，就地覆盖同名变量。改环境不用重编固件，改个文本文件就行；代价是要有一块 U-Boot 已经认得的介质。
3. **运行时执行编译后的脚本**：`boot.scr`，下一小节的主角。

tiger 的用法是一和三搭配：出厂默认值走 CONFIG_EXTRA_ENV_SETTINGS 烧进二进制，启动逻辑脚本化走 boot.scr。uEnv.txt 今天只认个脸熟——它要求介质上先有 FAT 分区，tiger 的存储主线是 NAND/UBI，chapter 9 才轮得到分区形态的事。

#### 7.6.2 boot.scr 的"recipe 化"：管道早就铺好了

boot.scr 值得正式介绍：**启动脚本（boot.scr）**——U-Boot 的编译后启动脚本，由纯文本的 `boot.cmd` 经 mkimage 工具打包生成；U-Boot 自举时找到它就逐条执行里面的命令。mkimage 是 U-Boot 的镜像打包工具（U-Boot 源码树自带，主机侧使用），给裸文本脚本加上 U-Boot 认得的头部。

阿凯本以为要为此写一份新 recipe——"把 boot.cmd 编译成 boot.scr，总得有个配方干这活吧？"他话音没落就自己摇头了：7.2.2 读 `u-boot.inc` 时那个条件 DEPENDS 还记着呢。回去把两段拼起来看，管道是原配方作者铺好的：

```bash
# 文件路径：~/workspace/poky/meta/recipes-bsp/u-boot/u-boot.inc（mkimage 打包段，do_compile 内节选）
    if [ -n "${UBOOT_ENV}" ] && [ "${UBOOT_ENV_SUFFIX}" = "scr" ]
    then
        ${UBOOT_MKIMAGE} -C none -A ${UBOOT_ARCH} -T script -d ${WORKDIR}/${UBOOT_ENV_SRC} ${WORKDIR}/${UBOOT_ENV_BINARY}
    fi
```

机制拆开来是三扣环：

- **第一扣**：bbappend 里 `UBOOT_ENV = "boot"` + `UBOOT_ENV_SUFFIX = "scr"`（默认值是 `txt`，所以后缀必须显式改）。`uboot-config.bbclass` 里一组 `?=` 派生变量的展开值随之变成：`UBOOT_ENV_SRC` 拼成 `boot.cmd`、`UBOOT_ENV_BINARY` 拼成 `boot.scr`、deploy 用的 `UBOOT_ENV_IMAGE` 拼成 `boot-<机器>-<版本>.scr`。
- **第二扣**：7.2.2 那行条件 DEPENDS 触发——后缀是 `scr`，`u-boot-mkimage-native` 自动进 DEPENDS。它由 `u-boot-tools` 配方提供（那份配方 `BBCLASSEXTEND = "native nativesdk"`，并以 `PROVIDES:class-native` 报出 `u-boot-mkimage-native` 这个名字）——native 配方，编出的是跑在构建主机上的 mkimage。
- **第三扣**：上面这段 do_compile 内联代码执行 mkimage 打包；do_deploy 里对应的一段再把 `boot.scr` 送进 deploy 目录，配上带机器与版本名的实体文件和符号链接——和 u-boot.bin 同等待遇。

"所以'boot script 的 recipe 化'是个假命题。"阿凯在笔记里写，"不用另写 recipe——`UBOOT_ENV` 两个变量加上 `file://boot.cmd` 进 SRC_URI，整条管道自己就转起来了。"

#### 7.6.3 boot.cmd 雏形：A/B 选择的伏笔

脚本本体是本章交付物之一。写之前先把边界钉死：脚本里"从 NAND 读内核"的命令，两端都还没就位——kernel 和 dtb 等 chapter 8，NAND 上的 UBI 卷等 chapter 9。所以本章交付的是**打包形态**（boot.cmd 能被 mkimage 打包、boot.scr 能进 deploy 目录），执行语义是伏笔不是承诺。阿凯把这个边界写进了文件头注释里：

<!-- 【待验证】boot.cmd 的打包形态（mkimage 可处理、deploy 产物形态）可随构建实测确认；NAND/UBI 命令的执行语义要等 chapter 8/9 就位后方可真跑，命令拼写以届时实测为准 -->
```text
# 文件路径：~/workspace/meta-tiger/recipes-bsp/u-boot/u-boot/boot.cmd
# tiger 启动脚本（雏形）：按 bootpart 环境变量选择 A/B 分区加载 kernel + dtb
# 边界：NAND/UBI 命令等 chapter 9 建好 UBI、kernel/dtb 等 chapter 8 就位后方可真跑；
# 本章只交付"mkimage 打包 + deploy 形态"，执行语义是伏笔

# bootpart 由 OTA 升级流程改写（a 或 b），缺省走 a
if test "${bootpart}" = "b"; then
    setenv bootvol rootfs_b
else
    setenv bootpart a
    setenv bootvol rootfs_a
fi

# 初始化 NAND 上的 UBI，挂载当前启动卷
# （卷名与命令形态以 chapter 9 的 UBI 布局为准）
ubi part nand0
ubifsmount ${bootvol}

# 从启动卷加载 kernel 与 dtb（chapter 8 交付这两个文件）
ubifsload ${kernel_addr_r} /boot/Image
ubifsload ${fdt_addr_r} /boot/tiger.dtb

# 启动：AArch64 的 Image 格式走 booti
booti ${kernel_addr_r} - ${fdt_addr_r}
```

骨架里最值钱的是开头那个 `bootpart` 判断——这是给 OTA 留的接口。设计立项时定过 A/B 分区策略：NAND 上 rootfs_a / rootfs_b 两套卷，升级写另一套，重启前把 `bootpart` 改指过去，起不来还能指回来。U-Boot 这一侧要做的全部事情就是"读变量、选分区"，今天这个 `if` 就是它的雏形。真正的升级流程怎么改写这个变量，是后期章节的事。

最后介绍兜底角色：**启动命令（bootcmd）**——U-Boot 环境变量，存默认的启动命令序列；倒计时结束没人按键，U-Boot 就执行它。分工是这样：bootcmd 里写"去找 boot.scr 并执行"，boot.scr 找不到时的回退动作也挂在 bootcmd 里。出厂的 bootcmd 来自 defconfig 里 CONFIG_EXTRA_ENV_SETTINGS 那行——三条路线在这里合流：环境变量出厂值（路线一）负责兜底逻辑，boot.scr（路线三）负责正式流程。

"这些命令现在跑不了，写出来算什么？"阿凯问。

达哥这次答了："算契约。chapter 8、chapter 9 照这个契约交付文件和卷名，chapter 10 整条链才敢通电。骨架先立在纸面上，后面两章是照着它填空，不是到时候再临时商量。"

### 7.7 回填与整镜验收：先查再写，这次真有人听

#### 7.7.1 先查：虚包真的在，而且有人在抢

接线完成，轮到 machine 配置认领了。阿凯刚要动笔，自己先停下了——chapter 5 的"先查再写"、chapter 6 的虚包查证，两轮教训养成了条件反射：TF-A 那次 `virtual/trusted-firmware-a` 根本不存在，认领行写了没人听；这次 `virtual/bootloader` 呢？

"老规矩。"达哥看他停下来，只说了三个字。

文件级证据其实 7.2.2 已经拿到一半：`u-boot.inc` 第 2 行 `PROVIDES = "virtual/bootloader"`——虚包真的存在。但 chapter 6 那课教的是要查全，阿凯把 meta-arm 也翻了一遍：

```bash
# 全库搜查：还有谁 PROVIDES 或引用 virtual/bootloader
grep -rn "virtual/bootloader" ~/workspace/meta-arm --include="*.inc" --include="*.bb" --include="*.yml" --include="*.conf"
```

输出（关键行，以本地实际输出为准）：

```text
/home/<your-username>/workspace/meta-arm/meta-arm/recipes-bsp/uefi/edk2-firmware.inc:6:PROVIDES = "virtual/bootloader"
/home/<your-username>/workspace/meta-arm/ci/u-boot.yml:8:    PREFERRED_PROVIDER_virtual/bootloader = "u-boot"
/home/<your-username>/workspace/meta-arm/ci/edk2.yml:8:    PREFERRED_PROVIDER_virtual/bootloader = "edk2-firmware"
/home/<your-username>/workspace/meta-arm/meta-arm-bsp/conf/machine/juno.conf:21:PREFERRED_PROVIDER_virtual/bootloader ?= "u-boot"
/home/<your-username>/workspace/meta-arm/meta-arm-bsp/conf/machine/juno.conf:23:EXTRA_IMAGEDEPENDS += "trusted-firmware-a virtual/bootloader firmware-image-juno"
```

第一行是新的竞争者：meta-arm 的 edk2-firmware（UEFI 固件配方）也 PROVIDES 同一个虚包。第三行更说明问题——meta-arm 自己的 CI 配置里，用 edk2 的机器就靠 `PREFERRED_PROVIDER_virtual/bootloader = "edk2-firmware"` 这行点名。同一个虚包，两个真实配方在抢，连"选谁"的实例都有现成的。

"这次和 TF-A 反过来了。"阿凯说，"`PREFERRED_PROVIDER` 不是没人听——**不写它，仲裁就缺席**：两个提供者都在场时，BitBake 只能挑一个默认的，挑谁不由我定。chapter 2 那句前瞻到今天才算兑现——不过得认个错：立项时我说的是 `= "u-boot-tiger"`，那是按'U-Boot 配方会自写'估的名字。7.2 定了复用 OE-Core 现货，提供者名就是 `u-boot`——u-boot-tiger 只是源码仓库，不是配方名。机制预判对了，名字预判错了。"

"两章互为镜像。"达哥说，"chapter 6 你查出来'虚包不存在、写了没人听'，今天查出来'虚包存在、不写没人替你仲裁'；chapter 6 我那句前瞻估错了虚包，你这句前瞻估错了名字。同一套查证动作，两种结论——结论可以变，动作不能省。"

#### 7.7.2 惯例取证与回填四行

写法照谁？meta-arm 自己的机器配置就是范本。阿凯把 juno.conf 的相关行摘出来——Juno 是 ARM 官方参考板，它的 TF-A + U-Boot 集成是这套惯例的标准长相：

```bash
# 看 meta-arm 自带机器配置的 U-Boot 相关声明（以 Juno 为例）
grep -n "bootloader\|UBOOT_MACHINE" ~/workspace/meta-arm/meta-arm-bsp/conf/machine/juno.conf
```

输出（以本地实际输出为准）：

```text
21:PREFERRED_PROVIDER_virtual/bootloader ?= "u-boot"
23:EXTRA_IMAGEDEPENDS += "trusted-firmware-a virtual/bootloader firmware-image-juno"
26:UBOOT_MACHINE = "vexpress_aemv8a_juno_defconfig"
```

三件套：`PREFERRED_PROVIDER` 点名 u-boot 当虚包的提供者；`EXTRA_IMAGEDEPENDS` 把 `virtual/bootloader` 挂上镜像依赖链——和 chapter 6 TF-A 同款逻辑，U-Boot 的产物进 deploy 目录、不进 rootfs，镜像构建靠这行把它拉起来；`UBOOT_MACHINE` 指定 defconfig。版本锁定另有同 PN 的现成惯例可查：corstone1000.inc 里 `PREFERRED_VERSION_u-boot ?= "2023.07%"`——同一个配方名、按版本通配，正是本章要写的形式；通配符该跟哪个版本走，7.7.3 讲透。

照着惯例，回填四行（接 chapter 6 的六段之后追加第七段）：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接 chapter 6 全文追加）

# ---- U-Boot 集成声明 ----
# tiger 的 U-Boot 平台配置；defconfig 住在 u-boot-tiger 仓库 configs/ 下（7.3.3）
UBOOT_MACHINE ?= "tiger_aarch64_defconfig"
# virtual/bootloader 虚包由 u-boot 提供：edk2-firmware 同为竞争者（7.7.1），必须显式点名
PREFERRED_PROVIDER_virtual/bootloader ??= "u-boot"
# 认 bbappend 改写后的生效 PV：源码基线 v2024.04，PV 已改写为 2024.04（机制见 7.7.3）
PREFERRED_VERSION_u-boot ??= "2024.04%"
# 镜像构建时把 U-Boot 拉上依赖链（产物进 deploy 目录，不进 rootfs）
EXTRA_IMAGEDEPENDS += "virtual/bootloader"
```

四个赋值符顺带交代一句，免得照抄时混写：`UBOOT_MACHINE` 随 qemuarm64 用 `?=`（平台默认值，允许发行版或 local.conf 覆写）；两行认领/锁定随 chapter 5 的惯例用 `??=`（最弱赋值，谁先设谁生效）；`EXTRA_IMAGEDEPENDS` 是追加不是赋值，前面 TF-A 那行的值要留住。

#### 7.7.3 为什么是 "2024.04%"：PREFERRED_VERSION 认的是生效 PV

第三行值得停下来讲透——它就是 7.2.3 说好的"第二次解决"。阿凯的第一直觉是写 `"2024.04%"`："源码是 2024.04，PV 也改成 2024.04 了，通配符当然跟着它。"写上，解析期安安静静，一条警告都没有。这次直觉对了。

"别急着收。"达哥说，"反着试一次——照文件名写 `"2024.01%"`，看着也名正言顺，会发生什么。"

阿凯把那行改成 `"2024.01%"`，重新解析，两条警告浮出来：

```text
WARNING: preferred version 2024.01% of u-boot not available (for item u-boot)
WARNING: versions of u-boot available: 1:2024.04
```

然后一切照常——不报错，构建照走，BitBake 静默落到唯一的候选上。坑就坑在这里：警告而不是错误，排查时一晃而过；而"退回唯一候选"在单版本场景下结果碰巧还是对的，错了都察觉不到。

注意第二行警告自己就把机制说了：`versions of u-boot available: 1:2024.04`——冒号前的 1 是 epoch（版本纪元前缀，本书配方未自设、用默认值），知道格式即可，注意力锚在 PV 段。BitBake 报"可用版本"时认的是 epoch:PV，而这里的 PV 已经是 bbappend 改写后的 2024.04。机制一句话：**PREFERRED_VERSION 匹配的是 bbappend 应用之后的生效 PV，不是配方文件名**。平时 PV 就是从文件名里剥出来的，两者相等，"PREFERRED_VERSION 认文件名"作为近似说法不会出事；一旦 bbappend 改写了 PV，两边分叉，通配符必须跟着新 PV 走。

"直觉这次对，是碰巧踩在机制上。"达哥说，"不反着试一次，不知道踩的是机制还是运气。落盘的是 `2024.04%`，带走的是那次反试。"

这也回头讲清了惯例取证时留下的那个细节：corstone1000.inc 里写 `"2023.07%"`——那行不是摆设，是真选择器。meta-arm-bsp 自己就带一份 `u-boot_2023.07.02.bb`（require 同一套 `u-boot-common.inc` / `u-boot.inc`，SRC_URI 指向 `u-boot-2023.07.y` 分支、SRCREV 钉死提交），corstone1000 面前其实摆着两个候选：poky 的 2024.01 和 meta-arm-bsp 的 2023.07.02，这行 pin 负责在两份并存时点旧的那份。那边没人改写 PV，文件名版本就是生效 PV，照文件名写刚好成立——它正是"常态情形"的样板，和 chapter 5 给 linux-tiger 预备的 `"6.6%"`、chapter 6 的 `"2.10.%"` 同族（`trusted-firmware-a_2.10.4.bb` 的 PV 也没人动过，照文件名写和照 PV 写指向同一个目标）；本章 tiger 是两边分叉的另一侧。顺带它还顺手演示了本节末尾那句"将来"：多份配方并存时 PREFERRED_VERSION 就是真选择器——corstone1000 是已经在用的现成实例。

`show-recipes` 也顺带成了实证——终态下它显示的版本不再是文件名里的 2024.01：

```bash
# 终态确认：可选版本显示为 epoch:PV（bbappend 改写后的生效 PV）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-recipes u-boot
```

输出（以本地实际输出为准）：

```text
=== Matching recipes: ===
u-boot:
  meta                 1:2024.04
```

一句话记住：**PREFERRED_VERSION 认生效 PV——包括 bbappend 改写后的 PV**。单版本场景下这行是防御性惯例——起自我文档化的作用，把"我们预期用哪份配方"写在纸面上；将来 poky 升级、多份 u-boot 配方并存时，它就是真选择器。

> **⚠️ 注意**：把 `PREFERRED_VERSION_u-boot` 照文件名写成 `"2024.01%"` 不会报错拉闸，只会收上面那两条 "not available" 警告、然后静默退回唯一的候选——看着像生效了，实际是选择器落了空，BitBake 自作主张选了仅剩的那份。这类"警告而不是错误"的形态，排查时容易一晃而过。

四行落盘，变量级验证分两路。先看配方这一侧——7.4.4 欠着的那笔账现在能还了：UBOOT_MACHINE 就位，配方不再被 SkipRecipe 整份跳过，`bitbake -e u-boot` 有内容可查。一次查全：

```bash
# 变量级实证：PROVIDES 与 bbappend 叠加后的关键变量终值（UBOOT_MACHINE 就位后配方才可解析）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e u-boot | grep -E "^(PROVIDES|PV|SRC_URI|SRCREV|UBOOT_ENV|UBOOT_ENV_SUFFIX|UBOOT_MACHINE)="
```

输出（以本地实际输出为准；终端里 SRC_URI 各条目间有多个空格，这里按整洁排版）：

```text
PROVIDES="u-boot virtual/bootloader"
PV="2024.04"
SRCREV="0000000000000000000000000000000000000000"
SRC_URI=" file://CVE-2025-24857.patch file://CVE-2024-57254.patch file://CVE-2024-57255.patch file://CVE-2024-57256.patch file://CVE-2024-57257.patch file://CVE-2024-57258-1.patch file://CVE-2024-57258-2.patch file://CVE-2024-57258-3.patch file://CVE-2024-57259.patch file://CVE-2024-42040.patch git://<internal-git-server>/bsp/u-boot-tiger.git;protocol=ssh;branch=main file://boot.cmd"
UBOOT_ENV="boot"
UBOOT_ENV_SUFFIX="scr"
UBOOT_MACHINE="tiger_aarch64_defconfig"
```

一次看四个面。PROVIDES 里虚包在列，`u-boot` 自己也在——配方名默认进入自己的 PROVIDES，看到两个值不用意外。PV 已改写为 2024.04——名实相符落到了变量层面，与上面 show-recipes 显示的 `1:2024.04` 互为印证。SRC_URI 里原 git 行不见了，fork 地址在列；十条 CVE patch 一条不少全在清单里——`:remove` 只摘了该摘的那条 git 行，7.4.2 的重审结论（全部保留）在变量值里看得见。条目顺序也顺带可读：原配方的十条 CVE patch 排在最前，bbappend 追加的 fork git 行居中、`file://boot.cmd` 居末——两刀 `:append` 的先后，在变量值里留着次序。SRCREV 还是全零占位，等 u-boot-tiger 就位替换。

再看全局这一侧，无目标 `bitbake -e`——chapter 5 建立的手法：

```bash
# 验证回填生效（无目标全局查询）
bitbake -e | grep -E "^(UBOOT_MACHINE|PREFERRED_PROVIDER_virtual/bootloader|PREFERRED_VERSION_u-boot|EXTRA_IMAGEDEPENDS)="
```

输出（本机实测回填）：

```text
EXTRA_IMAGEDEPENDS=" trusted-firmware-a virtual/bootloader"
PREFERRED_PROVIDER_virtual/bootloader="u-boot"
PREFERRED_VERSION_u-boot="2024.04%"
UBOOT_MACHINE="tiger_aarch64_defconfig"
```

四行全部生效。`EXTRA_IMAGEDEPENDS` 的值现在是两个——TF-A 和 virtual/bootloader 都在依赖链上了。

`tiger-aarch64.conf` 全文回顾（chapter 5 五段 + chapter 6 一段 + 本章一段）：

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

# ---- TF-A 集成声明 ----
# 镜像构建时把 TF-A 固件拉上依赖链（产物进 deploy 目录，不进 rootfs）；
# TF-A 无 virtual 虚包（单一配方、无竞争需仲裁），认领不走 PREFERRED_PROVIDER
EXTRA_IMAGEDEPENDS += "trusted-firmware-a"
# 锁定 TF-A 版本为 2.10（版本锁定见全书版本表）
PREFERRED_VERSION_trusted-firmware-a ??= "2.10.%"

# ---- U-Boot 集成声明 ----
# tiger 的 U-Boot 平台配置；defconfig 住在 u-boot-tiger 仓库 configs/ 下（7.3.3）
UBOOT_MACHINE ?= "tiger_aarch64_defconfig"
# virtual/bootloader 虚包由 u-boot 提供：edk2-firmware 同为竞争者（7.7.1），必须显式点名
PREFERRED_PROVIDER_virtual/bootloader ??= "u-boot"
# 认 bbappend 改写后的生效 PV：源码基线 v2024.04，PV 已改写为 2024.04（机制见 7.7.3）
PREFERRED_VERSION_u-boot ??= "2024.04%"
# 镜像构建时把 U-Boot 拉上依赖链（产物进 deploy 目录，不进 rootfs）
EXTRA_IMAGEDEPENDS += "virtual/bootloader"
```

#### 7.7.4 整镜验收：报错没有变多

老规矩验收。EXTRA_IMAGEDEPENDS 挂上了 `virtual/bootloader`，这次整镜构建会真的把 u-boot 拉起来编——构建本身也是验收的一部分。u-boot-tiger 仓库就位之前，先用干跑做报错收敛验证：

<!-- 【待验证·阻塞级】整镜完整构建依赖 u-boot-tiger 仓库就位（EXTRA_IMAGEDEPENDS 拉起 u-boot 构建；待验证清单 C-W10 项），构建摘要届时补记；下方报错收敛形态已用 bitbake -n 本地实测回填 -->
```bash
# 干跑整镜，观察报错形态（"预期中的错误"，与 chapter 5/6 的同一组对照看）
bitbake -n core-image-minimal
```

输出（关键行，以本地实际输出为准）：

```text
NOTE: Resolving any missing task queue dependencies
ERROR: Nothing PROVIDES 'virtual/kernel'
linux-dummy PROVIDES virtual/kernel but was skipped: PREFERRED_PROVIDER_virtual/kernel set to linux-tiger, not linux-dummy
linux-yocto-tiny PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-upstream PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-dev PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-rt PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
ERROR: Required build target 'core-image-minimal' has no buildable providers.
Missing or unbuildable dependency chain was: ['core-image-minimal', 'virtual/kernel']
```

和昨天、上周五逐行对比：主消息行一字不差，六条 skipped 提示还是同一批（本次行序是 dummy 领头、其后 tiny、upstream、yocto、dev、rt——行序没有语义，读行集）；收尾的 `no buildable providers` 宣判行是 chapter 6 起进入视野的，今天也在，指向的还是 `virtual/kernel` 那一组。多了 `NOTE: Resolving any missing task queue dependencies` 和 `Missing or unbuildable dependency chain was: [...]` 两行——这是 taskqueue 解析路径差异带来的正常噪音，不影响判据。**报错没有变多，缺的仍然只有 chapter 8 的内核配方**。u-boot 被真正拉起来编的那一刻，要等仓库就位后的完整构建——那是阻塞级验收，届时补记构建摘要。

### 7.8 QEMU 单独验证：`-bios u-boot.bin` 进命令行

先看一眼 deploy 目录今天的新住客：

<!-- 【待验证·阻塞级】deploy 清单依赖 u-boot-tiger 构建实测（待验证清单 C-W3 项）；u-boot-initial-env 是否产出取决于 defconfig，以实测为准。清单已按 ls -1 字典序预排，实测回填时复核 -->
```bash
# 查看 deploy 目录的 U-Boot 产物
ls -1 $BUILDDIR/tmp/deploy/images/tiger-aarch64/
```

输出（关键行，以本地实际输出为准）：

```text
bl1-tiger.bin
bl1.bin
# ...（TF-A 产物同 chapter 6，此处略）
boot-tiger-aarch64-2024.04-r0.scr
boot-tiger-aarch64.scr
boot.scr
u-boot-initial-env
u-boot-initial-env-tiger-aarch64
u-boot-initial-env-tiger-aarch64-2024.04-r0
u-boot-tiger-aarch64-2024.04-r0.bin
u-boot-tiger-aarch64.bin
u-boot.bin
```

指认一下命名逻辑，它和 PV 改写直接挂钩：`u-boot-tiger-aarch64-2024.04-r0.bin` 是实体——命名模板里的版本段来自 `UBOOT_VERSION ?= "${PV}-${PR}"`，7.4.3 那行 `PV = "2024.04"` 一路流到这里，名实相符在文件名上落了地；`u-boot-tiger-aarch64.bin` 和 `u-boot.bin` 是指向它的符号链接，引用方写不带版本的名字。`boot.scr` 一组同理。`u-boot-initial-env` 那组就是 7.6.1 说的出厂环境导出文本，可以直接 `cat` 看内容。

启动验证沿用 chapter 6 的手法：QEMU 二进制来自 qemu-system-native 的 sysroot，`-bios` 直喂 `u-boot.bin`——这次它扮演的是"把 BL33 喂进虚拟机"的角色：

<!-- 【待验证·阻塞级】串口日志全部内容依赖 u-boot-tiger（入口/装载约定、串口配置；待验证清单 C-W4 项），以下为按 U-Boot 常规形态书写的示意，严禁直接采用；banner 版本字符串应为 2024.04 基线 -->
```bash
# 用构建出的 u-boot.bin 单独启动 tiger 虚拟机，观察 U-Boot 串口输出
cd ~/workspace/poky
source oe-init-build-env ../build
$BUILDDIR/tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64 \
    -machine tiger \
    -bios $BUILDDIR/tmp/deploy/images/tiger-aarch64/u-boot.bin \
    -nographic
```

串口日志（示意形态，以本地实际输出为准）：

```text
U-Boot 2024.04-<u-boot-tiger 提交描述> (<构建时间戳>)

DRAM:  1 GiB
# ...（外设初始化信息，以实际输出为准）
Hit any key to stop autoboot:  3
```

banner 第一行先核一件事：版本字符串是 2024.04 基线——PV 改写、deploy 文件名、固件自报版本，三处口径一致，名实相符的最后一处实证。倒计时那行是 autoboot 机制：三秒内按任意键，U-Boot 停在命令行；不按，就执行 bootcmd。按键进去，两条命令把 7.6 讲的东西看在眼里（这段交互输出同为示意形态，以实测为准）：

```text
=> version
U-Boot 2024.04-<u-boot-tiger 提交描述> (<构建时间戳>)

=> printenv
# ...（出厂环境变量清单——即 defconfig 里 CONFIG_EXTRA_ENV_SETTINGS 烧进来的那套，含 bootcmd）
```

`printenv` 打出来的就是路线一烧进二进制的出厂环境，`bootcmd` 变量就在里面——7.6.3 说的兜底角色，此刻是一个能读到的变量值。看完按 Ctrl-C 退出 QEMU。

一个说明落在这里：今天 U-Boot 是**单核**跑的。tiger 的 `-smp 4` 对 QEMU 机器生效，但多核唤醒不是 U-Boot 自己能完成的——CPU 核的启停走 PSCI，而 PSCI 服务由常驻 EL3 的 BL31 提供。`-bios u-boot.bin` 单跑时 BL31 根本不在场，chapter 6 埋的 PSCI 伏笔今天回收一半：知道"单核是因为没人提供服务"，另一半（BL31 在场时怎么供出服务）等 chapter 10 启动链串通。

"昨天是'准备移交，没有东西可移交'。"阿凯盯着命令行提示符，"今天 BL33 自己先会跑了。"

"会跑，还不会接力。"达哥说，"它现在拿不到内核——内核还不存在；拿到了也不知道从哪读——NAND 还没建。但 BL33 这格从今天起是实的了。"

### 7.9 踩坑实录

按惯例交代时间线：这两个坑发生在 7.4 到 7.6 之间——你前面看到的正确文件和顺利验证，都是改回来之后的样子。

#### 7.9.1 踩坑 1：改了 defconfig，构建"成功"，配置没变

写 7.3.3 那份 defconfig 的时候，阿凯在 u-boot-tiger 仓库里改了一处：`CONFIG_EXTRA_ENV_SETTINGS` 里的 bootcmd 骨架调了几行，提交、推送，然后回到构建目录 `bitbake u-boot`。

构建秒回——任务全部命中缓存，`all succeeded`。他起 QEMU 一看，`printenv` 里还是旧的 bootcmd。

<!-- 【待验证·阻塞级】本坑全部演示输出依赖 u-boot-tiger 仓库实测（待验证清单 C-W5 项）；机制成立（SRCREV 不变则 do_fetch 及下游任务签名不变、命中 sstate/下载缓存），具体输出形态以实测回填 -->
"改也改了，构建也成功了，环境变量纹丝不动。"阿凯把三步操作来回查了两遍，才反应过来去看 bbappend——`SRCREV` 还指着昨天的提交。

机制一句话：**BitBake 不知道仓库里发生了什么，它只看 SRCREV 这串哈希**。fork 仓库里 defconfig 改了、推了，bbappend 里的 SRCREV 不动，do_fetch 的输入就没有任何变化——任务签名不变，下载缓存里的旧源码快照照用，下游 do_configure、do_compile 全部命中缓存。整条链安安静静地"成功"，产出和昨天逐字节相同。这不是 bug，是可重现构建的本职工作：**同样的输入必须给同样的产物**——只不过阿凯想要的"输入变化"发生在 BitBake 的视野之外。

这是"什么都不发生"型错误的第三集了：chapter 5 是注释掉认领行后报错一字不变，chapter 6 是依赖没声明时一切照旧，今天是改了源码构建却纹丝不动。共同点是 BitBake 从不主动告诉你"你预期的变化没有进入我的视野"。

修正也是一句话的事：把 SRCREV bump 到新提交。

```bash
# 修正：SRCREV 指向包含 defconfig 修改的新提交（哈希以仓库实际为准）
sed -i 's/^SRCREV = ".*"/SRCREV = "<新提交哈希>"/' \
    ~/workspace/meta-tiger/recipes-bsp/u-boot/u-boot_%.bbappend

# 复验：任务签名变化，do_fetch/do_configure/do_compile 自动重跑
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake u-boot
```

这次 do_fetch 重新拉取、下游任务连锁重跑，QEMU 里 `printenv` 出现了新的 bootcmd。达哥听完复盘，只补了一句："直拉仓库流的纪律就一条——**仓库动一次，SRCREV 跟一次**。写进提交信息里也好，写进流程里也好，总之别指望有人提醒你。"

#### 7.9.2 踩坑 2：boot.cmd 就在旁边，do_fetch 却找不到

第二个坑在 7.6 接 boot script 的时候。阿凯把 `boot.cmd` 随手放在了 `recipes-bsp/u-boot/` 里——和 bbappend 同目录，直觉上"就在旁边"。构建：

```bash
# 重演当时的错误状态：boot.cmd 放在 recipes-bsp/u-boot/（bbappend 旁边），而非 ${PN} 子目录
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake u-boot
```

<!-- 【待验证】报错具体措辞以实测为准（待验证清单 C-W9 项）；任务前缀的 epoch 形态（`1_2024.04`，源自 u-boot-common.inc 的 `PE = "1"`）与 "to download to <本地路径>" 子句已按 fetch2/local.py 源码级结论预写入，最终形态仍以仓库就位后实测为准 -->
输出（关键行，"你可能遇到的错误"，以本地实际输出为准）：

```text
ERROR: u-boot-1_2024.04-r0 do_fetch: Fetcher failure: Unable to find file file://boot.cmd anywhere to download to <本地路径>. The paths that were searched were: <搜索路径列表>
```

`file://boot.cmd` 找不到。chapter 4 学过这套搜索机制：`file://` 附件的查找目录默认只有配方同目录少数几个位置，bbappend 场景靠 `FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"` 把"bbappend 所在目录下的 `${PN}` 同名子目录"加进搜索路径。我们这份 bbappend 的 prepend 指向的是 `recipes-bsp/u-boot/u-boot/`——而 boot.cmd 躺在 `recipes-bsp/u-boot/` 里，差着一层目录。报错里那串"搜索过的路径"其实已经说了答案：列表里没有它躺着的那个目录。

修正：把文件移进 `${PN}` 子目录——就是 7.4.1 建好的那个：

```bash
# 修正：boot.cmd 移入 FILESEXTRAPATHS 覆盖的 ${PN} 同名子目录
mv ~/workspace/meta-tiger/recipes-bsp/u-boot/boot.cmd \
   ~/workspace/meta-tiger/recipes-bsp/u-boot/u-boot/boot.cmd

# 复验构建
bitbake u-boot
```

do_fetch 通过。阿凯把这个坑和 chapter 4 的 patch 目录对照着记在便签上："`file://` 的'旁边'不是人眼看的旁边，是 FILESEXTRAPATHS 算出来的旁边。目录层级差一层，fetch 阶段就出局——这个错死在构建最前面，倒是死得痛快。"

"死得痛快的错都是好错。"达哥说，"怕的是 7.9.1 那种一声不吭的。"

### 7.10 本章小结

一天走完，早上白板上的三个路标全部落地：

- **7.1**：BL33 补位——FIP 正式引入（TF-A 的固件打包格式，BL31/BL33 打包交 BL2 加载）；本章边界划清：只到"U-Boot 自己跑进命令行"，不动 TF-A 一行；`TFA_UBOOT ??= "0"` 旋钮是 meta-arm 预埋的机关，chapter 10 才拧开。
- **7.2**：配方从哪来——这次 OE-Core 自带，连 layer 都不用加，与 chapter 6"借邻居 layer"互为镜像；版本名实错位摆上桌：文件名 2024.01、源码基线 v2024.04，本章解决两次（bbappend 里 PV、conf 里 PREFERRED_VERSION）。
- **7.3**：UBOOT_MACHINE 正式引入——不设则 `uboot-config.bbclass` 解析期 SkipRecipe 整份跳过（与 TF-A 的 invalid 兜底同功不同构）；参数链终点是 `make tiger_aarch64_defconfig`；归属裁定落 machine conf；defconfig 随板支持代码住 u-boot-tiger 仓库，集成态微调走 `.cfg` 片段通道。
- **7.4**：没有旋钮的直拉仓库流——`SRC_URI:remove` 摘原 git 行 + `:append` 追加 fork；`PV = "2024.04"` 名实相符（不引 SRCPV，尊重原配方静态锁 SRCREV 的设计）；换大版本时旧 patch 清单逐条重审是正常工序——十条 CVE patch 头部全是 `Upstream-Status: Backport [<提交号>]`，拿提交号对基线，三档处置；预判全部保留，以实测为准。
- **7.5**：U-Boot 的 dts 和内核的 dts 是两份文件，各自演进；U-Boot 那份经 `CONFIG_DEFAULT_DEVICE_TREE` 随 u-boot.bin 走（是否编进二进制由 defconfig 的 OF_EMBED/OF_SEPARATE 决定，列入 C-W1 核对）。
- **7.6**：环境变量三条路线——CONFIG_EXTRA_ENV_SETTINGS 出厂烧入、uEnv.txt 运行时读文本、boot.scr 编译后脚本；boot.scr 不需要另写 recipe，`UBOOT_ENV` + `UBOOT_ENV_SUFFIX = "scr"` 触发 u-boot.inc 原生管道（条件 DEPENDS 拉 mkimage、do_compile 打包、do_deploy 部署）；boot.cmd 雏形含 bootpart 的 A/B 选择骨架——交付的是打包形态，执行语义是伏笔。
- **7.7**：先查再写第三次，镜像反转——这次虚包真的在，而且 edk2-firmware 在抢，`PREFERRED_PROVIDER` 从"没人听"变成"不写没人替你仲裁"（chapter 2 前瞻的机制兑现、名字认错：提供者是 `u-boot` 不是 `u-boot-tiger`）；juno 惯例三件套 + 版本锁定共四行回填；`PREFERRED_VERSION` 认 bbappend 改写后的生效 PV（平时与文件名版本相等，"认文件名"只是近似），写 `"2024.04%"`——照文件名写 `"2024.01%"` 会收 not available 警告并静默落空；变量级验证在此一次查全（7.4.4 因 SkipRecipe 时序欠下的账）；整镜验收报错没有变多。
- **7.8**：`-bios u-boot.bin` 进命令行——banner 版本字符串、deploy 文件名、PV 三处口径一致；`printenv` 看出厂环境；单核运行是因为 PSCI 服务要 BL31 在场，伏笔回收一半。
- **7.9**：两个坑一静一闹——改了 defconfig 没 bump SRCREV，构建"成功"而配置没变（"什么都不发生"系列第三集，纪律：仓库动一次 SRCREV 跟一次）；boot.cmd 放错目录层级，do_fetch 当场报找不到（FILESEXTRAPATHS 算出来的"旁边"才算旁边）。

本章产出清单：

- `meta-tiger` 新增 `recipes-bsp/u-boot/u-boot_%.bbappend`：源码改指 u-boot-tiger、PV 名实相符、CVE patch 清单重审结论、boot script 接线。
- `meta-tiger` 新增 `recipes-bsp/u-boot/u-boot/boot.cmd`：A/B 分区选择的启动脚本雏形。
- `meta-tiger/conf/machine/tiger-aarch64.conf`：新增第七段（UBOOT_MACHINE / PREFERRED_PROVIDER / PREFERRED_VERSION / EXTRA_IMAGEDEPENDS 四行）。
- deploy 目录新增三件套：`u-boot.bin`、`boot.scr`、`u-boot-initial-env`；QEMU 单独验证 U-Boot 进命令行通过。

提交并打 tag。本章唯一被修改的我方仓库仍是 `meta-tiger`：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Integrate U-Boot for tiger via bbappend and machine config"
git tag chapter7
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter7`；`u-boot-tiger` 是固件组的开发态仓库，本章只读不写、不打 tag（沿用 chapter 4/6 对 qemu-tiger、tf-a-tiger 的惯例）；`poky` 与 `meta-arm` 都是外部上游仓库，保持原样不打 tag。

后续任务清单：

- **task 09 / chapter 8**：集成 Linux Kernel——boot.cmd 里 `ubifsload /boot/Image` 的那两个文件终于要来真的了；内核配方走自写路线，和 chapter 6/7 的复用路线凑成完整的对照组。
- 留到 chapter 10 的三件伏笔：`TFA_UBOOT=1` 旋钮（BL33 进 FIP）、BL31→BL33 移交、PSCI 多核唤醒。

达哥下班前走到白板跟前，把 BL33 那格涂实了，在原来的那行字下面添了一句：BL33 报到，下半链齐活——等内核。

---

**延伸阅读**

1. U-Boot 官方文档 v2024.04（defconfig、环境变量、boot script 与 mkimage）：https://docs.u-boot.org/en/v2024.04/
2. Yocto 变量术语表，UBOOT_MACHINE / UBOOT_ENV / PREFERRED_VERSION / EXTRA_IMAGEDEPENDS 条目：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
3. Yocto BSP 开发手册（bootloader 集成与 MACHINE 配置惯例）：https://docs.yoctoproject.org/5.0/bsp-guide/
4. OpenEmbedded-Core 的 U-Boot 配方与 uboot-config.bbclass（本章引用的全部机制源码，本机路径 `poky/meta/recipes-bsp/u-boot/` 与 `poky/meta/classes-recipe/`）：https://git.openembedded.org/openembedded-core/tree/meta/recipes-bsp/u-boot?h=scarthgap
