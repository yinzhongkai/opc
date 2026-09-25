## 3 搭起 meta-tiger 的骨架

周三上午，阿凯工位。昨天画的项目全景地图还贴在显示器边上，图最底下"meta-tiger（集成态）"那一格被他用红笔圈了两圈。

达哥端着杯子路过，敲了敲他的桌子："地图看完了，今天开始动手。建一个 `meta-tiger`，先把架子搭起来，里面什么都不放。能让 bitbake 认识它就行。"

阿凯愣了一下："什么都不放？那验证什么？"

"验证架子本身是合格的。"达哥说，"一个 layer 光有目录不行，得让 BitBake 认得它、加载它、不报错。这一步做扎实了，后面往里填东西才不会塌。"

"具体做什么？"

"五件事。"达哥伸出一只手，"第一，按社区惯例把目录建出来；第二，写 `layer.conf`，这是 layer 的身份证；第三，补 README、许可证、维护者名单——别嫌这三个文件没用，以后接手的人会先看你这三个文件；第四，把 layer 注册进构建，验证 bitbake 真的认识它；第五，进 git，打 tag。"

阿凯在本子上记下五行："一天能干完？"

"顺利的话一上午。"达哥顿了顿，"但你这种人，多半要踩一两个坑。"

### 3.1 BSP layer 的目录约定

阿凯打开终端，第一反应是问达哥："layer 里该有哪些目录？"

达哥没回答，朝屏幕努了努嘴："`poky` 就在你机器上。`meta-poky` 是个发行版 layer，`meta-yocto-bsp` 是个 BSP layer，你自己去翻，归纳共同点。"

阿凯认命地敲命令。

```bash
# 看看 Poky 自带的两个官方 layer 长什么样
ls ~/workspace/poky/meta-poky/
ls ~/workspace/poky/meta-yocto-bsp/
```

输出：

```text
# meta-poky/
classes  conf  recipes-core  README.poky.md

# meta-yocto-bsp/
conf  lib  recipes-bsp  recipes-graphics  recipes-kernel  wic  README.hardware.md
```

他又看了一眼 OE-Core 本体的顶层目录。

```bash
# 观察 OE-Core（meta 层）的 recipes-* 分类
ls ~/workspace/poky/meta/
```

输出（关键行）：

```text
classes  conf  files  lib  site
recipes-bsp  recipes-connectivity  recipes-core  recipes-devtools
recipes-extended  recipes-gnome  recipes-graphics  recipes-kernel
recipes-multimedia  recipes-rt  recipes-sato  recipes-support
# ... (省略)
```

阿凯把三个输出摆在一起对比，归纳出三条规律。

第一条，**名字都叫 `meta-<名字>`**。`meta-poky`、`meta-yocto-bsp`，还有以后会遇到的 `meta-arm`（ARM 平台的官方 BSP layer）——全小写、连字符分隔，这是社区雷打不动的命名惯例。

第二条，**都有 `conf/` 目录**。他往里探了一下，每个 layer 的 `conf/` 里必有一个 `layer.conf`。看来这就是达哥说的"身份证"。

第三条，**配方按 `recipes-<类别>` 分类**。类别名不是随便起的，是 OE-Core 定下来的一套语义：`recipes-bsp` 放 bootloader、固件这类贴硬件的东西，`recipes-kernel` 放内核和模块，`recipes-core` 放镜像、包组这类基础系统件，`recipes-connectivity`、`recipes-multimedia` 以此类推。类别名即语义，别人扫一眼就知道该去哪找。

"归纳完了？"达哥不知什么时候站在他身后。

"嗯。`meta-tiger` 是 BSP layer，照 `meta-yocto-bsp` 的样子建：`conf/` 加三个 `recipes-*`？"

"对，但别贪多。"达哥说，"你现在只需要 `recipes-bsp`、`recipes-core`、`recipes-kernel` 三个。TF-A、U-Boot 以后进 `recipes-bsp`，linux-tiger 进 `recipes-kernel`，镜像配方和包组进 `recipes-core`——按你那张地图，哪一格将来归哪个目录，你心里应该有数。"

阿凯动手建目录。

```bash
# 创建 meta-tiger 的目录骨架
mkdir -p ~/workspace/meta-tiger/{conf,recipes-bsp,recipes-core,recipes-kernel}

# 预留后续章节的配置目录（本章保持为空，这是有意的）
mkdir -p ~/workspace/meta-tiger/conf/{machine,distro}
```

