## 11 创建 DISTRO：定义发行版策略

周三早上，阿凯工位。终端里还停着昨晚那行 `tiger-aarch64 login:`——全链开机成功的现场没舍得关。达哥端着咖啡路过，就是昨天双赢的那杯。

"先别关。"他在阿凯旁边站住，"把 `local.conf` 和 `tiger-aarch64.conf` 并排打开。给产品定规矩之前，先看看咱们现在按谁的规矩在跑。"

阿凯把两份文件并排在编辑器里摊开，从上到下过了一遍，目光停在 local.conf 靠中间的一行：

```text
DISTRO ?= "poky"
```

"这行……从 chapter 1 建构建目录那天就在，我十章没动过。"

"没动过，不等于没在用。"达哥说，"昨天开机那套用户空间，init 是谁的？特性清单是谁的？包格式是谁的？全是 poky 的。你骑它的发行版策略骑了十章——这没什么丢人的，本来就是拿它当脚手架。但 tiger 是产品，产品要有自己的发行规矩。"他顿了顿，"还有 local.conf 里那行 debug-tweaks，chapter 1 留的空密码便利——那是发行策略，躺在你的个人配置里躺了十章。"

"那我把特性裁剪写进 tiger-aarch64.conf——"阿凯手已经放上键盘。

"这行回答的是硬件问题吗？"达哥一句话把他按住。

阿凯缩回手。这话他自己立的——chapter 5 的 5.1 节，写 machine conf 每一行之前先问这句。"……不是。是软件策略。"

"规矩你立过。"达哥点点头，"现在不是破规矩，是给策略盖个正经房子。今天三步：**认账**——看看 poky 替我们定了什么；**归队**——把散在外面的策略收进来；**分家**——开发态和量产态分开。下班前我要看到 tiger 自己的 DISTRO。"

### 11.1 三张桌子，今天只动第二张

先回指一张旧图。chapter 5 的 5.1 节立过"三张桌子"：MACHINE 管硬件事实，DISTRO 管软件策略，IMAGE 管镜像里装什么。当时阿凯跑过一条三变量查询验证三张桌子各管各的，今天原样再跑一次，看看"现在按谁的规矩在跑"的实物证据：

```bash
# 初始化构建环境（每次新开终端都需要）
cd ~/workspace/poky
source oe-init-build-env ../build

# 三张桌子各看一个变量的最终合并值（同 chapter 5 的 5.1.2）
bitbake -e core-image-minimal | grep -E "^(MACHINE_FEATURES|DISTRO_FEATURES|IMAGE_INSTALL)="
```

输出（关键行，以本地实际输出为准）：

```text
MACHINE_FEATURES="ext2 rtc serial vfat"
DISTRO_FEATURES="acl alsa bluetooth debuginfod ext2 ipv4 ipv6 pcmcia usbgadget usbhost wifi xattr nfs zeroconf pci 3g nfc x11 vfat seccomp opengl ptest multiarch wayland vulkan sysvinit pulseaudio gobject-introspection-data ldconfig"
IMAGE_INSTALL="packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"
```

`MACHINE_FEATURES` 是 tiger 自己的（chapter 5 写的），`IMAGE_INSTALL` 是 core-image-minimal 的，中间那行长串 `DISTRO_FEATURES`——全是 poky 的。这就是"骑了十章"的物证。

chapter 5 的 5.1.3 节末尾，达哥留过一句话："等 chapter 11 做 tiger 自己的 DISTRO 时，才轮到第二张桌子。"今天就是 chapter 11。前十章动的是 MACHINE 这张桌子和 IMAGE 的现成配方，本章全程只动 DISTRO 这一张。

### 11.2 给 DISTRO 盖房子：`conf/distro/tiger-distro.conf`

#### 11.2.1 我新建一个文件，BitBake 凭什么去读它

动手之前阿凯先拦了自己一下："我在 meta-tiger 里新建一个 `tiger-distro.conf`，BitBake 凭什么知道去读它？machine conf 是 local.conf 里 `MACHINE = "tiger-aarch64"` 点名的，distro conf 的入口在哪？"

"问得好，别问我。"达哥指了指屏幕，"bitbake.conf 你读过好几遍了，自己去找。"

阿凯 grep 了一行：

```bash
# 定位 MACHINE / DISTRO 配置的加载点（829 行与 831-832 行，达哥马上要引用）
grep -n -E "conf/(machine|distro)/" ~/workspace/poky/meta/conf/bitbake.conf

# 顺便看看 poky 自己的发行版配置：头部身份声明 + require 接入点
head -40 ~/workspace/poky/meta-poky/conf/distro/poky.conf
grep -n "^require" ~/workspace/poky/meta-poky/conf/distro/poky.conf
```

输出（关键行）：

```text
829:include conf/machine/${MACHINE}.conf
831:include conf/distro/${DISTRO}.conf
832:include conf/distro/defaultsetup.conf
```

require 一族的位置（关键行）：

```text
68:require conf/distro/include/poky-world-exclude.inc
69:require conf/distro/include/no-static-libs.inc
70:require conf/distro/include/yocto-uninative.inc
71:require conf/distro/include/security_flags.inc
```

加载链就此拼全：local.conf 的 `DISTRO ?= "poky"`（这行来自模板文件 `meta-poky/conf/templates/default/local.conf.sample` 第 94 行，`oe-init-build-env` 建构建目录时抄进来的）→ bitbake.conf 第 831 行按 `${DISTRO}` 的值拼出路径 `conf/distro/<名字>.conf` → 沿 BBPATH 逐层找。BBPATH（chapter 3 引入的 BitBake 搜索路径）是 chapter 3 建骨架时 `layer.conf` 里 `BBPATH .= ":${LAYERDIR}"` 挂上来的——meta-tiger 的 `conf/` 目录早在搜索路径里。chapter 3 那句"`conf/distro/` 等 chapter 11 的 `tiger-distro.conf`"，今天兑现；空目录里的 `.gitkeep` 占位符，等的就是现在。

阿凯把 `poky.conf` 通读了一遍：开头（前十几行）是 `DISTRO_NAME`、`DISTRO_VERSION`、`DISTRO_CODENAME`、`TARGET_VENDOR` 一组身份声明，中间是 `DISTRO_FEATURES` 的拼接和几条 `PREFERRED_VERSION`，第 68-71 行是一串 `require conf/distro/include/...`——机制全靠 require 接进来，这个写法下午 tiger 要抄。

达哥只点拨了一句，是今天最容易被忽略的机理："注意 829 行和 831 行的先后——`include conf/machine/${MACHINE}.conf` 在前，`include conf/distro/${DISTRO}.conf` 在后。**机器配置先解析，发行版配置后解析，同一个变量后写的赢。**记住这个顺序，11.3 收拢纪律的时候要用。"

还有一条硬纪律当场立下：**`DISTRO` 的值必须等于文件名**。`DISTRO = "tiger-distro"` 对应的就是某层 `conf/distro/tiger-distro.conf`——名字和文件名是硬绑定的，没有第二套映射。这条纪律的反面教材，本章踩坑实录见。

#### 11.2.2 骨架落盘：名字、版本、厂商段

新文件第一段先落身份声明：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（新建）
# tiger 产品发行版公共策略基座（chapter 11）
# 开发态/量产态两个变体各自 require 本文件后写差异（11.6）

