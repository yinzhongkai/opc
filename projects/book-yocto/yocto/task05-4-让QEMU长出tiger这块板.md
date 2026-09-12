## 4 让 QEMU 长出 tiger 这块板

周四上午，阿凯工位。昨天刚打完 `chapter3` 的 tag，`meta-tiger` 还是个空壳。老周发来一条消息，只有一个内部 Git 仓库地址和两句话：

"这是另一个组同事写的 QEMU patch，tiger 这块虚拟板他们已经能跑了。你的任务不是开发 QEMU，是把它**集成进 Yocto**——让 bitbake 构建出来的 QEMU 能识别 `-M tiger`。"

给 QEMU 动手术——阿凯想起昨天收工时老周撂下的那句话，原来指的就是这个。他把地址抄下来，追问了一句："`tiger-aarch64` 和 `-M tiger` 是什么关系？"

"命名约定，先记住。"老周说，"Yocto 侧的 MACHINE 叫 `tiger-aarch64`，QEMU 侧的 machine 类型叫 `tiger`，中间的映射由 runqemu 负责。以后你写 MACHINE 配置时会亲手把这个映射接上。"

"集成具体做什么？"

"四步。"老周伸出手指，"导出 patch、写 bbappend、喂给 SRC_URI、构建验证。"

"那 runqemu 呢？集成完了 `runqemu tiger-aarch64` 就能跑了吧？"

"问到点子上了。"老周说，"那是第五步——也是本章你只能做一半的半步。机制今天能讲透，真正跑通还缺两块拼图，到时候你自己会数出来。"

阿凯翻开本子准备开工，老周临走补了一句："patch 这东西，顺序和版本，总有一个要咬你一口。别问，到时候你就知道了。"

### 4.1 从 qemu-tiger 导出 patch

#### 4.1.1 看看同事交付了什么

阿凯先把仓库 clone 下来。这是序章里说的**开发态**仓库——功能开发在那里完成，Yocto 只负责集成。

```bash
# clone 同事的 tiger QEMU 开发仓库（内部 Git 服务器，地址以实际为准）
cd ~/workspace
git clone git@<internal-git-server>:bsp/qemu-tiger.git

# 看提交历史，找 tiger machine 相关的提交
cd ~/workspace/qemu-tiger
git log --oneline
```

输出（提交哈希与数量以本地实际输出为准）：

```text
5c08e90 hw/arm/tiger: Register default configs and docs
7e21b4d hw/arm/tiger: Wire up onboard devices (PL011/PL031/m25p80/at24)
a3f9c12 hw/arm: Add tiger machine skeleton
b04d2f1 Merge tag 'v8.2.7' into tiger-dev
# ... (省略上游 QEMU 的大量提交)
```

三个提交正好对应三件事：machine 骨架、板载外设接入、构建系统与文档注册。阿凯扫了一眼改动规模。

```bash
# 看看从 v8.2.7 到 HEAD（三个 tiger 提交）一共改了多少代码
git diff --stat v8.2.7..HEAD
```

输出（以本地实际输出为准）：

```text
 hw/arm/Kconfig     |  16 +
 hw/arm/meson.build |   2 +
 hw/arm/tiger.c     | 687 +++++++++++++++++++++++++++++++++++++
 3 files changed, 705 insertions(+)
```

`tiger.c` 新增了近七百行 C。"这些我要读懂吗？"阿凯问。

"不用。"老周头也没抬，"machine 实现是另一组的活，他们负责维护。你用 `git diff --stat` 扫一眼规模是应该的——知道集成的分量，但别陷进去。你的战场在 Yocto 这边。"

#### 4.1.2 确认 base：patch 的"出生证明"

导出 patch 之前，还有一件事必须先做：确认这批 patch 是基于哪个 QEMU 版本写的。patch 不是独立于上下文存在的——它记录的是"从某个版本的源码改成另一个样子"，这个"某个版本"就是它的 base。base 和 Yocto 要打的源码版本对不上，后面全是麻烦。

Poky 侧的 QEMU 版本写在配方文件名里：`qemu-system-native_8.2.7.bb`，即 **配方版本号（PV）** 为 8.2.7。`PV` 是 BitBake 的内置变量，取值就来自配方文件名的版本部分，不用去文件里翻。

```bash
# 确认 Poky 锁定的 QEMU 版本：看配方文件名即可
ls ~/workspace/poky/meta/recipes-devtools/qemu/qemu*_8.2.7.bb
```

输出：

```text
/home/<your-username>/workspace/poky/meta/recipes-devtools/qemu/qemu_8.2.7.bb
/home/<your-username>/workspace/poky/meta/recipes-devtools/qemu/qemu-native_8.2.7.bb
/home/<your-username>/workspace/poky/meta/recipes-devtools/qemu/qemu-system-native_8.2.7.bb
```

再看 qemu-tiger 仓库的 base。刚才 `git log` 里那条 `Merge tag 'v8.2.7'` 已经给了提示——同事的 tiger 开发分支是从上游 QEMU 的 v8.2.7 标签拉出来的。用命令确认三个 tiger 提交的最近上游标签。

```bash
# 确认 tiger 提交的 base 是上游 v8.2.7
git describe --tags a3f9c12
```

输出（以本地实际输出为准）：

```text
v8.2.7-1-ga3f9c12
```

含义：`a3f9c12` 是从 `v8.2.7` 标签往后的第 1 个提交。base 是 v8.2.7，Poky 锁定的也是 8.2.7——对上了。这是本章最走运的一步，4.4 节再细说"对不上时会怎样"。

> **🔥 重要**：导出 patch 前先确认 base，是集成工作的第一条纪律。base 差一个小版本，patch 里的上下文行就可能错位，轻则打不上，重则打错了位置还不知道。

#### 4.1.3 git format-patch 导出序列

base 确认无误，导出 patch。`git format-patch` 把一段提交序列导出成一组 `.patch` 文件，每个文件既包含提交信息（作者、日期、说明），也包含代码 diff——这是 git 世界里搬运改动的标准"运输格式"。

```bash
# 把最近 3 个提交（tiger 相关的三个）导出为 patch 序列
cd ~/workspace/qemu-tiger
git format-patch -3 -o /tmp/tiger-patches

# 确认导出了三个文件
ls -1 /tmp/tiger-patches/
```

输出（文件名以本地实际提交说明为准）：

```text
0001-hw-arm-Add-tiger-machine-skeleton.patch
0002-hw-arm-tiger-Wire-up-onboard-devices.patch
0003-hw-arm-tiger-Register-default-configs-and-docs.patch
```

文件名前的 `0001`、`0002`、`0003` 是序号，按提交先后顺序编号——这个顺序不是装饰，4.3 节会看到它是有语义的。打开第一个 patch 看看结构。

