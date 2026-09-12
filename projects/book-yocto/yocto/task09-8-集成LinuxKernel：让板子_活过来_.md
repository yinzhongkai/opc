## 8 集成 Linux Kernel：让板子"活过来"

周三早上，阿凯工位。白板上的启动链图又变了个样：BL1、BL2、BL31、BL33 四格都涂实了，最右边最大的一格还空着，格子里写着两个字——Kernel。格子下面那行字是昨天收工时添的：BL33 报到，下半链齐活——等内核。

老周不知什么时候站到了白板前，伸手点了点那个还空着的格子："今天是大头。Kernel 是 BSP 工作的 80%——前面四章加起来，都是在给它搭舞台。"

"从哪开始？"

"先决定一件事。"老周说，"用 linux-yocto，还是自己写 recipe。"

阿凯翻开本子，把前两章的笔记摆出来："chapter 6 借 meta-arm 的现成配方，chapter 7 用 poky 自带的配方加 bbappend——这次照哪个抄？"

"这次我不给候选。"老周把笔帽扣上，"你去把 linux-yocto 那一家子读完，告诉我它多管了哪些事，再说 tiger 用不用得上。结论你自己下。"

### 8.1 用 linux-yocto 还是自己写：决策树

#### 8.1.1 先把 linux-yocto 一家子请出来

候选名单其实 chapter 5 就点过一遍——那天翻 `COMPATIBLE_MACHINE` 兼容名单时，poky 的内核配方全家都露过脸。今天换个读法，不看它们兼容谁，看它们**管什么**。先把目录列全：

```bash
# 列出 poky 的内核配方目录（linux-yocto 配方族全家福）
ls -1 ~/workspace/poky/meta/recipes-kernel/linux/
```

输出（以本地实际输出为准）：

```text
cve-exclusion.inc
cve-exclusion_6.6.inc
generate-cve-exclusions.py
kernel-devsrc.bb
linux-dummy
linux-dummy.bb
linux-yocto-dev.bb
linux-yocto-rt_6.6.bb
linux-yocto-tiny_6.6.bb
linux-yocto.inc
linux-yocto_6.6.bb
```

主角是 `linux-yocto_6.6.bb`——Yocto 官方维护的内核配方，chapter 5 见过它的兼容名单，也见过它派生出的 `linux-yocto-upstream` 变体。两边是兄弟：`linux-yocto-rt`（实时补丁系）、`linux-yocto-tiny`（极简裁剪系）、`linux-yocto-dev`（跟踪上游开发）。老熟人 `linux-dummy` 也住在这——chapter 5 那个空壳，今天的决策跟它的关系是：**不管选哪条路，它今天都要下岗**。

顺带核一眼基线：`linux-yocto_6.6.bb` 的分支是 `v6.6/standard/base`，6.6 系——和全书版本表锁定的 6.6 LTS 同代。也就是说，选不选 linux-yocto，跟"内核版本新旧"无关，两家基线是同一代。决策的关键在机制，不在版本。

#### 8.1.2 读配方：linux-yocto 多管了哪些事

动笔之前先读文件，老规矩。`linux-yocto_6.6.bb` 七十多行，关键段在这：

```bitbake
# 文件路径：~/workspace/poky/meta/recipes-kernel/linux/linux-yocto_6.6.bb（关键行节选）
KBRANCH ?= "v6.6/standard/base"

require recipes-kernel/linux/linux-yocto.inc

# board specific branches
KBRANCH:qemuarm64 ?= "v6.6/standard/qemuarm64"
# ...（其余机器的分支覆盖行，略）

SRCREV_machine:qemuarm64 ?= "39a4fe09d3d795042cc14eb3c78f6a03874c48df"
# ...（其余机器的 SRCREV_machine 覆盖行，略）
SRCREV_machine ?= "2baf8e92ef6ad38945005adf39342b9efb4509ec"
SRCREV_meta ?= "a77e1b965423603456f2d9dbf3de53bb8a3d75af"

SRC_URI = "git://git.yoctoproject.org/linux-yocto.git;name=machine;branch=${KBRANCH};protocol=https \
           git://git.yoctoproject.org/yocto-kernel-cache;type=kmeta;name=meta;branch=yocto-6.6;destsuffix=${KMETA};protocol=https"

LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"
LINUX_VERSION ?= "6.6.144"

PV = "${LINUX_VERSION}+git"

KMETA = "kernel-meta"
```

阿凯把这段读了三遍，在本子上列出 linux-yocto"多管的事"：

1. **两个 git 仓库，不是一个**。SRC_URI 里第一条是内核源码（`linux-yocto.git`，`;name=machine`），第二条是 `yocto-kernel-cache`（`;type=kmeta;name=meta`）——一个专门存内核配置与补丁元数据的仓库，对应修订号 `SRCREV_meta`。所以这份配方的修订号也是一对：`SRCREV_machine` 管源码，`SRCREV_meta` 管元数据。名字后缀跟着 SRC_URI 的 `name=` 走——chapter 6 的 `SRCREV_tfa` 那套规矩，这里是双实例。
2. **一套分支模型**。`KBRANCH` 是默认分支，`KBRANCH:qemuarm64` 这类机器覆盖行给每块板子指自己的分支——机器名挂在变量后缀上做特化。背后还有 `KMACHINE` 的概念：把 Yocto 的 MACHINE 映射到内核仓库里的机器描述。
3. **一套配置片段机制**。`yocto-kernel-cache` 仓库里装的是 `.scc`/`.cfg` 片段，配方经 `KERNEL_FEATURES` 声明叠加哪些片段——linux-yocto.inc 里那几行 `KERNEL_FEATURES:append = " ${@bb.utils.contains('MACHINE_FEATURES', 'vfat', 'cfg/fs/vfat.scc', '', d)}"` 就是活例：机器特性里有 `vfat`，就自动叠一个打开 vfat 支持的 cfg 片段。我们的 `tiger-aarch64.conf` 里恰好有 `vfat`，这套机制对 tiger 是会动的。
4. **一个专属的类**。这份配方 inherit 的不止 kernel，还有 kernel-yocto——上面这些机制（kmeta 仓库、片段合并、分支审计）全是 kernel-yocto.bbclass 实现的。

还有一段藏在 `linux-yocto.inc` 里，不看会错过：

```python
# 文件路径：~/workspace/poky/meta/recipes-kernel/linux/linux-yocto.inc（关键段节选）
LIC_FILES_CHKSUM ?= "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"
# ...（中间略）

# Skip processing of this recipe if it is not explicitly specified as the
# PREFERRED_PROVIDER for virtual/kernel. This avoids network access required
# by the use of AUTOREV SRCREVs, which are the default for this recipe.
python () {
    if d.getVar("KERNEL_PACKAGE_NAME") == "kernel" and d.getVar("PREFERRED_PROVIDER_virtual/kernel") != d.getVar("PN"):
        d.delVar("BB_DONT_CACHE")
        raise bb.parse.SkipRecipe("Set PREFERRED_PROVIDER_virtual/kernel to %s to enable it" % (d.getVar("PN")))
}
```

"等下。"阿凯指着那段 Python，"linux-yocto 的跳过条件不是 COMPATIBLE_MACHINE？"

这正是容易想当然的地方。chapter 5 的整镜报错里，linux-yocto 一族的 skipped 后缀写的是 `incompatible with machine ... (not in COMPATIBLE_MACHINE)`——那是因为它们各自 `.bb` 里的 COMPATIBLE_MACHINE 名单先拦了一道。但即使机器在名单里，这段 inc 里的代码还会再拦一道：**只要 `PREFERRED_PROVIDER_virtual/kernel` 没点名它，整份配方在解析期就 SkipRecipe**（还有个前提：KERNEL_PACKAGE_NAME 保持默认的 "kernel"，8.2 我们的配方不动它）。理由是注释里那句——这族配方的默认形态用 AUTOREV（自动取分支最新提交；严格说是 linux-yocto-dev 被 PREFERRED_PROVIDER 点名后的形态——它默认也是钉死的静态占位 SRCREV，点名后才翻 AUTOREV；6.6 这份的修订号则已钉死在配方里），解析期就得联网问仓库，没被点名的配方不该付这个代价。和 chapter 7 的 uboot-config 兜底同族：都是解析期 SkipRecipe，只是触发条件一个是"变量没设"，一个是"没被点名"。

#### 8.1.3 逐条裁决：tiger 用不上，结论自写

清单列完，对照 tiger 的场景逐条过：

| linux-yocto 多管的事 | tiger 用得上吗 |
|---------------------|---------------|
| 双仓库：源码 + kmeta 元数据 | 用不上。tiger 的板级配置就一份 defconfig 的量，撑不起一个元数据仓库 |
| KBRANCH/KMACHINE 分支模型 | 用不上。tiger 只有一块板、一个分支，模型是为"一套配方服务几十块板"设计的 |
| KERNEL_FEATURES .scc 片段 | 用不上。片段机制的价值在跨板复用与特性组合，tiger 没有第二块板要复用 |
| kernel-yocto.bbclass | 随之不用。它的活是给上面三样服务的 |
| 6.6 LTS 基线、CVE 跟进 | **想要**，但不必靠这份配方——锁定 6.6 基线是自己的仓库策略，不是配方机制 |

