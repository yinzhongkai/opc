## 15 修一个上游 bug

周四上午，SDK 发出去的第三天。阿凯刚把应用组的一轮使用问题回完，QA 组的小林拿着平板过来了，屏幕上是昨晚 CI 跑出来的报告——testimage 那一栏红着一条。

"watchdog 的边界用例挂了。"小林把报告递过来，"超时设到规格书上写的最大值，板子不复位。我们查了三天，测试脚本没问题，像是系统的事。"

阿凯把报告抄到白板上：

```text
── QA 报障单（复述）────────────────────────────
用例：watchdog 边界超时复位
步骤：watchdog -t 10 -T 131072 /dev/watchdog，停止喂狗
预期：超时后板子复位
实际：板子不复位，看门狗像没开一样
对照：-T 60（常用值）复位正常
环境：tiger-image，dev 态，CI 每晚构建
────────────────────────────────────────────────
```

"我先在板上复现一下，确认不是我们集成错了。"阿凯说。

"不用复现我也知道八成是什么。"老周路过白板，扫了一眼，"tiger 的看门狗用的是上游驱动，我们一行都没改过。先给你把背景补齐——"

他在白板角落画了三层：

```text
┌─ QEMU tiger machine ────────────────────────────────┐
│  Watchdog 简化模型（寄存器兼容 ARM SP805）            │
│  MMIO 0x09030000，时钟 32768 Hz，32 位递减计数器      │
└───────────────────────▲─────────────────────────────┘
                        │ 设备树节点 compatible = "arm,sp805"
┌───────────────────────┴─────────────────────────────┐
│  内核：上游 sp805_wdt 驱动（drivers/watchdog/ 下）    │
│  向 watchdog 核心注册，透出 /dev/watchdog            │
└───────────────────────▲─────────────────────────────┘
                        │ ioctl 设超时 / 写入即喂狗
┌───────────────────────┴─────────────────────────────┐
│  用户空间：busybox watchdog 小程序                   │
└─────────────────────────────────────────────────────┘
```

**看门狗（Watchdog）** 本章正式引入：看门狗定时器，系统异常时自动复位恢复的硬件机制——软件要周期性地"喂狗"（往 `/dev/watchdog` 写数据），喂狗中断超过超时值，硬件就强制复位整板。前史注记一句：tiger 的硬件清单里它一直在列（QEMU 侧的简化模型是 qemu-tiger 仓库的一部分），但正文从未让它登场——今天它带着一个 bug 登场了。

三层各司其职：QEMU 模型扮演硬件，寄存器布局兼容 ARM 的 SP805 看门狗 IP 核；因为兼容，内核侧不用写新驱动，设备树里一句 `compatible = "arm,sp805"` 就认领了上游的 `sp805_wdt` 驱动；用户空间只认 `/dev/watchdog` 这个标准接口。图里那句"ioctl 设超时"——`ioctl` 是用户空间对设备文件发控制命令的系统调用，设超时走它，喂狗则是普通写。QA 的用例挂在"设边界超时"这一步——问题要么在驱动，要么在模型。

"模型是固件组照 SP805 手册写的，驱动是上游几千人 review 过的。"阿凯说，"哪个更可疑？"

"都不许猜。"老周摆手，"复现、读代码、定位，按顺序来。而且我把话放这儿：**如果最后定位下来是上游驱动的问题，这就是你第一次走完整闭环——发现、修、提交回上游、集成到产品。IoT 产品要长期推安全补丁，这是基本功。**"

他在白板上列了四步：**拉源码、修、导回产品层、提交上游**。

"等下。"阿凯举手，"提交上游到上游合并，中间隔着多久没人知道。这段日子里 patch 在 meta-tiger 里怎么养？内核一升级它会不会失效？"

"问得好，第五步你自己加上去了。"老周点点头，没有展开——给自己加任务这个习惯，如今已经是他的本能反应，"开工。第一步，想想 chapter 8 你是怎么改内核的。"

### 15.1 用 devtool modify 拉出 kernel 源码

#### 15.1.1 认账：chapter 8 的笨办法

chapter 8（8.11.1）阿凯踩过的坑还历历在目：直接改 `tmp/work/` 下的工作区源码，次日 `do_unpack` 一重跑，手改静默消失。当时立的纪律是"回仓库改、提交、bump SRCREV"——改一行打印也要走完整套，等一次 fetch + unpack + 全量编译。那套流程是对的，但重。

当时 8.9 节的 💡 框里点过一个名字：devtool"能给配方开一块独立的开发工作区"。14.6 在 eSDK 里用 `devtool add` 收编 hello-tiger 时， externalsrc 机制已经见过实物。今天才是 devtool 的主战场：**`devtool modify`——把一个既有配方的源码拉进工作区改，`devtool finish`——把修改导出为 patch 收回层里**。

> **💡 提示**：modify 和 add 的分工——`devtool add` 无中生有（新组件、新配方，14.6 的 hello-tiger）；`devtool modify` 改造既有配方（源码已经在构建里的东西，比如今天的内核）。同一个工作区机制，两个入口。

注意执行地点：本章的 devtool 在**主构建环境**里跑（`~/workspace/build`），不是 eSDK——改的是产品层自己的配方，不是模拟"拿到 eSDK 的人"。14.4.3 装 eSDK 时立过的规矩继续有效、方向反过来用：注入过 SDK/eSDK 环境脚本的 shell 和 source 过 oe-init-build-env 的主构建会话各开各的终端，别混——而本章的 devtool/bitbake 命令，全部跑在 source 过 oe-init-build-env 的主构建会话里。

```bash
# 初始化主构建环境（本章所有 devtool/bitbake 命令都在这个会话里跑）
cd ~/workspace/poky
source oe-init-build-env ../build

# 把 linux-tiger 的源码拉进 devtool 工作区
devtool modify linux-tiger
```

输出（关键行）：

```text
NOTE: Starting bitbake server...
INFO: Source tree extracted to /home/<your-username>/workspace/build/workspace/sources/linux-tiger
INFO: Recipe linux-tiger now set up to build from /home/<your-username>/workspace/build/workspace/sources/linux-tiger
```

没有报错就是成了。`devtool modify` 在背后做的事：按配方声明把源码 fetch + unpack 到工作区（而不是 `tmp/work/`），就地初始化成一个 git 仓库，再挂一个 bbappend 让构建系统改从工作区取源码。

<!-- 【待验证·阻塞级】C-W31：devtool modify/finish 在"配方本就在 meta-tiger"情形下的真实输出逐字、workspace 目录实物、bbappend 逐字形态，待 linux-tiger 仓库就位后实测回填，续 C-W11~C-W16 批同口径。本章 devtool 行为描述已对照 Scarthgap 本地 poky 源码裁决（standard.py），输出文案为源码级推定的示意形态；V31（standard.py 行号区间校准）、V33（git log 装饰形态：devtool-base/devtool-patched 的 tag 前缀与 origin 远端有无）同批回填。 -->

#### 15.1.2 工作区解剖

**devtool 工作区（workspace）** 本章正式引入：devtool 在构建目录下开出的一块独立开发区——`build/workspace/` 本身就是一个 layer（有自己的 `conf/layer.conf`），配方被"接管"期间，它的源码和追加文件都住在这里。拆开看：

```bash
# 工作区结构
ls ~/workspace/build/workspace/
```

输出：

```text
appends  conf  sources
```

```bash
# 被接管的配方源码
ls ~/workspace/build/workspace/sources/
```

输出：

```text
linux-tiger
```

`workspace/sources/linux-tiger` 就是内核源码树——而且是个 git 仓库：

```bash
# 工作区源码树的 git 状态
cd ~/workspace/build/workspace/sources/linux-tiger
git branch
git log --oneline -3
git status
```

输出（关键行）：

```text
* devtool
7d2e9b4 (HEAD -> devtool, tag: devtool-patched, tag: devtool-base, origin/main) tiger: add board support
...
nothing to commit, working tree clean
```

