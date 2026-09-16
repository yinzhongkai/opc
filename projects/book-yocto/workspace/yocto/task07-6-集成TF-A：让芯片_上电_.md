## 6 集成 TF-A：让芯片"上电"

周一上午，阿凯工位。终端历史里还留着上周五那条无目标 `bitbake -e` 的查询结果——`PREFERRED_PROVIDER_virtual/kernel="linux-tiger"`，报错收敛后的状态。chapter 5 的 tag 也打完了。老周端着杯子过来，另一只手里多了支白板笔。

老周在小白板上画出启动链，把 BL1、BL2、BL31 三格圈起来："启动链从最底下开始搭。BL1 是上电后第一个跑的东西，它加载 BL2，BL2 再加载 BL31——这三级都是 TF-A 的活儿，一次构建一起产出。今天就把 TF-A 集成进来。"

"内核那边还报着错呢，先不管？"阿凯问。

"那个错是写给 chapter 8 的，不用管。"老周说，"今天结束时的成功判据也一样——不是'没有报错'，是那个报错**没有变多**。"

阿凯点头，等他往下拆。老周给了三个路标："先搞清楚 TF-A 这三级各干什么、谁来加载谁；再决定一件事——配方自己写还是借别人的；最后接线、构建、用 QEMU 单独把这条链跑一遍。"

"借别人的？"

"到那一步你自己会有答案。"老周端着杯子走了。

### 6.1 启动链全貌：BL1、BL2、BL31 各回答一个问题

#### 6.1.1 把启动链图再画一遍

chapter 2 读项目时，阿凯已经见过启动链的六个阶段，也立过 TF-A、BL1、BL2、BL31、BL33 这些名字。当时那张 Fig-2-2 回答的是"每个阶段对应 Yocto 项目里的哪个产物"；今天要动手构建，得把图按"谁加载谁"再画一遍。

**Fig-6-1 tiger 启动链：加载关系与本章产出范围**

```text
上电
 │
 ▼
┌──────────────┐   真实 SoC：由芯片内固化的 Boot ROM 加载（6.2.1）
│ BL1          │   QEMU：由 -bios 参数直接喂入（6.2.2）
│ （首段引导）  │
└──────┬───────┘
       │ 加载
       ▼
┌──────────────┐
│ BL2          │   初始化 DDR，把后续镜像搬进内存
│ （平台初始化）│
└──────┬───────┘
       │ 加载
       ▼
┌──────────────┐
│ BL31         │   常驻 EL3，提供 PSCI 等运行时服务
│ （运行时服务）│
└──────┬───────┘
       │ 移交
       ▼
┌──────────────┐
│ BL33         │   U-Boot——chapter 7 才长出来
└──────┬───────┘
       ▼
  Linux Kernel → rootfs（chapter 8、9）
```

这张图里，BL1、BL2、BL31 三格就是本章的产出范围——今天收工时，要让它们从图上的格子变成 deploy 目录里的文件。

#### 6.1.2 三级各回答一个问题

老周的白板上，三级旁边各写了一个问题。阿凯照着抄进本子：

- **BL1 回答"上电后第一段代码从哪来"**。它运行在芯片内一块很小的 SRAM 里，空间紧到只能干一件事：找到 BL2，把它加载进来，然后跳转。别的什么都不干，也没地方干。
- **BL2 回答"主内存和后续镜像谁来准备"**。BL2 手里最重的活是初始化 **DDR**（Double Data Rate SDRAM，双倍数据速率内存，tiger 的主内存）——DDR 控制器不初始化，主内存就是一块不会应答的硅片，后面谁都没有舞台。内存就位后，BL2 把 BL31 和 BL33 的镜像搬进去。
- **BL31 回答"常驻在最高特权级的代码提供什么服务"**。BL31 运行在 **异常级别 3（EL3）**——ARMv8-A 的最高安全异常级别，比内核所在的级别还高。它把 BL33 送上台之后**不退出**，常驻内存，等后面的世界通过特权调用找上门来。它提供的服务里最重要的一组是 **电源状态协调接口（PSCI）**：ARM 定义的电源管理接口，CPU 核心的启停和休眠都走它。本章只要知道 BL31 是它的提供方；真正用到它，是 chapter 7 之后多核唤醒的事。

"所以 TF-A 不是'一段固件'。"阿凯在三级之间画着箭头，"是三个特权级上各驻一段，一级把一级扶上台。"

"扶手这个词用得好。"老周说，"每一级把下一级扶上台，自己要么让位、要么退到后台待命。BL1、BL2 是让位的，用完即弃；BL31 是待命的，一直驻着。"

#### 6.1.3 一次构建，三级产物

回到工程视角，一个对今天至关重要的信息藏在开头那句话里：**一次 TF-A 构建，三级固件一起产出**。BL1、BL2、BL31 不是三个项目、三份配方——它们是同一份 TF-A 源码树按同一个平台参数编出来的三个文件。今天构建成功的话，deploy 目录里会同时出现 `bl1.bin`、`bl2.bin`、`bl31.bin`。

"所以集成工作量比听起来小。"阿凯说，"一个配方，一个平台参数，三个产物。"

"工作量是不大。"老周说，"所以今天的主要时间不在'构建'上，在前面两个问题上：第一级谁来加载，以及配方从哪来。"

### 6.2 BL1 谁来加载：Boot ROM 与 QEMU 的起点

#### 6.2.1 真实 SoC：BL1 由 Boot ROM 加载，BSP 管不到也不用管

Fig-6-1 最顶上留了一个悬而未决的问题：BL1 加载 BL2，那 BL1 谁来加载？

答案是芯片自己。真实 SoC 出厂时，芯片内部固化着一小段只读代码——Boot ROM，chapter 2 已经立过这个名字：SoC 出厂固化的最早引导代码驻留区。上电复位后，CPU 取的第一条指令就在 Boot ROM 里；它按芯片设计好的规矩，从启动介质（SPI NOR、NAND、eMMC，看具体芯片）把 BL1 读进 SRAM，然后跳转过去。

这段代码对 BSP 工程师有两个重要性质：**改不了**，也**不需要改**。改不了，是因为它是芯片出厂时做进硅片的掩膜，不是 Flash 里的文件；不需要改，是因为它足够通用——只认"从约定位置搬 BL1"这一件事，所有板级差异都从 BL1 之后才开始出现。TF-A 源码树里照常编译出 `bl1.bin`，但这个文件在真板上的"搬运工"永远是 Boot ROM。

"所以我们做 tiger 的 BSP，Boot ROM 这一格是不用交付的。"阿凯说。

"不交付，但要画在图上。"老周说，"启动链文档里少画这一格，后面的人就会问出你刚才那个问题。"

#### 6.2.2 QEMU 的等效手法：`-bios` 直接喂 BL1

tiger 跑在 QEMU 上，没有真的掩膜 ROM。QEMU 给了一个等效机关：`-bios <文件>` 参数——虚拟机上电后直接从指定文件开始执行，相当于把"Boot ROM 加载 BL1"这一步折进了模拟器里。

所以今天的验证手法已经定了：构建出 `bl1.bin` 之后，用 `qemu-system-aarch64 -machine tiger -bios bl1.bin` 启动，QEMU 扮演 Boot ROM 的角色，把 BL1 喂进虚拟机，然后串口日志就会替我们走完 BL1 → BL2 → BL31 这条链。这是 6.7.3 的事，先把机制记住。

> **💡 提示**：`-bios` 不是"绕过启动链"，是把 Boot ROM 的职责外包给 QEMU。将来在真板上烧写时，bl1.bin 要放在 Boot ROM 认得的启动介质位置——那是烧写脚本关心的事，不影响启动链结构本身。

### 6.3 自己写还是借：meta-arm 决策

