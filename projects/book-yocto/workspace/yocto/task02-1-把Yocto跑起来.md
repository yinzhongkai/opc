# 1 把 Yocto 跑起来

周一上午，项目启动会后的第一个工作日。阿凯拆开工牌套，Ubuntu 24.04 LTS 的开发机已经开好，终端里光标在闪。他打开本子，最上面一行还是昨天写下的——"先把 Yocto 项目跑起来"。

老周端着一杯咖啡走过来，没坐下，直接开口。

"开始吧。先别管 tiger，把官方 Poky 跑通。Scarthgap 分支，目标 `qemuarm64`，编一个能开机进 shell 的最小镜像。"

"我需要先装什么？"阿凯抬头问。

"自己查 Yocto 项目 Quick Build。查完把命令发我看看。"老周把杯子放到桌上，"装完先别急着 clone，检查三件事：你是谁、硬盘还剩多少、locale 对不对。"

阿凯"嗯"了一声，心里已经开始列清单。老周转身走了一步，又补了一句。

"一周内跑通。卡住了先自己查、自己试，真过不去再来。这一步跑不通，后面所有事都干不了。"

## 1.1 准备构建环境

阿凯查完 Quick Build，发现 Yocto 项目对宿主机的依赖并不复杂，但少一个包就可能让构建在几小时后失败。他先把系统包一口气装上。

### 1.1.1 安装系统依赖

```bash
# 安装 Yocto 项目 Scarthgap 5.0 在 Ubuntu 24.04 上的必需系统包
# 本命令直接在终端执行，无需写入文件
sudo apt update
sudo apt install -y gawk wget git diffstat unzip texinfo chrpath \
    build-essential python3 python3-pip python3-pexpect python3-jinja2 \
    python3-git xz-utils debianutils iputils-ping cpio file locales \
    zstd lz4 socat libacl1 python3-subunit
```

> **💡 提示**：如果你的 apt 源里某些包名不同，按提示调整。核心不能缺的是 `gawk`、`wget`、`git`、`diffstat`、`unzip`、`texinfo`、`chrpath`、`python3`、`gcc`、`make`、`xz-utils`、`libacl1`。这些东西看起来普通，但后续构建过程的 fetch、patch、compile 阶段都会直接调用它们。列表里还加了 `python3-pip`，虽然官方 Quick Build 指南未将其列为必需，但后续排查或安装辅助脚本时经常用到。

装完之后，阿凯先确认自己不是 root。

```bash
# 确认当前是普通用户（Yocto 项目禁止以 root 身份执行构建命令）
whoami
```

输出：

```text
akai
```

> **🔥 重要**：构建工具会主动拒绝 root 用户执行；如果你看到 `Do not use Bitbake as root` 之类的报错，立刻退出 root 账户，用普通用户重新来。

### 1.1.2 检查磁盘、内存与 locale

老周说的"三件事"，阿凯一条条检查。

```bash
# 查看当前用户 home 目录可用空间
df -h ~
```

输出：

```text
文件系统        容量  已用  可用 已用% 挂载点
/dev/sda2       196G   24G  163G   13% /home
```

```bash
# 查看可用内存
free -h
```

输出：

```text
              总计       已用       空闲     共享    缓冲/缓存    可用
内存：        15Gi       3.2Gi      8.1Gi     412Mi       4.3Gi     11Gi
交换：        2.0Gi        0B       2.0Gi
```

```bash
# 启用并生成 UTF-8 locale（若 /etc/locale.gen 中已启用，可跳过 sed）
sudo sed -i 's/^# en_US.UTF-8/en_US.UTF-8/' /etc/locale.gen
sudo locale-gen en_US.UTF-8
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
```

> **⚠️ 注意**：Yocto 项目的构建脚本依赖 `en_US.UTF-8` 解析一些带非 ASCII 字符的文件。如果 `locale` 输出里找不到 `en_US.UTF-8`，后续某些软件包源码的解包阶段可能会报编码错误。建议把上面两条 `export` 写进 `~/.bashrc`，然后 `source ~/.bashrc`。

