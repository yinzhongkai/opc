## 14 交付 SDK：从生产到使用

周一上午，工位。晨会刚散，应用组的排期表还贴在达哥屏幕边上——"tiger 应用开发"那一栏后面，写着两个鲜红的问号。

"应用组又来问了：他们什么时候能开始写 tiger 上的应用。"达哥在阿凯对面坐下，"总不能让他们一人装一套 Yocto、一人跑一遍 bitbake。"

"那给他们什么？"

"你想想以前芯片厂怎么发的。"达哥往后一靠，"一张光盘，一份 PDF，光盘里一个装完就能用的交叉编译环境。那就是 SDK。现在轮到我们发这一天了——**你产出 SDK 给应用组用，自己先要会用，不然有问题你都不知道。**今天你扮演两个角色：先做 BSP 工程师把 SDK 发出去，再扮演应用工程师把它跑通。"

他在白板上列了三件事：**发 SDK、发 eSDK、两个都自己验一遍**。

"我补一件。"阿凯举手——给自己加任务已经成了惯例，"发出去的东西，得知道里面装了什么、是哪一版。SDK 的版本和留痕，谁管？"

"问得好，这件归你立项。"达哥点头，"开工。`populate_sdk` 长什么样——自己看，别问我。"

### 14.1 SDK vs eSDK：什么场景用哪个

#### 14.1.1 两半结构：主机侧工具链 + 目标侧系统根

先认一笔拖了十三章的旧账。chapter 1 探索 build 目录时（1.5.2），`tmp/` 下躺着两个当时没展开的目录：

```bash
# 认账：chapter 1 见过没认的两个 sysroots 目录
ls -d ~/workspace/build/tmp/sysroots-*

# 顺藤摸瓜：生成 SDK 的类文件在哪
ls ~/workspace/poky/meta/classes-recipe/populate_sdk*.bbclass
```

输出：

```text
/home/<your-username>/workspace/build/tmp/sysroots-components
/home/<your-username>/workspace/build/tmp/sysroots-uninative
/home/<your-username>/workspace/poky/meta/classes-recipe/populate_sdk.bbclass
/home/<your-username>/workspace/poky/meta/classes-recipe/populate_sdk_base.bbclass
/home/<your-username>/workspace/poky/meta/classes-recipe/populate_sdk_ext.bbclass
```

三个类文件里，`populate_sdk.bbclass` 是个薄壳——继承 base 并把 `do_populate_sdk` 任务挂进镜像配方；主体逻辑在 `populate_sdk_base.bbclass`，14.2.1 直接翻它。

**系统根（Sysroot）** 本章正式引入：交叉编译环境里为目标硬件提供头文件和库的目录结构——编译 tiger 上的程序，编译器在 x86_64 主机上跑，但它引用的头文件和链接的库必须是 aarch64 的，这套"目标世界的素材"就是 sysroot。`tmp/sysroots-components/` 就是构建系统按配方逐个组件组装 sysroot 的现场（`sysroots-uninative/` 是 uninative 工具的，认得即可）。

达哥在白板上画了 SDK 的结构——**一个 SDK 是两半拼起来的**：

```text
┌──────────────── SDK（装在一台 x86_64 开发机上）────────────────┐
│                                                                │
│  主机侧（在开发机上运行）                                        │
│    cross 工具链：aarch64-tiger-linux-gcc 一族                   │
│      —— x86_64 上执行，产出 aarch64 二进制                       │
│    nativesdk- 工具：qemu、cmake 一族（命名前缀下文解释）          │
│                                                                │
│  目标侧 sysroot（只当素材，不在开发机上运行）                     │
│    sysroots/cortexa53-tiger-linux/                             │
│      ├─ usr/include/   头文件（glibc、libstdc++、……）           │
│      └─ usr/lib/       库（aarch64 的 .so / .a）                │
│                                                                │
└────────────────────────────────────────────────────────────────┘

eSDK = 上图的全部 + 一套封装好的 bitbake/devtool + 一份 sstate 快照
```

两半各自回答一个问题：主机侧回答"用什么编"，目标侧 sysroot 回答"对着谁编"。应用工程师 `source` 一个环境脚本，编译器、头文件、库三者就指向同一个小宇宙——这个脚本 14.2.3 装出来看实物。

#### 14.1.2 场景分野：SDK 给写应用的人，eSDK 给改系统的人

**eSDK（Extensible SDK，可扩展 SDK）** 本章正式引入：在 SDK 的基础上，再打包一套可离线工作的 bitbake 构建系统和 devtool 开发工具，外加一份锁定签名的 sstate 快照——拿到 eSDK 的人不用克隆 poky，也能往镜像里加组件、改配方。

两个都要发的理由，按接收方分工一句话说清：

| 维度 | SDK | eSDK |
|------|-----|------|
| 接收方 | 应用组——只写应用，编完部署上板 | 应用组/系统组——还要往镜像里加组件、改配方 |
| 内含 | 交叉工具链 + 目标 sysroot | SDK 全部 + bitbake/devtool + sstate 快照 |
| 体积 | 百 MB 级 | GB 级（full 型，14.4 细说） |
| 能干的事 | 编译、链接、调试应用 | 编译应用 + `devtool add/modify` 改系统 |

tiger 项目两个都需要：应用组写业务程序用 SDK；他们哪天发现"镜像里缺个库"，走 eSDK 自己加——不用回来排队等 BSP 组。

### 14.2 生成 SDK：populate_sdk 的细节

#### 14.2.1 机制实物：populate_sdk_base.bbclass 的两张清单

**生成 SDK（populate_sdk）** 本章正式引入：镜像配方的一个任务，把"交叉工具链 + sysroot"打成可分发的安装包。task13 末尾埋的那句伏笔——"populate_sdk 的输入镜像是 tiger-image，SDK 与镜像同 DISTRO 策略"——今天兑现：SDK 不是独立构建的东西，它长在 tiger-image 这棵树上。

阿凯翻开 `populate_sdk_base.bbclass`，先找"SDK 里装什么"的答案——两张清单，各管一半：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/populate_sdk_base.bbclass（节选，64-78/99/106 行）
TOOLCHAIN_HOST_TASK ?= " \
    nativesdk-packagegroup-sdk-host \
    packagegroup-cross-canadian-${MACHINE} \
    ${@bb.utils.contains('SDK_TOOLCHAIN_LANGS', 'go', 'packagegroup-go-cross-canadian-${MACHINE}', '', d)} \
    ${@bb.utils.contains('SDK_TOOLCHAIN_LANGS', 'rust', 'packagegroup-rust-cross-canadian-${MACHINE}', '', d)} \
"
TOOLCHAIN_HOST_TASK_ATTEMPTONLY ?= ""
TOOLCHAIN_TARGET_TASK ?= " \
    ${@multilib_pkg_extend(d, 'packagegroup-core-standalone-sdk-target')} \
    ${@bb.utils.contains('SDK_TOOLCHAIN_LANGS', 'go', multilib_pkg_extend(d, 'packagegroup-go-sdk-target'), '', d)} \
    ${@bb.utils.contains('SDK_TOOLCHAIN_LANGS', 'rust', multilib_pkg_extend(d, 'libstd-rs'), '', d)} \
    target-sdk-provides-dummy \
"
TOOLCHAIN_TARGET_TASK_ATTEMPTONLY ?= ""
TOOLCHAIN_OUTPUTNAME ?= "${SDK_NAME}-toolchain-${SDK_VERSION}"
# ...
SDK_RDEPENDS = "${TOOLCHAIN_TARGET_TASK} ${TOOLCHAIN_HOST_TASK}"
# ...
REAL_MULTIMACH_TARGET_SYS = "${TUNE_PKGARCH}${TARGET_VENDOR}-${TARGET_OS}"
```

逐行认。第 64-69 行是主机侧清单：默认两项——`nativesdk-packagegroup-sdk-host`（主机侧工具包组）和 `packagegroup-cross-canadian-${MACHINE}`。两个名字就地解释：**nativesdk-** 前缀表示"装在 SDK 里、在 SDK 使用者的主机上运行"的包形态——chapter 4 的 qemu 三配方分工里混过脸熟（`BBCLASSEXTEND = "nativesdk"`，当时注过"chapter 14 前瞻"，今天兑现），12.3.2 的 systemd.bbclass 节选里也闪过 class-nativesdk 一行；**cross-canadian** 是"加拿大式交叉"的命名典故——编译器在甲机器上跑、给乙机器产出代码，认识这个名字即可。

第 71-76 行是目标侧清单：默认是 `packagegroup-core-standalone-sdk-target` 包组（glibc-dev、libstdc++-dev 一族都在里面），加一个占位的 dummy 包；清单里出现的 `multilib_pkg_extend` 是 multilib 变体处理的辅助函数，本章不展开，认得即可。

第 99 行把两张清单汇成 SDK 的构建依赖；第 78 行的 `TOOLCHAIN_OUTPUTNAME` 是安装器命名模板，14.2.2 拆。第 106 行先记下——`REAL_MULTIMACH_TARGET_SYS = "${TUNE_PKGARCH}${TARGET_VENDOR}-${TARGET_OS}"`，把值代进去是 `cortexa53-tiger-linux`——14.2.3 会在文件名里再见到它。

> **💡 提示**：默认清单里 go/rust 两行是按 `SDK_TOOLCHAIN_LANGS` 条件追加的——默认空，即默认 SDK 只带 C/C++ 工具链。应用组哪天要写 Go，知道开关在哪就行。

#### 14.2.2 构建与产物：安装器文件名里的两次伏笔回收

开跑。主构建目录（`MACHINE=tiger-aarch64`、`DISTRO=tiger-distro-dev`，chapter 11 收尾以来的常态）：

```bash
# 主构建目录
cd ~/workspace/poky
source oe-init-build-env ../build