DISTRO_NAME = "tiger IoT Linux"
DISTRO_VERSION = "1.0"
DISTRO_CODENAME = "beacon"

# 厂商段：拼进 TARGET_SYS 三元组（bitbake.conf 第 167-168 行）
TARGET_VENDOR = "-tiger"
```

逐行交代。**DISTRO_NAME** 与 **DISTRO_VERSION** 就地解释：发行版的名字与版本号。两个变量各有各的兜底，出处还不一样——`DISTRO_NAME` 不写，bitbake.conf 第 800 行兜底成 `"OpenEmbedded"`；`DISTRO_VERSION` 不写，`default-distrovars.inc` 第 46 行兜底成 `"nodistro.0"`。一个发行版连名字都没有，SDK 文件名和登录横幅都会露馅。`DISTRO_CODENAME` 是可选的代号，poky 用的是 `"scarthgap"`，tiger 给自己起了一个。

**TARGET_VENDOR** 就地解释：厂商段，`bitbake.conf` 里默认值 `"-oe"`（poky 覆盖成 `"-poky"`），它被拼进 `TARGET_SYS = "${TARGET_ARCH}${TARGET_VENDOR}-${TARGET_OS}"` 这个目标三元组。改成 `-tiger` 之后，三元组从 `aarch64-poky-linux` 变成 `aarch64-tiger-linux`——chapter 8 踩坑时在 workdir 路径里见过的 `tiger_aarch64-poky-linux` 就是它的产物。先记下这个变化：**三元组一变，所有目标配方的工作目录和任务签名跟着变**，下午量 sstate 的时候要用。

#### 11.2.3 切换与首次解析验证

骨架落盘，切过去。local.conf 里那行躺了十章的默认值，今天换掉：

```bitbake
# 文件路径：~/workspace/build/conf/local.conf（编辑）
# 找到这一行并替换：
# 旧：DISTRO ?= "poky"
# 新（11.6 分家后这行会改成 tiger-distro-dev / tiger-distro-prod）：
DISTRO = "tiger-distro"
```

用硬 `=` 不用 `?=`：模板的 `?=` 是"用户没表态就用 poky"的弱默认值；现在是我们明确表态，发行版是项目的正式决策，不是兜底。

解析级验证，零构建成本：

```bash
# 看 DISTRO 一族的最终值
bitbake -e core-image-minimal | grep -E "^(DISTRO|DISTRO_NAME|DISTRO_FEATURES)="
```

输出（关键行，以本地实际输出为准）：

```text
DISTRO="tiger-distro"
DISTRO_NAME="tiger IoT Linux"
DISTRO_FEATURES="acl alsa bluetooth debuginfod ext2 ipv4 ipv6 pcmcia usbgadget usbhost wifi xattr nfs zeroconf pci 3g nfc x11 vfat seccomp pulseaudio sysvinit gobject-introspection-data ldconfig"
```

两个信息。好消息：`DISTRO_NAME` 已经是 tiger 的了——房子被读到了，加载链通的。值得盯着看的是 `DISTRO_FEATURES`：poky 加的桌面族（opengl、wayland、vulkan、ptest、multiarch）确实不见了，可 `x11`、`alsa`、`bluetooth` 还在——它们不是 poky 加的，是 OE-Core 的兜底清单（`default-distrovars.inc` 第 28-29 行，`DISTRO_FEATURES ?= "${DISTRO_FEATURES_DEFAULT}"`）填的。`sysvinit` 也还在，而它既不在兜底清单也不在 poky 追加里——它来自第三处来源，11.3 揭晓。

顺带看词序：这回 `pulseaudio` 排到了 `sysvinit` 前面——回填是按 bitbake.conf 第 925 行的声明顺序逐个 append 的（oe/utils.py 第 128-133 行）；11.1 的 poky 态 `sysvinit` 在前，是因为它由 `init-manager-sysvinit.inc` 在解析期先行 append、回填随后。同一个词两条来路，词序就是两路的收据。

也就是说：换一个空 distro conf，不等于"从零开始"，是换了一套默认。下一节把这张清单真正收到自己手里。

### 11.3 DISTRO_FEATURES：把发行策略收拢到一张清单

认账环节。达哥让阿凯把 11.1 打出的 poky 时代完整清单逐词过堂："tiger 的产品画像你先背一遍。"

"无屏、无音频、有网络、要安全。"

"那你对着清单自己写裁剪结果。我不给答案，写完我挑刺。"

阿凯把 29 个词分成两堆。留下的：`acl`、`ext2`、`ipv4`、`ipv6`、`vfat`、`xattr`、`seccomp`——网络两族、文件权限与扩展属性、安全计算模式，都是画像里点过名的；`bluetooth` 他想当然地归进了"有网络"一族，也留在了堆里。砍掉的：`x11`、`wayland`、`opengl`、`vulkan`（显示一族，无屏板子不需要）、`alsa`、`pulseaudio`（音频，无音频板子不需要）、`pcmcia`、`usbgadget`、`usbhost`、`wifi`、`3g`、`nfc`、`nfs`、`zeroconf`、`pci`、`debuginfod`、`ptest`、`multiarch`（tiger 的硬件事实和调试策略都不沾）。

"完了？"

"完了。"

"咱们板子上有蓝牙吗？"

阿凯一愣，回头看自己的"留下"堆——`bluetooth` 赫然在列。它藏在 OE-Core 兜底清单的中段，他按"poky 加的桌面族"的思路砍，漏了这个既不是 poky 私货、也不是画像所需的家伙。"没有。砍掉。"

分堆到此为止——清单上还有三个词他没动：`sysvinit`、`gobject-introspection-data`、`ldconfig`。它们不是画像取舍的对象，是回填名单上的住户：`sysvinit` 的去留不由这张清单管，11.5.2 交给 `INIT_MANAGER` 定夺；另外两个无害，本章留着，到 11.5.2 末尾再点名。

裁剪结果落盘，连同桌底那个自动回填的拦截：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# 发行版特性：按 tiger 产品画像裁剪（无屏、无音频、有网络、要安全）
DISTRO_FEATURES = "acl ext2 ipv4 ipv6 vfat xattr seccomp"

# pulseaudio 在自动回填名单里（bitbake.conf 第 925 行），无音频板子显式拦下
DISTRO_FEATURES_BACKFILL_CONSIDERED = "pulseaudio"
```

注意用的是硬 `=`：这张清单是本发行版的完整表态，不是在别人清单上修修补补——从别人的清单上 `:remove` 也是一种写法，但那意味着上游默认清单一变，你的系统跟着变。发行策略的要义恰恰是"我的清单我做主"。

> **💡 提示**：为什么写死了 `DISTRO_FEATURES`，`sysvinit`、`pulseaudio` 这类词还可能自己长回来？OE-Core 有两层名单：`DISTRO_FEATURES_DEFAULT`（你没表态时的兜底清单）和 `DISTRO_FEATURES_BACKFILL`（bitbake.conf 第 925 行：`pulseaudio sysvinit gobject-introspection-data ldconfig`）。回填名单里的词会被自动补进 `DISTRO_FEATURES`——除非你在 `DISTRO_FEATURES_BACKFILL_CONSIDERED` 里点名拦截。官方定义见 `documentation.conf` 第 146-147 行。这就是 11.2.3 里 sysvinit"不请自来"的出处；11.5 切 systemd 时还会用到这个机制。回填全清单不必背，知道机制、会查即可。