阿凯把 locale 设好，又把 `df` 的结果拍了张图发给老周。老周只回了一句："200G 还行，别低于 100G。"

### 1.1.3 网络代理配置（按需）

公司内网需要代理才能访问外网，阿凯先把 Shell 环境配好。

```bash
# 根据你所在网络环境填写代理地址
export http_proxy=http://<your-proxy>:<port>
export https_proxy=http://<your-proxy>:<port>
export no_proxy=localhost,127.0.0.1,.local

# 测试能否到达 Yocto 项目上游仓库
# 若命令无输出且返回 0，表示网络可达
wget -q --spider https://git.yoctoproject.org
```

输出：

```text
# ...（无输出表示可达）
```

> **⚠️ 注意**：这里只是让当前 Shell 能访问外网。构建工具（BitBake）的 fetcher 跑在子进程里，不一定会自动继承 `http_proxy`。如果后面下载源码阶段大量报错，请看本章末尾的"踩坑 2：代理未传入 BitBake"。

## 1.2 Clone Poky 并了解目录结构

系统依赖装完，阿凯开始 clone 上游 Poky。

### 1.2.1 克隆 Poky

```bash
# 创建项目根目录并 clone Poky 的 scarthgap 分支
mkdir -p ~/workspace
cd ~/workspace
git clone -b scarthgap git://git.yoctoproject.org/poky.git
```

> **💡 提示**：如果公司网络不允许 `git://` 协议，可以把地址换成 `https://git.yoctoproject.org/poky.git`。两种协议最终拿到的代码完全一致。

克隆完成后，阿凯先确认分支，再看一眼顶层目录。

```bash
# 确认当前分支
cd ~/workspace/poky
git branch
```

输出：

```text
* scarthgap
```

```bash
# 查看 Poky 顶层目录
ls -1
```

输出：

```text
bitbake
meta
meta-poky
meta-selftest
meta-skeleton
meta-yocto-bsp
oe-init-build-env
README.OE-Core.md
README.qemu.md
scripts
```

### 1.2.2 Poky 是什么

阿凯看着这些目录，想起老周说"先跑通官方 Poky"。他还不清楚 Poky 到底指哪一部分，于是先在心里画个框。

**Poky** 是 Yocto 项目的参考发行版，它把构建执行引擎、核心元数据和一套默认配置打包在一起。你可以把它理解为 Yocto 项目给新人准备的一个"最小可用系统"：不是只给一个空框架，而是把链条上已经验证过的组件串好，让你跑一遍就能看到完整的构建结果。

Poky 里最关键的三样东西是：

- **BitBake**：Yocto 项目的构建执行引擎，负责解析配方并调度任务。
- **OE-Core**：OpenEmbedded-Core，Poky 中的核心元数据层，对应 `meta/` 目录。
- 发行版与 BSP 元数据：`meta-poky/` 和 `meta-yocto-bsp/`。

老周端着咖啡又晃过来，扫了眼屏幕："哪些目录最重要，你先自己 `ls` 一遍，再来问我。"

### 1.2.3 目录结构探索

阿凯按老周说的，逐个目录看过去。

```bash
# BitBake 命令与相关子命令
ls bitbake/bin/
```

输出：

```text
bitbake
bitbake-diffsigs
bitbake-layers
bitbake-prserv
bitbake-selftest
bitbake-server
# ... (省略)
```

```bash
# OE-Core 顶层结构
ls meta/
```

输出：

```text
classes
conf
lib
recipes-bsp
recipes-connectivity
recipes-core
recipes-devtools
recipes-extended
recipes-graphics
# ... (省略)
```

```bash
# Poky 发行版配置
ls meta-poky/
```

输出：

```text
conf
recipes-core
# ... (省略)
```

```bash
# Yocto 官方参考 BSP 层，包含 genericarm64 等参考板；
# qemuarm64 等 QEMU 机器由 OE-Core 的 meta/conf/machine/ 提供
ls meta-yocto-bsp/
```

输出：

```text
conf
recipes-bsp
recipes-core
recipes-kernel
# ... (省略)
```

```bash
# 辅助脚本，runqemu 就在里面
ls scripts/
```