# 生成 SDK 安装器（首次要编 cross-canadian 工具链一族，约 30~60 分钟级，实测回填）
bitbake tiger-image -c populate_sdk
```

<!-- 【待验证·阻塞级】C-W27：tiger 组 SDK/eSDK 构建实测——populate_sdk 实际耗时、tmp/deploy/sdk/ 安装器文件名逐字形态、安装器体积，全部待 tiger 组仓库就位后回填，续 C-W17~C-W26 批同口径。本章文件名与耗时为按 Scarthgap 源码命名模板推导的示意形态（模板本身已源码核实，见 V17 注记）；修复前版本段源码裁决预期为 nodistro.0（bitbake.conf:832 → defaultsetup.conf → default-distrovars.inc:45），实测时顺带终核。另：14.4.3 `which devtool` 输出的 `layers/poky/scripts/esdk-tools/` 路径，本机 poky 树 scripts/ 下静态追踪未找到生成点，列为本批实测重点核对项（若实测路径不同，输出与正文同步改）。 -->

构建完成，看产物：

```bash
# SDK 产物目录
ls -lh tmp/deploy/sdk/
```

输出（仅保留权限、链接数与文件名三列；示意形态，逐字以实测为准）：

```text
-rwxr-xr-x 1 oecore-tiger-image-x86_64-cortexa53-tiger-aarch64-toolchain-nodistro.0.sh
-rw-r--r-- 1 oecore-tiger-image-x86_64-cortexa53-tiger-aarch64-toolchain-nodistro.0.host.manifest
-rw-r--r-- 1 oecore-tiger-image-x86_64-cortexa53-tiger-aarch64-toolchain-nodistro.0.target.manifest
-rw-r--r-- 1 oecore-tiger-image-x86_64-cortexa53-tiger-aarch64-toolchain-nodistro.0.testdata.json
```

`.sh` 是自解压安装器，两个 `.manifest` 是主机侧/目标侧的包装箱清单（版本级），`.testdata.json` 是测试描述文件（13.3 的 testdata.json 在 SDK 侧的亲戚，testsdk 用，认得即可）。

阿凯盯着文件名看了三秒："不对——`oecore` 是谁？还有这个 `nodistro.0`——我们的版本号不是 1.0 吗？这串东西哪来的？"

"问得好。"达哥说，"11.2.1 你看过 `poky.conf` 的身份声明段；11.8.3 我埋过一句话——SDK 命名模板在 poky.conf 第 25 行。回去对账。"

```bash
# 对账：poky 的 SDK 命名是谁给的
grep -n "SDK" ~/workspace/poky/meta-poky/conf/distro/poky.conf

# 再看 OE-Core 的兜底模板
grep -n "^SDK_NAME\|^SDK_NAME_PREFIX" ~/workspace/poky/meta/conf/bitbake.conf
```

输出（关键行）：

```text
# poky.conf：
5:SDK_VENDOR = "-pokysdk"
6:SDK_VERSION = "${@d.getVar('DISTRO_VERSION').replace('snapshot-${METADATA_REVISION}', 'snapshot')}"
7:SDK_VERSION[vardepvalue] = "${SDK_VERSION}"
# ...
25:SDK_NAME = "${DISTRO}-${TCLIBC}-${SDKMACHINE}-${IMAGE_BASENAME}-${TUNE_PKGARCH}-${MACHINE}"
26:SDKPATHINSTALL = "/opt/${DISTRO}/${SDK_VERSION}"

# bitbake.conf：
459:SDK_NAME_PREFIX ?= "oecore"
460:SDK_NAME = "${SDK_NAME_PREFIX}-${IMAGE_BASENAME}-${SDK_ARCH}-${TUNE_PKGARCH}-${MACHINE}"
```

账对上了：文件名里的 `oecore-` 前缀来自 bitbake.conf 第 459-460 行的兜底模板，**`SDK_NAME` 和 `SDK_VERSION` 都是 DISTRO 配置的活**——poky 在自己的发行版配置里认领了这两行（第 25、6 行），而 tiger-distro 自建那天（11.2.2）没认领，于是 SDK 一落生就姓了别人家的姓。

`nodistro.0` 是更妙的证据。它不是空值，是 OE-Core 的**全局兜底**：`SDK_VERSION ??= "nodistro.0"` 写在 default-distrovars.inc 第 45 行，这份 inc 经 defaultsetup.conf 被 bitbake.conf 第 832 行**无条件 include**——跟发行版自己 require 什么无关，对所有 DISTRO 生效。物证一行就能自查：

```bash
# "双重寄养"的物证：兜底值写在哪
grep -n "nodistro" ~/workspace/poky/meta/conf/distro/include/default-distrovars.inc
```

输出：

```text
45:SDK_VERSION ??= "nodistro.0"
46:DISTRO_VERSION ??= "nodistro.0"
```

两行都在——SDK 版本号和发行版版本号，相邻两行各兜一个。看清楚这行的性质：`nodistro.0` 不是哪个发行版配置的名字，是 OE-Core 给没认领版本号的发行版预备的兜底值。我们的 SDK 不光前缀姓了 oecore，连版本号都认了一个叫"无发行版"的干爹——双重寄养，没有比这更直白的"你没认领身份"的物证了。

这是 11.8.3 那句 💡 框——"SDK 与目标镜像必须出自同一套 DISTRO 策略，chapter 14 交付 SDK 时回来对这句话"——的另一半：**策略不光是特性清单，还包括命名身份**。补认，三行落进公共基座（dev/prod 两态都该有同一套 SDK 身份，落 base 不落 delta）：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加，SDK 身份段）
# SDK 命名三件套（14.2）：poky.conf 第 6/25/26 行的自家版本——
# 自己的 DISTRO 自己认领，兑现 11.8.3 的伏笔
SDK_VERSION = "${DISTRO_VERSION}"
SDK_NAME = "${DISTRO}-${TCLIBC}-${SDKMACHINE}-${IMAGE_BASENAME}-${TUNE_PKGARCH}-${MACHINE}"
SDKPATHINSTALL = "/opt/${DISTRO}/${SDK_VERSION}"
```

`SDK_VERSION = "${DISTRO_VERSION}"`——11.2.2 写下的 `DISTRO_VERSION = "1.0"`，第一次变成看得见摸得着的文件名。重建（命名变量变了，BitBake 的变量依赖追踪——vardeps，任务重算时参考的变量名单——会让 do_populate_sdk 重跑；canadian 工具链已在 sstate 里，分钟级，实测回填）：

```text
# （仅保留权限与文件名两列；逐字以实测为准）
-rwxr-xr-x 1 tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.sh
-rw-r--r-- 1 tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.host.manifest
-rw-r--r-- 1 tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.target.manifest
-rw-r--r-- 1 tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.testdata.json
```

把安装器文件名逐段拆开，每一段都能认出亲爹：

```text
tiger-distro-dev - glibc - x86_64 - tiger-image - cortexa53 - tiger-aarch64 - toolchain - 1.0 .sh
     │              │        │          │            │              │                  │
   ${DISTRO}   ${TCLIBC} ${SDKMACHINE} │      ${TUNE_PKGARCH}    ${MACHINE}        ${SDK_VERSION}
                          （构建主机架构）  ${IMAGE_BASENAME}                          （= DISTRO_VERSION）
```