"四条用不上，一条想要但可以自己背。"阿凯下了结论，"自写。linux-yocto 是为'一个发行版支持一堆 qemu 板和参考板'设计的配方，多管的那些事恰恰是我们不需要的；tiger 的板级支持本来就该住在 linux-tiger 仓库里演进——defconfig、dts、驱动，都是开发态的活。配方的职责只剩一件：把那个仓库按 6.6 基线编出来。"

"代价呢？"老周问。

"安全补丁的跟进自己背。"阿凯答，"linux-yocto 那边有人维护 6.6 的 backport 队列，我们自写就意味着锁一个 6.6 LTS 基线之后，后续稳定补丁靠固件组在 linux-tiger 仓库里 rebase 跟进。这是自写路线要背的锅，立项时就认了。"

老周点头，把三条集成路线在白板边上并排写下来："到这里，三种走法你全见过真章了——chapter 6 借邻居 layer 的现成配方，chapter 7 用自家现货加 bbappend，chapter 8 从零写。**复用层级越低，自由度越高，背的责任也越多**。以后每接一个组件，先把这张表在脑子里过一遍。"

> **📖 深入阅读**：linux-yocto 的 kmeta 机制（yocto-kernel-cache 仓库、.scc 片段、KBRANCH/KMACHINE 模型）在 Yocto Kernel 开发手册里有完整说明。将来 tiger 若衍生出第二、第三块板，配置开始需要组合复用时，值得回头重估这条路线。延伸阅读见本章末尾。

### 8.2 linux-tiger.bb：从零写一份内核配方

#### 8.2.1 kernel.bbclass 已经备好了什么

从零写不等于从零造。chapter 4 写那份三行探针配方时，什么都得自己来；内核配方不用——OE-Core 有一个专门的内核类备好了整条流水线：**内核类（kernel.bbclass）**——Yocto 核心类文件，为内核配方提供编译、安装、部署的标准流程和钩子。

先看它白送的第一件东西，第 14 行：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/kernel.bbclass（关键行节选）
PROVIDES += "virtual/kernel"
DEPENDS += "virtual/${TARGET_PREFIX}binutils virtual/${TARGET_PREFIX}gcc kmod-native bc-native bison-native"
```

第一行值得停下来和 chapter 5 对照着看：`linux-dummy.bb` 里那行 `PROVIDES += "virtual/kernel"` 是**手写**的——它是空壳，不 inherit kernel，虚包声明只能自己写。而任何 `inherit kernel` 的配方，这一行由类自带，**不需要手写 PROVIDES**。空壳与真配方在纸面上的唯一交集，就是这一行字。

第二行是工具链依赖：交叉编译器、binutils、kmod（模块工具）、bc、bison——内核构建的家当，类里已经声明，配方不用管。

再往下，类里躺着一整条任务链：`do_configure`（把 defconfig 展开成 `.config`）、`do_compile`（编出 KERNEL_IMAGETYPE 指定的内核镜像）、`do_compile_kernelmodules`（编模块）、`do_install`（把产物安装进 ${D}）、`do_deploy`（把产物送进 deploy 目录）。这些任务 8.6 逐个过，这里先记住一件事：**我们写的配方本体可以很短，因为重活全在类里**。这也是和前两章的对照——TF-A 借来的配方里 do_deploy 是作者手写的，U-Boot 的打包管道是 u-boot.inc 手写的；内核这边，标准流程被社区收进了类，成为所有内核配方共享的地基。

#### 8.2.2 逐行写出 linux-tiger.bb

目录沿用 chapter 4 的约定——`recipes-kernel/` 这个一级目录在 meta-tiger 骨架里已经等了三章：

```bash
# 创建内核配方的落位目录
mkdir -p ~/workspace/meta-tiger/recipes-kernel/linux
```

配方全文——本章核心交付物：

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-kernel/linux/linux-tiger.bb
# tiger 平台的 Linux 内核配方。源码来自 linux-tiger 开发态仓库（固件组维护，
# 基于上游 6.6 LTS）；defconfig、dts 等平台代码随仓库演进，本配方只做集成

SUMMARY = "Linux kernel for the tiger board"
DESCRIPTION = "tiger 开发板内核：基于 Linux 6.6 LTS，板级支持（defconfig、设备树、外设驱动）在 linux-tiger 仓库维护"
LICENSE = "GPL-2.0-only"
# 内核源码树根部的 COPYING；md5 待 linux-tiger 仓库就位后核对替换
# （参照：linux-yocto.inc:6 为 d7810fab7487fb0aad327b76f1be7cd7，
#  linux-yocto_6.6.bb:46 覆盖为 6bc538ed5bd9a7fc9398086aedcd7e46）
LIC_FILES_CHKSUM = "file://COPYING;md5=00000000000000000000000000000000"

# kernel.bbclass：自带 PROVIDES += "virtual/kernel"（无需手写），
# 并 inherit kernel-devicetree（KERNEL_DEVICETREE 机制随之就位，8.5）
inherit kernel

# 单一 git 源、无 name= 后缀，修订号变量即 SRCREV（规矩同 chapter 6/7）；
# 全零为占位，待 linux-tiger 仓库就位替换为真实提交——仓库动一次，SRCREV 跟一次
SRC_URI = "git://<internal-git-server>/bsp/linux-tiger.git;protocol=ssh;branch=main"
SRCREV = "0000000000000000000000000000000000000000"

# 单一 git 源解包到 ${WORKDIR}/git（与 u-boot-common.inc 同例）
S = "${WORKDIR}/git"

# 版本口径随 linux-yocto 同族（linux-yocto_6.6.bb: LINUX_VERSION ?= "6.6.144" +
# PV = "${LINUX_VERSION}+git"）：LINUX_VERSION 给大版本，PV 加 +git 后缀表示
# "6.6 基线之上的 git 快照"；machine conf 占位行 "6.6%" 按前缀匹配生效 PV
LINUX_VERSION ?= "6.6"
PV = "${LINUX_VERSION}+git"

# defconfig 住 linux-tiger 仓库 arch/arm64/configs/tiger_defconfig（8.3 详解）：
# kernel.bbclass 并不解释 KBUILD_DEFCONFIG（它是 kernel-yocto.bbclass 的约定），
# 故用 KERNEL_CONFIG_COMMAND 把 do_configure 的配置命令接管过来
KBUILD_DEFCONFIG = "tiger_defconfig"
KERNEL_CONFIG_COMMAND = "oe_runmake -C ${S} O=${B} ${KBUILD_DEFCONFIG}"
```

<!-- 【待验证·阻塞级】SRCREV 为全零占位、LIC_FILES_CHKSUM 的 md5 为占位值（linux-tiger 仓库尚不存在，待验证清单 C-W11 项），定稿前替换为真实提交与实测 md5 -->

逐行过一遍，每行都能回答"这行回答什么问题"：

- **SUMMARY / DESCRIPTION / LICENSE / LIC_FILES_CHKSUM**：声明四件套，chapter 4 探针配方的老规矩——那次是"写个真的骨架试试水"，这次写真的。内核是 GPL-2.0-only，LIC_FILES_CHKSUM 指源码树根部的 `COPYING`；md5 一列等仓库就位后核对，旁边的注释里留了两个参照值（linux-yocto 家两个文件恰好给了两个值，都以实际仓库为准再定夺）。
- **inherit kernel**：整份配方的支点。虚包声明、任务链、工具链依赖，全从这一行来。
- **SRC_URI / SRCREV / S**：直拉仓库流，与前两章同一条纪律。单一 git 源、没有 `name=` 后缀，修订号变量就是光秃秃的 `SRCREV`；全零占位等仓库替换。`S = "${WORKDIR}/git"` 显式声明解包位置，git 单源配方的固定写法。
- **LINUX_VERSION / PV**：版本口径照着 linux-yocto 自家的写法来——`LINUX_VERSION` 声明内核大版本，`PV` 拼上 `+git` 后缀表示这是基线之上的 git 快照。这行的对口验收在下一小节：chapter 5 写下的占位行是 `PREFERRED_VERSION_linux-tiger ??= "6.6%"`，而我们这份配方文件名里没有版本号，PV 全由这两行显式生成——`6.6+git`，通配符按前缀匹配，正好咬住。chapter 7 讲透的那条口径在这里原样生效：**PREFERRED_VERSION 认生效 PV**。
- **KBUILD_DEFCONFIG / KERNEL_CONFIG_COMMAND**：defconfig 的接线，8.3 专门讲——这里先埋个钩子：这两行里有一行**不写不会报错，但不写就等于没写**。
- **刻意不写的几样**：没有 `COMPATIBLE_MACHINE`——单一配方，没有竞争要挡，8.1 读过的 linux-yocto 那两道拦截（兼容名单 + 没被点名就 SkipRecipe）都是多候选场景的设施；也没有 `PROVIDES`——类自带。这份配方的职责就三件：声明身份、指定源码、接 defconfig。