#### 6.3.1 老周不答，让阿凯自己去翻

"第二个问题。"老周在白板边上敲了敲，"TF-A 的配方——自己写一份，还是……？"

他停住了，没把话说完。阿凯等着下文，老周却转身指了指屏幕："Yocto 官方有个 ARM 平台的 layer 叫 meta-arm，chapter 2 查配方的时候扫到过一眼。你把它 clone 下来，找到 TF-A 的配方读完，然后告诉我：如果自己写，你打算抄多少行。"

阿凯领命。clone 仓库，先看顶层结构：

```bash
# clone Yocto 官方 ARM 平台 layer 仓库（scarthgap 分支，与全书版本锁定一致）
cd ~/workspace
git clone -b scarthgap git://git.yoctoproject.org/meta-arm.git

# 看顶层目录结构
ls -1 ~/workspace/meta-arm
```

输出（以本地实际输出为准）：

```text
ci
COPYING.MIT
documentation
kas
meta-arm
meta-arm-bsp
meta-arm-systemready
meta-arm-toolchain
README.md
scripts
SECURITY.md
```

顶层目录先放一放，那是 6.3.3 的素材。阿凯直奔 TF-A 配方：

```bash
# 定位 meta-arm 里的 TF-A 配方
ls -1 ~/workspace/meta-arm/meta-arm/recipes-bsp/trusted-firmware-a/
```

输出（以本地实际输出为准）：

```text
files
fiptool-native_2.10.4.bb
tf-a-tests_2.10.0.bb
trusted-firmware-a_%.bbappend
trusted-firmware-a_2.10.4.bb
trusted-firmware-a.inc
```

版本号对上了——`trusted-firmware-a_2.10.4.bb`，TF-A v2.10，正是全书版本表锁定的版本。阿凯打开 `.bb` 和 `.inc` 两个文件，把关键行摘了出来。

`.bb` 文件很薄，核心是版本锁定三行，外加一条 6.5.4 还会再见的 patch：

```bitbake
# 文件路径：~/workspace/meta-arm/meta-arm/recipes-bsp/trusted-firmware-a/trusted-firmware-a_2.10.4.bb（关键行节选）
require recipes-bsp/trusted-firmware-a/trusted-firmware-a.inc

# TF-A v2.10.4
SRCREV_tfa = "569e16caad976a0684147da1ecc6333fd9b7f813"
SRCBRANCH = "lts-v2.10"

# continue to boot also without TPM（.bb 尾部第 16-18 行，这条 patch 6.5.4 还会再见）
SRC_URI += "\
    file://0001-qemu_measured_boot.c-ignore-TPM-error-and-continue-w.patch \
"
```

真正的工程量都在 `.inc` 里。阿凯逐段读，摘出这些关键行：

```bitbake
# 文件路径：~/workspace/meta-arm/meta-arm/recipes-bsp/trusted-firmware-a/trusted-firmware-a.inc（关键行节选）

# ---- 源码获取：上游 TF-A 官方仓库；注意 ?= 预留了替换旋钮 ----
SRC_URI_TRUSTED_FIRMWARE_A ?= "git://git.trustedfirmware.org/TF-A/trusted-firmware-a.git;protocol=https"
SRCBRANCH = "master"
SRC_URI = "${SRC_URI_TRUSTED_FIRMWARE_A};name=tfa;branch=${SRCBRANCH}"
SRCREV_FORMAT = "tfa"

# ---- 机器适配：两个 invalid 兜底，不为具体机器特化就不给构建 ----
COMPATIBLE_MACHINE ?= "invalid"
TFA_PLATFORM ?= "invalid"

# ---- 构建目标：默认只编 bl1 ----
TFA_BUILD_TARGET ?= "bl1"

# ---- 裸机构建：只要交叉编译器，不要 libc；屏蔽 Yocto 组装的编译参数 ----
DEPENDS:remove = "virtual/${TARGET_PREFIX}compilerlibs virtual/libc"
CFLAGS[unexport] = "1"
LDFLAGS[unexport] = "1"
export CROSS_COMPILE="${TARGET_PREFIX}"

# ---- 平台参数经 EXTRA_OEMAKE 递交给 TF-A 自己的 Makefile（EXTRA_OEMAKE 到 6.6.1 正式介绍） ----
EXTRA_OEMAKE += "BUILD_BASE=${B} PLAT=${TFA_PLATFORM}"
```

#### 6.3.2 阿凯汇报："要抄的比我预想的多得多"

老周听完阿凯的汇报，只问了一句："多少行？"

"inc 文件两百五十行，bb 再二十行。"阿凯翻着自己的笔记，"要抄的东西我分了六类：

1. 源码获取与版本锁定——上游仓库地址、分支、SRCREV 一整套；
2. 裸机构建的特殊处理——把 libc 和编译器运行库从 DEPENDS 里摘掉，把 Yocto 组装的 CFLAGS、LDFLAGS 屏蔽掉，只借交叉编译器本身；
3. EXTRA_OEMAKE 的编排——十几次 append，清理 LD 和 CC 里的冲突参数、拼平台参数、指 OPENSSL 和 HOSTCC；
4. do_compile 按构建目标循环编译；
5. do_install 的产物命名和符号链接；
6. do_deploy 把产物拷进 deploy 目录。

这还不算它顺手处理的可选路径——安全相关支持、UEFI 启动支持，还有 CVE 追踪的产品名登记，今天一样都用不上。"

"抄完然后呢？"

"然后跟着上游维护。"阿凯自己说下去了，"TF-A 升一次版本，我得对着上游配方 diff 一次，判断哪些改动要搬过来。抄一遍不如叠一层——用它现成的配方，tiger 的差异用 bbappend 叠上去。bbappend 那套机制 chapter 4 刚学完，全是现成的。"

"记住你今天数行的感觉。"老周说，"这是一条可迁移的判据：**组件的构建逻辑复杂、上游又有维护良好的 layer 时，复用优于自写**。判据的另一半——什么时候反而该自写——chapter 8 写内核配方时你自己回答，到时候这两章互为对照组。"

#### 6.3.3 一个仓库，多个 layer

回头处理 6.3.1 那份目录清单。阿凯注意到顶层有四个 `meta-*` 目录，当时没细想，现在要看清了——注册之前必须知道"注册的是什么"。

"meta-arm 这个仓库里有四个 layer？"阿凯指着屏幕。

"去验证。"老周说，"layer 的身份证是什么？"

"`conf/layer.conf`。"阿凯逐个翻过去——四个 `meta-*` 目录各有各的 `conf/layer.conf`，集合名分别是 `meta-arm`、`arm-toolchain`、`meta-arm-bsp`、`meta-arm-systemready`。第二个值得多看一眼：目录叫 `meta-arm-toolchain`，集合名却叫 `arm-toolchain`——**目录名和集合名也不必同名**，layer 的身份认集合名，不认目录名。**仓库是 git 的边界，layer 是 BitBake 的边界，两者不是一回事**：一个仓库可以装多个 layer，注册、依赖、优先级都按 layer 算，不按仓库算。

tiger 今天只需要 TF-A 配方，它住在 `meta-arm/meta-arm` 这个子 layer 里。`meta-arm-bsp` 装的是 N1SDP、Juno 这些真实开发板的机器配置，`meta-arm-systemready` 是认证测试相关，`meta-arm-toolchain` 是 ARM 工具链相关——后三个今天都用不上。

"只注册 `meta-arm/meta-arm` 一个。"阿凯说。

"你试试。"老周说。

### 6.4 引入 meta-arm：注册与依赖声明

#### 6.4.1 注册：一个预校验拦下的报错

```bash
# 初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 注册 meta-arm 子 layer
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm
```