```bash
# 查看 patch 文件的结构（前 30 行）
head -30 /tmp/tiger-patches/0001-hw-arm-Add-tiger-machine-skeleton.patch
```

输出（关键行，以本地实际输出为准）：

```text
From a3f9c12de8... Mon Sep 17 00:00:00 2001
From: 陈工 <chen@<your-company>.com>
Date: Tue, 14 Jul 2026 10:23:41 +0800
Subject: [PATCH 1/3] hw/arm: Add tiger machine skeleton

---
 hw/arm/Kconfig     |  8 +++++
 hw/arm/meson.build |  1 +
 hw/arm/tiger.c     | 87 ++++++++++++++++++++++++++++++++++++++++++++
 3 files changed, 96 insertions(+)

diff --git a/hw/arm/Kconfig b/hw/arm/Kconfig
index 1a2b3c4..5d6e7f8 100644
--- a/hw/arm/Kconfig
+++ b/hw/arm/Kconfig
# ... (省略)
```

一个 patch 文件分三段：邮件头（`From` 哈希、作者、日期）、`Subject` 与统计信息、`diff` 正文。`diff` 正文里以 `@@` 开头的行标记每处修改的位置和上下文——每个 `@@` 段叫一个 hunk，4.4 节会正式认识它。

到这里，第一步完成。阿凯把整条流向画了下来。

**Fig-4-1 开发态到集成态的 patch 流向**

```text
qemu-tiger 仓库（开发态）                meta-tiger（集成态）              构建产物
┌──────────────────────┐
│ v8.2.7 上游 base      │
│  + a3f9c12 骨架       │  git format-patch
│  + 7e21b4d 外设       │ ───────────────►  /tmp/tiger-patches/0001~0003.patch
│  + 5c08e90 注册       │
└──────────────────────┘                            │
                                                     │ cp 到 layer + SRC_URI 声明
                                                     ▼
                            meta-tiger/recipes-bsp/qemu/qemu-system-native/*.patch
                                                     │
                                                     │ bitbake qemu-system-native
                                                     │ （do_patch 按序应用）
                                                     ▼
                            qemu-system-aarch64 -M tiger
```

### 4.2 bbappend：不动原配方地追加行为

#### 4.2.1 为什么不能直接改 poky

patch 到手了。阿凯的第一反应很直接：打开 `poky/meta/recipes-devtools/qemu/qemu-system-native_8.2.7.bb`，把三个 patch 加进去，完事。

他刚把编辑器打开，老周不知道什么时候站在了身后："你干什么呢？"

"把 patch 加进配方啊，就加几行……"

"上游仓库，一行都不许动。"老周把话放得很平，但没有商量的余地，"今天你在 poky 里加三行，三个月后升级 Scarthgap 小版本，`git pull` 一拉就是冲突。半年后没人记得 poky 里哪行是你加的。这是你序章就学过的——集成态和开发态的分界线，poky 是上游，跟 qemu-tiger 一样，只读。"

阿凯合上编辑器："那怎么把 patch 喂进去？"

老周没回答，反问他："你昨天写的 `layer.conf`，priority 写的是几？注释里说的预留是干什么用的？"

阿凯翻开 `meta-tiger/conf/layer.conf`——`BBFILE_PRIORITY_meta-tiger = "6"`，注释写着"为后续用 bbappend 覆盖下层配方预留能力（本章还用不上）"。再往下看，`BBFILES` 的通配里赫然写着 `*.bbappend`。

"……bbappend。"阿凯反应过来了，"layer 里可以放一种 `.bbappend` 文件，叠加到下层同名配方上，不用改原配方。"

"priority 6 就是干这个的。"老周点头，"你的 layer 比 OE-Core 的 5 高，你的追加文件会被叠在最后。写吧。"

这种 `.bbappend` 文件正式的名字叫 **bbappend 文件（Append file）**：它与某个 `.bb` 配方同名，内容会被 BitBake 追加到该配方的末尾参与解析，效果上等于"在原配方末尾续写几行"，但原配方一个字节都不用改。这是 Yocto 里定制下游行为的标准手段，也是 meta-tiger 存在的核心理由之一。

#### 4.2.2 命名规则：`<PN>_%.bbappend`

bbappend 文件的命名有硬规则：`<PN>_<版本>.bbappend`。`PN` 是 **配方名（PN）**，即配方文件名去掉版本号的部分——`qemu-system-native_8.2.7.bb` 的 PN 就是 `qemu-system-native`。BitBake 靠文件名前缀把 bbappend 和配方配对：只有文件名里的名字与配方的 PN 一致（或通配匹配），追加才会发生。

版本部分一般写成 `%`，这是通配符，表示"匹配任意版本"。写成 `qemu-system-native_%.bbappend`，将来 OE-Core 把 QEMU 从 8.2.7 升到 8.2.8，这个文件不用改名，照常生效。

阿凯刚要敲文件名，又停住了："`qemu-system-native_8.2.7.bb` 第一行不是有个 `BPN = "qemu"` 吗？匹配的是 `PN` 还是 `BPN`？"

他翻出配方确认。`BPN` 是 **基础配方名（BPN）**，PN 去掉 `native`、`nativesdk-` 这类前后缀之后的基础名。QEMU 的三个配方同源，BPN 都是 `qemu`——所以 OE-Core 才把 patch 统一放在 `qemu/` 子目录下共享。

```bash
# 确认 qemu-system-native 配方的 BPN 声明
head -3 ~/workspace/poky/meta/recipes-devtools/qemu/qemu-system-native_8.2.7.bb
```

输出：

```bitbake
BPN = "qemu"

require qemu-native.inc
```

"匹配的是 PN。"老周只说了五个字。

阿凯把这条记在便签上：**bbappend 文件名按 PN 匹配，与 BPN 无关**。`qemu_%.bbappend` 和 `qemu-system-native_%.bbappend` 是两个不同配方的追加文件——这句话他现在只是抄下来了，4.8 节的一次虚惊会让他真正读懂它的分量。

#### 4.2.3 在 meta-tiger 里落位

按本书的目录约定，QEMU 的 bbappend 放在 `recipes-bsp/qemu/` 下——和 OE-Core 的 `recipes-devtools/qemu/` 类别名保持同一语义层级，tiger 的启动链组件将来都归 `recipes-bsp`。patch 文件再往下放一级，以 PN 命名的目录里。

```bash
# 创建 bbappend 与 patch 的落位目录（两级目录：类别/qemu-system-native）
mkdir -p ~/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native

# 把导出的三个 patch 拷进去
cp /tmp/tiger-patches/000*.patch \
   ~/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native/

# 确认目录结构
find ~/workspace/meta-tiger/recipes-bsp | sort
```

