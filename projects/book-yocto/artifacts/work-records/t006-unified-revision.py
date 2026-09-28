# -*- coding: utf-8 -*-
"""T-006 chapter 1 统一修订脚本（一次性执行，保留于 workspace 供追溯）。
执行顺序：1) D-008 改名 老周->达哥；2) 正文直引号->弯引号（保护围栏代码块与行内代码）；
3) 按条内容改写（R-1~R-5 / U-1~U-16，每条带命中计数校验）；4) 残留断言。
任一规则未命中则中止，不写盘。"""
import re, sys

PATH = r"C:\Users\YinZh\Desktop\book-yocto\projects\book-yocto\workspace\yocto\task02-1-把Yocto跑起来.md"
text = open(PATH, encoding="utf-8").read()

# ---------- 1. D-008 改名 ----------
n_rename = text.count("老周")
assert n_rename == 24, f"老周 计数 {n_rename} != 24"
text = text.replace("老周", "达哥")

# ---------- 2. 正文直引号 -> 弯引号（保护代码块/行内代码） ----------
lines = text.split("\n")
in_fence = False
n_quote_chars = 0
odd = []
for i, line in enumerate(lines):
    if line.strip().startswith("```"):
        in_fence = not in_fence
        continue
    if in_fence:
        continue
    parts = re.split(r"(`[^`]*`)", line)
    open_q = True  # 每行重置，跨行内代码段保持
    total = sum(p.count('"') for p in parts if not p.startswith("`"))
    if total % 2:
        odd.append((i + 1, line))
    for j, part in enumerate(parts):
        if part.startswith("`") or '"' not in part:
            continue
        out = []
        for ch in part:
            if ch == '"':
                out.append("“" if open_q else "”")
                open_q = not open_q
                n_quote_chars += 1
            else:
                out.append(ch)
        parts[j] = "".join(out)
    lines[i] = "".join(parts)
assert not odd, f"奇数引号行: {odd}"
text = "\n".join(lines)

# ---------- 3. 内容改写规则（作用于改名+弯引号后的文本） ----------
rules = []
def rule(name, old, new, n=1):
    rules.append((name, old, new, n))

# U-1 系统依赖改为每包一行、字母序
rule("U-1 依赖命令块",
"""```bash
# 安装 Yocto 项目 Scarthgap 5.0 在 Ubuntu 24.04 上的必需系统包
# 本命令直接在终端执行，无需写入文件
sudo apt update
sudo apt install -y gawk wget git diffstat unzip texinfo chrpath \\
    build-essential python3 python3-pip python3-pexpect python3-jinja2 \\
    python3-git xz-utils debianutils iputils-ping cpio file locales \\
    zstd lz4 socat libacl1 python3-subunit
```""",
"""```bash
# 安装 Yocto 项目 Scarthgap 5.0 在 Ubuntu 24.04 上的必需系统包
# 以下命令逐条在终端执行，无需写入文件
sudo apt update
sudo apt install -y build-essential
sudo apt install -y chrpath
sudo apt install -y cpio
sudo apt install -y debianutils
sudo apt install -y diffstat
sudo apt install -y file
sudo apt install -y gawk
sudo apt install -y git
sudo apt install -y iputils-ping
sudo apt install -y libacl1
sudo apt install -y locales
sudo apt install -y lz4
sudo apt install -y python3
sudo apt install -y python3-git
sudo apt install -y python3-jinja2
sudo apt install -y python3-pexpect
sudo apt install -y python3-pip
sudo apt install -y python3-subunit
sudo apt install -y socat
sudo apt install -y texinfo
sudo apt install -y unzip
sudo apt install -y wget
sudo apt install -y xz-utils
sudo apt install -y zstd
```""")
rule("U-1 配套叙事", "他先把系统包一口气装上。", "他照着清单把系统包逐个装上。")

# U-2 whoami 输出 akai -> oops
rule("U-2 whoami 输出", "```text\nakai\n```", "```text\noops\n```")

