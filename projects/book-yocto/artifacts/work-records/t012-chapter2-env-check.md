# T-012 chapter 2 命令、配置与输出真实环境核验记录

## 1. 核验范围与环境

- 执行者：`reviewer`
- 执行日期：2026-09-26
- 受评正文：`workspace/yocto/task03-2-读懂这个项目.md`，Git `0f2c44c`，583 行；执行前核对当前正文相对该提交无差异。
- 执行环境：远程主机 `tiger`（`192.168.3.120`），用户 `oops`；Ubuntu 24.04，x86_64，内核 6.8.0-101-generic。
- Yocto 基线：Poky `scarthgap`，提交 `77d1feb37e280733684ae8a9449fb031d5d7ff40`（`yocto-5.0.20-82-g77d1feb37e`）；BitBake 2.8.1；`MACHINE="qemuarm64"`，`DISTRO="poky"`；构建目录 `/home/oops/workspace/build`。
- 前置状态：Poky 工作树干净；`local.conf`、`bblayers.conf` 及既有 qemuarm64 构建产物可读。
- 证据边界：本任务复用 T-009 已有构建目录，执行章节中的查询、源码查看、变量解析和一次可恢复的配置反例实验；本章没有要求重新构建镜像，故未把既有产物冒充本次从零构建结果。未创建 `meta-tiger` 或四个开发态仓库，也未修改受评正文。

## 2. 逐项核验结果

| 编号 | 正文位置与对象 | 实际命令/依据 | 实际结果摘要 | 状态 |
|---|---|---|---|---|
| V-1 | L31-84：定位并查看 `qemuarm64.conf`、`qemu.inc` | `find`、`cat`、`grep` | 路径为 `/home/oops/workspace/poky/meta/conf/machine/qemuarm64.conf`；正文摘录的 `require`、`KERNEL_IMAGETYPE`、`UBOOT_MACHINE`、`SERIAL_CONSOLES`、`QB_*` 及 `qemu.inc` 三项关键变量均与实物一致 | 通过 |
| V-2 | L90-117：在 linux-yocto 元数据查驱动片段 | 正文原命令；另查构建后的 Linux 6.6.151 源码与配置 | `.scc/.cfg` 搜索为空，符合正文“可能为空”；源码中有 `amba-pl011.c`、`rtc-pl031.c`、`at24.c`，但没有 `m25p80.c` 驱动，详见 N-1 | 失败（局部） |
| V-3 | L104-115：`mtd-utils` recipe | `find ~/workspace/poky/meta/recipes-devtools -name "mtd-utils*.bb"` | 唯一命中 `/home/oops/workspace/poky/meta/recipes-devtools/mtd/mtd-utils_git.bb`，与正文一致 | 通过 |
| V-4 | L145-196：deploy 产物与 kernel provider | `ls`、`bitbake -e virtual/kernel`、`bitbake-getvar` | 有 `Image`、带时间戳及稳定链接的 `.ext4`；`PREFERRED_PROVIDER_virtual/kernel="linux-yocto"`；未发现 U-Boot、BL1/2/31、FIP/flash 产物，bootloader/TF-A provider 未设置；`QB_DEFAULT_KERNEL` 最终选择 `Image` | 通过；前瞻表述见 N-5 |
| V-5 | L248-298：layer 与 recipe 查询 | `bitbake-layers show-layers`、`show-recipes`、`show-recipes -f` | 三层名称、路径、优先级均与正文一致；`core-image-minimal` 来自 `meta` 1.0，`-f` 输出完整 `.bb` 路径 | 通过；职责解释见 N-2 |
| V-6 | L306-326：feature 与优先级机制 | `bitbake -e core-image-minimal`；本地源码与 BitBake 官方变量说明 | `DISTRO_FEATURES` 与正文一致；实际 `MACHINE_FEATURES` 还含 `rtc qemu-usermode`；`COMBINED_FEATURES` 为两者交集中的 `alsa bluetooth usbgadget vfat`；`BBFILE_PRIORITY` 机制表述正确 | 通过（输出观察 O-1） |
| V-7 | L328-337：`bblayers.conf` | `cat /home/oops/workspace/build/conf/bblayers.conf` | 三个绝对路径及文件内容与正文一致 | 通过 |
| V-8 | L358-434：镜像和包组 recipe | 正文两条 `cat`；核对 `core-image.bbclass` | `core-image-minimal.bb`、`packagegroup-core-boot.bb` 的变量与继承关系成立，`core-image.bbclass` 确实 `inherit image`；正文代码块对引号做了多余转义，详见 N-3 | 失败（展示层） |
| V-9 | L438-457：最终 `IMAGE_INSTALL` 与 runtime provider | `bitbake -e`、`bitbake-getvar` | `IMAGE_INSTALL="packagegroup-core-boot "`；`VIRTUAL-RUNTIME_base-utils="busybox"`、dev manager=`udev`、init manager=`sysvinit`，与正文机制说明一致 | 通过 |
| V-10 | L477-530：在 MACHINE 中追加 `DISTRO_FEATURES += "systemd"` | 临时备份 `qemuarm64.conf`，追加原文配置，执行原文 `bitbake -e core-image-minimal`，自动恢复并检查 Git 状态 | `bitbake` rc=1，报 `Nothing RPROVIDES 'udev'` 和目标无可构建 provider；与正文“不会报错”相反。恢复后 Poky 工作树干净 | 失败，见 N-4 |
| V-11 | L577-580：`git tag chapter2` | D-011 | 用户已确认移除逐章 tag 流程；为避免制造已废止状态，本次明确未执行 | 未执行（已确认不适用） |