输出：

```text
/home/<your-username>/workspace/meta-tiger/recipes-bsp
/home/<your-username>/workspace/meta-tiger/recipes-bsp/qemu
/home/<your-username>/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native
/home/<your-username>/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native/0001-hw-arm-Add-tiger-machine-skeleton.patch
/home/<your-username>/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native/0002-hw-arm-tiger-Wire-up-onboard-devices.patch
/home/<your-username>/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native/0003-hw-arm-tiger-Register-default-configs-and-docs.patch
```

上一章画层结构图时，`recipes-bsp/` 标注的是"等 TF-A 和 U-Boot 来填"——结果第一个入住的不是启动链组件，是 QEMU 的 bbappend。顺序无妨：这个目录收的是"让板子跑起来"相关的一切集成，模拟器也算在内。

先创建一个空的 bbappend 文件占位，验证叠加关系——内容下一节再填。

```bash
# 先建空的 bbappend 验证叠加（内容 4.3 节填写）
touch ~/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native_%.bbappend

# 初始化构建环境（每次新开终端都需要）
cd ~/workspace/poky
source oe-init-build-env ../build

# 验证：BitBake 是否把这个 bbappend 叠加到了 qemu-system-native 上
bitbake-layers show-appends qemu-system-native
```

输出：

```text
=== Matched appended recipes ===
qemu-system-native_8.2.7.bb:
  /home/<your-username>/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native_%.bbappend
```

`show-appends` 是检查 bbappend 生效关系的第一手段：它列出每个配方被哪些追加文件叠加。输出说明 `meta-tiger` 的追加文件已经挂到了 OE-Core 的 `qemu-system-native_8.2.7.bb` 上——叠加链路通了。

**Fig-4-2 bbappend 叠加示意**

```text
OE-Core（priority 5）
  meta/recipes-devtools/qemu/qemu-system-native_8.2.7.bb
  ── 原配方：下载 QEMU 8.2.7 源码 + OE 自带 patch + 编译安装
            ▲
            │ 文件名按 PN 匹配（qemu-system-native），% 通配版本
            │
meta-tiger（priority 6）
  recipes-bsp/qemu/qemu-system-native_%.bbappend
  ── 追加：FILESEXTRAPATHS 扩展 + SRC_URI 追加 3 个 tiger patch
            │
            ▼
  合并后的有效配方 = 原配方全部内容 + bbappend 追加内容
  （poky 仓库一个字节未动）
```

### 4.3 SRC_URI 与 file:// 协议

#### 4.3.1 SRC_URI：配方的"采购清单"

空文件挂上了，现在往里填内容。核心是一个变量：**源码地址（SRC_URI）**，配方获取源料的地址清单——去哪里下载源码包、拉哪个 git 仓库、附带哪些本地文件，全写在这里。BitBake 的 do_fetch 任务按这个清单把源料取回，do_unpack 解包，do_patch 应用其中的 patch。

看一眼 OE-Core 的 `qemu.inc`，真实的 SRC_URI 长这样：

```bitbake
# 文件路径：~/workspace/poky/meta/recipes-devtools/qemu/qemu.inc（SRC_URI 节选，中间省略二十余个 patch 条目）
SRC_URI = "https://download.qemu.org/${BPN}-${PV}.tar.xz \
           file://powerpc_rom.bin \
           file://run-ptest \
           file://0001-qemu-Add-addition-environment-space-to-boot-loader-q.patch \
           file://CVE-2025-11234-01.patch \
           file://CVE-2025-11234-02.patch \
           "
```

地址按协议分三类：`https://` 是从网络下载的源码包（这里用 `${BPN}-${PV}` 拼出 `qemu-8.2.7.tar.xz`）；`git://` 是从 git 仓库拉源码（本章用不上，chapter 8 集成内核时再见）；`file://` 是本地文件——OE-Core 给 QEMU 打的二十多个 patch（CVE 修复、glibc 兼容、musl 适配等）全是 `file://` 形式，就放在配方旁边的 `qemu/` 子目录里。我们要做的，就是把三个 tiger patch 追加进这张清单。

#### 4.3.2 file:// 去哪找：FILESEXTRAPATHS 与 THISDIR

`file://` 只写文件名、不写全路径，BitBake 去哪找？默认搜索路径是配方旁边的三类子目录：以 `${BP}` 命名的（即 `${BPN}-${PV}`，比如 `qemu-8.2.7/`）、以 `${BPN}` 命名的（比如 `qemu/`）、以及 `files/`。OE-Core 的 patch 放在 `qemu/` 子目录（BPN 命名）就是这个原因。注意默认清单里既没有配方文件所在目录本体，也没有以 PN 命名的子目录。

我们的 patch 在 `meta-tiger` 里，不在 OE-Core 配方旁边，默认搜索路径怎么都够不到它。所以 bbappend 里有一个雷打不动的配套动作：用 **文件搜索扩展路径（FILESEXTRAPATHS）** 把 meta-tiger 这边的目录加进搜索列表。配套出现的另一个变量是 **当前文件目录变量（THISDIR）**——BitBake 内置变量，解析到哪个文件就指向哪个文件所在的目录。在 bbappend 里，`${THISDIR}` 就是 `meta-tiger/recipes-bsp/qemu/`。

标准写法一行：

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
```

逐部分拆开：`:prepend` 是前置追加（把新目录排在默认路径最前面）；`:=` 是立即展开（立刻把 `${THISDIR}`、`${PN}` 替换成实际值）；`${PN}` 展开为 `qemu-system-native`——合起来就是"把 `meta-tiger/recipes-bsp/qemu/qemu-system-native/` 加进 file:// 的搜索路径最前面"。这正是 4.2.3 里 patch 落位的那个目录。

> **⚠️ 注意**：这行里的每个符号都有讲究。`:prepend` 的值必须以路径开头、以冒号结尾（路径列表的分隔符）；`:=` 写成 `=` 会导致展开时机不同，某些场景下出问题。照抄模板即可，别自由发挥。

#### 4.3.3 写出 bbappend 全文

两件事都清楚了，写出完整文件。

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native_%.bbappend
# 从 qemu-tiger 仓库导出的 tiger machine 补丁（base：QEMU v8.2.7）

# 把本文件旁的 qemu-system-native/ 目录加进 file:// 搜索路径
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# 追加三个 patch。列出顺序即 do_patch 的应用顺序，必须与导出序号一致
SRC_URI:append = " \
    file://0001-hw-arm-Add-tiger-machine-skeleton.patch \
    file://0002-hw-arm-tiger-Wire-up-onboard-devices.patch \
    file://0003-hw-arm-tiger-Register-default-configs-and-docs.patch \
"
```