`conf/machine/` 和 `conf/distro/` 现在是两个空目录——`tiger-aarch64.conf` 要到 chapter 5 才写，`tiger-distro.conf` 更晚。先把位置占出来，是让 Fig-3-1 这棵目录树从今天起就是"最终形态"，后面每章只往里填文件、不再动结构。

还有一个技术细节：git 不跟踪空目录。如果什么都不放，首次 commit 时这些空目录会凭空消失，下个 clone 仓库的人看到的结构就缺胳膊少腿。惯例做法是在空目录里放一个 `.gitkeep` 占位文件。

```bash
# git 不跟踪空目录，先放占位文件
touch ~/workspace/meta-tiger/recipes-bsp/.gitkeep \
      ~/workspace/meta-tiger/recipes-core/.gitkeep \
      ~/workspace/meta-tiger/recipes-kernel/.gitkeep \
      ~/workspace/meta-tiger/conf/machine/.gitkeep \
      ~/workspace/meta-tiger/conf/distro/.gitkeep

# 查看建好的目录结构
find ~/workspace/meta-tiger | sort
```

输出：

```text
/home/<your-username>/workspace/meta-tiger
/home/<your-username>/workspace/meta-tiger/conf
/home/<your-username>/workspace/meta-tiger/conf/distro
/home/<your-username>/workspace/meta-tiger/conf/distro/.gitkeep
/home/<your-username>/workspace/meta-tiger/conf/machine
/home/<your-username>/workspace/meta-tiger/conf/machine/.gitkeep
/home/<your-username>/workspace/meta-tiger/recipes-bsp
/home/<your-username>/workspace/meta-tiger/recipes-bsp/.gitkeep
/home/<your-username>/workspace/meta-tiger/recipes-core
/home/<your-username>/workspace/meta-tiger/recipes-core/.gitkeep
/home/<your-username>/workspace/meta-tiger/recipes-kernel
/home/<your-username>/workspace/meta-tiger/recipes-kernel/.gitkeep
```

阿凯把它画成一张图，贴到全景地图旁边。

**Fig-3-1 meta-tiger 目录结构**

```text
~/workspace/meta-tiger/
├── conf/
│   ├── layer.conf          ← 3.2 节编写
│   ├── machine/            ← 空，chapter 5 填充（tiger-aarch64.conf）
│   └── distro/             ← 空，chapter 11 填充（tiger-distro.conf）
├── recipes-bsp/            ← 空，chapter 6/7 填充（TF-A、U-Boot）
├── recipes-core/           ← 空，后续填充（镜像配方、包组）
├── recipes-kernel/         ← 空，chapter 8 填充（linux-tiger）
├── README                  ← 3.3 节编写
├── COPYING.MIT             ← 3.3 节编写
└── MAINTAINERS             ← 3.3 节编写
```

### 3.2 编写 layer.conf

"目录是壳，`layer.conf` 才是身份证。"达哥说，"BitBake 加载一个 layer 时，第一件事就是找它的 `conf/layer.conf`。这个文件回答四个问题：我是谁、我的文件在哪、我依赖谁、我跟哪个版本的 Yocto 兼容。"

阿凯新建文件，达哥让他一个变量一个变量地写，边写边讲。

第一个是 `LAYERDIR`。这是 BitBake 的**层目录变量（LAYERDIR）**，内置变量，加载某个 layer 的 `layer.conf` 时自动指向该 layer 的顶层路径。layer.conf 里所有路径都基于它拼，绝不手写绝对路径——这样 layer 挪到任何机器、任何目录下都能工作。

第二个是 `BBPATH`，**BitBake 搜索路径（BBPATH）**，声明 BitBake 去哪些目录找 `.conf` 和 `.bbclass` 文件。layer 自己的目录必须追加进去，否则将来在 `conf/machine/` 里写了配置 BitBake 也找不到。

第三个是 `BBFILES`，**配方文件通配（BBFILES）**，声明本 layer 的 `.bb` 和 `.bbappend` 文件都在哪。注意它的通配结构是 `recipes-*/*/*.bb`——两条斜杠，一级是类别目录、一级是配方名目录，第三级才是配方文件。这个两级目录结构不是风格建议，是机制约定，3.6 节阿凯会在这上面摔一跤。

接下来是三个带 `_meta-tiger` 后缀的变量。阿凯写到这停下了："为什么这三个变量后面要挂 layer 名？"

"因为 BitBake 把所有 layer 的 layer.conf 读进同一个命名空间。"达哥说，"不挂后缀，变量就互相覆盖了。挂后缀，每个 layer 的声明各归各。"

