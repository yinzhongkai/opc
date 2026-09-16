## 9 NAND + UBI：把"硬盘"建好

周一早上，阿凯工位。白板上启动链图的最右格——Kernel——上周五已经涂实，格子下面那行字还留着：内核报到，启动链三件齐备——差一个能挂的根。今天白板旁边多画了一个空格，上面写着两个字：存储。

"板子有内核了，但还没有'硬盘'。"老周端着杯子站到白板前，"今天搞 rootfs。我们的板子上是 NAND，文件系统用 UBI/UBIFS。这是嵌入式的特色话题——你实习时玩的 Buildroot 打个镜像就完事的日子，到头了。"

"上周五您说'建房子'，就是指这个？"

"对。内核已经能跑到 rootfs 门槛了——panic 那行'Unable to mount root fs'是它在喊'给我一块地'。"老周拿起笔，在"存储"格下面列了一串，"今天的活我拆给你：先搞清楚 NAND 的脾气和 UBI 这层是干什么的；让构建系统产出 UBI 镜像；把 mkfs.ubifs 和 ubinize 两头的参数算明白；给 NAND 画分区表——立项时答应客户的 A/B，今天落成字节；产出的配置文件读懂它；内核侧把设备树和 defconfig 补齐；最后想办法挂上去看一眼。七步，顺序走。"

阿凯在本子上记下七条，抬头问了一句："最后那步'挂上去看一眼'——rootfs 还没法当根，怎么上去看？"

老周没答，把笔帽扣上："chapter 8 你的笔记里抄过一条边界说明——有个东西能桥接这段路。想起来再动手。"

### 9.1 NAND 与 UBI：为什么不是 ext4

#### 9.1.1 NAND 的脾气

动手之前，老周把阿凯叫到白板前，十分钟速讲。这一段是今天所有算术的地基，省不得。

**NAND Flash** 这种存储介质，chapter 2 读项目时点过名——非易失性存储，嵌入式设备的常客。它便宜、密度高，但脾气和磁盘、SD 卡完全不同，四条：

1. **按页写**。最小的写入单元是页（Page），tiger 这片 NAND 一页 2 KiB。想改一个字节？没法单独改，得按页来。
2. **按块擦**。写入之前必须先擦除，而擦除的最小单元是块（Erase Block）——tiger 这片一块 128 KiB，也就是 64 页。擦除的动作是把整块全写成 1。
3. **不能就地改写**。一个页从 1 写成 0 可以，想从 0 翻回 1，只能把所在的整块擦掉重来。所以"覆盖写"一个文件的实际操作是：把新数据写到别的空页，把旧页标记作废，攒够一个块再擦。
4. **有坏块，有寿命**。NAND 出厂就可能带 **坏块（Bad Block）**——制造缺陷导致永远不可用的块，用着用着还会新增；每块的擦写次数有上限（SLC——每个存储单元只存 1 bit 的那类 NAND——典型约十万次），擦爆了就变成新的坏块。

"这四条凑一起，就是答案。"老周说，"ext4 假设底下是个能按扇区随机改写的块设备——改一个 4 KiB 块，写回去就完了。NAND 做不到这件事。硬上 ext4，要么靠一层 FTL（闪存翻译层，SD 卡和 SSD 里内置的那种固件）把 NAND 伪装成块设备——咱们的裸 NAND 没有这层固件；要么写入放大的账很快把块擦爆，坏块没人管，文件系统哪天自己烂掉都不知道。"

"所以 UBI 干的活，相当于把 SSD 主控里那套固件搬进了内核？"阿凯问。

"方向对了。"老周点头，"而且它分得比主控干净。"

#### 9.1.2 MTD 与 UBI：把坏块和磨损收进一层

老周在白板上画了个分层图，四层：

```text
┌─────────────────────────────────────┐
│ UBIFS      文件系统：目录、文件、权限 │
├─────────────────────────────────────┤
│ UBI        卷管理：坏块管理 + 磨损均衡 │
├─────────────────────────────────────┤
│ MTD        Linux 的 Flash 存储抽象层  │
├─────────────────────────────────────┤
│ NAND Flash 物理介质：页写、块擦、有坏块 │
└─────────────────────────────────────┘
```

最底下是物理介质。往上一层是 **MTD（Memory Technology Device）**——Linux 的 Flash 存储抽象层，把"页写块擦有坏块"的硬件包装成内核里统一的设备形态：`/dev/mtd0`、`/dev/mtd1` 一族。MTD 只管抽象，不管坏块调度，直接往 mtd 设备上放普通文件系统，前三条脾气一条都没人接。

再接一层是 UBI——chapter 2 立过名字的卷管理子系统，今天把它落到实处。UBI 把整个 MTD 分区接管过来，干两件大事：**坏块管理**（发现坏块就拉黑，数据搬到好块，预留一部分块专门做替换）和 **磨损均衡（Wear Leveling）**——让每个块的擦写次数尽量拉平，不出现"某个块被日志文件擦爆、旁边的块还是新的"的偏科。这两件大事被 UBI 收进自己一层，上面的文件系统从此不用知道坏块的存在。

最上面是 UBIFS，UBI 之上的 Flash 文件系统——目录树、权限、日志、压缩，文件系统该管的它管，Flash 的脏活全推给楼下。

"内核里 UBI 这一层也可以直接裸露出来用——比如存放结构化数据但不需要文件系统的场合，那是后话。"老周说，"我们走全栈：MTD 之上 UBI，UBI 之上 UBIFS，rootfs 住 UBIFS。"

#### 9.1.3 PEB 与 LEB：一张图，两个块

最后这层概念是 9.3 算术的命根子。UBI 接管 MTD 分区后，把物理块换个名字管起来：**逻辑/物理擦除块（LEB / PEB，Logical / Physical Erase Block）**——PEB 对应 NAND 上的一个物理擦除块，LEB 是 UBI 卷里对外可见的逻辑块。上层只见 LEB，坏块替换时 UBI 悄悄把 LEB 重新指向另一个 PEB，上层无感。

关键在一个"差"字：**一个 LEB 比一个 PEB 小**。每个 PEB 的开头，UBI 要占用两个页放自己的元数据——一个页存擦除计数头（这块擦过多少次），一个页存卷标识头（这块属于哪个卷、是第几个 LEB）。剩下的才是 LEB 的可用空间：

```text
一个 PEB（128 KiB = 64 页 × 2 KiB）
┌──────┬──────┬────────────────────────────┐
│ 页 0 │ 页 1 │ 页 2 ~ 页 63               │
│ EC 头│ VID 头│  ← 这就是 LEB：124 KiB →  │
└──────┴──────┴────────────────────────────┘
UBI 元数据 2 页（4 KiB）    128 KiB - 4 KiB = 124 KiB
```

"记住这个图。"老周用笔点了点，"今天后面所有的参数，全围着这两个块转——谁按 PEB 算、谁按 LEB 算，认错了，全盘皆输。"

阿凯把图抄进本子，在"124 KiB"下面画了两道线。他还不知道，这道线下午就要绊他一次。

### 9.2 IMAGE_FSTYPES 加 ubi：类里发生了什么

#### 9.2.1 兑现 chapter 5 的注释

概念打完底，动配置。chapter 5 写 machine 配置时，第三段有一行注释，原话是"镜像输出先用通用格式；UBI 格式等 chapter 9 NAND 建好再加"。今天就是"等"到的那一天。打开 `tiger-aarch64.conf`，把那行注释划掉改写，`IMAGE_FSTYPES` 追加 `ubi`：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（第三段修订）

# ---- 机器能力与镜像格式 ----
# 只陈述硬件事实：串口、RTC；不替 Distro 决定软件策略
MACHINE_FEATURES = "ext2 rtc serial vfat"
# 镜像输出：通用格式之外追加 UBI（NAND 根文件系统，9.2 起；
# IMAGE_TYPEDEP 机制会自动带出 ubifs 中间产物，9.2.2 详解；
# 依赖的 MKUBIFS_ARGS/UBINIZE_ARGS 见第九段，9.3）
IMAGE_FSTYPES += "tar.bz2 ext4 ubi"
```

注意只加了 `ubi` 一个词。`ubifs` 没写——这是故意的，下一小节看机制就知道为什么。先做解析级验证，看变量终值里有没有它：

```bash
# 查 IMAGE_FSTYPES 终值（解析级验证；全零 SRCREV 占位下 -e 不触发 fetch）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e core-image-minimal | grep ^IMAGE_FSTYPES=
```

输出（本机实测）：

```text
IMAGE_FSTYPES=" tar.bz2 ext4 ubi"
```

`ubi` 进去了。眼尖的会注意到 `tar.bz2` 前面还有个空格——不是手滑：IMAGE_FSTYPES 的默认值只是 bitbake.conf 里 `?=` 弱赋值的 `tar.gz`（bitbake.conf:845），`?=` 是延迟弱赋值——`+=` 一落笔变量即已定义，弱默认值整体作废，追加发生在空串上，BitBake 拼词时补了一个连接空格。和 chapter 8 在 PROVIDES 里见过的双空格同族，都是 BitBake 的拼接痕迹，无害。但这只是纸面——`ubi` 这两个字一进 IMAGE_FSTYPES，BitBake 内部发生了什么？

#### 9.2.2 image_types.bbclass：ubi 这两个字的下游

接信的还是一个类。chapter 1 见过 image.bbclass 给镜像配方提供标准流程；今天认识它的搭档——**镜像类型类（image_types.bbclass）**：Yocto 核心类文件，IMAGE_FSTYPES 名单里每个格式词的具体打包逻辑全在它里面，`ubi`、`ext4`、`tar.bz2` 各有一段 `IMAGE_CMD:<格式>`。

先回答阿凯的问题："ubi 和 ubifs 到底什么关系？为什么只加一个？"答案在三行源码里：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/image_types.bbclass（关键段节选，行号本机核实）
IMAGE_CMD:ubi () {
	multiubi_mkfs "${MKUBIFS_ARGS}" "${UBINIZE_ARGS}"
}
IMAGE_TYPEDEP:ubi = "${UBI_IMGTYPE}"

IMAGE_CMD:ubifs = "mkfs.ubifs -r ${IMAGE_ROOTFS} -o ${IMGDEPLOYDIR}/${IMAGE_NAME}.ubifs ${MKUBIFS_ARGS}"
```