`SRC_URI:append = " ..."` 把三个 `file://` 条目追加到原配方的 SRC_URI 末尾——注意末尾有个空格分隔，BitBake 的 `:append` 不会自动补空格，漏了空格新条目会和原清单最后一项粘在一起。

为什么强调"顺序"？因为 BitBake 的 do_patch 任务（任务链中负责打 patch 的一环，排在 do_unpack 之后、do_configure 之前）是**按 SRC_URI 里列出的顺序逐个应用 patch** 的。三个 tiger patch 是一个序列：0002 在 0001 创建的 `tiger.c` 和 `config TIGER` 配置段上继续改，0003 又依赖前两个的结果。顺序即语义——0001 没打，0002 的上下文就不存在。

而且这三个 patch 不是打在原版 8.2.7 源码上，是打在"OE-Core 二十多个 patch 打完之后的源码"上——它们排在 SRC_URI 清单的最后，最后才被应用。同事的 patch 与 OE 的 CVE 修复没有改到同一片代码，上下文不冲突，所以能直接叠加。这也是 4.1.2 确认 base 时埋下的前提：base 对齐，加上改动区域不重叠，叠加才成立。

写完，用 `bitbake -e` 确认追加后的 SRC_URI 真的带上了三个 patch。

```bash
# 查看合并后的 SRC_URI 中是否包含 tiger patch
# （先锁定 SRC_URI 的最终值行——bitbake -e 的历史注释里也会重复出现这些条目——再过滤 tiger 条目）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e qemu-system-native | grep '^SRC_URI=' | grep -o "file://000[0-9]-hw-arm[^\"]*"
```

输出：

```text
file://0001-hw-arm-Add-tiger-machine-skeleton.patch
file://0002-hw-arm-tiger-Wire-up-onboard-devices.patch
file://0003-hw-arm-tiger-Register-default-configs-and-docs.patch
```

三个 patch 已经进了清单，顺序正确。到这里，四步里的前三步——导出、bbappend、SRC_URI——全部落地。

### 4.4 patch 与 QEMU 8.2.7 的兼容性

#### 4.4.1 base 对齐检查清单

构建之前，老周让阿凯把 4.1.2 做过的检查正式落成一张清单。"这次对上了是运气。以后每次从开发态拿 patch，都按这个单子过一遍。"

1. **版本对齐**：开发仓库的 base 标签（`git describe` 给出的 `v8.2.7`）与 Yocto 配方锁定的版本（文件名里的 `8.2.7`，即 PV）一致。
2. **上下文对齐**：开发态 patch 与集成目标已有的 patch 序列（OE-Core 的二十多个）不改同一片代码——`git log` 看改动文件列表，和 `qemu.inc` 里 patch 的目标文件对照，没有重叠。
3. **顺序对齐**：SRC_URI 里列出的顺序与 `git format-patch` 导出的序号一致（4.3.3 已确认）。

三条都过，patch 才算有了"准生证"。如果第一条就对不上——比如同事基于 v8.2.5 开发，而 Poky 是 8.2.7——patch 里的上下文行可能已经错位。这时 patch 工具会报 **补丁片段（hunk）** 失败：hunk 是 diff 里以 `@@` 标记的最小修改单元，记录"第几行附近、删掉什么、加上什么"，上下文对不上，这个 hunk 就落不了地。4.8 节会见到它失败的完整样貌。

#### 4.4.2 给 patch 补上 Upstream-Status 头

阿凯翻 patch 文件时发现一个缺失：OE-Core 的 patch 头部都有一行 `Upstream-Status`，同事的三个 patch 没有。

```bash
# 看看 OE-Core 的 patch 里 Upstream-Status 写在哪、长什么样
grep -n "Upstream-Status" ~/workspace/poky/meta/recipes-devtools/qemu/qemu/CVE-2025-11234-01.patch
```

输出：

```text
29:Upstream-Status: Backport [https://gitlab.com/qemu-project/qemu/-/commit/911c814c8cc5f836286bd96694843036db83e99f]
```

第 29 行——在提交说明之后、统计信息的 `---` 分隔线之前。**上游状态（Upstream-Status）** 是补丁头里的元数据字段，回答一个问题：这个 patch 和上游是什么关系？合法取值有六种：`Pending`（还没提交上游）、`Submitted`（已提交、等待合入）、`Backport`（从上游新版本回移）、`Denied`（上游明确拒绝）、`Inappropriate`（不适合也不打算提交上游）、`Inactive-Upstream`（上游已不再维护）。旧资料里常见的 `Accepted` 已经废弃，写上去会被 QA 检查判为格式错误，别用。

Yocto 的 QA 检查就盯着这个字段，而且不只是"留个警告"那么温和：对 core 层的配方——`qemu-system-native` 正是——缺这行字的 patch 会让构建在 QA 阶段直接报错失败。meta-tiger 自己的配方默认不查这一项，但我们改的是 core 层配方，躲不掉。

tiger 是虚构硬件，patch 不可能也不应该提交 QEMU 上游，正确的写法是：

```text
Upstream-Status: Inappropriate [tiger 为项目虚构硬件，不上游]
```

阿凯用编辑器给三个 patch 的 `Subject` 段之后各补上这一行，然后验证一遍：

```bash
# 确认三个 patch 都补上了 Upstream-Status（每个文件应各 1 处）
grep -c "Upstream-Status" ~/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native/*.patch
```

输出（路径以本地为准）：

```text
.../0001-hw-arm-Add-tiger-machine-skeleton.patch:1
.../0002-hw-arm-tiger-Wire-up-onboard-devices.patch:1
.../0003-hw-arm-tiger-Register-default-configs-and-docs.patch:1
```

这是一分钟的事，但三年后做 license 审计或升级评估的人，会靠这行字判断"这个 patch 能不能指望上游接盘"。

> **💡 提示**：`Upstream-Status` 行加在 patch 头部的提交说明之后、统计信息的 `---` 分隔线之前。它不改变 patch 的技术内容，是给人和 QA 工具看的元数据。

### 4.5 构建与验证：QEMU 认识 tiger 了

#### 4.5.1 runqemu 用的是哪个 QEMU

敲构建命令之前，阿凯先解决一个悬着的问题："`qemu_8.2.7.bb`、`qemu-native_8.2.7.bb`、`qemu-system-native_8.2.7.bb`——三个配方，我们的 patch 为什么喂给 `qemu-system-native`？runqemu 用的到底是哪个？"

老周又一次把问题抛了回来："你自己打开三个文件看头部，再想想 chapter 1 学过的 Native recipe。"

阿凯把三个文件并排翻了。

`qemu-native_8.2.7.bb`：`require qemu-native.inc`，target 列表来自 `get_qemu_usermode_target_list`——只编 linux-user 用户态模拟工具。