输出（关键行，以本地实际输出为准）：

```text
ERROR: Layer 'meta-arm' depends on layer 'arm-toolchain', but this layer is not enabled in your configuration
ERROR: Parse failure with the specified layer added, exiting.
```

报错当场就来了，最后那行 fatal 就是回滚的落地声——按 chapter 3 已经领教过的行为，`add-layer` 写入 `bblayers.conf` 后会立刻做一次完整解析校验，校验不过自动回滚：meta-arm 此刻并没有被注册进去。这个"先写后验、验不过就回滚"的预校验行为，6.8.1 收尾时还会再撞见一次。

阿凯顺着报错回看刚才翻过的 `meta-arm/meta-arm/conf/layer.conf`：

```text
LAYERDEPENDS_meta-arm = " \
    core \
    arm-toolchain \
"
```

"它声明依赖的集合是 `arm-toolchain`——就是 `meta-arm-toolchain` 那个目录提供的集合，6.3.3 对过的'目录名 ≠ 集合名'在这儿兑现了。得先把 `meta-arm-toolchain` 子 layer 注册进来——它就在同一个仓库里。"阿凯说。

"这就是 6.3.3 说'仓库不等于 layer'的代价面。"老周说，"clone 是按仓库 clone 的，注册和依赖是按 layer 算的。你 clone 了一个仓库，不等于仓库里的 layer 都进了构建。"

按依赖顺序补注册：

```bash
# 先注册被依赖的 meta-arm-toolchain，再注册 meta-arm
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm-toolchain
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm

# 验证 layer 列表
bitbake-layers show-layers
```

输出（以本地实际输出为准）：

```text
layer                 path                                                                                priority
=====================================================================================================================
core                  /home/<your-username>/workspace/poky/meta                                           5
yocto                 /home/<your-username>/workspace/poky/meta-poky                                      5
yoctobsp              /home/<your-username>/workspace/poky/meta-yocto-bsp                                 5
meta-tiger            /home/<your-username>/workspace/meta-tiger                                          6
arm-toolchain         /home/<your-username>/workspace/meta-arm/meta-arm-toolchain                         5
meta-arm              /home/<your-username>/workspace/meta-arm/meta-arm                                   5
```

再确认 TF-A 配方已经能被 BitBake 看见：

```bash
# 确认 trusted-firmware-a 配方可见、归属与版本
bitbake-layers show-recipes trusted-firmware-a
```

输出（以本地实际输出为准）：

```text
=== Matching recipes: ===
trusted-firmware-a:
  meta-arm             2.10.4 (skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE))
```

配方在列，归属 `meta-arm` 层，版本 2.10.4，与版本锁定一致。尾巴上那段 `(skipped: ...)` 也值得记住：它对 tiger 整份跳过——现在还没有任何机器认领它。这个标记正是 6.5.3 要解决的"上岗证"问题的预告，先混个眼熟。

#### 6.4.2 声明依赖：LAYERDEPENDS 第一次真正用上

还有一笔账要补。chapter 3 写 `layer.conf` 时，`LAYERDEPENDS_meta-tiger = "core"` 那行一直只有 `core`——因为 meta-tiger 确实只依赖 OE-Core。从今天起情况变了：meta-tiger 里将要写的 bbappend，叠加目标住在 `meta-arm` 这个 layer 里。**bbappend 是跨 layer 的引用，跨 layer 就要声明依赖**——这是 `LAYERDEPENDS` 这个变量第一次真正派上用场。

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/layer.conf（将 LAYERDEPENDS 一行替换为）
# 我依赖谁：OE-Core，以及 meta-arm（recipes-bsp 下的 bbappend 叠加目标住在那里）
LAYERDEPENDS_meta-tiger = "core meta-arm"
```

"不改会怎样？"阿凯问，"刚才 meta-arm 也注册了，bbappend 也能叠上去。"

"不改，今天什么都不会发生。"老周说，"真正的考验在 meta-arm 不在场的那天——到时候报错是会有，但说不说得清楚话，就看你今天这行写没写。具体什么样子，6.8 的踩坑实录你自己演一遍。现在先把正解写上。"

### 6.5 编写 trusted-firmware-a_%.bbappend：换源码、传平台

#### 6.5.1 目录落位

沿用 chapter 4 建立的目录约定：`recipes-bsp/<配方名>/` 两级目录，bbappend 文件名按 PN 匹配加 `%` 通配版本。`trusted-firmware-a` 是 recipes-bsp 类别下的第二个住客（第一个是 qemu）。

```bash
# 创建 bbappend 落位目录
mkdir -p ~/workspace/meta-tiger/recipes-bsp/trusted-firmware-a
```

#### 6.5.2 两种接线方式：patch 流 vs 直拉仓库流

动笔之前，老周让阿凯先把这次和 chapter 4 的区别说清楚。

chapter 4 集成 qemu-tiger 用的是 **patch 流**：原配方的 SRC_URI 不动（上游 tarball 照下），自有改动导出成三个 `.patch` 文件，用 `SRC_URI:append` 追加进去。本章不一样：tf-a-tiger 是固件组维护的**长期 fork 仓库**——`plat/tiger/` 平台代码在里面持续演进，有自己的提交历史，不是"上游版本加两三个 patch"的形态。对这种形态，更自然的接线是 **直拉仓库流**：把 SRC_URI 的源码地址整体改指 tf-a-tiger 仓库，让 do_fetch 直接拉 fork。

两种流派的判据，阿凯总结在便签上：

- 自有改动是"上游某个版本 + 少量补丁"说得清的——patch 流。diff 小，base 明确，跟着上游 tarball 升级就重导一遍。
- 自有改动是一个活着的 fork，自带分支和提交历史，由专门的组维护——直拉仓库流。把 patch 当运输格式反而碍事，直接拉仓库，版本用提交哈希锁死。

直拉仓库流里"锁死版本"的机关是本章第一个新变量：**源码修订号（SRCREV）**——BitBake 变量，指定 git 仓库的提交哈希或标签，锁定 recipe 拉取的源码版本。git 分支是会动的指针，`branch=main` 今天和明天可能指向不同的提交；可重现构建要求每次拉到的字节完全一致，所以直拉 git 仓库的配方必须用 SRCREV 钉住一个具体提交。

读 meta-arm 原配方时有个细节在此刻派上用场：它的 SRC_URI 条目带 `;name=tfa`，对应的修订号变量就叫 `SRCREV_tfa`——名字后缀跟着 SRC_URI 里的 `name=` 走，`.bb` 文件里那句 `SRCREV_tfa = "569e16ca..."` 就是这么来的。我们的 bbappend 照这个规矩写。

#### 6.5.3 bbappend 全文

写之前还有一个从原配方里读出来的硬约束，不能漏：inc 文件里 `COMPATIBLE_MACHINE ?= "invalid"`——原配方默认对**任何**机器都不兼容，不为具体机器特化就整份跳过。6.4.1 那条 `(skipped: ...)` 尾巴就是它干的好事。meta-arm 自己那份 bbappend 是旁证：里面用 `COMPATIBLE_MACHINE:qemuarm64-secureboot = "qemuarm64-secureboot"` 这样的机器覆盖语法逐台认领。我们的 bbappend 要为 `tiger-aarch64` 认领，这行是配方的上岗证，漏了它，配方对 tiger 来说等于不存在。

第二处要改的默认值：`TFA_BUILD_TARGET ?= "bl1"`——默认只编 BL1 一级。本章要的是三级一起出，显式列全。

本章核心交付物全文：

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend
# tiger 平台的 TF-A 集成：源码改指 tf-a-tiger 开发态仓库（固件组维护），
# 平台代码 plat/tiger/ 在开发态仓库里演进；本 layer 只做集成，不做功能开发

# 本配方为 tiger-aarch64 构建（原配方默认 COMPATIBLE_MACHINE ?= "invalid"，
# 不认领则对本机整份跳过）
COMPATIBLE_MACHINE = "tiger-aarch64"

# 源码改指 tf-a-tiger 仓库：原配方用 ?= 预留了 SRC_URI_TRUSTED_FIRMWARE_A 旋钮，
# BitBake 变量惰性展开，bbappend 里覆盖它即改变 SRC_URI 的最终值
SRC_URI_TRUSTED_FIRMWARE_A = "git://<internal-git-server>/bsp/tf-a-tiger.git;protocol=ssh"
SRCBRANCH = "main"
# 锁定提交哈希，保证可重现构建；变量名带 _tfa 后缀，对应原配方 SRC_URI 里的 ;name=tfa
SRCREV_tfa = "8f3d2e1c7a9b4e5f6c0d1a2b3e4f5a6b7c8d9e0f"

# tiger 平台：名字必须与 tf-a-tiger 仓库 plat/tiger/ 目录名逐字符一致（大小写敏感）
TFA_PLATFORM = "tiger"

# 一次构建产出三级固件并打包（原配方默认只编 bl1）
TFA_BUILD_TARGET = "bl1 bl2 bl31 fip"

# 固件串口日志级别（TF-A 构建开关；6.7.3 看到的 NOTICE/INFO 输出受它控制）
EXTRA_OEMAKE:append = " LOG_LEVEL=40"
```