注意第一段是 `tiger-distro-dev` 而不是 `tiger-distro`——`${DISTRO}` 取的是当前变体名。这不是瑕疵，是免费的区分度：dev 态产的 SDK 带 `-dev` 段，prod 态产的带 `-prod` 段，两态的产物永远撞不了名（dev 态带调试档和 ptest 姿态，prod 态是出货配置，本来就不该是同一份 SDK）。

#### 14.2.3 安装与 environment-setup：一个脚本注入整个小宇宙

安装器是自解压脚本，跑起来会问安装路径：

```bash
# 安装 SDK（默认路径 /opt/tiger-distro-dev/1.0 需要 sudo；装家目录省事）
cd tmp/deploy/sdk
./tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.sh
```

输出（关键行，以实测为准）：

```text
tiger IoT Linux SDK installer version 1.0
=========================================
Enter target directory for SDK (default: /opt/tiger-distro-dev/1.0): ~/workspace/sdk/tiger
Extracting SDK...........................done
Setting it up...done
SDK has been successfully set up and is ready to be used.
Each time you wish to use the SDK in a new shell session, you need to source the environment setup script e.g.
 $ . ~/workspace/sdk/tiger/environment-setup-cortexa53-tiger-linux
```

> **⚠️ 注意**：默认路径 `/opt/...` 需要 root 权限。团队分发时这个默认值挺合适（`SDKPATHINSTALL` 三件套里已经声明了），自己机器上开发就直接装进家目录。

横幅上那行 `tiger IoT Linux SDK installer version 1.0`——11.2.2 的 `DISTRO_NAME` 和刚认领的 `SDK_VERSION`，第二张收据。

安装完，每次要用 SDK 的 shell 会话都先 source 那个环境脚本。**environment-setup 脚本**就地解释：SDK 安装目录下的环境注入入口，source 它之后，`CC`/`CXX`/`CFLAGS`/`LDFLAGS`/`PATH` 一族全部指向 SDK 内部。脚本名 `environment-setup-cortexa53-tiger-linux` 就是 14.2.1 第 106 行那个 `REAL_MULTIMACH_TARGET_SYS` 的实物：`cortexa53`（TUNE_PKGARCH）+ `-tiger`（TARGET_VENDOR）+ `-linux`（TARGET_OS）——11.2.2 改 `TARGET_VENDOR = "-tiger"` 的伏笔，第二次回收（第一次是 11.8.3 的 `tmp/work/tiger_aarch64-tiger-linux/` 路径预言）。

source，验三样：

```bash
# 注入 SDK 环境
source ~/workspace/sdk/tiger/environment-setup-cortexa53-tiger-linux

# 编译器是谁、sysroot 在哪、编译器能跑
echo $CC
echo $SDKTARGETSYSROOT
$CC --version | head -1
```

输出（关键行，长行折行排版，以实测为准）：

```text
aarch64-tiger-linux-gcc -mcpu=cortex-a53 ... --sysroot=/home/<your-username>/workspace/sdk/tiger/sysroots/cortexa53-tiger-linux
/home/<your-username>/workspace/sdk/tiger/sysroots/cortexa53-tiger-linux
aarch64-tiger-linux-gcc (GCC) 13.x ...
```

三件事各有看头。`$CC` 的前缀 `aarch64-tiger-linux-` 是 TARGET_SYS 三元组（TARGET_ARCH + TARGET_VENDOR + TARGET_OS），跟脚本名的 `cortexa53-tiger-linux`（TUNE_PKGARCH 打头）是同一个 `-tiger` 的两张脸。**SDKTARGETSYSROOT** 就地解释：environment-setup 导出的目标 sysroot 路径变量——应用侧排查"头文件在不在"的问题，都从这里查起。`--sysroot=` 参数已经缝死在 `$CC` 里，应用工程师不用手写任何路径。

> **💡 提示**：脚本开头有一段检查——`LD_LIBRARY_PATH` 非空时拒绝注入（toolchain-scripts.bbclass 生成的固定检查段）。哪天 source 失败先看它。

### 14.3 SDK 内容定制：TOOLCHAIN_HOST_TASK / TOOLCHAIN_TARGET_TASK

#### 14.3.1 两张清单的分工

默认 SDK 装完，sysroot 里只有 `packagegroup-core-standalone-sdk-target` 带来的 glibc/libstdc++ 一族。够用吗？达哥只点拨了一句："SDK 装出来的 sysroot 里有没有你要的头文件，取决于一张清单——12.2 你往镜像里装了 `mtd-utils-ubifs` 和 `i2c-tools`，应用组十有八九要对着它们编程。"

**工具链主机任务 / 工具链目标任务（TOOLCHAIN_HOST_TASK / TOOLCHAIN_TARGET_TASK）** 本章正式引入：两张 BitBake 变量清单，分别声明 SDK 里面向构建主机的工具包（进主机侧 sysroot）和面向目标板的库与头文件包（进目标侧 sysroot）。分工一张表：

| 变量 | 管哪一半 | 装进去的东西 | 什么时候加 |
|------|---------|-------------|-----------|
| `TOOLCHAIN_HOST_TASK` | 主机侧 | nativesdk- 工具（cmake、qemu 等） | 应用组的构建流程需要某个主机工具时 |
| `TOOLCHAIN_TARGET_TASK` | 目标侧 sysroot | -dev 包（头文件 + 库） | 镜像里有的组件，应用组要对它编程时 |

tiger 的活明显在第二张：加 `mtd-utils` 和 `i2c-tools` 的开发文件。

#### 14.3.2 定制与验证：一半在，一半不在

阿凯提笔就写——把 12.2 名单里这两个包名原样抄了过来：

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-core/images/tiger-image.bb（错误示范，请勿模仿）
# 第一稿：想当然——"镜像里装什么，SDK 里就配什么"
TOOLCHAIN_TARGET_TASK:append = " mtd-utils-ubifs i2c-tools"
```

重建 SDK——风平浪静，安装器照常产出。两个包名都真实存在、都装得进 sysroot，构建系统没有任何理由拦他。

"构建没报错。"阿凯说，"但 12.7.1 立的规矩我没忘——13.8.1 还炸过第二回。装上看看。"

```bash
# 装第一稿 SDK（另选目录，不动已装好的那份）
./<SDK 安装器>.sh
# 即 14.2.3 装出的那份 .sh；这次装到 ~/workspace/sdk/tiger-draft

# 查两家头文件
source ~/workspace/sdk/tiger-draft/environment-setup-cortexa53-tiger-linux
find $SDKTARGETSYSROOT/usr/include -name "*smbus*"
find $SDKTARGETSYSROOT/usr/include -name "libmtd.h" -o -name "libubi.h"
```

输出：

```text
/home/<your-username>/workspace/sdk/tiger-draft/sysroots/cortexa53-tiger-linux/usr/include/i2c/smbus.h
（第二条 find 无输出）
```

阿凯愣住。i2c 的头文件**在**，libmtd/libubi 的**不在**——可两个名字明明是同一种写法写进去的。

顺手澄清一个容易混淆的点：`usr/include/mtd/` 目录其实是在的——你可以当场自查，`ls $SDKTARGETSYSROOT/usr/include/mtd/` 就能看到。里面 `mtd-user.h` 一族是**内核 UAPI 头**（内核暴露给用户空间的接口头文件，认得即可），由 `linux-libc-headers` 配方（把内核头文件导出给 C 库/SDK 用的专用配方，认得即可）随 glibc-dev 的依赖进场，任何默认 SDK 都有；应用组要操作 MTD 设备，缺的是 mtd-utils **自家**的 `libmtd.h`/`libubi.h`——两户人家，别认错门。

他把这条线索拿给达哥看。达哥没答："sysroot 里有没有，你已经查了。再查一层——`i2c-tools-dev` 这个名字，你自己写过吗？"

没写过。那它是谁装进去的？查装箱清单：

```bash
# 看 SDK 目标侧的包清单（deploy 目录里的 .target.manifest，cwd 即该目录）
grep -E "i2c-tools|mtd" <SDK 安装器基名>.target.manifest
```

输出（关键行）：

```text
i2c-tools 4.3 ...
i2c-tools-dev 4.3 ...
mtd-utils-ubifs 2.1.6 ...
```

清单里躺着 `i2c-tools-dev`——没人写它，它自己出现了。阿凯回头翻 populate_sdk_base.bbclass，这次有目标地翻，翻到两段：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/populate_sdk_base.bbclass（节选，20/37 行）
COMPLEMENTARY_GLOB[dev-pkgs] = '*-dev'
# ...
SDKIMAGE_FEATURES ??= "dev-pkgs dbg-pkgs src-pkgs ${@bb.utils.contains('DISTRO_FEATURES', 'api-documentation', 'doc-pkgs', '', d)}"
```

