# T-008 真实环境运行核查记录：chapter 1 环境信息

```text
项目：book-yocto
关联任务：T-008
核查人：reviewer
核查日期：2026-09-23
核查对象：workspace/yocto/task02-1-把Yocto跑起来.md（T-006 统一修订版，Git 9772f9c，792 行）
环境基线：Docker Client/Server 29.8.0；镜像 embedded:dev（由 workspace/docker/dockerfile 构建）；容器 books（docker start books 恢复，docker exec 逐条执行）；容器内用户 oops、家目录 /home/oops、主机名 tiger、Ubuntu 24.04.5 LTS、清华 apt 镜像源、locale en_US.UTF-8
核查范围声明：仅书稿环境信息与容器实测的一致性（依赖包、用户/路径/主机名、oe-init-build-env 行为、目录与版本输出）；不修改书稿正文；首次构建与 QEMU 启动不在本次范围（见"未执行项"）
```

## 一、一致项（逐条证据）

| # | 书稿位置 | 核查内容 | 结果 | 证据（容器实测） |
|---|---|---|---|---|
| 1 | 1.1.1（L23-51） | 24 个依赖包逐条 `sudo apt install -y <pkg>` | ✅ 一致 | `sudo apt update` rc=0 后，24 包按书稿逐条执行全部 rc=0（输出末行均为 "0 upgraded, 0 newly installed..."，即包名全部有效且已就位）：build-essential, chrpath, cpio, debianutils, diffstat, file, gawk, git, iputils-ping, libacl1, locales, lz4, python3, python3-git, python3-jinja2, python3-pexpect, python3-pip, python3-subunit, socat, texinfo, unzip, wget, xz-utils, zstd |
| 2 | 1.1.1 后续（L59-68） | `whoami` 输出 `oops` | ✅ 一致 | 实测输出 `oops`；容器用户由 docker run -u oops / 镜像 USER oops 确定 |
| 3 | 1.1.2 / U-11 口径 | 用户名/家目录/主机名 | ✅ 一致 | `whoami`=oops、`echo $HOME`=/home/oops、`hostname`=tiger，提示符形态 oops@tiger；与 Dockerfile（useradd oops、--hostname tiger）及书稿回填值三者一致 |
| 4 | 1.1.2（L99-107） | locale 为 en_US.UTF-8 | ✅ 一致 | `locale` 输出 LANG/LC_ALL 及全部子项均为 en_US.UTF-8（Dockerfile 已固化 LANG/LC_ALL 环境变量，书稿 sed/locale-gen 步骤在容器内为幂等确认） |
| 5 | 1.1 叙事 | 宿主系统 Ubuntu 24.04 LTS | ✅ 一致 | `/etc/os-release`：Ubuntu 24.04.5 LTS；apt 源为 mirrors.tuna.tsinghua.edu.cn（与 Dockerfile sed 替换一致） |
| 6 | 1.2.1（L121） | `git clone -b scarthgap https://git.yoctoproject.org/poky.git` | ✅ 一致 | 实测克隆成功（1m48s，6687 个文件）；`git branch` 输出 `* scarthgap`，与 L137 输出块一致 |
| 7 | 1.2.1（L147-169） | `ls -1` poky 顶层 21 项清单及顺序 | ✅ 一致（逐字） | 容器（en_US.UTF-8）实测输出与书稿 21 项逐字一致、顺序一致（bitbake, contrib, documentation, LICENSE, LICENSE.GPL-2.0-only, LICENSE.MIT, MAINTAINERS.md, MEMORIAM, meta, meta-poky, meta-selftest, meta-skeleton, meta-yocto-bsp, oe-init-build-env, README.hardware.md, README.md, README.OE-Core.md, README.poky.md, README.qemu.md, scripts, SECURITY.md）；`ls -1A` 证实 .gitignore/.templateconf 为隐藏文件不显示。此项同时为 T-006 R-1 复核的实测证据 |
| 8 | 1.2.3（L194-204 等） | bitbake/bin、meta/、meta-poky/、scripts/ 目录列表 | ✅ 一致 | 书稿所列条目全部存在且带 `# ... (省略)` 标记；实测另有 bitbake-getvar、toaster 等（meta/ 另有 classes-global、recipes-sato 等），省略标记使用正确 |
| 9 | 1.2.3 注释（L240-241） | genericarm64 属 meta-yocto-bsp；qemuarm64 由 meta/conf/machine/ 提供 | ✅ 一致 | 实测 `meta-yocto-bsp/conf/machine/` 含 genericarm64.conf；`meta/conf/machine/qemuarm64.conf` 存在 |
| 10 | 1.3.1（L287-329） | `source oe-init-build-env ~/workspace/build/` 行为 | ✅ 一致 | rc=0；执行后 PWD 自动切换为 /home/oops/workspace/build（书稿 L290 所述）；`echo $BUILDDIR`=/home/oops/workspace/build（L304）；`which bitbake`=/home/oops/workspace/poky/bitbake/bin/bitbake（L315），三项与书稿输出块逐字一致 |
| 11 | 1.3.2（L335） | local.conf 默认 MACHINE 为 qemux86-64 | ✅ 一致 | 实测 conf/local.conf L39：`MACHINE ??= "qemux86-64"`（弱默认值，未注释行）；EXTRA_IMAGE_FEATURES ?= "debug-tweaks" 默认已存在（L149），与书稿"保留默认 debug-tweaks"口径一致 |
| 12 | 1.3.3（L396-400） | bblayers.conf 自动生成内容 | ✅ 一致（逐字） | 实测 BBLAYERS 三行路径 /home/oops/workspace/poky/meta、meta-poky、meta-yocto-bsp 与书稿示例逐字一致 |
| 13 | 1.8（L792） | BitBake 2.8 手册与版本匹配 | ✅ 一致 | 容器实测 `bitbake --version` = BitBake Build Tool Core version 2.8.1（poky scarthgap 自带） |
| 14 | 1.6.2（L768） | oe-init-build-env 将代理变量并入 BB_ENV_PASSTHROUGH_ADDITIONS | ✅ 一致（源码核查） | poky scarthgap scripts/oe-buildenv-internal L110-114：BB_ENV_PASSTHROUGH_ADDITIONS_OE 含 HTTP_PROXY http_proxy HTTPS_PROXY https_proxy ... NO_PROXY no_proxy；与书稿改写后的机制说明一致（T-006 R-2 复核证据） |