输出：

```text
# ... (省略)
runqemu
runqemu-extract-sdk
# ... (省略)
```

看了一圈，阿凯回来汇报。

"`bitbake/` 下面是构建引擎；`meta/` 是 OE-Core；`meta-poky/` 像发行版配置；`meta-yocto-bsp/` 是官方参考 BSP；`scripts/` 里放了 `runqemu` 这些脚本。"

老周点点头："对。你把 `meta/`、`meta-poky/`、`meta-yocto-bsp/` 这三个都叫 **层（Layer）**——组织元数据的目录结构，可以叠加复用。本章只认识它们的目录组织，具体机制后面再讲。"

"还有，"老周指了指屏幕里那些 `recipes-*` 目录，"那些 `.bb` 文件叫 **配方（Recipe）**，描述怎么获取、编译、安装一个软件包。本章只混个脸熟，配方语法留给后面。"

## 1.3 初始化构建环境

目录看明白了，下一步是建立构建目录。Poky 提供了一个入口脚本，一步就能把环境搭好。

### 1.3.1 source oe-init-build-env

```bash
# 进入 poky 目录，初始化构建环境
# 参数 ../build 表示在 poky 同级目录创建 build 目录
cd ~/workspace/poky
source oe-init-build-env ~/workspace/build/
```

执行后，当前 Shell 会自动切到 `~/workspace/build` 目录，并且 `PATH` 里已经能找到 `bitbake` 等命令。这个脚本叫 **构建环境初始化脚本（oe-init-build-env）**，是 Poky 提供的 Shell 脚本，初始化 BitBake 构建环境并创建 `conf/` 目录。

> **🔥 重要**：`oe-init-build-env` 必须用 `source` 或 `.` 执行，不能 `./oe-init-build-env`。因为它要修改当前 Shell 的 `PATH`、`BUILDDIR` 等环境变量，用子进程执行的话这些改动不会传回来。

阿凯验证了一下环境变化。

```bash
# 确认构建目录路径
echo $BUILDDIR
```

输出：

```text
/home/akai/workspace/build
```

```bash
# 确认 bitbake 已在 PATH 中
which bitbake
```

输出：

```text
/home/akai/workspace/poky/bitbake/bin/bitbake
```

```bash
# 查看生成的配置目录
ls $BUILDDIR/conf/
```

输出：

```text
bblayers.conf
local.conf
templateconf.cfg
```

老周指着 `conf/` 里的两个文件："这两个文件你以后会天天改。`local.conf` 是 **本地配置文件（local.conf）**，构建目录下的用户级配置入口，优先级高于层中的默认值；`bblayers.conf` 是 **层配置文件（bblayers.conf）**，声明 BitBake 加载哪些层及其搜索顺序。"

### 1.3.2 配置 local.conf

阿凯打开 `local.conf`，发现默认的 `MACHINE` 是 `qemux86-64`。他想起 tiger 是 ARM 的，但本章目标是 `qemuarm64`，于是跑来问老周。

"`MACHINE` 要不要改成 `tiger-aarch64`？"

老周反问："这一章我们跑什么机器？"

"官方 Poky 的 `qemuarm64`……"

"那你现在知道该怎么设了？"

阿凯回去改配置。**MACHINE 变量（MACHINE）** 是 BitBake 变量，设置目标机器名称，写在 `local.conf` 里。**Machine 配置（Machine）** 则定义目标硬件平台特征。本章只把 `MACHINE` 设为现成的 `qemuarm64`，不写新的 Machine 配置。

老周让他在原有 `local.conf` 末尾追加或修改以下几项。这里涉及的 BitBake 变量先在正文里过一遍：

- **下载目录（DL_DIR）** 存放从网络拉取的源码包。
- **Sstate 缓存（Sstate）** 的落盘位置由 `SSTATE_DIR` 变量指定。
- **临时目录（TMPDIR）** 是构建过程中的临时工作目录。
- `BB_NUMBER_THREADS` 控制 BitBake 自身并行调度的任务数。
- `PARALLEL_MAKE` 控制 `make` 编译时的并行 job 数。
- `EXTRA_IMAGE_FEATURES` 是控制镜像级特性的 BitBake 变量。
- `debug-tweaks` 是 **镜像特性（IMAGE_FEATURES）** 的一项，启用空 root 密码等调试便利。

