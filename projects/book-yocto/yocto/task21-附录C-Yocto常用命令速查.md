## 附录 C　Yocto 常用命令速查

> **这份文档面向本书的全体读者。**三条收录原则：一，只收全书十六章正文与附录 A/B 里真实敲过的命令，没敲过的一律不收；二，每条命令带"见 chapter N"回指，忘了细节顺章号翻回去；三，按工作流阶段分六组，不按字母序——查命令时人想的是"我在干哪步活"，不是"命令首字母是什么"。附录 A 管应用开发者，附录 B 管下一任 BSP 工程师，这一篇管所有人的每一天。全部命令以 Yocto Scarthgap 5.0 / BitBake 2.8.x 为准，跨版本使用先查官方文档（见篇末延伸阅读）。尾声里说过，这张表适合打印出来贴在显示器边上——它就按这个用途写。

### C.1 环境与层

每天打开终端的第一件事是 `source` 构建环境；层的注册、摘除与官方体检在这组。

| 命令 | 干什么 | 出处章 |
|------|--------|--------|
| `source oe-init-build-env <build目录>` | 初始化构建环境并进入构建目录；每新开一个终端都要重做一次 | chapter 1 |
| `bitbake-layers add-layer <层路径>` | 把层注册进 `bblayers.conf`；带整配置预校验，失败则回滚 | chapter 3 / 6 |
| `bitbake-layers remove-layer <层路径>` | 摘除层注册；同样有预校验与回滚 | chapter 3 / 6 |
| `bitbake-layers show-layers` | 列出当前生效的层、优先级与路径 | chapter 1 / 3 / 6 |
| `bitbake-layers show-recipes [-f] <配方名>` | 查配方在哪个层、什么版本；`-f` 给出完整路径 | chapter 2 |
| `bitbake-layers show-appends <配方名>` | 查哪些 bbappend 叠在这个配方上，按层注册顺序列出 | chapter 4 |
| `yocto-check-layer <层目录>` | 官方体检：层结构与通用性检查；chapter 13 复用进 CI | chapter 3 / 13 |

> **⚠️ 注意**：每新开一个终端都要重新 `source oe-init-build-env`；devtool 会话与 oe-init-build-env 会话不混用（见 chapter 14）。

### C.2 构建与查询

构建本体，加"变量终值"与"构建计划"两类查询。其中 `bitbake -e <目标> | grep ^<变量>=` 一条是本书出场次数最多的查询命令。

| 命令 | 干什么 | 出处章 |
|------|--------|--------|
| `bitbake <镜像名>` | 构建整镜像（core-image-minimal 到 tiger-image） | chapter 1 / 12 |
| `bitbake <配方名>` | 只构建单个配方 | chapter 4 / 6 / 7 / 8 |
| `bitbake -e <目标> \| grep ^<变量>=` | 查变量的最终生效值；不带目标则是全局查询 | chapter 2 起 |
| `bitbake -n <目标>` | 干跑构建计划：只解析不执行，看任务数与报错收敛 | chapter 5 / 7 / 8 / 11 |
| `bitbake -c compile -f <配方名>` | 强制重跑编译任务（快迭代手法） | chapter 8 |
| `bitbake -c cleansstate <配方名>` | 把配方连 sstate 缓存一起清掉，重建从源码再来 | chapter 8 / 16 |

### C.3 运行与验证

从总装到起跑，再到自动化验证与对账。`mknandimg.sh` 是本书自产的总装脚本，不是 Yocto 自带命令——破例收录一行：它是这个项目每天都会敲的"项目命令"。板上的验证命令（ptest-runner、ubiattach、devmem 等）散见各章验证段，不在本表。

| 命令 | 干什么 | 出处章 |
|------|--------|--------|
| `bash ~/workspace/meta-tiger/scripts/mknandimg.sh` | NAND 整片总装：bootloader 三件加三卷 UBI 铺进 512 MiB 镜像 | chapter 10 起 |
| `runqemu qemuarm64 nographic slirp` | 第一次起跑（脚手架镜像）；slirp 用户态网络首见于此（1.5.1） | chapter 1 |
| `runqemu qemuarm64 <镜像名> nographic slirp` | 对照组起跑：slirp 用户态网络，主机 2222 端口转发到板上 22 | chapter 12 / 13 / 14 |
| `runqemu tiger-aarch64 nographic` | tiger 组全链起跑（BL1 → U-Boot → kernel） | chapter 10 / 12 / 15 |
| `bitbake <镜像名> -c testimage` | 自动化整机测试（先构建镜像，再跑这条） | chapter 13 |
| `buildhistory-diff` | 构建账本对账：最近一次与上一次构建的包与体积差异 | chapter 13 |
| `~/workspace/poky/scripts/resulttool report tmp/log/oeqa` | 把 testimage 结果汇总成可读报告（未 source 会话时用此全路径） | chapter 16 |