- `BBFILE_COLLECTIONS` 是**层集合声明（BBFILE_COLLECTIONS）**，声明本 layer 的集合名。chapter 2 里 `bitbake-layers show-layers` 输出的第一列——`core`、`yocto`、`yoctobsp`——显示的就是这个名字，不是目录名。
- `BBFILE_PATTERN` 是**层路径模式（BBFILE_PATTERN）**，用正则界定"哪些路径下的文件算本集合的"。
- `BBFILE_PRIORITY` 在 chapter 2 已经讲过规则，这里落地取值 `6`——比三个官方层的 `5` 高一级，呼应全景地图上"meta-tiger 在最上层、覆盖下层"的位置。

阿凯又皱眉："`BBFILE_COLLECTIONS` 和 `BBFILE_PATTERN` 感觉是一回事，为什么要写两个？"

"一个声明'我是谁'，一个声明'我的文件在哪'。"达哥一句话点破，"名字是名字，地盘是地盘。将来你往一个 layer 里塞多个集合，就知道为什么分开了。"

最后两个变量：

- `LAYERDEPENDS` 是**层依赖声明（LAYERDEPENDS）**，声明本 layer 依赖哪些其他 layer。meta-tiger 只依赖 OE-Core，集合名 `core`。"以后集成 TF-A 的时候可能要加 `meta-arm`，"达哥补了一句，"到那一章再说，现在不加。"
- `LAYERSERIES_COMPAT` 是**层兼容系列（LAYERSERIES_COMPAT）**，声明本 layer 兼容哪个 Yocto 发布系列。本书锁定 Scarthgap 5.0，值就是 `scarthgap`。

完整文件如下，逐行注释。

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/layer.conf

# 本 layer 的 conf/ 和将来的 classes/ 目录，追加进 BitBake 搜索路径
BBPATH .= ":${LAYERDIR}"

# 本 layer 的配方按 recipes-<类别>/<配方名>/ 两级目录组织
# 同时通配 .bb（配方）和 .bbappend（追加文件）
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"

# 我是谁：集合名 meta-tiger（show-layers 第一列将显示这个名字）
BBFILE_COLLECTIONS += "meta-tiger"

# 我的文件在哪：本集合内文件的路径范围
BBFILE_PATTERN_meta-tiger = "^${LAYERDIR}/"

# 我的优先级：6，高于官方层 core/yocto/yoctobsp 的 5
# 为后续用 bbappend 覆盖下层配方预留能力（本章还用不上）
BBFILE_PRIORITY_meta-tiger = "6"

# 我依赖谁：只依赖 OE-Core
LAYERDEPENDS_meta-tiger = "core"