（以上节选自第 241-246 行。）读法分两层：

- **IMAGE_TYPEDEP 是格式间的依赖声明**。第 244 行说：要产 `ubi` 格式的镜像，先得有 `UBI_IMGTYPE` 指定的格式——而 `UBI_IMGTYPE ?= "ubifs"`（第 173 行）。所以 IMAGE_FSTYPES 写一个 `ubi`，构建系统会自动先把 `ubifs` 产物编出来，再喂给 `ubi` 的打包逻辑。**一个 `ubi` 带出一个 `ubifs`，这就是为什么 machine conf 里只写了一个词。** 立项时的纸面方案把"加 ubifs 和 ubi"两个词并列，机制上是冗余——`ubifs` 已在依赖链里，再写一遍只是显式一点。
- **两条 IMAGE_CMD 对应 mtd-utils 的两个工具**。`IMAGE_CMD:ubifs` 调 `mkfs.ubifs`：把 rootfs 目录打成 UBIFS 文件系统镜像（.ubifs 文件）；`IMAGE_CMD:ubi` 调 `multiubi_mkfs`，内部先自动写一份 ubinize 配置，再调 `ubinize` 把 .ubifs 包成 UBI 卷镜像（.ubi 文件）——那份自动生成的配置就是 9.5 的主角。两个 mkfs 工具都来自 `mtd-utils`——MTD 子系统的配套工具包，主机侧由 mtd-utils-native 配方提供，目标侧的 `ubiattach` 等命令也由它的 target 形态提供，9.7 还会见到它。

这两个工具什么时候进的构建系统？还是老规矩，depends 链上白纸黑字：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/image_types.bbclass（关键行节选，296-298 行）
do_image_ubi[depends] += "mtd-utils-native:do_populate_sysroot"
do_image_ubifs[depends] += "mtd-utils-native:do_populate_sysroot"
do_image_multiubi[depends] += "mtd-utils-native:do_populate_sysroot"
```

`do_image_ubi` 一族开工前，mtd-utils-native 必须先装好——mkfs.ubifs 和 ubinize 从它的 sysroot 里来。这条链和 chapter 5 以来见过的所有 do_image 依赖同一个句式：声明格式词，类负责拉工具、跑命令、把产物放进 deploy 目录。而且它不只是纸面声明——下一小节的报错现场会留下脚印：do_image_ubifs 开工前的日志里写着 `NOTE: Installed into sysroot: ['e2fsprogs-native', 'mtd-utils-native', 'lzo-native']`，depends 白纸黑字在现场兑现。

> **💡 提示**：反过来写——IMAGE_FSTYPES 只写 `ubifs` 不写 `ubi`——只会得到 .ubifs 半成品：mkfs.ubifs 跑了，ubinize 没跑，NAND 上没有可落盘的 UBI 卷镜像。

另外 `ubi` 不是唯一的多卷通道：类里还有 `multiubi` 格式和 `MULTIUBI_BUILD` 机制（同一份镜像按多组参数各打一份 UBI），9.5 末尾点名它的适用场景后弃用。

> **💡 提示**：版本边界——mtd-utils 属全书版本表"不锁定"族，随 Scarthgap 走：当前提供 2.1.6（本机实测：`mkfs.ubifs (mtd-utils) 2.1.6` / `ubinize (mtd-utils) 2.1.6`），mkfs.ubifs/ubinize 的行为以本机版本为准。一个可复现性细节：主机侧裸跑 sysroot 里的 mkfs.ubifs 要先用 LD_LIBRARY_PATH 指上 lzo-native 的库目录（它动态链接 liblzo2.so.2），ubinize 不链 lzo、不需要——9.8.1 的主机侧演示会用到这条。

#### 9.2.3 预期报错：参数没设，两道防线拦下

`ubi` 加上了，参数还没设——先让它错一次，看看类里埋的检查长什么样。这正是本章的预期报错演示。不过实测的剧本和预想不完全一样：**防线有两道，报错也分两场**，一场一场看。

tiger-aarch64 的内核 SRCREV 还是全零占位，整镜构建会卡在 fetch。演示换个干净环境：另开一个临时构建目录，MACHINE 用 qemuarm64、内核提供者指 linux-dummy（chapter 5 起一直顶岗到上周的那个空壳，今天再出山一次——不为内核，只为绕开内核构建），IMAGE_FSTYPES 加 `ubi` 但**不设** MKUBIFS_ARGS/UBINIZE_ARGS：

```bash
# 预期报错演示环境：临时构建目录，绕开内核构建，只验 do_image 的参数检查
cd ~/workspace/poky
source oe-init-build-env ../build-ubi-demo
# 在 conf/local.conf 中设置（演示用，本章结束即弃）：
#   MACHINE = "qemuarm64"
#   PREFERRED_PROVIDER_virtual/kernel = "linux-dummy"
#   IMAGE_FSTYPES:append = " ubi"
bitbake core-image-minimal
```

**第一场：两个参数都不设。** 预想中拦人的是类里的 bbfatal，实测却轮不到它出场——四千多个任务跑到尾声，先死的是 do_image_ubifs：

```text
ERROR: core-image-minimal-1.0-r0 do_image_ubifs: ExecutionError('/home/<your-username>/workspace/poky/build-ubi-demo/tmp/work/qemuarm64-poky-linux/core-image-minimal/1.0/temp/run.do_image_ubifs.<pid>', 255, None, None)
ERROR: Logfile of failure stored in: /home/<your-username>/workspace/poky/build-ubi-demo/tmp/work/qemuarm64-poky-linux/core-image-minimal/1.0/temp/log.do_image_ubifs.<pid>
Log data follows:
| ...
| Error: min. I/O unit was not specified (use -h for help)
| WARNING: exit code 255 from a shell command.
NOTE: recipe core-image-minimal-1.0-r0: task do_image_ubifs: Failed
ERROR: Task (/home/<your-username>/workspace/poky/meta/recipes-core/images/core-image-minimal.bb:do_image_ubifs) failed with exit code '1'
...
Summary: 1 task failed:
  /home/<your-username>/workspace/poky/meta/recipes-core/images/core-image-minimal.bb:do_image_ubifs
```

为什么死在 ubifs 而不是 ubi？回看 9.2.2 的依赖链：`IMAGE_TYPEDEP:ubi = "${UBI_IMGTYPE}"` 让 do_image_ubifs 排在 do_image_ubi 前面；`IMAGE_CMD:ubifs` 把空的 `${MKUBIFS_ARGS}` 直接喂给 mkfs.ubifs，**工具自己的用法检查先炸了**——"min. I/O unit was not specified"，-m 都没给。这道防线不是类埋的，是 mkfs.ubifs 自带的参数检查；它喊的话里一个字不提 UBINIZE_ARGS，因为执行根本没走到那一步。

**第二场：设一半。** 把 MKUBIFS_ARGS 设上、UBINIZE_ARGS 留空再构建——这次 do_image_ubifs 顺利通过，类里埋的检查才在 do_image_ubi 炸出来：

```text
ERROR: core-image-minimal-1.0-r0 do_image_ubi: MKUBIFS_ARGS and UBINIZE_ARGS have to be set, see http://www.linux-mtd.infradead.org/faq/ubifs.html for details
ERROR: core-image-minimal-1.0-r0 do_image_ubi: ExecutionError('/home/<your-username>/workspace/poky/build-ubi-demo/tmp/work/qemuarm64-poky-linux/core-image-minimal/1.0/temp/run.do_image_ubi.<pid>', 1, None, None)
```

这才是预想中的 bbfatal——`multiubi_mkfs` 开头先检查两个参数变量，有空就 `bbfatal` 当场毙掉任务（image_types.bbclass:194-196）。两道防线一个共同点：**都拦在正道上**。和"什么都不发生"系列不同，这次是构建系统（和工具）主动喊停：它知道自己缺什么，还告诉你去哪查。`MKUBIFS_ARGS` 和 `UBINIZE_ARGS`——两个新名字，下一节把它们算出来。

"第二场报错里还附了个网址。"阿凯指着倒数第二行。

"那是 linux-mtd 项目的 UBI/UBIFS FAQ，这个领域的祖师爷文档。"老周说，"网址抄进你的延伸阅读清单——正文别放链接，老规矩。"

### 9.3 MKUBIFS_ARGS 与 UBINIZE_ARGS：围着两个块转

#### 9.3.1 NAND 数据手册摆上桌

两个参数变量，七个数字，全部从 NAND 的几何参数推出来。老周把 tiger 这片 NAND 的数据手册参数写在白板上——立项时硬件组定的料，就三行：

| 参数 | 值 |
|------|---|
| 页大小（Page Size） | 2 KiB（2048 字节，另有带外备用区 OOB——Out-Of-Band，每页附带的几十字节，本章不涉及） |
| 擦除块大小（Erase Block） | 128 KiB（131072 字节 = 64 页） |
| 总容量 | 512 MiB（4096 个块） |

"参数就这些。"老周放下笔，"MKUBIFS_ARGS 要 -m、-e、-c 三个数，UBINIZE_ARGS 要 -m、-p、-s、-O 四个数。哪个数从哪来，你自己推。推完我看。"

**老周没有给答案。** 阿凯翻开 mkfs.ubifs 和 ubinize 的手册页，把七个参数的含义抄下来，对着 9.1.3 那张 PEB/LEB 图开始推。

#### 9.3.2 阿凯的推导：PEB 和 LEB 各就各位

半小时后，阿凯把推导结果摆上白板：

**MKUBIFS_ARGS**——mkfs.ubifs 造的是 UBIFS 文件系统，UBIFS 活在 LEB 的世界里，所以它的参数全按逻辑块算：

- `-m 2048`：**最小 I/O 单元**，即 NAND 页大小。UBIFS 的读写按页对齐，2 KiB 页就填 2048。
- `-e 126976`：**LEB 尺寸**。这是整张表最容易错的数——不是 128 KiB！按 9.1.3 的图，LEB = PEB − 2 页元数据 = 131072 − 4096 = **126976 字节**（124 KiB）。UBIFS 的文件内容、索引节点全装在这个空间里。
- `-c 3968`：**最大 LEB 数**，即这个文件系统将来最大能占多少个 LEB。算法：nand0 分区给到 UBI 的物理块一共 4056 块（分区表 9.4 画，这里先报数）；UBI 自己要扣掉卷表 2 块，再扣坏块替换预留——注意这笔预留的基数按**整片** NAND 算（内核默认上限每 1024 块留 20，4096 块约扣 80 块），可用约 3974 块；再留几块余量给运行期的磨损均衡调度，**取 3968**。-c 是"上限"，宁小勿大——拍小了浪费点空间，拍大了挂载时内核会拒绝（这是另一类常见踩法，参数表旁边记一笔就够）。

**UBINIZE_ARGS**——ubinize 干的是把文件系统镜像铺进物理块序列，它的参数按物理块算：

- `-m 2048`：同样是最小 I/O 单元，页大小。
- `-p 131072`：**PEB 尺寸**，物理擦除块原样大小 128 KiB。ubinize 按这个尺寸把输出切成一个个 PEB 排好。
- `-s 2048`：子页（Sub-page）大小——某些 NAND 页内还能分更小的写单元；tiger 这片按页写，子页即页，填 2048。
- `-O 2048`：VID 头在块内的偏移，即 EC 头占掉的那个页之后——1 个页，2048。

推完阿凯自己有点发抖：七个数里五个是 2048，剩下两个一个 131072 一个 126976，差 4096。

老周看完，只问了一句："**你 -e 用的是哪个块？**"

"LEB。"阿凯指着图上那道线，"126976，扣掉 EC 头和 VID 头的两个页。mkfs.ubifs 看见的世界全是 LEB，PEB 是 ubinize 那边 -p 的事。"

"想清楚了就行。"老周说，"**三个参数围着两个块转，块认错一个，全盘皆输。** 今天你只是算——等你敲进配置里、敲错一次，就知道这句话的分量了。"

这道引信，9.8 坑 1 会烧到头。

#### 9.3.3 参数落盘：machine conf 第九段

推导过了老周的目，落配置。归属没有悬念：这两个参数描述的是"这块板的 NAND 长什么样"，machine conf 是天经地义的家——和 KERNEL_IMAGETYPE、SERIAL_CONSOLES 同桌。追加第九段：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf（接 chapter 8 全文追加）

# ---- UBI 镜像参数（NAND 几何：页 2 KiB / 擦除块 128 KiB / 容量 512 MiB）----
# 算术见 9.3：mkfs.ubifs 认 LEB（-e 126976 = 块 131072 - 元数据 2 页），
# ubinize 认 PEB（-p 131072 原样块大小）；-c 为最大 LEB 数（上限宁小勿大）
MKUBIFS_ARGS ?= "-m 2048 -e 126976 -c 3968"
UBINIZE_ARGS ?= "-m 2048 -p 131072 -s 2048 -O 2048"
# UBI 根卷卷名：对齐 boot.cmd 的 ${bootvol} 缺省值 rootfs_a（9.4.2 裁决）
UBI_VOLNAME ?= "rootfs_a"
```