#### 8.2.3 上岗登记与版本认领

验证全是旧艺。先登记上岗：

```bash
# 检查 linux-tiger 配方已被构建系统发现（解析级验证，不触发 fetch）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-recipes linux-tiger
```

输出（本机实测回填）：

```text
=== Matching recipes: ===
linux-tiger:
  meta-tiger           6.6+git
```

版本列显示 `6.6+git`——不是从文件名剥的（文件名根本没带版本号），是配方里 `PV = "${LINUX_VERSION}+git"` 显式生成的。这一行同时完成了 C 位验收：machine conf 里 chapter 5 写下的 `PREFERRED_VERSION_linux-tiger ??= "6.6%"`，前缀 `6.6` 匹配生效 PV `6.6+git`，锁定成立。chapter 7 用一次反向实验钉死的口径——通配符跟着生效 PV 走——这次连实验都不用做，两边都在纸面上看得见。

再查变量终值：

```bash
# 查 linux-tiger 的关键变量终值（解析级；SRCREV 全零占位下 -e 不触发 fetch）
bitbake -e linux-tiger | grep -E "^(PN|PV|PROVIDES|SRC_URI|SRCREV|LINUX_VERSION|KBUILD_DEFCONFIG|KERNEL_CONFIG_COMMAND)="
```

输出（本机实测回填，行序与展开形态均为 `bitbake -e` 原始输出；构建目录按本书规范写作 build）：

```text
KBUILD_DEFCONFIG="tiger_defconfig"
KERNEL_CONFIG_COMMAND="oe_runmake -C /home/<your-username>/workspace/poky/build/tmp/work/tiger_aarch64-poky-linux/linux-tiger/6.6+git/git O=/home/<your-username>/workspace/poky/build/tmp/work/tiger_aarch64-poky-linux/linux-tiger/6.6+git/build tiger_defconfig"
LINUX_VERSION="6.6"
PN="linux-tiger"
PROVIDES="linux-tiger  virtual/kernel"
PV="6.6+git"
SRCREV="0000000000000000000000000000000000000000"
SRC_URI="git://<internal-git-server>/bsp/linux-tiger.git;protocol=ssh;branch=main"
```

这份输出有三个细节值得停下来看。一是行序：SRCREV 排在 SRC_URI 前面——`bitbake -e` 按字母序输出（'R' 排在 '_' 前），跟命令里 grep 参数的顺序无关，chapter 7 的输出块里就见过这一条。二是 `KERNEL_CONFIG_COMMAND` 给的是**展开后的终值**：我们在配方里写的 `${S}`、`${B}`、`${KBUILD_DEFCONFIG}`，在输出里已经全部变成了绝对路径和 `tiger_defconfig`——变量接力到这里跑完，do_configure 实际执行的就是 `oe_runmake -C <git 目录> O=<build 目录> tiger_defconfig`。"BitBake 变量只是信使"，这行输出是又一实证。三是 `PROVIDES` 里两个值中间的双空格——`linux-tiger  virtual/kernel`——两个空格各来一处：bitbake.conf 默认 `PROVIDES:prepend = "${PN} "` 自带一个尾随空格，kernel.bbclass:14 那行 `PROVIDES += "virtual/kernel"` 的 `+=` 又带一个前导空格，就此会师。值本身没毛病。

重点看 `PROVIDES`——`virtual/kernel` 在列，而我们一行 PROVIDES 都没写。kernel.bbclass 第 14 行的自带声明，在变量值里落了地。chapter 5 那个空壳靠手写同一行字顶岗的日子，到头了。

"配方写好了。"阿凯活动手腕，"但我现在有个问题——内核几万个 Kconfig 选项，tiger 这份'该开哪些'的清单，住哪？"

"问到点子上了。"老周说，"这是今天第二件大事。"

### 8.3 defconfig 住哪：三条路线

#### 8.3.1 先把 Kconfig 到 .config 的流转画清楚

chapter 7 给 defconfig 立过名字：一份"这个平台的 Kconfig 选项该开哪些"的清单，是 menuconfig 等一切配置改动的起点。内核这边比 U-Boot 多一层要讲透——配置从清单到二进制，中间走三步，都在 Kbuild 体系里：

```text
defconfig（清单，人维护）
   │  make <xxx>_defconfig：展开
   ▼
.config（全量配置，构建用——几万个选项每个都有定值）
   │  make：Kbuild 读 .config 决定编什么、怎么编
   ▼
Image / 模块 / dtb
```

defconfig 和 .config 的区别值得一句话说死：defconfig 是**增量**——只写和默认值不同的选项，几百行；.config 是**全量**——每个选项都有定值，上万行。人维护增量，构建用全量，中间的展开动作由 Kbuild 完成。8.11 的坑 2 会用到这个区别。

#### 8.3.2 三条路线，一张对照表

清单住哪，社区里有三条成熟路线：

1. **住内核源码树**（in-tree）：defconfig 放在内核仓库的 `arch/arm64/configs/` 下，成为源码的一部分，随内核一起演进、一起 rebase。主线内核自己的 defconfig 全走这条路。
2. **住 layer**：defconfig 作为 `file://defconfig` 附件进 meta-tiger，kernel.bbclass 的 do_configure 有一条专用通道把它拷成 `.config`。
3. **住 kmeta 元数据仓库**：linux-yocto 的路线——defconfig 拆成一堆 cfg 片段，按板按特性组合。8.1 已经裁决过，这条路连同它的类一起不用。

第二条路线的机制值得看一眼源码，因为待会儿要用它作对照：

```bash
# 文件路径：~/workspace/poky/meta/classes-recipe/kernel.bbclass（do_configure 内节选，约 682-686 行）
	# Copy defconfig to .config if .config does not exist. This allows
	# recipes to manage the .config themselves in do_configure:prepend().
	if [ -f "${WORKDIR}/defconfig" ] && [ ! -f "${B}/.config" ]; then
		cp "${WORKDIR}/defconfig" "${B}/.config"
	fi
```

通道很简单：`SRC_URI` 里有 `file://defconfig`，do_fetch 就把它放进 WORKDIR；do_configure 发现 `${B}/.config` 还不存在，就把这份 defconfig 拷过去当起点，然后跑配置命令展开成全量。**这是"defconfig 住 layer"的官方通道**，内核开发态调试时很顺手——改 layer 里的文件比重打包仓库快。

"tiger 选哪条？"阿凯自问自答，顺着开发态纪律往下推，"第一条。和 chapter 7 的 U-Boot defconfig 同一个道理：tiger_defconfig 是板级支持代码，跟着板子支持走，住 linux-tiger 仓库的 `arch/arm64/configs/` 下，跟 dts、驱动一起演进。layer 只做集成，不住功能代码——这条纪律 chapter 6 立的，不换组件。"

"那第二条什么时候用？"老周问。

"调试期临时拧配置、又不想动仓库的时候。"阿凯想了想，补了一句，"但那就意味着同一时期有两份清单真相——仓库一份、layer 一份。短期可以，长期是祸根。主线必须只有一份。"

#### 8.3.3 一个不写不报错、不写等于没写的变量

路线定了，接线时阿凯撞上了一堵软墙。他的第一稿只写了 `KBUILD_DEFCONFIG = "tiger_defconfig"` 一行——这个名字在别处常见，直觉上"声明了就行"。写完他心里不踏实，倒回去查了一手：

```bash
# KBUILD_DEFCONFIG 这个名字，kernel.bbclass 里有处理它的代码吗？
grep -n "KBUILD_DEFCONFIG" ~/workspace/poky/meta/classes-recipe/kernel.bbclass; echo "exit=$?"
```

输出（以本地实际输出为准）：

```text
exit=1
```

**零命中。** kernel.bbclass 从头到尾没有处理 `KBUILD_DEFCONFIG` 的代码。再查它住哪：

```bash
# KBUILD_DEFCONFIG 的处理代码在哪个类里？
grep -n "KBUILD_DEFCONFIG" ~/workspace/poky/meta/classes-recipe/kernel-yocto.bbclass
```

输出（关键行，以本地实际输出为准）：

```text
146:	# If a defconfig is specified via the KBUILD_DEFCONFIG variable, we copy it
154:	if [ -n "${KBUILD_DEFCONFIG}" ]; then
155:		if [ -f "${S}/arch/${ARCH}/configs/${KBUILD_DEFCONFIG}" ]; then
163:				cp -f ${S}/arch/${ARCH}/configs/${KBUILD_DEFCONFIG} ${WORKDIR}/defconfig
169:			bbfatal "A KBUILD_DEFCONFIG '${KBUILD_DEFCONFIG}' was specified, but not present in the source tree (${S}/arch/${ARCH}/configs/)"
```

机制全在 **kernel-yocto.bbclass** 里——8.1 裁决"不用"的那个类。它做的事读得懂：找到 `${S}/arch/${ARCH}/configs/${KBUILD_DEFCONFIG}`，拷成 WORKDIR/defconfig，走 8.3.2 那条官方通道进 `.config`；找不到还知道喊 bbfatal。**`KBUILD_DEFCONFIG` 是 kernel-yocto 一家的约定，不是内核构建的通用变量名。**

