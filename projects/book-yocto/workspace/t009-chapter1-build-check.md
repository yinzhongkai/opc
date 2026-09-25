# T-009 核查记录：chapter 1 首次构建与启动（真实服务器实测）

- 任务：T-009（真实环境首次构建与启动核查 chapter 1 主线）
- 执行人：reviewer
- 执行日期：2026-09-25
- 执行环境：远程服务器 tiger（`ssh tiger`，192.168.3.120），用户 oops、家目录 /home/oops、主机名 tiger——与书稿配套固化环境三要素一致；Ubuntu 24.04.4 LTS（kernel 6.8.0-101-generic），8 核 / 30Gi 内存 / home 分区可用 647G；裸机 Ubuntu，非 WSL/容器。
- 核对对象：chapter 1（workspace/yocto/task02-1-把Yocto跑起来.md，Git ff9c794 = 89ccb72 内容 + 编辑器格式化，内容零改动）
- 环境基线：poky scarthgap 分支，HEAD `77d1feb37e`（build-appliance-image: Update to scarthgap head revision）；BitBake 2.8.1。注意与 T-008 容器基线（cbd62bb2a9，yocto-5.0.20-106）不是同一提交，scarthgap 点版本有漂移。
- 既有状态说明：该服务器 2026-09-19 01:26 已完成过一次 core-image-minimal 全量构建（deploy 产物时间戳 20260919012616，用户自行按书稿操作），本机无该次构建的控制台日志。本次执行为**断点续跑**（sstate 全命中），未复现"首次全量构建"的从零过程；首次构建耗时与"0 didn't need to be rerun"行的首次形态未在本次复测，如实标注。
- 执行方式声明：构建与 QEMU 均由 reviewer 经 ssh 在远程服务器实际执行；guest 内命令通过向 nographic 串口注入输入完成，全程日志留存于服务器 ~/t009-build-rerun.log、~/t009-runqemu.log。
- 关联记录：[T-008 chapter 1 环境核查记录](t008-chapter1-env-check.md)（本记录补其"未执行项"）

## 一、local.conf 配置核对（对应书稿 1.3.2）

按书稿将 `MACHINE ?=` 改 `MACHINE =`、取消 DL_DIR/SSTATE_DIR/TMPDIR 注释、末尾追加 BB_NUMBER_THREADS/PARALLEL_MAKE（原文件已备份为 conf/local.conf.t009-bak）。`bitbake-getvar` 七项生效值实测：

| 变量 | 实测生效值 | 书稿 | 结果 |
|---|---|---|---|
| MACHINE | qemuarm64 | qemuarm64 | ✅ 一致 |
| DL_DIR | /home/oops/workspace/build/downloads | ${TOPDIR}/downloads | ✅ 一致 |
| SSTATE_DIR | /home/oops/workspace/build/sstate-cache | ${TOPDIR}/sstate-cache | ✅ 一致 |
| TMPDIR | /home/oops/workspace/build/tmp | ${TOPDIR}/tmp | ✅ 一致 |
| BB_NUMBER_THREADS | 8 | "8" | ✅ 一致 |
| PARALLEL_MAKE | -j 8 | "-j 8" | ✅ 一致 |
| EXTRA_IMAGE_FEATURES | debug-tweaks | debug-tweaks（保留默认） | ✅ 一致 |

BitBake 版本 2.8.1，与 1.8 节延伸阅读的 BitBake 2.8 手册链接一致 ✅。

## 二、bitbake core-image-minimal（对应书稿 1.4）

- 实测命令：`source oe-init-build-env ~/workspace/build/` 后 `bitbake core-image-minimal`（LC_ALL/LANG=en_US.UTF-8）。
- 实测任务汇总行（日志原文）：

```text
NOTE: Tasks Summary: Attempted 4073 tasks of which 4073 didn't need to be rerun and all succeeded.
```

- 结果：✅ 基本一致。书稿 L482 行为 `Attempted 4059 tasks of which 0 didn't need to be rerun and all succeeded`——成功标志的文本形态逐字一致；总数 4059 vs 4073 为 scarthgap 点版本漂移（T-009 登记时已预期；T-008 容器 9-23 试起跑同为 4073）。"didn't need to be rerun"数值不同是本次为断点续跑（sstate 全命中，4073 全部免重跑）所致，首次从零构建形态本次未复测（见观察项 O-2）。
- deploy 产物 `tmp/deploy/images/qemuarm64/` 实测清单：`Image`、`Image--6.6.151+git0+...bin`、`core-image-minimal-qemuarm64.rootfs-20260919012616.{ext4,manifest,qemuboot.conf,spdx.tar.zst,tar.bz2,testdata.json}` 及无时间戳软链名。书稿 1.4.3 列出的关键文件（`Image`、`.ext4`、`.qemuboot.conf`、`.manifest`）全部存在，多余条目在书稿省略标记覆盖范围内 ✅。
- 观察项 O-1（提示级）：书稿任务总数 4059 为历史实测值，当前 scarthgap 头部实为 4073；1.4.3 输出块无"以实测为准"标注。不阻断阅读（构建成功标志不受影响），建议定稿微调时给该行加注或在回填时更新。
- 观察项 O-2（证据边界）：书稿"首次构建约需 1-3 小时，视网络与 CPU 而定"本次未复测（服务器既有 9-19 首建无计时记录；本次续跑约 90 秒全命中缓存，不构成对首建耗时的证据）。

## 三、runqemu 启动与 guest 验证（对应书稿 1.5.1）