三个变量，逐个交代：

- **MKUBIFS_ARGS** 与 **UBINIZE_ARGS**：首次正式引入——前者是传给 mkfs.ubifs 的参数列表，后者传给 ubinize，9.2.2 的 `IMAGE_CMD:ubifs` 和 `multiubi_mkfs` 里那两个 `${MKUBIFS_ARGS}`、`${UBINIZE_ARGS}` 就是它们的落点。都写成弱赋值 `?=`——产品后期换 NAND 料号时，local.conf 或衍生 machine 可以整体覆盖，不必改 layer。
- `UBI_VOLNAME`：就地解释——UBI 根卷的卷名，image_types.bbclass:171 给了默认值 `${MACHINE}-rootfs`（即 `tiger-aarch64-rootfs`），我们显式覆盖为 `rootfs_a`。为什么叫这个名字，下一节裁决——它必须和 chapter 7 boot.cmd 里 `setenv bootvol rootfs_a` 那行**一字不差地对上**，这是纸面契约第一次反向约束配置值。

参数链画出来，和 chapter 6/7/8 的参数链图同一个句式：

```text
machine conf（第九段）
  MKUBIFS_ARGS ──→ IMAGE_CMD:ubifs ──→ mkfs.ubifs -m 2048 -e 126976 -c 3968 -r <rootfs> -o <镜像>.ubifs
  UBINIZE_ARGS ──→ multiubi_mkfs ────→ ubinize -m 2048 -p 131072 -s 2048 -O 2048 ubinize-<镜像>.cfg
  UBI_VOLNAME  ──→ write_ubi_config ─→ ubinize-<镜像>.cfg 的 vol_name 行（9.5）
```

**BitBake 变量只是信使，接信的是 mtd-utils 的两个工具。** 落盘后用 chapter 5 以来的老手法验证终值：

```bash
# 查四个变量的终值（解析级验证）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e core-image-minimal | grep -E "^(IMAGE_FSTYPES|MKUBIFS_ARGS|UBINIZE_ARGS|UBI_VOLNAME)="
```

输出（本机实测）：

```text
IMAGE_FSTYPES=" tar.bz2 ext4 ubi"
MKUBIFS_ARGS="-m 2048 -e 126976 -c 3968"
UBINIZE_ARGS="-m 2048 -p 131072 -s 2048 -O 2048"
UBI_VOLNAME="rootfs_a"
```

（第一行的前导空格，9.2.1 已经认过亲：`+=` 的拼接痕迹。）"七个参数名在我脑子里打转。"阿凯揉了揉眼睛，"下次换料号，这套算术还得重跑一遍。"

"所以我把数据手册那三行钉在白板上了。"老周说，"参数推导的源头是硬件，不是记忆。"

### 9.4 NAND 分区表：纸面布局 vs 已交付脚本

#### 9.4.1 A/B 分区，从承诺落成字节

分区表这事，立项那天就有个承诺压着：客户要 OTA 安全升级——升级失败能回滚。落实这个承诺的物理基础，今天正式引入：**A/B 分区（A/B partition）**——双分区布局策略，系统备两套，升级写另一套，起不来还能指回旧套。这套策略立项时就定下了；chapter 7 它在 boot.cmd 里长出了骨架——那段 `if test "${bootpart}" = "b"` 的判断；今天它落成 NAND 上的字节布局。

立项时的纸面方案给 chapter 9 画的分区表是九段：bootloader / env / kernel_a / dtb_a / rootfs_a / kernel_b / dtb_b / rootfs_b / data。阿凯照着这张纸面布局画到一半，停住了。

"周哥，九段表里 kernel_a、dtb_a 是独立分区。可 chapter 7 的 boot.cmd 已经交付了——它是 `ubifsload ${kernel_addr_r} /boot/Image`，从 UBIFS 卷里按路径读文件。这俩对不上啊。"

#### 9.4.2 kernel/dtb 住哪：boot.cmd 裁决

这是今天第一个真设计决策点。两条路线摆上桌：

- **kernel/dtb 占独立 MTD 分区**（纸面九段表）：内核和 dtb 各占一块裸 MTD 空间，U-Boot 用 `nand read` 按偏移读。好处：加载不依赖 UBI 驱动就绪，简单粗暴。代价：内核升级时没有文件系统保护，写一半断电就是砖；A/B 切换要管理六个分区的指针；且——**boot.cmd 已经按另一条路交付了**。
- **kernel/dtb 住 UBIFS 卷内**（boot.cmd 既成事实）：内核和 dtb 作为 `/boot/Image`、`/boot/tiger.dtb` 两个文件住在 rootfs 卷里，U-Boot 用 `ubifsload` 读。UBIFS 有日志、有校验，写一半断电能回滚到旧版本——这正是 OTA 要的安全性；A/B 切换只需要指卷名，`bootpart` 一个变量管全部，boot.cmd 里那个 `if` 已经写好了。代价：U-Boot 必须带 UBI/UBIFS 支持——boot.cmd 里那三族 ubi 命令的前提就是这份能力，但 chapter 7 交付的是**纸面契约**，u-boot-tiger 的 defconfig 里还没核过这笔账；chapter 10 真跑之前要在那里核一遍（记在 chapter 10 的账上）。

"裁决依据不是哪条路更优雅，"老周说，"是**哪条路已经交付**。boot.cmd 是 chapter 7 的产出物，`ubifsload` 那两行是 chapter 8 反复核对的接力棒——内核和 dtb 的 deploy 文件名，就是照着它俩的名字备的货。纸面分区表给已交付的脚本让路。"

分区表就此精简：**MTD 分三段，nand0 整段交给 UBI，卷内再分三卷**。

#### 9.4.3 三段分区 + 三卷布局

最终布局，师徒俩一起画定：

```text
NAND 512 MiB（4096 PEB × 128 KiB）
┌──────────────┬──────────┬───────────────────────────────────┐
│  bootloader  │   env    │              nand0                │
│ 0 ~ 4 MiB    │ 4~5 MiB  │  5 MiB ~ 512 MiB（507 MiB）       │
│ （32 PEB）   │（8 PEB） │        （4056 PEB）               │
│ TF-A + U-Boot│ U-Boot   │  整段交给 UBI 管理                │
│              │ 环境变量 │                                   │
└──────────────┴──────────┴───────────────────────────────────┘
                              │ nand0 内的 UBI 卷：
                              ├─ rootfs_a   根文件系统 A 套（本章构建产出）
                              ├─ rootfs_b   根文件系统 B 套（chapter 10 总装时建）
                              └─ data       持久数据卷（日志、事件；chapter 10 建）
```