## 二、不符项（按评审问题格式登记）

- **E-1（低，事实）**｜位置：1.2.3 节 L245-252 `ls meta-yocto-bsp/` 输出块｜实测：scarthgap 的 meta-yocto-bsp 顶层为 `conf、lib、README.hardware.md、recipes-bsp、recipes-graphics、recipes-kernel、wic`——**书稿所列 `recipes-core` 不存在**（`ls -d meta-yocto-bsp/recipes-core` 实测 "No such file or directory"）；书稿同时漏列 lib/、wic/（有省略标记可容忍漏列，但不能列出不存在项）｜影响：读者对照目录时会发现输出块与实际不符｜建议：移除 `recipes-core` 一行（可选择补 lib/ 或保持省略）｜原稿责任人：writer
- **E-2（中，事实）**｜位置：1.3.3 节 L414-419 `bitbake-layers show-layers` 输出块｜实测输出层名为 **`core`、`yocto`、`yoctobsp`**（取自各层 layer.conf 集合名），书稿写的是目录名 `meta`、`meta-poky`、`meta-yocto-bsp`；实测 path 列宽更宽，分隔线 104 个 `=`。实测原文：

  ```text
  layer                 path                                                                    priority
  ========================================================================================================
  core                  /home/oops/workspace/poky/meta                                          5
  yocto                 /home/oops/workspace/poky/meta-poky                                     5
  yoctobsp              /home/oops/workspace/poky/meta-yocto-bsp                                5
  ```

  ｜影响：读者照做看到的层名与书稿输出块完全不同（三个全不同）；U-12 台词锚点"每行最后那列数字是优先级"仍成立（优先级均为 5 ✓），U-11 路径回填部分 ✓ 不受影响｜建议：按实测输出改写该输出块；如需保留目录名讲法，可在正文补一句层集合名与目录名的对应（core=meta、yocto=meta-poky、yoctobsp=meta-yocto-bsp）｜原稿责任人：writer