机制齐了。SDK 的目标 sysroot 装完 TOOLCHAIN_TARGET_TASK 的明面清单后，还有一道**补装通道**：`SDKIMAGE_FEATURES` 默认带着 `dev-pkgs` 这个词，对应的通配规则是 `*-dev`——包管理器对已装清单里每个包做名字替换（`i2c-tools` → `i2c-tools-dev`），存在就顺手装进去（ipk 后端的 `install_complementary`，13.2.1 那张三级证据链里的 `COMPLEMENTARY_GLOB` 就是这套机制）。

于是两半命运分家的原因清楚了：

- `i2c-tools` 被装上了 → 补装通道找到 `i2c-tools-dev` → **头文件被"救活"**；
- `mtd-utils-ubifs` 被装上了 → 替换出 `mtd-utils-ubifs-dev` → 这个包**不存在**，静默跳过。那 mtd-utils 主包自己的 `-dev` 同伴有没有机会进场？没有，三个因果环扣死了：
  1. mtd-utils 的配方把 libmtd/libubi 只打了**静态库**（do_install 里 `oe_libinstall -a`，认得即可）；
  2. ubifs 工具不依赖 mtd-utils 主包，主包不会经依赖链进 sysroot；
  3. 主包不在已装清单里，`mtd-utils` → `mtd-utils-dev` 的名字替换从头到尾不会发生。

阿凯把第一稿那行划掉，改写正确的形态：

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-core/images/tiger-image.bb（追加，14.3 交付物）
# SDK 目标侧 sysroot 清单（14.3）：镜像里装了 mtd-utils-ubifs / i2c-tools（12.2），
# 应用组要对它们编程——头文件在 -dev 包里；libmtd/libubi 只有静态库形态，.a 归 -staticdev 包。
# 显式写 -dev/-staticdev 包名，不靠 dev-pkgs 补装机制赏饭（教训见 14.3.2 正文与 14.8.1）
TOOLCHAIN_TARGET_TASK:append = " mtd-utils-dev mtd-utils-staticdev i2c-tools-dev"
```

多出来的 `mtd-utils-staticdev` 是第二条包名纪律：默认拆分里，头文件和 `.so` 链接归 `-dev` 包，`.a` 静态库归 `-staticdev` 包（bitbake.conf 的 FILES 拆分规则）；而补装通道默认只补 `-dev/-dbg/-src` 三族（`SDKIMAGE_FEATURES` 里没有 `staticdev-pkgs`）。mtd-utils 的库只有静态形态——应用组要链接 libmtd，`-staticdev` 就必须显式点名。还是那句主线：**清单里没写的，sysroot 里就没有**。

重建、重装（装回 `~/workspace/sdk/tiger`，先把旧目录删掉再装），验证三样——头文件、静态库、体积。注意会话分工：重建要回 source 过 oe-init-build-env 的构建会话里跑，别在注入过 SDK 环境的 shell 里跑 bitbake：

```bash
# 重建命令同 14.2.2（bitbake tiger-image -c populate_sdk，在构建会话里跑），然后：
# 修正版 SDK 的头文件与库验证
rm -rf ~/workspace/sdk/tiger
# 装回操作回到原 shell（cwd 仍在 tmp/deploy/sdk）
./<SDK 安装器>.sh   # 路径仍选 ~/workspace/sdk/tiger

source ~/workspace/sdk/tiger/environment-setup-cortexa53-tiger-linux
find $SDKTARGETSYSROOT/usr/include \( -name "mtd-*.h" -o -name "libmtd.h" -o -name "libubi.h" \)
ls $SDKTARGETSYSROOT/usr/lib/libmtd.a $SDKTARGETSYSROOT/usr/lib/libubi.a
du -sh ~/workspace/sdk/tiger
```

输出（关键行，以实测为准）：

```text
.../usr/include/mtd/mtd-user.h
.../usr/include/mtd/mtd-abi.h
.../usr/include/libmtd.h
.../usr/include/libubi.h
.../usr/lib/libmtd.a
.../usr/lib/libubi.a
1.2G    /home/<your-username>/workspace/sdk/tiger
```

> **💡 提示**：SDK 天生胖——`SDKIMAGE_FEATURES` 默认的 `dev-pkgs dbg-pkgs src-pkgs` 三词会把 sysroot 里每个包的 -dev/-dbg/-src 同伴都补进来，这是它该有的样子（应用组要调试符号和头文件）。但**定制前先量**：改 TOOLCHAIN_TARGET_TASK 之前 `du -sh` 记基线，改完再量——"要加先量"，12.7.2 的纪律在 SDK 场景原样适用。而且 13.5 开的 buildhistory 已经在自动记 SDK 的账（`sdk` 是它的三个记录类别之一），每次构建的 SDK 体积变化都落盘，14.7 认这个实物。

#### 14.3.3 坑 1 发作实录：构建不拦，应用工程师当场炸

倒带一步：假如 14.3.2 没查那一层，第一稿的 SDK 直接发给了应用组，会发生什么？阿凯用第一稿 SDK 的环境模拟了一遍应用组的第一天：

```bash
# 用第一稿 SDK 的环境（你可能遇到的错误——仅当 TOOLCHAIN_TARGET_TASK 写了运行包名）
source ~/workspace/sdk/tiger-draft/environment-setup-cortexa53-tiger-linux

# 应用组视角：写个探针——要操作 MTD 设备，先包含 mtd-utils 自家的库头文件
cat > /tmp/mtd_probe.c <<'EOF'
#include <libmtd.h>
int main(void) { return 0; }
EOF

$CC -c /tmp/mtd_probe.c -o /tmp/mtd_probe.o
```

输出：

```text
/tmp/mtd_probe.c:1:10: fatal error: libmtd.h: No such file or directory
    1 | #include <libmtd.h>
      |          ^~~~~~~~~
compilation terminated.
```

`fatal error: No such file or directory`——应用工程师拿到 SDK 的第一天就撞上这堵墙，然后回来找 BSP 组："你们的 SDK 是不是没打全？"构建期没有一行报错预告过这件事。这段发作与复盘，收进 14.8.1。

### 14.4 生成 eSDK：populate_sdk_ext、SDK_EXT_TYPE 选择

#### 14.4.1 机制实物：populate_sdk_ext.bbclass

**生成 eSDK（populate_sdk_ext）** 与 **eSDK 类型（SDK_EXT_TYPE）** 本章一并正式引入：前者是镜像配方的另一个任务，产出含 bitbake/devtool 的可扩展 SDK；后者控制它的形态——`full`（默认）或 `minimal`。类文件头上的实物：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/populate_sdk_ext.bbclass（节选，11-26 行）
# Used to override TOOLCHAIN_HOST_TASK in the eSDK case
TOOLCHAIN_HOST_TASK_ESDK = " \
    meta-environment-extsdk-${MACHINE} \
    "
# ...
SDK_EXT = ""
SDK_EXT:task-populate-sdk-ext = "-ext"
# ...
# Options are full or minimal
SDK_EXT_TYPE ?= "full"
SDK_INCLUDE_PKGDATA ?= "0"
SDK_INCLUDE_TOOLCHAIN ?= "${@'1' if d.getVar('SDK_EXT_TYPE') == 'full' else '0'}"
SDK_INCLUDE_NATIVESDK ?= "0"
SDK_INCLUDE_BUILDTOOLS ?= '1'
```

三行要点：

1. eSDK 不用 14.3 那两张清单——它把 TOOLCHAIN_HOST_TASK 整个换成 `meta-environment-extsdk-${MACHINE}`（第 12-14 行），还把 TOOLCHAIN_TARGET_TASK 清空（类文件后文第 786 行）：eSDK 的主机侧不是一包工具，是**一整套构建系统**；目标侧素材走 sstate 快照通道，不走包清单——所以 14.3 的清单定制对 eSDK 不生效，别惊讶。
2. 安装器文件名里的 `-ext` 段，来自类文件第 85-86 行的 `TOOLCHAINEXT_OUTPUTNAME` 模板（`toolchain-ext` 硬编码在模板里，经 task 级覆写接管输出名）；而第 18-19 行的 `SDK_EXT` 是另一个 task 级变量，它拼进的是 buildhistory 等处的目录命名——两处都带 `-ext`，别认错门。
3. 第 22-24 行是本节的题眼：`SDK_EXT_TYPE` 默认 `full`，`SDK_INCLUDE_TOOLCHAIN` 跟着它走——full 型把工具链的 sstate 快照打进安装包，minimal 不打。