# U-3 删除 1.1.3（代理内容并入 1.6.2，随 R-2 改写自包含）
rule("U-3 删除 1.1.3",
"""### 1.1.3 网络代理配置（按需）

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

> **⚠️ 注意**：这里只是让当前 Shell 能访问外网。构建工具（BitBake）的 fetcher 跑在子进程里，不一定会自动继承 `http_proxy`。如果后面下载源码阶段大量报错，请看本章末尾的“踩坑 2：代理未传入 BitBake”。

## 1.2 Clone Poky 并了解目录结构""",
"""## 1.2 Clone Poky 并了解目录结构""")

# U-4 clone 改 https + 提示口径翻转
rule("U-4 clone 命令",
"git clone -b scarthgap git://git.yoctoproject.org/poky.git",
"git clone -b scarthgap https://git.yoctoproject.org/poky.git")
rule("U-4 提示翻转",
"> **💡 提示**：如果公司网络不允许 `git://` 协议，可以把地址换成 `https://git.yoctoproject.org/poky.git`。两种协议最终拿到的代码完全一致。",
"> **💡 提示**：如果公司网络只允许 `git://` 协议出口，可以把地址换回 `git://git.yoctoproject.org/poky.git`。两种协议最终拿到的代码完全一致。")

# R-1 ls -1 输出改为 scarthgap 顶层完整清单
rule("R-1 顶层目录清单",
"""```text
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
```""",
"""```text
bitbake
contrib
documentation
LICENSE
LICENSE.GPL-2.0-only
LICENSE.MIT
MAINTAINERS.md
MEMORIAM
meta
meta-poky
meta-selftest
meta-skeleton
meta-yocto-bsp
oe-init-build-env
README.hardware.md
README.md
README.OE-Core.md
README.poky.md
README.qemu.md
scripts
SECURITY.md
```""")
rule("R-1 汇报补 documentation",
"“`bitbake/` 下面是构建引擎；`meta/` 是 OE-Core；`meta-poky/` 像发行版配置；`meta-yocto-bsp/` 是官方参考 BSP；`scripts/` 里放了 `runqemu` 这些脚本。”",
"“`bitbake/` 下面是构建引擎；`meta/` 是 OE-Core；`meta-poky/` 像发行版配置；`meta-yocto-bsp/` 是官方参考 BSP；`scripts/` 里放了 `runqemu` 这些脚本；`documentation/` 是官方文档的源码，其余顶层文件主要是许可和说明。”")

# U-5 层定义台词
rule("U-5 层定义台词",
"达哥点点头：“对。你把 `meta/`、`meta-poky/`、`meta-yocto-bsp/` 这三个都叫 **层（Layer）**——组织元数据的目录结构，可以叠加复用。本章只认识它们的目录组织，具体机制后面再讲。”",
"达哥点点头：“对。`meta/`、`meta-poky/`、`meta-yocto-bsp/` 这三个都是 **层（Layer）**：组织元数据的目录结构，可以叠加复用。本章只认识它们的目录组织，具体机制后面再讲。”")

# U-6 source vs ./ 说明
rule("U-6 source 说明",
"> **🔥 重要**：`oe-init-build-env` 必须用 `source` 或 `.` 执行，不能 `./oe-init-build-env`。因为它要修改当前 Shell 的 `PATH`、`BUILDDIR` 等环境变量，用子进程执行的话这些改动不会传回来。",
"> **🔥 重要**：`oe-init-build-env` 必须用 `source` 或 `.` 执行，不能 `./oe-init-build-env`。这个脚本要修改的是当前 Shell 的 `PATH`、`BUILDDIR` 等环境变量；用 `./` 执行会让它在子进程里运行，子进程对环境变量的改动只留在子进程里，不会反映回当前 Shell——结果就是当前 Shell 的 `PATH` 里没有 `bitbake`，下一条 `bitbake` 命令直接提示找不到。")