"要不是查了这手，这行就白写了。"阿凯后背有点凉，"解析不报错，变量查得到，do_configure 跑默认命令——三个环节全都安安静静，defconfig 就是没生效。"

"这就是为什么要读类。"老周说，"变量名本身没有魔力，魔力全在处理它的代码里。你继承的是 kernel，不是 kernel-yocto。"

补救只需要一行，而且补救方案 kernel.bbclass 自己备好了——do_configure 的最后一行是 `${KERNEL_CONFIG_COMMAND}`，一个可覆写的钩子，默认值在第 636 行：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/kernel.bbclass（关键行节选）
KERNEL_CONFIG_COMMAND ?= "oe_runmake_call -C ${S} O=${B} olddefconfig || oe_runmake -C ${S} O=${B} oldnoconfig"
```

默认动作是 `olddefconfig`：拿现成的 `.config`（比如 file://defconfig 通道拷进来的那份）展开补全。我们的配方覆写它，直接点名 in-tree 的 defconfig 目标——就是 8.2.2 配方里的最后两行：

```bitbake
# linux-tiger.bb 中的接线（覆写 do_configure 的配置命令钩子）
KBUILD_DEFCONFIG = "tiger_defconfig"
KERNEL_CONFIG_COMMAND = "oe_runmake -C ${S} O=${B} ${KBUILD_DEFCONFIG}"
```

展开后执行的就是 `make -C ${S} O=${B} tiger_defconfig`——Kbuild 为 `arch/arm64/configs/` 下的每份 defconfig 自动生成同名 make 目标。这条链和 chapter 7 §7.3.2 那条完全同构：`UBOOT_MACHINE` → `make tiger_aarch64_defconfig`。**BitBake 变量只是信使，真正接信的是组件自己的构建系统**——上一章的话，这一章原样兑现。保留 `KBUILD_DEFCONFIG` 这个变量名不写死进命令里，是给自己留的文档：值只在一处维护，名字本身也在声明"这份清单的出处"。嫌这行有误导性的团队，也可以把 `tiger_defconfig` 直接写进 `KERNEL_CONFIG_COMMAND`、删掉 `KBUILD_DEFCONFIG`——两种写法都对，区别在于值维护在一处，还是名字自带出处。

> **⚠️ 注意**：`KBUILD_DEFCONFIG = "xxx"` 单独写在 `inherit kernel` 的配方里**不生效也不报错**——它是 kernel-yocto.bbclass 的机制。自写内核配方要么配 `KERNEL_CONFIG_COMMAND` 接管（本书做法），要么走 `file://defconfig` 通道。判断依据永远是你 inherit 的那个类里有没有处理代码，grep 一下十秒钟。

<!-- 【待验证·阻塞级】以下为 tiger_defconfig 的示意形态（linux-tiger 仓库尚不存在，待验证清单 C-W11 项），定稿时以仓库实测替换 -->
按这条路线，tiger 的 defconfig 住在 linux-tiger 仓库 `arch/arm64/configs/tiger_defconfig`，关键行的示意形态（以仓库实际内容为准）：

```text
# 文件路径：~/workspace/linux-tiger/arch/arm64/configs/tiger_defconfig（关键行节选，示意形态）
# 串口：PL011 控制台（tiger 的串口，对应 ttyAMA0）
CONFIG_SERIAL_AMBA_PL011=y
CONFIG_SERIAL_AMBA_PL011_CONSOLE=y
# MTD / UBI / UBIFS：NAND 根文件系统的内核侧支持（chapter 9 伏笔）
CONFIG_MTD=y
CONFIG_MTD_UBI=y
CONFIG_UBIFS_FS=y
# 板载外设：I2C EEPROM（at24）与 SPI NOR（m25p80），编成模块
CONFIG_EEPROM_AT24=m
CONFIG_MTD_M25P80=m
# ...（其余平台项，以仓库实际内容为准）
```

串口两行编进内核（`=y`），两个外设编成模块（`=m`）。模块是什么、`=y` 与 `=m` 怎么选，8.6 正式讲——这里先记住一件事：串口必须 `=y`，控制台要从最早期的日志就可用；`=m` 这两个选择带来的打包形态，8.7 讲。

### 8.4 设备树住哪：tiger.dts 的归属

chapter 7 §7.5 划过的那条界限，今天走到另一侧：U-Boot 的 dts 不是内核的 dts，两份文件描述同一块板子，各自演进。U-Boot 那份已经住在 u-boot-tiger 仓库；内核这份——`tiger.dts`——今天落位。

归属问题先摆上桌。两个候选：住 linux-tiger 仓库的 `arch/arm64/boot/dts/`，或者住 meta-tiger layer。阿凯把代价对比列出来：

- **住内核仓库**：dts 和驱动、defconfig 在同一个仓库里演进。内核升级 rebase 时，绑定（binding）的变化、节点的增删和驱动代码同一次提交完成，原子性好。代价：集成态看不见这份文件，改一行要过仓库流程。
- **住 layer**：以 `file://` 附件或 patch 形式进 meta-tiger，集成态可见、改起来快。代价：内核一升级，layer 里的 dts 和仓库里的驱动版本就分叉——8.3.2 那句"两份清单真相是祸根"，对 dts 一字不差地成立。

"还是开发态纪律。"阿凯说，"dts 是板级支持代码，住 linux-tiger 仓库。meta-tiger 只做集成。"老周点头——这个决定昨天做 U-Boot defconfig 时做过一次，今天只是同一个句式落到新组件上。

<!-- 【待验证·阻塞级】以下为 tiger.dts 的示意形态（linux-tiger 仓库尚不存在，待验证清单 C-W11 项）；存放路径的 vendor 子目录层级以仓库实际为准（arm64 惯例为 dts/<vendor>/tiger.dts，层级影响 8.5 的 KERNEL_DEVICETREE 取值形态） -->
内核这份 dts 的骨架示意（以仓库实际内容为准）：

```text
// 文件路径：~/workspace/linux-tiger/arch/arm64/boot/dts/<vendor>/tiger.dts（骨架节选，示意形态）
/dts-v1/;

/ {
	model = "tiger";
	compatible = "tigersemicon,tiger";

	chosen {
		stdout-path = "serial0:115200n8";
	};

	aliases {
		serial0 = &uart0;
	};

	// PL011 串口（地址以 tiger 平台定义为准）
	uart0: uart@<基地址> {
		compatible = "arm,pl011";
		reg = <0x0 0x<基地址> 0x0 0x1000>;
		// ...（时钟、中断，以仓库实际内容为准）
		status = "okay";
	};

	// NAND 控制器节点（chapter 9 伏笔）
	// ...（以仓库实际内容为准）
};
```

`chosen` 里的 `stdout-path` 声明的是默认控制台——内核命令行没有 `console=` 时，内核按它决定主控制台归谁（8.10 我们会显式传 `console=`，到时它退居备胎）；比控制台注册更早的输出，要命令行显式带 `earlycon` 参数，内核才会照它建立早期控制台。串口节点是 8.10 串口日志的硬件侧前提。NAND 控制器节点今天只画个框——chapter 9 做 UBI 根文件系统时，它的分区布局才是主角。

### 8.5 KERNEL_DEVICETREE：告诉构建系统编哪份 dtb

#### 8.5.1 机制：变量是名单，类是工人

dts 文件躺在仓库里，构建系统不会自己去翻——哪份 dts 要编成 dtb，得有人点名。点名的就是 **KERNEL_DEVICETREE**——chapter 2 读项目时立过名字的变量：指定内核需要编译和部署的设备树列表。接收名单的工人是 kernel-devicetree.bbclass。先看一个容易漏的事实：这个类**不用我们 inherit**——kernel.bbclass 的最后一行（约 941 行）自己 `inherit kernel-devicetree`，8.2 那行 `inherit kernel` 把它一并带了进来。

工人的活分三段，源码都在类里：

```bash
# 文件路径：~/workspace/poky/meta/classes-recipe/kernel-devicetree.bbclass（关键段节选，约 74-90 行）
	for dtbf in ${KERNEL_DEVICETREE}; do
		dtb=`normalize_dtb "$dtbf"`
		oe_runmake $dtb CC="${KERNEL_CC} $cc_extra " LD="${KERNEL_LD}"  # ...（其余参数，略）
		# ...
	done
	# （do_install 段）
	for dtbf in ${KERNEL_DEVICETREE}; do
		dtb=`normalize_dtb "$dtbf"`
		dtb_path=`get_real_dtb_path_in_kernel "$dtb"`
		# ...
		install -Dm 0644 $dtb_path ${D}/${KERNEL_DTBDEST}/$dtb
	done
```