收拢纪律的 ⚠️ 框，把两个方向的死法一次收拢：

> **⚠️ 注意**：`DISTRO_FEATURES` 这些行只能出现在 distro conf。写进 machine conf 有两种死法，都见过了。方向一"污染"：chapter 2 的踩坑实录演示过——在 machine 配置里 `DISTRO_FEATURES += "systemd"`，poky 的 `DISTRO_FEATURES ?= ...` 是弱默认值，你先赋值它就闭嘴，系统策略被硬件配置悄悄改掉。方向二"被覆盖"：本章 11.2.1 讲过的解析顺序——machine conf 先解析（bitbake.conf 第 829 行）、distro conf 后解析（第 831 行），tiger-distro 用的是硬 `=`，你在 machine 里写的会被后解析的 distro conf 整个盖掉。两个方向同一句话：它不属于 machine conf。

### 11.4 PREFERRED_VERSION：哪些版本归 DISTRO 管

裁剪完，达哥抛了个新问题："咱们 machine conf 里已经有三个 `PREFERRED_VERSION` 了——linux-tiger、u-boot、trusted-firmware-a。要不要趁盖房子，搬进 distro conf？"

阿凯认真想了一会儿，给的答案是"不搬"，并且把理由摆成了裁决表：

| 版本锁 | 去向 | 理由 |
|--------|------|------|
| `PREFERRED_VERSION_linux-tiger ??= "6.6%"` | 留在 machine conf | 内核版本与这块板的启动链、设备树绑定，是 BSP 事实 |
| `PREFERRED_VERSION_u-boot ??= "2024.04%"` | 留在 machine conf | 同上；meta-arm 的 n1sdp.conf 同款惯例，chapter 6 已取证 |
| `PREFERRED_VERSION_trusted-firmware-a ??= "2.10.%"` | 留在 machine conf | 同上 |
| 跨组件的用户空间公共库 | 归 distro conf | 换一块板子不该换策略，换一个产品才换策略 |

"对。"达哥说，"搬家不是什么都要搬——**边界本身就是知识点**。DISTRO 管的是'这个产品线上所有板子共用'的用户空间版本策略。举个例子落一行，意思到了就行："

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# 用户空间版本策略：跨组件公共库归 DISTRO 管（11.4）
PREFERRED_VERSION_openssl ?= "3.%"
```

openssl 是安全件，产品要钉住 3.x 族——Scarthgap 自带的配方是 `openssl_3.5.7.bb`，`3.%` 前缀匹配正好罩住。用 `?=` 是留给 local.conf 或下层变体临时顶改的余地。这一行更多是立规矩：以后凡是"跨组件的用户空间版本"，都往这张桌子上放。

### 11.5 包格式与 init 系统

#### 11.5.1 包格式：显式选 ipk

**包格式类变量（PACKAGE_CLASSES）** 首次正式引入：声明发行版用什么包格式打包所有配方产物，三选一——`package_ipk`、`package_deb`、`package_rpm`。ipk 就是 opkg——目标板上的包管理器——的包格式，Debian deb 的轻量亲戚；本章只借它的包格式，包管理器上不上板是后面的决策。

选 ipk 的理由点到为止：嵌入式惯例、包管理器和元数据开销都小。真正值得说的是核实默认值时发现的两个事实：OE-Core 的默认值（`defaultsetup.conf` 第 16 行）本来就是 `package_ipk`，但 poky 在自己头上把它改成了 `package_rpm`（`poky.conf` 第 33 行）——也就是说，前十章骑 poky 策略时，包产物落在 `tmp/deploy/rpm/`；切到 tiger-distro 并显式写下这行之后，才会变成 `tmp/deploy/ipk/`。哪怕 OE-Core 默认就是它，显式声明防的也是上游默认值漂移——**默认值是别人的决策，显式声明才是自己的策略**。这本身就是发行版思维。

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# 包格式：ipk。OE-Core 默认即 ipk，poky 改成了 rpm——显式选回 ipk（11.5.1）
PACKAGE_CLASSES = "package_ipk"
```

还有一行 poky 替我们干过、现在要自己认账的。`poky.conf` 第 31 行有一句不起眼的 `TCLIBCAPPEND = ""`——它压的是 `defaultsetup.conf` 第 12-13 行的默认值：`TCLIBCAPPEND ?= "-${TCLIBC}"` 加 `TMPDIR .= "${TCLIBCAPPEND}"`，意思是**默认情况下构建目录不叫 `tmp`，叫 `tmp-glibc`**。前十章一直是 `tmp`，是 poky 帮我们置了空；切到自己的 DISTRO，poky.conf 不再被加载，这行保护就没了——不显式置空，整个构建目录改名 `tmp-glibc`，`tmp/deploy/...` 一族路径预言全部扑空（发作形态见 11.9.2 坑 2 的孪生兄弟）。也别指望 chapter 1 在 local.conf 里钉的那行 `TMPDIR = "${TOPDIR}/tmp"` 救命——`defaultsetup.conf` 的 append 排在 local.conf 之后执行（bitbake.conf 第 827 行 vs 第 832 行，同一条解析链），硬赋值照样被续上后缀。

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# C 库后缀置空（poky.conf 第 31 行同款）：不置空则 defaultsetup.conf
# 第 12-13 行把 TMPDIR 拼成 tmp-glibc，全部 tmp/ 路径预言扑空
TCLIBCAPPEND = ""
```

**TCLIBCAPPEND** 就地解释：拼在 `TMPDIR` 尾部的 C 库后缀段，OE 给"同一构建目录多套 C 库并存"留的口子；tiger 是单 C 库（glibc）产品，直接置空，目录名保持 `tmp`。

<!-- 【事实核查注记·留审校裁决】task09（chapter 8）8.7 节已发表内容写的是 `tmp/deploy/ipk/tiger_aarch64/`，但当时 DISTRO=poky，按 poky.conf:33 的 `PACKAGE_CLASSES ?= "package_rpm"` 核实，实际应为 `tmp/deploy/rpm/`。该处带【待验证·阻塞级】C-W12/C-W16 回填标记，回填时应一并勘正为 rpm，或在本章审校阶段裁决处理方式。本节正文按源码核实结果（poky=rpm、tiger-distro=ipk）落字，与 task09 已发表行的出入不在 draft 阶段静默覆盖。 -->

#### 11.5.2 init 系统：一行接入 systemd

init 系统是两个候选的对台戏。**sysvinit** 就地解释：poky 默认的 init，老派的 System V 风格，`/etc/inittab` 加一排启动脚本——昨天 login 之前跑的就是它，实物证据是 11.1 那张清单里 `sysvinit` 在列、`systemd` 缺席。**systemd** 首次正式引入：init 系统的一种现代实现，内核拉起 1 号进程后，由它并行拉起和管理用户空间的全部服务。选型理由一句话：产品的 OTA、日志、看门狗托管都要长在服务管理器上，systemd 是这条路上的主流选项；内部机制本章一律不展开。

切 systemd 怎么写？阿凯的第一反应是 `DISTRO_FEATURES:append = " systemd"` 加几个虚包认领——被达哥拦了："Scarthgap 有现成机制，先去源码里把它挖出来，别手写散件。"

挖出来的机制长这样（全部对照本地 poky 源码核实）：`defaultsetup.conf` 第 20-21 行写着 `INIT_MANAGER ??= "none"` 和 `require conf/distro/include/init-manager-${INIT_MANAGER}.inc`——**INIT_MANAGER** 就地解释：init 系统选择变量，发行版配置声明它，`defaultsetup.conf` 按它的值去 require 对应的机制文件。poky 默认的 sysvinit 也不是写死的散件，是 `poky.conf` 第 77-78 行 `POKY_INIT_MANAGER = "sysvinit"` 加 `INIT_MANAGER ?= "${POKY_INIT_MANAGER}"` 两行接进去的。

所以 tiger 只需要一行：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# init 系统：systemd（11.5.2）——机制由 defaultsetup.conf 的
# init-manager-${INIT_MANAGER}.inc 接入，不手写散件
INIT_MANAGER = "systemd"
```