## 3. 不符项与观察项

### N-1：`m25p80` 被写成当前 Linux 的独立驱动

- 位置：L91-117、L128。
- 事实：Linux 6.6.151 源码中的当前实现位于 `drivers/mtd/spi-nor/`；`m25p80` 仍作为芯片名或兼容标识出现，但不存在正文所称的 `m25p80` 驱动文件。PL011、PL031、at24 则分别有 `amba-pl011.c`、`rtc-pl031.c`、`at24.c`。
- 影响：把器件标识当成驱动名，会误导读者查源码和设备树绑定。
- 建议：改为“SPI NOR 子系统（`drivers/mtd/spi-nor/`，具体兼容串以器件和模型为准）”；如 tiger 最终确用 M25P80 型号，再把 `m25p80` 明确写成器件/兼容标识。

### N-2：三层“才凑出 qemuarm64 环境”的职责说明过度归因

- 位置：L262、L343-348。
- 事实：`qemuarm64.conf` 本身位于 OE-Core 的 `meta`；`core-image-minimal` 也由 `meta` 提供。`meta-poky` 提供 Poky DISTRO；`meta-yocto-bsp` 虽在默认 `bblayers.conf` 中启用并提供参考 BSP，但不提供 `qemuarm64` MACHINE。
- 影响：读者可能把“默认启用了某层”误解为该层是当前 MACHINE 的必需提供者。
- 建议：说明这是 Poky 模板默认启用的三层，并分别指出当前 qemuarm64 MACHINE/镜像来自 `meta`、DISTRO 来自 `meta-poky`、`meta-yocto-bsp` 用作其他参考 BSP，而不是三层缺一不可。

### N-3：源码输出块多出了反斜杠

- 位置：L377、L416-418、L429-430。
- 事实：远程 `cat` 的实际源码使用普通双引号；Markdown 代码块中的 `\"` 会把反斜杠直接展示给读者。
- 影响：标为“输出/关键行”的源码与真实文件不逐字一致，复制时会得到错误文本。
- 建议：代码围栏内无需转义双引号，删除这些反斜杠；保留原源码的续行反斜杠。

### N-4：MACHINE/DISTRO 踩坑实验的实际结果是解析失败