```bitbake
# 文件路径：~/workspace/build/conf/local.conf
# 在原有 local.conf 末尾追加或修改以下内容
# （MACHINE 行把默认的 qemux86-64 改为 qemuarm64，其余变量若已存在则取消注释并修改）

MACHINE = "qemuarm64"

# 保留默认的 debug-tweaks，让 core-image-minimal 允许空密码 root 登录
EXTRA_IMAGE_FEATURES ?= "debug-tweaks"

# 下载缓存目录：${TOPDIR} 指构建目录顶层路径，所有从网络拉取的源码包都存在这里
DL_DIR = "${TOPDIR}/downloads"

# Sstate 缓存目录：共享状态缓存的落盘位置，具体作用见 1.5.2
SSTATE_DIR = "${TOPDIR}/sstate-cache"

# 构建临时目录：所有配方的解压、编译、安装、打包都在这里进行
TMPDIR = "${TOPDIR}/tmp"

# BitBake 自身并行调度的任务数，根据你的 CPU 核心数调整
BB_NUMBER_THREADS = "8"

# make 编译时的并行 job 数，根据你的 CPU 核心数调整
PARALLEL_MAKE = "-j 8"
```

保留默认的 **调试调整（debug-tweaks）** 特性，可以让 `core-image-minimal` 允许空密码 root 登录，方便在 QEMU 里验证。后续进入 **Distro 配置（DISTRO）** 阶段时，我们会再谈如何关闭这类调试特性。

> **💡 提示**：`BB_NUMBER_THREADS` 和 `PARALLEL_MAKE` 建议设成你 CPU 物理核心数或稍多一点。阿凯的机器是 8 核，所以都写 8。如果你的机器是 4 核，可以改成 4；内存不足时宁可设小一点，否则并行编译会把机器卡死。

> **⚠️ 注意**：`DL_DIR`、`SSTATE_DIR`、`TMPDIR` 默认就在 `${TOPDIR}` 下。显式写出来有两个好处：一是以后想把这些目录挂到外部大容量分区时，只改这几个变量；二是让你对 build 目录里那几个大文件夹心里有数。

### 1.3.3 配置 bblayers.conf

`local.conf` 改完，阿凯又看 `bblayers.conf`。`oe-init-build-env` 已经自动生成如下内容，他只需要核对路径是否与自己的工作目录一致。

```bitbake
# 文件路径：~/workspace/build/conf/bblayers.conf
# oe-init-build-env 已自动生成如下内容
# 写入配置文件时，请把 /home/<your-username> 替换为你主机的实际绝对路径
BBLAYERS ?= " \
  /home/<your-username>/workspace/poky/meta \
  /home/<your-username>/workspace/poky/meta-poky \
  /home/<your-username>/workspace/poky/meta-yocto-bsp \
  "
```

**层列表变量（BBLAYERS）** 声明当前构建加载了哪些层及其搜索顺序。本章只用到 Poky 默认的三个层：`meta`（OE-Core）、`meta-poky`（Poky 发行版配置）、`meta-yocto-bsp`（官方 BSP）。

阿凯用 **层管理命令（bitbake-layers）** 验证了一下。`bitbake-layers` 是 BitBake 子命令，用于添加、移除、验证层注册。

```bash
# 查看当前加载的层列表
bitbake-layers show-layers
```

输出：

```text
layer                 path                                      priority
==================================================================
meta                  /home/<your-username>/workspace/poky/meta            5
meta-poky             /home/<your-username>/workspace/poky/meta-poky       5
meta-yocto-bsp        /home/<your-username>/workspace/poky/meta-yocto-bsp  5
```

> **💡 提示**：实际输出会显示为你主机的真实绝对路径（如 `/home/akai/workspace/poky/meta`），这里用 `<your-username>` 占位。