`qemu-system-native_8.2.7.bb`：同样 `require qemu-native.inc`，但 target 列表来自 `get_qemu_system_target_list`——只编 softmmu（QEMU 的整机模拟模式，与 linux-user 用户态模拟相对），也就是 `qemu-system-aarch64` 这类整机模拟器。它还 `DEPENDS += ... qemu-native`——配方注释写明两者安装的文件有交集，靠 qemu-native 补齐安装集避免冲突。

`qemu_8.2.7.bb`：第一行 `BBCLASSEXTEND = "nativesdk"`，是 target + nativesdk 配方。

结论自己浮出来了：runqemu 要启动整机，用的是 `qemu-system-aarch64`，来自 `qemu-system-native`。证据还能更硬——阿凯在 `qemuboot.bbclass` 里翻到一行：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/qemuboot.bbclass（最后一行）
EXTRA_IMAGEDEPENDS += "qemu-system-native qemu-helper-native:do_addto_recipe_sysroot"
```

镜像构建强制依赖 `qemu-system-native`——chapter 1 构建 `core-image-minimal` 时它就已经被连带编出来了，runqemu 从它的产物里拿 `qemu-system-aarch64`。这就是 patch 必须喂给它的原因。

**Fig-4-3 QEMU 三配方速查表**

| 配方 | 构建内容 | 谁在用 | 本章角色 |
|------|---------|--------|---------|
| `qemu-native` | linux-user 用户态模拟工具（如 `qemu-aarch64`） | 构建系统在主机上跑目标架构小程序的场景 | 被依赖，命中缓存 |
| `qemu-system-native` | softmmu 整机模拟器（`qemu-system-aarch64` 等） | **runqemu**、testimage（自动化镜像测试框架） | **patch 的目标** |
| `qemu`（target + nativesdk） | 目标板/ SDK 版 QEMU | SDK 使用者（chapter 14 前瞻） | 与本章无关 |

三个配方都是 Native recipe 家族的成员（前两个）或其亲戚：在构建主机 x86_64 上编译、在构建主机上运行，产物不进目标板镜像。产物落在 `tmp/sysroots-components/x86_64/` 下——`x86_64` 这个目录名就是"跑在主机上"的标记。

#### 4.5.2 bitbake qemu-system-native

```bash
# 构建 qemu-system-native（QEMU 全量编译，视机器性能约需 20-60 分钟）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake qemu-system-native
```

输出（关键行，任务数以本地实际输出为准）：

```text
Loading cache: 100% |############################################| Time: 0:00:01
# ... (省略)
NOTE: Tasks Summary: Attempted 43 tasks of which 18 didn't need to be rerun and all succeeded.
```

重新执行的是 `qemu-system-native` 自己的任务链——do_fetch、do_unpack、do_patch、do_configure、do_compile、do_install 等二十来个；它依赖的 `qemu-native`、`glib-2.0-native` 等没有变化，全部命中 Sstate 缓存。构建时间的大头只有一个：QEMU 本体的全量编译。

> **💡 提示**：如果 QEMU 源码包已在 `downloads/` 里（chapter 1 构建镜像时下载过），do_fetch 秒过；否则第一次会从 `download.qemu.org` 拉一个一百多 MB 的源码包。

#### 4.5.3 产物验证

构建成功不代表 patch 生效，要亲眼看到 `tiger` 出现在 machine 列表里。产物在 Native recipe 的 sysroot 目录。

```bash
# 验证 1：QEMU 的 machine 列表里有没有 tiger
~/workspace/build/tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64 -machine help | grep tiger
```

输出（tiger 行的具体描述文字以本地集成结果为准）：

```text
tiger                tiger virtual development board (Cortex-A53)
```

出现了。再打一发冒烟测试：真的用这个 machine 类型实例化一次虚拟机，不给内核、不给镜像，`-S` 让 CPU 停在复位状态，能起来就说明 machine 的设备模型全部初始化成功。

```bash
# 验证 2：冒烟实例化（-S 停在复位状态，-display none 无显示；无输出即成功，Ctrl-C 退出）
~/workspace/build/tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64 \
    -machine tiger -display none -S
```

命令静默挂住，不报错——这就是成功。Ctrl-C 退出。对照一下不支持的 machine 类型会报什么，以后看到这个错就知道是 machine 名没对上：

```bash
# 对照实验：请求一个不存在的 machine 类型（预期报错）
~/workspace/build/tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64 \
    -machine notexist -display none