三个名字认一下：`devtool` 是工作分支，你的修改提交在它上面；`devtool-base` 是基线标记——配方 SRCREV 指的那个提交，devtool 往后靠它算"哪些提交是你加的"；`devtool-patched` 标记的是"配方里的 patch 应用完之后"的位置（linux-tiger.bb 目前还没有 file:// patch，所以它和基线重叠）。工作区干净，可以开工。

接管的另一头在 `workspace/appends/`：

```bash
# 工作区生成的接管件
cat ~/workspace/build/workspace/appends/linux-tiger.bbappend
```

输出（全文）：

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
FILESPATH:prepend := "/home/<your-username>/workspace/build/workspace/sources/linux-tiger/oe-local-files:"
# srctreebase: /home/<your-username>/workspace/build/workspace/sources/linux-tiger

inherit externalsrc
# NOTE: We use pn- overrides here to avoid affecting multiple variants in the case where the recipe uses BBCLASSEXTEND
EXTERNALSRC:pn-linux-tiger = "/home/<your-username>/workspace/build/workspace/sources/linux-tiger"
SRCTREECOVEREDTASKS = "do_validate_branches do_kernel_checkout do_fetch do_unpack do_kernel_configcheck"

do_patch[noexec] = "1"

do_configure:append() {
    cp ${B}/.config ${S}/.config.baseline
    ln -sfT ${B}/.config ${S}/.config.new
}

do_kernel_configme:prepend() {
    if [ -e ${S}/.config ]; then
        mv ${S}/.config ${S}/.config.old
    fi
}

# initial_rev: 7d2e9b4f1a2c3d4e5f60718293a4b5c6d7e8f9a0
```

和 14.6.2 hello-tiger 那份三行 bbappend 对照着看：核心同样是 `inherit externalsrc` + `EXTERNALSRC` 指本地目录——构建不再 fetch/unpack 源码，直接用工作区这棵树。内核配方多出几行特有的，逐行认：

- `SRCTREECOVEREDTASKS`：把 `do_fetch`、`do_unpack` 一族任务标记为"工作区已代办"——构建时这些任务直接跳过。这行是 15.6 一个坑的伏笔，先记住它的长相。
- `do_patch[noexec] = "1"`：同理，配方里的 patch 已经在抽取时作为提交落进工作区了，构建期不再重复打。
- `do_configure:append` 那段：每次配置后把 `.config` 备份成 `.config.baseline` 并软链 `.config.new`——devtool 靠这两份 diff 出内核配置改动（14.6 没走到这一步，认得即可）。
- `do_kernel_configme:prepend` 那段：配置前把已有的 `.config` 挪成 `.config.old` 备用——`do_kernel_configme` 是 kernel-yocto 类配方才有的任务，linux-tiger 是普通 kernel 配方，这段等于空挂，认得即可。
- 末尾 `# initial_rev` 注释：基线提交哈希，和 `devtool-base` 标记同一件事的两种记法。

> **⚠️ 注意**：工作区接管期间，`tmp/work/` 下那份内核源码不再被使用——改它、看它都会得到过时内容。认准 `workspace/sources/linux-tiger` 这一个现场。

### 15.2 复现、调试、修复

#### 15.2.1 板上复现：先立对照组

动手之前先补记一笔准备动作：QA 脚本里的 `watchdog` 命令，产品镜像里得先有。OE-Core 的 busybox 默认配置没编这个小程序（`# CONFIG_WATCHDOG is not set`），受理报障当天阿凯就在 meta-tiger 里补了一份配置片段——`recipes-core/busybox/busybox/watchdog.cfg` 一行 `CONFIG_WATCHDOG=y`，配一份 `busybox_%.bbappend`，用 `FILESEXTRAPATHS` + `SRC_URI` 把它挂进 busybox 配方（4.3.2 的手法）；busybox 配方的 do_configure 会拿 merge_config（内核树的配置合并脚本，busybox 配方复用了它）把片段合进默认配置。片段当天合入，昨晚 CI 构建的镜像里命令已经就位。

<!-- 【待验证】busybox watchdog.cfg 片段与 busybox_%.bbappend 的逐字形态（含 do_configure merge_config 机制措辞）随 C-W30 批实测回填确认。 -->

按 QA 的步骤在板上走一遍。先起板（总装镜像还是 chapter 10 那条流水线，昨晚 CI 构建的产物）：

```bash
# 启动 tiger 全链（主构建环境会话）
runqemu tiger-aarch64 nographic
```

板上登录后，先立**对照基线**——常用超时值，确认看门狗基本功能正常：

```bash
# 对照组：60 秒超时，每 10 秒喂一次狗
# -F 前台跑、& 挂后台——busybox 的 watchdog 默认 daemonize，不加 -F 就没有作业可杀
watchdog -F -t 10 -T 60 /dev/watchdog &

# 模拟系统挂死：用 SIGKILL 杀掉喂狗进程
kill -9 %1
```

输出（约 60 秒后，串口日志关键行）：

```text
[  187.331205] watchdog: watchdog0: watchdog did not stop!
NOTICE:  BL1: tiger platform initializing
...
```

为什么用 `-9` 而不是普通 `kill`？普通 `kill` 发的是 SIGTERM，busybox 的 watchdog 接得住——它收到 SIGTERM 会往 `/dev/watchdog` 写一个 magic 字符 'V'，把狗**干净地停掉**，板子反而不复位，"挂死"就演不成了。`-9`（SIGKILL）它接不住：进程暴毙、没人写 'V'，内核这才会报 "watchdog did not stop!" 并继续计数到复位。板子复位、BL1 重新报到——基线正常，和 QA 报告一致。再打边界值：

```bash
# 实验组：边界超时值 131072 秒（规格书写的最大值）
watchdog -F -t 10 -T 131072 /dev/watchdog &

# 看门狗的递减计数器此刻的值（WDTVALUE 寄存器，基地址 + 0x004）
# devmem：busybox 自带的物理内存读写小工具，参数是地址和位宽，板上读寄存器最直接的家伙
devmem 0x09030004 32

# 杀掉喂狗进程，等复位
kill -9 %1
```

输出：

```text
0x00000000
```

计数器是 0。喂狗进程杀掉再等——板子安安静静，复位永远不来了。QA 的报障坐实。

阿凯盯着那个 `0x00000000` 看了半天。对照组里同一个寄存器是什么表现？回头补一拍：

```bash
# 回到对照组看一眼正常形态：设 60 秒超时后立即读计数器
watchdog -F -t 10 -T 60 /dev/watchdog &
devmem 0x09030004 32
sleep 1
devmem 0x09030004 32

# 看完收工：普通 kill（SIGTERM）——它接住信号、写 magic 字符干净停狗
kill %1
```

输出：

```text
0x001DF2C0
0x001D6E40
```

正常形态：计数器从装载值（60 秒 × 32768 Hz ≈ 0x1E0000）开始往下掉，一秒钟少了约 0x8480——和 32768 Hz 的时钟对得上。边界值的形态：装载进去的就是 0，计数器死在原地。

"报障成立，下一步呢？"老周不知何时站在身后。

"读驱动？"

"读驱动。"老周点头，然后只给了一句话，"**看驱动里 timeout 是怎么算出来的，再看它报给上层的是多少。两个数对上对不上，你自己算。**"

<!-- 【待验证·阻塞级】C-W30：tiger 组 watchdog 全链实测（复现/对照基线/复验/章末集成的串口与板上输出逐字、devmem 寄存器值、tiger watchdog 模型 MMIO 基地址与 32768 Hz 时钟的实物确认、复位时串口日志逐字形态），前提 qemu-tiger 的 watchdog 模型与 linux-tiger 仓库就位，续 C-W17~C-W29 批同口径。本节数值按 32768 Hz 时钟推算：60 s 装载值 = 60×32768 = 1966080 = 0x1E0000；边界值 131072 s 装载值 = 131072×32768 = 2^32，u32 截断回绕为 0。V24（SIGTERM magic close vs SIGKILL 行为）、V25（WDIOC_SETTIMEOUT 报错逐字）、V29（计数器读数与 sleep 时长换算自洽）同批实测终核。 -->