三段各有说法：

- **bootloader 段（4 MiB，32 块）**：TF-A 的 BL1/BL2/FIP 和 U-Boot 的落点，chapter 6/7 的产出游到 NAND 上的家。4 MiB 是按 FIP + u-boot.bin 的实际体量放足余量拍的。
- **env 段（1 MiB，8 块）**：U-Boot 环境变量的持久化存储——`bootpart` 这个变量将来被 OTA 流程改写后，就住在这里。
- **nand0 段（507 MiB，4056 块）**：剩下全部，一个 MTD 分区，整段交给 UBI。label 就叫 `nand0`——**这个名字不是随手起的**：boot.cmd 里 `ubi part nand0` 那行点名了它，dts 分区表里的 label 必须和脚本一字不差。这是纸面契约第二次反向约束配置值。

三卷里，本章只构建 rootfs_a；rootfs_b 和 data 是 chapter 10 总装时的活——本章 9.5 会看到，自动生成的 ubinize 配置只能产单卷，多卷布局需要自写配置，那是总装的一部分。

"和 OTA 的关系收个尾。"老周说，"A/B 双卷 + bootpart 指针 + env 持久化，升级的**物理基础**今天齐了。升级流程本身——怎么写另一套、怎么改指针、起不来怎么回滚——那是产品功能层面的设计，本书不展开，目录里也没有这一章。"

### 9.5 ubinize.cfg：先构建，再读懂它

#### 9.5.1 对照组构建：deploy 目录的三件新住客

参数齐了，让构建真跑一次。tiger-aarch64 的整镜构建还卡在 linux-tiger 仓库的占位 SRCREV 上（fetch 过不了），先用 qemuarm64 对照组把**机制**验掉——9.2.3 那个演示环境补全参数，接着用：

```bash
# 对照组整镜构建：qemuarm64 + linux-dummy + 全套 UBI 参数（首次构建约 1-3 小时）
cd ~/workspace/poky
source oe-init-build-env ../build-ubi-demo
# 在 conf/local.conf 中追加（对照组专用，与 tiger 几何同参数只为验证机制）：
#   MKUBIFS_ARGS = "-m 2048 -e 126976 -c 3968"
#   UBINIZE_ARGS = "-m 2048 -p 131072 -s 2048 -O 2048"
bitbake core-image-minimal
```

构建完成后看 deploy 目录，三件新住客（本机实测，关键行）：

```bash
# 查看 deploy 目录新增的 UBI 三件套
ls -l tmp/deploy/images/qemuarm64/
```

```text
# ...（qemuarm64 原有产物，略）
-rw-r--r-- 2 <your-username> <your-username> 10223616 ... core-image-minimal-qemuarm64.rootfs-<时间戳>.ubi
-rw-r--r-- 2 <your-username> <your-username>  9650176 ... core-image-minimal-qemuarm64.rootfs-<时间戳>.ubifs
lrwxrwxrwx 2 <your-username> <your-username>       54 ... core-image-minimal-qemuarm64.rootfs.ubi -> core-image-minimal-qemuarm64.rootfs-<时间戳>.ubi
lrwxrwxrwx 2 <your-username> <your-username>       56 ... core-image-minimal-qemuarm64.rootfs.ubifs -> core-image-minimal-qemuarm64.rootfs-<时间戳>.ubifs
-rw-r--r-- 2 <your-username> <your-username>      288 ... ubinize-core-image-minimal-qemuarm64.rootfs-<时间戳>.cfg
```

命名形态先认两个细节：一是文件名里有 `.rootfs` 一段——IMAGE_NAME_SUFFIX 的默认值就是 ".rootfs"，拼在镜像名和时间戳之间（9.7 会见到一个不设后缀的对照：initramfs 配方把它置空，产物就没有这一段）；二是**三件套的实体全带时间戳**，不带时间戳的裸名只有 .ubi/.ubifs 两个符号链接——cfg 没有裸名链接，引用它得认准带时间戳的那一份。

三件套各说各的话：`.ubifs` 是 mkfs.ubifs 的产物（IMAGE_TYPEDEP 自动带出来的依赖格式——只写 `ubi` 一个词，它照样在场，机制实证）；`.ubi` 是 ubinize 的产物，**可以按 PEB 序列直接落盘 NAND 的东西**；第三个文件 `ubinize-<镜像名>-<时间戳>.cfg` 是意外之喜——没人写过它，它自己出现在 deploy 目录里。

<!-- 【待验证·阻塞级】C-W18：tiger-aarch64 组（machine conf 第九段生效后）的三件套清单待仓库就位后实测回填（命名形态除机器名外应与对照组同构） -->

"这个 cfg 哪来的？"阿凯问。

"9.2 读过它的出处，回去找。"老周说。

#### 9.5.2 逐行解读：write_ubi_config 的手笔

回 image_types.bbclass 翻——9.2.2 读的 `multiubi_mkfs` 里有一行 `write_ubi_config "${vname}"`，调的就是这个函数（第 175-187 行）：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/image_types.bbclass（write_ubi_config 全文，175-187 行）
write_ubi_config() {
	local vname="$1"

	cat <<EOF > ubinize${vname}-${IMAGE_NAME}.cfg
[ubifs]
mode=ubi
image=${IMGDEPLOYDIR}/${IMAGE_NAME}${vname}.${UBI_IMGTYPE}
vol_id=0
vol_type=${UBI_VOLTYPE}
vol_name=${UBI_VOLNAME}
vol_flags=autoresize
EOF
}
```

看懂了：cfg 是**类自动生成的**——ubinize 需要一份描述卷布局的 INI 配置，类按变量现写一份，用完还搬进 IMGDEPLOYDIR 留档（第 211 行的 `mv`）。立项时的方案稿写"ubinize.cfg 的生成"，言下之意像是要手写——Scarthgap 的实际机制是自动生成单卷配置，本章的活是**读懂它**。对照组实测全文：

```ini
# 文件路径：tmp/deploy/images/qemuarm64/ubinize-core-image-minimal-qemuarm64.rootfs-<时间戳>.cfg（自动生成，非仓库文件）
[ubifs]
mode=ubi
image=<IMGDEPLOYDIR 绝对路径>/core-image-minimal-qemuarm64.rootfs-<时间戳>.ubifs
vol_id=0
vol_type=dynamic
vol_name=qemuarm64-rootfs
vol_flags=autoresize
```

（`image=` 一行的 IMGDEPLOYDIR 在构建目录里实际指向 `tmp/work/qemuarm64-poky-linux/core-image-minimal/1.0/deploy-core-image-minimal-image-complete/`——类先把 cfg 和 .ubifs 都放在任务私有的 deploy 暂存目录，do_image_complete 再一起搬进公共 deploy 目录。）逐行过：

- `[ubifs]` / `mode=ubi`：段名与模式——告诉 ubinize 这段描述的是一个 UBIFS 镜像，按 UBI 卷打包。
- `image=`：输入文件，即 9.2.2 那条依赖链先产出的 .ubifs——注意它指向的是**带时间戳的实体**，不是裸名链接。
- `vol_id=0`：卷号 0。这个 0 后面会出现在目标板的设备名里——`/dev/ubi0_0`，9.7 挂载时见实物。
- `vol_type=dynamic`：动态卷（对应只读场景的 static 卷），由 `UBI_VOLTYPE ?= "dynamic"`（第 172 行）给出，根文件系统要读写，动态卷是正解。
- `vol_name=qemuarm64-rootfs`：对照组没设 UBI_VOLNAME，落在默认值 `${MACHINE}-rootfs` 上。**tiger 组这一行会是 `rootfs_a`**——9.3 写下的 UBI_VOLNAME 在这里落地，和 boot.cmd 的 `${bootvol}` 缺省值一字不差，9.3 那次反向约束在这里收口。
- `vol_flags=autoresize`：自动扩卷标志——UBI 第一次 attach 时，把这个卷扩到吃掉分区里所有剩余空间。

autoresize 这行值得一个设计注记：

> **⚠️ 注意**：autoresize 对单卷镜像是便利，对 A/B 多卷布局是**冲突源**——rootfs_a 自动扩到吃满整个 nand0，rootfs_b 和 data 就没有立足之地了。本章的测试镜像是单卷，autoresize 正好让卷吃满 507 MiB，无害且方便；chapter 10 总装 A/B 全布局时，要自写多卷 ubinize.cfg——给每个卷显式 `vol_size=`，把 autoresize 只留给 data 卷（让它吃掉 A/B 之外的剩余空间）。

自写多卷 cfg 这条路，类里其实也有个官方对应物：`multiubi` 格式 + `MULTIUBI_BUILD`（multiubi_mkfs 就是为它写的）——能为同一镜像按多组参数各打一份 UBI，适用场景是"同一份 rootfs 进多块不同板子"；tiger 的 A/B 是"同一板子上多个不同的卷"，不是它的菜，点名即弃，chapter 10 自写配置。
### 9.6 内核侧：dts 分区表与 defconfig 增量

#### 9.6.1 chapter 8 预埋的三行，今天配上硬件

镜像侧齐了，轮到内核。先回收一个伏笔。阿凯翻开 chapter 8 交付的 tiger_defconfig，那三行还在，头顶的注释行也还在：

```text
# chapter 8 交付的 tiger_defconfig 关键行（已在仓库，上周预埋至今）
# MTD / UBI / UBIFS：NAND 根文件系统的内核侧支持（chapter 9 伏笔）
CONFIG_MTD=y
CONFIG_MTD_UBI=y
CONFIG_UBIFS_FS=y
```

"'chapter 9 伏笔'——今天就是 chapter 9 了。"阿凯把注释行指给老周，"当时板子上还没有 NAND，为什么先把这三行写进去？"

"defconfig 是**平台级清单**。"老周说，"dts 和 defconfig 落笔时，把这块板最终要什么一次备齐；构建系统这边的接线——machine conf、分区表——随后跟上。三行从上周预埋到今天，等的全是今天的东西：MTD 之下要有真实的 NAND 控制器节点（dts），MTD_UBI 之上要有真实的卷（9.5 的镜像），UBIFS 之上要有真实的 rootfs（9.7 挂载）。"

但三行还不够——**NAND 控制器驱动**不在其中。chapter 8 当时只有空框节点，驱动符号没处可指；今天 dts 补节点，defconfig 补驱动，两件一起办。

#### 9.6.2 dts：NAND 节点与 fixed-partitions

chapter 8 的 tiger.dts 里，NAND 控制器节点只画了个框。今天把框填实：控制器节点本体 + 分区表。分区表的声明语法就地介绍——`fixed-partitions`：设备树里声明 MTD 固定分区的标准写法，在控制器节点下挂一个 `partitions` 子节点，每个分区一个子节点，`reg` 给偏移和长度，`label` 给名字。

<!-- 【待验证·阻塞级】C-W17：以下为 tiger.dts NAND 段的示意形态（linux-tiger 仓库尚不存在）。控制器 compatible 串、寄存器地址、时序参数以 qemu-tiger 的 NAND 模型实现为准；存放路径的 vendor 子目录层级以仓库实际为准 -->

```text
// 文件路径：~/workspace/linux-tiger/arch/arm64/boot/dts/<vendor>/tiger.dts（NAND 段补全，示意形态）