# U-2 两处 /home/akai 字面路径
rule("U-2 BUILDDIR 输出", "/home/akai/workspace/build", "/home/oops/workspace/build")
rule("U-2 which 输出", "/home/akai/workspace/poky/bitbake/bin/bitbake", "/home/oops/workspace/poky/bitbake/bin/bitbake")

# U-8 同族：conf 两文件冗余首释
rule("U-8 conf 文件台词",
"达哥指着 `conf/` 里的两个文件：“这两个文件你以后会天天改。`local.conf` 是 **本地配置文件（local.conf）**，构建目录下的用户级配置入口，优先级高于层中的默认值；`bblayers.conf` 是 **层配置文件（bblayers.conf）**，声明 BitBake 加载哪些层及其搜索顺序。”",
"达哥指着 `conf/` 里的两个文件：“这两个文件你以后会天天改。`local.conf` 是本地配置文件，构建目录下的用户级配置入口，优先级高于层中的默认值；`bblayers.conf` 是层配置文件，声明 BitBake 加载哪些层及其搜索顺序。”")

# U-7 MACHINE 叙事
rule("U-7 MACHINE 叙事",
"阿凯打开 `local.conf`，发现默认的 `MACHINE` 是 `qemux86-64`。他想起 tiger 是 ARM 的，但本章目标是 `qemuarm64`，于是跑来问达哥。",
"阿凯打开 `local.conf`，发现默认的 `MACHINE` 是 `qemux86-64`——x86 架构的 QEMU 机器，跟 tiger 的 ARM 架构对不上。要不要直接改成 tiger 的机器名？可本章说好先用官方的 `qemuarm64` 跑通。他拿不准，跑来问达哥。")

# U-8 同族：MACHINE 冗余首释（reviewer 建议①）
rule("U-8 MACHINE 首释",
"阿凯回去改配置。**MACHINE 变量（MACHINE）** 是 BitBake 变量，设置目标机器名称，写在 `local.conf` 里。**Machine 配置（Machine）** 则定义目标硬件平台特征。本章只把 `MACHINE` 设为现成的 `qemuarm64`，不写新的 Machine 配置。",
"阿凯回去改配置。`MACHINE` 是设置目标机器名称的 BitBake 变量，写在 `local.conf` 里；机器配置则定义目标硬件平台的特征。本章只把 `MACHINE` 设为现成的 `qemuarm64`，不写新的机器配置。")

# U-9 叙事句拆开追加/修改
rule("U-9 叙事句",
"达哥让他在原有 `local.conf` 末尾追加或修改以下几项。这里涉及的 BitBake 变量先在正文里过一遍：",
"达哥让他打开 `local.conf`：已存在的变量在原行修改，被注释的先取消注释；没有的变量追加到文件末尾。这里涉及的 BitBake 变量先在正文里过一遍：")

# U-8 + U-13 变量释义列表统一
rule("U-8/U-13 变量列表",
"""- **下载目录（DL_DIR）** 存放从网络拉取的源码包。
- **Sstate 缓存（Sstate）** 的落盘位置由 `SSTATE_DIR` 变量指定。
- **临时目录（TMPDIR）** 是构建过程中的临时工作目录。
- `BB_NUMBER_THREADS` 控制 BitBake 自身并行调度的任务数。
- `PARALLEL_MAKE` 控制 `make` 编译时的并行 job 数。
- `EXTRA_IMAGE_FEATURES` 是控制镜像级特性的 BitBake 变量。
- `debug-tweaks` 是 **镜像特性（IMAGE_FEATURES）** 的一项，启用空 root 密码等调试便利。""",
"""- **下载目录（`DL_DIR`）** 存放从网络拉取的源码包。
- **共享状态缓存（sstate）** 的落盘位置由 `SSTATE_DIR` 变量指定。
- **临时目录（`TMPDIR`）** 是构建过程中的临时工作目录。
- **并行任务数（`BB_NUMBER_THREADS`）** 控制 BitBake 自身并行调度的任务数。
- **编译并行度（`PARALLEL_MAKE`）** 控制 `make` 编译时的并行 job 数。
- **镜像级特性（`EXTRA_IMAGE_FEATURES`）** 控制镜像级功能的开关。
- `debug-tweaks` 是可加入 `EXTRA_IMAGE_FEATURES` 的特性之一，启用空 root 密码等调试便利。""")