老周看了一眼输出："三个层，够用。本章先不讲层优先级和冲突，那是 chapter 3 的事。"

## 1.4 第一次 BitBake

环境搭好了，阿凯终于要开始构建。

### 1.4.1 启动构建

```bash
# 构建 qemuarm64 的最小镜像
# 首次构建约需 1-3 小时，视网络与 CPU 而定
bitbake core-image-minimal
```

**镜像（Image）** 是 Yocto 项目构建输出的根文件系统镜像。**核心最小镜像（core-image-minimal）** 是 Poky 提供的最小镜像配方，仅包含启动到 shell 所需的基础包，常被当作自定义镜像的起点。

> **🔥 重要**：首次构建通常会执行 4000 多个任务，从下载源码、打补丁、配置、编译、打包，一直到组装根文件系统。不要因为屏幕长时间只刷任务名就以为卡死了；但如果某一行报错后任务不再推进，那才是真正卡住。

### 1.4.2 构建过程解读

构建开始后，终端会滚动显示当前执行的任务。BitBake 大致按这个顺序推进：

```text
do_fetch      # 从网络或本地缓存获取源码包
do_unpack     # 解压源码
do_patch      # 打补丁
do_configure  # 运行配置脚本
do_compile    # 编译
do_install    # 安装到临时目录
do_package    # 打成二进制包
do_rootfs     # 组装成根文件系统镜像
```

这是一般软件包配方的典型阶段。`core-image-minimal` 这类镜像配方在镜像配方的 class 文件（`image.bbclass`，class 机制后面会专门讲）里把 `do_fetch` 到 `do_package` 都标记为 `noexec`（即不实际执行），概念上仍沿这条流水线组装，真正执行的是 `do_rootfs`、`do_image` 等镜像相关任务。

阿凯看了一会儿，发现有很多包名后面带着 `:native`，比如 `gcc-native`、`python3-native`。他不明白为什么要在自己的 x86 机器上编译这么多东西。

> **💡 提示**：这些带 `:native` 的包叫 **本机配方（Native recipe）**，它们编译出来的二进制程序在构建主机上运行，而不是目标板。Yocto 项目需要先在 host 上搭建一套完整的编译工具链和辅助程序，再用它们去交叉编译目标板的程序。所以首次构建会比后续慢得多—— host 工具链只编一次，后面有 Sstate 缓存复用。

### 1.4.3 构建成功确认与产物查看

几个小时后，终端不再滚动，阿凯看到这样一行：

```text
NOTE: Tasks Summary: Attempted 4059 tasks of which 0 didn't need to be rerun and all succeeded.
```

这就是构建成功的标志。阿凯松了口气，赶紧去产物目录看结果。构建产物最终落在 `tmp/deploy/images/<MACHINE>/` 下。`$BUILDDIR/tmp/deploy/images/qemuarm64/` 就是 **Deploy 目录（Deploy Directory）**，存放最终镜像、内核等构建产物。

```bash
# 查看 qemuarm64 镜像产物
ls $BUILDDIR/tmp/deploy/images/qemuarm64/
```

输出：

```text
Image
core-image-minimal-qemuarm64.rootfs-<时间戳>.ext4
# ... (省略)
core-image-minimal-qemuarm64.rootfs-<时间戳>.qemuboot.conf
core-image-minimal-qemuarm64.rootfs-<时间戳>.manifest
# ... (省略)
```

> **💡 提示**：文件名里的 `<时间戳>` 会随构建时间变化，不用跟书里完全一样。关键文件是 `core-image-minimal-qemuarm64.rootfs-<时间戳>.ext4`（根文件系统镜像）、`Image`（Linux 内核镜像）、`.qemuboot.conf`（runqemu 用的启动配置）。

## 1.5 启动镜像与看懂 build 目录

镜像已经生成，但它真的能启动吗？阿凯决定立刻验证。

### 1.5.1 用 runqemu 启动

```bash
# 在终端里以串口方式启动 QEMU
# nographic 表示不使用图形窗口，输出直接落到当前终端
# slirp 表示使用用户态网络，不需要 sudo 配置 TAP 网桥
runqemu qemuarm64 nographic slirp
```

