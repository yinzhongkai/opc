# T-016 QEMU 模型阶段原始证据

执行人：book-yocto / qemu-engineer。主要工程执行跨 2026-10-14～15，最终核对与治理交付为 2026-10-16（Asia/Shanghai）；各日志内宿主时间按原样保留。

[主报告](../t016-qemu-model-validation.md)解释结果及阶段边界。[manifest.json](manifest.json)列出 263 个远端原始文本文件的路径、字节数、SHA-256 和所属包，以及最终工程/执行文件/固件身份。九个日志包按原始字节拼接，每个 FILE 头注明原文件长度及 hash；头尾分隔符属于归档封装，内部数据未重排、裁剪或改写。传输后逐包核对长度和 SHA-256；本目录 .gitattributes 禁止日志文本换行转换。

- [controller.log](controller.log)：工具回读、基线、实际命令、注册路径定位失败、ROM owner 修复、patch 应用/风格检查及工程提交。跨日失效的初次 setup 回读明确标为未知，由后续 probe 核实状态；不把缺失返回码改成 0。
- [final-state.log](final-state.log)：交付时远端精确提交/干净状态、master 未变、无本阶段 QEMU 残留进程及配置 hash；[local-governance.log](local-governance.log)保存本地结构/写入边界/字节校验。
- [recipe-env-before.log](recipe-env-before.log)、[recipe-env-after.log](recipe-env-after.log)：完整 bitbake -e 输出；可以核对 native 配方、已有 patches、S/B/EXTERNALSRC 和 MACHINE=qemuarm64。
- [session-builds.log](session-builds.log)：devtool modify/build、固件构建及三轮模型矩阵的完整顶层输出/返回码文件、checkpatch、辅助 machine 枚举。
- [bitbake-tasks.log](bitbake-tasks.log)：本轮 native 任务日志，包括真实 configure/compile/install/sysroot 输出。
- [build-configuration.log](build-configuration.log)：最终 bblayers/local.conf 与 devtool append；workspace override 仍在，尚未作正式清理复验。
- [firmware-inspection.log](firmware-inspection.log)：工具 argv/输出、manifest、ELF headers/symbols/disassembly。ELF/raw/map 和源代码仍位于工程工作区，不复制到 OPC。
- [model-run-1.log](model-run-1.log)：初轮 QOM owner 故障。原脚本的 14 个 pass 标签含 3 个崩溃误判，**不能作为 14 项负向已通过的结论**；主报告据真实返回码重新判读。
- [model-run-2.log](model-run-2.log)：ROM owner 已修复，实际 guest/reset 状态完成；脚本最后错误地从 stderr 查 trace，整体返回 1，保留失败。
- [model-run-3.log](model-run-3.log)：修正 trace 落点并严格核对拒绝诊断后的最终 16/16 通过；完整 QMP、GDB、指令/PL011 trace、寄存器/哨兵、退出码和 argv 都在包内。

OPC 仅归档治理证据。工程脚本快照、失败源码快照、ROM dumps、ELF/raw/map 和 native 二进制保留于 tiger 的授权临时目录；正式源码与 patch 位于 meta-tiger 的阶段提交，详见主报告。模型自测与后续 Yocto 干净构建/runqemu 验收分别记录。