#### 15.2.2 读驱动：两个数对不上

工作区源码树里打开 `drivers/watchdog/sp805_wdt.c`。先给会去看真实源码的读者交个底：linux-tiger 树里这份驱动是**简化改写版**——换算和 probe 都比真实 6.6 上游简（差异清单在 15.2.3 的 💡 框），节选关键段做了标注，本章所有 diff 都在这个简化版上做。先认寄存器定义段：

```c
// 文件：workspace/sources/linux-tiger/drivers/watchdog/sp805_wdt.c（节选·示意改写，行号略）

#define WDTLOAD		0x000
	#define LOAD_MIN	0x00000001
	#define LOAD_MAX	0xFFFFFFFF
#define WDTVALUE	0x004
#define WDTCONTROL	0x008
	#define INT_ENABLE	(1 << 0)
	#define RESET_ENABLE	(1 << 1)
#define WDTINTCLR	0x00C
#define WDTLOCK		0xC00
	#define UNLOCK		0x1ACCE551
```

`WDTLOAD` 是装载寄存器——32 位，所以可表达的最大装载值是 `LOAD_MAX`（0xFFFFFFFF）；`WDTVALUE` 是当前计数值，刚才 devmem 读的就是它；`WDTINTCLR` 是喂狗动作的落点——用户空间每次写 `/dev/watchdog`，驱动最终就是写它。再看 timeout 的换算链，两个地方。

第一处，`wdt_setload`——用户空间设超时后，装载值的换算落点（上游的 `.set_timeout` 操作就指向它）：

```c
// 同文件，wdt_setload 段（节选·示意改写）

static int wdt_setload(struct watchdog_device *wdd,
		       unsigned int new_timeout)
{
	struct sp805_wdt *wdt = watchdog_get_drvdata(wdd);

	wdd->timeout = new_timeout;
	/* timeout in seconds * clock rate = load value */
	wdt->load_val = new_timeout * wdt->rate;

	return 0;
}
```

第二处，probe 尾段——驱动向 watchdog 核心上报自己的能力边界：

```c
// 同文件，probe 尾段（节选·示意改写）

	wdd->min_timeout = 1;
	wdd->max_timeout = wdt->rate ? LOAD_MAX / wdt->rate + 1 : 0;
```

`max_timeout` 是驱动告诉上层的"我最大能设多少秒"；用户空间的 `-T` 值超过它，会被 watchdog 核心直接拒绝。把数代进去算。tiger 的看门狗时钟 32768 Hz：

```text
max_timeout = LOAD_MAX / rate + 1
            = 4294967295 / 32768 + 1
            = 131071 + 1
            = 131072        ← 报给上层的最大值

边界值换算回装载值：
load = 131072 × 32768
     = 4294967296
     = 0x1_0000_0000        ← 33 位，u32 装不下
     → 截断回绕为 0          ← 装载寄存器得到 0
```

两个数对不上了。**驱动上报的最大超时是 131072 秒，但 131072 秒换算成装载值是 2³²，溢出了 32 位寄存器整整一圈，回绕成 0。** 上层校验放行（131072 ≤ max_timeout，合法），驱动装载出错（0），看门狗被静默停用。devmem 读到的那个 `0x00000000`，就是这个回绕的物证。

> **⚠️ 注意**：tiger 的简化模型把装载值 0 视为"看门狗未启用"，于是现象是"永不复位"。真实 SP805 芯片对装载 0 的行为不同（立即超时）——这是模型与真实硬件的一个差异点，但 bug 本体不在这里：上报值多算一秒，放在真硬件上也是错的。

<!-- 【待验证】V32：⚠️ 框中"真实 SP805 装载 0 立即超时"的表述，待查 ARM SP805 TRM 的 WDTLOAD 章节原文后定稿。 -->

为什么多报一秒？看那个 `+ 1`——写驱动的人大概是这么想的："计数器从 load 往下数到 0，一共 load + 1 拍，所以最大超时要加一拍。"思路没错，但忘了装载值本身最大只能表达到 `LOAD_MAX`：load 已经是 0xFFFFFFFF 时，那"多出来的一拍"在寄存器里没有容身之处。差一错误（off-by-one）：多报一秒，倒在边界上。

"找到了？"老周问。

"`max_timeout` 上报多算一秒，边界值装载溢出回绕成零。"阿凯把算式递过去。

"嗯。修法你来定——但我提醒你一句，别只修上报值。"老周说完就回自己工位了。

<!-- 【事实核查注记·V23】节选符号名已对齐 6.6 实物结构（wdt_setload、WDTINTCLR 0x00C、WDTLOCK 0xC00、寄存器定义段逐字吻合）；虚构 bug 构造为 probe 上报值的人为 +1 差一（真实 6.6 的 probe 不上报 max_timeout，真实换算为 div_u64(rate,2)*timeout-1 且自带 LOAD_MIN..LOAD_MAX 钳制），修复 = 删 +1 还原正确算术 + 简化版换算补钳制；正文已向读者声明 linux-tiger 用简化版换算（15.2.2 交底段 + 15.2.3 💡 框）。待回填：驱动节选与真实 6.6 逐字对照终核。不影射真实 CVE 一项初核成立：真实 6.6 驱动无此结构。 -->

#### 15.2.3 修复与分钟级迭代

老周那句"别只修上报值"阿凯想明白了：上报值改对之后，边界值会被上层拒掉，bug 路径堵死——但 `wdt_setload` 里那个毫无防备的乘法还在，哪天再有别的路径喂进来一个溢出值，照样静默回绕。**上报值修对 + 换算处兜底钳制**，两手都上。第一刀是还原不是发明——删掉 `+ 1`，让上报值回到它本该有的算法；第二刀钳制只是保险，它只兜得住回绕为 0 这一种形态（回绕成个小正数照样漏过去），真正的闸门是上报值：

```diff
# 工作区内的修改（git diff 形态）
diff --git a/drivers/watchdog/sp805_wdt.c b/drivers/watchdog/sp805_wdt.c
index 9f3c7a21..4b8e0d56 100644
--- a/drivers/watchdog/sp805_wdt.c
+++ b/drivers/watchdog/sp805_wdt.c
@@ -58,8 +58,12 @@
 	struct sp805_wdt *wdt = watchdog_get_drvdata(wdd);
+	u32 load;
 
 	wdd->timeout = new_timeout;
-	/* timeout in seconds * clock rate = load value */
-	wdt->load_val = new_timeout * wdt->rate;
+	/* timeout * rate; clamp a wrapped load value to LOAD_MIN */
+	load = new_timeout * wdt->rate;
+	if (load < LOAD_MIN)
+		load = LOAD_MIN;
+	wdt->load_val = load;
 
 	return 0;
 }
@@ -213,2 +217,8 @@
 	wdd->min_timeout = 1;
-	wdd->max_timeout = wdt->rate ? LOAD_MAX / wdt->rate + 1 : 0;
+	/*
+	 * WDTLOAD is a 32-bit register; the largest load value it can
+	 * express is LOAD_MAX. Advertising one second more makes the
+	 * boundary load = timeout * rate exactly 2^32, which wraps to
+	 * 0 and silently disables the watchdog.
+	 */
+	wdd->max_timeout = wdt->rate ? LOAD_MAX / wdt->rate : 0;
```

> **💡 提示**：对照真实上游源码的读者请注意——6.6 的 `sp805_wdt.c` 里这道换算比简化版多两道保险：`rate` 先减半（SP805 先超时中断、再复位，计数器实际走两轮），装载值且自带 `LOAD_MIN..LOAD_MAX` 钳制；probe 也不上报 `max_timeout`，用的是 DEFAULT_TIMEOUT 加 `watchdog_init_timeout` 那套。linux-tiger 树里这份是简化改写版（tiger 的模型只有一轮计数），上面 diff 里的"补钳制"补的正是简化版相对真实驱动缺掉的那道保险。

改动连注释在内十六行，真正动逻辑的不过三行。编译验证走工作区通道：

