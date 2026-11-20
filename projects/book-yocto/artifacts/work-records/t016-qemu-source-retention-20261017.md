# T-016 QEMU 源码迁移与长期保存

执行人：project-manager（用户明确指定的目录维护）；日期：2026-10-17。授权原文与长期约束见 [D-015](../../DECISIONS.md)，精确实施结果见[远端原始回执](t016-qemu-source-retention-20261017.json)。

## 当前位置与历史

- SSH 主机：`tiger`，用户 `oops`；持久目录 `/home/oops/workspace/sources/qemu-system-native`。
- 原目录 `/home/oops/workspace/build/workspace/sources/qemu-system-native` 已完整迁入，原位置保留软链接，兼容已有构建和调试路径；两条路径指向同一份源码。
- branch 为 `devtool`，HEAD `7ed31149ac01a9d6bb3a682b9d768936c4315ed3`，tree `1270be61f9ee6f1132a8bcadfb2e0e800ff055a5`。源码、原有分支/标签/notes 均保留，工作树干净；没有修改模型 C/构建注册/固件或正式工程 patch。
- 该 checkout 是用户明确授权的辅助开发/学习工作区，正式交付继续采用 WORKSPACE 锁定的 meta-tiger；没有新增上游产品 fork 或改变正式仓库版本。

## devtool 与恢复

- 原生成 append 的 FILESPATH、srctreebase、EXTERNALSRC 三处旧路径改为新路径，同步更新 `.devtool_md5` 对该文件的登记；既有 initial_rev/31 个 OE patch 来源等元数据保留。
- `devtool status` 现在显示 `qemu-system-native: /home/oops/workspace/sources/qemu-system-native`；`bitbake -e qemu-system-native` 的 PN/PV 为 qemu-system-native/8.2.7，S 和 EXTERNALSRC 均为新目录，FILESPATH 指向新目录 oe-local-files。B 仍为 `/home/oops/workspace/build/tmp/work/x86_64-linux/qemu-system-native/8.2.7/qemu-8.2.7`。
- Git 对象连通性、原 refs 对账、工作树状态及 Git bundle verify 通过。本次解析使用的 BitBake 服务随后停止；不执行新的模型/系统构建或验收。
- 源码 Git 内保留 `archive/t016-minimal-model-20261017`，指向本次迁移时的模型提交；完整 refs 恢复 bundle 位于新目录 `.git/opc-retention/20261017/qemu-system-native-all-refs.bundle`，163966544 字节，SHA-256 `c1561b554b272d0a6b847fab6fb5bbcd3707bcc5060d5353713f55279878fd6e`。
- 同目录保留原 append/MD5 登记/refs、迁移回执、devtool 状态和解析变量记录；`.git/OPC-PERSISTENT-SOURCE.json` 标明保存授权与规则。首轮校验因自设 nounset 与 oe-init-build-env 不兼容而停止，日志按 attempt1 保留；关闭该选项后 devtool 与 BitBake 解析通过。模型源码未受此校验脚本问题影响。

## 后续使用与学习

后续 QEMU 开发在此检出中保留可阅读的功能提交、阶段版本和正式 meta-tiger patch/工程提交映射。清理 devtool 覆盖和正式干净构建验证时，保留本目录、Git 历史及恢复资料；删除或覆盖须经用户明确同意。历史报告按执行时点保留原路径。再次关联已有源码时先核对 devtool status、用户修改和源码基线，再按当前版本使用 `devtool modify -n qemu-system-native /home/oops/workspace/sources/qemu-system-native`；已处于 workspace 管理时直接复用。

```sh
ssh tiger
cd /home/oops/workspace/sources/qemu-system-native
git log --oneline --reverse devtool-patched..HEAD
git show 7ed31149a -- hw/arm/tiger.c hw/arm/Kconfig hw/arm/meson.build
```

本轮只验证目录、历史和源码引用保持正确；模型作者原 16/16 结论仍指向[原模型报告](t016-qemu-model-validation.md)的固定代码/产物，正式 metadata/fixture/helper、解除 externalsrc 后干净 BitBake 重构建与 runqemu 系统验收仍待 Yocto。

本轮 OPC 记录校验：框架结构和 543 处本地链接、差异空白检查通过；目录治理与证据之外的模型报告/原始评审、其他任务、正式 WORKSPACE 配置及 A-005/A-006 与维护前一致。