这一行背后做的事，引用机制文件原文为证：

```bitbake
# 文件路径：~/workspace/poky/meta/conf/distro/include/init-manager-systemd.inc（全文）
# Use systemd for system initialization
DISTRO_FEATURES:append = " systemd usrmerge"
DISTRO_FEATURES_BACKFILL_CONSIDERED:append = " sysvinit"
VIRTUAL-RUNTIME_init_manager ??= "systemd"
VIRTUAL-RUNTIME_initscripts ??= "systemd-compat-units"
VIRTUAL-RUNTIME_login_manager ??= "shadow-base"
VIRTUAL-RUNTIME_dev_manager ??= "systemd"
# systemd hardcodes /root in its source codes, other values are not offically supported
ROOT_HOME ?= "/root"
```

三件事逐条对：第一，`DISTRO_FEATURES` 追加 `systemd`（搭配项 `usrmerge` 一并带上，把 `/bin`、`/sbin` 收进 `/usr` 的布局调整，认得即可）；第二，把 `sysvinit` 点名进回填拦截名单——11.3 那个 💡 框的机制在这里第一次被官方文件用上；第三，定 **VIRTUAL-RUNTIME_init_manager**——就地解释：1 号进程这个虚包的认领声明，`??=` 弱默认值，留了下层变体翻盘的余地。

机制为什么接得上？还是 11.2.1 那条解析链：bitbake.conf 先 include 我们的 distro conf（第 831 行），再 include `defaultsetup.conf`（第 832 行）。等 defaultsetup 拼 `init-manager-${INIT_MANAGER}.inc` 的时候，`INIT_MANAGER` 已经是 `"systemd"` 了。这就是为什么这行写在 distro conf 里恰到好处，写进 local.conf 也一样有效但不合纪律——策略要进仓库。

顺带把安全加固旗标也接进来，poky 同款做法（`poky.conf` 第 71 行）：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# 安全加固编译旗标（poky 同款）：栈保护、PIE、FORTIFY、relro 一族，
# 由该文件追加进 TARGET_CC_ARCH / TARGET_LDFLAGS，随发行版全局生效
require conf/distro/include/security_flags.inc
```

<!-- 【事实核查注记】blueprint KP-3 原拟在 DISTRO_FEATURES 中加入 "security" 一词配合 security_flags；经本地 poky Scarthgap 全库 grep 核实，OE-Core 无任何文件消费 DISTRO_FEATURES 的 "security" 标记，poky 的做法是 poky.conf:71 无条件 require security_flags.inc（该文件把 SECURITY_CFLAGS 追加进 TARGET_CC_ARCH，security_flags.inc:56-59）。本节按核实结果改为 require 机制，不写无消费者的 "security" 词。 -->

> **⚠️ 注意**：systemd 内部（unit 文件、target 层级、服务编排）本章一概不展开。服务自启动怎么落在 tiger 的镜像里，下一章见。

验证一把，还是那条零成本的命令：

```bash
# 看特性清单、init 选择与包格式的最终值
bitbake -e core-image-minimal | grep -E "^(DISTRO_FEATURES|INIT_MANAGER|PACKAGE_CLASSES)="
```

输出（关键行，以本地实际输出为准）：

```text
DISTRO_FEATURES="acl ext2 ipv4 ipv6 vfat xattr seccomp systemd usrmerge gobject-introspection-data ldconfig"
INIT_MANAGER="systemd"
PACKAGE_CLASSES="package_ipk"
```

`systemd` 在列、`sysvinit` 消失、`x11`/`wayland`/`bluetooth` 一族全部不见——裁剪清单生效；回填名单里剩下的 `gobject-introspection-data` 和 `ldconfig` 无害，留着。第二张桌子第一次按 tiger 的规矩摆好了。

### 11.6 开发态 vs 量产态：一次分家

#### 11.6.1 base + delta：一份公共策略，两个变体

下午的戏份从达哥的问题开始："空密码登录这条便利，上产线吗？"

"肯定不上。"

"那现在就要把它从 local.conf 收进发行版——可收进唯一的发行版，开发态的你天天被登录卡住。"达哥在白板上写了两行，"OTA 推补丁要可审计，debug 符号和空密码不能上产线；但开发态没有这些便利，效率减半。**一套产品策略，两种执行态**——开发态和量产态，分家。"

结构套路是 BitBake 配置的标准招式，**base + delta**：公共部分留在 `tiger-distro.conf`，dev、prod 两个文件各自 `require` 它，再各写各的增量。require 的语义 chapter 5 立过——找不到文件就报错，这半句下午还会咬人。

#### 11.6.2 dev 态：debug-tweaks 归队

开发态变体全文：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro-dev.conf（新建）
# 开发态：公共策略 + 调试增量（chapter 11）
require conf/distro/tiger-distro.conf

# 调试便利：空 root 密码等（从 local.conf 搬家而来，见正文）
EXTRA_IMAGE_FEATURES = "debug-tweaks"

# 优化总开关：置 1 后 SELECTED_OPTIMIZATION 取调试档
DEBUG_BUILD = "1"
```

第一行增量是本章"归队"的主角。chapter 1 的 1.3.2 节，local.conf 里写过 `EXTRA_IMAGE_FEATURES ?= "debug-tweaks"`，注释是"让 core-image-minimal 允许空密码 root 登录"——它让前十章的每一次 QEMU 验证都顺畅，但它躺在个人配置里，意味着换一台机器、换一个同事、CI 上起一个干净构建，这个策略就消失了。**发行策略的容身之处是发行版配置，不是 local.conf。** 搬家是两条动作，缺一不可：

```bitbake
# 文件路径：~/workspace/build/conf/local.conf（编辑）
# 找到并删除这一行（搬家的另一半）：
# EXTRA_IMAGE_FEATURES ?= "debug-tweaks"
```

> **💡 提示**：搬家只搬一半是最常见的翻车姿势——dev conf 里加了，local.conf 旧行忘了删。后果比"重复"更糟：local.conf 解析在 distro conf 之前（bitbake.conf 第 827 行 vs 831 行），prod 变体不写 `EXTRA_IMAGE_FEATURES`，local.conf 里残留的旧行就一直生效——**量产态照样带空密码**。11.8.1 的变量级验证里，prod 态的 `EXTRA_IMAGE_FEATURES` 必须是空的，拿它当验收项。

第二行增量 **DEBUG_BUILD** 就地解释：OE 的优化总开关。bitbake.conf 第 670 行的 **SELECTED_OPTIMIZATION** 按它二选一——置了 `"1"` 取 **DEBUG_OPTIMIZATION**（第 669 行，`-Og ${DEBUG_FLAGS} -pipe`），没置取 **FULL_OPTIMIZATION**（第 668 行，`-O2 -pipe ${DEBUG_FLAGS}`）。`-Og` 是"为调试体验保留可调试性"的优化档，比 `-O2` 好下断点，又不至于像 `-O0` 那样慢得失真。它还会顺带把主机侧工具的 `BUILD_OPTIMIZATION`（第 672 行）切到调试档，知道即可。