- 位置：L471-530，尤其 L503-507。
- 实测：在 `qemuarm64.conf` 追加原文配置后，原文命令 `bitbake -e core-image-minimal` 返回 rc=1：

```text
ERROR: Nothing RPROVIDES 'udev' (but .../packagegroup-core-boot.bb RDEPENDS on or otherwise requires it)
ERROR: Required build target 'core-image-minimal' has no buildable providers.
```

- 影响：“BitBake 不会报错，变量确实被改了”是可复现的错误结论，且正处于本章核心教学示例。
- 建议：按实测改写为“配置语法能进入解析，但越界组合可能使 provider 选择不一致并直接失败”；展示本次实际报错，或另设计并实测一个不会引发 provider 冲突的非策略型反例，不能继续保留未执行的成功输出。

### N-5：启动链阶段与产物映射混淆了 Boot ROM、TF-A BL1 和平台打包物

- 位置：L141、L189-196、L540。
- 依据：当前 qemuarm64 实测只有内核和 rootfs 关键产物；TF-A 官方构建说明把 `bl1.bin`、`bl2.bin`、`bl31.bin` 列为可能的独立构建物，并以 `fip.bin` 作为通用 Firmware Image Package。`flash.bin` 是平台集成命名，不能作为 TF-A BL2 的通用并列产物。正文表格本身也已写明 Boot ROM 不由 Yocto 构建，与小结“启动链的每个阶段都会物化为 deploy 文件”矛盾。
- 影响：读者会把 SoC Boot ROM 与 TF-A BL1 当成同一阶段，并误以为每一阶段必有统一文件名和 deploy 产物。
- 建议：先明确 tiger 设计究竟是 Boot ROM 直接进入 BL2，还是使用 TF-A BL1；把 Boot ROM 与可选 TF-A BL1 分开；将 FIP/flash 等打包文件标为平台相关、待四仓库实测定名；小结改成“由构建系统负责的阶段会按 MACHINE/provider 和平台打包规则部署相应产物”。

### O-1：同一固定基线下 `MACHINE_FEATURES` 示例少两项

- 位置：L315-320。
- 实测：

```text
MACHINE_FEATURES="alsa bluetooth usbgadget screen vfat rtc qemu-usermode"
```

- 评价：正文已经提示具体值可能随版本变化，因此不单独构成阻断；但本项目使用固化 Scarthgap 环境，建议回填当前实测值，减少读者对 `COMBINED_FEATURES` 的疑惑。

## 4. 机制性资料核查

- BitBake `BBFILE_PRIORITY`：数值更大的 layer 优先，且优先级可压过 recipe 的 `PV`；正文修正后的机制成立。依据：BitBake 官方变量说明及本环境 `show-layers`。
- `COMBINED_FEATURES`：当前实测为 `MACHINE_FEATURES` 与 `DISTRO_FEATURES` 的交集子集，正文“不是简单拼接”的修正成立。依据：Yocto Project Scarthgap 变量说明及 `bitbake -e`。
- TF-A 构建产物：官方构建说明列出 `bl1.bin`、`bl2.bin`、`bl31.bin` 等平台相关产物，FIP 默认名为 `fip.bin`；用于 N-5 的边界判断。

参考：

- <https://docs.yoctoproject.org/bitbake/2.8/bitbake-user-manual/bitbake-user-manual-ref-variables.html#term-BBFILE_PRIORITY>
- <https://docs.yoctoproject.org/scarthgap/ref-manual/variables.html#term-COMBINED_FEATURES>
- <https://trustedfirmware-a.readthedocs.io/en/latest/getting_started/build-options.html>

## 5. 结论

结论：`revise`。本章主要查询命令可执行，layer/provider/IMAGE_INSTALL/feature 的大部分修正有真实环境和源码支撑；但 N-1、N-3、N-4、N-5 含可见技术错误，N-2 会造成职责误解，须由 writer 随 T-011 统一修订后由 reviewer 复核。T-012 核验执行已完成，任务记录待 project-manager 复核后关闭。