读法：对 KERNEL_DEVICETREE 名单里的每个条目，先过 `normalize_dtb` 规整——写全路径会被警告并裁成基名，正确写法就是 dtb 名本身（或带相对前缀）；然后 `oe_runmake <dtb>`——直接以 dtb 名为 make 目标，Kbuild 知道怎么从对应的 dts 编出它；最后 install 进打包目录的 `${KERNEL_DTBDEST}`。do_deploy 里对应的一段再把它送进 deploy 目录——dtb 这边的实体/链接方向和 Image 一族相反：裸名 `tiger.dtb` 是实体，带版本名和带机器名的两个都是指向它的符号链接（8.6.2 指认）。

编译 dtb 的编译器顺带说一句：**dtc（设备树编译器）**——把 .dts 编译成 .dtb 的工具，内核源码树的 `scripts/dtc` 自带一份，构建 dtb 时内核就地编出它来用。所以内核配方不需要像 U-Boot 配方那样往 DEPENDS 里加 `dtc-native`——chapter 7 在 `u-boot_2024.01.bb` 里见过的那行，是 U-Boot 的构建体系要主机侧 dtc；内核这边自给自足。同一个工具，两套构建体系各取所需。

#### 8.5.2 取值形态与 boot.cmd 伏笔的半场兑现

KERNEL_DEVICETREE 的值写在哪？machine conf——dtb 选择是"这块板子长什么样"的陈述，和 KERNEL_IMAGETYPE 同桌。具体回填留到 8.8（那里凑齐整镜验收），这里先把取值形态钉死：

- dts 若直接落在 `arch/arm64/boot/dts/` 下，值就是基名：`"tiger.dtb"`；
- 若落在 vendor 子目录（arm64 的惯例，如 `dts/tigersemicon/tiger.dts`），值要带相对前缀：`"tigersemicon/tiger.dtb"`——类里的 `get_real_dtb_path_in_kernel` 按相对路径找产物。

tiger 的实际层级以 linux-tiger 仓库为准（已列入待验证清单 C-W11），8.8 回填按基名形态写，前缀形态在注释里交代。

写到这，chapter 7 埋的两个文件名可以兑现一半了。boot.cmd 雏形里那两行——`ubifsload ${kernel_addr_r} /boot/Image`、`ubifsload ${fdt_addr_r} /boot/tiger.dtb`——今天之后，**这两个文件在构建系统里都有了出处**：Image 由 8.6 的任务链产出，tiger.dtb 由本节的机制产出。文件名不再是纸面契约。至于 `ubifsload` 本身——NAND 上的 UBI 卷还不存在，那是 chapter 9 递过来的接力棒。

### 8.6 编译产物：Image、dtb、模块一锅出

#### 8.6.1 任务链全景

配方、defconfig、dts 全部就位，构建之前老周让阿凯先把任务链画出来——"等下构建跑几十分钟，你得知道它正在干什么。"阿凯把 kernel.bbclass 的任务和 kernel-devicetree 的 append 段拼成一张图：

**Fig-8-1 linux-tiger 的任务链与产物**

```text
do_fetch / do_unpack / do_patch
   │  从 linux-tiger 仓库取 6.6 基线源码（SRCREV 锁定）
   ▼
do_configure
   │  KERNEL_CONFIG_COMMAND → make tiger_defconfig → ${B}/.config
   ▼
do_compile ──────────────────┬──────────────────┐
   │ make Image              │ kernel-devicetree │ do_compile_kernelmodules
   │ （KERNEL_IMAGETYPE）     │ oe_runmake tiger. │ make modules
   ▼                         │ dtb（KERNEL_      ▼
Image                        │ DEVICETREE）     *.ko（内核模块）
                             ▼
                          tiger.dtb
   └──────────┬─────────────┘
              ▼
do_install → do_package（kernel-module-split 逐模块分包，8.7）
              ▼
do_deploy → deploy 目录（tmp/deploy/images/tiger-aarch64/）
```

三件事出自同一次构建：**Image**（KERNEL_IMAGETYPE 指定，chapter 5 就写进了 machine conf）、**tiger.dtb**（KERNEL_DEVICETREE 指定）、**内核模块**（defconfig 里 `=m` 的选项）。一锅出这个事实 8.9 有大用，先在图上把它看实。

模块这个词今天正式引入：**内核模块（Kernel Module）**——可动态加载的内核功能单元，编译产物为 `.ko` 文件；编入内核镜像（`=y`）还是编成模块（`=m`），由 .config 决定。8.3 的 defconfig 示意里，at24 和 m25p80 走的就是模块路线：串口要从第一行日志就可用，必须 `=y`；EEPROM 和 SPI NOR 属于"用到再加载"的外设，`=m` 让内核镜像瘦一点，也让 8.7 有东西可讲。

#### 8.6.2 构建与产物指认

<!-- 【待验证·阻塞级】构建与全部产物清单依赖 linux-tiger 仓库就位（待验证清单 C-W11/C-W12 项）；deploy 命名形态已按 kernel.bbclass do_deploy（约 813-833 行）与 kernel-artifact-names.bbclass（KERNEL_ARTIFACT_NAME 模板）预排，实测回填时复核 -->
```bash
# 首次全量构建 tiger 内核（预计 20-60 分钟量级，视构建机而定；需 linux-tiger 仓库可达）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake linux-tiger
```

构建完成后，看 deploy 目录的新住客：

```bash
# 查看 deploy 目录的内核产物
ls -1 $BUILDDIR/tmp/deploy/images/tiger-aarch64/
```

输出（关键行，以本地实际输出为准）：

```text
# ...（TF-A、U-Boot 产物同 chapter 6/7，此处略）
Image--6.6+git-r0-tiger-aarch64-<构建时间戳>.bin
Image-tiger-aarch64.bin
Image
modules--6.6+git-r0-tiger-aarch64-<构建时间戳>.tgz
modules-tiger-aarch64.tgz
tiger--6.6+git-r0-tiger-aarch64-<构建时间戳>.dtb
tiger-tiger-aarch64.dtb
tiger.dtb
```

命名逻辑和 chapter 7 的 u-boot.bin 一族同族：版本段 `6.6+git-r0` 一路来自 8.2 的 `LINUX_VERSION`/`PV` 两行——名实相符在内核这边也落了地。但实体与符号链接的方向，Image 和 dtb 两族恰好相反，别眼花：Image 一族里，带版本名的 `Image--6.6+git-r0-...bin` 是 install 出来的实体，`Image` 和 `Image-tiger-aarch64.bin` 是指向它的符号链接（kernel.bbclass 的 do_deploy）；**dtb 一族反过来**——裸名 `tiger.dtb` 才是实体，`tiger--6.6+git-r0-...dtb`（带版本名）和 `tiger-tiger-aarch64.dtb`（带机器名）两个都是指向它的符号链接（kernel-devicetree.bbclass 的 do_deploy 段）。方向相反，留给引用方的稳定名字倒是一样——boot.cmd 里的 `/boot/Image`、`/boot/tiger.dtb`，对应的就是这组裸名。

### 8.7 内核模块的打包：KERNEL_MODULE_AUTOLOAD

模块编出来了，还得说清楚它们怎么变成"包"。kernel.bbclass 开头 inherit 的另一个类 `kernel-module-split` 管这件事：它把每个 `.ko` 打成一个独立的 ipk 包，包名按 `kernel-module-<模块名>` 生成——一个模块一个包，镜像要谁装谁。

看一眼产物形态：

<!-- 【待验证·阻塞级】模块包清单依赖 linux-tiger 构建实测（待验证清单 C-W12/C-W16 项）；at24 是否成包取决于 defconfig 中 CONFIG_EEPROM_AT24=m 的实际形态；包名内嵌版本段为 KERNEL_VERSION 形态（kernel-module-split.bbclass），具体值以实测回填 -->
```bash
# 查看内核模块的独立打包形态（内核配方 PACKAGE_ARCH 是机器架构（kernel.bbclass），模块包随机器目录）
ls -1 $BUILDDIR/tmp/deploy/ipk/tiger_aarch64/ | grep kernel-module
```

输出（关键行，以本地实际输出为准）：

```text
kernel-module-at24-6.6.x_6.6+git-r0_tiger_aarch64.ipk
kernel-module-m25p80-6.6.x_6.6+git-r0_tiger_aarch64.ipk
# ...（其余 =m 模块各成一包，以实际构建为准）
```

包名里有两段版本，各说各的话：模块名后面那段是 **KERNEL_VERSION**——从构建出的内核源码树提取的实际版本（6.6 基线一般是 `6.6.x` 形态，kernel-module-split 按它拼包名后缀）；下划线后面那段是**包版本与包修订的合写** `${PKGV}-${PKGR}`——PKGV（包版本变量，默认取 PV 的值）在这里是 `6.6+git`，PKGR（包修订变量，默认取 PR）是 `r0`，正是 8.2 的 `LINUX_VERSION`/`PV` 两行一路传下来的版本。名实相符在这里有两层：包说的是"哪个版本的内核编的"和"哪份配方打的"，两件事。一个模块一个包，打包形态到此确认——但"打进包"和"开机自动加载"是两件事。tiger 的板载 EEPROM 是固定资产，开机就该认出它来，不能指望谁手工 modprobe（Linux 用户空间加载模块的命令）。声明这件事的变量是 **内核模块自动加载（KERNEL_MODULE_AUTOLOAD）**——BitBake 变量，声明需要在启动时自动 modprobe 的内核模块名列表。写法是模块名清单，在内核配方或 machine conf 里声明皆可（本书不落盘这一行——原因下面说）：