逐行过一遍职责：`COMPATIBLE_MACHINE` 认领本机；`SRC_URI_TRUSTED_FIRMWARE_A` + `SRCBRANCH` + `SRCREV_tfa` 三行完成"换源码"——注意没有动 `SRC_URI` 本身，而是覆盖原配方作者预留的旋钮，这比整体重写 SRC_URI 的改法小，上游配方调整 SRC_URI 拼装方式时我们跟着受益（顺带一提，`git://...;protocol=ssh` 是 do_fetch 的 fetcher 语法，和人敲 `git clone git@...` 的地址写法不是一套，别拿 shell 的习惯来读它）；`TFA_PLATFORM` 是平台参数，6.6 讲它怎么流进 TF-A 的 Makefile；`TFA_BUILD_TARGET` 列出三级构建目标（`fip` 一项 6.6.2 解释）；`EXTRA_OEMAKE:append` 是往构建参数里追加我们自己的开关。

<!-- 【待验证】SRCREV_tfa 的哈希值为占位（tf-a-tiger 仓库尚不存在），定稿前替换为真实提交 -->

"固件组那边我只对过仓库地址和分支。"阿凯说，"平台代码长什么样我没看。"

"不用看。"老周说，"开发态的规矩——功能开发在他们的仓库里完成，你只做集成。`plat/tiger/` 里面的代码有问题，提给固件组，不在 Yocto 里改。"

#### 6.5.4 验证叠加：老手法，新配方

验证手法全是 chapter 4、chapter 5 学过的。先看 bbappend 挂上没有：

```bash
# 初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 检查 trusted-firmware-a 的 bbappend 叠加关系
bitbake-layers show-appends trusted-firmware-a
```

输出（以本地实际输出为准）：

```text
=== Matched appended recipes ===
trusted-firmware-a_2.10.4.bb:
  /home/<your-username>/workspace/meta-arm/meta-arm/recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend
  /home/<your-username>/workspace/meta-tiger/recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend
```

输出里有两行，值得停一下：meta-arm 自己就带了一份 bbappend，我们的排在它后面——两行的顺序跟着 layer 的注册顺序走，不是按路径字典序排的。meta-arm 那份全是 `:qemuarm64-secureboot` 之类的机器覆盖语法，对 tiger-aarch64 一条都不命中，静默无效——它在场但不说话。我们的这份没有机器后缀，生效。

再看变量叠加后的最终值：

```bash
# 查 bbappend 叠加后的关键变量最终值
bitbake -e trusted-firmware-a | grep -E "^(COMPATIBLE_MACHINE|SRC_URI|SRCBRANCH|SRCREV_tfa|TFA_PLATFORM|TFA_BUILD_TARGET)="
```

输出（以本地实际输出为准；终端里 SRC_URI 各条目间有多个空格、行尾有尾随空格，这里按整洁排版）：

```text
COMPATIBLE_MACHINE="tiger-aarch64"
SRC_URI="git://<internal-git-server>/bsp/tf-a-tiger.git;protocol=ssh;name=tfa;branch=main file://0001-qemu_measured_boot.c-ignore-TPM-error-and-continue-w.patch"
SRCBRANCH="main"
SRCREV_tfa="8f3d2e1c7a9b4e5f6c0d1a2b3e4f5a6b7c8d9e0f"
TFA_PLATFORM="tiger"
TFA_BUILD_TARGET="bl1 bl2 bl31 fip"
```

`SRC_URI` 的最终值确认了两件事：git 源已经改指 tf-a-tiger（`name=tfa`、`branch=main` 都是原配方拼装逻辑帮我们拼好的）；原配方自带的那条 qemu TPM patch 仍在清单里——就是 6.3.1 节选里 `.bb` 尾部那三行。它 patch 的是 TF-A 源码树里 `plat/qemu/` 的文件，tf-a-tiger 作为 fork 保留了那部分上游代码，补丁照常落得上，不碍事。

### 6.6 参数传递与构建：TFA_PLATFORM、EXTRA_OEMAKE、do_deploy

#### 6.6.1 TFA_PLATFORM：名字即路径片段

"配方怎么知道编哪个平台？"阿凯把问题摆出来，自己沿着变量链往下追。

答案在 6.3.1 摘出的那行里：`EXTRA_OEMAKE += "BUILD_BASE=${B} PLAT=${TFA_PLATFORM}"`。这里正式引入这个变量：**额外构建参数（EXTRA_OEMAKE）**——BitBake 变量，配方组装好的、要原样递给 `make` 命令行的额外参数。TF-A 的构建不归 Yocto 的编译流程管，它有自己的 Makefile；Yocto 一侧能做的，就是把参数摆到 make 的命令行上。`PLAT=${TFA_PLATFORM}` 这一拼，我们在 bbappend 里写的 `TFA_PLATFORM = "tiger"` 就变成了 TF-A Makefile 里的 `PLAT=tiger`。

而 TF-A 的 Makefile 拿到 `PLAT` 之后做的事很直接：去源码树的 `plat/<PLAT 值>/` 目录下找平台代码。也就是说 **`TFA_PLATFORM` 不是一个普通的标识符，它是一段路径片段**——值必须与 tf-a-tiger 仓库里 `plat/tiger/` 的目录名逐字符一致，包括大小写。写成 `Tiger` 会发生什么，6.8.2 见。

整条参数链捋直了是这样的：

```text
meta-tiger bbappend: TFA_PLATFORM = "tiger"
        │
        ▼ （meta-arm 配方的拼装逻辑）
EXTRA_OEMAKE: ... PLAT=tiger
        │
        ▼ （oe_runmake 把 EXTRA_OEMAKE 摆上 make 命令行）
TF-A Makefile: 到 plat/tiger/ 找平台代码，编译
```

参数链不止嘴上捋，变量最终值里能直接看见它：

```bash
# 实证：EXTRA_OEMAKE 最终值里的参数链各就各位
bitbake -e trusted-firmware-a | grep ^EXTRA_OEMAKE=
```

输出（关键行，长路径以省略号代替；终端里各条目间有多个空格，这里按整洁排版，以本地实际输出为准）：