```bash
# 工作区迭代构建（分钟级；对照 8.6 的全量 20-60 分钟）
cd ~/workspace/build
devtool build linux-tiger
```

`devtool build` 只重编这棵树里动过的文件——从"改一行代码"到"新内核出来"是分钟级，而不是 chapter 8 那套"回仓库、提交、bump SRCREV、等 fetch + 全量"的小时级。这就是工作区的价值：**迭代环路的半径**。

新内核进镜像、总装、起板，全套复用前章动作：

```bash
# 镜像重建（增量，内核包更新）→ 总装 → 起板
bitbake tiger-image
bash ~/workspace/meta-tiger/scripts/mknandimg.sh
runqemu tiger-aarch64 nographic
```

板上复验，两个数都要看：

```bash
# 复验 1：修复后边界值 131072 应被上层拒绝（max_timeout 现在是 131071）
watchdog -F -t 10 -T 131072 /dev/watchdog &
```

输出（预期中的"报错"——拒绝即正确）：

```text
watchdog: WDIOC_SETTIMEOUT: Invalid argument
```

报错即正确——上层把越界值拒了。注意 busybox 这里只是警告（`ioctl_or_warn`），进程不退出：它还活着，按默认超时继续喂狗。收工，这次用普通 `kill`——SIGTERM 它接得住，写 magic 字符干净停狗：

```bash
# 复验 1 收工：SIGTERM 优雅停狗
kill %1

# 复验 2：合法最大值 131071 装载正常，计数器起走
watchdog -F -t 10 -T 131071 /dev/watchdog &
devmem 0x09030004 32
```

输出：

```text
0xFFFF7A80
```

`0xFFFF8000`（131071 × 32768）附近、正在递减——装载值不再回绕。修复板上成立。QA 的用例脚本改写成"边界值被拒 + 最大值可用"两条断言，那是他们的事；BSP 侧的活，干完了前半截。

<!-- 【待验证·阻塞级】C-W30：复验串口输出逐字回填（同上批，含 V25 报错形态终核）。 -->

### 15.3 devtool finish：把修改导回产品层

#### 15.3.1 坑 1 发作：finish 导出的是提交，不是工作区

修复在工作区里验完了，阿凯急着把它收进 meta-tiger：

```bash
# 错误示范：改完还没提交，直接导出
devtool finish linux-tiger meta-tiger
```

输出（预期中的错误）：

```text
ERROR: Source tree is not clean:

 M drivers/watchdog/sp805_wdt.c

Ensure you have committed your changes or use -f/--force if you are sure there's nothing that needs to be committed
```

被拦下了。阿凯盯着第二行那句提示看——"Ensure you have committed your changes"。

"它怎么知道我改了没提交？"

"工作区是一棵 git 仓库，`git status --porcelain` 一问就知道。"老周说，"关键是想清楚 **finish 导出的是什么**：它把 `devtool-base` 之后、工作分支上的**提交**逐个导出成 patch 文件。你的修改还躺在工作区里没成提交，对它来说就不存在。"

> **🔥 重要**：这条拦网是 devtool 在救你。别顺手加 `-f/--force` 强推——强制模式下它跳过检查照常导出，而你未提交的修改不会变成任何 patch：**导出的 patch 是空的，你的修复被静默丢弃**，工作区随后还被拆除，连找回来的机会都没有。看到 "Source tree is not clean"，唯一正确的动作是回工作区提交。

<!-- 【事实核查注记·V22 处置留痕】坑 1 发作形态已对照 Scarthgap 本地源码裁决：devtool finish 起手调用 check_git_repo_dirty（standard.py 第 2185-2190 行），工作区脏且无 -f 时抛 DevtoolError "Source tree is not clean ..."（报错拦截，非静默丢弃、非自动提交）；-f 强制路径下导出的是 devtool-base..HEAD 的提交（_export_patches → GitApplyTree.extractPatches，standard.py 第 1358-1382 行），未提交改动不进 patch——"报错 / 强制下静默丢弃"两态均有源码依据。输出文案为源码级推定的示意形态，逐字以实测为准（随 C-W31 回填）。 -->

#### 15.3.2 工作区提交：签名先行

回工作区，按规矩提交。注意 `-s` 这个选项——它会往提交信息末尾自动追加一行 `Signed-off-by`：

```bash
# 回工作区提交（-s 追加 Signed-off-by 签名行）
cd ~/workspace/build/workspace/sources/linux-tiger
git add drivers/watchdog/sp805_wdt.c
git commit -s -m "watchdog: sp805: fix max_timeout reporting off-by-one

The SP805 WDTLOAD register is a 32-bit down-counter, so the largest expressible load value is LOAD_MAX. The driver however advertises
max_timeout as LOAD_MAX / rate + 1. Setting the timeout to that
boundary value makes load = timeout * rate exactly 2^32, which wraps
to 0 when truncated to u32 and silently disables the watchdog.

Advertise LOAD_MAX / rate instead, and clamp a wrapped load value to
LOAD_MIN in wdt_setload as a belt-and-suspenders fix."
```

提交信息的写法先认个脸熟，15.5 写提交信时还要回来对照：第一行是"子系统前缀 + 一句话"（`watchdog: sp805: ...`），正文讲清楚**问题、根因、修法**三件事。注意语言——这是要给上游看的东西，用英文写，代码里的注释同理（patch 正文里那两段英文注释就是这么来的）。消息体里没写签名行——`-s` 会在提交时把它自动追加到末尾，`git log -1` 能看到。

**补丁签名（Signed-off-by）** 本章正式引入：补丁和提交中的签名行惯例，表明提交者审阅过自己的修改、并同意按开发者原创证书（DCO，Developer Certificate of Origin——一句话版：我保证这是我写的或我有权提交）贡献给项目。内核社区没有它不收 patch；养成习惯，凡是可能出门的提交都带 `-s`。

#### 15.3.3 finish 导出与产物解剖

提交落了，重跑 finish：

```bash
# 导出修改回产品层
cd ~/workspace/build
devtool finish linux-tiger meta-tiger
```

输出（关键行）：

```text
INFO: Adding new patch watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
INFO: Updating recipe linux-tiger.bb
```

> **⚠️ 注意**：finish 成功后会把 linux-tiger 移出工作区——`workspace/sources/linux-tiger` 默认拆除，接管件 bbappend 一并撤掉，构建恢复到"按配方正常 fetch"的状态。所以 finish 之前确认该提交的都提交了（上一条 🔥 框的另一半原因）。还想继续改？再 `devtool modify` 开一次就是。

看产物。两个落点，一一解剖。第一个是 patch 文件本体：

```bash
# patch 落在了配方同名的 files 目录
cd ~/workspace/meta-tiger
git status
```

输出（关键行）：

```text
Changes not staged for commit:
	modified:   recipes-kernel/linux/linux-tiger.bb
Untracked files:
	recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
```

