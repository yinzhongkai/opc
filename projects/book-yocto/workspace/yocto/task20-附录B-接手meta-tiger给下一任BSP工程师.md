## 附录 B　接手 meta-tiger：给下一任 BSP 工程师

> **这份文档面向接手 meta-tiger 仓库的下一任 BSP 工程师。**写这份文档的人已经去带 falcon 项目组（尾声交代过），这份文档是他交接那天留下的：仓库怎么读、关键配方在哪、已知的坑有哪些、上游怎么跟，以及实在没辙了怎么找到他。默认你通读过本书正文，或有同等的 Yocto 项目经验——你有完整构建环境、有仓库权限、要改仓库，本篇不教新概念，只做导览与索引；没读过正文的，每个条目都标了去哪章补课。
>
> 五节一张图：B.1 仓库怎么读 / B.2 关键配方在哪 / B.3 已知的坑 / B.4 上游怎么跟 / B.5 怎么找到我。

### B.1 仓库结构导览

你的工作区里是 4+1 格局：四个开发态仓库（linux-tiger、qemu-tiger、tf-a-tiger、u-boot-tiger）各归各家，功能开发在那里做；meta-tiger 是集成层，通过 bbappend 与 patch 把开发成果引入构建。这份文档只导览集成层——四个开发态仓库的内容就是它们自己的权威，不在这里展开，也不在下面这棵树里；poky 与 meta-arm 是上游源码，同样不在。

读这棵树时记住一条方法论：**每样资产有且只有一个家**。看到树就知道东西该放哪——硬件事实在 `conf/machine/`，发行版策略在 `conf/distro/`，对上游配方的修改走 `recipes-*/` 下的 bbappend，自写组件自立 recipe。反过来说也成立：不要在 meta-tiger 里做功能开发（开发态/集成态纪律，序章就立了）；源码手改回开发态仓库改，别直接 vim 构建工作区里的源码——下次 bump SRCREV，手改会静默消失（chapter 8 坑 1）。

meta-tiger 终态全景（行尾标注来源章）：

```text
meta-tiger/
├── .gitlab-ci.yml                  # CI 流水线（chapter 13）
├── COPYING.MIT                     # 层许可证（chapter 3）
├── MAINTAINERS                     # 维护者名单（chapter 3）
├── README                          # 层说明（chapter 3）
├── conf/
│   ├── layer.conf                  # 层声明（chapter 3；chapter 6 加 LAYERDEPENDS meta-arm）
│   ├── distro/
│   │   ├── tiger-distro.conf       # 发行版公共基座（chapter 11；chapter 12/13/14 逐次追加）
│   │   ├── tiger-distro-dev.conf   # 开发态 delta（chapter 11；chapter 13 加测试姿态）
│   │   └── tiger-distro-prod.conf  # 生产态 delta（chapter 11；chapter 16 加 archiver）
│   └── machine/
│       └── tiger-aarch64.conf      # MACHINE 配置，硬件事实全集（chapter 5–10 逐章长成；chapter 12 删 IMAGE_INSTALL 归队镜像配方——conf 里没有 IMAGE_INSTALL 行是正常的）
├── lib/
│   └── oeqa/runtime/cases/
│       └── tiger_sysinfo.py        # 自定义 runtime 测试用例（chapter 13）
├── recipes-bsp/
│   ├── qemu/
│   │   ├── qemu-system-native/     # 三个 tiger machine patch（chapter 4）
│   │   │   ├── 0001-hw-arm-Add-tiger-machine-skeleton.patch
│   │   │   ├── 0002-hw-arm-tiger-Wire-up-onboard-devices.patch
│   │   │   └── 0003-hw-arm-tiger-Register-default-configs-and-docs.patch
│   │   └── qemu-system-native_%.bbappend
│   ├── trusted-firmware-a/
│   │   └── trusted-firmware-a_%.bbappend   # TF-A 集成（chapter 6；chapter 10 拧上 TFA_UBOOT）
│   └── u-boot/
│       ├── u-boot/
│       │   └── boot.cmd            # 引导脚本（chapter 7；chapter 10 修 console 指错）
│       └── u-boot_%.bbappend       # chapter 7
├── recipes-core/
│   ├── busybox/
│   │   ├── busybox/
│   │   │   └── watchdog.cfg        # 打开 CONFIG_WATCHDOG（chapter 15）
│   │   └── busybox_%.bbappend      # chapter 15
│   ├── images/
│   │   └── tiger-image.bb          # 出货镜像（chapter 12；chapter 14 SDK 定制）
│   ├── tiger-sysinfo/
│   │   ├── tiger-sysinfo/
│   │   │   ├── run-ptest           # ptest 入口（chapter 13）
│   │   │   ├── tiger-sysinfo.service  # chapter 12
│   │   │   └── tiger-sysinfo.sh    # chapter 12
│   │   └── tiger-sysinfo_1.0.bb    # chapter 12；chapter 13 加 inherit ptest
│   └── tiger-testkey/
│       ├── tiger-testkey/
│       │   └── authorized_keys     # 测试公钥（chapter 13；私钥永不进仓库）
│       └── tiger-testkey_1.0.bb    # chapter 13
├── recipes-extended/
│   └── timezone/
│       └── tzdata_%.bbappend       # 默认时区（chapter 12）
├── recipes-kernel/
│   └── linux/
│       ├── linux-tiger/
│       │   └── watchdog-sp805-fix-max_timeout-reporting-off-by-one.patch  # chapter 15，Pending
│       └── linux-tiger.bb          # 内核配方（chapter 8；chapter 15 挂 patch）
└── scripts/
    └── mknandimg.sh                # NAND 总装脚本（chapter 10；chapter 12 镜像换名 tiger-image）
```