#### 14.4.2 full vs minimal：一张表说清钱花在哪儿

| 维度 | `full`（默认） | `minimal` |
|------|---------------|-----------|
| 安装器体积 | GB 级 | 小一个数量级 |
| 内含 sstate 快照 | 有（锁定签名的任务产物快照） | 无 |
| 首次 devtool build | 快照命中，分钟级 | 找不到快照 → 从源码全量重建工具链，小时级；源码包还得现下载 |
| 网络假设 | 离线可用 | 假设有网络（或配好 sstate mirror） |
| 适用 | 内网/离线交付 | 在线环境、有 sstate 镜像服务器的团队 |

一句话：**minimal 省下的体积，是把 sstate 的获取推迟到了使用时**。这个推迟值不值钱，取决于接收方的网络环境——这句话先放着，14.4.3 听达哥疼过一次，14.8.2 复盘。表里的 devtool 实物 14.6 才上手——这里只需记住一件事：`devtool build` 要动工具链任务，而工具链任务的快照只在 full 里。

#### 14.4.3 构建、安装，与一次体积诱惑

主线用默认 full。构建还是在干净的构建会话里跑（前面 source 过 SDK 环境的 shell 别拿来跑 bitbake；注意这个任务标了 `nostamp`，每次必重跑，不复用自身旧产物）：

```bash
# 生成 eSDK（含构建系统本体与 sstate 快照，首次构建小时级；产物 GB 级——
# 耗时与体积实测回填，磁盘预留参考：10 GB 以上富余）
bitbake tiger-image -c populate_sdk_ext

# 看产物（与 14.2 的 SDK 安装器同目录）
ls -lh tmp/deploy/sdk/
```

输出（仅保留权限、链接数、体积与文件名四列；示意形态，逐字与体积以实测为准）：

```text
-rwxr-xr-x 1 1.5G tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-ext-1.0.sh
-rwxr-xr-x 1 300M tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.sh
# ... (manifest 与 testdata.json 一族省略)
```

安装到老规矩的家目录路径：

```bash
# 安装 eSDK 到 ~/workspace/esdk/tiger（先进产物目录，与 14.2.3 同体例）
cd tmp/deploy/sdk

# 提示符处输入安装路径：Enter target directory ... : ~/workspace/esdk/tiger
# （eSDK 的默认目录是 ~/<DISTRO>_sdk，即 ~/tiger-distro-dev_sdk——与 SDK 的 /opt 默认不同）
# 尾段的 "Preparing build system..." 要数分钟——full 型的 sstate 快照在此展开
./tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-ext-1.0.sh
```

> **⚠️ 注意**：eSDK **不能以 root 安装**——安装器前置检查里写死了（populate_sdk_ext.bbclass 的 sdk_ext_preinst，`id -u` 为 0 直接报错退出）。`sudo` 装 eSDK 是新人常撞的第一堵墙。

新开一个 shell，注入环境，认主角：

```bash
# 新开 shell（⚠️ 别在 source 过 oe-init-build-env 的会话里混用，eSDK 脚本会警告）
source ~/workspace/esdk/tiger/environment-setup-cortexa53-tiger-linux
which devtool
```

输出（关键行，以实测为准）：

```text
SDK environment now set up; additionally you may now run devtool to perform development tasks.
Run devtool --help for further details.
/home/<your-username>/workspace/esdk/tiger/layers/poky/scripts/esdk-tools/devtool
```

阿凯盯着那个 1.5G 的安装器看了一会儿："full 这档也太大了。`minimal` 小一个数量级——发 minimal 不行吗？"

"你自己量量看。"达哥说，"顺便想清楚它把什么推迟了。"

阿凯会话级切了一档（演示用，不落仓库）：

```bash
# 在 ~/workspace/build/conf/local.conf 末尾临时追加（演示用，验完即删，不进仓库）：
SDK_EXT_TYPE = "minimal"
```

```bash
# 重建 eSDK（回到构建会话 ~/workspace/build；nostamp 必重跑；耗时实测回填）
bitbake tiger-image -c populate_sdk_ext
ls -lh tmp/deploy/sdk/*toolchain-ext*.sh
```

输出（仅保留权限、链接数、体积与文件名四列；示意形态，体积对比实测回填）：

```text
-rwxr-xr-x 1 120M tiger-distro-dev-...-toolchain-ext-1.0.sh
```

<!-- 【待验证·阻塞级】C-W28：eSDK full/minimal 实际体积对比、minimal 构建耗时与安装行为，待实测回填，续 C-W17~C-W27 批同口径。 -->

> **⚠️ 注意**：minimal 安装器与 full 同名，会**覆盖** deploy 目录里那份 full 产物。这次只构建不安装；验完体积把 local.conf 的 `SDK_EXT_TYPE` 行删掉、重跑一次 `-c populate_sdk_ext` 把 full 产物恢复回来（sstate 兜底，分钟级）。

体积确实诱人。阿凯刚要松口说"发 minimal 也行"，达哥按住他："这个坑我替你踩过。上家公司的项目，我图体积小数倍发了 minimal 给一家客户——内网环境，没外网，也没架 sstate 镜像。客户在 eSDK 里第一次 `devtool build`，电话就打回来了。"

发作链条：minimal 的包里没有 sstate 快照，`devtool build` 要用的工具链任务一个都 setscene 不了（setscene：sstate 快照复用的执行形式——快照在就直接解包复用，不在才落回真实执行），全部落回真实执行；真实执行第一步是 `do_fetch` 拉源码包——离线环境拉不到：

```text
# 报错文案为示意形态，精确发作形态（报错文案/发作时机）待核实——见 14.8.2 的 V18 注记
ERROR: binutils-cross-aarch64-...: do_fetch: Fetcher failure: Unable to fetch URL ...
# ... (连锁失败，省略)
```

这是达哥的事故复盘，不是现场演示——读者手里能 `devtool add` 的本地源码要到 14.5.1 才出生，断网一节在容器里也做不干净；机理记住即可。有网没镜像的情形也好不到哪去：拉得下源码，就要从源码全量重建工具链——小时级。minimal 推迟的那笔账，在"使用时"连本带利地收。复盘归 14.8.2。

### 14.5 自我验证：用 SDK 编一个示例应用，部署上板跑通

"发件人的活干完了。"达哥看了眼白板，"换帽子——**从现在起你是应用工程师**，我是旁观的。"

#### 14.5.1 hello-tiger：一个独立目录里的示例应用

应用工程师的第一纪律先表态：示例应用放 `~/workspace/hello-tiger/`——**独立目录，不进 meta-tiger**。功能开发在独立仓库（开发态），Yocto 只做集成（集成态），这条原则对应用代码同样成立；把它想象成应用组自己的 git 仓库就对了：

```bash
# 应用组的"仓库"（独立目录，模拟应用组视角）
mkdir -p ~/workspace/hello-tiger
cd ~/workspace/hello-tiger
git init -b main
```

应用写什么？写一个探针：读 `/etc/os-release` 报出自己的发行版身份，再附一个 `uname` 核对运行环境——跟 12.3 的 tiger-sysinfo 读的是同一个文件，BSP 侧和应用侧在同一个文件上闭环：

```c
/* 文件路径：~/workspace/hello-tiger/hello-tiger.c（新建） */
/* 应用组视角的探针：读 /etc/os-release 报身份，附 uname 核对运行环境 */
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

static void print_pretty_name(void)
{
    FILE *fp = fopen("/etc/os-release", "r");
    char line[256];

    if (!fp) {
        perror("/etc/os-release");
        return;
    }
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *v = line + 12;
            v[strcspn(v, "\n")] = '\0';
            if (v[0] == '"') {          /* 去掉首尾引号 */
                v++;
                v[strlen(v) - 1] = '\0';
            }
            printf("board identity: %s\n", v);
        }
    }
    fclose(fp);
}

int main(void)
{
    struct utsname uts;

    print_pretty_name();
    if (uname(&uts) == 0)
        printf("running on: %s %s %s\n", uts.sysname, uts.release, uts.machine);
    return 0;
}
```