```bitbake
# 示意（本章不落盘）：声明 at24 开机自动加载
KERNEL_MODULE_AUTOLOAD = "at24"
```

边界要在这里划死，免得这一节看着像没做完：**本章只承诺"打包正确"**。自动加载的运行效果——开机后 EEPROM 真的被认出——发生在根文件系统里：modprobe 是用户空间命令，模块包要被 IMAGE_INSTALL 装进 rootfs、加载动作由 rootfs 里的启动流程执行。而 tiger 的 rootfs 是 chapter 9 才建的东西。所以本章的验收清单里，模块这一项只到"deploy/ipk 里查得到包"为止；`KERNEL_MODULE_AUTOLOAD` 这行连同它的运行验证，都是 chapter 9 之后的事。

"先把包打对，把声明的姿势学会。"老周说，"加载效果是没有观众的戏——rootfs 不在场，演了也没人看。"

### 8.8 回填与整镜验收：报错没有了

#### 8.8.1 machine conf 第八段：只加一行

轮到 machine 配置收尾。chapter 5 写下两行占位时，注释里有句原话——"配方到位后无需再动本文件"。今天配方到位了，验证这句话：两行占位**原样不动**，只追加第八段一行（dtb 名单，8.5 裁定的归属）：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接 chapter 7 全文追加）

# ---- 内核设备树 ----
# tiger 内核要编译的 dtb；dts 住 linux-tiger 仓库 arch/arm64/boot/dts/ 下（8.4/8.5）；
# 若 dts 落在 vendor 子目录，值带相对前缀（如 "tigersemicon/tiger.dtb"），以仓库实际为准
KERNEL_DEVICETREE ?= "tiger.dtb"
```

`tiger-aarch64.conf` 全文回顾（chapter 5 五段 + chapter 6 一段 + chapter 7 一段 + 本章一段）：

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

# ---- 内核设备树 ----
# tiger 内核要编译的 dtb；dts 住 linux-tiger 仓库 arch/arm64/boot/dts/ 下（8.4/8.5）；
# 若 dts 落在 vendor 子目录，值带相对前缀（如 "tigersemicon/tiger.dtb"），以仓库实际为准
KERNEL_DEVICETREE ?= "tiger.dtb"
```

八个段落，每段都能说出是哪一章加的、回答什么问题。这份文件本身就是这本书的进度条。

#### 8.8.2 整镜干跑：占位行写下以来第一次全绿

验收时刻。占位行写下以来，前两章结尾做的是同一个验收、读同一份报错——判据一直是"报错没有变多"。今天的判据换一个：

```bash
# 干跑整镜构建计划（配方骨架 + 全零 SRCREV 下，-n 不触发 fetch，可解析级验收）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -n core-image-minimal
```

输出（关键行，本机实测回填）：

```text
NOTE: Resolving any missing task queue dependencies
NOTE: Executing Tasks
# ...（3480 行 Running task 输出，另有 17 行 noexec，略）
NOTE: Tasks Summary: Attempted 3497 tasks of which 0 didn't need to be rerun and all succeeded.
```

没有了。`Nothing PROVIDES 'virtual/kernel'` 没有了，六条 skipped 提示没有了，`no buildable providers` 的宣判行没有了。chapter 5 有过两次字面上的全绿——那是 linux-dummy 顶岗的假绿；占位行写下之后，那一族报错才钉进每次整镜构建里。今天，它第一次一行不剩。判据从"没有变多"翻转为**"没有了"**——因为缺的那件东西，今天补上了。

仪式感的一刻留给了 chapter 5 的那条查询。那天它回答 `PN="linux-dummy"`，今天再跑一次：

```bash
# 查 virtual/kernel 当前落到哪个配方头上
bitbake -e virtual/kernel | grep ^PN=
```

输出（本机实测回填）：

```text
PN="linux-tiger"
```

`linux-dummy` 的沉默谎言正式终结。chapter 5 那课——"不报错的构建，比报错的构建更危险"——到今天才算上完整：占位行先把假话掰成显式报错，真配方再把报错消掉。两个动作缺一个，今天这行都翻不过来。

占位者本人的现状也顺手查了（本机实测）：`bitbake-layers show-recipes linux-dummy` 的回答里，layer 列是 `meta`，版本列是 `unknown (skipped: PREFERRED_PROVIDER_virtual/kernel set to linux-tiger, not linux-dummy)`——从 chapter 5 的"无声顶岗"，到今天被点名规则显式跳过，白纸黑字写着自己的下场。

阿凯把 5.5.3 的三步查证套路在旁边补了第四行注释："第四步：配方写完后，用同一条查询确认上岗者真的换了人。**查证手法首尾相同，中间隔着两章。**"

> **💡 提示**：全绿的 `-n` 只是解析与排队层面的验收——它证明依赖闭环了，不证明能编过。真正的整镜构建要等 linux-tiger 仓库就位（SRCREV 替换为真实提交）后跑，届时 `bitbake core-image-minimal` 会把 TF-A、U-Boot、内核全部拉起来编。那是阻塞级验收，已列入待验证清单。

### 8.9 sstate 与内核：改一行 dts 的代价

下午，阿凯在笔记本上算一笔账，算完去找老周："固件组同事说，他在 dts 里改了一行，内核重编了 40 分钟。Sstate 缓存不是拦着重编的吗？"

"你猜猜为什么。"老周没接话，"任务链你画过了，自己推。"

阿凯把 Fig-8-1 在纸上重画了一遍，推着推着停住了——**任务的粒度是配方级的**。dts 一行改动，经 SRCREV  bump 流进配方，do_unpack/do_patch 重跑是小事，关键是 **do_compile 的签名失效了**。而 do_compile 这一格里，Image 和 dtb 是**一锅出**的，紧随其后的 do_compile_kernelmodules 编模块——BitBake 不知道"你只改了 dtb"，在它的视野里任务才是最小单位：签名变了，整排重跑。Image 重编、模块重编、dtb 重编，四十分钟是这一排的分量。

下游跟着连锁：do_compile 变了，吃它产物的 do_install、do_package、do_deploy 全部签名失效；再往下分两条道——do_package 打出的 ipk 包将来喂给 do_rootfs，do_deploy 送进 deploy 目录的产物由 do_image 一族消费（内核这边是 KERNEL_DEPLOY_DEPEND 挂上来的——它把内核的 deploy 产物挂上镜像任务的依赖链，do_image 因此等内核就绪；TF-A 那行 EXTRA_IMAGEDEPENDS 也是挂在镜像任务上）——将来做镜像时，两条道都得跟着来。一行 dts 的涟漪，一路荡到镜像。链条画出来是这样：

```text
do_compile / do_compile_kernelmodules   ← 签名失效的源头
   ▼
do_install → do_package → do_deploy     ← 吃其产物，连锁失效
              │              │
              ▼              ▼
        do_rootfs        do_image       ← 包通道 / deploy 通道，将来做镜像时跟着来
```

"Sstate 救不了我。"阿凯回过味来，"sstate 缓存的是任务产物，按签名索引。签名失效，缓存里那份就成了别人的东西——chapter 5 TUNE 那次是全村签名失效，这次是 do_compile 一户失效。机制同一个，受灾面不同。"

"推得对。"老周点头，然后补了一刀，"记住两个名字，今天不展开：`do_symlink_kernsrc`——unpack 之后把内核源码树共享到 tmp/work-shared，供树外模块配方用；`do_shared_workdir`——内核编完后，它把工作区里外部模块需要的那部分（头文件、构建中间物）整理共享出去，以后别人写树外内核模块配方，靠它对接内核工作区。这两条任务的存在意味着一件事：**内核工作区不只是你自己的，别在里面乱动**——下个坑就用得上。"

> **💡 提示**：真到了天天改内核的开发期，有专门工具管这件事——**devtool**：它能给配方开一块独立的开发工作区，改动在集成构建的临时目录之外进行。深入阅读指向开发任务手册，本章点到为止。

### 8.10 QEMU 单独验证：kernel panic 是成功画面

收官动作：把内核单独跑起来。手法沿用 chapter 6/7 的同构思路——那次是 `-bios bl1.bin` 单跑 TF-A、`-bios u-boot.bin` 单跑 U-Boot；这次绕开整条启动链，用 QEMU 的 `-kernel` 直通装载，把 Image 和 dtb 直接喂给虚拟机：