逐区职责速览五句：`conf/` 是两张桌子——machine 答"硬件是什么"，distro 答"发行版要什么策略"；`recipes-*/` 按 OE 分类法归位，改上游配方的 bbappend 与自写组件的 recipe 都在这里；`scripts/` 是总装工具，把 deploy 目录的零件拼成整片 NAND 镜像；`lib/oeqa/` 是自测考题，testimage 按 BBLAYERS 逐层收集；`.gitlab-ci.yml` 是流水线，把正文敲过的命令按顺序排了一遍。

章末 tag 自 `chapter3` 起只打在 meta-tiger 上（到 `chapter16`，外加 `epilogue`、`appendix-a` 两个纯书签）；chapter 1/2 与 `prologue` 三枚书签打在 poky——那时 meta-tiger 还不存在。meta-arm 与四个开发态仓库从不打——别给它们补。

### B.2 关键 recipe 索引

| 文件 | 干什么 | 建立于 | 后续改动 | 接手要点 |
|------|--------|--------|----------|----------|
| `conf/layer.conf` | 层身份证：BBFILES / 优先级 / 兼容声明 | chapter 3 | chapter 6 LAYERDEPENDS 加 meta-arm | 跨 LTS 时在这里加系列代号（B.4.3） |
| `conf/machine/tiger-aarch64.conf` | 硬件事实全集 | chapter 5 | chapter 6–10 逐章长成；chapter 12 归队删 IMAGE_INSTALL | 每行都要能回答"这行回答什么问题"（尾声 falcon 账本法） |
| `conf/distro/tiger-distro.conf` | 发行版公共基座 | chapter 11 | chapter 12 PACKAGECONFIG zstd + hostname；chapter 13 buildhistory；chapter 14 SDK 命名三件套 | 两态共用的策略都进这里 |
| `conf/distro/tiger-distro-dev.conf` / `-prod.conf` | dev/prod 两态 delta | chapter 11 | chapter 13 测试姿态进 dev；chapter 12 只读落点挪 `:pn-tiger-image`；chapter 16 archiver 进 prod | 切换在 local.conf，纪律见 B.3.5 |
| `recipes-bsp/qemu/qemu-system-native_%.bbappend` + 三个 patch | 虚拟板本体 | chapter 4 | 无 | patch 全部 `Inappropriate`，台账见 B.3.1 |
| `recipes-bsp/trusted-firmware-a/trusted-firmware-a_%.bbappend` | TF-A 集成 | chapter 6 | chapter 10 `TFA_UBOOT = "1"` | u-boot 动一次，FIP（固件镜像包）跟着重打一次（chapter 10） |
| `recipes-bsp/u-boot/u-boot_%.bbappend` + `boot.cmd` | 引导加载 | chapter 7 | chapter 10 bootargs console 修正 | `SRC_URI:remove` 的脆弱点见 B.4.3 |
| `recipes-kernel/linux/linux-tiger.bb` | 内核配方 | chapter 8 | chapter 15 devtool finish 直改 .bb 挂 watchdog patch | bump SRCREV 前先读 B.3.1 |
| `recipes-kernel/linux/linux-tiger/watchdog-*.patch` | sp805 max_timeout 差一修复 | chapter 15 | 无（Pending 中） | 台账见 B.3.1 |
| `recipes-core/busybox/busybox_%.bbappend` + `watchdog.cfg` | 打开 CONFIG_WATCHDOG | chapter 15 | 无 | 配置片段挂载手法回指 chapter 4 |
| `recipes-core/images/tiger-image.bb` | 出货镜像 | chapter 12 | chapter 14 TOOLCHAIN_TARGET_TASK 三件套 | IMAGE_INSTALL 加东西先量体积（B.3.4） |
| `recipes-core/tiger-sysinfo/tiger-sysinfo_1.0.bb`（+ service / sh / run-ptest） | 板子身份服务 + 自测 | chapter 12 | chapter 13 `inherit ptest` | 板上验收命令已逐条对进 `tiger_sysinfo.py` 用例（chapter 13） |
| `recipes-core/tiger-testkey/tiger-testkey_1.0.bb` | 测试公钥，仅 dev 态进镜像 | chapter 13 | 无 | 公钥可进仓库，私钥永不进（chapter 13 🔥 级纪律） |
| `recipes-extended/timezone/tzdata_%.bbappend` | 默认时区 Asia/Shanghai | chapter 12 | 无 | 时区是产品策略，归产品走 |
| `scripts/mknandimg.sh` | NAND 总装 | chapter 10 | chapter 12 镜像换名 tiger-image | rootfs_a/b 各 64 MiB 的卷预算在这份脚本里（B.3.4） |

