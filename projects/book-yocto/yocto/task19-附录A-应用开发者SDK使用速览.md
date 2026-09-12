## 附录 A　应用开发者 SDK 使用速览

> **这份文档面向 tiger 项目的应用开发者。**你手里的 SDK 安装器由 BSP 组随镜像版本交付（对应本书 chapter 14 的产出）。本附录默认你有一台 x86_64 Linux 开发机，会写 C、会用命令行——不需要装 Yocto，也不需要知道 BitBake 是什么。

全篇六张卡片，每张都是同一副面孔：**前提 → 步骤 → 验证**。按顺序走完，你就拥有了"装 SDK → 编应用 → 远程调试"的完整链路；撞墙了直接翻 A.6。

### A.1 SDK 安装

BSP 组交付的东西里，你只用得上一个文件：`*.sh` 自解压安装器。旁边两个 `.manifest` 文件是主机侧/目标侧的版本级装箱清单，还有一个 `.testdata.json` 是这份 SDK 的测试描述文件（BSP 组自动化验证用）——这三样你都不用打开，但别删：哪天要和 BSP 组核对"这份 SDK 里到底装了什么"，它们就是凭据。

```bash
# 进入安装器所在目录（你从 BSP 组收到它的位置）
cd <安装器所在目录>

# 安装 SDK（glob 形态：拿到什么装什么；[0-9] 段只命中标准 SDK 的版本号）
./tiger-distro-*-toolchain-[0-9]*.sh
```

> **💡 提示**：文件名里的版本段随每次交付漂移，用上面的通配符形态即可，不用背。两个边界情况：目录里若还有 `*-toolchain-ext-*.sh`，那是 eSDK 安装器（给要改系统的人，见 A.6 第 6 条）——`[0-9]` 段天然把它排除在外，别装错；若 dev/prod 两份安装器同时在场，glob 会一次展开成两个文件名，目录里只留你要装的那份。

输出（关键行）：

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

> **⚠️ 注意**：默认路径 `/opt/<distro>/<版本>`（如 `/opt/tiger-distro-dev/1.0`）需要 root 写权限——安装器检测到权限不足会自动向你借 sudo，按提示输密码即可，不必自己在命令前加 sudo。团队统一部署用这个默认路径正合适；自己机器上开发，像上面那样直接装进家目录。

记住横幅第一行——`tiger IoT Linux SDK installer version 1.0` 是这份 SDK 的版本身份证。向 BSP 组报任何问题时，先报这一行。

### A.2 环境设置

SDK 的全部本领由一个脚本注入：source 它一次，这个 shell 会话里的编译器、头文件、库就全部指向 SDK 内部。会话纪律只有一条——**每次新开 shell 都要 source**。

```bash
# 注入 SDK 环境（每次新开 shell 都要做）
source ~/workspace/sdk/tiger/environment-setup-cortexa53-tiger-linux

# sanity check 两行
$CC --version | head -1
echo $SDKTARGETSYSROOT
```

输出（关键行）：

```text
aarch64-tiger-linux-gcc (GCC) 13.x ...
/home/<your-username>/workspace/sdk/tiger/sysroots/cortexa53-tiger-linux
```

打勾清单：

- [ ] `$CC --version` 第一行以 `aarch64-tiger-linux-` 开头——交叉编译器在位
- [ ] `$SDKTARGETSYSROOT` 指向你安装目录下的 `sysroots/cortexa53-tiger-linux`——目标素材在位

source 之后，你的 shell 里多了一组环境变量。平时它们被构建系统默默消费，但排障时你需要认得它们——速查表如下：