```text
EXTRA_OEMAKE=" LD='aarch64-poky-linux-ld' CC='aarch64-poky-linux-gcc ' V=1 E=0 BUILD_BASE=<配方构建目录> PLAT=tiger OPENSSL_DIR=<...> HOSTCC='gcc ' RUNTIME_SYSROOT=<...> LOG_LEVEL=40"
```

值里能同时读出两段来源：`BUILD_BASE=... PLAT=tiger` 是 meta-arm 配方组装的那一段，`PLAT` 端端正正等于 `tiger`；`LOG_LEVEL=40` 挂在值的最末尾——那是我们 bbappend 里 `EXTRA_OEMAKE:append` 的落点，append 的字面意思在变量值里看得见。其余的 LD、CC、OPENSSL_DIR 那一串，是 6.3.2 数过的"清理冲突参数、指 OPENSSL 和 HOSTCC"的编排成果，今天不用逐个拆开。

#### 6.6.2 构建与产物：do_deploy 把交付物送进 deploy 目录

构建：

```bash
# 构建 TF-A（首次构建约需几分钟；需要 tf-a-tiger 仓库可达）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake trusted-firmware-a
```

<!-- 【待验证】构建摘要与 deploy 文件清单依赖 tf-a-tiger 仓库，全部待实测（待验证清单 C-4 项，阻塞级）；文件名按 meta-arm do_install 源码逻辑推定 -->
输出（关键行，以本地实际输出为准）：

```text
NOTE: Tasks Summary: Attempted <任务数> tasks of which <命中数> didn't need to be rerun and all succeeded.
```

产物进 deploy 目录——chapter 1 引入过的"构建产物的最终输出目录"。今天它比 chapter 1 讲得具体一些：deploy 目录里的文件不是天上掉下来的，是每个配方由自己的 **do_deploy（部署任务）** 送进去的——这是一个 BitBake 任务，职责就是把配方的交付物从自己的安装目录送进共享的 deploy 目录。meta-arm 配方里的实现短得可以全文引用：

```bitbake
# 文件路径：~/workspace/meta-arm/meta-arm/recipes-bsp/trusted-firmware-a/trusted-firmware-a.inc（do_deploy 全文）
do_deploy() {
    cp -rf ${D}/firmware/* ${DEPLOYDIR}/
}
addtask deploy after do_install
```

`do_install` 先把编译产物按统一命名装进 `/firmware`（每个文件带 `-tiger` 平台后缀，再配一个不带后缀的符号链接）；`do_deploy` 里的 `${DEPLOYDIR}` 还不是终点，它是配方自己 work 目录下的暂存区——任务执行时产物先拷到那里，再由 sstate 机制镜像进共享的 `tmp/deploy/images/<machine>/`，待会儿 `ls` 的是最后一站。`addtask` 把这个任务挂到 `do_install` 之后。去看产物：

```bash
# 查看 TF-A 部署到 deploy 目录的产物
ls -1 $BUILDDIR/tmp/deploy/images/tiger-aarch64/
```

输出（关键行，以本地实际输出为准）：

```text
bl1-tiger.bin
bl1.bin
bl2-tiger.bin
bl2.bin
bl31-tiger.bin
bl31.bin
fip-tiger.bin
fip.bin
# ...（do_install 还会无条件装上各级 .elf 调试产物；若平台带设备树，另有 .dtb）
```

指认一下：`bl1.bin`、`bl2.bin`、`bl31.bin` 就是 6.1 说的三级固件，带 `-tiger` 后缀的是实体文件，不带后缀的是指向它们的符号链接——将来别的环节引用产物时写不带后缀的名字，平台演进换后缀时不动引用方。`fip.bin` 是 TF-A 的固件打包格式，把 BL31 和将来的 BL33 打成一个包交给 BL2 加载——现在包里只有 BL31，BL33 的位置空着，等 chapter 7 的 U-Boot 来填。

这个"空着"不是白来的：TF-A 上游的规矩是，平台只要编 BL2 就默认要求 BL33 在场，fip 打包找不到 BL33 会拒绝构建。tf-a-tiger 的 `plat/tiger/platform.mk` 显式把这项要求关掉了（`NEED_BL33=no`），"BL33 先空着、chapter 7 再填"才成立。这是"平台代码该长什么样"里我们依赖固件组的一条约定——6.5.3 说"不用看"的平台代码，这条算半个例外：不用看，但要知道有它。

<!-- 【待验证·阻塞级】NEED_BL33=no 为本章构建前提（待验证清单 C-4 项），须 tf-a-tiger 的 plat/tiger/platform.mk 确认 -->


#### 6.6.3 一句对照：TUNE 管不到 TF-A 头上

chapter 5 结尾留过一个伏笔：TF-A 是裸机代码，不走 target 工具链。现在有了真配方，可以把这句话落到实处——就是 6.3.1 摘出的那几行：`DEPENDS:remove` 把 libc 和编译器运行库从依赖里摘掉（裸机代码没有操作系统可依赖），`CFLAGS[unexport]`、`LDFLAGS[unexport]` 把 TUNE 一路组装过来的编译参数屏蔽掉，只留 `CROSS_COMPILE` 借交叉编译器的"手"。TF-A 的 Makefile 自己决定用哪些 flags。

所以 chapter 5 那条长长的因果链（TUNE 变化 → 编译参数变化 → 任务签名全变）到 TF-A 门口就断了：给 glibc 准备的 `-mcpu=cortex-a53`，TF-A 根本不听。同一份源码，两种构建纪律——应用层归 Yocto 管，固件归它自己的 Makefile 管，泾渭分明。

### 6.7 回填与单独验证

#### 6.7.1 回填 tiger-aarch64.conf：先查，再写

接线全部完成，配方就位。按 chapter 5 定下的模式，接下来该 machine 配置"显式认领"了。阿凯甚至有点期待这一步——`PREFERRED_PROVIDER` 这套东西，chapter 2 老周前瞻过，chapter 5 在内核上实战过，今天是第二次用。

他先把 chapter 2 记的那句前瞻翻出来，按 chapter 5 的弱赋值惯例改写成 `??=`，又仿内核那两行补了一条版本锁定：

```bitbake
# （草稿，未落盘）
PREFERRED_PROVIDER_virtual/trusted-firmware-a ??= "trusted-firmware-a"
PREFERRED_VERSION_trusted-firmware-a ??= "2.10.%"
```

落盘之前，他停了一下——chapter 5 的教训之一是"先查，再信"。linux-dummy 的事让他多长了一个心眼：TF-A 这边会不会也有一个静默兜底的东西？认领行写不写，真的能改变什么吗？

"去查配方那边 PROVIDES 什么。"老周听完他的疑问，只给了一个方向。

```bash
# 查 trusted-firmware-a 配方声明自己提供什么
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e trusted-firmware-a | grep ^PROVIDES=
```

<!-- 【待验证】PROVIDES 输出待实测（待验证清单 C-5 剩余子项）；尾随空格有 bitbake.conf:310-311 源码级依据 -->
输出（以本地实际输出为准）：

```text
PROVIDES="trusted-firmware-a "
```

只有它自己。没有 `virtual/trusted-firmware-a`。阿凯不放心，把两棵源码树整个翻了一遍：

```bash
# 在 poky 与 meta-arm 全库搜索 virtual/trusted-firmware-a 的任何踪迹
grep -rn "virtual/trusted-firmware-a" ~/workspace/poky/meta ~/workspace/meta-arm
```

没有任何输出——零命中。虚包根本不存在。

"虚包是为什么存在的？"老周问。

阿凯想了想 chapter 5 的内核："仲裁。好几个配方都能提供同一个功能名，得有人点名用谁……可 TF-A 只有一个配方，没有竞争，就没人需要仲裁，也就没人造这个虚包。"