```

输出（这是"预期中的错误"）：

```text
qemu-system-aarch64: unsupported machine type
Use -machine help to list supported machines
```

到此，老周的四步全部走完：bitbake 构建出来的 QEMU 认识 `-M tiger` 了。

### 4.6 runqemu 是怎么认出 tiger 的

#### 4.6.1 自己找答案：`-machine virt` 是谁给的

"那现在 `runqemu tiger-aarch64` 能跑了吗？"阿凯问。

"你 chapter 1 敲过 `runqemu qemuarm64`。"老周不答，"启动的时候 QEMU 命令行里有个 `-machine virt`——这串参数是谁给的？`qemu-system-aarch64` 自己是不知道什么叫 qemuarm64 的。去 conf 里找。"

阿凯回到 `qemuarm64.conf`，这次不读全文，直接过滤。

```bash
# 聚焦 qemuarm64.conf 里的 QB_ 行（chapter 2 读过全文，这次只看 QB_*）
grep "^QB_" ~/workspace/poky/meta/conf/machine/qemuarm64.conf
```

输出：

```bitbake
QB_SYSTEM_NAME = "qemu-system-aarch64"
QB_MACHINE = "-machine virt"
QB_CPU = "-cpu cortex-a57"
QB_SMP ?= "-smp 4"
QB_CPU_KVM = "-cpu host -machine gic-version=3"
QB_GRAPHICS = "-device virtio-gpu-pci"
QB_OPT_APPEND = "-device qemu-xhci -device usb-tablet -device usb-kbd"
QB_TAP_OPT = "-netdev tap,id=net0,ifname=@TAP@,script=no,downscript=no"
QB_NETWORK_DEVICE = "-device virtio-net-pci,netdev=net0,mac=@MAC@"
QB_ROOTFS_OPT = "-drive id=disk0,file=@ROOTFS@,if=none,format=raw -device virtio-blk-pci,drive=disk0"
QB_SERIAL_OPT = "-device virtio-serial-pci -chardev null,id=virtcon -device virtconsole,chardev=virtcon"
QB_TCPSERIAL_OPT = "-device virtio-serial-pci -chardev socket,id=virtcon,port=@PORT@,host=127.0.0.1,nodelay=on -device virtconsole,chardev=virtcon"
```

找到了。这一族 `QB_` 前缀的变量是 **QEMU 启动参数变量族（QB_\* 变量）**，QB 意为 Qemu Boot，专门写给 runqemu 看的。其中两个最关键：**QB_SYSTEM_NAME** 告诉 runqemu 用哪个 QEMU 二进制（`qemu-system-aarch64`），**QB_MACHINE** 告诉它传什么 machine 参数（`-machine virt`）。`runqemu qemuarm64` 的那条 QEMU 命令行，就是从这十几行拼出来的。

那 `tiger-aarch64` 的映射在哪接？答案已经呼之欲出：将来 tiger 的 MACHINE 配置里写一行 `QB_MACHINE = "-machine tiger"`，runqemu 就会把 `tiger-aarch64` 映射到 QEMU 的 `-M tiger`——老周开场说的命名约定，机制上就靠这一行兑现。

#### 4.6.2 qemuboot.conf：QB_\* 的运输载体

`qemuarm64.conf` 里的变量在构建主机上，runqemu 启动时怎么拿到？阿凯想起 chapter 1 在 deploy 目录里见过一类文件：`.qemuboot.conf`。

```bash
# 查看 qemuarm64 镜像的 qemuboot.conf 实例（关键行）
cat ~/workspace/build/tmp/deploy/images/qemuarm64/core-image-minimal-qemuarm64.rootfs.qemuboot.conf
```

输出（关键行，时间戳与路径以本地实际输出为准）：

```ini
[config_bsp]
machine = qemuarm64
qb_system_name = qemu-system-aarch64
qb_machine = -machine virt
qb_cpu = -cpu cortex-a57
qb_mem = -m 256
qb_smp = -smp 4
qb_rootfs_opt = -drive id=disk0,file=@ROOTFS@,if=none,format=raw -device virtio-blk-pci,drive=disk0
serial_consoles = 115200;ttyAMA0 115200;hvc0
staging_bindir_native = ../../../work/x86_64-linux/qemu-helper-native/1.0/recipe-sysroot-native/usr/bin
# ... (省略)
```

这份 INI 格式的文件就是 **runqemu 启动配置文件（qemuboot.conf）**：镜像构建时，`qemuboot.bbclass` 里的 `do_write_qemuboot_conf` 任务（排在 do_rootfs 之后、do_image 之前）把当时生效的所有 `QB_*` 变量连同一批构建变量（机器名、内核信息、sysroot 路径等）收集起来，写进 deploy 目录。变量名落盘时转成小写——`QB_MACHINE` 变成 `qb_machine`——runqemu 读的时候再转回去。

链路闭环了：MACHINE 配置里的 `QB_*` → `do_write_qemuboot_conf` → `qemuboot.conf` → runqemu 读取并拼出 QEMU 命令行。chapter 1 那次 `runqemu qemuarm64 nographic slirp`，背后读的就是这份文件。

**Fig-4-4 runqemu 数据流**

```text
qemuarm64.conf（MACHINE 配置）
  QB_SYSTEM_NAME = "qemu-system-aarch64"
  QB_MACHINE     = "-machine virt"
  QB_CPU / QB_MEM / QB_ROOTFS_OPT / ...
        │  镜像构建时
        ▼  do_write_qemuboot_conf
tmp/deploy/images/qemuarm64/<image>.qemuboot.conf
        │  runqemu 启动时读取
        ▼  拼命令行
qemu-system-aarch64 -machine virt -cpu cortex-a57 -m 256 \
    -kernel Image -drive ...（来自 qemu-system-native 的产物）
```

#### 4.6.3 预期报错：现在还跑不了

机制清楚了，阿凯当场试一下。

```bash
# 尝试用 runqemu 启动 tiger-aarch64（预期报错，演示用）
cd ~/workspace/poky
source oe-init-build-env ../build
runqemu tiger-aarch64 nographic slirp
```

输出（这是"预期中的错误"，关键行，以本地实际输出为准）：

```text
INFO: Running MACHINE=tiger-aarch64 bitbake -e ...
ERROR: /home/<your-username>/workspace/build/tmp/deploy/images/tiger-aarch64 not a directory valid DEPLOY_DIR_IMAGE
WARNING: Can't find qemuboot conf file, DEPLOY_DIR_IMAGE is NULL!
ERROR: Unable to determine QEMU PC System emulator for tiger-aarch64 machine.
ERROR: As tiger-aarch64 is not among valid QEMU machines such as,
ERROR: qemux86-64, qemux86, qemuarm64, qemuarm, qemumips64, qemumips64el, qemumipsel, qemumips, qemuppc
ERROR: Set qb_system_name with suitable QEMU PC System emulator in .*qemuboot.conf.
```

逐行读这个报错，恰好把缺失的拼图数了出来。

第一步，runqemu 用 `bitbake -e` 查 `tiger-aarch64` 这台机器的部署目录——`tmp/deploy/images/tiger-aarch64/` 不存在，因为从没给这台机器构建过镜像。

部署目录无效，自然找不到 `qemuboot.conf`，`QB_SYSTEM_NAME` 也就拿不到。

最后它试着从机器名猜 QEMU 二进制——猜测表只认 `qemux86`、`qemuarm64` 这些官方命名，`tiger-aarch64` 不在表里，报错退出。

缺的两块拼图，正好是老周说的"半步"之外的部分：

- **MACHINE 配置**：`conf/machine/tiger-aarch64.conf` 还不存在——`QB_SYSTEM_NAME = "qemu-system-aarch64"`、`QB_MACHINE = "-machine tiger"` 都要写在那里。这是下一章（chapter 5）的活。
- **内核与镜像**：`tmp/deploy/images/tiger-aarch64/` 要等有内核（chapter 8）和镜像构建之后才会有内容。runqemu 完整启动 tiger，属于 phase 4 的事。

"所以现在是什么状态？"老周问。

阿凯想了想："QEMU 已经认识 `tiger` 这块板了，但 Yocto 还不认识 `tiger-aarch64` 这台机器。中间的路是 qemuboot.conf，路的两端都还没修。"

"这就是那半步。"老周说，"今天机制讲透了，就够了。"

### 4.7 为什么不把 QEMU fork 塞进 layer

收工前，阿凯问了一个憋了一下午的问题："三个 patch 要维护 base、维护顺序、补 Upstream-Status——这么麻烦，为什么不把 qemu-tiger 整个仓库塞进 meta-tiger，让配方直接从本地源码编？一劳永逸。"

老周难得地多说了几句。

"三句话。第一，layer 是集成清单，不是开发现场——meta-tiger 里的每个文件都应该回答'集成了什么'，而不是'怎么开发的'，近七百行 `tiger.c` 塞进来，这个 layer 就说不清了。第二，patch 序列是两个世界之间的 diff，diff 越小越好审、越好升级——三个 patch，每个我能十分钟读完；一个 fork 的 QEMU 源码树，十万行起步，没人审得动。第三，看远一点：将来 QEMU 从 8.2 升 9.x，你的活是把三个 patch rebase 到新 base 上，大概率一下午；如果维护的是一个 fork，你要面对的是整棵树的合并，工作量不在一个量级。"

"序章说的开发态和集成态，"阿凯接上，"不只是'在哪干活'的规矩，是给未来的维护成本上的保险。"

"今天你踩的两个坑——等下就要讲——本质上都是 diff 管理的问题。"老周拿起杯子，"diff 越小，咬你的地方越少。"

### 4.8 踩坑实录

老周上午的预言应验了，两次。先交代时间线：这两件事都发生在 4.3 写完 bbappend、4.5 构建成功之前——你在前面看到的正确文件和一路顺利的构建，都是修正之后的样子。

#### 4.8.1 踩坑 1：bbappend 写成 `qemu_%.bbappend`，patch 喂错了配方

当时阿凯给 bbappend 起名，图"直觉"写成了 `qemu_%.bbappend`——"QEMU 的配方嘛，就叫 qemu"。文件挂上后他也没验证，直接准备构建。幸好老周路过时多问了一句："你 `show-appends` 看过吗？"

我们来重演这个错误。把 4.3 写好的正确文件改名，回到那个下午。

```bash
# 重演当时的错误：把 bbappend 改名为 qemu_%.bbappend（错误示范，请勿模仿）
cd ~/workspace/meta-tiger/recipes-bsp/qemu
mv qemu-system-native_%.bbappend qemu_%.bbappend