Makefile 的关键是三个变量都用**环境里的值**——`$(CC)`、`$(CFLAGS)`、`$(LDFLAGS)` 一个都不自定义，这正是 SDK 的用法（environment-setup 注了什么，工程就消费什么）。顺手带上 `install` 目标，14.6 的 devtool 要用：

```make
# 文件路径：~/workspace/hello-tiger/Makefile（新建）
# CC/CFLAGS/LDFLAGS 全部来自 environment-setup 注入的环境变量，一个都不自定义
PREFIX ?= /usr

hello-tiger: hello-tiger.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

install: hello-tiger
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 hello-tiger $(DESTDIR)$(PREFIX)/bin/

clean:
	rm -f hello-tiger

.PHONY: install clean
```

建仓就落第一笔提交——应用组的仓库没有空壳期。这一笔不只是形式：14.6 的 devtool 收编本地源码树时，只要目录里有 `.git` 它就会去读 HEAD，零提交的仓库会让它当场报错退出：

```bash
# 应用组建仓的第一笔提交（14.6 的 devtool add 要读这个 HEAD）
git add -A
git commit -m "Initial commit: hello-tiger probe"
```

#### 14.5.2 SDK 编译：环境注入后，make 就是 make

```bash
# 应用工程师的日常：source 环境，然后该 make 就 make
source ~/workspace/sdk/tiger/environment-setup-cortexa53-tiger-linux
cd ~/workspace/hello-tiger
make

# 产物是什么架构？
file hello-tiger
```

输出（关键行，以实测为准）：

```text
aarch64-tiger-linux-gcc -mcpu=cortex-a53 ... --sysroot=... -o hello-tiger hello-tiger.c
hello-tiger: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-aarch64.so.1, ...
```

`file` 确认：aarch64 的 ELF，动态链接，解释器路径是标准的 `/lib/ld-linux-aarch64.so.1`。x86_64 主机上编出了 aarch64 产物——SDK 的第一半职责兑现。

部署之前先回答一个合理疑问：这个 SDK 是 cortexa53 调优的（tiger 的 MACHINE），而接下来要跑的板子是 cortexa57 的 qemuarm64（对照组），产物能跑吗？能，两个条件都满足：指令集上，cortexa53 调优产物用的是 ARMv8-A 指令集（含 crc 扩展，cortex-a57 同样支持），cortexa57 同为 AArch64 实现，向上兼容；库上，两边镜像出自**同一个 DISTRO**（都是 tiger-distro-dev）、同一份 glibc 源码版本——sysroot 里的库和板上的库同源同版本。

> **⚠️ 注意**：这条兼容论证的边界要记清——它只在"同 DISTRO、同 C 库、ISA 子集兼容"时成立。哪天对照组换成 musl libc 或别家发行版配置的镜像，这套说辞立刻作废，老老实实各配各的 SDK。

#### 14.5.3 部署上板：对照组先跑，tiger 组挂账

部署目标是**对照组** qemuarm64 + tiger-image（dev 态）——13.3.2 装好的 dropbear 门和 tiger-testkey 钥匙原样复用，scp 通道 13.3 已经考过 PASSED。tiger 板本身没有网卡（外设清单只有 UART/I2C/SPI/存储一族），SSH 通道不存在，板上部署走串口或其他路径——这条挂账，正文末尾注记。

对照组启动（build-distrotest，dev 态镜像）：

```bash
# 对照组环境：MACHINE=qemuarm64 + DISTRO=tiger-distro-dev
cd ~/workspace/poky
source oe-init-build-env ../build-distrotest

# 启动（slirp 用户态网络：主机 2222 转发到板上 22）
runqemu qemuarm64 tiger-image nographic slirp
```

另开一个终端，把编好的应用送上板、跑起来：

```bash
# 若这是 13.3.2 之后新开的会话，先把钥匙挂回 ssh-agent（挂钥匙的动作按会话生效）：
eval $(ssh-agent -s) && ssh-add

# 部署（免交互选项只在开发环境用）
scp -P 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
    ~/workspace/hello-tiger/hello-tiger root@localhost:~

# 登板执行
ssh -p 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@localhost
```

板上：

```bash
# 板上执行（登录后）
./hello-tiger
```

输出（以实测为准）：

```text
board identity: tiger IoT Linux 1.0 (beacon)
running on: Linux 6.6.x ... aarch64
```

第一行是 `PRETTY_NAME` 的实物——11.2.2 的 `DISTRO_NAME` + `DISTRO_VERSION` + `DISTRO_CODENAME` 三件套，经 os-release.bb 拼成，落到板上 `/etc/os-release`，再被应用读出来。SDK 编的应用，在自己发行的系统上报出了自己的身份。**编译—部署—跑通**，最小闭环合上。

<!-- 【待验证·阻塞级】C-W29：tiger 板上运行 SDK 编译应用的部署路径与实测输出。tiger 板无网卡（无外接网络模型），候选路径：(a) 串口 base64 贴传（busybox base64 applet）；(b) qemu-tiger 追加 virtio-net 等网络模型（需 qemu-tiger 仓库裁决可行性）。待仓库就位后回填，并一并裁决本节的最终措辞，续 C-W17~C-W28 批同口径。 -->

阿凯扮演完应用工程师，把帽子摘回来，记了一笔：**应用开发者视角的完整速览**——环境变量清单、CMake 工程的接法、调试姿势——今天只走了最小闭环，完整版归附录 A，本节是那里的底稿。

### 14.6 自我验证：用 eSDK 的 devtool add 加一个新组件

#### 14.6.1 devtool：eSDK 里的主角

**devtool** 本章正式引入：Yocto 提供的开发辅助命令行工具，覆盖"加新组件、改现有配方、升级版本"一族开发动作。前史两段：chapter 8 改内核时它就在 💡 框里点过名（8.9——"能给配方开一块独立的开发工作区"，今天的 externalsrc 正是那句描述的实物）；13.6 瞭望上游哨兵时又露过一面（`devtool check-upgrade-status`），当时只跑了条查询命令。今天正式上手它的主战场功能。

为什么在 eSDK 里用？14.4.1 已经交代：eSDK 的主机侧是一整套封装好的 bitbake + devtool + sstate 快照——拿到 eSDK 的人没有 poky 源码树，devtool 就是他改系统的全部家伙。阿凯继续扮演应用工程师：镜像里缺一个自家组件（就用 hello-tiger 扮演这个角色），把它加进来。

#### 14.6.2 devtool add / build 最小闭环

```bash
# 新开 shell，注入 eSDK 环境（提醒：与 oe-init-build-env 的会话不要混用）
source ~/workspace/esdk/tiger/environment-setup-cortexa53-tiger-linux

# eSDK 的环境脚本不切换目录——workspace/ 与 tmp/ 都是相对安装根的路径
cd ~/workspace/esdk/tiger

# 把本地源码树收编成配方（本地路径，离线友好——不拉任何网络源码）
devtool add hello-tiger ~/workspace/hello-tiger
```

`devtool add` 干了两件事：在 eSDK 的 workspace 里生成一份配方，再挂一个 bbappend 把源码指到你的本地目录。看实物：

```bash
# 生成的配方与外挂 bbappend
cat workspace/recipes/hello-tiger/hello-tiger.bb
cat workspace/appends/hello-tiger.bbappend
```

输出（节选，示意形态，逐字以实测为准）：

```text
# workspace/recipes/hello-tiger/hello-tiger.bb：
# NOTE: LICENSE is being set to "CLOSED" to allow you to at least start building - if
# this is not accurate with respect to the licensing of the software being built (it
# will not be in most cases) you must specify the correct value before using this
# recipe for anything other than initial testing/development!
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""
SRC_URI = ""
# ... (其余从略)

# workspace/appends/hello-tiger.bbappend：
inherit externalsrc
EXTERNALSRC = "/home/<your-username>/workspace/hello-tiger"
EXTERNALSRC_BUILD = "/home/<your-username>/workspace/hello-tiger"

# initial_rev .: <建仓提交的哈希>
```