"对。`virtual/kernel` 有一群候选在抢，`virtual/bootloader` 有 OE-Core 里现成的 PROVIDES，chapter 7 你会用到真的。TF-A 这里，meta-arm 从没给它立过虚包——所以认领行写下来也不是错，是**没人听**：变量设上了，没有任何机制读它。"老周顿了顿，"chapter 2 我那句前瞻是按内核和 bootloader 的惯例估的，估错了。你刚才'先查再写'那一停，把这句话拦在了纸面之外。"

"那 TF-A 的'认领'长什么样？"

"去看 meta-arm 自己的机器配置怎么写。"

```bash
# 看 meta-arm 自带机器配置如何声明 TF-A（以 N1SDP 这块真实开发板为例）
grep -n "trusted-firmware-a" ~/workspace/meta-arm/meta-arm-bsp/conf/machine/n1sdp.conf
```

输出（关键行，以本地实际输出为准）：

```text
28:EXTRA_IMAGEDEPENDS += "trusted-firmware-a"
30:PREFERRED_VERSION_trusted-firmware-a ?= "2.10.%"
```

两行，各有各的分工。`EXTRA_IMAGEDEPENDS`——chapter 4 在 qemuboot.bbclass 那行里见过它——声明"构建镜像时额外依赖哪些配方"：TF-A 的产物不进 rootfs，但镜像构建要求它在 deploy 目录就位，镜像这一侧靠这行把固件拉上依赖链。`PREFERRED_VERSION` 是老熟人，锁定版本 2.10。没有竞争，所以不需要 `PREFERRED_PROVIDER`；有版本要锁，所以 `PREFERRED_VERSION` 保留。

删掉草稿里那行没人听的认领行，按 meta-arm 的惯例回填：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接 chapter 5 全文追加）

# ---- TF-A 集成声明 ----
# 镜像构建时把 TF-A 固件拉上依赖链（产物进 deploy 目录，不进 rootfs）；
# TF-A 无 virtual 虚包（单一配方、无竞争需仲裁），认领不走 PREFERRED_PROVIDER
EXTRA_IMAGEDEPENDS += "trusted-firmware-a"
# 锁定 TF-A 版本为 2.10（版本锁定见全书版本表）
PREFERRED_VERSION_trusted-firmware-a ??= "2.10.%"
```

`tiger-aarch64.conf` 全文回顾（chapter 5 各段 + 本章新增段）：

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
```

写一段验一段，无目标 `bitbake -e` 查全局变量——chapter 5 建立的手法，现在阿凯不用提醒就自己敲了：

```bash
# 验证回填生效（无目标全局查询；带 core-image-minimal 会因缺内核解析失败）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e | grep -E "^(EXTRA_IMAGEDEPENDS|PREFERRED_VERSION_trusted-firmware-a)="
```

输出（以本地实际输出为准）：

```text
EXTRA_IMAGEDEPENDS=" trusted-firmware-a"
PREFERRED_VERSION_trusted-firmware-a="2.10.%"
```

`EXTRA_IMAGEDEPENDS` 值前面那个空格阿凯认得——chapter 5 的 `IMAGE_FSTYPES` 同款痕迹，`+=` 往空值上追加留下的。

"所以这套模式不是每次都长一个样。"阿凯在便签上补了一行，"**配方提供产物 + 配置声明使用谁**——思想不变，但'声明'的写法要看生态给什么：有虚包走 `PREFERRED_PROVIDER`，没虚包就直接挂依赖。意图落在纸面上之前，得先确认纸面认不认这个意图。"

"今天这句话比上次的值钱了。"老周说。

#### 6.7.2 整镜构建：报错没有变多

早上老周定下的成功判据，现在验收。跑一次整镜构建，看报错：

```bash
# 整镜构建，观察报错形态（这是"预期中的错误"，与 chapter 5 的 5.5.3 对比着看）
bitbake core-image-minimal
```

输出（关键行，以本地实际输出为准）：

```text
ERROR: Nothing PROVIDES 'virtual/kernel'
linux-yocto PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-rt PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-tiny PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-upstream PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-yocto-dev PROVIDES virtual/kernel but was skipped: incompatible with machine tiger-aarch64 (not in COMPATIBLE_MACHINE)
linux-dummy PROVIDES virtual/kernel but was skipped: PREFERRED_PROVIDER_virtual/kernel set to linux-tiger, not linux-dummy
ERROR: Required build target 'core-image-minimal' has no buildable providers.
```

和上周五逐行对比——主消息行一字不差，六条 skipped 提示还是同一批（行序没有语义，读行集，chapter 5 说过的）。队尾那行 `no buildable providers` 也不是新面孔（chapter 5 只截了关键行，这行当时就在）：它是 `virtual/kernel` 报错的收尾行——主消息说完"没人提供 virtual/kernel"，这句收尾顺势宣判"所以 core-image-minimal 这个目标没有可构建的提供者"，终端 Summary 里的 ERROR 计数因此是 2 条而不是 1 条。trusted-firmware-a 已经进了镜像依赖链，但配方可见、tiger-aarch64 已被认领，它没有添任何一条新报错。**报错没有变多，缺的仍然只有 chapter 8 的内核配方**——本章成功判据达成。

#### 6.7.3 QEMU 单独验证：日志停在该停的地方

最后一步，把 TF-A 链路单独跑起来。用 6.2.2 说好的手法：QEMU 二进制从 qemu-system-native 的 sysroot 里起（chapter 4 的既有约定），`-bios` 扮演 Boot ROM，把 `bl1.bin` 喂进去：

```bash
# 用构建出的 bl1.bin 单独启动 tiger 虚拟机，观察 TF-A 链路串口日志
# （QEMU 二进制来自 qemu-system-native 的 sysroot；-nographic 把串口接到终端）
cd ~/workspace/poky
source oe-init-build-env ../build
$BUILDDIR/tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64 \
    -machine tiger \
    -bios $BUILDDIR/tmp/deploy/images/tiger-aarch64/bl1.bin \
    -nographic
```

<!-- 【待验证】整条流程可行性与串口日志全部内容依赖 tf-a-tiger 平台代码（BL1 装载 BL2/fip 的 io 方案），严禁直接采用（待验证清单 C-7 项，阻塞级）；以下为按 TF-A v2.10 日志惯例书写的示意形态，实际输出以 tf-a-tiger 实测为准 -->
串口日志（示意形态，以本地实际输出为准）：

```text
NOTICE:  BL1: v2.10.4(release):<tf-a-tiger 提交短哈希>
NOTICE:  BL1: Built : <构建时间戳>
INFO:    BL1: Loading BL2
NOTICE:  BL2: v2.10.4(release):<tf-a-tiger 提交短哈希>
INFO:    BL2: Initializing DDR
INFO:    BL2: DDR init done
NOTICE:  BL31: v2.10.4(release):<tf-a-tiger 提交短哈希>
INFO:    BL31: Initializing runtime services
INFO:    BL31: Preparing for EL3 exit to normal world
# ……输出停在这里，没有下文
```

日志逐级走下来：BL1 报版本、加载 BL2；BL2 初始化 DDR；BL31 初始化运行时服务、宣布准备移交。然后——停了。屏幕安静着，光标一动不动。阿凯盯着它等了十几秒。

老周不问"为什么停了"，问的是："下一级是谁？"

"BL33。"阿凯反应过来，"BL31 移交的对象是 BL33——U-Boot。U-Boot 要 chapter 7 才长出来，现在 fip 包里它的位置还是空的。BL31 说'准备移交'，然后没有东西可移交，就停在这。"他抬起头，"停在这就是全对。它要是继续往下跑出什么来，反而见鬼了。"

老周点点头，伸手按了 Ctrl-C 退出 QEMU——日志停在等待 BL33 处，验证通过。

