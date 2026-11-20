# tiger 工作区测试目录清理记录

用户在核对用途和占用后明确要求“删除吧”。本次清理从 2026-11-19 开始，2026-11-20 完成，目标为 `tiger:/home/oops/workspace` 下的 T-015、T-016 测试目录。补查发现 `t016-image-name.m9hnwe`，一并核验和清理，共 17 处。

删除前核对了目录的绝对路径、进程工作目录与打开文件、挂载点、正常构建配置及源码链接。目标均为空闲目录，正常构建及持久源码没有指向目标的依赖链接。采用明确列出的路径逐个删除。

## 清理结果

- 17 个目标目录全部删除，顶层同类测试目录剩余 0 个。
- 清理前目标分配空间为 33,560,141,824 字节，约 31.3 GiB。
- 删除前后磁盘可用空间差为 33,560,137,728 字节，约 31.3 GiB。空间测量在归档完成后进行。
- `poky`、`meta-tiger`、正常 `build`、下载缓存、sstate 缓存及持久的 `sources/qemu-system-native` 已通过清理前后快照核对。比较范围包括目录身份、Git HEAD/分支/refs/状态、工程文件内容、构建配置和源码兼容链接。
- 清理后仅将 `meta-tiger/tests/integration/README.md` 中的旧实验路径改为使用独立构建目录的说明，其余快照内容一致。远程 `git diff --check` 通过。

## 归档及追溯

归档位于 `tiger:/home/oops/workspace/archives/book-yocto/cleanup-20261119/`。历史记录中下列已删除路径对应的实验日志、配置、脚本、补丁、JSON 结果和小型实验文件，可按原相对路径在 `evidence.tar.gz` 中查找。

- 归档收录 1,742 个文件及符号链接，压缩后 8,640,943 字节，约 8.2 MiB。
- 归档 SHA-256 为 `407a2a4710396fd8311138d72438c839a91a90219a26c7eb1639ba4de8caa443`。
- 删除前逐个校验归档普通文件的内容摘要及符号链接目标。
- 同目录 `manifest.json` 保存归档文件清单、摘要、生成目录排除清单、删除进度和空间回执，`protected-before.json` 保存清理前快照，`integration-readme-before.md` 保存说明文件的原文。
- `tmp`、下载和 sstate 缓存、重复的上游源码与临时 Git 对象随清理回收。重新运行实验需要重建工具与固件。
- 本地保存了[远程清理回执](server-workspace-cleanup-20261120.json)。

## 已删除的目录

以下路径均相对于 `tiger:/home/oops/workspace/`。

```text
build-t016-yocto/
t015-chapter3-validation/
t016-firmware-main-20261114.VROaUD/
t016-image-name.m9hnwe/
t016-ld-format-20261114.1wx3vc/
t016-make-minboot.LiV2yN/
t016-make-minboot.PTC7tb/
t016-make-shell.UGGdkF/
t016-minboot-name.NRmGZ8/
t016-minboot-simple.1xX3uq/
t016-minboot-uart.b88qmv/
t016-patch-format-20261113.JD4Cyf/
t016-puts-format.zQOOSa/
t016-qemuboot-image.OPtkof/
t016-qemuboot-standard.pFxguO/
t016-reader-flow.TO6elk/
t016-wv01-20261112.k8rn7S/
```

## 补充清理 build/t016-qemu

2026-11-20 用户进一步要求核对 `build/t016-qemu`。该目录为早期模型开发的固件、构建日志和三轮模型测试结果，占用 20,156,416 字节，约 19.2 MiB。核对没有进程占用、配置引用或持久源码依赖后，完整归档并删除。

- 归档位于同一服务器归档目录的 `t016-qemu-evidence.tar.gz`，收录全部 279 个文件，压缩后 682,948 字节。
- 归档 SHA-256 为 `945cb13f04d9e91523c3840f336ad160fa99565672ce476dd1293cc4b9f11932`，删除前逐文件核验。
- 删除后目标路径不存在，磁盘可用空间增加 20,156,416 字节。
- `meta-tiger/tests/model/README.md` 的三处测试输出路径改为 `build/model-tests/` 下的示例路径，其余受保护工程、构建配置、持久源码和源码链接的快照一致。远程 `git diff --check` 通过。
- [补充清理回执](server-build-t016-qemu-cleanup-20261120.json)记录归档和空间测量。原始模型验证报告与日志继续按历史时点解释，目录中的实验文件可从完整归档查找。

## 移除 meta-tiger/tests 并同步 minboot 命名

2026-11-20 用户明确要求去掉 `meta-tiger/tests`。工程配方没有引用其中的脚本，目录内五份现存文件完整归档、逐文件校验后删除。归档位于同一服务器归档目录的 `removed-meta-tiger-tests.tar.gz`，11,645 字节，SHA-256 为 `fd73db44514ed413443a1a34492ab1c354d2a1562eb112a7b45b047117f6a432`。历史工程报告与测试结果按原执行时点保留。

用户随后明确选择“工程同步为书稿当前的 minboot 命名”。实际工程完成以下调整。

- 固件目录和配方改为 `recipes-test/minboot/minboot_0.1.bb`，链接脚本改为 `files/minboot.ld`，Makefile 生成 `minboot.bin`、`minboot.elf` 和 `minboot.map`。
- 镜像配方改为 `recipes-core/images/minboot-image.bb`，依赖 `minboot:do_deploy`，使用 `IMAGE_NAME = "${PN}-${MACHINE}"`。
- 部署固件为 `minboot-0.1-tiger.bin`、`.elf` 和 `.map`，启动配置为 `minboot-image-tiger.qemuboot.conf`，`tiger.conf` 的 BIOS 文件名同步。
- `main.c`、`start.S` 和链接脚本内容在重命名前后逐字节一致。重命名前的工程文件另存为同一归档目录的 `pre-minboot-rename.tar.gz`。

第 4 章目录树移除 `tests/`，4.6 改用 `runqemu` 与 QEMU 监控器的 `info registers`、`xp`、`cont`、`stop`、`system_reset` 和 `quit`，给出实际输出。保留当前书稿已经采用的 4.7 小结。书稿工作区 blob 为 `f85d36fa6ab6794eb69d3087b689ad46d25543d4`。

验证通过正常入口 `bitbake minboot-image` 完成，740 项任务成功，其中 730 项无需重跑。在 `tests/` 已删除的环境中使用新配置启动，核对冷启动及两次复位的 PC、EL3h、UART 四个寄存器值和三条串口问候语，最后从监控器退出，启动器返回 0。工程中旧配方名及脚本引用扫描通过，持久 QEMU 源码工作树干净。

书稿的 49 个代码块边缘空行和八个 shell 块本地语法检查通过，Makefile 示例与实际工程逐字节一致，书稿及工程差异空白检查通过。对应[工程与构建回执](remove-tests-minboot-result.json)及[监控器原始日志](manual-monitor-minboot-20261120.log)保存在本目录。