```text
# 文件：meta-tiger/recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch（全文）

From 3f4a1c2e9d8b7a6c5e4f3a2b1c0d9e8f7a6b5c4d Mon Sep 17 00:00:00 2001
From: Kai <kai@example.com>
Date: Thu, 20 Aug 2026 11:07:31 +0800
Subject: [PATCH] watchdog: sp805: fix max_timeout reporting off-by-one

The SP805 WDTLOAD register is a 32-bit down-counter, so the largest expressible load value is LOAD_MAX. The driver however advertises
max_timeout as LOAD_MAX / rate + 1. Setting the timeout to that
boundary value makes load = timeout * rate exactly 2^32, which wraps
to 0 when truncated to u32 and silently disables the watchdog.

Advertise LOAD_MAX / rate instead, and clamp a wrapped load value to
LOAD_MIN in wdt_setload as a belt-and-suspenders fix.

Signed-off-by: Kai <kai@example.com>
---
 drivers/watchdog/sp805_wdt.c | 16 ++++++++++++++---
 1 file changed, 13 insertions(+), 3 deletions(-)

diff --git a/drivers/watchdog/sp805_wdt.c b/drivers/watchdog/sp805_wdt.c
index 9f3c7a21..4b8e0d56 100644
--- a/drivers/watchdog/sp805_wdt.c
+++ b/drivers/watchdog/sp805_wdt.c
@@ -58,8 +58,12 @@
 	struct sp805_wdt *wdt = watchdog_get_drvdata(wdd);
+	u32 load;
 
 	wdd->timeout = new_timeout;
-	/* timeout in seconds * clock rate = load value */
-	wdt->load_val = new_timeout * wdt->rate;
+	/* timeout * rate; clamp a wrapped load value to LOAD_MIN */
+	load = new_timeout * wdt->rate;
+	if (load < LOAD_MIN)
+		load = LOAD_MIN;
+	wdt->load_val = load;
 
 	return 0;
 }
@@ -213,2 +217,8 @@
 	wdd->min_timeout = 1;
-	wdd->max_timeout = wdt->rate ? LOAD_MAX / wdt->rate + 1 : 0;
+	/*
+	 * WDTLOAD is a 32-bit register; the largest load value it can
+	 * express is LOAD_MAX. Advertising one second more makes the
+	 * boundary load = timeout * rate exactly 2^32, which wraps to
+	 * 0 and silently disables the watchdog.
+	 */
+	wdd->max_timeout = wdt->rate ? LOAD_MAX / wdt->rate : 0;
-- 
2.43.0
```

4.1.3 认过的 patch 三段结构，今天从"看别人的"变成"自己产出的"：邮件头（From/Date/Subject）、提交信息（含 Signed-off-by）、diff 正文。文件名由 Subject 机器化而来——冒号删掉、空格转连字符；注意**没有 `0001-` 序号**，devtool 导出用的是无序号模式（15.4.1 细说）。

第二个落点是配方侧。`git diff` 看看 finish 对 `linux-tiger.bb` 做了什么：

```bash
# 配方侧改动
cd ~/workspace/meta-tiger
git diff recipes-kernel/linux/linux-tiger.bb
```

输出（关键行）：

```diff
-SRC_URI = "git://<internal-git-server>/bsp/linux-tiger.git;protocol=ssh;branch=main"
+SRC_URI = "git://<internal-git-server>/bsp/linux-tiger.git;protocol=ssh;branch=main \
+           file://watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch \
+           "
```

注意一个与 4.3.3 经验不同的地方：**这次没有生成 .bbappend，patch 是直接写进 linux-tiger.bb 的**。4.3.3 给 qemu-system-native 打 patch 时走 bbappend，是因为原配方住在 poky 里、改不得；今天 linux-tiger.bb 本来就住在 meta-tiger 里，finish 的目的地也是 meta-tiger——同一层，devtool 就直接更新原配方。patch 文件落在 `linux-tiger/` 子目录（`${THISDIR}/${PN}`，默认文件搜索路径之一），所以连 `FILESEXTRAPATHS` 那行都不用加。

构建系统视角验证一遍——patch 确实进了 SRC_URI：

```bash
# 验证 patch 已被配方认领
cd ~/workspace/build
bitbake -e linux-tiger | grep -o 'file://watchdog[^"\\]*'
```

输出：

```text
file://watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
```

<!-- 【事实核查注记·V21 处置留痕】finish 落点已对照 Scarthgap 本地源码裁决：目的地层与原配方同层时走"Same layer, update the original recipe"分支（standard.py 第 2240-2243 行，appendlayerdir = None），不生成 bbappend；patch 模式下新 patch 落 files 目录并把 file:// 条目追加进原配方 SRC_URI（standard.py 第 1839-1863 行，"Adding new patch"/"Updating recipe"日志行同出处）；导出用 git format-patch 无序号模式并剥离序号前缀（patch.py 第 521 行、standard.py 第 1374 行）——故文件名无 0001- 前缀。files 目录由 _determine_files_dir 决定，${THISDIR}/${PN} 在默认 FILESPATH 内，无需 FILESEXTRAPATHS。finish 末尾 _reset 拆除工作区（standard.py 第 2341-2345 行）。git diff 逐字形态随 C-W31 实测回填。 -->

到这里，修复已经是产品层的一份正式资产：meta-tiger 里躺着 patch，配方认领了它，构建系统会把它打进每一次内核构建。但这份资产还缺一样东西——它的"履历"。

### 15.4 patch 的命名与元数据

#### 15.4.1 命名：提交主题决定文件名，应用顺序看 SRC_URI

回看 patch 文件名：`watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch`。它不是谁手工起的——devtool 导出时拿提交信息的第一行（Subject）机器化而来：`[PATCH]` 标记和冒号删掉、空格转连字符、下划线保留。没有 `0001-` 序号——devtool 导出走的是无序号模式，哪怕工作区里压着好几笔提交、一次导出多个 patch，文件名也不带序号。

那多个 patch 的应用顺序谁定？**SRC_URI 里的排列顺序**——do_patch 按 SRC_URI 列出的先后逐个打，4.8.2 的坑 2 就是在排序上栽过的。文件名只负责"让人认"，顺序归配方管。

所以**命名规范的一半在写提交信息的第一行时就定了**——`watchdog: sp805: fix ...` 这个"子系统: 驱动: 动作"的写法不是装饰，它直接决定 patch 文件名长什么样，下一任工程师扫一眼文件名就知道它动的是谁。

#### 15.4.2 元数据全家福：Signed-off-by 已在，还差 Upstream-Status

4.4.2 引入 Upstream-Status 时立过规矩：meta-tiger 里每个 patch 头部都要有这一行。当时只见过两个值——meta-tiger 自己写的是 `Inappropriate`（tiger 三个硬件 patch，虚构硬件不上游），`Backport` 是拿 OE-Core 里 qemu 的 CVE patch 取的词。今天这个 patch 的命运和它们都不同：**它是要出门的**。先把取值的全家福认全。OE-Core 自己就是最大的 patch 陈列馆，grep 一把实物：

```bash
# 统计 poky 里 patch 的 Upstream-Status 取值分布（去掉方括号尾巴再统计）
grep -rh "Upstream-Status" ~/workspace/poky/meta --include='*.patch' | sed 's/ *\[.*//' | sort | uniq -c | sort -rn
```

输出（示意形态，数字以实测为准）：

```text
    132 Upstream-Status: Backport
     61 Upstream-Status: Pending
     23 Upstream-Status: Submitted
     18 Upstream-Status: Inappropriate
      1 Upstream-Status: Accepted
```

Yocto 5.0 官方认六个值，对照着记：

| 值 | 含义 | 用例 |
| --- | --- | --- |
| `Pending` | 还没提交上游 | 本章 patch 的初态 |
| `Submitted [where]` | 已提交并附存档去处，等上游结果 | 真发之后改这个（本书停在 dry-run，本章保持 Pending） |
| `Backport [version]` | 从上游回合的修复（含已被上游收下、等发布） | 4.4.2 qemu CVE patch |
| `Denied` | 上游拒收 | 少见，但比 Pending 诚实 |
| `Inactive-Upstream [lastcommit/lastrelease]` | 上游停更，提交无门 | 少见 |
| `Inappropriate [reason]` | 不适合提交上游 | 4.4.2 tiger 硬件三 patch |

> **💡 提示**：grep 结果里那个孤零零的 `Accepted` 是旧版文档口径的历史残留——现在的官方定义里，"上游已收、等发布"的 patch 一律标 `Backport [version]`。新 patch 别再用 `Accepted`。

这张表是 patch 的生命周期：一个要出门的 patch，出生时是 `Pending`，发出去了改 `Submitted` 并附上游存档链接，被收进上游的新版本后，产品层在某次升级中把它整个删掉——那就是它的寿终正寝。每一跳都在这一行字里留痕。

给今天的 patch 补上。手工编辑 patch 文件头部——注意编辑位置在 Subject 段之后、`---` 分隔线之前（4.4.2 的手法），这一块是"头字段安全区"，不碰 diff 正文就不会影响 patch 的应用：