- **E-3（低，事实）**｜位置：1.3.1 节 L323-329 `ls $BUILDDIR/conf/` 输出块｜实测 conf/ 含 5 个文件：`bblayers.conf、conf-notes.txt、conf-summary.txt、local.conf、templateconf.cfg`；书稿列 3 个且**无省略标记**（与本轮 R-1 修复前同类问题）｜影响：读者会看到多出的两个文件｜建议：补 `# ... (省略)` 标记或列全｜原稿责任人：writer

## 三、观察项（非错误，如实记录）

- O-1｜1.1.2 的 `df -h ~`（/dev/sda2 196G）与 `free -h`（swap 2.0Gi）输出块为叙事场景示例，容器实测为 overlay 1007G/可用 892G、内存 15Gi/swap 4.0Gi。书稿未声称这两块输出来自固化环境（💡 仅声明用户名与路径取自固化环境），不构成不一致；但读者在 Docker 环境对照时会看到明显不同数值，建议 writer 斟酌是否加"以本机实际为准"（1.5.1 的 uname 已带此类标注）。
- O-2｜上游 scarthgap 分支当前 HEAD 为 cbd62bb2a9（2026-09-21），`git describe` = yocto-5.0.20-106，poky.conf DISTRO_VERSION = "5.0.20"。书稿 1.5.1 guest banner 示例为 5.0.18——按当前 HEAD 实测回填时将得到 5.0.20；与 U-14 已记录的"版本号随上游更新"口径一致，建议随 D-001 实测回填统一处理（如需版本稳定可 pin 提交）。

## 四、未执行项（超出本次范围，注明理由）

| 书稿位置 | 内容 | 理由 |
|---|---|---|
| 1.4.1-1.4.3 | 首次构建 4000+ 任务、4059 tasks 汇总行、deploy 产物清单 | 需实际构建（约 1-3 小时），本次仅核查环境信息；建议随 D-001 实测回填安排 |
| 1.5.1 | runqemu 启动、guest banner（5.0.18）、uname/cpuinfo/free/df guest 内数值 | 依赖构建产物，未执行 |
| 1.5.2 | 构建后 build/ 顶层清单（downloads/、sstate-cache/ 等） | 需构建后状态；实测构建前 build/ 为 bitbake-cookerdaemon.log、cache、conf、tmp（show-layers 解析后），与书稿的构建后语境不冲突 |
| 1.6.1/1.6.2 | 踩坑场景的报错输出复现 | 故障注入式复现非本次范围；机制层面已由源码核查覆盖（一致项 #14） |

## 五、结论

- 书稿环境主干信息（依赖清单、用户/路径/主机名、locale、oe-init-build-env 行为、bblayers.conf、默认 MACHINE、BitBake 版本、目录结构主体）与容器实测**一致**，共 14 项。
- 不符项 3 个：E-1（meta-yocto-bsp 多出 recipes-core）、E-2（show-layers 层名与列宽）、E-3（conf/ 清单无省略标记漏 2 文件），均定位明确、可按建议直接修订。
- 观察项 2 个（O-1 叙事数值、O-2 上游点版本漂移），不阻断。
- 原始命令与输出即本记录各表"证据"列；执行人 reviewer，执行日期 2026-09-23，容器 books（镜像 embedded:dev）。