# 初始化构建环境
cd ~/workspace/poky
source oe-init-build-env ../build

# 检查 qemu（target 配方）的叠加情况
bitbake-layers show-appends qemu
```

输出（关键行）：

```text
=== Matched appended recipes ===
qemu_8.2.7.bb:
  /home/<your-username>/workspace/meta-tiger/recipes-bsp/qemu/qemu_%.bbappend
```

patch 被挂到了 `qemu_8.2.7.bb` 上——那是 target + nativesdk 配方，跟 runqemu 毫无关系。再看真正的目标：

```bash
# 检查 qemu-system-native 的叠加情况
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-appends qemu-system-native
```

输出：

```text
=== Matched appended recipes ===
No append files found
```

空的。`qemu` 和 `qemu-system-native` 是两个 PN 不同的独立配方，bbappend 文件名按 PN 精确匹配——4.2.2 抄在便签上的那句话，现在用一次"差点白构建"兑现了它的分量。如果当时没发现就 `bitbake qemu-system-native`，构建会照常成功，产物里照样没有 `tiger`——这种"成功了但没生效"的错误最难查。

再补一刀证据：此刻三个 tiger patch 实际进了谁的 SRC_URI。

```bash
# 确认 tiger patch 进了 target 配方 qemu 的 SRC_URI（本不该出现）
# （同样先锁定 SRC_URI 最终值行，再过滤 tiger 条目）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake -e qemu | grep '^SRC_URI=' | grep -o "file://000[0-9]-hw-arm[^\"]*"
```

输出：

```text
file://0001-hw-arm-Add-tiger-machine-skeleton.patch
file://0002-hw-arm-tiger-Wire-up-onboard-devices.patch
file://0003-hw-arm-tiger-Register-default-configs-and-docs.patch
```

3 个 patch 全在 target 配方里。注意这里只做了 `bitbake -e` 解析验证，**不要**真的去 `bitbake qemu`——target 版 QEMU 编译耗时比 native 版还长，拿它验证一个已知错误不划算。

修正：把文件名改回来，重新验证叠加关系。

```bash
# 修正：文件名改回按 PN 匹配的正确形式
cd ~/workspace/meta-tiger/recipes-bsp/qemu
mv qemu_%.bbappend qemu-system-native_%.bbappend

# 重新验证（输出同 4.2.3：bbappend 挂到 qemu-system-native_8.2.7.bb 上）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-appends qemu-system-native
```

教训两条。一是 bbappend 起名先查 PN——`bitbake-layers show-recipes | grep qemu` 看一眼配方家族再动手，别凭直觉。二是**改完配方文件名，`show-appends` 是第一检查手段**：它十秒钟就能告诉你追加文件挂没挂对地方，比一次错误构建便宜得多。

#### 4.8.2 踩坑 2：SRC_URI 里 patch 顺序写反，do_patch hunk 失败

第二个坑发生在阿凯手写 SRC_URI 的时候：复制三个 patch 文件名时，顺手把 `0002` 排到了 `0001` 前面。bbappend 挂载位置没问题，`show-appends` 也正常——但 `bitbake qemu-system-native` 在 do_patch 阶段直接炸了。

重演这个错误：用编辑器把 bbappend 里 `SRC_URI:append` 的三行顺序改成 `0002`、`0001`、`0003`。改完的文件长这样（错误示范，请勿模仿）：

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native_%.bbappend（错误示范，请勿模仿）
# 只展示被改错的 SRC_URI 段：0002 被排到了 0001 前面
SRC_URI:append = " \
    file://0002-hw-arm-tiger-Wire-up-onboard-devices.patch \
    file://0001-hw-arm-Add-tiger-machine-skeleton.patch \
    file://0003-hw-arm-tiger-Register-default-configs-and-docs.patch \
"
```

确认一下错误状态，然后构建：

```bash
# 确认 SRC_URI 顺序已被改错（错误示范状态）
grep -A4 "SRC_URI:append" ~/workspace/meta-tiger/recipes-bsp/qemu/qemu-system-native_%.bbappend

# 构建（在 do_patch 阶段失败）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake qemu-system-native
```

grep 输出（`-A4` 连匹配行共五行，顺序已错）：

```text
SRC_URI:append = " \
    file://0002-hw-arm-tiger-Wire-up-onboard-devices.patch \
    file://0001-hw-arm-Add-tiger-machine-skeleton.patch \
    file://0003-hw-arm-tiger-Register-default-configs-and-docs.patch \
"
```

构建输出（这是"你可能遇到的错误"，关键行，行号与路径以本地实际输出为准）：

```text
ERROR: qemu-system-native-8.2.7-r0 do_patch: Applying patch '0002-hw-arm-tiger-Wire-up-onboard-devices.patch' on target directory '/home/<your-username>/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7'
CmdError('quilt ... push', 1, 'Applying patch 0002-hw-arm-tiger-Wire-up-onboard-devices.patch
patching file hw/arm/Kconfig
Hunk #1 FAILED at 208.
1 out of 1 hunk FAILED -- rejects in file hw/arm/Kconfig
Patch 0002-hw-arm-tiger-Wire-up-onboard-devices.patch does not apply (enforce with -f)')
ERROR: Logfile of failure stored in: /home/<your-username>/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/temp/log.do_patch.12345
ERROR: Task (/home/<your-username>/workspace/poky/meta/recipes-devtools/qemu/qemu-system-native_8.2.7.bb:do_patch) failed with exit code '1'
```