# 我兼容哪个 Yocto 发布系列：Scarthgap 5.0
LAYERSERIES_COMPAT_meta-tiger = "scarthgap"
```

**Fig-3-2 layer.conf 变量速查表**

| 变量 | 作用 | 本章取值 |
|------|------|---------|
| `BBPATH` | BitBake 搜索 `.conf` / `.bbclass` 的路径 | 追加 `${LAYERDIR}` |
| `BBFILES` | 本 layer 配方文件的通配路径 | `recipes-*/*/*.bb` + `*.bbappend` |
| `BBFILE_COLLECTIONS` | 集合名（"我是谁"） | `meta-tiger` |
| `BBFILE_PATTERN_meta-tiger` | 集合内文件的路径范围（"我的地盘"） | `^${LAYERDIR}/` |
| `BBFILE_PRIORITY_meta-tiger` | 层优先级，数值越大越优先 | `6` |
| `LAYERDEPENDS_meta-tiger` | 依赖的其他 layer | `core` |
| `LAYERSERIES_COMPAT_meta-tiger` | 兼容的 Yocto 发布系列 | `scarthgap` |

写完，达哥给了阿凯一个自查作业："别急着信我。去把 `meta-poky` 的 layer.conf 打开，逐行对照你写的，看差在哪。"

```bash
# 对照官方 layer 的写法，检查自己的 layer.conf
cat ~/workspace/poky/meta-poky/conf/layer.conf
```

输出：

```bitbake
# We have a conf and classes directory, add to BBPATH
BBPATH =. "${LAYERDIR}:"

# We have recipes-* directories, add to BBFILES
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"

BBFILE_COLLECTIONS += "yocto"
BBFILE_PATTERN_yocto = "^${LAYERDIR}/"
BBFILE_PRIORITY_yocto = "5"

LAYERSERIES_COMPAT_yocto = "scarthgap"

# This should only be incremented on significant changes that will
# cause compatibility issues with other layers
LAYERVERSION_yocto = "3"

LAYERDEPENDS_yocto = "core"

REQUIRED_POKY_BBLAYERS_CONF_VERSION = "2"
```

阿凯逐行核对了三件事：变量骨架完全一致；官方层多了 `LAYERVERSION`（layer 重大变更时递增的版本号，可选，meta-tiger 暂时不需要）和 `REQUIRED_POKY_BBLAYERS_CONF_VERSION`（Poky 私有检查项，自家 layer 不用写）。还有一个细节：`meta-poky` 的 `BBPATH` 是把 `${LAYERDIR}` **前置**（`=.`），`meta-yocto-bsp` 是**后置**（`.=`），两种写法都合法，meta-tiger 跟随 `meta-yocto-bsp` 的后置写法。

"骨架对上了。"阿凯说。

"嗯。但 layer.conf 写对没有，最终要 BitBake 说了算。"达哥提醒，"先别急着注册，把文档三件套补齐，一次到位。"

### 3.3 README、COPYING.MIT 与 MAINTAINERS

"三个纯文本文件，技术含量为零，为什么非要现在写？"阿凯问。

达哥只说了一句："三年后接手你这个 layer 的人，第一眼看的不是 layer.conf。"

阿凯想了想 chapter 2 里那场"三年后"的讨论，没再追问。第一个文件是 README——这个 layer 是什么、依赖什么、怎么用、补丁往哪提、找谁。

```text
# 文件路径：~/workspace/meta-tiger/README

meta-tiger
==========

meta-tiger 是 tiger 开发板（ARM Cortex-A53）的 Yocto BSP layer，
提供 MACHINE 配置、启动链集成（TF-A / U-Boot / linux-tiger）
以及 tiger 专用的镜像配方与包组。

当前状态：骨架阶段，仅包含 layer 配置与文档，配方将随后续开发逐步加入。

Dependencies
============

- OE-Core（Poky 中的 meta 层），scarthgap 分支

Adding the meta-tiger layer to your build
=========================================

在构建目录下执行：

    bitbake-layers add-layer <path-to-meta-tiger>

或将 meta-tiger 的绝对路径手动加入 build/conf/bblayers.conf 的 BBLAYERS。

Patches
=======

提交补丁或问题反馈，请发送邮件至维护者邮箱，
邮件主题请注明 [meta-tiger]。

Maintainer
==========

阿凯 <kai@<your-company>.com>（职责分工详见 MAINTAINERS 文件）
```

Patches 和 Maintainer 这两节不是客套话——3.4 节的官方体检工具会真的打开这个文件，检查里面有没有维护者、补丁提交方式和联系邮箱。先按社区标准模板写齐，省得将来返工。

第二个文件是许可证。OE 社区的惯例是：layer 整体的许可证文本放在 `COPYING.MIT`，取 MIT 许可证——OE-Core 本体（`meta/COPYING.MIT`）、`meta-arm` 等官方层都是这么做的。BSP layer 选宽松许可证是行业惯例，方便任何人在此基础上叠加自己的 layer；至于将来每个配方各自的许可证，由配方文件里的 `LICENSE` 字段单独声明，和这个文件是两回事。

```text
# 文件路径：~/workspace/meta-tiger/COPYING.MIT

MIT License

Copyright (c) 2026 tiger 项目维护团队

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

第三个文件是 MAINTAINERS，服务于"交接与问责"：哪个目录、哪个文件归谁管，出了问题找谁。现在团队只有两个人，内容很短，但这个文件会随 layer 长大。

```text
# 文件路径：~/workspace/meta-tiger/MAINTAINERS

meta-tiger 维护者名单
=====================

Maintainer: 阿凯 <kai@<your-company>.com>
Reviewer:   达哥 <zhou@<your-company>.com>

职责说明：
- Maintainer 负责 layer 的日常开发与提交
- Reviewer 负责变更审核，重大结构调整需 Reviewer 确认
```

> **💡 提示**：真实项目里把 `<your-company>` 换成你的实际邮箱域名。MAINTAINERS 的价值不在第一天，而在第三年的某个深夜——有人要改启动链集成，先查这个文件就知道该找谁。

### 3.4 注册与验证：让 BitBake 认识它

骨架齐了。阿凯问出今天最关键的问题："怎么证明 bitbake 真的认识它了？"

达哥反问："你前两天用什么命令看过 layer 列表？"

阿凯立刻反应过来——`bitbake-layers show-layers`，chapter 1 里查过三个官方层，chapter 2 里又用它追过 `core-image-minimal` 的归属。要让 meta-tiger 出现在那个列表里，得先注册。

```bash
# 初始化构建环境（在 build 目录下执行后续命令）
cd ~/workspace/poky
source oe-init-build-env ../build

# 把 meta-tiger 注册进构建（自动改写 bblayers.conf）
bitbake-layers add-layer ~/workspace/meta-tiger
```

`add-layer` 做的事很单纯：把 layer 的绝对路径追加进 `build/conf/bblayers.conf` 的 `BBLAYERS`。比手改文件安全，不会写错引号和换行。但它还有一层不那么显眼的行为：写入之后，它会立刻做一次完整的配置解析校验，新加的 layer 有任何问题都会当场报错并自动回滚——3.6 节阿凯会亲身体验到这个设计的好处。

```bash
# 验证 1：确认 bblayers.conf 追加了 meta-tiger
cat ~/workspace/build/conf/bblayers.conf
```

输出（关键行）：

```bitbake
BBLAYERS ?= " \
  /home/<your-username>/workspace/poky/meta \
  /home/<your-username>/workspace/poky/meta-poky \
  /home/<your-username>/workspace/poky/meta-yocto-bsp \
  /home/<your-username>/workspace/meta-tiger \
  "
```

```bash
# 验证 2：确认 BitBake 识别了 meta-tiger，且 priority 为 6
bitbake-layers show-layers
```

输出：

```text
NOTE: Starting bitbake server...
layer                path                                                                               priority
========================================================================================================
core                 /home/<your-username>/workspace/poky/meta                                           5
yocto                /home/<your-username>/workspace/poky/meta-poky                                      5
yoctobsp             /home/<your-username>/workspace/poky/meta-yocto-bsp                                 5
meta-tiger           /home/<your-username>/workspace/meta-tiger                                          6
```

`meta-tiger` 出现在列表里，`BBFILE_COLLECTIONS` 声明的名字进了第一列，priority 是 `6`——layer.conf 里写的每一个变量都兑现了。

```bash
# 验证 3：看看 meta-tiger 贡献了什么配方
bitbake-layers show-recipes | grep -i tiger
```

输出：

```text
# （无任何输出）
```

> **⚠️ 注意**：`show-recipes` 里找不到 meta-tiger 的条目是**预期结果**，不是错误——这是一个空 layer，还没有任何配方。chapter 6 集成 TF-A 之后，这里才会出现内容。

最后是闭环验收：空 layer 加入后，原来的构建不能受影响。重新构建一次 `core-image-minimal`。

```bash
# 验证 4：重新构建，确认空 layer 无副作用（Sstate 缓存命中，秒级完成）
bitbake core-image-minimal
```

输出（关键行，版本号与任务数以本地实际输出为准）：

```text
Loading cache: 100% |############################################| Time: 0:00:00
NOTE: Resolving any missing task queue dependencies

Build Configuration:
BB_VERSION           = "2.8.1"
BUILD_SYS            = "x86_64-linux"
MACHINE              = "qemuarm64"
DISTRO               = "poky"
DISTRO_VERSION       = "5.0.18"
# ... (省略)

NOTE: Tasks Summary: Attempted 4059 tasks of which 4059 didn't need to be rerun and all succeeded.
```

全部任务命中 Sstate 缓存，一个都不用重跑——任务数和 chapter 1 首次构建时一模一样。空 layer 进了 `BBLAYERS`，但没有给构建世界增加任何配方、没有改变任何签名——这就是"里面什么都不放"的严格含义。

还有一个可选但建议养成的动作：官方体检。`yocto-check-layer` 是**层检查命令（yocto-check-layer）**，Yocto 官方提供的 layer 规范检查工具，会验证 layer 的结构、可解析性、兼容性声明等一系列项目。以后每次对 meta-tiger 做大改动，都应该跑一遍。

有一个使用前提要知道：它要求被检查的 layer **不在**当前构建的 `BBLAYERS` 里——它要自己控制 layer 的加载过程，才能测出"加上这个 layer 会不会改变世界"。所以先临时移除，检查完再加回来。

```bash
# 官方体检前，先临时移除 meta-tiger（yocto-check-layer 要求被测 layer 不在 BBLAYERS 中）
bitbake-layers remove-layer ~/workspace/meta-tiger

# 运行官方 layer 规范检查（首次运行需获取全量 world 签名，可能需要十几分钟）
yocto-check-layer ../meta-tiger
```

输出（关键行，以本地实际输出为准）：

```text
INFO: Detected layers:
INFO: meta-tiger: LayerType.SOFTWARE, /home/<your-username>/workspace/meta-tiger
INFO:
INFO: Setting up for meta-tiger(LayerType.SOFTWARE), /home/<your-username>/workspace/meta-tiger
INFO: Getting initial bitbake variables ...
INFO: Getting initial signatures ...
INFO: Starting to analyze: meta-tiger
INFO: ----------------------------------------------------------------------
INFO: test_layerseries_compat (common.CommonCheckLayer.test_layerseries_compat) ... ok
INFO: test_parse (common.CommonCheckLayer.test_parse) ... ok
INFO: test_readme (common.CommonCheckLayer.test_readme) ... ok
INFO: test_signatures (common.CommonCheckLayer.test_signatures) ... ok
# ... (省略)
INFO:
INFO: Summary of results:
INFO:
INFO: meta-tiger ... PASS
```

三个细节值得注意。第一，单项测试通过时打印的是 `ok`，`PASS` 只出现在最后的汇总行——以后 grep 检查结果时别搜错词。第二，meta-tiger 被识别为 `SOFTWARE` 类型而不是 `BSP` 类型——因为 `conf/machine/` 还是空的，等 chapter 5 写了 `tiger-aarch64.conf`，它就会被识别为 BSP layer 并追加 BSP 专项检查。第三，`test_readme` 是真实存在的检查项：工具会打开 README，检查里面有没有维护者、补丁提交方式和邮箱——3.3 节那份 README 里的 Patches 小节和带邮箱的 Maintainer 行，就是为它准备的。

```bash
# 检查通过，把 meta-tiger 加回构建
bitbake-layers add-layer ~/workspace/meta-tiger
```

> **📖 深入阅读**：`yocto-check-layer` 背后是 Yocto 官方的 "Yocto Project Compatible" 兼容徽章计划，社区里挂这个徽标的 layer 都通过了这套检查。具体检查项定义见 Poky 源码 `scripts/lib/checklayer/` 目录。

### 3.5 初始化 git 仓库与首次 commit

达哥验收完 `show-layers` 的输出，说了今天最长的一段话："还剩最后一步，也是最容易被新人跳过的一步。`meta-tiger` 从今天起是一个独立 git 仓库——五个仓库里的第五个，跟 `poky` 平级，不归 poky 管。为什么第一天就进 git？因为后面每一章的产物都要用 tag 锚定，读者——包括三个月后的你——要能随时 checkout 回任意一章结束时的状态。裸目录做不到这一点。"

阿凯点头。前两章的 tag 打在 `poky` 仓库，是因为改动都发生在 poky 的构建里；本章起，`meta-tiger` 有了自己的仓库，tag 改打在这里。

```bash
# 初始化 meta-tiger 为独立 git 仓库，默认分支 main
cd ~/workspace/meta-tiger
git init -b main

# 暂存全部文件：layer.conf、文档三件套、各空目录的 .gitkeep
git add -A

# 检查暂存内容，确认没有把临时文件混进来
git status

# 首次提交
git commit -m "Initial meta-tiger layer skeleton"

# 确认提交历史
git log --oneline
```

输出：

```text
# git log --oneline（提交哈希以本地实际为准）
a1b2c3d Initial meta-tiger layer skeleton
```

> **⚠️ 注意**：本章的 tag 打在 `~/workspace/meta-tiger` 仓库。`poky` 本章没有任何改动（`bblayers.conf` 属于 `build/` 目录，不进版本库），不打新 tag。

### 3.6 踩坑实录

达哥上午临走时的预言应验了。两个坑，都不在命令上，在"想当然"上。

#### 3.6.1 踩坑 1：LAYERSERIES_COMPAT 大小写写错，add-layer 当场拒绝

先交代时间线：这事发生在 3.2 写完、3.4 注册之前——你在前两节看到的 layer.conf 和一路顺利的注册流程，都是修正之后的样子。

当时阿凯写 layer.conf，把兼容系列写成了 `"Scarthgap"`——首字母大写。理由很充分："发行版名字嘛，专有大写，文档里不都这么印的？"

我们来重演这个错误。阿凯先把 meta-tiger 从构建里摘出来，回到 3.4 注册之前的状态，再重演那个写错的下午。

```bash
# 先回到注册之前的状态：把 meta-tiger 从构建里移除（在 build 目录下执行）
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers remove-layer ~/workspace/meta-tiger

# 重演当时的错误：把正确的小写值改成首字母大写的 "Scarthgap"（错误示范，请勿模仿）
sed -i 's/"scarthgap"/"Scarthgap"/' ~/workspace/meta-tiger/conf/layer.conf

# 注册 layer
bitbake-layers add-layer ~/workspace/meta-tiger
```

输出（这是"你可能遇到的错误"，尾部输出以本地实际为准）：

```text
ERROR: Layer meta-tiger is not compatible with the core layer which only supports these series: scarthgap (layer is compatible with Scarthgap)
ERROR: Parse failure with the specified layer added, exiting.
```

错误在 `add-layer` 执行当场就炸了，根本没有"注册成功、以后再说"的机会。原因在 3.4 节埋过：`add-layer` 不是简单地往 `bblayers.conf` 里追加一行文本，它写入之后会立刻做一次完整的配置解析校验；校验不过，它会把 `bblayers.conf` 回滚到添加前的状态再退出。不兼容的 layer 连门都进不来。

报错第一行其实说得很直白：core 层只认 `scarthgap`，而你声明的是 `Scarthgap`。这个变量是**字符串精确匹配**，比较的是发行版代号的小写形式，不是印给人看的标题——官方层的 layer.conf 里清一色小写，写的时候照抄就不会错。

再确认一下回滚的效果——meta-tiger 此刻并没有被注册进去：

```bash
# 确认 bblayers.conf 已被回滚：meta-tiger 不在 layer 列表中
bitbake-layers show-layers
```

输出：

```text
layer                path                                                                               priority
========================================================================================================
core                 /home/<your-username>/workspace/poky/meta                                           5
yocto                /home/<your-username>/workspace/poky/meta-poky                                      5
yoctobsp             /home/<your-username>/workspace/poky/meta-yocto-bsp                                 5
# meta-tiger 不在列表中——add-layer 校验失败，bblayers.conf 已自动回滚
```

```bash
# 修正：改回小写 scarthgap（与版本锁定一致）
sed -i 's/"Scarthgap"/"scarthgap"/' ~/workspace/meta-tiger/conf/layer.conf

# 重新注册并验证（输出同 3.4 节验证 2，meta-tiger 回到列表，priority 6）
bitbake-layers add-layer ~/workspace/meta-tiger
bitbake-layers show-layers
```

教训有两条。一是 `LAYERSERIES_COMPAT` 照抄官方层的小写代号，不要凭"专有大写"的直觉。二是 layer.conf 改完立刻验证——好在 `add-layer` 自带校验，这个坑连潜伏期都没有，当场暴露、当场回滚、当场修。

#### 3.6.2 踩坑 2：recipe 少一级目录，BitBake 找不到配方

第二个坑来自阿凯的一个合理疑问："空 layer 真的能装配方吗？口说无凭。"

他写了个最小探针配方 `tiger-probe_0.1.bb`，只有三行声明，随手放在 `recipes-bsp/` 下。

```bash
# 创建最小探针配方（故意放错位置：直接放在类别目录下，少了一级"配方名"目录）
cat > ~/workspace/meta-tiger/recipes-bsp/tiger-probe_0.1.bb << 'EOF'
SUMMARY = "临时探针配方：验证 meta-tiger 的配方能被 BitBake 发现"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
EOF
```

三行里后两行是第一次见：`LICENSE` 声明许可证名，`LIC_FILES_CHKSUM` 指向许可证文本并校验内容，两者成对出现、每个配方必备。许可证机制后面有专门章节，这里照抄模板即可——它们不是本坑的重点。

```bash
# 注册 layer 后查找探针配方
cd ~/workspace/poky
source oe-init-build-env ../build
bitbake-layers show-recipes tiger-probe
```

输出：

```text
# （完全没有任何输出——连 === Matching recipes: === 的标题都不打印）
```

找不到。而且注意这个细节：不是"列出了标题但下面为空"，是命令静默返回，一个字都没有——`=== Matching recipes: ===` 这个标题只在有匹配项要打印时才出现。阿凯把文件翻来覆去检查了三遍，语法没错、路径没拼错。达哥路过，没看配方，指了指 layer.conf 里那行 `BBFILES`：

"`recipes-*/*/*.bb`。两条斜杠，你数清楚了吗？"

阿凯数了：第一条斜杠分隔**类别目录**（`recipes-bsp`），第二条分隔**配方名目录**，第三级才是配方文件。他少建了中间一级。

```bash
# 修正：补出"配方名"一级目录，把配方挪进去
mkdir -p ~/workspace/meta-tiger/recipes-bsp/tiger-probe
mv ~/workspace/meta-tiger/recipes-bsp/tiger-probe_0.1.bb \
   ~/workspace/meta-tiger/recipes-bsp/tiger-probe/

# 重新查找
bitbake-layers show-recipes tiger-probe
```

输出：

```text
=== Matching recipes: ===
tiger-probe:
  meta-tiger           0.1
```

找到了。`BBFILES` 的通配结构决定了目录约定的**强制性**——目录层级不是风格问题，是机制问题：不符合 `recipes-*/*/*.bb` 这个模式的文件，BitBake 压根不会去看。刚才那次"一个字都没有"的静默返回，就是"压根没去看"的直接证据。

验证完毕，收尾动作同样重要：探针配方是验证手段，不是交付内容。达哥开场的承诺是"里面什么都不放"，这个承诺要兑现。

```bash
# 删除探针配方，恢复空 layer 的承诺
rm -rf ~/workspace/meta-tiger/recipes-bsp/tiger-probe

# 确认工作区干净（探针配方尚未 commit，删除后不留痕迹）
cd ~/workspace/meta-tiger
git status
```

输出：

```text
nothing to commit, working tree clean
```

> **🔥 重要**：验证手段不等于交付内容。为了验证机制临时加的东西，验证完要删掉、确认工作区干净再提交——否则半年后没人说得清 `tiger-probe` 是干什么用的。

### 3.7 本章小结

一天结束，阿凯的五件事全部落地：

- **3.1**：layer 目录有社区惯例——`meta-<名字>` 命名、`conf/` 必含 `layer.conf`、配方按 `recipes-<类别>` 分类；meta-tiger 建了 `conf/` + 三个 `recipes-*` 目录，空目录用 `.gitkeep` 占位。
- **3.2**：`layer.conf` 是 layer 的身份证，七个变量回答四个问题——我是谁（`BBFILE_COLLECTIONS`）、我的文件在哪（`BBPATH` / `BBFILES` / `BBFILE_PATTERN`）、我排第几（`BBFILE_PRIORITY = "6"`）、我依赖谁、兼容谁（`LAYERDEPENDS` / `LAYERSERIES_COMPAT`）。
- **3.3**：README、COPYING.MIT、MAINTAINERS 是工程门面，技术含量为零，交接价值最高——而且 README 里的维护者和补丁信息会被官方体检工具真的读。
- **3.4**：`bitbake-layers add-layer` 注册，四重验证闭环——`bblayers.conf` 追加、`show-layers` 可见（priority 6）、`show-recipes` 为空（预期）、重建 `core-image-minimal` 全部命中缓存；`yocto-check-layer` 官方体检 PASS。
- **3.5**：`meta-tiger` 成为第五个独立 git 仓库，首次 commit 完成。
- **3.6**：两个坑都源于"想当然"——`LAYERSERIES_COMPAT` 是精确匹配的小写代号，写错会在 `add-layer` 当场暴露并回滚；`BBFILES` 的 `recipes-*/*/*.bb` 决定了配方必须放在两级目录下。

现在阿凯手里有一个能被 BitBake 识别、通过了官方体检、但空无一物的 layer。那些空目录不是缺陷，是七张等着兑现的期票：`conf/machine/` 等 chapter 5 的 `tiger-aarch64.conf`，`conf/distro/` 等 chapter 11 的 `tiger-distro.conf`，`recipes-bsp/` 等 TF-A 和 U-Boot，`recipes-kernel/` 等 linux-tiger，`BBFILE_PRIORITY = "6"` 等着第一次 bbappend 覆盖。

后续任务清单：

- **task 05 / chapter 4**：让 QEMU 长出 tiger 这块板。
- **task 06 / chapter 5**：写第一份 MACHINE 配置，填充本章留空的 `conf/machine/`。
- 远景：task 07–09 将逐个填充 `recipes-bsp/` 和 `recipes-kernel/`。

达哥收拾东西下班，在门口留下一句："架子搭好了。下一章我们去给 QEMU 动手术，让它长出 tiger 这块板。"

阿凯在 `meta-tiger` 仓库打上本章的 tag。

```bash
# 在 meta-tiger 仓库标记本章终点（注意：不是 poky 仓库）
cd ~/workspace/meta-tiger
git tag chapter3
```

> **💡 提示**：从本章起，章末 tag 都打在 `meta-tiger` 仓库；`poky` 作为上游参考保持不动，本章无改动、不打新 tag。