> **⚠️ 注意**：`bitbake -c testimage` 不替你建镜像——先 `bitbake <镜像名>` 再 `-c testimage`，否则缺 `testdata.json` 当场 fatal（机制陈述见 13.3.2）。

### C.4 配方开发

devtool 五件套加内核配置两条。patch 的导出、重放与发送归 C.6。

| 命令 | 干什么 | 出处章 |
|------|--------|--------|
| `devtool add <配方名> <源码目录>` | 把本地源码收编成新配方（eSDK 会话内） | chapter 14 |
| `devtool modify <配方名>` | 给已有配方开工作区，externalsrc 接管源码树 | chapter 15 |
| `devtool build <配方名>` | 工作区内构建，迭代分钟级 | chapter 14 / 15 |
| `devtool finish <配方名> <目标层>` | 收尾：提交导出为 patch 收回层里，工作区随之拆除 | chapter 15 |
| `devtool check-upgrade-status <配方名>` | 瞭望上游是否有新版本 | chapter 13 |
| `bitbake -c menuconfig <配方名>` | 交互式内核配置 | chapter 8 |
| `bitbake -c savedefconfig <配方名>` | 把 menuconfig 的结果导出为 defconfig，记得回仓库提交 | chapter 8 / 9 |

> **🔥 重要**：`devtool finish` 前必须 `git commit`——工作区不干净时 finish 直接被 `Source tree is not clean` 报错拦网，什么都不丢；可一旦加 `-f` 强推，未提交的改动会被静默丢弃（拦网与 `-f` 之禁见 chapter 15 的 15.3.1 坑 1）。

### C.5 SDK 与交付

本组只收生产侧动作；安装之后的应用侧使用细节见附录 A。

| 命令 | 干什么 | 出处章 |
|------|--------|--------|
| `bitbake <镜像名> -c populate_sdk` | 产出标准 SDK 安装器（cross 工具链加目标 sysroot） | chapter 14 |
| `bitbake <镜像名> -c populate_sdk_ext` | 产出 eSDK 安装器（带 bitbake/devtool 与 sstate 快照） | chapter 14 |
| `./<安装器>.sh` | 自解压安装；通配符形态见附录 A.1 | chapter 14 |
| `source <SDK目录>/environment-setup-*` | 加载交叉编译环境；环境变量速查见附录 A.2 | chapter 14 |
| `sha256sum <安装器>.sh > <安装器>.sha256` | 归档留校验和；chapter 16 升级为整发布包的清单级校验 | chapter 14 / 16 |

### C.6 git 工作流与章末 tag

patch 三件套，外加全书的 tag 约定——这张表本身就是最后一枚 tag 的落点。

| 命令 | 干什么 | 出处章 |
|------|--------|--------|
| `git format-patch -<条数> -o <目录>` | 导出 patch 序列作为层内资产；`git format-patch -1` 是一次性发送产物 | chapter 4 / 15 |
| `git am -3 <patch>` | 三方合并重放 patch；解完冲突 `git am --continue` | chapter 15 |
| `git send-email --dry-run ...` | 发送前彩排（本书诚实停在 dry-run） | chapter 15 |
| `git tag <章末tag名>` | 章末书签：正文用 `chapterN`，边界章用名字 | 每章末 |

全书 tag 的归属约定（前言"怎么读这本书"首次立下，此处收拢备查）：

| tag | 打在哪个仓库 | 覆盖范围 |
|-----|-------------|----------|
| `chapter1` / `chapter2` / `prologue` | poky | meta-tiger 尚不存在的三章 |
| `chapter3` … `chapter16` | meta-tiger | 自 chapter 3 起的全部正文 |
| `epilogue` / `appendix-a` / `appendix-b` / `appendix-c` | meta-tiger 现有 HEAD | 边界章纯书签，无交付改动 |
| 其余仓库（四个开发态仓库与 meta-arm） | 不打 tag | 只读引用或各有维护方 |

至此书签打齐——从 poky 上的 `chapter1` 到 meta-tiger 上的 `appendix-c`，这本书的仓库时间线完整了。以后哪天翻不回某条命令的上下文，先用本表找到章号，再用 tag 检出那一刻的仓库——前言里"怎么读这本书"交代的读法，与本表互为呼应。

本篇无任何仓库交付改动——meta-tiger 保持原样。按边界章惯例，tag 打在现有 HEAD 所指的提交上，作为纯书签（与 `chapter16`、`epilogue`、`appendix-a`、`appendix-b` 指向同一提交）：

```bash
# 附录 C 终点标记：本篇无交付改动，tag 打在 meta-tiger 现有 HEAD 所指的提交上
cd ~/workspace/meta-tiger
git tag appendix-c
```

---

**延伸阅读**

1. BitBake 用户手册（bitbake 命令行与任务语法的权威口径）：https://docs.yoctoproject.org/bitbake/2.8/
2. Yocto 开发任务手册（devtool、testimage、SDK 等工作流的命令索引）：https://docs.yoctoproject.org/5.0/dev-manual/index.html