<!-- 【待验证·阻塞级】直通启动与全部串口日志依赖 linux-tiger 构建产物与 qemu-tiger 的 -kernel 直通支持（待验证清单 C-W12/C-W13 项；写作前由排字台优先核实 qemu-tiger 的 tiger machine 是否实现 -kernel 直通装载，若不可用按蓝图 §8 预案改验证形态）。以下为按内核常规启动形态书写的示意，严禁直接采用 -->
```bash
# 单独验证内核：QEMU 直通装载 Image + dtb，内核命令行指定 PL011 控制台
cd ~/workspace/poky
source oe-init-build-env ../build
$BUILDDIR/tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64 \
    -machine tiger \
    -kernel $BUILDDIR/tmp/deploy/images/tiger-aarch64/Image \
    -dtb $BUILDDIR/tmp/deploy/images/tiger-aarch64/tiger.dtb \
    -append "console=ttyAMA0,115200" \
    -nographic
```

三个参数逐一说清：`-kernel` 让 QEMU 把内核镜像直接装进内存并跳转——绕过 TF-A 和 U-Boot，本章只验内核自己；`-dtb` 把编出的 tiger.dtb 一并递进去——内核认硬件全靠它；`-append` 是内核命令行，`console=ttyAMA0,115200` 把控制台指到 PL011——和 machine conf 里 `SERIAL_CONSOLES`、`QB_KERNEL_CMDLINE_APPEND` 指向同一个串口，三处口径一致。

串口日志逐行指认（示意形态，以本地实际输出为准）：

```text
Booting Linux on physical CPU 0x0000000000 [0x410fd034]
Linux version 6.6.x-<linux-tiger 提交描述> (<构建者>) (<工具链>) #1 SMP ...
Machine model: tiger
# ...（内存布局解析、memblock 保留区等早期初始化，以实际输出为准）
Serial: AMBA PL011 UART driver
<基地址>.ttyAMA0: ttyAMA0 at MMIO ... (irq = ..., base_baud = ...) is a PL011 ...
printk: console [ttyAMA0] enabled
# ...（各类子系统初始化与外设探测，以实际输出为准）
VFS: Cannot open root device "(null)" or unknown-block(0,0): error -6
Please append a correct "root=" boot option; here are the available partitions:
Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)
```

读法按启动顺序走：第一行，`Booting Linux on physical CPU 0x0`——内核在 primary core（主核）上起跑，方括号里是 CPU 的硬件标识（型号寄存器值）；`Linux version` 自报版本，6.6 基线；`Machine model: tiger`——**这行是 dtb 被读懂的直接证据**，model 字符串来自我们那份 dts 的根节点；中间 PL011 那两行——串口驱动 probe（探测并认领硬件）成功、`console [ttyAMA0] enabled`，从这行起内核日志正式走控制台，8.3 defconfig 里那两行 `=y` 的回报；最后一行，`Kernel panic - not syncing: VFS: Unable to mount root fs`——内核找了一圈根文件系统，找不到，自我了断。

"panic 了。"阿凯盯着最后那行，"这算……成了？"

"这就是成功画面。"老周说，"你倒过来读这行 panic：它说'找不到 rootfs'——意味着它**活到了需要 rootfs 的那一刻**。dtb 读得懂、串口驱得动、内存管起来了、中断控制器认领了、几万行早期初始化全走完了，最后站在用户空间的门槛上，发现门后还没人盖房子。rootfs 是 chapter 9 要递过去的接力棒——今天内核跑到门槛，就是满分。"

阿凯把这条读日志的心法记下来：**内核 panic 不都是坏消息，先看它 panic 在哪**。panic 在"找不到 rootfs"，是边界宣告；panic 若出现在 PL011 probe 之前，那才是内核自己的病。

两个边界说明落在这里。一是**初始内存文件系统（initramfs）**——内核启动后先挂载的临时根文件系统，可以桥接"内核起来"到"真根就位"之间的路——本章不给它上场，tiger 的根文件系统主线在 NAND/UBI 上，initramfs 用不用、怎么用，chapter 9 见。二是 **单核**：今天只有主核在跑。tiger 是 4 核板，`-smp 4` 对 QEMU 机器生效，但其余三个核的唤醒走 PSCI，而 PSCI 服务由常驻 EL3 的 BL31 提供——直通启动里 BL31 不在场，和 chapter 7 `-bios u-boot.bin` 单跑时同一个道理。多核唤醒等 chapter 10 启动链串通后回收，本章不承诺核数。

"deploy 目录里 Image 和 tiger.dtb 并排躺着，内核能独自活到 rootfs 门槛。"阿凯在当天的日志末尾写，"三件固件，都单独会跑了。"

"接下来就是把它们串起来。"老周说，"但串之前，还差一块地——NAND 还空着。下周一，建房子。"

### 8.11 踩坑实录

按惯例交代时间线：这两个坑发生在 8.3 到 8.6 之间，发现都在之后的日子——你前面看到的正确文件和顺利构建，都是改回来之后的样子。

#### 8.11.1 踩坑 1：在构建工作区里改 dts，改动被下一次构建静默抹掉

8.4 定完 dts 归属后，阿凯要验证一处串口节点改动。走仓库流程——改 linux-tiger、提交、推送、bump SRCREV、重建——一圈下来二十分钟起步。他盯上了捷径：构建工作区里不就躺着解出来的内核源码吗？`tmp/work/tiger_aarch64-poky-linux/linux-tiger/6.6+git/git/` 下面，`arch/arm64/boot/dts/` 里的 tiger.dts 改起来只要十秒钟：

```bash
# 错误示范：直接编辑构建工作区里的 dts（请勿模仿——工作区是临时物）
vim $BUILDDIR/tmp/work/tiger_aarch64-poky-linux/linux-tiger/6.6+git/git/arch/arm64/boot/dts/<vendor>/tiger.dts

# 强制重编验证（-f 强制重跑 do_compile）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -c compile -f linux-tiger
```

dtb 重编，QEMU 里验证通过，收工。**次日**，固件组推了另一个改动，阿凯 bump SRCREV，普通构建跑了一遍——再起 QEMU，昨天的串口改动没了。

<!-- 【待验证·阻塞级】本坑的演示输出（任务重跑序列、QEMU 复验现象）依赖 linux-tiger 仓库实测（待验证清单 C-W14 项）；机制成立（do_unpack 重跑即从 SRCREV 快照重新解包，工作区手改随之消失），具体输出形态以实测回填 -->

机制一句话：**工作区是从 SRCREV 快照 unpack 出来的临时物，不是仓库**。SRCREV 一变，do_fetch/do_unpack 签名失效重跑，`git/` 目录按新快照重新解包——手改的那几行不在任何提交里，解包一盖，无声消失。没有报错，没有警告，没有任何一行提示"你的工作区里曾有未入库的改动"。

这是"什么都不发生"系列的第四集了：chapter 5 注释掉认领行、chapter 6 依赖没声明、chapter 7 SRCREV 没 bump，今天是工作区手改。老周听完复盘，把这个系列的共性点破了："四次都是同一句话——**BitBake 的视野之外发生的事，BitBake 不知道，也不会替你知道**。仓库里的提交它看得见，工作区里的手改它看不见。开发态纪律的内核版就一条：内核源码的改动只能发生在 linux-tiger 仓库，集成态工作区**只读**——8.9 说的 do_shared_workdir 还把工作区共享给别人用，更没理由在里面动东西。"

修正没有技术动作，只有流程动作：回到 linux-tiger 仓库改 dts、提交、bump SRCREV。阿凯在便签上写："捷径省了二十分钟，亏了一上午。"

#### 8.11.2 踩坑 2：menuconfig 调好的配置，cleansstate 之后全丢

第二个坑在 8.6 构建之前。阿凯想试一个配置项，`bitbake -c menuconfig linux-tiger`——kernel.bbclass 内置的交互配置任务（inherit cml1），改的是构建目录里的 `.config`——调出来那个熟悉的蓝色界面，改了三个选项，保存退出，构建——产物里确实生效了。他没带出任何文件。

一周后，磁盘吃紧清了一次 sstate（`bitbake -c cleansstate linux-tiger` 连带清了工作区），再构建——三个选项全没了，`.config` 打回 tiger_defconfig 原样。

<!-- 【待验证·阻塞级】本坑的演示输出（menuconfig 改动生效→cleansstate→丢失的对比、savedefconfig 的实际输出）依赖 linux-tiger 仓库实测（待验证清单 C-W15 项）；机制锚点 kernel.bbclass:691-696 已本机核实 -->

机制还是 8.3.1 那张图：menuconfig 改的是 `${B}/.config`——**构建目录里的全量配置，临时物**，和坑 1 的工作区同一个性质。defconfig 才是入库的真相；.config 只是它展开出来的中间形态。工作区一清，中间形态按真相重新展开，没入库的改动自然归零。

丢的那三处还找得回来吗？找不回——它们从没进过任何提交，也不在任何文件里。好在那三个选项阿凯还记得。救场第一步不是找工具，是**把改动调回来**：再开一次 menuconfig，照记忆把三个选项调好，保存退出——`.config` 重新带上那三处。然后趁改动还在场，立刻把它收下来。收割工具 kernel.bbclass 内置好了（约 691-696 行）：

```bash
# 改动还在场时立刻收下：把当前 .config 浓缩回最小 defconfig（生成到 ${B}/defconfig）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -c savedefconfig linux-tiger
```