| 变量 | 值长什么样 | 什么时候直接用它 |
|------|-----------|----------------|
| `CC` | `aarch64-tiger-linux-gcc -mcpu=cortex-a53 ... --sysroot=...` | 手写一行编译命令时（A.3） |
| `CXX` / `CPP` | 同上形态，`g++` / `gcc -E` | C++ 工程 / 只做预处理 |
| `LD` | `aarch64-tiger-linux-ld ... --sysroot=...` | 一般不直接碰，链接走 `$CC` |
| `CFLAGS` / `CXXFLAGS` / `CPPFLAGS` / `LDFLAGS` | `-O2 -pipe -g ...` 一族 | 交给 Makefile / CMake 消费（A.4） |
| `AR` / `NM` / `OBJCOPY` / `OBJDUMP` / `READELF` / `RANLIB` / `STRIP` | `aarch64-tiger-linux-` 前缀的同名工具 | 打包静态库、检查二进制 |
| `GDB` | `aarch64-tiger-linux-gdb` | 远程调试（A.5） |
| `TARGET_PREFIX` / `CROSS_COMPILE` | `aarch64-tiger-linux-` | 交叉工具链前缀；内核风格的 Makefile 认 `CROSS_COMPILE` |
| `CONFIGURE_FLAGS` | `--target=... --host=... --build=... ...` | autotools 工程（A.4） |
| `CONFIG_SITE` | `.../site-config-cortexa53-tiger-linux` | autotools 的预置答案文件，自动生效 |
| `PKG_CONFIG_SYSROOT_DIR` / `PKG_CONFIG_PATH` | sysroot 下的 pkgconfig 目录 | pkg-config 找库时自动生效 |
| `CMAKE_TOOLCHAIN_FILE` | `.../usr/share/cmake/OEToolchainConfig.cmake` | CMake 工程（A.4） |
| `SDKTARGETSYSROOT` | `~/workspace/sdk/tiger/sysroots/cortexa53-tiger-linux` | 查"头文件/库在不在"从这里查起 |
| `OECORE_TARGET_SYSROOT` / `OECORE_NATIVE_SYSROOT` | 目标侧 sysroot / 主机侧 sysroot（`sysroots/` 下两个子目录） | 写构建脚本时引用 |

这张表不是全集——`AS`、`KCFLAGS`、`ARCH`、`OECORE_SDK_VERSION` 等变量脚本同样导出，完整清单看 environment-setup 脚本本体；表里只收你排障时会直接认的。

三个名字就地解释，认得即可：**sysroot** 是 SDK 内部为目标板准备的头文件与库的根目录——交叉编译时编译器拿它当根去找头文件和库（`--sysroot` 参数就指向它），上表的 `SDKTARGETSYSROOT` 就是目标侧那一棵；**pkg-config** 是库查找辅助工具——`pkg-config --cflags --libs <库名>` 会报出这个库的头文件路径与链接参数，上表两个 `PKG_CONFIG_*` 变量保证它查的是目标侧 sysroot 而不是你主机的；**OEToolchainConfig.cmake** 是 SDK 自带的 CMake 交叉编译描述文件，声明 sysroot 与查找路径规则，A.4 会用到。

### A.3 第一个 hello world

前提：已 source 环境（A.2）。目标：验证"编得出、架构对"。

```c
/* 文件路径：hello.c（临时示例，新建） */
#include <stdio.h>

int main(void)
{
    printf("hello, tiger\n");
    return 0;
}
```

```bash
# 一行编译——三个变量全部来自环境注入，一个都不自定义
$CC $CFLAGS $LDFLAGS -o hello hello.c

# 产物是什么架构？
file hello
```

输出（关键行）：

```text
hello: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-aarch64.so.1, ...
```

在 x86_64 主机上产出了 aarch64 二进制——环境注入对了。这个二进制本机上跑不了（架构不符），部署上板怎么跑见 A.5 与 chapter 14（14.5）；本卡只管到"编得出、架构对"为止。

### A.4 三种构建系统接入 SDK

你的工程多半已经有一套构建系统。SDK 不需要你改造它——source 之后，构建系统从环境里拿编译器和参数即可。三套主流各一张卡。

**Makefile**：原则一行——`$(CC)`、`$(CFLAGS)`、`$(LDFLAGS)` 全用环境注入的值，一个都不自定义。最小骨架：

```makefile
# 文件路径：Makefile（最小骨架示例，新建）
hello: hello.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
```

`$@` / `$<` 是 make 的自动变量（目标名 / 第一个依赖名），关键是命令行里一个编译器前缀都不写死。带完整工程形态的实物见 chapter 14 的 hello-tiger/Makefile（14.5.1）。

> **⚠️ 注意**：Makefile 里硬写 `gcc` 或 `arm-linux-gnueabihf-` 之类的前缀 = 白 source 了——编出来的要么是本机二进制，要么根本找不到编译器。查出硬编码：`grep -n "gcc" Makefile`。

**CMake**：最小工程三行，cmake 直接认得交叉设置——source 之后 `CMAKE_TOOLCHAIN_FILE` 已被注入（值就是 A.2 表里那个 `OEToolchainConfig.cmake`），SDK 自带的 cmake 会读取这个同名环境变量。

```cmake
# 文件路径：CMakeLists.txt（最小示例，新建）
cmake_minimum_required(VERSION 3.16)
project(hello C)
add_executable(hello hello.c)
```