// NAND 控制器节点（chapter 8 的空框，今天填实）
// compatible/寄存器/时序以 qemu-tiger NAND 模型为准
nand_controller: nand-controller@<基地址> {
	compatible = "<控制器兼容串>";
	reg = <0x0 0x<基地址> 0x0 0x1000>;
	// ...（时钟、中断，以仓库实际内容为准）

	nand@0 {
		reg = <0>;

		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			// 9.4.3 分区表：偏移/长度均按 128 KiB 擦除块对齐（9.8 坑 2 的教训）
			bootloader@0 {
				label = "bootloader";
				reg = <0x0000000 0x0400000>;	// 0 ~ 4 MiB，32 块
			};
			env@400000 {
				label = "env";
				reg = <0x0400000 0x0100000>;	// 4 ~ 5 MiB，8 块
			};
			nand0@500000 {
				label = "nand0";		// 对齐 boot.cmd 的 "ubi part nand0"
				reg = <0x0500000 0x1FB00000>;	// 5 ~ 512 MiB，4056 块
			};
		};
	};
};
```

三个分区的 label 将来就是 `/proc/mtd` 里看得见、U-Boot 和内核命令行里点得名的名字。`nand0` 的 label 和 boot.cmd 的 `ubi part nand0` 对表——9.4.3 说的第二次反向约束，落笔就在这一行。

> **📖 深入阅读**：内核命令行上还有一种分区方案——`mtdparts=<控制器设备名>:4M(bootloader),1M(env),-(nand0)` 这样的参数（冒号前是 MTD 设备名，括号里才是分区 label），内核启动时按它现场切分区。本书不走这条路：分区表住 dts 是单一真相源，mtdparts 等于同一事实写第二遍，改一处忘一处的祸根（chapter 8 谈 defconfig 归属时的那句"两份清单真相是祸根"，在这里一字不差地成立）。mtdparts 只需认得，见到别人的板子用它会读即可。

#### 9.6.3 defconfig 增量：五步闭环走一遍

驱动符号要进 defconfig。这一次，没有救场情节——chapter 8 坑 2 用一次丢失换来的五步闭环，今天第一次作为**日常纪律**走一遍：menuconfig 里把 NAND 控制器驱动符号打开（`=y`，控制器是根文件系统的命根，不存在"用到再加载"）→ `bitbake -c savedefconfig linux-tiger` 收下 → diff 核对增量 → 回 linux-tiger 仓库提交 → bump SRCREV。流程全文见 8.11.2，这里只交差增量本身：

<!-- 【待验证·阻塞级】C-W17（续）：控制器驱动的实际 Kconfig 符号名以 qemu-tiger NAND 模型对应的内核驱动为准，下列为示意形态 -->

```text
# 文件路径：~/workspace/linux-tiger/arch/arm64/configs/tiger_defconfig（本章增量 diff，示意形态）
 # MTD / UBI / UBIFS：NAND 根文件系统的内核侧支持（chapter 9 伏笔）
 CONFIG_MTD=y
 CONFIG_MTD_UBI=y
 CONFIG_UBIFS_FS=y
+# NAND 控制器驱动（编进内核：rootfs 所在介质，不存在"用到再加载"）
+CONFIG_MTD_NAND=y
+CONFIG_MTD_NAND_<控制器>=y
```

"上章那句话今天兑现了。"阿凯提交完说，"闭环从踩坑教训变成日常纪律——这次我没丢任何东西。"

#### 9.6.4 内核命令行形态：切 root 留给明天

最后把明天要用的内核命令行形态认个脸熟。等启动链串通、真正切 root 那天，内核命令行上要出现这么一组：

```text
# 明天（chapter 10）的内核命令行形态预告，今天只读不写
console=ttyAMA0,115200 ubi.mtd=nand0 root=ubi0:rootfs_a rootfstype=ubifs
```

三个新参数：`ubi.mtd=nand0` 让内核启动时自动对 nand0 分区做 UBI attach（等价于手工 ubiattach）；`root=ubi0:rootfs_a` 按"设备号:卷名"点名根文件系统所在的卷——卷名第三次对上 boot.cmd 的契约；`rootfstype=ubifs` 告诉内核按 UBIFS 挂载。

今天这组参数**不上场**——切 root 是启动链串通之后的事，预支了 chapter 10 的高潮就没了。今天的验证用另一条路上去。

### 9.7 验证：initramfs 脚手架与 mount -t ubifs

#### 9.7.1 怎么先上去看一眼：initramfs 登场

回到早上那个被老周挡回来的问题：root 还没切过去，怎么先上去看一眼？阿凯翻回 8.10 的笔记——那天 kernel panic 之后，笔记里落下过两条边界说明，头一条说的是 initramfs："可以桥接'内核起来'到'真根就位'之间的路"，本章不给它上场，chapter 9 见。

想起来了。initramfs——chapter 8 边界说明里立过名字的初始内存文件系统——今天让它上场。定位先说死：**它是 bring-up 脚手架，不是量产路径**——tiger 的根文件系统主线在 NAND/UBI（chapter 8 就划过的口径），initramfs 只出场这一次：把内核用 QEMU 的 `-initrd` 顶进内存里的临时根，进去手动 attach、手动 mount，亲眼确认 NAND 上的 rootfs 挂得上，然后收工。

机制只点一句：Yocto 侧有一族 `INITRAMFS_IMAGE` 变量（kernel.bbclass:35 起，默认空）能把 initramfs **捆绑进内核镜像**（INITRAMFS_IMAGE_BUNDLE=1 的形态）——那条路今天不走。我们用的是独立 cpio 产物：不捆进内核镜像，启动时由 QEMU 的 `-initrd` 单独递进去，运行起来就是个临时根——**产物形态和运行期**，它和内核分得开，脚手架用完即弃。但"分得开"只到这一层为止：构建链上两边并不分家，下一小节搭脚手架时会亲眼看见，bitbake 的任务图照样把内核拉进来。

#### 9.7.2 构建脚手架，拼 NAND 测试镜像

Poky 自带一份现成的脚手架配方：`core-image-minimal-initramfs`——裁剪到只剩早期初始化脚本的镜像配方，产物形态就是 cpio。先在对照组环境确认形态（tiger 组的构建本节末再交代）：

```bash
# 构建 initramfs 脚手架（对照组环境；约 10-30 分钟量级，多数组件已有 sstate）
cd ~/workspace/poky
source oe-init-build-env ../build-ubi-demo
bitbake core-image-minimal-initramfs
```

产物（本机实测，关键行）：

```text
# 实体带时间戳，裸名是符号链接；注意文件名里没有 .rootfs 段——
# 该配方 IMAGE_NAME_SUFFIX ?= ""，和 9.5.1 三件套形成对照
tmp/deploy/images/qemuarm64/core-image-minimal-initramfs-qemuarm64-<时间戳>.cpio.gz
tmp/deploy/images/qemuarm64/core-image-minimal-initramfs-qemuarm64.cpio.gz -> core-image-minimal-initramfs-qemuarm64-<时间戳>.cpio.gz
```

> **💡 提示**：对照组的 deploy 目录里，脚手架还顺手产出了自己的 `.ubifs` / `.ubi` / `ubinize-*.cfg`——演示环境的 IMAGE_FSTYPES 含 `ubi`，这个名单对**所有**镜像配方生效，脚手架也不能幸免。这些 UBI 产物对本章没有用，看一眼知道来路就行。

"脚手架里有没有 `ubiattach`，解开看包清单就知道。"老周说。阿凯翻开 manifest——base-passwd、eudev、kmod 一应在列（脚手架的设备管理实现是 eudev：配方清单里写的 `udev`，poky 默认解析到 eudev 配方），但 busybox 一族只有三个包：`busybox`、`busybox-syslog`、`busybox-udhcpc`，没有任何 ubi 打头的。再查 busybox 1.36.1 的 .config：`# CONFIG_UBIATTACH is not set`、`# CONFIG_UBIMKVOL is not set`——**busybox 没带 ubi 系 applet**，脚手架里不会有 ubiattach，要追加 `mtd-utils-ubifs`（mtd-utils 的 target 形态子包，9.2.2 点过名）。

`ubiattach` 与 `ubimkvol` 就地介绍一句：UBI 卷的目标侧管理命令，attach 是把 MTD 分区挂进 UBI 子系统，mkvol 是在 UBI 设备上建新卷，都来自 mtd-utils。

追加这一下，本身又是个小坑。阿凯的第一稿写的是熟手句式——给指定配方追加 IMAGE_INSTALL：

```bitbake
# 演示环境 conf/local.conf 第一稿（无效写法，请勿模仿）
IMAGE_INSTALL:append:pn-core-image-minimal-initramfs = " mtd-utils-ubifs"
```