```diff
# 文件：meta-tiger/recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch（头部节选）

 Signed-off-by: Kai <kai@example.com>
+Upstream-Status: Pending
 ---
 drivers/watchdog/sp805_wdt.c | 16 ++++++++++++++---
```

为什么是 `Pending` 而不是别的？patch 还没发出——它此刻的真实身份是"已修好、待提交"。等它真发出去、拿到存档链接，才轮到 `Submitted`——15.5 会演示到 dry-run 为止，本书不走那一步，本章它停在 `Pending`。

> **💡 提示**：这行字不是写给构建系统看的——do_patch 根本不读它。它是写给人看的：半年后、一年后接手 meta-tiger 的人（很可能就是你自己），靠每个 patch 的这一行决定"这个该不该还留着、能不能删"。附录 B 的交接清单里，它就是维护地图。

### 15.5 提交给上游（流程示意）

#### 15.5.1 自检两板斧：checkpatch 与找门牌

内核社区的门槛在提交之前就立好了：patch 要先过自检，再找对收件人。两件工具都长在内核源码树里——linux-tiger 仓库本身就是一棵内核树，用它：

```bash
# 第一板斧：风格与规范自检（--strict 更严）
cd ~/workspace/linux-tiger
./scripts/checkpatch.pl --strict ~/workspace/meta-tiger/recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
```

`checkpatch.pl` 是内核的 patch 体检脚本——风格、常见错误、提交信息格式，一把全查。第一遍输出（示意形态）：

```text
WARNING: Prefer a maximum 75 chars per line (possible unwrapped commit description?)
#6: 
The SP805 WDTLOAD register is a 32-bit down-counter, so the largest expressible load value is LOAD_MAX. The driver however advertises

total: 0 errors, 1 warnings, 0 checks, 49 lines checked
```

提交信息正文第一行一百三十多个字符，超了 75 的上限。改它——提交信息在 patch 的头字段安全区里，手工编辑不影响应用：

```diff
# 文件：meta-tiger/recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch（头字段安全区内编辑）
-The SP805 WDTLOAD register is a 32-bit down-counter, so the largest expressible load value is LOAD_MAX. The driver however advertises
+The SP805 WDTLOAD register is a 32-bit down-counter, so the largest
+expressible load value is LOAD_MAX. The driver however advertises
```

改完重跑：

```text
total: 0 errors, 0 warnings, 0 checks, 50 lines checked
```

<!-- 【待验证】V28：checkpatch 桥段已改为真实可触发形态（提交信息正文首行 >75 字符，15.3.2 起即如此，15.3.3 patch 全文与之一致，本节的编辑 diff 为真实重排）；warning 行号与 lines checked 数字随本机实跑 checkpatch.pl --strict 回填。 -->

清零。老周凑过来看了一眼，只说了句："**checkpatch 清零是下限不是上限**——它的 CHECK 级建议有些该听有些不该听，把风格 warning 修到牺牲可读性就是过度修正。判断归你。"

第二板斧，找收件人：

```bash
# 第二板斧：这个 patch 该发给谁
./scripts/get_maintainer.pl ~/workspace/meta-tiger/recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
```

输出（示意形态）：

```text
Wim Van Sebroeck <wim@linux-watchdog.org> (maintainer:WATCHDOG DEVICE DRIVERS)
Guenter Roeck <linux@roeck-us.net> (maintainer:WATCHDOG DEVICE DRIVERS)
linux-watchdog@vger.kernel.org (open list:WATCHDOG DEVICE DRIVERS)
linux-kernel@vger.kernel.org (open list)
```

`get_maintainer.pl` 读的是内核树里的 `MAINTAINERS` 文件——每个子系统在其中登记了维护者和邮件列表。**邮件列表（mailing list）** 就地解释一句：内核社区不用 issue 系统，patch 以邮件形式发到子系统列表，review、修改、合入全在邮件线程里进行。watchdog 子系统的门牌是 linux-watchdog 列表，抄送维护者和 linux-kernel 总列表。

#### 15.5.2 提交信：三要素，自己写

"提交信这东西……老周你帮我写了吧。"阿凯试探。

"自己写。"老周头也没抬，"写完我看一遍。"

其实信的主体已经有了——15.3.2 那笔提交信息就是底稿。对照内核社区的提交规范（docs.kernel.org 的 submitting-patches 文档，地址见章末延伸阅读），检查三要素：

1. **问题**：什么现象、怎么复现（边界超时值看门狗不动作）；
2. **根因**：为什么（上报值多一秒，边界装载溢出回绕为 0）；
3. **修法**：改了什么、为什么这样改（修正上报 + 换算兜底，两手各自的职责）。

再加上 Subject 前缀规范——`[PATCH] watchdog: sp805: ...`：`[PATCH]` 声明这是一封补丁邮件，`watchdog: sp805:` 让维护者扫一眼收件箱就知道归属。这些在 15.3.2 的提交里都已就位。阿凯通读两遍，交给老周。老周看了一遍，改了一个词，签了"发吧"。

#### 15.5.3 发送：走到 dry-run 为止

发送动作在 linux-tiger 仓库里做：把 patch 重放成一个本地提交，`git format-patch` 出正式提交版，然后 `git send-email`。有一个细节先处理：15.4 补进头部的 `Upstream-Status: Pending` 是给 OE 侧看的行，`git am` 会把它一并带进提交信息——发上游前在重放出的提交里把那行剔掉：

```bash
# 在 linux-tiger 仓库开分支，把 patch 重放为提交
cd ~/workspace/linux-tiger
git checkout -b watchdog-max_timeout-fix
git am ~/workspace/meta-tiger/recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
git commit --amend   # 剔除提交信息里的 Upstream-Status 行（上游不看这个；会打开编辑器，删掉那行后保存退出）

# 出正式提交版 patch（git format-patch 带 0001- 序号——这是发送用的一次性产物，与 meta-tiger 里的资产命名两码事）
git format-patch -1

# 发送演练：只打印信封，不真发（git-email 包未装则先 sudo apt install -y git-email）
git send-email --dry-run \
    --to=linux-watchdog@vger.kernel.org \
    --cc=wim@linux-watchdog.org --cc=linux@roeck-us.net \
    0001-watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
```

输出（示意形态）：

```text
From: Kai <kai@example.com>
To: linux-watchdog@vger.kernel.org
Cc: Wim Van Sebroeck <wim@linux-watchdog.org>,
    Guenter Roeck <linux@roeck-us.net>
Subject: [PATCH] watchdog: sp805: fix max_timeout reporting off-by-one
...
Dry-OK. Log says:
```

<!-- 【待验证】V27：git send-email --dry-run 收尾形态按 git 2.43 源码口径写为 "Dry-OK. Log says:"（源码中不存在 "No emails were sent"），本机实跑回填逐字。 -->

`--dry-run` 把信封、收件人、主题全部打印一遍，收尾的 `Dry-OK` 确认演练通过但不发送——发送前的最后一次对账。真发还需配置 `sendemail.smtpserver` 等 SMTP 参数，公司内网另有规矩，不展开。

到这儿必须交个底：**这封邮件永远停在草稿箱**。tiger 是虚构硬件，这个 bug 是教学示例，不能真往 linux-watchdog 列表发——那是对社区时间的浪费。流程的每一步都是真的，只有按下发送键这一步被本书按住了。

"真项目里，"老周补了最后一课，"**按发送键之前，先确认你的修复在最新 mainline 上还能复现**——别拿 6.6 这棵老树当目标，维护者只看最新主线。复现得了、patch 打得上了，再发。发出去拿到 lore（kernel.org 的邮件存档站）上的存档链接，回来把 meta-tiger 里那行 `Pending` 改成 `Submitted` 并附上链接——这个 patch 才算真正上了路。"

### 15.6 产品层的 patch 维护：等上游合并的日子

#### 15.6.1 patch 的身份：过渡资产

邮件在本书永远停在草稿箱；真项目里它此刻已经上路。从按下发送键起，patch 进入它生命里最漫长的一段：等。等维护者 review，等合并窗口，等随某个 6.6.x 点版本回到 linux-tiger 的上游——这段日子可能几周，可能几个月。