"TF-A 链路闭环了。"老周说，"BL1 到 BL31 三级，版本、加载关系、DDR、运行时服务，每一级都报过到了。启动链的下半段从今天起是实的，不再是图上的格子。"

### 6.8 踩坑实录

按惯例交代时间线：这两个坑发生在 6.4 到 6.6 之间——你前面看到的正确文件和顺利验证，都是改回来之后的样子。

#### 6.8.1 踩坑 1：layer 依赖没声明，报错来了却指错了人

6.4.2 阿凯问过"不改会怎样"，老周当时说"今天什么都不会发生，考验在 meta-arm 不在场的那天"。现在来演一遍。

重演当时的错误：把 `LAYERDEPENDS` 里的 `meta-arm` 撤掉，回到没声明的状态。

```bash
# 重演错误：撤掉 meta-arm 依赖声明（错误示范，演示完必须恢复）
sed -i 's/LAYERDEPENDS_meta-tiger = "core meta-arm"/LAYERDEPENDS_meta-tiger = "core"/' \
    ~/workspace/meta-tiger/conf/layer.conf

# 检查 bbappend 叠加关系
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-appends trusted-firmware-a
```

输出和 6.5.4 一模一样——bbappend 照常挂接，两行都在。再查变量：

```bash
# 查变量叠加结果
bitbake -e trusted-firmware-a | grep ^TFA_PLATFORM=
```

输出：

```text
TFA_PLATFORM="tiger"
```

一切正常。**什么都不发生**——BitBake 不检查"你的 bbappend 指向的 layer 有没有声明依赖"，今天 meta-arm 在构建里，叠加就照常发生，声明缺着也毫无动静。

"那这行声明到底管什么？"阿凯问。

"管 meta-arm 不在场的那一天。"老周说，"模拟一下：把 meta-arm 从构建里拿掉。"

```bash
# 模拟"meta-arm 不在场"的环境：从构建中移除两个 meta-arm 子 layer
bitbake-layers remove-layer ~/workspace/meta-arm/meta-arm
bitbake-layers remove-layer ~/workspace/meta-arm/meta-arm-toolchain

# 再次解析，观察我们 bbappend 的命运
bitbake -e trusted-firmware-a | grep ^TFA_PLATFORM=
```

输出（关键行，以本地实际输出为准）：

```text
ERROR: No recipes in default available for:
  /home/<your-username>/workspace/meta-tiger/recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend
Parsing of 920 .bb files complete (0 cached, 920 parsed). 1878 targets, 60 skipped, 0 masked, 0 errors.
```

meta-arm 一撤，trusted-firmware-a 配方整个没了，我们的 bbappend 成了没有叠加对象的孤儿。但 BitBake 没有放它过去——配方解析的收尾阶段撞上这个没人接的追加文件，致命报错拉闸。注意报错下面还跟着一行解析计数：920 份配方照数解析完、计数照打，fatal 是解析收尾的动作，不是中途腰斩；但构建到此为止——grep 一行输出都没有，也根本走不到"配方没了、该报谁缺"那一层。报错是真的，可读它你能知道什么？只知道"有个追加文件找不到叠加对象"。它指向的是**受害者**——我们的 bbappend 文件，而不是**缺席者** meta-arm；更不会告诉你 meta-tiger 的半边身子架在谁上面。新人拿到这条 ERROR，得自己顺藤摸瓜：这个 bbappend 想叠谁？那个配方原来住哪个 layer？那个 layer 为什么没了？

恢复声明，再看同一个场景（meta-arm 仍不在场）：

```bash
# 恢复依赖声明（layers 仍缺席，观察报错的变化）
sed -i 's/LAYERDEPENDS_meta-tiger = "core"/LAYERDEPENDS_meta-tiger = "core meta-arm"/' \
    ~/workspace/meta-tiger/conf/layer.conf

# 同样的"meta-arm 不在场"场景，这次解析当场报错
bitbake -e trusted-firmware-a | grep ^TFA_PLATFORM=
```

输出（关键行，以本地实际输出为准）：

```text
ERROR: Layer 'meta-tiger' depends on layer 'meta-arm', but this layer is not enabled in your configuration
```

同一个场景，两种报错：都没让你混过去，这一点 BitBake 不含糊；区别在报错的**指向**。没声明，它只说"有个追加文件没人接"——fatal 是 fatal，可缺了谁要你自己猜；声明了，它直接点名：meta-tiger 依赖 meta-arm，而 meta-arm 不在。这就是 `LAYERDEPENDS` 的全部意义——**把"将来那条语义含糊的报错"换成"当场直指根因的报错"**。再叠上前半段的观察——meta-arm 在场时声明缺着毫无动静——完整图景是：这行声明今天沉默，为的是明天报错时能说清楚话。和 chapter 5 linux-dummy 那堂课是同一个句式：报错出现不等于出错，报错消失更不等于没问题。

最后把 meta-arm 加回来，恢复环境。这一步阿凯又撞了一回墙，值得摆出来。此刻的状态是：声明刚刚恢复、两个 meta-arm 子 layer 还缺席——他顺手就敲了 `add-layer`：

```bash
# 直觉的收尾：声明已恢复，直接把层加回来（错误示范，加不进去）
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm-toolchain
```

输出（关键行，以本地实际输出为准）：

```text
ERROR: Layer 'meta-tiger' depends on layer 'meta-arm', but this layer is not enabled in your configuration
ERROR: Parse failure with the specified layer added, exiting.
```

加不进去。眼熟吗——这正是 6.4.1 那套"写入后立刻解析校验、校验不过自动回滚"的预校验行为，这回回旋到了自己头上：meta-arm-toolchain 刚写进 `bblayers.conf`，校验一跑，meta-tiger 的声明要求 meta-arm 在场，而 meta-arm 还没加进来——校验失败，回滚，一层都没加成。**预校验查的不是新加的这一层，是整份配置的一致性**；带着声明加层，会卡在自己声明的依赖上。

正确的收尾顺序是：先把声明撤回到沉默状态，两层按依赖顺序进齐，再恢复声明，最后复验：

```bash
# 正确收尾：撤声明 → 按依赖顺序加回两个子 layer → 恢复声明 → 复验
sed -i 's/LAYERDEPENDS_meta-tiger = "core meta-arm"/LAYERDEPENDS_meta-tiger = "core"/' \
    ~/workspace/meta-tiger/conf/layer.conf
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm-toolchain
bitbake-layers add-layer ~/workspace/meta-arm/meta-arm
sed -i 's/LAYERDEPENDS_meta-tiger = "core"/LAYERDEPENDS_meta-tiger = "core meta-arm"/' \
    ~/workspace/meta-tiger/conf/layer.conf

# 复验：layer 列表、bbappend 挂接与变量叠加全部恢复
bitbake-layers show-layers
bitbake-layers show-appends trusted-firmware-a
bitbake -e trusted-firmware-a | grep ^TFA_PLATFORM=
```

输出与 6.4.1、6.5.4 一致，环境复原。

"声明、注册、校验是三件事，但校验看的是它们合起来的整体。"老周收了个尾，"所以动其中任何一个的时候，先想清楚另外两个此刻摆的是什么姿势。"

#### 6.8.2 踩坑 2：PLATFORM 写了个首字母大写，死在别人的构建系统里

第二个坑发生在 6.5.3 写 bbappend 的时候。阿凯写 `TFA_PLATFORM` 那行，凭英语直觉把专名首字母大写了——`TFA_PLATFORM = "Tiger"`。

重演：

```bash
# 重演错误：平台名首字母大写（错误示范，演示完必须恢复）
sed -i 's/TFA_PLATFORM = "tiger"/TFA_PLATFORM = "Tiger"/' \
    ~/workspace/meta-tiger/recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend

# 构建
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake trusted-firmware-a
```