重建，解开新 cpio——`ubiattach` 不在场。变量终值里明明有它，为什么没进镜像？翻配方找答案：

```bitbake
# 文件路径：~/workspace/poky/meta/recipes-core/images/core-image-minimal-initramfs.bb（关键行节选）
INITRAMFS_SCRIPTS ?= "\
                      initramfs-framework-base \
                      initramfs-module-setup-live \
                      initramfs-module-udev \
                      initramfs-module-install \
                      initramfs-module-install-efi \
                     "
PACKAGE_INSTALL = "${INITRAMFS_SCRIPTS} ${VIRTUAL-RUNTIME_base-utils} udev base-passwd ${ROOTFS_BOOTSTRAP_INSTALL}"
```

原因在这行 `PACKAGE_INSTALL`——就地解释：**镜像配方实际吃进 rootfs 的包清单变量**。它通常由 image 类用 `?=` 从 IMAGE_INSTALL 推导出来（image.bbclass:86；core-image 类继承 image 类获得这条默认），所以平时往 IMAGE_INSTALL 里 append 就够了；但这个配方**用 `=` 硬写了 PACKAGE_INSTALL**——`?=` 的弱默认值一遇硬写即作废，推导链被绕过，IMAGE_INSTALL 上的 append 整个落空。**装包入口不止 IMAGE_INSTALL 一个——加包不生效时先读配方，看它吃哪个变量。** 改对：

```bitbake
# 演示环境 conf/local.conf 定稿（有效写法）
PACKAGE_INSTALL:append:pn-core-image-minimal-initramfs = " mtd-utils-ubifs"
```

再重建，新 cpio 里 `usr/sbin/ubiattach -> /usr/sbin/ubiattach.mtd-utils` 在场，脚手架备齐。

接着在主机侧拼本章的测试用 NAND 镜像——注意边界：今天**只铺 nand0 一段**，bootloader 和 env 两段留空，全量布局是 chapter 10 总装的活：

```bash
# 当前目录：演示构建目录（cd ~/workspace/poky/build-ubi-demo）
# 生成 512 MiB 空白 NAND 镜像（NAND 擦除态是全 0xFF，不是全 0——先造 0 再逐字节翻成 0xFF）
dd if=/dev/zero bs=1M count=512 | tr '\000' '\377' > nand-test.img

# 把 ubinize 产物写到 nand0 分区起点（偏移 5 MiB = dd 的 seek 5 × 1 MiB；
# 文件名用 9.5.1 见过的裸名符号链接，带 .rootfs 段；.ubi 文件本身就是
# PEB 序列，可直接落盘；conv=notrunc 保住镜像其余部分）
dd if=tmp/deploy/images/<machine>/core-image-minimal-<machine>.rootfs.ubi \
   of=nand-test.img bs=1M seek=5 conv=notrunc
```

<!-- 【待验证·阻塞级】C-W19（阻塞级，优先核实）：qemu-tiger 的 NAND 后端接法（QEMU 命令行参数形态）、空白镜像 0xFF 擦除态假设、镜像大小与分区表的对账方式均待 qemu-tiger 仓库核实；**若其 NAND 模型几何与 D5 拍板值（页 2 KiB / 块 128 KiB / 512 MiB）不符，9.3 全部数字与本节偏移须重算** -->

> **⚠️ 注意**：空白镜像为什么填 0xFF 不填 0？9.1.1 说过，NAND 擦除态是全 1（0xFF）——一个"从没被写过"的块，读到的是 0xFF。全 0 的块在 UBI 眼里是一堆坏元数据，attach 时会当坏块处理甚至报错。测试镜像要模拟的是"出厂擦好的 NAND"，所以先 dd 出全 0，再用 `tr` 把每个字节翻成 0xFF。

这里拼的是对照组的镜像。直通那天挂上去的，是 tiger 组自己的那一份——tiger 三件套的 .ubi（卷名 rootfs_a）同法铺进另一张空白镜像，步骤与这里逐字同构，只是输入文件换成 tiger-aarch64 目录下的产物；tiger 组的镜像拼装与直通挂盘要等 qemu-tiger 仓库就位后实测，本节方法以对照组为准。

tiger 组的脚手架呢？阿凯翻到配方里一行，觉得自己发现了捷径："这份配方自带 `PACKAGE_EXCLUDE = "kernel-image-*"`（第 20-21 行，注释原话 "Don't allow the initramfs to contain a kernel"）——**脚手架的内容物里本就不含内核包**。那它跟 linux-tiger 的占位 SRCREV 就没关系了吧？主构建目录直接能跑。"

"别急着下结论。"老周说，"内容物不含内核，和构建碰不碰内核，是两回事。任务图数一遍再说话——`bitbake -n` 只排任务不执行，全零 SRCREV 不碍事。"

阿凯在主构建目录干跑 `bitbake -n core-image-minimal-initramfs`，数完愣住了：任务清单里 linux-tiger 一家占了 **27 个**——do_fetch、do_unpack、do_patch、do_configure、do_compile、do_install、do_package、do_packagedata、do_package_write_rpm，整条内核编译链都在脚手架的构建链上。

内核怎么被拉进来的？image.bbclass 里有两条默认挂着的依赖边，注释里还自认可摘：

- 第 114 行 `KERNELDEPMODDEPEND ?= "virtual/kernel:do_packagedata"`，加进 do_rootfs 的依赖——注释里自认：构建小 initramfs 时你可能不想要这个依赖，它会造成依赖环；
- 第 150 行 `KERNEL_DEPLOY_DEPEND ?= "virtual/kernel:do_deploy"`，加进 do_build 的依赖——chapter 8 集成内核时登记过它，管的是内核 deploy 产物进 DEPLOY_DIR 的时序；注释明说可以手动置空它。

阿凯照方抓药，在 local.conf 里把两个变量都置空再数一遍——27 变成 **24**，消失的三个里包括 do_deploy。**摘不干净。** 数完他把那两行删掉，主构建目录不留手脚。剩下的 24 个从哪来，值得一个深挖：

> **📖 深入阅读**：这条边文档里没写，藏在递归任务闭包里——image.bbclass:121 的 `do_rootfs[recrdeptask] += "do_packagedata"` 与 rootfs_rpm.bbclass:30 的 `do_rootfs[recrdeptask] += "do_package_write_rpm do_package_qa"`，把 do_rootfs 递归闭包里所有配方的打包任务全牵进来。闭包里的 iptables 在 `RRECOMMENDS:${PN}`（推荐依赖——有就装上的弱声明）里列了一串 kernel-module-*（iptables_1.8.10.bb:97 起），内核配方凭 `PACKAGES_DYNAMIC += "^${KERNEL_PACKAGE_NAME}-module-.*"`（kernel.bbclass:209）充当这类包的 provider——iptables 的打包任务就这样钉在 linux-tiger 的 do_packagedata 上，整条编译链跟着进门。这就是置空两个变量仍剩 24 个的原因。

所以结论得改口：脚手架的**内容物**确实不含内核包（PACKAGE_EXCLUDE 兜着，这层是真的），但**任务图**上内核跑不掉——主构建目录这一跑同样要等 linux-tiger 仓库就位，占位 SRCREV 下 do_fetch 过不去。tiger 组的脚手架留到仓库就位后补测：同一句 `PACKAGE_INSTALL:append:pn-core-image-minimal-initramfs = " mtd-utils-ubifs"` 落到 build 的 local.conf，预期产物 `core-image-minimal-initramfs-tiger-aarch64.cpio.gz`，产物清单与 ubiattach 在场核对一并补上。

至于平时为什么无感：按本书的构建顺序，chapter 8 早已把 linux-tiger 构建过一遍，这些任务真跑时基本命中 sstate、无需重编——**"无感"是 sstate 兜底，不是任务图上没有。**

<!-- 【待验证·阻塞级】C-W18（续）：tiger 组脚手架（主构建目录，MACHINE=tiger-aarch64）的 cpio 产物清单与 ubiattach 在场核对待 linux-tiger 仓库就位后回填；27/24 两个任务数为编排方 build-verify 环境（tiger-aarch64）2026-08-25 实测终局（bitbake -n；置空 KERNELDEPMODDEPEND / KERNEL_DEPLOY_DEPEND 后 27→24，消失的 3 个含 do_deploy），正文已按此终局改写；直通环节仍归 C-W19/C-W20 -->

#### 9.7.3 挂得上：串口日志逐行指认

高潮时刻。QEMU 直通启动：`-kernel` 递 Image、`-dtb` 递 tiger.dtb、`-initrd` 递脚手架 cpio、NAND 镜像挂上 qemu-tiger 的后端：

<!-- 【待验证·阻塞级】C-W19/C-W20：下列 QEMU 命令的 NAND 参数形态与全部串口输出为按常规形态书写的示意，严禁直接采用；以 qemu-tiger 仓库实测为准 -->

```bash
# 直通验证：内核 + dtb + initramfs 脚手架 + 测试 NAND 镜像
$BUILDDIR/tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64 \
    -machine tiger \
    -kernel $BUILDDIR/tmp/deploy/images/tiger-aarch64/Image \
    -dtb $BUILDDIR/tmp/deploy/images/tiger-aarch64/tiger.dtb \
    -initrd $BUILDDIR/tmp/deploy/images/tiger-aarch64/core-image-minimal-initramfs-tiger-aarch64.cpio.gz \
    -append "console=ttyAMA0,115200" \
    <NAND 镜像接法参数，以 qemu-tiger 后端为准> \
    -nographic
```

注意 `-append` 里**没有** `root=`——9.6.4 那组切 root 参数今天不用，内核起来后直接把 initramfs 当根。内核日志跑到尾段，不再是 panic，而是脚手架的 shell 提示符。然后手动三步：

```bash
# initramfs shell 内：看 MTD 分区表是否被内核认出
cat /proc/mtd
# 把 nand0 挂进 UBI 子系统（生成 /dev/ubi0）
ubiattach /dev/ubi_ctrl -m 2
# 挂载卷 0（ubinize.cfg 里的 vol_id=0）到 /mnt
mount -t ubifs /dev/ubi0_0 /mnt
# 看 rootfs 里的东西
ls /mnt /mnt/boot
```