配方本体是自动生成的骨架——hello-tiger 目录里没有任何长得像许可证的文件，recipetool 找不到可猜的对象，就把 LICENSE 按 `CLOSED` 占位，并在注释里留下官方原话：除了最初的测试和开发，用这份配方之前**必须**改成正确的许可证值。真正有意思的是那个 bbappend：**externalsrc** 就地解释——一个让配方"不拉源码、直接用你指给它的本地目录"的机制；`devtool add` 给的本地路径就写在这里。你改 `~/workspace/hello-tiger/` 里的代码，devtool 构建时用的就是工作现场，不用提交、不用打包。

bbappend 里另外两行也认一下：`EXTERNALSRC_BUILD` 与源目录同路径——构建目录与源目录合一（B==S）的明示，Makefile 工程原地编译；末尾 `# initial_rev` 那行注释是 devtool 记下的建仓提交哈希，是它往后追踪源码改动的锚点——14.5.1 那笔初始提交要是没做，这一步根本走不到。

构建：

```bash
# 在 eSDK 里构建新组件（full 快照命中，分钟级）
devtool build hello-tiger

# 产物在哪
find tmp/work -path "*hello-tiger*" -name "hello-tiger" -type f 2>/dev/null | head -3
```

输出（关键行，路径以实测为准）：

```text
tmp/work/cortexa53-tiger-linux/hello-tiger/.../image/usr/bin/hello-tiger
```

工作路径里的 `cortexa53-tiger-linux` 三元组又见面了——eSDK 里的构建目录结构和完整构建系统是同一张脸。产物 `image/usr/bin/hello-tiger` 是配方的安装暂存，对应 Makefile 里那个 `install` 目标（`$(DESTDIR)$(PREFIX)/bin`）。

顺带把 14.4.3 的账合上：这次 `devtool build` 分钟级收工，靠的就是 full 安装器里那份 sstate 快照——**full 的 GB 级体积，买的是这一刻**。

#### 14.6.3 点到为止：主菜在下一章

今天只走 add + build 的最小闭环。devtool 的完整工作流——`devtool modify` 拉出现有配方（比如内核）的源码改 bug、`devtool finish` 把修改导出为 patch 收回 layer——是 chapter 15 的主菜，到时候这把刀才真正开锋（13.6 埋的 AUH 悬念，从那一章起陆续收）。

> **💡 提示**：还有一个可选动作 `devtool deploy-target hello-tiger root@<目标>`——把构建产物直接部署上板。它需要目标板的 SSH 通道：对照组 slirp 网络的 2222→22 转发是 runqemu 默认自带的（源码可证），但 deploy-target 的端到端行为未经实测，挂账 V20；tiger 板无网卡，同 C-W29 一并裁决。今天不占主线。

### 14.7 SDK 的分发与版本化

阿凯早上给自己立的第四件事，现在收账。三件事：版本化锚点、留痕、合规。

**版本化锚点**已经有了，而且是双份：安装器文件名里的 `toolchain-1.0`（14.2.2，`SDK_VERSION = "${DISTRO_VERSION}"`），和文件名首段的 `-dev`/`-prod` 变体区分（`${DISTRO}` 段）。SDK 与镜像同一条 DISTRO 策略、同一个版本号——应用组报问题时报一句"1.0 的 SDK"，BSP 组就知道对应哪份镜像。归档时给安装器配一份校验和：

```bash
# 发布归档的最小动作：校验和随安装器一起走
cd ~/workspace/build/tmp/deploy/sdk
sha256sum tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.sh \
    > tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64-toolchain-1.0.sha256
```

**留痕**不用动手——13.5 开的 buildhistory 已经把 SDK 构建自动记上了（`BUILDHISTORY_FEATURES` 三个词里就有 `sdk`）。认一下实物：

```bash
# buildhistory 的 SDK 存档目录（目录名就是 SDK_NAME + 镜像名）
ls ~/workspace/build/buildhistory/sdk/
ls ~/workspace/build/buildhistory/sdk/tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64/tiger-image/
```

输出（关键行，以实测为准）：

```text
tiger-distro-dev-glibc-x86_64-tiger-image-cortexa53-tiger-aarch64

files-in-sdk.txt  host/  sdk-files/  sdk-info.txt  target/
```

`sdk-info.txt` 里记着 `SDKSIZE`（KiB 级体积）、两张 TOOLCHAIN 清单的当时值——14.3 那次清单修正，在这里留着两次构建的 diff 可查。`files-in-sdk.txt` 是 SDK 内容的全量文件清单；`host/` 与 `target/` 两个子目录各存一份 `installed-package-names.txt`——主机侧、目标侧的包装箱名单分开放，正是 SDK 两半结构在账本上的投影。每次构建自动 git 提交（13.5 的 `BUILDHISTORY_COMMIT` 默认开），"谁把 SDK 喂胖了"和"谁改了清单"都有账可查。

**合规**先立一个意识：**SDK 也是分发物**——它里面装着 glibc、libstdc++ 等一票第三方组件的库和头文件，发 SDK 和发镜像一样要过 license 这道关。

现状的账先记清：镜像侧的 license 清单（`tmp/deploy/licenses/` 下按镜像名归目录的 `license.manifest` 一族）由 license_image.bbclass 在镜像 rootfs 后处理阶段产出；而 SDK 的装配不走那条通道，**SDK 自身没有对应的 license.manifest 产物**——SDK 自带的 `.target.manifest` / `.host.manifest` 是版本级的包清单，可以当合规的底稿用，但 license 列要另想办法。完整的动作——archiver 源码归档、SBOM（Software Bill of Materials，软件物料清单）、发布清单——归 chapter 16，这里只把问题挂上账（V19）。

> **💡 提示**：CI 顺手延伸一句——13.7 的流水线要给 SDK 加 job，只是再加一行 `bitbake tiger-image -c populate_sdk` 的事（stage 挂在 build 之后，产物收 artifacts）。知道即可，YAML 今天不动。

### 14.8 踩坑实录

#### 14.8.1 坑 1：TOOLCHAIN_TARGET_TASK 写运行包名，SDK 照常生成、应用编译才炸

发作过程见 14.3.2/14.3.3：第一稿把 12.2 名单里两个包名抄成 `mtd-utils-ubifs i2c-tools`，populate_sdk 构建期不拦（两个包真实存在、能装进 sysroot），应用工程师 `#include <libmtd.h>` 当场 `fatal error: No such file or directory`。

机理两句话。其一，**sysroot 里有什么，取决于 TOOLCHAIN_TARGET_TASK 装什么**——头文件住在 `-dev` 包里，`.a` 静态库住在 `-staticdev` 包里，运行包名装进来的只有二进制。其二，这个坑的阴险在**不对称**：`i2c-tools` 被 `dev-pkgs` 补装通道救活了（已装清单名字替换出 `i2c-tools-dev`，存在即补装），`mtd-utils-ubifs` 没有同名 `-dev` 同伴、又不经依赖链拉回主包，补装通道爱莫能助——**同一个错误写法，一半被机制兜底、一半裸奔**，这正是不排查就发现不了的原因。

族谱认门：这是 13.8.1 那一族——**构建期不拦、运行时才炸**——的第二次发作。第一次炸在板上（ptest 引用了主机路径），这次炸在应用工程师的编译命令行里。共同纪律也原样适用并升级：别信"构建没报错"；改完 TOOLCHAIN_TARGET_TASK，装出 SDK 后把你要的头文件和库**逐个点名验过**（`find $SDKTARGETSYSROOT -name "libmtd.h"`、`ls $SDKTARGETSYSROOT/usr/lib/libmtd.a`），再发出去。

#### 14.8.2 坑 2：SDK_EXT_TYPE = "minimal" 的离线坑

发作过程见 14.4.3：minimal 安装器小一个数量级，很诱人；但离线（或没配 sstate 镜像）环境下，eSDK 里第一次 `devtool build` 就现原形——sstate 快照不在包里，任务全部落回真实执行，`do_fetch` 拉不到源码直接 fetch 失败；有网没镜像也好不到哪去，从源码全量重建工具链是小时级。

<!-- 【事实核查注记·V18】SDK_EXT_TYPE = "minimal" 在离线环境下的精确发作形态——报错文案逐字、发作时机（安装时还是首次 devtool build 时）——以 Scarthgap 5.0 dev-manual 与 populate_sdk_ext.bbclass 行为为准核实回填。正文报错行为示意形态；源码级已确证的部分：minimal 不打 sstate 快照（populate_sdk_ext.bbclass 第 24 行 SDK_INCLUDE_TOOLCHAIN 随型取值、第 541-564 行 minimal 分支只处理 derivative 情形），eSDK 的 DL_DIR 为空（写入 local.conf 的 `DL_DIR = "${TOPDIR}/downloads"`）。 -->