这张表怎么用，三句话：按职责查文件——要动的东西先在这张表里找到家；按"后续改动"列去对应章看演进理由——每次改动为什么发生，正文都记了；改任何东西之前先 `git log` 这个文件——如果改的是四个开发态仓库之一，记得仓库动一次、SRCREV 跟一次（chapter 7 坑 1 的纪律）。

### B.3 已知问题清单

这份清单按"症状/现状 → 为什么 → 你该怎么办"三拍写。

#### B.3.1 patch 台账（Pending × 1 + Inappropriate × 3）

- **watchdog 差一修复**（`recipes-kernel/linux/linux-tiger/`，chapter 15）：`Upstream-Status: Pending`——提交流程走到了 `git send-email --dry-run`，邮件停在草稿箱，没有真发（诚实边界：tiger 是虚构硬件，这个 bug 是教学示例，不占用社区时间）。**你该怎么办**：每次 bump 内核 SRCREV，它必过堂——打得上就翻篇；`do_patch` 报 FAILED 就走 chapter 15 的门（摘哑弹——先移除失效 patch——再 `devtool modify` → `git am -3` → `devtool finish`）。真项目里它的下一站是 Submitted 与 lore 存档链接；"寿终正寝"那一天长什么样，见尾声第 4 节。
- **三个 qemu tiger patch**（`recipes-bsp/qemu/qemu-system-native/`，chapter 4）：`Upstream-Status: Inappropriate`——tiger 是项目虚构硬件，不上游。不上游不等于没人管：0001 骨架、0002 外设、0003 注册，每一个你都要认得它为什么在仓库里。

#### B.3.2 测试证据边界

- **全部自动化测试证据出自对照组**（qemuarm64）**加 dev 态**——testimage 与 ptest 都跑在那边（chapter 13）。tiger 组的 testimage 没有接线，机制上三处要动，这里直接写清：
  1. **起跑点冲突**：machine conf 全链接线把 `QB_DEFAULT_KERNEL` 置了 `"none"`（chapter 10）——runqemu 默认拼 `-kernel Image` 直通装载，与 `-bios bl1.bin` 从 BL1 经 NAND 起的全链路径抢同一个起跑点；而 testimage 的 QEMU 启动恰恰默认走直通内核路径（按内核镜像名拼路径递给 runqemu），接管时要先给它换条路。
  2. **网络接线**：对照组靠 slirp（QEMU 用户态网络，主机 2222 端口转发到板上 22）把 SSH 通道接进板；tiger 全链起的是 `-bios` + NAND 后端参数（经 `QB_OPT_APPEND` 递参），slirp 一族参数要重新接进这套命令行。
  3. **boot 等待与超时**：对照组默认超时内就能见到 login 提示符；NAND 全链要跑完 BL1→BL31→U-Boot→kernel 才见 login banner，boot pattern 与超时（`TEST_QEMUBOOT_TIMEOUT` 调参位，chapter 13）都得另调。

  **你该怎么办**：要接 tiger 组，从 `QB_DEFAULT_KERNEL` 这个冲突点下手；对照组那套接线（装门、配钥匙、slirp 通道）的完整实物在 chapter 13 的 testimage 一节——照机制改，别照抄。
