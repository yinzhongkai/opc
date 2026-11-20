# T-016 C4-WV-01 首次外置提取实测

2026-11-12，project-manager 按用户要求在 `oops@tiger` 实测第 4 章。书稿以 `22c428c3aa4257b0072a5ba66da59af2cfc2a12c` 为基底，测试前工作区 blob 为 `21e40046a13ec1a46877c80c7694dc340aa664c0`，回填 §4.1.1、§4.1.2 和 §4.3.4 后为 `c50faac3306949ea40f7977807bd168e715cee74`。

## 固定输入与实验目录

- Poky Scarthgap 5.0.20，`77d1feb37e280733684ae8a9449fb031d5d7ff40`，BitBake 2.8.1；`MACHINE=qemuarm64`、`DISTRO=poky`。
- `meta-tiger` 锁定工程 `4dd7dfda8f2af2d59bdcfe6585418cf43eaa73e8`。构建只加载同仓 `0e3da48b34ef772532535a730a21878cc30ccf67` 的 chapter 3 layer 骨架，以及 Poky 的 `meta`、`meta-poky`、`meta-yocto-bsp`。骨架内没有 `.bbappend`。
- 新构建目录为 `/home/oops/workspace/t016-wv01-20261112.k8rn7S/build`，源码目标为 `/home/oops/workspace/t016-wv01-20261112.k8rn7S/sources/qemu-system-native`。运行前确认目标不存在，下载、sstate、TMPDIR 和 workspace 的写入都在本次实验目录。
- 实验从现有本地缓存复制 `qemu-8.2.7.tar.xz` 并校验 SHA-256 `1f0604f296ab9acb4854c054764a1ba408643fc299bd54a6500cccfaaca65b55`。现有下载和 sstate 以本地文件镜像读取，实验配置 `BB_NO_NETWORK = "1"`。

## 检查与结果

| 检查 | 预期和实际结果 |
|---|---|
| `devtool find-recipe qemu-system-native` | 退出 `0`，最后一行是 `/home/oops/workspace/poky/meta/recipes-devtools/qemu/qemu-system-native_8.2.7.bb`。 |
| 提取前 `devtool status` | 退出 `0`，显示 workspace 为空。 |
| `devtool modify qemu-system-native /home/oops/workspace/t016-wv01-20261112.k8rn7S/sources/qemu-system-native` | 退出 `0`，提取步骤 93 项任务均成功。终端报告 `Source tree extracted to` 和 `now set up to build from`，路径均为新目录。 |
| 提取后 `devtool status`、`bitbake -e qemu-system-native` | 均退出 `0`。配方已登记，`S` 和 `EXTERNALSRC` 都等于指定的新目录，`FILE` 与 `find-recipe` 一致。 |
| 新目录中的源码及 Git 历史 | `VERSION` 为 `8.2.7`，`HEAD` 和 `devtool-patched` 指向同一源码树，`devtool-base..HEAD` 有 31 个补丁提交。工作树干净，提取结果没有 `hw/arm/tiger.c`。用锁定工程的 tiger patch 执行 `git apply --check`，无输出，退出 `0`。 |
| `devtool reset -n qemu-system-native` 后复查 | reset、`devtool status` 和 `bitbake -e` 均退出 `0`。workspace 恢复为空，展开结果没有 `EXTERNALSRC=`，`S` 回到新构建的 work 目录。外置源码 `HEAD`、tree 和 refs 与 reset 前一致。 |
| 原持久源码及兼容链接 | `/home/oops/workspace/sources/qemu-system-native` 前后 `HEAD`、tree、refs 摘要和干净状态一致，原 workspace 下的兼容链接仍指向该目录。 |

C4-WV-01 通过。后续应用 tiger patch、开发构建和固件联测的连续复跑仍为 C4-WV-02。

原始文件在 `tiger:/home/oops/workspace/t016-wv01-20261112.k8rn7S/evidence/`。[日志摘录和退出码](t016-wv01-external-source-20261112.log)保存了正文所引行、31 项提交标题及完整日志的 SHA-256。

本轮隔离配置下出现空 layer、hash equivalence 镜像和无法联网取得 uninative shim 的告警，源码提取和解析正常完成。旧记录中的提取提交 `3a261f929a6154bdc315aabf4cb6c79c57f73652` 在当前持久检出中无法直接读取，因此以锁定配方实际提取结果、Git 历史和锁定 patch 的适用性核对。首次执行脚本在写出 `RESULT=pass` 后遇到尾部回车并返回 `127`，单独回读确认八条命令均返回 `0`，复核进程也返回 `0`。