```bash
# 配置并构建（无需任何交叉相关的命令行参数）
cmake -S . -B build
cmake --build build
```

> **💡 提示**：想把依赖写死在命令行上（CI 脚本、或你的 cmake 版本偏旧不认环境变量时），用显式写法：`cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE`。两种写法效果相同，显式写法永远可靠。一个边界：环境变量只在**新构建树首次配置**时被读取——之后换 SDK 或改环境变量，已有的 build 目录不会跟着变，先删掉该目录（或清掉其中的 CMake cache）再重新配置。

**autotools**：autoconf/automake 一族构建系统——工程根目录带 `configure` 脚本的那种。两行：

```bash
# 假设你的工程带 configure 脚本
./configure ${CONFIGURE_FLAGS}
make
```

`CONFIGURE_FLAGS` 把 `--host`/`--target`/`--build` 三元组一次给齐；交叉编译时 configure 跑不了探测小程序，它的缓存变量由脚本注入的 `CONFIG_SITE` 文件预先兜住——你什么都不用管。

三套卡共用同一个验证收尾：编完 `file <产物>`，认出 `ARM aarch64` 即通过。SDK 默认也带 meson 构建工具，本篇不出步骤。

### A.5 远程调试（gdbserver）

`printf` 大法总有撑不住的时候：段错误没有现场、时序问题一插打印就消失。这时候该上调试器了。**gdbserver**：运行在目标板上的 GDB 调试代理——它在板上启动并挂住你的程序，经网络与主机侧的 GDB 会话；断点、单步、看变量，全部在主机侧操作。

> **⚠️ 注意**：tiger 板没有网卡（外设清单只有 UART/I2C/SPI/存储一族），本卡的网络调试姿势在 tiger 板上不适用。以下演示环境与 chapter 14 相同——对照组 qemuarm64（本书用 QEMU 模拟的参考板，tiger 板回来前的替身）加 tiger-image 的 dev 态（开发态：带调试便利配置的镜像形态）。真实板的调试通道（串口 gdbserver 等）请与 BSP 组确认。

前提检查表，三项各自打勾：

- **主机侧**：`echo $GDB`——输出 `aarch64-tiger-linux-gdb` 即就位。SDK 自带交叉 GDB（`$GDB` 是 environment-setup 导出的主机侧交叉调试器变量），你不需要安装任何东西。
- **目标侧**：板上执行 `command -v gdbserver`。当前 tiger-image 未包含它——向 BSP 组提需求一句话即可：在镜像配方里加 `IMAGE_INSTALL:append = " gdbserver"`（gdbserver 是 gdb 配方拆出的独立包，镜像里加上这个包名就有）。提需求时附上你要调试的程序名，方便 BSP 组一并核对依赖。
- **运行目标**：你手上有一个正在运行、网络可达、可经 SSH 登录的目标。本书读者就是 chapter 13/14 的对照组（qemuarm64，`ssh -p 2222 root@localhost` 免密）；独立使用本篇的读者，向 BSP 组要一句话即可："我要远程调试，请给我一台烧好 tiger-image（dev 态）、网络可达的实体板或 QEMU 实例，并告诉我 SSH 端口与登录方式。"

三项都就位后，先把 A.3 编出的程序送上板（主机侧、hello.c 所在目录执行）：

```bash
# 部署：把 hello 送上板（端口与免交互选项和下面的 ssh 同一套，按 BSP 组给你的替换）
scp -P 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null hello root@localhost:~
```

然后调试流三段。主机侧开通道并登录：

```bash
# 主机侧：把主机 2345 端口经 SSH 通道转发到目标的 localhost:2345
# （端口 2222 与免密钥匙是本书对照组的取值；你的目标以 BSP 组交付的地址、端口、登录方式为准，对应替换即可）
eval $(ssh-agent -s) && ssh-add
ssh -L 2345:localhost:2345 -p 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@localhost
```

板上（就在这个 ssh 会话里）：

```bash
# 板上：gdbserver 挂住程序，监听 2345 端口
gdbserver localhost:2345 ./hello
```

输出：

```text
Process ./hello created; pid = <pid>
Listening on port 2345
```

主机侧另开一个终端（同样先 source SDK 环境）：

```bash
# 主机侧：交叉 GDB 加载你编出来的原件，连上转发端口
$GDB ./hello
```

GDB 会话转录（`(gdb)` 开头的是你敲的命令，其余是输出）：