# U-9 代码注释同步
rule("U-9 注释同步",
"# 在原有 local.conf 末尾追加或修改以下内容",
"# 已存在的变量在原行修改或取消注释，没有的追加到文件末尾")

# U-13 代码注释内 Sstate
rule("U-13 注释 sstate",
"# Sstate 缓存目录：共享状态缓存的落盘位置，具体作用见 1.5.2",
"# sstate 缓存目录：共享状态缓存的落盘位置，具体作用见 1.5.2")

# U-10 并行度取值补理由
rule("U-10 并行度提示",
"> **💡 提示**：`BB_NUMBER_THREADS` 和 `PARALLEL_MAKE` 建议设成你 CPU 物理核心数或稍多一点。阿凯的机器是 8 核，所以都写 8。如果你的机器是 4 核，可以改成 4；内存不足时宁可设小一点，否则并行编译会把机器卡死。",
"> **💡 提示**：`BB_NUMBER_THREADS` 和 `PARALLEL_MAKE` 建议设成你 CPU 物理核心数或稍多一点——编译过程有大量时间花在等磁盘和网络 I/O 上，任务数略多于核心数，才能在这些等待的空隙里把 CPU 填满。阿凯的机器是 8 核，所以都写 8。如果你的机器是 4 核，可以改成 4；反方向的边界是内存：内存不足时宁可设小一点，否则并行编译会把机器卡死。")

# U-11 bblayers.conf 回填真实路径
rule("U-11 bblayers 块",
"""```bitbake
# 文件路径：~/workspace/build/conf/bblayers.conf
# oe-init-build-env 已自动生成如下内容
# 写入配置文件时，请把 /home/<your-username> 替换为你主机的实际绝对路径
BBLAYERS ?= " \\
  /home/<your-username>/workspace/poky/meta \\
  /home/<your-username>/workspace/poky/meta-poky \\
  /home/<your-username>/workspace/poky/meta-yocto-bsp \\
  "
```""",
"""```bitbake
# 文件路径：~/workspace/build/conf/bblayers.conf
# oe-init-build-env 已自动生成如下内容（路径为本机实际绝对路径）
BBLAYERS ?= " \\
  /home/oops/workspace/poky/meta \\
  /home/oops/workspace/poky/meta-poky \\
  /home/oops/workspace/poky/meta-yocto-bsp \\
  "
```""")

# U-8 同族：BBLAYERS / bitbake-layers 首释补反引号
rule("U-8 BBLAYERS 首释", "**层列表变量（BBLAYERS）**", "**层列表变量（`BBLAYERS`）**")
rule("U-8 bitbake-layers 首释", "**层管理命令（bitbake-layers）**", "**层管理命令（`bitbake-layers`）**")

# U-11 show-layers 输出回填
rule("U-11 show-layers 输出",
"""```text
layer                 path                                      priority
==================================================================
meta                  /home/<your-username>/workspace/poky/meta            5
meta-poky             /home/<your-username>/workspace/poky/meta-poky       5
meta-yocto-bsp        /home/<your-username>/workspace/poky/meta-yocto-bsp  5
```""",
"""```text
layer                 path                                     priority
==================================================================
meta                  /home/oops/workspace/poky/meta           5
meta-poky             /home/oops/workspace/poky/meta-poky      5
meta-yocto-bsp        /home/oops/workspace/poky/meta-yocto-bsp 5
```""")