这段日子里，meta-tiger 里这个 patch 的身份是**过渡资产**：它必须活着（产品要用修复），但它的目标是被删掉（上游合并、内核升级、patch 功成身退）。养它的日常有两件：一是 Upstream-Status 随行就市（Pending → Submitted → 删除），13.6 节立的哨兵——`devtool check-upgrade-status` 与 AUH——在这段日子里第一次派上真用场：上游一动，它先知道。二是**内核每次升级，都得带它过一遍门**。

三周后，哨兵报信：固件组把 linux-tiger 跟进到了 6.6.x 新点版本。阿凯按惯例 bump 配方里的 SRCREV，然后——

#### 15.6.2 坑 2 发作：上游动了上下文

```bash
# 坑 2 发作：bump SRCREV 后构建
cd ~/workspace/build
bitbake linux-tiger
```

输出（预期中的错误，关键行）：

```text
ERROR: linux-tiger-6.6+git-r0 do_patch: Applying patch 'watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch'...
Hunk #2 FAILED at 213.
ERROR: ... do_patch: patch failed
```

4.8.2 见过的场面：do_patch 炸、hunk FAILED。看 `.rej` 文件确认细节的手法也一样（4.8.2 用过的 `cat .../*.rej`，不重演）——第二个 hunk 落不了地。但原因和 4.8.2 不同：那次是**自己把 patch 顺序排错了**，这次是**上游点版本在 sp805_wdt.c 的邻近行做了改动**，patch 里记的上下文对不上了。同族不同因——账记到 15.7 一起算。

> **💡 提示**：一个容易自我怀疑的时刻——如果你是在工作区还开着（modify 之后、finish 之前）的时候 bump SRCREV，构建会"毫无变化"，连错都不报：15.1.2 见过的 `SRCTREECOVEREDTASKS` 那行把 `do_fetch`/`do_unpack` 都摘了，externalsrc 接管期间配方里的 SRCREV 根本没人读。本章的 bump 之所以立刻发作，是因为 15.3 的 finish 已经把工作区拆了。改内核前先看一眼 `workspace/sources/` 里有没有它的名字——有，则配方改动一律无效。

修正流四步。思路一句话：**patch 打不上新基线，就回到新基线上把它重新生出来**。

第一步，把打不上的 patch 从配方里摘掉——devtool 开工作区时会重打 SRC_URI 里的所有 patch，带着一颗哑弹进工作区等于把刚才的烂摊子再演一遍：

```bash
# 第一步：摘掉哑弹（编辑 linux-tiger.bb，SRC_URI 恢复为单 git:// 行）
cd ~/workspace/meta-tiger
```

编辑的 diff 形态——就是 15.3.3 里 finish 追加那几行的逆操作，file:// 条目和两个续行符一起删：

```diff
# 文件：meta-tiger/recipes-kernel/linux/linux-tiger.bb（手工编辑）
-SRC_URI = "git://<internal-git-server>/bsp/linux-tiger.git;protocol=ssh;branch=main \
-           file://watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch \
-           "
+SRC_URI = "git://<internal-git-server>/bsp/linux-tiger.git;protocol=ssh;branch=main"
```

第二步，开工作区——这次基线是新 SRCREV：

```bash
# 第二步：新基线上的干净工作区
cd ~/workspace/build
devtool modify linux-tiger
```

第三步，把旧 patch 在新基线上重放。重放——git 世界里这类活也叫 rebase，不过今天用的命令是 `git am -3`：`-3` 启用三方合并，能自动对齐的自动对齐，对不齐的地方标出冲突交给人：

```bash
# 第三步：重放旧 patch（-3 启用三方合并）
cd ~/workspace/build/workspace/sources/linux-tiger
git am -3 ~/workspace/meta-tiger/recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
```

输出（预期中的冲突，关键行）：

```text
Applying: watchdog: sp805: fix max_timeout reporting off-by-one
Using index info to reconstruct a base tree...
Falling back to patching base and 3-way merge...
Auto-merging drivers/watchdog/sp805_wdt.c
CONFLICT (content): Merge conflict in drivers/watchdog/sp805_wdt.c
error: Failed to merge in the changes.
```

打开冲突文件，`<<<<<<<` / `=======` / `>>>>>>>` 三段标记围住的就是对不齐的地方：上游在 probe 尾段邻近行改了一处时钟获取的写法，我们的 hunk 上下文因此漂移。解它不需要动脑子改逻辑——保留上游的新写法，把我们的 `max_timeout` 那一行放回它该在的位置：

```bash
# 编辑 sp805_wdt.c 解冲突（保留上游新上下文 + 我们的修改），然后：
git add drivers/watchdog/sp805_wdt.c
git am --continue
```

第四步，重新导出，刷新产品层里的 patch：

```bash
# 第四步：刷新 meta-tiger 里的 patch
cd ~/workspace/build
devtool finish linux-tiger meta-tiger
```

输出（关键行）：

```text
INFO: Adding new patch watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch
INFO: Updating recipe linux-tiger.bb
```

同名 patch 文件被按新基线的上下文重新生成，SRC_URI 的 file:// 条目随之回归——第一步摘掉的那行，finish 给补回来了。元数据也不用担心：15.4 手工补的那行 `Upstream-Status: Pending` 还在——`git am -3` 重放时把它带进了提交信息，finish 从提交重新导出时它自然幸存。git am 帮我们保住了元数据（只有 git apply / patch 打进工作区再手写提交的重放路径才会把它弄丢——patch 头字段随应用动作丢失，新提交的消息要从头写）。顺手验证一眼：

```bash
# 元数据幸存验证
grep "Upstream-Status" ~/workspace/meta-tiger/recipes-kernel/linux/linux-tiger/*.patch
```

收尾复验：

```bash
# 复验：patch 在新基线上应用、构建通过
bitbake linux-tiger
```

构建通过。这个 patch 又能在新基线上活一个升级周期了。

<!-- 【待验证·阻塞级】C-W30/C-W31：坑 2 修正流（摘行 → modify → git am -3 → finish 刷新）依赖 linux-tiger 仓库与一次真实的点版本上下文漂移，发作与修正输出逐字随 C-W30/C-W31 批回填。流程中 devtool 行为的关键依据已源码核实：抽取时 PATCHTOOL=git + PATCH_COMMIT_FUNCTIONS=1（standard.py 第 578-580 行），配方 patch 以 git am -3 落成工作区提交、冲突时三级回落 git apply → GNU patch（patch.py 第 626-653 行）——故"摘行再进工作区"是必要的预备动作；finish 重新导出以提交为载体（extractPatches），经 git am 重放路径 Upstream-Status 随提交幸存。【V35 处置注记】丢元数据的路径已修正口径：rebase/cherry-pick 保留提交信息，真正丢失的是 git apply / patch 打进工作区再手写提交的路径（头字段随应用动作丢失、提交消息从头写）。 -->

#### 15.6.3 留给发布章的一句话

"patch 养好了。"阿凯合上笔记本。

"记住它现在的样子。"老周说，"chapter 16 做发布的时候，meta-tiger 里每个 patch 的清单、每个 Upstream-Status 的取值，都要跟着 SRCREV 一起进发布证据链——**别人问'这版系统里有哪些还没进上游的修改'，你要能一张表答出来。**"

### 15.7 踩坑实录

#### 15.7.1 坑 1：finish 前忘了 commit——工具的状态机不会替你走

发作过程见 15.3.1：修复在工作区验完，兴冲冲直接 `devtool finish`，被 "Source tree is not clean" 拦下。这一拦是幸运的——真正的陷阱在 `-f` 强推那侧：强制导出照常进行，未提交的修改不进任何 patch，工作区随后拆除，修复人间蒸发，而你会以为"已经导出了"。

机理一句话：**devtool finish 导出的是提交（devtool-base 之后工作分支上的 commit），不是工作区文件**。工作区是一棵 git 仓库，修改 → 验证 → 提交 → 导出，是工具设计好的状态机——它不拦你改，不拦你验证，但跳过"提交"直接"导出"，轻则报错，重则（-f）静默丢弃。