#### 11.6.3 prod 态：优化、strip、只读根

量产态变体全文：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro-prod.conf（新建）
# 量产态：公共策略 + 收口增量（chapter 11）
require conf/distro/tiger-distro.conf

# 体积优化：-O2 → -Os（默认值出处 bitbake.conf 第 668 行）
# DEBUG_FLAGS 保留：符号在打包阶段被 strip 拆进 -dbg 包，产线镜像不装 -dbg，
# 出问题时拿 -dbg 包离线对符号
FULL_OPTIMIZATION = "-Os -pipe ${DEBUG_FLAGS}"

# 只读根文件系统：只钉在出货镜像 core-image-minimal 上，不全局生效（裁决见正文）
IMAGE_FEATURES:append:pn-core-image-minimal = " read-only-rootfs"
```

三件事。第一件是"不写"：prod 里没有 `EXTRA_IMAGE_FEATURES`，debug-tweaks 随之消失，空 root 密码这条便利随之关闭——回指 chapter 1 的术语条目，debug-tweaks 管的就是空密码与无密码 SSH。

第二件体积优化：`FULL_OPTIMIZATION` 的默认值是 `-O2 -pipe ${DEBUG_FLAGS}`（bitbake.conf 第 668 行，已核实），改法就一处，`-O2` 换成 `-Os`——优化目标从速度换成体积。`DEBUG_FLAGS` 保留的原因连着第三件事的前半：**符号裁剪（strip）** 本章正式引入——移除二进制文件中的调试符号以减小目标板体积。OE 在打包阶段默认就 strip，并把拆下来的符号收进 `-dbg` 包；所以构建期带着 `-g` 不吃亏，产线镜像只要不装 `-dbg` 包，落盘的体积就是裁剪后的。strip 的开关细节点到为止，本章只用它的默认行为。

第三件，**只读根文件系统（read-only-rootfs）** 本章正式引入：`IMAGE_FEATURES` 的可选值，把根文件系统挂成只读，写入走 tmpfs 等易失挂载（overlay 是另一个特性 `overlayfs-etc`，本章不开）——产线设备防掉电损坏、防误改的常规收口。它的落点阿凯斟酌了一阵，最后写成 `:pn-core-image-minimal` 限定，理由两条：其一，这个构建目录还会构建 `core-image-minimal-initramfs`——chapter 9 的 9.7 节那套 initramfs 脚手架，它要在里面 `ubiattach`、`mount`，全局 append 只读会波及它；其二，只读是"出货镜像"的属性，不是"这个发行版下每个镜像"的属性。等 chapter 12 的 `tiger-image.bb` 落地，这行会挪进镜像配方自己手里——那是更正的归宿，本章先用 override 钉住。同族纪律回指 chapter 9 的 9.7.2：变量往哪儿写，先想清楚它的作用域该多大。

<!-- 【写作定夺注记】blueprint 附录第 6 条要求 draft 阶段实测裁决 read-only-rootfs 落点。本环境无构建目录、无法实测 initramfs 受影响形态，按 blueprint 的首选建议落 `:pn-core-image-minimal` 限定，理由（作用域纪律 + initramfs 脚手架）已写进正文；若后续实测发现该限定写法在 Scarthgap 有意外行为，审校阶段改判"挂账到 ch12 tiger-image.bb"。 -->

#### 11.6.4 切换方式：local.conf 一行

分家之后，日常切换就是 local.conf 里的一行：

```bitbake
# 文件路径：~/workspace/build/conf/local.conf（编辑）
# DISTRO 行按需在两个值之间切换：
DISTRO = "tiger-distro-dev"     # 日常开发
# DISTRO = "tiger-distro-prod"  # 产线构建 / 发布前验证
```

变量名与文件名同改——`tiger-distro-dev` 对应 `conf/distro/tiger-distro-dev.conf`，11.2.1 的硬绑定纪律在这里第二次用上。

阿凯切完 dev 态，盯着屏幕想了想，提出一个真实疑问："等下——TARGET_VENDOR 变了，三元组变了，那切一次 DISTRO，sstate 会失效多少？prod 态首次构建要多久？"

达哥没回答："这问题别猜。先把验证做完，踩坑实录里你自己量。"

### 11.7 license 策略：放进来的门槛

分家落定，达哥补了今天最后一个问题："产品要过合规，Yocto 在哪一层拦 license？"

阿凯想了想三张桌子："DISTRO 层。拦什么放什么是产品策略。"

"对。先把现行机制摸清——注意是**现行**，网上一半教程写的是废止机制。"达哥让他去源码里取证。

取证结果（全部对照本地 poky 源码核实）。recipe 侧，一批配方带着商业授权标记——`ffmpeg`、`x264`、`gstreamer1.0-plugins-ugly` 的配方里都有同一行 `LICENSE_FLAGS = "commercial"`；没放行时，该配方在解析期直接被跳过——base.bbclass 第 535-536 行抛出 SkipRecipe，构建任务还没轮到它，它在配方名单里就被除名了；附带的说明原文在第 531 行：`Has a restricted license 'commercial' which is not listed in your LICENSE_FLAGS_ACCEPTED.`。distro 侧就是这两个变量：**LICENSE_FLAGS / LICENSE_FLAGS_ACCEPTED** 首次正式引入——前者是配方给自己挂的门槛标记，后者是发行版的白名单，默认空，即一律不放行。

排除方向另有一个变量：**INCOMPATIBLE_LICENSE** 就地解释——按许可证名整体排除某些配方参与构建，支持通配符，官方文档示例是 `*GPL-3.0*`（`documentation.conf` 第 229 行，官方定义原话是 excluded from the build）；GPLv3 族排除是常见的产品合规诉求，法务细节不展开。

落到 `tiger-distro.conf` 的是两行注释加一段纪律——默认不放行，需要时取消注释：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# license 门槛（11.7）：默认一律不放行商业授权标记的包；
# 确有需要时逐个审计后取消注释——放行是法务决策，不是技术默认值
# LICENSE_FLAGS_ACCEPTED = "commercial"
# 排除方向示例：GPLv3 族排除（按需启用，chapter 16 合规审计时定夺）
# INCOMPATIBLE_LICENSE = "*GPL-3.0*"
```

<!-- 【事实核查注记】blueprint KP-7 示意写法 `LICENSE_FLAGS_ACCEPTED = "commercial_gst-plugins-ugly"` 系旧版细粒度组合形态；经本地 poky Scarthgap 核实，现行配方（ffmpeg / x264 / gstreamer1.0-plugins-ugly）的 LICENSE_FLAGS 均为裸 "commercial"，无 `commercial_<配方名>` 细粒度后缀实例。正文按现行机制落字。另：COMMERCIAL_LICENSE / COMMERCIAL_LICENSE_FLAGS 在 poky 全库 grep 零命中，确认 Scarthgap 已废止，正文 ⚠️ 框已按现行机制教学；glossary 中 COMMERCIAL_LICENSE 条目与现实的出入按 blueprint 指示留审校阶段裁决。 -->