**runqemu** 是 Yocto 项目提供的用 QEMU 启动构建镜像的辅助脚本，它会根据 `MACHINE` 自动组装 QEMU 命令行参数，并挑选 `tmp/deploy/images/<MACHINE>/` 下最新的镜像启动。

> **💡 提示**：某些环境里也可以显式指定镜像名：`runqemu qemuarm64 core-image-minimal nographic slirp`。如果这条命令报 `IMAGE_LINK_NAME` 错误，就改用上面只指定机器的写法。

片刻后，串口输出停下来，出现登录提示：

```text
# ... (省略大量启动日志)

Poky (Yocto Project Reference Distro) 5.0.18 qemuarm64 /dev/ttyAMA0

qemuarm64 login:
```

阿凯输入 `root`，直接进了 shell——`core-image-minimal` 默认允许空密码登录 root，方便调试。登录时**不需要密码**，输入 `root` 后直接按回车即可。

```text
qemuarm64 login: root
root@qemuarm64:~#
```

他在镜像里做了几项基本验证。

```bash
# 查看内核与机器信息（具体小版本号随 Scarthgap 点版本更新而变化，以本地输出为准）
uname -a
```

输出：

```text
Linux qemuarm64 6.6.127-yocto-standard #1 SMP PREEMPT ... aarch64 GNU/Linux
```

```bash
# 查看 CPU 信息
cat /proc/cpuinfo
```

输出：

```text
processor       : 0
BogoMIPS        : 125.00
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid
CPU implementer : 0x41
CPU architecture: 8
CPU variant     : 0x1
CPU part        : 0xd07
CPU revision    : 0

# ... (省略 processor 1-3)
```

```bash
# 查看内存
free
```

输出：

```text
              total        used        free      shared  buff/cache   available
Mem:         232996       26412      191728        4132       14856      195760
Swap:             0           0           0
```

> **💡 提示**：`core-image-minimal` 里的 `free` 来自 busybox，输出单位是 **KiB**。本章未额外配置 `QB_MEM`，因此 QEMU 默认使用 256Mi 内存；总内存约 227Mi（232996 KiB），比 256Mi 略小是因为内核和固件预留了一部分地址空间。如果你用的是完整 procps-ng 的 `free -h`，则会显示 `234Mi` 这样的带单位格式，数值含义相同。

```bash
# 查看根文件系统挂载情况
df -h
```

输出：

```text
Filesystem                Size      Used Available Use% Mounted on
/dev/root                19.3M     12.9M      4.9M  72% /
devtmpfs                111.5M         0    111.5M   0% /dev
tmpfs                   113.8M     72.0K    113.7M   0% /run
tmpfs                   113.8M     60.0K    113.7M   0% /var/volatile
```

> **⚠️ 注意**：`core-image-minimal` 默认把 `/dev`、`/run`、`/var/volatile` 挂载为 tmpfs/devtmpfs，容量约为 guest 物理内存的一半（本章默认 256Mi，所以看到约 111–114Mi）。`/dev/root` 的大小随镜像配方和版本变化，以本地输出为准。

```bash
# 确认 busybox 等基础命令存在
ls /bin/
```

输出：

```text
# ... (省略)
busybox
cat
cp
# ... (省略)
```

验证完，阿凯输入 `poweroff` 关闭系统。

```bash
# 在 guest 内正常关闭系统
poweroff
```

执行 `poweroff` 后，guest 会 halt，但 QEMU 进程不一定自动退出。在 `-nographic` 模式下，先按 `Ctrl+a`，松开后按 `x`，即可退出 QEMU 回到宿主机提示符。如果 `Ctrl+a x` 没有反应，可以先按 `Ctrl+a` 松开后按 `c` 进入 QEMU monitor，再输入 `quit`。

### 1.5.2 看懂 build 目录

老周看阿凯跑通了，没夸他，只问了一句："`build/` 下面那几个大文件夹，`downloads/` 和 `sstate-cache/` 是干什么的？"

阿凯又跑回去翻目录。

```bash
# 查看 build 顶层结构
ls -1 $BUILDDIR
```