输出（关键行，以本地实际输出为准）：

```text
Saving defconfig to:
<构建目录>/defconfig
```

**do_savedefconfig**——内核类内置任务：把当前 `.config` 反向浓缩成最小 defconfig（只留和默认值不同的选项），落到 `${B}/defconfig`。8.3.1 说的"defconfig 是增量、.config 是全量"，这个任务就是反方向的那条路。注意它的定位是**收**，不是**找回**——它只能浓缩当下还在场的 `.config`；要是 cleansstate 之后直接跑它，收回来的只是重置后的原样，和 tiger_defconfig 逐行 diff 会是空的。所以顺序不能反：先 menuconfig 把改动调回来，再 savedefconfig 收进 defconfig，落袋为安。拿收下的这份和仓库里的 tiger_defconfig 逐行 diff，确认就是自己要的那三处，然后走正道：

```bash
# 修正闭环：diff 确认后带回 linux-tiger 仓库，提交并 bump SRCREV
diff ~/workspace/linux-tiger/arch/arm64/configs/tiger_defconfig \
     $BUILDDIR/tmp/work/tiger_aarch64-poky-linux/linux-tiger/6.6+git/build/defconfig
# 确认无误后覆盖仓库里的 defconfig、提交、推送
cp $BUILDDIR/tmp/work/tiger_aarch64-poky-linux/linux-tiger/6.6+git/build/defconfig \
   ~/workspace/linux-tiger/arch/arm64/configs/tiger_defconfig
cd ~/workspace/linux-tiger
git add arch/arm64/configs/tiger_defconfig
git commit -m "tiger_defconfig: enable <选项说明>"
# 回到直拉仓库流的纪律：bump linux-tiger.bb 里的 SRCREV 指向新提交（哈希以仓库实际为准）
```

"menuconfig 是个很好的**试验台**，很差的**仓库**。"老周总结，"在里面改东西没有问题，改完不带回 defconfig，等于白改。savedefconfig 也不是后悔药——改动在场才收得回来。记住闭环：menuconfig 试、savedefconfig 收、diff 核对、回仓库提交、bump SRCREV——五步，一步不能少。"

### 8.12 本章小结

一天走完，白板上最大的一格涂实了。早上的决策和后续每一格，逐个回顾：

- **8.1**：linux-yocto 多管了四样事——双 git 仓库（源码 + kmeta 元数据）、KBRANCH/KMACHINE 分支模型、KERNEL_FEATURES cfg 片段、kernel-yocto.bbclass—— tiger 逐条裁决"用不上"，结论自写；它还有道出人意料的门槛：没被 PREFERRED_PROVIDER 点名就解析期 SkipRecipe（与 COMPATIBLE_MACHINE 是第二道拦截）。三条集成路线（借 layer / 现货+bbappend / 从零写）至此凑成完整对照组；自写的代价是安全补丁靠自己 rebase 跟进。
- **8.2**：linux-tiger.bb 从零写——`inherit kernel` 自带 `PROVIDES += "virtual/kernel"`（与 linux-dummy 的手写对照）、整条任务链和工具链依赖；`LINUX_VERSION ?= "6.6"` + `PV = "${LINUX_VERSION}+git"` 名实相符，与占位行 `"6.6%"` 前缀匹配（认生效 PV）；SRCREV 全零占位待仓库替换。
- **8.3**：defconfig 三条路线——in-tree【主线】/ `file://defconfig` 通道（kernel.bbclass do_configure 内拷贝段）/ linux-yocto cfg 片段；`KBUILD_DEFCONFIG` 是 kernel-yocto.bbclass 一家的约定，plain kernel.bbclass 不处理（grep 零命中实证），自写配方用 `KERNEL_CONFIG_COMMAND` 接管，终点 `make tiger_defconfig`——与 U-Boot 的参数链同构。
- **8.4/8.5**：内核这份 dts 住 linux-tiger 仓库（开发态纪律第三度落子）；KERNEL_DEVICETREE 点名名单，kernel-devicetree.bbclass（由 kernel.bbclass 自带 inherit）对每个条目 `oe_runmake <dtb>` 并部署；dtc 内核树自带；boot.cmd 的两个文件名伏笔兑现一半。
- **8.6**：任务链一锅出——do_compile 出 Image、kernel-devicetree 出 dtb、do_compile_kernelmodules 出 .ko，do_deploy 统一进 deploy 目录；deploy 命名与 U-Boot 同族，dtb 一族的实体/链接方向与 Image 相反。
- **8.7**：kernel-module-split 把每个 .ko 打成独立的 `kernel-module-<名>` 包；KERNEL_MODULE_AUTOLOAD 声明开机自动加载——本章只承诺打包形态，运行效果是 chapter 9 之后的事（rootfs 不在场，演了没人看）。
- **8.8**：machine conf 第八段只加 `KERNEL_DEVICETREE` 一行，chapter 5 的占位注释原话兑现；整镜干跑占位行写下以来**第一次全绿**——判据从"报错没有变多"翻转为"报错没有了"；`bitbake -e virtual/kernel | grep ^PN=` 从 linux-dummy 翻成 linux-tiger，沉默谎言终结。
- **8.9**：改一行 dts 重编 40 分钟的账——任务粒度是配方级，do_compile 一族签名失效下游连锁（包通道到 do_rootfs、deploy 通道到 do_image）；sstate 按签名索引救不了失效的任务；do_symlink_kernsrc/do_shared_workdir 与 devtool 点到为止。
- **8.10**：`-kernel Image -dtb tiger.dtb` 直通验证——日志逐行指认到 `Kernel panic: Unable to mount root fs`，panic 即本章成功画面（内核活到了 rootfs 门槛）；initramfs 本章不上场；单核是因为 PSCI 服务要 BL31 在场，多核留 chapter 10。
- **8.11**：两个坑都是"临时物当仓库"——工作区手改 dts 被 do_unpack 重跑静默抹掉（"什么都不发生"系列第四集：BitBake 视野之外的事它不替你知道）；menuconfig 改动没带回 defconfig，cleansstate 后全丢（闭环五步：试、收、核、提交、bump——savedefconfig 是"收"不是"找回"，改动在场才收得回来）。

本章产出清单：

- `meta-tiger` 新增 `recipes-kernel/linux/linux-tiger.bb`：本章核心交付物——inherit kernel、源码指 linux-tiger、PV 名实相符、defconfig 接线。
- `meta-tiger/conf/machine/tiger-aarch64.conf`：新增第八段（KERNEL_DEVICETREE 一行），全文八段。
- deploy 目录新增：`Image`、`tiger.dtb`、模块包一族（构建产物，仓库就位后实测）。
- QEMU 单独验证：内核直通启动跑到 rootfs 门槛（panic 收场）。

提交并打 tag。本章唯一被修改的我方仓库仍是 `meta-tiger`：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Add linux-tiger kernel recipe and KERNEL_DEVICETREE for tiger"
git tag chapter8
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter8`；`linux-tiger` 是固件组的开发态仓库，本章只读不写、不打 tag（沿用 qemu-tiger / tf-a-tiger / u-boot-tiger 的惯例）；`poky` 与 `meta-arm` 保持原样不打。

后续任务清单：

- **task 10 / chapter 9**：NAND + UBI 根文件系统——boot.cmd 里的 `ubi part`、`ubifsmount`、`ubifsload` 终于要真跑了；`IMAGE_FSTYPES` 加 UBI 格式；defconfig 里 MTD/UBI 那几行伏笔回收；模块的自动加载也将第一次有观众。
- 留到 chapter 10 的伏笔（在 chapter 7 三件之上新增一件）：`TFA_UBOOT=1` 旋钮、BL31→BL33→kernel 完整移交、PSCI 多核唤醒，以及——把 panic 换成 login 提示符，第一次开机成功。

老周下班前走到白板跟前，把 Kernel 那格涂实，在下面添了一行：内核报到，启动链三件齐备——差一个能挂的根。

---

**延伸阅读**

1. Yocto Kernel 开发手册（kernel.bbclass、defconfig 管理、cfg fragment 与 kmeta 机制）：https://docs.yoctoproject.org/5.0/kernel-dev/index.html
2. Yocto 变量术语表，KERNEL_DEVICETREE / KERNEL_IMAGETYPE / KERNEL_MODULE_AUTOLOAD / KERNEL_CONFIG_COMMAND / LINUX_VERSION 条目：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
3. Yocto 开发任务手册，devtool 与内核开发工作流（8.9 点到为止的部分）：https://docs.yoctoproject.org/5.0/dev-manual/index.html
4. Linux Kernel 6.6 文档（Kconfig/Kbuild、设备树绑定）：https://docs.kernel.org/6.6/
5. OpenEmbedded-Core 的 kernel.bbclass 与 kernel-devicetree.bbclass（本章引用的全部机制源码，本机路径 `poky/meta/classes-recipe/`）：https://git.openembedded.org/openembedded-core/tree/meta/classes-recipe?h=scarthgap
