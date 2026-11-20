# T-016 Yocto 系统验证原始证据

执行：2026-10-18，book-yocto / yocto-engineer，SSH tiger/oops；正式构建树 `/home/oops/workspace/build-t016-yocto`。结论、工程版本和边界见[阶段报告](../t016-yocto-system-validation.md)。这里保留失败和成功原文，不能仅据某个 launcher 返回 0 推断启动通过。

## 文件和完整性

- [controller.log](controller.log)：接收时完整只读核对命令、输出和返回码。
- [orchestration-supplement.log](orchestration-supplement.log)：只读 devtool 检索和辅助控制探针的原始输出，包括错误路径/本机语法/旧 PID 已退出的记录。
- [handoff-2/manifest.json](handoff-2/manifest.json)：9 个日志包的字节数、SHA-256、记录数，以及 291 份原文件的绝对路径/字节数/hash。
- [verification.json](verification.json)：复制到 OPC 后逐包及逐份内嵌文本重新计算的本地核验回执，9/9 包、291/291 文本一致。
- [governance-check.json](governance-check.json)：任务/索引只变本人区段、冻结输入和书稿未变的本地范围回执。
- [governance-checks.log](governance-checks.log)：框架、差异和范围检查的命令/原始输出，包括本地 Git 控制脚本修正前的两次检查失败及恢复。

`handoff-2/*.log` 是 JSON 数组，每项为 `{path, bytes, sha256, text}`；text 是完整 UTF-8 原文（换行和控制字符由 JSON 转义保留）。对 text 重新 encode UTF-8 后的字节数/hash 对应原文件，不能对展示界面里的折行或截取重算。包本身的 hash 另在 manifest 中。manifest SHA-256 为 `5f3e6190d3193963c3a676f945fd3f7b39998b7e45d7d4b95c8e832ddc270708`。

归档只含文本与身份摘要，不把 QEMU/BIOS/ELF/ROM dump、Git bundle 等二进制产品搬入 OPC。测试/控制脚本的文本快照属于原始执行证据，正式工程文件以报告中的 meta-tiger 路径和精确提交为准。完整二进制、正式部署文件和保留源码仍在授权远端工作区。

| 日志包 | 原文件数 | 内容 |
|---|---:|---|
| [configuration-and-control](handoff-2/configuration-and-control.log) | 76 | 原配置/override 备份、source 前状态、两轮 env/图、构建/系统控制、实际 sigdata 展开、asset-audit、stage-final-check 和执行脚本 |
| [critical-task-logs](handoff-2/critical-task-logs.log) | 90 | QEMU/helper/fixture 的真实 run.do_* 与 log.do_*；跳过 canonical 软链接，保留编号原件 |
| [deployed-text-identities](handoff-2/deployed-text-identities.log) | 7 | 正式 manifest、commands、readelf/nm/disassembly、native identity、qemuboot.conf |
| [interrupted-dependency](handoff-2/interrupted-dependency.log) | 2 | 首轮被中断 LLVM 编译的完整 run/log |
| [runqemu-2](handoff-2/runqemu-2.log) | 13 | 冷启动及同进程两次 reset，QMP/GDB/指令/PL011、最终 argv/state、原测试快照 |
| [model-2](handoff-2/model-2.log) | 76 | 正式 helper/fixture 的 16 项模型复验及每例 argv/诊断/trace/状态 |
| [deployment-2](handoff-2/deployment-2.log) | 5 | 首次缺 BIOS/kernel 的私有配置与原输出，保留第二例失败；controller/traceback 另在 control 包 |
| [deployment-3](handoff-2/deployment-3.log) | 9 | 修正隔离后的三项部署诊断、私有配置、零指令 trace 和测试快照 |
| [runqemu-recovery-3](handoff-2/runqemu-recovery-3.log) | 13 | 错误注入后的正式 conf 恢复启动和两次 reset |

首份远端 handoff-1 归档未包含错误注入的私有 `.conf`；补齐过滤规则后重新生成 handoff-2，旧归档未覆盖。本目录交付的是完整 handoff-2。

## 定位执行与失败

control 包中按 path 后缀定位原文：

- `parse-controller.log` / `parse-1.rc`：nounset 导致环境初始化失败；parse-2.sh、parse-2.rc 为恢复。
- `build-1.sh` / `build-1.log` / `build-1-stop.json` / `build-1.rc`：先保留源码再解除 override、隔离构建起点、图形依赖的受控中断；`system-1.rc=1` 表示等待的前置构建未成功，不是 CPU 验收。
- `build-2.sh` / `build-2.log` / `build-2.rc=0`：TCG/串口配置后的正式构建；uninative 告警保留，全部所需任务成功。
- `qemu-env-2.log` / `fixture-env-2.log` / `task-depends-2.dot` / `generate-signature-2.log`：最终变量、三条依赖边及实际签名。
- `asset-audit.json`：工具路径/hash、生成配置/native 身份、固件源和输出身份；`stage-final-check.json`：精确工程头、源码/refs/bundle 保留对照、任务日志身份、运行及终态。
- `system-pipeline-2.log` / `system-2.rc=1`：冷启动/reset 和模型已通过，部署第二例失败，后续恢复未执行。
- `system-pipeline-3.log` / `system-3.rc=0`：3/3 部署诊断后，正式主线恢复通过。unexpected-kernel 的 launcher=0 是记录到的锁定启动器局限，伴随明确 QEMU 拒绝和零执行，绝不记为成功启动。

runqemu-2 的 QEMU PID 997465、恢复轮 PID 997810；各轮三次完整 guest 标记，GDB 三次 main 前初始化观察，同进程两次 RESET 事件和恢复前状态，以及 QMP quit/launcher=0 均在相应包。16 项模型结果在 model-2 的 results.json；直接 QEMU 的 14 项拒绝均要求退出 1 和对应诊断。原输出含 TERM 未设置时的 tput 提示，不对其删改。

结束时全部文本包逐字节复核通过。目录本地回执和报告不改写原日志中的失败、警告、路径或源版本；独立审校和整章批准仍待后续。