- **prod 发布物没有专属测试报告**——发布包 MANIFEST 已如实登记（chapter 16），缺口摆在纸面上。接手后的第一件补课就是它。

#### B.3.3 硬件限制：tiger 板无网卡

tiger 板的外设清单只有 UART/I2C/SPI/存储一族，没有网卡。现状：部署与调试全走串口（chapter 14 部署一节交代过这个绕行）。候选方案两条，记在这里给你省一轮调研：一是串口 base64 贴传——busybox 源码里有 base64 applet，但 Scarthgap 的 defconfig 默认没开（与 CONFIG_WATCHDOG 同性质），要用得走 chapter 15 挂 watchdog.cfg 的同款配置片段手法开 CONFIG_BASE64，开好后小文件可行；二是给 qemu-tiger 追加 virtio-net 网卡——这要动 qemu-tiger 仓库的机器模型，板上内核侧的 virtio 驱动也得进 linux-tiger defconfig（开发态仓库的另一处改动），可行性在这两个仓库裁决。注意：附录 A 的 A.5 远程调试姿势依赖网络，在 tiger 板上不适用，别照抄。

#### B.3.4 容量预算

`mknandimg.sh` 里白纸黑字：rootfs_a 与 rootfs_b 两卷各 64 MiB。`IMAGE_INSTALL` 加东西先量体积——chapter 12 坑 2 就是顺手加了个 python3，总装时 ubinize 直接报错，"要加先量"四字落纸于那一坑；chapter 14 沿用。改完用 `buildhistory-diff` 对账，哪个包把镜像喂胖有自动答案（chapter 13 启用）。

#### B.3.5 dev/prod 切换注意

- DISTRO 切换在 `build/conf/local.conf` 里做，改动不进仓库——这是"策略进仓库"纪律的唯一合法例外（chapter 11）。
- 切态代价：任务签名族翻转，首次切态按一次全量构建计（chapter 11 坑 2）。
- 两态行为差异一句话：dev 带 debug-tweaks 与 ptest/testimage 测试姿态、-Og 调试档；prod 是 -Os 体积档加只读 rootfs。root 密码两态相同——`zap_empty_root_password` 只清得动空字段，清不动实哈希（chapter 12）。
- 发布构建必须 prod 态，并且重跑一遍 `mknandimg.sh`（chapter 16）——否则发布包里的镜像件内部 dev/prod 自相矛盾。

#### B.3.6 首轮复测清单

交接时证据覆盖的缺口集中在 tiger 组一侧。建议你头两周补齐这三件：

- tiger 组全链开机亲手跑一遍，串口日志逐行过——这部分的归档覆盖不如对照组厚；
- tiger-sysinfo 自启动纳入 CI——现在 `.gitlab-ci.yml` 的 test job 只覆盖对照组口径（MACHINE 参数是 qemuarm64）；
- SDK 安装器横幅、eSDK 的 devtool 路径亲手跑一遍——应用组天天在用的东西，值得继任者亲眼见一次（用法见附录 A，eSDK 侧见 chapter 14）。

### B.4 升级路径建议

达哥在尾声说"Yocto 会变，但工作方法不会"——这节是"会变"那半的工程师版。你接手的这套基线迟早要跟上游动，分两档走。

#### B.4.1 基线现状

| 组件 | 版本 |
|------|------|
| poky / meta-arm | scarthgap 分支（Yocto 5.0 LTS） |
| Linux 内核 | 6.6 LTS |
| TF-A | lts-v2.10 |
| U-Boot | v2024.04 |
| QEMU | 8.2.x |

时钟提醒：表里的 LTS 是长期支持版本（Long-Term Support）的缩写——内核 6.6 LTS 仍在上游维护中，截止日会被上游顺延，以 kernel.org 的 LTS 页面为准，换内核 LTS 大概率是你任期内的第一件大事；Scarthgap 的维护截止日以官方 Releases 页为准（延伸阅读第 4 条）。

#### B.4.2 点版本跟进（同 LTS 内，日常动作）

流程五步：

1. 分两层跟进：① 上游层——poky/meta-arm `git pull` 跟 scarthgap 分支头（git 检出动作，它们没有 SRCREV）；② 配方侧——bump SRCREV，linux-tiger 跟内核 6.6.y 点版本（u-boot bbappend 的 `SRCREV`、TF-A 的 `SRCREV_tfa` 同理）；
2. patch 过堂（B.3.1）——watchdog patch 打得上就翻篇，FAILED 走 chapter 15 的门；
3. 对照组跑 testimage——快，有自动化；
4. tiger 组全链总装——`mknandimg.sh` 重跑一遍；
5. `buildhistory-diff` 对账——包版本与镜像体积的变化一眼收拢。