输出：

```text
bitbake-cookerdaemon.log
bitbake.lock
cache
conf
downloads
sstate-cache
tmp
```

```bash
# 下载缓存目录里存放的是从网络拉取的源码包，版本号会随上游更新而变化
ls $BUILDDIR/downloads/ | head -n 10
```

输出：

```text
busybox-<版本号>.tar.bz2
curl-<版本号>.tar.xz
gcc-<版本号>.tar.xz
# ... (省略)
```

```bash
# 临时构建目录里内容最多
ls $BUILDDIR/tmp/
```

输出：

```text
deploy
hosttools
stamps
sysroots-components
sysroots-uninative
work
work-shared
# ... (省略)
```

其余如 `hosttools`、`work-shared` 等目录本章暂不深入，后续涉及交叉编译和共享源码时会再解释。

阿凯整理了一下，给老周汇报。

"`downloads/` 是下载缓存，`sstate-cache/` 是构建中间产物的缓存，`tmp/` 是真正干活的地方。`tmp/deploy/images/<MACHINE>/` 放最终镜像，`tmp/work/` 放每个配方的解压、编译、安装目录，`tmp/stamps/` 是 BitBake 记录任务完成状态的标记文件。"

老周点点头："`sstate-cache/` 里的东西叫 **Sstate 缓存（Sstate）**，缓存构建中间产物以加速后续构建。你下次改一个配置重编，很多东西不用重新编译，直接从这儿拿。`tmp/sysroots-components/` 是交叉编译用的系统根组件，本章只混个脸熟，后面讲交叉编译时再细说。"

## 1.6 踩坑实录

老周说得没错：跑通环境本身就是道坎。阿凯在这周踩了两个最典型的坑。

### 1.6.1 踩坑 1：磁盘空间不足导致构建失败

阿凯第一次跑 `bitbake core-image-minimal` 时，构建快到一小时，终端突然刷红。他盯着最后几行发愣。

输出（你可能遇到的错误）：

```text
# ... (省略)
write error: No space left on device
# ... (省略)
ERROR: Task (...) failed with exit code '1'
NOTE: Tasks Summary: Attempted 2156 tasks of which 0 didn't need to be rerun and 1 failed.
```

"编译错误？"老周凑过来看。

"不像，最后说 `No space left on device`……"阿凯往上翻日志。

"那就是磁盘满了。你 `df -h ~` 看看。"老周说。

```bash
# 发现 home 目录可用空间只剩几百 MB
df -h ~
```

输出：

```text
文件系统        容量  已用  可用 已用% 挂载点
/dev/sda2        50G   49G  256M  99% /home
```

老周叹了口气："你光看到硬盘还有 30G 就觉得够？Yocto 项目首次构建要下载源码、解包、编译、打包，完整 `core-image-minimal` 轻轻松松吃掉 40-60G。我的习惯是留 100G 以上。"

修复方法分两步：先清理失败的构建，再腾出空间或把缓存目录迁到外部大容量分区。

```bash
# 进入构建目录，只删除 tmp，保留 downloads 和 sstate-cache 以减少重新下载
cd ~/workspace/build
rm -rf tmp/
```

如果 home 分区本身不够大，可以把 `DL_DIR`、`SSTATE_DIR`、`TMPDIR` 改到外挂分区，比如 `/data/yocto-downloads`、`/data/yocto-sstate`、`/data/yocto-tmp`。改完后重跑：

```bash
# 清理后重新构建最小镜像
bitbake core-image-minimal
```

> **🔥 重要**：不要把 `rm -rf tmp/` 当成日常清理手段。失败时清 `tmp/` 是为了去掉不完整的状态；正常构建后，`tmp/` 里的 sstate 签名和 deploy 产物对你后续排查问题很有用。

### 1.6.2 踩坑 2：代理未传入 BitBake 导致 fetch 失败

第二次构建，阿凯在公司内网跑。`do_fetch` 阶段大量报错：

输出（你可能遇到的错误）：