族谱：这是一个**新族——环境假设坑**：发件人假设了接收方的网络环境存在。跟 1.6.2 的代理坑是远亲（那次是构建机自己的网络假设没传递给 BitBake，这次是把网络假设打包发给了别人）。

纪律落纸：发 eSDK 之前先问接收方一句话——"你那边有没有外网/有没有 sstate 镜像服务器？"内网交付，要么发 full（快照在包里，离线自足），要么 minimal 配套架一台 sstate mirror（`SSTATE_MIRRORS` 机制，名字点到即可）。体积从来不该是这份决策的第一考量。

### 14.9 本章小结

白板四件事回顾：

- **14.1 两半结构**：SDK = 主机侧 cross 工具链（x86_64 上跑）+ 目标侧 sysroot（aarch64 素材）；Sysroot 正式引入，chapter 1 的 `sysroots-*` 目录认账；SDK vs eSDK 按接收方分工——只写应用拿 SDK，要改系统拿 eSDK。
- **14.2 生成 SDK**：populate_sdk 正式引入（长在 tiger-image 上的任务）；populate_sdk_base.bbclass 两张 TASK 清单 + 命名模板实物；**poky 搬家认账又一回**——`SDK_NAME`/`SDK_VERSION`/`SDKPATHINSTALL` 三行从 poky.conf 认领进 tiger-distro.conf，文件名从 `oecore-...-toolchain-nodistro.0.sh`（OE-Core 全局兜底、认别人姓）变成 `tiger-distro-dev-...-toolchain-1.0.sh`，`DISTRO_VERSION = "1.0"` 第一次变成文件名；安装 + environment-setup，`cortexa53-tiger-linux` 脚本名与 `aarch64-tiger-linux-` 前缀把 11.2.2 的 TARGET_VENDOR 伏笔二次回收。
- **14.3 定制**：TOOLCHAIN_HOST_TASK / TOOLCHAIN_TARGET_TASK 正式引入与分工；`TOOLCHAIN_TARGET_TASK:append = " mtd-utils-dev mtd-utils-staticdev i2c-tools-dev"` 落进 tiger-image.bb（与 12.2 的 IMAGE_INSTALL 清单呼应；头文件归 -dev、.a 静态库归 -staticdev，两条包名纪律）；坑 1 发作——运行包名构建不拦、dev-pkgs 补装只救得了一半；💡 框"要加先量"+ buildhistory 的 SDK 记账。
- **14.4 eSDK**：populate_sdk_ext / SDK_EXT_TYPE 正式引入；full（默认，含 sstate 快照）vs minimal（体积诱人，把获取推迟到使用时）对照表；eSDK 不能以 root 安装；坑 2 埋雷——达哥事故复盘：minimal 离线发作（演示走 local.conf 会话级，主仓库零改动）。
- **14.5 SDK 自验证**：角色切换扮演应用工程师；hello-tiger 放 `~/workspace/hello-tiger/` 独立目录（开发态/集成态纪律延伸到 SDK 场景，建仓即落初始提交——14.6 的 devtool 要读 HEAD）；Makefile 三段全用环境变量；对照组 qemuarm64 部署跑通（复用 13.3.2 的门与钥匙），PRETTY_NAME 三件套闭环；tiger 组部署挂 C-W29；附录 A 底稿一句。
- **14.6 eSDK 自验证**：devtool 正式上手（chapter 8 点名、13.6 命令露面，两段前史补登）；`devtool add` 本地源码离线收编（externalsrc 机制，bbappend 里 EXTERNALSRC / EXTERNALSRC_BUILD / initial_rev 三行各认一遍）、`devtool build` 分钟级（full 快照兑现）；modify/finish 主菜归 chapter 15；deploy-target 可选加分项挂 V20。
- **14.7 分发与版本化**：版本化锚点双份（文件名 1.0 + dev/prod 变体段）+ sha256 归档；buildhistory/sdk 实物认账（SDKSIZE、host/target 双清单子目录、自动 git 提交）；合规意识——SDK 也是分发物，license 清单缺口挂 V19、完整动作归 chapter 16；💡 框 CI 加 sdk job 一句。
- **14.8 踩坑复盘**：坑 1 认门"构建期不拦、运行时才炸"族（第二次发作）；坑 2 立新族"环境假设坑"（1.6.2 代理坑远亲）。

本章产出清单：

- `meta-tiger` 编辑：`recipes-core/images/tiger-image.bb`（追加 TOOLCHAIN_TARGET_TASK 一行，14.3 交付物）；`conf/distro/tiger-distro.conf`（追加 SDK 命名三件套，14.2 交付物）。
- `meta-tiger` 之外：`~/workspace/sdk/tiger`（装出的 SDK）、`~/workspace/esdk/tiger`（装出的 eSDK）、`~/workspace/hello-tiger`（独立目录，模拟应用组仓库，git 建仓并有初始提交、不打 tag——它不是五个项目仓库的成员）；`tmp/deploy/sdk/` 的安装器与清单是构建产物，不进仓库。

提交并打 tag：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Add SDK toolchain customization and SDK naming for tiger-image"
git tag chapter14
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter14`，它是本章唯一有改动的我方仓库；build 目录 local.conf 的 `SDK_EXT_TYPE = "minimal"` 演示行已删、不进仓库；hello-tiger 模拟应用组仓库不打 tag；poky、meta-arm 与四个开发态仓库本章无改动，不打，沿用惯例。

后续任务清单：

- **task 16 / chapter 15 修上游 bug**：devtool modify/finish 的完整工作流主菜——今天这把刀只出了鞘，下一章开锋；13.6 的 AUH 悬念自该章起陆续收（chapter 16 合规发布固定版本时，这个哨兵仍是情报来源）。
- **task 17 / chapter 16 合规交付**：SDK 的 license 清单缺口（V19）、archiver/SBOM/发布清单——14.7 挂的账到那一章一起收。
- **task 19 / 附录 A 应用开发者 SDK 使用速览**：14.5 的最小闭环是它的底稿——环境变量清单、CMake 工程、调试姿势在那里补齐。

<!-- 【事实核查注记·V17 处置留痕】蓝图 V17 两项已在写作中对照本地 Scarthgap 源码核实并落为确定口径：TOOLCHAIN_HOST_TASK / TOOLCHAIN_TARGET_TASK 默认值（populate_sdk_base.bbclass 第 64-78 行，packagegroup-core-standalone-sdk-target 一族健在）、安装器命名模板（bitbake.conf 第 459-460 行兜底 + poky.conf 第 5-6/25-26 行 DISTRO 侧认领）。R1 审校修正：未认领时版本段为 nodistro.0 全局兜底（bitbake.conf:832 → defaultsetup.conf → default-distrovars.inc:45），非空值——正文已按此重写。残留待核实项并入 C-W27：真机/真仓库构建出的安装器文件名逐字形态、体积与耗时回填。 -->

<!-- 【事实核查注记·V19】SDK 构建的 license 清单产物位置：源码级核查结论为"SDK 无对应产物"——license_image.bbclass 的 license_create_manifest 只挂镜像 rootfs 后处理通道（第 297 行 ROOTFS_POSTUNINSTALL_COMMAND），SDK 装配不经该通道；SDK 自带的 .target.manifest/.host.manifest（populate_sdk_base.bbclass 第 120-123 行）为版本级包清单。该负向结论已经 reviewer-fact 终核确认（R1）；chapter 16 写作时按此落笔。 -->

<!-- 【事实核查注记·V20】devtool deploy-target 的端到端实测（经 slirp 2222 转发部署到对照组），决定 14.6.3 可选加分项的最终写法，待实测回填。端口转发本身已由 runqemu 源码证实自动存在（slirp 默认 hostfwd=tcp:127.0.0.1:2222-:22），不再是未知项——挂账范围 R1 起收窄为端到端实测。 -->

---

**延伸阅读**

1. Yocto 开发任务手册（SDK / eSDK / devtool 章节）：https://docs.yoctoproject.org/5.0/dev-manual/index.html
2. Yocto 变量术语表（TOOLCHAIN_HOST_TASK、TOOLCHAIN_TARGET_TASK、SDK_EXT_TYPE、SDKIMAGE_FEATURES、SDK_NAME 等条目）：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
3. Yocto 参考手册 class 章节（populate_sdk_base、populate_sdk_ext、toolchain-scripts 等类）：https://docs.yoctoproject.org/5.0/ref-manual/classes.html