# U-11 占位说明改写（连带 U-2 体例并存收尾）
rule("U-11 占位说明",
"> **💡 提示**：实际输出会显示为你主机的真实绝对路径（如 `/home/akai/workspace/poky/meta`），这里用 `<your-username>` 占位。",
"> **💡 提示**：本书示例中的用户名和路径取自配套的固化构建环境（用户 `oops`、家目录 `/home/oops`、主机名 `tiger`）；你在自己机器上看到的，会是你的实际家目录路径。")

# U-12 层优先级台词锚定数字列
rule("U-12 优先级台词",
"达哥看了一眼输出：“三个层，够用。本章先不讲层优先级和冲突，那是 chapter 3 的事。”",
"达哥看了一眼输出：“三个层，够用。注意每行最后那列数字——那就是层的优先级。优先级和层冲突本章先不讲，那是 chapter 3 的事。”")

# U-8 同族：core-image-minimal / oe-init-build-env 首释补反引号
rule("U-8 core-image-minimal 首释", "**核心最小镜像（core-image-minimal）**", "**核心最小镜像（`core-image-minimal`）**")
rule("U-8 oe-init-build-env 首释", "**构建环境初始化脚本（oe-init-build-env）**", "**构建环境初始化脚本（`oe-init-build-env`）**")

# U-13 正文 Sstate
rule("U-13 正文 sstate", "后面有 Sstate 缓存复用", "后面有 sstate 缓存复用")

# R-4 free -h 数值
rule("R-4 free -h 数值",
"如果你用的是完整 procps-ng 的 `free -h`，则会显示 `234Mi` 这样的带单位格式，数值含义相同。",
"如果你用的是完整 procps-ng 的 `free -h`，则会显示约 `228Mi` 这样的带单位格式，数值含义相同。")

# reviewer 建议② tmpfs 表述精确化
rule("建议② tmpfs 表述",
"容量约为 guest 物理内存的一半（本章默认 256Mi，所以看到约 111–114Mi）",
"容量上限约为 guest 内核可用内存的一半（本章内存 256Mi、内核可用约 227Mi，所以看到约 111–114Mi）")

# U-14 downloads 真实版本号（2026-09-23 poky scarthgap 配方核查）
rule("U-14 downloads 版本号",
"""busybox-<版本号>.tar.bz2
curl-<版本号>.tar.xz
gcc-<版本号>.tar.xz""",
"""busybox-1.36.1.tar.bz2
curl-8.7.1.tar.xz
gcc-13.4.0.tar.xz""")

# U-13 章末 Sstate 首释改为已释引用
rule("U-13 章末 sstate",
"达哥点点头：“`sstate-cache/` 里的东西叫 **Sstate 缓存（Sstate）**，缓存构建中间产物以加速后续构建。你下次改一个配置重编，很多东西不用重新编译，直接从这儿拿。`tmp/sysroots-components/` 是交叉编译用的系统根组件，本章只混个脸熟，后面讲交叉编译时再细说。”",
"达哥点点头：“`sstate-cache/` 里就是 1.3.2 说过的 sstate 缓存，缓存构建中间产物以加速后续构建。你下次改一个配置重编，很多东西不用重新编译，直接从这儿拿。`tmp/sysroots-components/` 是交叉编译用的系统根组件，本章只混个脸熟，后面讲交叉编译时再细说。”")