为什么默认不放行？带 `commercial` 标记的包不是 license 违法，是条款需要逐个过——专利授权费、使用范围限制，每一个都是产品决策。发行版的默认姿态应该是"全拦，明示放行"，而不是"默认放行，出事了再查"。

> **⚠️ 注意**：老教程里的 `COMMERCIAL_LICENSE` / `COMMERCIAL_LICENSE_FLAGS` 在 Scarthgap 已经不存在——本章在 poky 全库 grep 过，零命中。老的 `LICENSE_FLAGS_WHITELIST` 也改了名，而且更名不是"自动翻译"：bitbake.conf 第 115 行把它登记进 `BB_RENAMED_VARIABLES`，此后数据层在解析期撞上旧名，会当场抛 `Variable LICENSE_FLAGS_WHITELIST has been renamed to LICENSE_FLAGS_ACCEPTED` 并标记解析失败（data_smart.py 第 553-557 行），解析就此中止。照抄老教程的写法不会"静默生效"，是连解析都过不去。写 license 策略前，先确认教程的年代。

> **📖 深入阅读**：本章只管"放进来的门槛"。构建产物的 license.manifest、SBOM 清单、GPL 源码归档，是 chapter 16 发布与合规的活——本章的白名单和排除项，正是那张清单的输入。

至此，`tiger-distro.conf` 公共基座全文齐了，落纸对一遍：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（全文回顾）
# tiger 产品发行版公共策略基座（chapter 11）
# 开发态/量产态两个变体各自 require 本文件后写差异（11.6）

DISTRO_NAME = "tiger IoT Linux"
DISTRO_VERSION = "1.0"
DISTRO_CODENAME = "beacon"

# 厂商段：拼进 TARGET_SYS 三元组（bitbake.conf 第 167-168 行）
TARGET_VENDOR = "-tiger"

# 发行版特性：按 tiger 产品画像裁剪（无屏、无音频、有网络、要安全）
DISTRO_FEATURES = "acl ext2 ipv4 ipv6 vfat xattr seccomp"
DISTRO_FEATURES_BACKFILL_CONSIDERED = "pulseaudio"

# 包格式：ipk（11.5.1）
PACKAGE_CLASSES = "package_ipk"

# C 库后缀置空（poky 同款，11.5.1）：不置空则 TMPDIR 变 tmp-glibc
TCLIBCAPPEND = ""

# init 系统：systemd（11.5.2）
INIT_MANAGER = "systemd"

# 安全加固编译旗标（poky 同款，11.5.2）
require conf/distro/include/security_flags.inc

# 用户空间版本策略：跨组件用户空间版本归 DISTRO 管（11.4）
PREFERRED_VERSION_openssl ?= "3.%"

# license 门槛（11.7）：默认不放行，放行是法务决策
# LICENSE_FLAGS_ACCEPTED = "commercial"
# INCOMPATIBLE_LICENSE = "*GPL-3.0*"
```

### 11.8 验证：同一个 IMAGE，两种 DISTRO

验证按成本分三级，贵的在后头。

#### 11.8.1 一级：变量级并排 diff（零构建成本）

dev、prod 两态各打一次环境，关键变量并排对照：

```bash
# dev 态（local.conf: DISTRO = "tiger-distro-dev"）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e core-image-minimal | grep -E "^(DISTRO|SELECTED_OPTIMIZATION|EXTRA_IMAGE_FEATURES|IMAGE_FEATURES)="

# 把 local.conf 的 DISTRO 改成 "tiger-distro-prod"，同一条命令再跑一次
```

两态对照表（以本地实际输出为准，`DEBUG_PREFIX_MAP` 展开略）：

| 变量 | tiger-distro-dev | tiger-distro-prod |
|------|------------------|-------------------|
| `DISTRO` | `tiger-distro-dev` | `tiger-distro-prod` |
| `SELECTED_OPTIMIZATION` | `-Og -g ... -pipe`（调试档） | `-Os -pipe -g ...`（体积档） |
| `EXTRA_IMAGE_FEATURES` | `debug-tweaks` | （空——11.6.2 💡 框的验收项） |
| `IMAGE_FEATURES` | `debug-tweaks` | `read-only-rootfs` |

dev 态 `EXTRA_IMAGE_FEATURES` 与 `IMAGE_FEATURES` 同值不是巧合：bitbake.conf 第 913 行写着 `IMAGE_FEATURES += "${EXTRA_IMAGE_FEATURES}"`，前者的内容会整体汇入后者——所以 debug-tweaks 在两行都现身，prod 态两行一起消失。

四行全对上，分家成立。任何一行对不上——尤其 prod 态 `EXTRA_IMAGE_FEATURES` 不是空——回 11.6.2 查 local.conf 的旧行删没删。

#### 11.8.2 二级：镜像级与行为级（耗时长，标注时间）

变量对了，镜像还得真构建出来对。tiger 组依赖的仓库还没就位（C-W 批次待回填），二级验证用 qemuarm64 当对照组——chapter 9 的 build-ubi-demo 开过一个先例，本章照办，另开构建目录，不弄脏主目录：

```bash
# 另开对照组构建目录（沿用 chapter 9 build-ubi-demo 先例）
cd ~/workspace/poky
source oe-init-build-env ../build-distrotest

# 新目录的 bblayers.conf 只有模板自带的 meta / meta-poky / meta-yocto-bsp，
# meta-tiger 不在其中；而 meta-tiger 的 layer.conf 第 20 行声明了
# LAYERDEPENDS = "core meta-arm"，meta-arm 自己又依赖 arm-toolchain 集合——
# 依赖链整条注册，先被依赖者（chapter 6 注册 meta-arm 的同款顺序）：
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm-toolchain
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm
bitbake-layers add-layer ~/workspace/meta-tiger

# 编辑 conf/local.conf：
#   MACHINE = "qemuarm64"
#   DISTRO = "tiger-distro-dev"
#   DL_DIR / SSTATE_DIR 指到主目录的共享缓存

# dev 态构建（首次近全量，约 1-3 小时量级，实测回填）
bitbake core-image-minimal