族谱认门：候选新族——**工具状态机坑**：工具不拦你跳过它的状态转换，跳过了就拿到一个"成功形态的空结果"。与 8.11.1（直接改工作区文件）是近亲：都是没搞清"工具认哪个现场"。

纪律落纸：`devtool finish` 之前，先 `git status`——工作区不干净，finish 免谈；`-f/--force` 只在你确知脏文件与本次导出无关时用。

#### 15.7.2 坑 2：内核升级后 patch 打不上——同族不同因

发作过程见 15.6.2：linux-tiger 跟进 6.6.x 点版本，bump SRCREV，do_patch 第二个 hunk FAILED。

族谱认门：归 4.8.2 的 hunk FAILED 一族，但**同族不同因**——4.8.2 是自己把 SRC_URI 里 patch 顺序排错（内因，排序错误），这次是上游点版本改了邻近行、上下文漂移（外因，基线移动）。发作形态一样，处方完全不同：前者调顺序，后者回新基线重放（摘哑弹 → modify → git am -3 解冲突 → finish 刷新）。

纪律落纸：patch 是过渡资产——**每次 bump 内核 SRCREV，先把 meta-tiger 里的 patch 清单过一遍**，把"哪些 patch 可能打不上"想在构建之前；finish 刷新后 grep 一眼确认 Upstream-Status 还在（git am 重放路径它随提交幸存，git apply / patch 打进去再手写提交的路径才会丢）。

### 15.8 本章小结

白板五步回顾：

- **15.1 拉源码**：`devtool modify linux-tiger` 在主构建环境开工作区（add 无中生有 / modify 改造既有）；工作区解剖——`workspace/sources/linux-tiger` 是 git 仓库（devtool 分支 + devtool-base 基线标记），`workspace/appends/linux-tiger.bbappend` 是 externalsrc 接管件（内核特化的 `SRCTREECOVEREDTASKS`、`do_patch[noexec]` 两行是 15.6 的伏笔）；chapter 8 坑 1 的笨办法在此正式还账。
- **15.2 复现调试修复**：看门狗（Watchdog）正式引入（三角色：QEMU 模型 / sp805_wdt 驱动 / /dev/watchdog）；对照基线先行（-F 前台 + kill -9 模拟挂死，-T 60 复位正常）再打边界值（-T 131072 不复位，devmem 计数器恒 0）；读驱动定位——上报的 max_timeout 多算一秒（LOAD_MAX/rate + 1），边界值装载值 131072×32768 = 2³² 回绕为 0；修复两手（删 +1 还原上报 + 换算兜底钳制）；`devtool build` 分钟级迭代对照 chapter 8 全量。
- **15.3 导回产品层**：坑 1 发作——未 commit 直接 finish 被拦（源码裁决：报错拦截；-f 强推则静默丢弃）；`git commit -s` 签名先行，补丁签名（Signed-off-by）正式引入（DCO 一句话）；`devtool finish linux-tiger meta-tiger` 同层落点直接改 .bb 不生成 bbappend（源码裁决），patch 落 `recipes-kernel/linux/linux-tiger/`；finish 后工作区拆除。
- **15.4 命名与元数据**：patch 文件名由提交信息第一行机器化而来（无序号——多 patch 的应用顺序看 SRC_URI 排列）；Upstream-Status 六值全家福（Pending / Submitted [where] / Backport [version] / Denied / Inactive-Upstream / Inappropriate，4.4.2 单点升级全景）；补 `Pending`，Pending → Submitted → 删除的生命周期落纸（本书停在 dry-run，本章保持 Pending）。
- **15.5 提交上游（示意）**：checkpatch.pl 自检清零（头字段安全区手工重排超长行，不过度修正）；get_maintainer.pl 找门牌（linux-watchdog 列表）；提交信三要素（问题 / 根因 / 修法）+ `[PATCH] watchdog: sp805:` 前缀；`git send-email --dry-run` 为止——诚实边界：tiger 是虚构硬件，邮件不真发；老周的临别一课：发之前先在最新 mainline 复现。
- **15.6 产品层维护**：patch 的过渡资产身份；13.6 哨兵收第一笔；坑 2——点版本上下文漂移 do_patch FAILED（4.8.2 同族不同因），修正流四步（摘哑弹 → modify → am -3 解冲突 → finish 刷新），git am 保住了 Upstream-Status 元数据；externalsrc 遮蔽 💡 框（工作区开着时 bump SRCREV 无效）。
- **15.7 踩坑复盘**：坑 1 候选新族"工具状态机坑"；坑 2 认门 hunk FAILED 族、同族不同因。

章末集成验证——修复随产品镜像走完整条流水线：

```bash
# 章末集成：重建镜像 → 总装 → 全链起跑
cd ~/workspace/build
bitbake tiger-image
bash ~/workspace/meta-tiger/scripts/mknandimg.sh
runqemu tiger-aarch64 nographic
```

板上终验两条（15.2.3 复验的镜像版）：`-T 131072` 被拒、`-T 131071` 计数器正常起走。meta-tiger 合入后，CI 流水线（13.7）每晚自动把 QA 的 watchdog 用例跑在最新构建上——修复进了仓库，验证就不再依赖谁的记性。

<!-- 【待验证·阻塞级】C-W30：章末集成串口输出逐字回填（同上批）。 -->

本章产出清单：

- `meta-tiger` 新增：`recipes-core/busybox/busybox/watchdog.cfg` 与 `recipes-core/busybox/busybox_%.bbappend`（15.2.1 的准备动作——busybox 配置片段打开 `CONFIG_WATCHDOG`，bbappend 把片段挂进 SRC_URI）。
- `meta-tiger` 新增：`recipes-kernel/linux/linux-tiger/watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch`（含 Signed-off-by 与 `Upstream-Status: Pending`）。
- `meta-tiger` 编辑：`recipes-kernel/linux/linux-tiger.bb`（SRC_URI 追加 file:// patch 行——devtool finish 自动写入）。
- `meta-tiger` 之外：`~/workspace/linux-tiger` 里的 `watchdog-max_timeout-fix` 分支是上游提交的演练现场，不推送、不进交付；构建目录 workspace 已随 finish 拆除。

提交并打 tag：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Add watchdog boundary fix patch pending upstream"
git tag chapter15
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter15`，它是本章唯一有交付改动的我方仓库；linux-tiger 仓库 main 分支本章无我方提交（固件组的点版本跟进是外部演进，阿凯本地的演练分支不推送），不打 tag；poky、meta-arm 与其余开发态仓库无改动，不打，沿用惯例。

后续任务清单：

- **task 17 / chapter 16 发布与合规交付**：meta-tiger 里这个 `Upstream-Status: Pending` 的 patch 随 SRCREV 一起进发布证据链——patch 清单是"这版系统里有哪些未进上游的修改"的标准答案，也是合规交付物的一部分。
- **task 18 / 尾声**：上游合并与否，是"一年之后"的天然素材——若那时 patch 已进 6.6.y，meta-tiger 里删掉它的那一笔，就是这份过渡资产的寿终正寝。
- **task 20 / 附录 B**：meta-tiger 里每个 patch 的 Upstream-Status，就是下一任 BSP 工程师的维护地图——交接清单的一项，本章落了第一个条目。

---

**延伸阅读**

1. Linux 内核提交补丁官方指南（submitting-patches，含提交信格式、DCO、checkpatch 规范）：https://docs.kernel.org/6.6/process/submitting-patches.html
2. Yocto 开发任务手册（devtool 章节，modify/finish/update-recipe 完整参考）：https://docs.yoctoproject.org/5.0/dev-manual/index.html
3. Linux 内核 MAINTAINERS 文件与邮件列表流程说明：https://docs.kernel.org/6.6/process/maintainers.html

<!-- 【待验证】V34：延伸阅读 3 的标题口径（"MAINTAINERS 文件与邮件列表流程说明"）与该文档实际内容是否对得上，人工点开核一次后定稿。 -->