```text
(gdb) target remote localhost:2345
Remote debugging using localhost:2345
...
(gdb) break main
Breakpoint 1 at ...: file hello.c, line 6.
(gdb) continue
Breakpoint 1, main () at hello.c:6
```

hello 太小，`continue` 一脚就到头了；换成你自己的工程，断点下在你怀疑的那一行。调试结束 `kill` 或 `detach`，板上 gdbserver 随之退出。

> **⚠️ 注意**：调试符号不用额外操心——SDK 的默认 `CFLAGS` 已带 `-g`，你编出的原件自带符号。板上那份二进制被 strip 过也不影响调试：符号在主机侧的原件里，`$GDB ./hello` 加载的是它；只要两边是同一次编译的产物即可。

<!-- 【待验证·阻塞级】V41：本卡调试流端到端实测——对照组 qemuarm64 + 含 gdbserver 的 tiger-image（dev），scp 部署 + ssh -L 2345 转发 + 板上 gdbserver + 主机 $GDB + target remote 全链，依赖镜像侧先加 gdbserver（IMAGE_INSTALL:append），与 C-W29（tiger 板部署路径裁决）同批回填，续前批挂账同口径。输出块形态以实测为准。 -->

### A.6 常见问题速查

这六条大多能在正文里找到出处——完整机理见 chapter 14 的对应段落，这里只给症状与动作。

1. **报 `aarch64-tiger-linux-gcc: command not found`，或 `echo $CC` 是空的** → 这个 shell 还没注入 SDK 环境 → `source <安装目录>/environment-setup-cortexa53-tiger-linux`（A.2）。每次新开 shell 都要做——这是本篇唯一的会话纪律，也是撞墙率第一名。
2. **source 报 "Your environment is misconfigured ... unset LD_LIBRARY_PATH"** → 脚本开头的固定检查段发现 `LD_LIBRARY_PATH` 非空，拒绝注入 → `unset LD_LIBRARY_PATH` 后重新 source（它多半是你 .bashrc 或某个第三方工具链脚本留下的）。
3. **编译报 `fatal error: xxx.h: No such file or directory`** → SDK 的 sysroot 里没有这个头文件——先 `find $SDKTARGETSYSROOT -name "xxx.h"` 坐实，然后找 BSP 组在 SDK 的目标侧清单里补对应的 `-dev` / `-staticdev` 包。不要自己编库塞进 sysroot。报告时附上你要包含的头文件名，BSP 组好查。
4. **默认路径 `/opt/...` 装不上** → 该路径需要 root 写权限——安装器检测到权限不足会自动向你借 sudo，按提示输密码即可；不想碰 `/opt` 就改装家目录（A.1）。顺带一条会话边界：如果这台开发机上也有人跑 BSP 组的那种 Yocto 构建会话，SDK 环境与它不能混用——新开一个 shell，只 source 一边。
5. **应用编出来了，板上跑不了** → 先 `file` 对架构（是不是编成了本机 x86_64 产物）；架构对还不行，找 BSP 组确认你这份 SDK 与板上镜像同源同 DISTRO——不同发行版策略的库不保证兼容。
6. **要往镜像里加组件 / 改系统配置** → 这不是 SDK 的活。找 BSP 组要 eSDK（可扩展 SDK，带构建系统的版本），一句话指路，本篇不展开。

### 版本与反馈

向 BSP 组报问题时，附上三件套，一次说清：

- 安装器横幅行（A.1 的 `... installer version 1.0`）——你用的是哪份 SDK；
- 你要包含的头文件 / 链接的库名——你要什么；
- 完整的报错输出——你撞上了什么。

如果你的问题越过了 SDK 的边界——要改镜像、改仓库——那是 BSP 侧的活；接手 meta-tiger 仓库本身的同事，请翻附录 B。

本篇无任何仓库交付改动——meta-tiger 保持原样。按边界章惯例，tag 打在现有 HEAD 所指的提交上，作为纯书签（与 `chapter16`、`epilogue` 指向同一提交）：

```bash
# 附录 A 终点标记：本篇无交付改动，tag 打在 meta-tiger 现有 HEAD 所指的提交上
cd ~/workspace/meta-tiger
git tag appendix-a
```

---

**延伸阅读**

1. Yocto SDK 手册（Scarthgap 5.0）：https://docs.yoctoproject.org/5.0/sdk-manual/index.html
2. Yocto 开发任务手册（SDK / eSDK 章节）：https://docs.yoctoproject.org/5.0/dev-manual/index.html