串口输出（示意形态，待仓库实测——本章的成功画面预告）：

```text
# cat /proc/mtd
dev:    size   erasesize  name
mtd0: 00400000 00020000 "bootloader"
mtd1: 00100000 00020000 "env"
mtd2: 1fb00000 00020000 "nand0"

# ubiattach /dev/ubi_ctrl -m 2
UBI: attaching mtd2 to ubi0
UBI: scanning is finished
UBI: attached mtd2 (name "nand0", size 507 MiB) to ubi0
UBI: available PEBs: 3974, total PEBs: 4056

# mount -t ubifs /dev/ubi0_0 /mnt
UBIFS: mounted UBI device 0, volume 0, name "rootfs_a"

# ls /mnt/boot
Image  tiger.dtb
```

<!-- 【待验证·阻塞级】C-W20（续）：上述全部输出为按内核常规日志形态书写的示意，行序、数字（available PEBs 按 9.3.2 算术为 3974）、提示文本以实测回填为准 -->

逐行指认，按出场顺序：

- `/proc/mtd` 三行——**dts 分区表被内核认出的直接证据**。三个 label、三段大小、erasesize 全是 128 KiB（0x20000），和 9.4.3 画的表逐字节对得上；`nand0` 是 mtd2，所以 ubiattach 后面跟的是 `-m 2`。
- `UBI: attached mtd2`——attach 成功；`available PEBs` 那行是 9.3.2 算术的现场验证：总 4056 块，扣掉卷表 2 块，再扣按整片 4096 块预留的约 80 块坏块替换，可用的正是 3974——`-c 3968` 的上限拍得恰到好处。
- `UBIFS: mounted UBI device 0, volume 0, name "rootfs_a"`——**本章的一行千金**。卷名 rootfs_a，从 machine conf 的 UBI_VOLNAME，到 ubinize.cfg 的 vol_name，到 boot.cmd 的 `${bootvol}`，三处契约在这行日志里会师。
- `ls /mnt/boot` 列出 `Image` 和 `tiger.dtb`——chapter 7 纸面契约里的两个文件名、chapter 8 构建产出的两个实物，此刻躺在 NAND 上的 UBIFS 卷里。boot.cmd 的两行 `ubifsload`，对象全部在场。顺带一眼 `/mnt/lib/modules`——chapter 8 的模块包也躺在里面，8.7 挂账"运行效果是 chapter 9 之后的事（rootfs 不在场，演了没人看）"，今天观众第一次到场，虽然只是肉眼。

"挂上了。"阿凯盯着那行 `mounted UBI device 0`，"上周五 panic 喊'给我一块地'——今天地有了。"

"看一眼就够了，收工。"老周说，"`umount /mnt`，关 QEMU。login 是明天的事——别贪杯，把 chapter 10 的高潮提前喝了。"

阿凯 `umount` 收工，在今天的日志末尾写："NAND 上有了第一块能挂的根。boot.cmd 纸面上的每一行，从今天起都有实物对应。"

顺手把账记清：chapter 8 预告过 boot.cmd 里 `ubi part`、`ubifsmount`、`ubifsload`"终于要真跑了"——今天验证走的是 initramfs 直通，U-Boot 没有上场，这三族命令的真跑留给明天启动链串通；今天先让它们要读的卷和文件全部备齐、亲眼验证在场。

### 9.8 踩坑实录

按惯例交代时间线：这两个坑发生在 9.3 到 9.6 之间——前面看到的正确参数和顺利挂载，都是改回来之后的样子。

#### 9.8.1 坑 1：-e 和 -p 拿错块

9.3.2 老周那句"你 -e 用的是哪个块"不是白问的。阿凯的第一稿参数，把 128 KiB 同时塞给了两个变量：

```bitbake
# 错误示范：~/workspace/meta-tiger/conf/machine/tiger-aarch64.conf 第九段的错误形态
# ——-e 填了 PEB 尺寸（mkfs.ubifs 要的是 LEB），请勿模仿
MKUBIFS_ARGS ?= "-m 2048 -e 131072 -c 3968"
UBINIZE_ARGS ?= "-m 2048 -p 131072 -s 2048 -O 2048"
```

126976 和 131072 只差 4096——两个页。阿凯当时的想法是"UBIFS 不是也要占整个块吗，128 KiB 没错"，忘了 9.1.3 图上 UBI 先吃掉的那两个元数据页。老周问了那句之后，他翻回 mkfs.ubifs 的手册页才反应过来：`-e` 的全称是 LEB size，**mkfs.ubifs 的世界里根本没有 PEB**，它的天花板是 UBI 扣完元数据剩下的 124 KiB。

那这个错如果当时没人问，会走多远？这正是这个坑最阴的地方——**参数错，但工具不拦**。阿凯后来在主机侧用 mtd-utils-native 的工具亲手试了错误组合（本机实测）：

```bash
# 主机侧演示：在演示构建目录下执行（cd ~/workspace/poky/build-ubi-demo）
# 工具直接用 mtd-utils-native 的 sysroot 副本
# mkfs.ubifs 是动态链接的，裸跑要先用 LD_LIBRARY_PATH 指上 lzo-native 的库
# （否则报 liblzo2.so.2 找不到；ubinize 不链 lzo，不需要这步）
export LD_LIBRARY_PATH=$PWD/tmp/sysroots-components/x86_64/lzo-native/usr/lib

# (a) -e 填 PEB 值 131072（正确应为 LEB 值 126976）
tmp/sysroots-components/x86_64/mtd-utils-native/usr/sbin/mkfs.ubifs \
    -r <测试目录> -m 2048 -e 131072 -c 3968 -o bad-e.ubifs
echo $?

# 对照：正确参数的产物
tmp/sysroots-components/x86_64/mtd-utils-native/usr/sbin/mkfs.ubifs \
    -r <测试目录> -m 2048 -e 126976 -c 3968 -o ok.ubifs
ls -l bad-e.ubifs ok.ubifs
```

输出（本机实测）：

```text
0
-rw-r--r-- 1 <your-username> <your-username> 1835008 ... bad-e.ubifs
-rw-r--r-- 1 <your-username> <your-username> 1777664 ... ok.ubifs
```

静默放行，退出码 0——畸形镜像当场无任何异样，甚至**比正确参数的产物还大**（每块多算了 4 KiB 的可用空间，索引布局跟着膨胀）。ubinize 一侧也一样：`-p` 误填 LEB 值 126976，放行，只有一条和几何无关的例行提示（volume size 未指定，按能装下镜像的最小值处理）；`-m` 填 4096（和真实页 2048 不符）同样放行——主机上的工具根本无从得知目标 NAND 的真实页大小。

工具也不是永远不拦。阿凯补了一组对照：`-e 2048`——这次 mkfs.ubifs 当场喊停：

```text
Error: too small LEB size 2048, minimum is 15360
```

退出码 255。看明白没有——**工具会拦，但拦的方向和你犯错的方向正交**：它只在 LEB 小到装不下自己元数据时才喊停，喊的话里没有一个字提"块认错"。你真正容易犯的错——把 PEB 当 LEB、把 LEB 当 PEB——恰好全在它的射程之外。

那病什么时候发作？要到 attach 和 mount——UBI 按真实 LEB 126976 把块喂上来，镜像内部却按 131072 排版，两边对不上，挂载时才爆（本章直通挂的是改对参数后的镜像，发作形态留待实测补全）。共同点是：**发作的位置离犯错的思维十万八千里**。错在"把 PEB 当成了 LEB"，表现却是"空间对不上、挂载报错"——没有一行输出会替你喊出"你块认错了"。

"三个参数围着两个块转，块认错一个，全盘皆输。"老周把 9.3 那句话又念了一遍，"这种坑跟'什么都不发生'不是一个族——那个系列是 BitBake 视野之外它不替你知道；这一族是**工具视野之内它照样放行**，因为你的参数语法上完全合法，只是语义上认错了块。防法只有一个：推导时把每个参数属于哪个世界——mkfs.ubifs 的 LEB 世界还是 ubinize 的 PEB 世界——写在旁边，像你今天白板上那样。"

修正就是把 9.3.2 的推导做扎实：`-e 126976`（LEB），`-p 131072`（PEB），两个数字差两个页，差之毫厘谬以千里。

#### 9.8.2 坑 2：分区边界没按擦除块对齐

第二个坑在 9.6.2 写 dts 分区表时。阿凯的第一稿给 bootloader 段拍长度时，心里估的是"FIP + u-boot.bin 加余量，4 MiB 出头"——他写了 0x410000（4 MiB + 64 KiB）。env 段的起点就跟着落在 0x410000：

```text
// 错误示范：0x410000 不是 128 KiB（0x20000）的整数倍——多出来的 64 KiB 是半个块，请勿模仿
bootloader@0 {
	label = "bootloader";
	reg = <0x0000000 0x0410000>;	// 4 MiB + 64 KiB
};
env@410000 {
	label = "env";
	reg = <0x0410000 0x0100000>;	// 起点落在第 32 块的正中间
};
nand0@510000 {
	label = "nand0";
	reg = <0x0510000 0x1FAF0000>;	// 起点跟着歪到 0x510000
};
```

0x410000 写成十进制是 4259840，除以 131072 得 32.5——**半个块**。env 段从第 32 块的正中间开始，nand0 的起点 0x510000 跟着歪在同一个半块位置上。阿凯当时检查过吗？检查过——他用的是十进制心算："4 MiB 是 32 块，多出来的 64 KiB……64 是 128 的一半，四舍五入算一块吧。"把半个块四舍五入没了。

毛病在下午的直通验证时现身——内核认出分区表后，日志里躺着两行：

<!-- 【待验证·阻塞级】C-W21：分区未对齐时内核 partitions 解析警告的真实日志形态待仓库实测回填（警告文本、是否强制只读、UBI attach 时是否另有报错），下列为按内核常规形态的示意 -->