```text
# ... (省略)
Fetcher failure for URL: 'https://example.com/some-package-1.0.tar.gz'. Unable to fetch URL from any source.
# ... (省略)
NOTE: Tasks Summary: Attempted 123 tasks of which 0 didn't need to be rerun and 45 failed.
```

"浏览器能上网啊。"阿凯嘀咕。

老周凑过来："BitBake 的 fetcher 跑在子进程里，你 Shell 里的 `http_proxy` 不一定传得进去。先确认你在 `source oe-init-build-env` 之前已经 export 了代理变量。"

阿凯检查了一下，发现他虽然设了 `http_proxy`，但是在 `source oe-init-build-env` 之后补的，当前 Shell 的环境变量没被 BitBake 继承。修复方法是在当前 Shell 中先 export 代理变量，再 source 构建环境脚本：

```bash
# 在 source oe-init-build-env 之前执行，让 BitBake 子进程继承代理变量
export http_proxy=http://<your-proxy>:<port>
export https_proxy=http://<your-proxy>:<port>
export no_proxy="localhost,127.0.0.1"
cd ~/workspace/poky
source oe-init-build-env ../build
```

**环境变量传递白名单（BB_ENV_PASSTHROUGH_ADDITIONS）** 是 BitBake 读取的环境变量白名单，控制哪些 Shell 环境变量会传给 BitBake 子进程。`oe-init-build-env` 已经把 `http_proxy`、`https_proxy`、`no_proxy` 等代理变量加入白名单；如果还是失败，再检查 `BB_ENV_PASSTHROUGH_ADDITIONS` 是否在 Shell 中 export。

然后他重新验证网络：

```bash
# 验证代理生效后能否访问 Yocto 项目上游仓库
wget -q --spider https://git.yoctoproject.org
```

确认连通后，再次构建：

```bash
# 代理生效后重新构建最小镜像
bitbake core-image-minimal
```

> **💡 提示**：如果你不想每次都在 Shell 里 export 代理，也可以直接在 `local.conf` 里写死 `http_proxy = "http://..."`。但这样会把个人网络配置提交到团队共享的环境里，不推荐在协作仓库里这么做。

## 1.7 本章小结

阿凯跑通了人生中第一个 Yocto 项目构建。他回到工位，在本子上把完成的事打勾：

- 在 Ubuntu 24.04 上装好了 Yocto 项目 Scarthgap 5.0 的构建依赖。
- 从上游 clone 了 Poky，认清了 `bitbake/`、`meta/`、`meta-poky/`、`meta-yocto-bsp/`、`scripts/` 等关键目录。
- 用 `oe-init-build-env` 初始化构建目录，配置了 `local.conf` 和 `bblayers.conf`。
- 成功运行 `bitbake core-image-minimal`，为 `qemuarm64` 构建出最小镜像。
- 用 `runqemu` 启动镜像并完成了基本验证。
- 看懂了 `build/` 目录里 `conf/`、`downloads/`、`sstate-cache/`、`tmp/` 这四大区域的作用。

本章严格遵守边界：只跑通官方 Poky 的 `qemuarm64` `core-image-minimal`，没有碰 tiger 硬件、没有创建 `meta-tiger`、没有写自定义 MACHINE。这些工作从下一章开始。

最后，阿凯在 Poky 仓库里打了一个 tag，标记本章结束时的状态。

```bash
# 在 poky 仓库标记本章终点
cd ~/workspace/poky
git tag chapter1
```

> **💡 提示**：如果你在后续章节遇到构建问题，可以随时 `git checkout chapter1` 回到本章结束时的代码状态，然后对比差异排查。

## 1.8 延伸阅读

本章涉及的官方文档链接都放在这里，正文中不再出现 URL：

- Yocto Project 5.0 Quick Build 指南：https://docs.yoctoproject.org/5.0/brief-yoctoprojectqs/index.html
- Yocto Project 5.0 参考手册 - 变量术语表：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
- Yocto Project 5.0 参考手册 - 构建目录结构：https://docs.yoctoproject.org/5.0/ref-manual/structure.html
- BitBake 2.8 用户手册：https://docs.yoctoproject.org/bitbake/2.8/