日常瞭望的哨兵是 `devtool check-upgrade-status`（chapter 13 的现行姿势）；上游还有个 AUH 自动升级助手，知道名字即可。

#### B.4.3 跨 LTS 演练（Scarthgap → 下一个 LTS）

方法论四步：开演练分支，不动主线；拿官方迁移指南（每版一份"这一版改了什么、你要动什么"的清单）逐条对进自己的层；下面这张敏感清单逐项过；两态构建加测试全绿再合。

| 敏感点 | 为什么敏感 |
|--------|-----------|
| machine conf 三个 PREFERRED_VERSION 通配（`6.6%` / `2024.04%` / `2.10.%`） | 上游候选版本表一变就要复核；PREFERRED_VERSION 认生效 PV、不认文件名（chapter 7 反向实验） |
| `u-boot_%.bbappend` 的 `SRC_URI:remove` | 按行文本摘原 git 行，上游改了那行文本就静默落空——升级后第一件事 `bitbake -e u-boot` 查 SRC_URI 终值，别凭"构建没报错"放行（chapter 7） |
| patch rebase | watchdog patch 走 chapter 15 流程；三个 qemu patch 随上游 QEMU 大版本整体重导（chapter 4） |
| 类与变量消亡 | 升级先查自己在用的名字还在不在——checkpkg 随 distrodata.bbclass 消亡（chapter 13）；可重现构建在 Scarthgap 没有开关，旧教程里的开关变量已随 Yocto 4.0 移除（chapter 16 那次"找不到开关"的反转） |
| LAYERSERIES_COMPAT | 在 `conf/layer.conf` 加新系列代号（chapter 3） |
| TF-A `plat/` 结构、U-Boot Kconfig 命名 | 跨大版本可能变（chapter 6/7 的版本选择口径） |
| sstate / downloads | sstate 不跨版本复用，首建按全量计（chapter 11 坑 2 的签名翻转是同款机理）；downloads 可跨版本共享（chapter 16 双目录实验） |

#### B.4.4 升级后的验证顺序

一句话：先对照组（快、有自动化）再 tiger 组（全链总装）——和本书的写作验证顺序同款。

### B.5 联系前任的方式

先自助，后求人。按这个顺序翻：

1. `git log --oneline` 加章末 tag——`chapter3` 到 `chapter16` 每个 tag 都是历史锚点，随手 `git checkout`；
2. 本附录 B.2 的索引——找到文件，顺"建立于"列翻对应章节；
3. buildhistory——chapter 13 起每次构建自动记账，存档目录自成 git 仓库；
4. 发布包 MANIFEST——层基线、SRCREV 表、patch 表，chapter 16 的模板；
5. patch 台账——B.3.1。

```bash
# 自助查账三命令
cd ~/workspace/meta-tiger
git log --oneline      # 章末 tag 即锚点：chapter3 … chapter16
grep -rn "Upstream-Status" ~/workspace/meta-tiger --include='*.patch'   # patch 台账一眼收拢
cd ~/workspace/build && buildhistory-diff   # 构建目录内：最近一次构建与上一次的对账（需已 source 构建环境 + GitPython，chapter 13 注）
```

还是没答案？那就来找我——我现在在 falcon 组，工位不远。带上你查过的证据：DISTRO 与 MACHINE、完整的报错输出，外加一句"我试过什么"（附录 A 三件套精神的 BSP 版）。

这份文档的写法本身也是交付物——你将来交接给下一个人的时候，照这个格式更新它；到那天，这一节的联系方式就换成你的。

本篇无任何仓库交付改动——meta-tiger 保持原样。按边界章惯例，tag 打在现有 HEAD 所指的提交上，作为纯书签（与 `chapter16`、`epilogue`、`appendix-a` 指向同一提交）：

```bash
# 附录 B 终点标记：本篇无交付改动，tag 打在 meta-tiger 现有 HEAD 所指的提交上
cd ~/workspace/meta-tiger
git tag appendix-b
```

---

**延伸阅读**

1. Scarthgap 迁移指南（跨版本演练的脚本）：https://docs.yoctoproject.org/migration-guides/migration-5.0.html
2. Yocto BSP 开发手册（machine/layer 官方口径）：https://docs.yoctoproject.org/5.0/bsp-guide/index.html
3. Yocto 变量术语表（当字典用）：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
4. Yocto Project Wiki Releases 页（各系列维护截止日）：https://wiki.yoctoproject.org/wiki/Releases