这套输出是 hunk 失败的标准样貌，读法有固定套路，从下往上读：

1. **最后一行**：哪个任务失败——`do_patch`，配方是 `qemu-system-native_8.2.7.bb`。
2. **Logfile 行**：完整日志在 `temp/log.do_patch.<进程号>`，细节不够时去那里翻。
3. **CmdError 段**：patch 工具（BitBake 默认用 quilt）的原始输出——`Hunk #1 FAILED at 208`，0002 在 `hw/arm/Kconfig` 第 208 行附近的 hunk 落不了地，失败的修改被存进了 `hw/arm/Kconfig.rej`。

阿凯顺着日志路径找到了 `.rej` 文件。

```bash
# 查看 hunk 失败留下的 rejects 文件（在配方的工作源码目录里）
cat ~/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7/hw/arm/Kconfig.rej
```

输出（关键行，以本地实际输出为准）：

```diff
--- hw/arm/Kconfig
+++ hw/arm/Kconfig
@@ -208,7 +208,10 @@
 config TIGER
     bool
+    select PL011
+    select PL031
     ...
```

`.rej` 文件里就是没能落地的那个 hunk：0002 想往 `config TIGER` 配置段里追加 `select PL011` 等行——可这个配置段是 0001 创建的，0001 还没打，第 208 行附近根本没有 `config TIGER`，上下文对不上，hunk 失败。4.3.3 说的"顺序即语义"，这就是反面教材。

修正：把 SRC_URI 的三行调回 `0001`、`0002`、`0003` 的顺序，重新构建。

```bash
# 修正后重新构建（SRC_URI 变更触发任务签名变化，do_unpack/do_patch 自动从头重跑）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake qemu-system-native
```

输出（关键行，以本地实际输出为准）：

```text
NOTE: Tasks Summary: Attempted 43 tasks of which 18 didn't need to be rerun and all succeeded.
```

不用手工清理工作目录——SRC_URI 变了，任务签名就变了，BitBake 会自动重新解包源码、按新顺序重打 patch。构建成功后按 4.5.3 重新验证 `-machine help`。

这套排障流程——读报错末行定位任务、按 Logfile 路径翻日志、看 `.rej` 找失败的 hunk、回忆"顺序即语义"或"base 对齐"两条规则——在 phase 3 集成 TF-A、U-Boot、内核时还会反复用到，值得现在就练熟。

> **🔥 重要**：patch 序列的顺序依赖是隐式的，文件里看不出来。从开发态导出 patch 时，保持 `git format-patch` 的序号原样进 SRC_URI，是成本最低的防错手段。

### 4.9 本章小结

一天结束，老周的四步加半步全部落地：

- **4.1**：patch 是开发态到集成态的运输格式。导出前先确认 base（`git describe` 对照配方版本号 PV），再用 `git format-patch` 导出序列。
- **4.2**：bbappend 文件不动原配方地追加行为；文件名按 PN 匹配（与 BPN 无关），`%` 通配版本；`show-appends` 验证叠加关系。
- **4.3**：SRC_URI 是配方的采购清单；`file://` 本地文件靠 `FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"` 找到；do_patch 按列出顺序应用 patch，顺序即语义。
- **4.4**：patch 兼容性三查——版本对齐、上下文对齐、顺序对齐；`Upstream-Status` 是 patch 的身份证。
- **4.5**：QEMU 三配方分工——`qemu-native`（用户态）、`qemu-system-native`（整机，runqemu 用它，patch 喂给它）、`qemu`（target + SDK）；`-machine help` 见 tiger、`-machine tiger -S` 冒烟通过。
- **4.6**：runqemu 的数据流是 MACHINE 配置 `QB_*` → `qemuboot.conf` → 拼 QEMU 命令行；`runqemu tiger-aarch64` 现在还跑不了，缺 MACHINE 配置和内核两块拼图。
- **4.8**：两个坑都源于"想当然"——bbappend 文件名凭直觉写成 `qemu_%.bbappend`（按 PN 匹配，挂错了配方）；SRC_URI 顺序排反（hunk 失败，`.rej` 文件指路）。

本章产出清单：

- `meta-tiger` 新增 `recipes-bsp/qemu/`：1 个 `qemu-system-native_%.bbappend` + 3 个 tiger patch。
- 构建产物：`tmp/sysroots-components/x86_64/qemu-system-native/usr/bin/qemu-system-aarch64` 支持 `-M tiger`（`-machine help` 可见）。
- 一套可复用的集成范式：**format-patch → bbappend → SRC_URI → 构建验证**。chapter 6/7/8 集成 TF-A、U-Boot、linux-tiger 时，走的就是今天这条路。

提交并打 tag。本章唯一被修改的仓库是 `meta-tiger`：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Integrate tiger machine into qemu-system-native via bbappend"
git tag chapter4
```

> **⚠️ 注意**：本章三个仓库的 tag 归属——`meta-tiger` 打 `chapter4`；`poky` 保持原样、一行未动，不打新 tag；`qemu-tiger` 本章只读使用（clone 和 format-patch 不产生提交），不打 tag，更不要往里推任何东西——那是别的组的开发仓库。

后续任务清单：

- **task 06 / chapter 5**：写 `tiger-aarch64.conf`，把 `QB_MACHINE = "-machine tiger"` 落地，让 Yocto 认识这台机器。
- 远景：task 07–09 用本章的 bbappend + patch 范式集成 TF-A、U-Boot、linux-tiger；task 11 第一次开机成功，runqemu 的拼图全部归位。

老周下班前看了眼阿凯本子上的 Fig-4-4："QEMU 这关过了。明天写 MACHINE 配置——你那张全景地图上，`conf/machine/` 这个空目录，明天就填上。"

---

**延伸阅读**

1. Yocto 开发任务手册，"Customizing a Recipe with bbappend" 与 "Patching Source Code" 小节：https://docs.yoctoproject.org/5.0/dev-manual/
2. BitBake 用户手册，SRC_URI / FILESEXTRAPATHS / PN / BPN / PV 变量条目：https://docs.yoctoproject.org/bitbake/2.8/
3. QEMU 8.2 文档，System Emulation 章节（machine 类型与设备模型）：https://www.qemu.org/docs/8.2/
4. Yocto 变量术语表，QB_* 变量族与 EXTRA_IMAGEDEPENDS：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