<!-- 【待验证】TF-A 构建系统报错文案依赖 tf-a-tiger 构建，待实测（待验证清单 C-9 项）；已按上游真实形态书写（make 解析期平台检查无匹配即 $(error)），报错 makefile 文件名、行号与平台列表以实测为准，叙事不绑定具体文件名 -->
输出（关键行，以本地实际输出为准）：

```text
ERROR: trusted-firmware-a-2.10.4-r0 do_compile: oe_runmake failed
| make_helpers/plat_helpers.mk:<行号>: *** Error: Invalid platform. The following platforms are available: <可用平台列表>.  Stop.
```

这个坑的教学价值不在大小写本身，在**报错的位置**。回头看整条链：bbappend 挂接正常（show-appends 绿），变量叠加正常（`bitbake -e` 里 `TFA_PLATFORM="Tiger"` 端端正正），Yocto 一侧的所有检查全过——错误穿过了整个 Yocto 包装层，一路流到 TF-A 自己的 Makefile 里才炸：Makefile 刚被读进来，就拿 `PLAT=Tiger` 去匹配 `plat/Tiger/platform.mk`，匹配落空，平台检查当场 `$(error)` 拉闸——一行代码都还没开始编，可用平台列表里端端正正印着小写的 `tiger`。**错在别人的构建系统里**，犯错的位置和报错的位置隔了一层。

排查这类错的手法也就固定了：Yocto 层查不出问题时，顺着 do_compile 的日志往组件自己的构建系统里钻——`tmp/work/` 下该配方的 `temp/log.do_compile` 里有 make 的完整输出。

修正并复验：

```bash
# 修正：平台名改回与 plat/tiger/ 目录逐字符一致的小写
sed -i 's/TFA_PLATFORM = "Tiger"/TFA_PLATFORM = "tiger"/' \
    ~/workspace/meta-tiger/recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend

# 复验构建（任务签名变化自动重跑 do_compile）
bitbake trusted-firmware-a
```

构建通过，deploy 目录产物与 6.6.2 一致。老周路过补了一刀："这集 chapter 3 我看过——那次是 `LAYERSERIES_COMPAT` 写成 'Scarthgap'，同一个'专名大写'直觉，换了个地方又栽一回。"教训收成一条：`TFA_PLATFORM` 这类"名字即路径片段"的参数，取值去组件源码树里对，不凭语言直觉写。

### 6.9 本章小结

一天走完，早上白板上的三个路标全部落地：

- **6.1**：启动链三级各回答一个问题——BL1 管"第一段代码从哪来"，BL2 管"DDR 和后续镜像谁准备"，BL31 常驻 EL3 提供 PSCI 等运行时服务；一次 TF-A 构建三级一起产出。
- **6.2**：真实 SoC 上 BL1 由固化的 Boot ROM 加载，BSP 管不到也不用管；QEMU 用 `-bios` 扮演 Boot ROM，直接喂 BL1。
- **6.3**：自己数一遍再决策——meta-arm 配方里源码获取、裸机参数、EXTRA_OEMAKE 编排、install/deploy 逻辑全是现成的，复用 + bbappend 特化优于自写；判据可迁移，chapter 8 内核配方是它的对照组。仓库 ≠ layer：meta-arm 仓库装四个 layer，集合名还不一定跟目录同名。
- **6.4**：注册按 layer 算——meta-arm 依赖 arm-toolchain 集合，缺了连门都进不来；`LAYERDEPENDS_meta-tiger = "core meta-arm"` 补上跨 layer 引用声明。
- **6.5**：直拉仓库流——SRC_URI 经原配方预留的旋钮改指 tf-a-tiger，SRCREV 锁定提交；`COMPATIBLE_MACHINE` 认领是上岗证，漏了配方对本机不存在。
- **6.6**：`TFA_PLATFORM` 经 EXTRA_OEMAKE 变成 TF-A Makefile 的 `PLAT`，名字即路径片段；do_deploy 经 sstate 把三级固件和 fip.bin 送进 deploy 目录（fip 里 BL33 空着，前提是平台代码关掉 NEED_BL33）；TF-A 裸机代码屏蔽 TUNE 参数，chapter 5 的因果链到它门口就断。
- **6.7**：回填前先查——`virtual/trusted-firmware-a` 虚包不存在，认领改走 meta-arm 惯例（EXTRA_IMAGEDEPENDS 挂依赖链 + PREFERRED_VERSION 锁版本）；整镜构建报错没有变多；QEMU `-bios bl1.bin` 验证 BL1→BL2→BL31 全链，日志停在等待 BL33 处即为通过。
- **6.8**：两个坑互为呼应——依赖没声明，meta-arm 在场时毫无动静、缺席时只给一条指向孤儿 bbappend 的含糊 fatal，声明了报错才直指缺失的 layer；平台名大写是"错在别人的构建系统里"；查证手法分别是"模拟依赖缺席"和"钻进 do_compile 日志"。收尾顺序也有讲究——带着声明加层会被 add-layer 的整配置预校验回滚，先撤声明、层进齐、再恢复声明。

本章产出清单：

- `meta-tiger` 新增 `recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend`：TF-A 源码改指 tf-a-tiger，平台、构建目标、机器认领就位。
- `meta-tiger/conf/layer.conf`：`LAYERDEPENDS_meta-tiger` 追加 `meta-arm`。
- `meta-tiger/conf/machine/tiger-aarch64.conf`：新增 TF-A 集成声明两行（EXTRA_IMAGEDEPENDS + PREFERRED_VERSION）。
- TF-A 三级固件（bl1/bl2/bl31 + fip.bin）落 deploy 目录；QEMU 单独验证 TF-A 链路通过。
- 构建环境注册两个 meta-arm 子 layer（外部上游仓库，属环境配置，不进任何我方仓库）。

提交并打 tag。本章唯一被修改的我方仓库是 `meta-tiger`：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Integrate TF-A via meta-arm bbappend for tiger platform"
git tag chapter6
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter6`；`tf-a-tiger` 是固件组的开发态仓库，本章只读不写、不打 tag（与 chapter 4 对 qemu-tiger 的惯例相同）；`poky` 与 `meta-arm` 都是外部上游仓库，保持原样不打 tag；`bblayers.conf` 的注册改动属于本地构建环境，不进任何仓库。

后续任务清单：

- **task 08 / chapter 7**：集成 U-Boot——把 BL33 填上，串口日志就能从"等待移交"再往前走。U-Boot 规矩多（defconfig、dts、环境变量、boot script），而"配方提供 + 配置声明"这套模式将迎来第三次复用——这次 `virtual/bootloader` 的虚包是真的存在的，chapter 2 那句前瞻到那时才完全兑现。
- 伏笔：BL31 常驻 EL3 提供的 PSCI 今天只是名字——U-Boot 单核先跑起来，多核唤醒要等完整启动链串通后由 BL31 供出服务。

老周下班前在白板角落写了一行字：启动链，下半段已通电。

---

**延伸阅读**

1. TF-A 官方文档 v2.10（平台移植指南与构建系统说明，`PLAT` 参数与 `plat/<平台>/` 目录约定）：https://trustedfirmware-a.readthedocs.io/en/v2.10/
2. meta-arm layer 仓库与文档（layer 划分、TF-A 配方组织）：https://git.yoctoproject.org/meta-arm
3. Yocto 变量术语表，SRCREV / EXTRA_OEMAKE / PREFERRED_VERSION / EXTRA_IMAGEDEPENDS 条目：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
4. Yocto BSP 开发手册（BSP layer 中固件与 MACHINE 配置的组织约定）：https://docs.yoctoproject.org/5.0/bsp-guide/