- 实测命令：`runqemu qemuarm64 nographic slirp`，串口输出停止后出现登录提示，输入 `root` 空密码直接进入 shell ✅（与书稿叙事一致）。
- 逐项核对：

| 项 | 书稿 | 实测 | 结果 |
|---|---|---|---|
| 登录横幅 | `Poky (Yocto Project Reference Distro) 5.0.18 qemuarm64 /dev/ttyAMA0` | `Poky (Yocto Project Reference Distro) 5.0.20 qemuarm64 /dev/ttyAMA0` | ⚠️ 不符 N-1（见下） |
| uname -a | `Linux qemuarm64 6.6.127-yocto-standard #1 SMP PREEMPT ... aarch64 GNU/Linux` | `Linux qemuarm64 6.6.151-yocto-standard #1 SMP PREEMPT Tue Aug 18 18:50:33 UTC 2026 aarch64 GNU/Linux` | ✅ 一致（书稿该处已有"小版本号随 Scarthgap 点版本更新而变化，以本地输出为准"注释，口径覆盖） |
| /proc/cpuinfo | 0xd07 / BogoMIPS 125.00 / Features fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid 等 | 逐字段一致 | ✅ 一致 |
| free（busybox, KiB） | total 232996 | total 232852 | ✅ 基本一致（同机不同次启动的正常波动；书稿 227Mi 换算口径仍成立） |
| df -h /dev/root | `19.3M 12.9M 4.9M 72% /` | 逐字一致 | ✅ 一致 |
| df -h tmpfs/devtmpfs | 111.5M / 113.8M / 113.7M | 111.4M / 113.7M / 113.6M | ✅ 基本一致（可用内存微差引起的末位 0.1 波动） |
| ls /bin/ | busybox、cat、cp 等 | busybox、cat、cp 均存在 | ✅ 一致 |
| poweroff | guest halt，"QEMU 进程不一定自动退出"，给 Ctrl+a x 退出法 | guest halt 后 QEMU 本次自动退出（`reboot: Power down`，runqemu cleanup） | ✅ 一致（书稿"不一定"措辞兼容两种情况；Ctrl+a x 法本次无需触发，未实测） |

- 不符项 N-1（低severity，事实漂移）｜位置：1.5.1 节登录横幅输出块（当前版 L527）｜依据与影响：书稿写 Poky `5.0.18`，实测 `5.0.20`（scarthgap 点版本漂移）；该输出块无"以本地输出为准"类标注，而同页 uname 行有——读者照做会看到不同版本号且无预期管理｜建议：仿照 uname 行在命令注释或块后补"版本号随点版本更新，以本地输出为准"，或回填时更新为实测值｜原稿责任人：writer
- 对比观察：T-008 容器中出现的 `WARNING: You are running bitbake under WSLv2` 在本裸机环境未出现，佐证该警告为 Docker Desktop WSL2 后端所致，非书稿环境问题。

## 四、build 目录结构（对应书稿 1.5.2）

- `ls -1 $BUILDDIR` 实测（构建完成后）：`bitbake-cookerdaemon.log`、`cache`、`conf`、`downloads`、`sstate-cache`、`tmp` 共 6 项。
- 不符项 N-2（低severity，状态时点）｜位置：1.5.2 节 `ls -1 $BUILDDIR` 输出块（当前版 L643）｜依据与影响：书稿清单含 `bitbake.lock`，但该锁文件仅在 BitBake 运行期间存在，构建结束退出后即删除；1.5.2 的语境是构建完成后"看懂 build 目录"，读者此时照做看不到 `bitbake.lock`｜建议：移除该行，或加注"仅构建进行中存在"｜原稿责任人：writer
- `ls $BUILDDIR/downloads/` 实测含 `busybox-1.36.1.tar.bz2`、`curl-8.7.1.tar.xz`、`gcc-13.4.0.tar.xz`，与 U-14 回填的真实版本号逐一相符 ✅。
- `ls $BUILDDIR/tmp/` 实测 13 项：abi_version、buildstats、cache、deploy、hosttools、log、pkgdata、saved_tmpdir、sstate-control、stamps、sysroots-components、sysroots-uninative、work、work-shared——书稿列出的 7 项（deploy、hosttools、stamps、sysroots-components、sysroots-uninative、work、work-shared）全部存在，其余在书稿省略标记覆盖范围内 ✅。
- 书稿对 downloads / sstate-cache / tmp 三区职责的讲解与实测目录行为一致 ✅。

## 五、汇总

- 一致项：local.conf 七项生效值、BitBake 2.8.1、任务汇总行形态与成功标志、deploy 关键产物四件、guest 登录流程（root 空密码）、uname（注记覆盖）、cpuinfo 全字段、free/df 数值（波动在口径内）、/bin 基础命令、poweroff 行为、downloads 三个真实版本号、tmp/ 七项、三区职责讲解。
- 不符项：N-1（登录横幅版本号 5.0.18→5.0.20，无免责标注）、N-2（1.5.2 清单含构建结束后不存在的 bitbake.lock）。均为低severity，建议随 writer 定稿微调（U-17~U-20）一并处理。
- 观察项：O-1（任务总数 4059 vs 4073 漂移，建议加注）、O-2（首次构建耗时与首次形态本次未复测，证据边界如实标注）。
- 未执行项：无（T-008 遗留的"首次构建与 runqemu"两项本次均已执行；首次从零构建因服务器已有 9-19 首建成果，以断点续跑替代并如实标注证据边界）。
- 远程改动留痕：`conf/local.conf` 按书稿 1.3.2 落盘（备份 local.conf.t009-bak）；日志 ~/t009-build-rerun.log、~/t009-runqemu.log 留存于服务器供复核。