```text
# 内核启动日志（示意形态，以实测为准）
mtd: partition "env" doesn't start on an erase/write block boundary -- force read-only
mtd: partition "nand0" doesn't start on an erase/write block boundary -- force read-only
```

阿凯盯着这两行，第一反应是"内核算错了吧"——env 的起点不就是 bootloader 段的结尾吗，bootloader 段给了 4 MiB 还多，怎么会不在块边界上？老周让他把数字全写成十六进制，拿 0x20000 挨个过：0x410000 ÷ 0x20000——**除不尽**，32 余 0x10000，尾巴上那半个块现了原形。十进制里"4 MiB 出头"听起来很整齐的数，十六进制下无处遁形。

"教训两条。"老周说，"第一条是算术纪律：**分区表的每个偏移和长度，都用十六进制按擦除块大小过一遍**——128 KiB = 0x20000，整不整，换成块数看：除以 0x20000 得整数才算过。"

"十六进制下也有快检法：末四位是零、且 0x10000 位是偶数——0x410000 末尾正好四个零，可 0x10000 位上是 1，当场出局。十进制的 4259840 除以 131072，谁也口算不出 32.5。"

"第二条：估尺寸先按块拍整数——'4 MiB 出头'这种话，该翻译成'32 块不够就给 40 块'，而不是'4 MiB 再加 64 KiB'。"

修正就是 9.4.3 那张表：bootloader 4 MiB（0x400000，整 32 块）、env 1 MiB（0x400000 起，整 8 块）、nand0 从 0x500000 起——每个数都是 0x20000 的整倍数，十六进制一眼过检。

> **💡 提示**：内核解析 fixed-partitions 时对未对齐分区只警告并强制只读，不拦启动——警告在启动早期的 partitions 解析期就打印，真正的失败留到 UBI attach 那一刻才发作。**边界对齐是分区表作者的责任，工具不替你兜底。**

### 9.9 本章小结

一天走完，白板上"存储"格涂实了。早上拆的七步，逐个回顾：

- **9.1**：NAND 四条脾气（页写、块擦、不能就地改写、有坏块有寿命）→ 不能直接上 ext4；四层栈：MTD 抽象、UBI 收坏块管理与磨损均衡、UBIFS 管文件系统；PEB/LEB 大图——LEB = PEB − 2 页元数据，本章所有算术的地基。
- **9.2**：chapter 5 的注释兑现，`IMAGE_FSTYPES += "ubi"`（终值的前导空格是 `+=` 拼接痕迹）；image_types.bbclass 机制——`IMAGE_TYPEDEP:ubi = "${UBI_IMGTYPE}"` 让 `ubi` 自动带出 `ubifs` 产物（所以只写一个词）；两条 IMAGE_CMD 对应 mtd-utils 的 mkfs.ubifs/ubinize（本机 2.1.6）；do_image_ubi 一族的 depends 链拉 mtd-utils-native，报错现场的 sysroot 日志实证兑现；参数未设时是**两道防线**——全不设，mkfs.ubifs 自己的用法检查先在 do_image_ubifs 拦下；设一半，multiubi_mkfs 开头的 bbfatal 在 do_image_ubi 拦下，都拦在正道上。
- **9.3**：NAND 几何（页 2 KiB / 块 128 KiB / 512 MiB）推出七个参数——MKUBIFS_ARGS `-m 2048 -e 126976 -c 3968`（LEB 世界：-e 是块减两页），UBINIZE_ARGS `-m 2048 -p 131072 -s 2048 -O 2048`（PEB 世界：-p 是原样块）；落 machine conf 第九段，UBI_VOLNAME 覆盖为 rootfs_a 对上 boot.cmd 契约；老周没直接给答案，只问"你 -e 用的是哪个块"。
- **9.4**：A/B 分区正式落成字节；纸面九段表 vs 已交付 boot.cmd 的裁决——kernel/dtb 住 UBIFS 卷内（UBIFS 的日志和校验正是 OTA 要的安全性）；MTD 三段（bootloader 4 MiB / env 1 MiB / nand0 507 MiB）+ UBI 三卷（rootfs_a / rootfs_b / data，后两卷 chapter 10 建）；nand0 的 label 对上 `ubi part nand0`。
- **9.5**：构建出 deploy 三件套（.ubifs / .ubi / ubinize-*.cfg——实体带时间戳、文件名含 .rootfs 段、cfg 无裸名链接）；ubinize.cfg 是 write_ubi_config 自动生成（image_types.bbclass:175-187），逐行读懂——vol_id=0、vol_type=dynamic、vol_name（对照组落在默认 qemuarm64-rootfs，tiger 组落 rootfs_a）、vol_flags=autoresize；autoresize 与 A/B 多卷布局冲突，全布局 cfg 留 chapter 10 自写；multiubi 点名即弃。
- **9.6**：chapter 8 defconfig 三行伏笔回收（注释行"chapter 9 伏笔"到期；平台级清单一次备齐的道理）；dts 补全 NAND 控制器节点 + fixed-partitions 三段分区表；defconfig 增量走五步闭环——从踩坑教训变成日常纪律的第一回；`ubi.mtd=` / `root=ubi0:rootfs_a` / `rootfstype=ubifs` 命令行形态认脸熟，切 root 留明天；mtdparts 只作对照弃用。
- **9.7**：initramfs 登场——chapter 8 边界说明里立过名字，今天实质使用，bring-up 脚手架，不进量产路径；core-image-minimal-initramfs 产 cpio（无 .rootfs 段，IMAGE_NAME_SUFFIX 置空）；busybox 没带 ubi 系 applet（manifest 里 busybox 一族只有三个包），追加 mtd-utils-ubifs 时发现配方硬写 PACKAGE_INSTALL 绕开了 IMAGE_INSTALL——加包不生效先读配方；内容物不含内核 ≠ 构建不碰内核——脚手架的任务图拉起 27 个 linux-tiger 任务，置空两条文档自认可摘的依赖边仍剩 24 个（平时无感是 sstate 兜底）；主机侧 dd 拼测试 NAND 镜像（空白镜像填 0xFF 不填 0）；QEMU 直通挂 NAND，initramfs shell 里 ubiattach + mount -t ubifs 挂上 rootfs_a——/boot/Image、/boot/tiger.dtb 实物在场，三处契约一行日志里会师；umount 收工，login 留给明天。
- **9.8**：两个坑都是算术——-e/-p 拿错块（LEB/PEB 认错，工具静默放行；对照组证明工具会拦，但拦的方向和你犯错的方向正交——与"什么都不发生"不同族）；分区边界没按擦除块对齐（十六进制换成块数过检纪律 + 估尺寸按块拍整数）。

本章产出清单：

- `meta-tiger/conf/machine/tiger-aarch64.conf`：第三段修订（IMAGE_FSTYPES 加 `ubi`，注释兑现）+ 新增第九段（MKUBIFS_ARGS / UBINIZE_ARGS / UBI_VOLNAME），全文九段。
- `linux-tiger`（固件组仓库）：tiger.dts 补全 NAND 节点与分区表、tiger_defconfig 补控制器驱动——提交在开发态仓库，不打 tag（沿用惯例）。
- deploy 目录新增三件套：`.ubifs` / `.ubi` / `ubinize-*.cfg`（对照组已验机制，tiger 组待仓库就位后核对）；initramfs 脚手架 cpio 一份（已追加 mtd-utils-ubifs）。
- NAND 上第一块可挂的根：rootfs_a 卷，initramfs 脚手架内 mount 验证通过。

提交并打 tag。本章唯一被修改的我方集成仓库仍是 `meta-tiger`：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Add UBI image support: IMAGE_FSTYPES, MKUBIFS_ARGS/UBINIZE_ARGS, UBI_VOLNAME"
git tag chapter9
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter9`；`linux-tiger` 是固件组的开发态仓库，本章有提交但不打 tag（沿用 qemu-tiger / tf-a-tiger / u-boot-tiger 的惯例）；`poky` 与 `meta-arm` 保持原样不打。

后续任务清单：

- **task 11 / chapter 10**：打通启动链——全量 NAND 镜像总装（各分区偏移总表、全布局 ubinize.cfg 自写、rootfs_b 与 data 卷建立）、`ubi part` / `ubifsload` 在 U-Boot 里真跑、TFA_UBOOT=1 旋钮、PSCI 多核唤醒、把 panic 换成 login——第一次开机成功，全书最高潮。
- 留到 phase 5 的伏笔：rootfs 内容实用化（chapter 12）、模块 system 级 autoload（今天只是第一次肉眼看见 .ko 躺在 NAND 上）。

老周下班前走到白板跟前，把"存储"那格涂实，在下面添了一行：地有了，根挂上了——明天交房，搞不定正常。

"搞不定正常？"阿凯抬头。

"明天你就知道了。"老周拿起外套，"我赌你明天搞不定——串通启动链这事，没人第一回就能成。赌约立在这儿，后天咱们对账。"

---

**延伸阅读**

1. linux-mtd 项目的 UBI/UBIFS FAQ（MKUBIFS_ARGS/UBINIZE_ARGS 每个参数的权威解释，即 bbfatal 报错里附的那份文档）：http://www.linux-mtd.infradead.org/faq/ubifs.html
2. Yocto 变量术语表，IMAGE_FSTYPES / MKUBIFS_ARGS / UBINIZE_ARGS / UBI_VOLNAME / INITRAMFS_IMAGE / PACKAGE_INSTALL 条目：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
3. OpenEmbedded-Core 的 image_types.bbclass（本章引用的全部机制源码，本机路径 `poky/meta/classes-recipe/`）：https://git.openembedded.org/openembedded-core/tree/meta/classes-recipe?h=scarthgap
4. mtd-utils 项目（mkfs.ubifs / ubinize / ubiattach 的源码出处）：https://git.infradead.org/mtd-utils.git
5. Linux Kernel 6.6 文档，MTD/UBI/UBIFS 子系统与 fixed-partitions 绑定：https://docs.kernel.org/6.6/