# 切 prod 态（DISTRO = "tiger-distro-prod"）再构建一次
# 预计时间：切换 DISTRO 后 sstate 大面积失效（11.9.2 量过），又一轮近全量，
# 两态合计数小时级——建议下班前触发第二态
bitbake core-image-minimal
```

"新构建目录是白板，layer 得重新注册——而且依赖链要整条挂。"达哥在旁边补了一句，"只挂 meta-tiger 不挂 meta-arm，`add-layer` 的整配置预校验当场就回滚，chapter 6 注册 meta-arm 时你撞过一回。要是这三步全省了呢？你下一行 `DISTRO = "tiger-distro-dev"` 会直接撞 11.9.1 那张 sanity 体检表——distro conf 沿 BBPATH 找不到，因为 meta-tiger 根本不在路径里。"

体积对比：

```bash
# 两态镜像体积并排（两态构建产物带时间戳共存于 deploy 目录）
ls -l tmp/deploy/images/qemuarm64/*.rootfs.tar.bz2
```

输出（数字实测回填）：dev 态的 tar.bz2 更大——`EXTRA_IMAGE_FEATURES` 带进的内容和调试档的产物都在里面；prod 态更小——`-Os` 加 strip 的叠加效果。

行为对比，两态各 `runqemu` 一次：

```bash
# 启动对照组镜像（slirp 用户态网络，同 chapter 1 的用法）
runqemu qemuarm64 nographic slirp
```

dev 态：空密码 root 直登，和前十章每一次一样。prod 态：`login:` 提示符照常出现，但空密码登录被拒——debug-tweaks 已随发行版移除，这是它第一次以"不在场"的方式被验证。只读确认需要先进系统：prod 态进不去登录，用内核命令行的老手法绕过——在 build-distrotest 的 local.conf 临时加一行 `QB_KERNEL_CMDLINE_APPEND = "init=/bin/sh"`，让内核直接拉 shell 而不过登录（就地一句：这是嵌入式调试的常用手法，本章只借它验证，不构成产品配置），进去后：

```bash
# init=/bin/sh 拿到的 shell 里，确认根文件系统挂载形态
mount | grep ' / '
```

输出（实测回填）：`... on / type ext4 (ro,...)`——`ro` 在列，只读生效。验证完删掉那行临时 append。

#### 11.8.3 三级：tiger 组全链复核（C-W22 待回填）

<!-- 【待验证·阻塞级】C-W22：串口日志逐行形态随 linux-tiger / qemu-tiger / tf-a-tiger / u-boot-tiger 仓库就位后回填，续接 C-W17~C-W21 批 -->

对照组验完，还剩真板组：tiger-aarch64 切到 `tiger-distro-dev` 后，把 chapter 10 的 10.7.3 流程完整重跑一遍——从 BL1 到 login。重点看两处差异：init 换成 systemd 后，`Run /sbin/init` 之后的用户空间拉起日志换了一副面孔（systemd 的 target 序列取代 sysvinit 的 rc 脚本）；登录提示符依旧，空密码直登依旧（dev 态）。串口日志的逐行形态随四个开发态仓库就位后回填——本项挂 C-W22，续接 C-W17~C-W21 批。

一个可以现在就预言的实体变化：构建目录里目标配方的工作路径将从 `tmp/work/tiger_aarch64-poky-linux/...` 变成 `tmp/work/tiger_aarch64-tiger-linux/...`——11.2.2 改 `TARGET_VENDOR` 的实体证据，也是下一个坑的引信。

> **💡 提示**：为后面的章节埋一句——poky.conf 第 25 行的 `SDK_NAME` 模板里带着 `${DISTRO}` 段。SDK 与目标镜像必须出自同一套 DISTRO 策略，chapter 14 交付 SDK 时回来对这句话。

### 11.9 踩坑实录

#### 11.9.1 坑 1：DISTRO 名与文件名错位

阿凯收工前最后一次切换，手滑敲出一个不存在的名字：

```bitbake
# 文件路径：~/workspace/build/conf/local.conf（错误示范，请勿模仿）
# DISTRO 名拼错
DISTRO = "tiger-dsitro"
```

跑 `bitbake core-image-minimal`，构建还没开始就撞上体检：

```text
ERROR:  OE-core's config sanity checker detected a potential misconfiguration.
    Either fix the cause of this error or at your own risk disable the checker (see sanity.conf).
    Following is the list of potential problems / advisories:

    DISTRO 'tiger-dsitro' not found. Please set a valid DISTRO in your local.conf
```

这条报错值得拆开看，因为它**不是解析器给的**。11.2.1 读过，bitbake.conf 第 831 行用的是 `include` 不是 `require`——`include` 找不到文件时静默跳过（chapter 5 立过的语义）。也就是说解析阶段一句话没有，你的 distro conf 根本没被加载，`DISTRO_FEATURES` 悄悄回落到 OE-Core 兜底值，单看解析一切正常。真正的拦网在 sanity.bbclass 第 818-823 行：每次构建前的体检会沿 BBPATH 显式检查 `conf/distro/${DISTRO}.conf` 存不存在（也接受 `conf/distro/include/${DISTRO}.inc` 的替代形式，本章用不上，知道即可），不存在就把问题列进上面那张体检单。它和 chapter 5 的 `MACHINE=tiger-aarch64 is invalid` 是同一张体检表上的两个格子——连输出形态都是同一张 wrapper。

坑的另一半在 distro conf 内部：dev/prod 变体里 `require` 的路径写错（比如 `require conf/distro/tiger-dsitro.conf`），症状不同——`require` 找不到文件是解析期直接抛 ParseError：`Could not include required file conf/distro/tiger-dsitro.conf`。注意报错措辞沿的是 include 的壳：解析器给 require 传入的报错前缀本来就是 include required（ast.py 第 42 行），看报错认指令时别被字面带偏。同一个"名字对不上"，在 local.conf 里发作于构建前体检，在 require 里发作于解析期——记症状更要记机理：**`DISTRO` 的值与 `conf/distro/` 下的文件名硬绑定，查找沿 BBPATH 逐层进行**（回指 chapter 3 的 3.2 节）。

修正：拼写改回 `tiger-distro-dev`，重跑 11.8.1 的变量级验证，四行对照表全对上，收工。

#### 11.9.2 坑 2：切换 DISTRO 后 sstate 大面积失效

达哥留了作业的问题，阿凯回主构建目录自己量。先交代口径：基线这行的命中前提，是主目录前十章攒下的正好是 poky + tiger-aarch64 的缓存——同一台机器、同一套调优，命中天然成立；而本章切完 DISTRO 之后，主目录只跑过 `bitbake -e` 的解析级查询，一个构建任务都没攒过（两态的全量构建排在 11.8.2 的对照组里），tiger 态是真空白。poky 态攒下的缓存条目也不会替 tiger 态顶包——三元组的厂商段从 poky 换成了 tiger，任务签名对不上。所以下面两个数字看结构就好，绝对值取决于各机缓存积累，不必逐字复现。干跑两次，不动真格：

```bash
# 基线：poky 态干跑（local.conf 临时回到 DISTRO ?= "poky"，前十章的缓存全在）
bitbake -n core-image-minimal 2>&1 | tail -3

# 切到 tiger-distro-dev 后再干跑
bitbake -n core-image-minimal 2>&1 | tail -3
```

输出（任务数实测回填，看结构不看绝对值）：

```text
# poky 态（已有构建缓存）：
NOTE: Tasks Summary: Attempted 4xxx tasks of which 4xxx didn't need to be rerun and all succeeded.

# tiger-distro-dev 态：
NOTE: Tasks Summary: Attempted 4xxx tasks of which <很小一个数> didn't need to be rerun and all succeeded.
```

第一行几乎全命中，第二行几乎全待跑。机理不用再猜——11.2.2 改过 `TARGET_VENDOR`，三元组从 `aarch64-poky-linux` 变成 `aarch64-tiger-linux`，所有目标配方的工作目录跟着改名；叠加 `DISTRO_FEATURES`、`SELECTED_OPTIMIZATION` 一族变化，任务签名大面积翻转，sstate 命中崩塌。native 配方不受影响，所以待跑的主要是 target 一侧——但 target 一侧恰恰是大头。

这个坑还有一个没发作的孪生兄弟，阿凯是写下 `TCLIBCAPPEND = ""` 那行之后才意识到的：如果忘了 11.5.1 的置空，`TMPDIR` 会整个改名 `tmp-glibc`——11.8.2 的 `ls tmp/deploy/...`、11.8.3 的路径预言全部扑空，旧 `tmp/` 目录还躺在原地装没事人，排查时极易误判成"构建没跑"。poky 时代这行由 poky.conf 第 31 行代劳，自己的 DISTRO 得自己认账——发行版思维又一条：**别人替你兜过的默认值，切走那天全是你的。**

这个坑的教学点不是"怎么避免"——避免不了，也不该避免（缓存失效正是策略变更生效的证据）。教学点是**预期管理与工程安排**：

- DISTRO 决策要趁早定。项目中期才自建发行版，代价就是一次次"接近全量重建"。
- CI 上 dev/prod 各备一份 sstate 缓存，别让两态互相当小偷。
- 产线构建（prod 态）下班前触发，第二天早上看结果——别在上班时间干等。

"量出来了？"达哥路过看了一眼对照输出。

"接近全量。四个小时上下的量级。"阿凯把数字记进本子，"这算不算今天最贵的两行配置？"

"算。"达哥说，"发行版策略是项目里最便宜的开头、最贵的回头——所以它排在'做工程'的第一章。"

### 11.10 本章小结

白板上的三步回顾：

- **11.1 认账**：三张桌子回指（chapter 5 的 5.1），`bitbake -e` 三行 grep 亮出物证——`DISTRO_FEATURES` 整行都是 poky 的，前十章骑的是别人的发行策略。
- **11.2 盖房子**：加载链摸清——local.conf 的 `DISTRO`（模板 `local.conf.sample` 第 94 行带来）→ bitbake.conf 第 831 行 `include conf/distro/${DISTRO}.conf` → 沿 BBPATH 逐层找；`DISTRO` 值与文件名硬绑定；machine 先解析、distro 后解析。骨架落盘：`DISTRO_NAME` / `DISTRO_VERSION` / `DISTRO_CODENAME` / `TARGET_VENDOR = "-tiger"`（三元组变 `aarch64-tiger-linux`，坑 2 的引信）。
- **11.3 收特性**：`DISTRO_FEATURES` 硬 `=` 裁剪到 7 个词（acl / ext2 / ipv4 / ipv6 / vfat / xattr / seccomp），阿凯自己写清单、漏砍 bluetooth 被达哥一句"咱们板子上有蓝牙吗"逮住；回填名单上另外三个词（sysvinit / gobject-introspection-data / ldconfig）不进画像取舍；`DISTRO_FEATURES_BACKFILL_CONSIDERED` 拦下 pulseaudio 回填；⚠️ 框收拢 machine conf 写策略的两种死法（污染 vs 被覆盖）。
- **11.4 版本裁决**：启动链三个 `PREFERRED_VERSION` 留在 machine conf（BSP 事实，meta-arm 同款惯例），跨组件用户空间版本归 DISTRO（`PREFERRED_VERSION_openssl ?= "3.%"` 示例）；不是什么都要搬，边界本身就是知识点。
- **11.5 包格式与 init**：`PACKAGE_CLASSES = "package_ipk"` 显式声明（OE-Core 默认即 ipk、poky 改成了 rpm——防上游默认值漂移）；`TCLIBCAPPEND = ""` 认账 poky 代劳的置空（不置空则 TMPDIR 变 `tmp-glibc`，全部 `tmp/` 路径扑空）；`INIT_MANAGER = "systemd"` 一行接入，`init-manager-systemd.inc` 原文为证做了三件事（append systemd、拦截 sysvinit 回填、认领 `VIRTUAL-RUNTIME_init_manager`）；`security_flags.inc` 随 poky 惯例接入。systemd 内部留给下一章。
- **11.6 分家**：base + delta——`tiger-distro-dev.conf` / `tiger-distro-prod.conf` 各自 `require` 公共基座再写增量；`EXTRA_IMAGE_FEATURES = "debug-tweaks"` 从 local.conf 正式归队（搬家的另一半：删旧行，💡 框立了 prod 态验收项）；`DEBUG_BUILD = "1"` 切调试档（`-Og`，bitbake.conf 第 668-670 行核实）；prod 态 `-Os`、strip 与 read-only-rootfs 正式引入，后者用 `:pn-core-image-minimal` 限定落点并挂账 chapter 12。
- **11.7 license 门槛**：现行机制 recipe 侧 `LICENSE_FLAGS = "commercial"` ↔ distro 侧 `LICENSE_FLAGS_ACCEPTED` 白名单（未放行=解析期 SkipRecipe 除名，不是构建期报错）；排除方向 `INCOMPATIBLE_LICENSE`（排除的是构建）；`COMMERCIAL_LICENSE` 旧机制在 Scarthgap 全库零命中，⚠️ 框警示老教程（更名机制是解析期报错，不是静默翻译）；清单/SBOM 伏笔指向 chapter 16。
- **11.8 三级验证**：变量级并排 diff（四行对照表，prod 态 `EXTRA_IMAGE_FEATURES` 必须为空）→ qemuarm64 对照组两态构建与体积/行为对比（另开 build-distrotest，layer 依赖链整条重新注册：meta-arm-toolchain → meta-arm → meta-tiger，两态合计数小时级）→ tiger 组全链复核挂 C-W22 待回填。
- **11.9 两个坑**：DISTRO 名拼错（include 静默跳过 + sanity 体检拦网，与 chapter 5 的 MACHINE 体检同一张表）；切换 DISTRO 后 sstate 大面积失效（`bitbake -n` 干跑量出，教学点是预期管理与工程安排），连带点名漏置 `TCLIBCAPPEND` 的孪生坑——`tmp-glibc` 扑空。

本章产出清单：

- `meta-tiger`：新增 `conf/distro/tiger-distro.conf`（公共基座）、`conf/distro/tiger-distro-dev.conf`、`conf/distro/tiger-distro-prod.conf` 三个文件；`conf/distro/.gitkeep` 占位符完成历史使命可删。
- `build/conf/local.conf`：`DISTRO ?= "poky"` 换为 `DISTRO = "tiger-distro-dev"`；删除 chapter 1 留下的 `EXTRA_IMAGE_FEATURES ?= "debug-tweaks"` 旧行。local.conf 从此只剩个人/环境配置——MACHINE、DL_DIR、SSTATE_DIR、并行度，策略全部进了仓库。

提交并打 tag：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Add tiger DISTRO with dev/prod variants"
git tag chapter11
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter11`，它是本章唯一有改动的我方仓库；local.conf 的改动不进仓库（chapter 5 起的老规矩）；`poky`、`meta-arm` 与四个开发态仓库本章无改动，不打。

后续任务清单：

- **task 13 / chapter 12**：`tiger-image.bb` 将建立在 tiger-distro 之上——systemd 服务自启动、IMAGE_FEATURES 与 IMAGE_INSTALL 的辨析、read-only-rootfs 从 override 挪进镜像配方，都以本章的决策为前提。
- 伏笔：SDK 与目标镜像同 DISTRO 策略（chapter 14）；license 清单、SBOM 与源码归档（chapter 16）；ptest 的 DISTRO_FEATURES 开关（chapter 13）。

---

**延伸阅读**

1. Yocto 变量术语表（DISTRO_FEATURES、INIT_MANAGER、PACKAGE_CLASSES、LICENSE_FLAGS_ACCEPTED、INCOMPATIBLE_LICENSE、DEBUG_BUILD 各条目）：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
2. Yocto 开发任务手册（自定义发行版与发行版特性章节）：https://docs.yoctoproject.org/5.0/dev-manual/index.html
3. Scarthgap 迁移指南（变量更名与废弃机制，含 license 白名单一族的变迁）：https://docs.yoctoproject.org/migration-guides/migration-5.0.html