# R-2 踩坑 2 机制改写（1.6.2 自包含，含 U-3 并入的代理内容）
rule("R-2 踩坑 2 改写",
"""“浏览器能上网啊。”阿凯嘀咕。

达哥凑过来：“BitBake 的 fetcher 跑在子进程里，你 Shell 里的 `http_proxy` 不一定传得进去。先确认你在 `source oe-init-build-env` 之前已经 export 了代理变量。”

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

> **💡 提示**：如果你不想每次都在 Shell 里 export 代理，也可以直接在 `local.conf` 里写死 `http_proxy = "http://..."`。但这样会把个人网络配置提交到团队共享的环境里，不推荐在协作仓库里这么做。""",
"""“浏览器能上网啊。”阿凯嘀咕。

达哥凑过来：“浏览器走的是系统设置里的代理，跟终端是两回事。先看你运行 `bitbake` 的这个 Shell 里有没有 `http_proxy`。”

阿凯检查了一下，发现问题了：代理变量是他在另一个终端窗口里 export 的，当前这个窗口的 Shell 里根本没有 `http_proxy`，BitBake 自然拿不到。修复方法是在运行 `bitbake` 的当前 Shell 中 export 代理变量：

```bash
# 在运行 bitbake 的当前 Shell 中 export 代理变量
export http_proxy=http://<your-proxy>:<port>
export https_proxy=http://<your-proxy>:<port>
export no_proxy="localhost,127.0.0.1"

# 验证代理生效后能否访问 Yocto 项目上游仓库
wget -q --spider https://git.yoctoproject.org
```

确认连通后，再次构建：

```bash
# 代理生效后重新构建最小镜像
bitbake core-image-minimal
```

**环境变量传递白名单（`BB_ENV_PASSTHROUGH_ADDITIONS`）** 是 BitBake 读取环境变量的白名单，控制哪些 Shell 环境变量会传给构建任务。`oe-init-build-env` 已经把 `http_proxy`、`https_proxy`、`no_proxy` 等代理变量加进白名单，BitBake 每次启动时按白名单从当前 Shell 环境里读取。所以关键是“运行 `bitbake` 的这个 Shell 里有没有 export”，与先 export 还是先 `source oe-init-build-env` 无关；如果确认 export 了仍失败，再检查代理地址本身是否可用。

> **💡 提示**：不想每次都手动 export，可以把这三行写进 `~/.bashrc` 再 `source ~/.bashrc`（或者直接重开终端）。也可以在 `local.conf` 里写死 `http_proxy = "http://..."`，但这样会把个人网络配置带进团队共享的环境，不推荐在协作仓库里这么做。""")

# U-15 / R-5 移除章末 git tag
rule("U-15 移除章末 tag",
"""最后，阿凯在 Poky 仓库里打了一个 tag，标记本章结束时的状态。

```bash
# 在 poky 仓库标记本章终点
cd ~/workspace/poky
git tag chapter1
```

> **💡 提示**：如果你在后续章节遇到构建问题，可以随时 `git checkout chapter1` 回到本章结束时的代码状态，然后对比差异排查。

## 1.8 延伸阅读""",
"""## 1.8 延伸阅读""")

for name, old, new, n in rules:
    c = text.count(old)
    if c != n:
        print(f"规则未命中: {name}  命中 {c} 次（期望 {n}）")
        sys.exit(1)
    text = text.replace(old, new)
    print(f"OK  {name}")

# ---------- 4. 残留断言 ----------
assert "老周" not in text, "老周 残留"
assert "akai" not in text, "akai 残留"
assert "<your-username>" not in text, "<your-username> 残留"
assert "Sstate" not in text, "Sstate 残留"
assert "<版本号>" not in text, "<版本号> 残留"
assert "git://git.yoctoproject.org/poky.git" in text, "git:// 备选写法应保留在提示中"
# 正文（围栏外、行内代码外）不得再有直引号
in_fence = False
stray = []
for i, line in enumerate(text.split("\n")):
    if line.strip().startswith("```"):
        in_fence = not in_fence
        continue
    if in_fence:
        continue
    for part in re.split(r"(`[^`]*`)", line):
        if not part.startswith("`") and '"' in part:
            stray.append((i + 1, line))
assert not stray, f"正文残留直引号: {stray}"

open(PATH, "w", encoding="utf-8", newline="\n").write(text)
print(f"\n改名 老周->达哥: {n_rename} 处")
print(f"正文直引号->弯引号: {n_quote_chars} 个字符")
print("全部规则命中，文件已写盘。")
