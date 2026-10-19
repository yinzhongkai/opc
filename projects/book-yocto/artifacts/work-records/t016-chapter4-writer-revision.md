# T-016：chapter 4 作者修订与自查交付

- 作者：book-yocto / writer；日期：2026-10-19（Asia/Shanghai）。
- 交付用途：唯一当前正文候选稿 v0.1，draft，可供 project-manager 核对后组织专业审校、独立审校及用户通读；本记录不启动评审轮、不批准正文，T-016 整体仍为 in_progress。
- 书稿：本机 `D:/book-yocto`，仓库 `git@github.com:yinzhongkai/book-yocto.git`，`feature/t-016-chapter4-revision`；路径 `yocto/task05-4-让QEMU长出tiger这块板.md`。
- 固定候选版本：`003910bf2971e10702019940535c690957740431`，正文 Git blob `394494f1f7cb078b197d562ff376f6ef8d76e9af`，792 行；普通提交 `docs(T-016): rewrite chapter 4 around verified minimal tiger boot`。仅改本章，510 insertions / 614 deletions；未推送或合入 main。
- 起点：书稿 feature/main `9c74596591728c0f2d614e357059034551a42dca`；OPC `27dceb8631de2db5756f8b0690f4da6da04af1c4`，复用任务配对分支。WORKSPACE 正式 revision 保留起点，不把开发提交冒充合入后的版本。

## 1. 接收与实际读取

已按当前会话协议刷新根/项目入口、TEAM、自身配置、writer 角色和 writing、book-production、reader-feedback、book-planning 四项技能；读取现行 v0.0.4 分支策略、项目协议/配置规范、PROJECT/STATUS/成果索引、相关决定与交接，以及 TASKS 中 2026-10-19 PM 系统交接核对和 writer 正文阶段全文。本人接收/开始记录写在该阶段，未改 PM 结论或其他成员记录。

固定设计/体例输入为 A-005 v0.2 approved / D-014、A-006 v0.1 及其继续实施授权、A-004 v0.5；与已读取版本经 Git 内容比较未变。读取两份工程报告和两次 PM 交接回执，重用未变 chapter 3/4 基线并核对必要前后章。A-005 的历史批准 SHA 和原 blob 按原记录保留，不批量改成重放后的历史 ID。

工程主机为 SSH `tiger` / `oops`，仓库 `/home/oops/workspace/meta-tiger`。实际只读重核任务 feature 为干净的 `4dd7dfda8f2af2d59bdcfe6585418cf43eaa73e8`，master 为声明的 `0e3da48b34ef772532535a730a21878cc30ccf67`；固定读取该头的 patch、四份固件源/许可、生成器、append、machine、fixture 配方、模型/集成说明和启动/部署测试脚本。实际构建 metadata 仍按系统报告的 `cd6c348836346004e801cb13f32ad3fcb12c1dfd` 定位，此后只有测试/README 调整，不宣称 4dd7dfd 是重新构建过的 metadata。

同时只读核对持久源码 `/home/oops/workspace/sources/qemu-system-native` 的干净 `devtool@7ed31149ac01a9d6bb3a682b9d768936c4315ed3`、patched 基点 `3a261f929a6154bdc315aabf4cb6c79c57f73652`。只向 stdout 导出最后一个提交并计算 SHA-256，得到 `5437db1b7a95d656840d3ef0e590b84c889a891aed378b0813e1d4b989b94797`，与正式唯一 patch 一致；没有在远端重写 patch 或源码。初次沙箱内 SSH 连接拒绝，受控权限下只读访问成功；没有关闭主机密钥校验、修改 SSH 配置或扩大操作范围。

原始证据覆盖方式：两份报告全文及 PM 回执已读；模型 9 包/263 份、系统 9 包/291 份原始文本全部重新按包及内嵌文本长度/SHA-256 校验通过。人工重点读取模型开发/提交/应用记录，以及系统配置、两轮构建/控制脚本、生成配置、GDB/启动输出、部署诊断及恢复的完整相关记录；没有把机器完整性校验说成逐行人工审查所有 GCC/LLVM 输出。完整原始证据仍由原报告/目录索引定位，未复制或改写。

## 2. 教学校准与证据对应

| 蓝图对象 | 新正文位置与处理 | 固定工程依据 |
|---|---|---|
| E-01 基线 | 4.1 说明 system-native 8.2.7、包+原 31 OE patches；4.4 精确 append | 模型报告 §1/3；系统报告 §1/3；env、show-appends、patch 原文 |
| E-02 workspace | 4.1 区分最初 devtool 默认目录与当前持久源码；4.6 保留源码再 reset -n | 模型开发记录、D-015/迁移记录；系统 override/source 前后对照 |
| E-03 模型 | 4.2 解释 Kconfig/Meson/tiger.c、CPU/ROM/RAM/UART/时钟及一次性 loader/reset | 固定真实 patch；模型报告与正式模型复验 |
| E-04 固件 | 4.3 解释入口/链接/data/BSS/栈/MMIO；4.6 正式资产；4.7 实际执行 | 固件源/链接脚本/生成器；正式 ELF/map/raw、GDB/trace |
| E-05 patch | 4.4 唯一增量、精确版本 append、files 路径/SRC_URI/已有次序；不由 layer priority 推断次序 | 格式化导出只读 hash 对照；实际 32 项应用与正式源比对 |
| E-06 fixture | 4.5 target fixture、compile→generate→deploy、helper/sysroot/相对路径/none 字段 | 正式配方/生成器/machine；实际图、签名、conf |
| E-07 干净重建 | 4.6 新 TMPDIR、关键配方初始空缓存/无 workspace；首轮中断、同树续跑及告警 | 两轮实际脚本/构建/关键任务日志；保留已有结果，不编造再跑 |
| E-08 cold boot | 4.7 以测试脚本调用真实 runqemu，附加 QMP/GDB/trace，继续执行和真实标记 | runqemu-2 的实际 argv、运行文件身份、GDB、串口/trace |
| E-09 reset | 4.7 同进程两次宿主 reset、恢复前 PC/EL/UART、RAM 哨兵保留再初始化 | 同进程快照/QMP RESET/GDB/ROM 及恢复轮 |
| E-10 负向/恢复 | 4.8 用正式脚本的私有配置演示三项部署诊断，保留 launcher=0 局限，重新恢复主线；模型矩阵有限说明 | 最终 16 项模型、deployment-3；原失败 deployment-2 和 recovery-3 保留 |

关键取舍：

- 沿用 4.1–4.9 显示节号，但按 A-006 重组节义和小节；师徒对话用于提出问题与理解，不虚构三份 patch/commit、内部仓库克隆或未发生的错误演示。完整工程文件在配套 layer，正文展示可定位的关键节选及理由，不把大量治理日志搬入书稿。
- 正式 raw 760 字节、旧独立模型 raw 752 字节分别说明；source-build-id 只关联源/声明 flags，不代替实际 binary 身份。保留真实完整标记，没有把 hash 改短后仍称原文。
- 无调试的 runqemu 命令仅解释入口，明确本次实际验证有 QMP/GDB/trace 和暂停后继续执行；主实验通过已验证脚本控制，输出目录必须是新的目录。宿主 reset 不等于 guest reset，RAM 保留值与启动代码重初始化分别解释。
- reset 教学以源码已外置、备份与身份核对为前提，使用实际验证的 `devtool reset -n`，不教删除持久源码或 `-r`。目录创建/导出重定向用于串联读者操作；只读导出的 patch 字节已核对，不编造这些本地写入动作的运行输出。
- 本章测试 `MACHINE=tiger` 与后续完整 `tiger-aarch64` 分开；没有实现或宣称 SPI NAND、完整启动链、SMP/IRQ/watchdog。保留首次 SPI 读 BL2/FIP 前的 CS/芯片/驱动/镜像门槛，chapter 15 留完整 devtool 教学。

## 3. 自查与待后续核对

作者静态自查通过：正文唯一 H1、九个连续 H2、44 个成对且带语言标记的代码块；三个作者图表按显示宽度对齐、无 Tab；九组 Kconfig/Meson/C/汇编/链接关键节选按固定源逐行有序对照；六组实际输出/配置节选与原始文本子串一致，保留空白/空行。正文无内部 T/A/E/task 编号、内部 Git 地址、旧 recipes-bsp/qemu、宽泛 QEMU append、虚构三 patch 或章末 tag 流程。当前正文没有 Markdown URL/文件超链接，完整工程位置用路径定位；未声称联网验证文献。默认差异空白检查通过，书稿提交仅一文件。

`python -B scripts/validate_framework.py` 返回 0：22 个角色、37 份技能、1 个实际项目、1 套项目模板、580 处本地链接。只读范围核对通过：OPC 只有 TASKS 本人正文阶段、索引本人新增区段及本记录三文件变化；两处共享文件的其他内容与交接起点一致，所有其他被跟踪的设计/配置/决定/专业报告/原文不变。书稿提交/正文 blob 与记录一致、main 未推进、书稿工作树干净。框架校验不验证工程业务结论或替代身份/权限与任务验收。

本轮没有再执行 devtool modify/reset、BitBake、QEMU 或错误注入。新稿是按已有固定工程结果校准的候选版本，静态自查不等于“受评新稿已独立逐句实测”；后续按 D-010/PM 安排对照该精确书稿版本组织专业核对和必要复验。没有新发现的模型/设计冲突，也没有新增模型/配置行为需要本阶段临时故障注入补跑。

后续需要明确的输入/范围：

- 工程 feature 尚未推送/合入。正文明确依赖本章配套工程，未给出虚构的 GitHub 取件命令或假设远端已有 4dd7dfd。正式试读/发布前由 PM 确保读者可取得同版完整文件；本轮在册成员可以从已授权 SSH 固定版本审校，不由 writer 擅自推送工程。
- `--out` 实测目录已经存在；正文要求复现时换未用过的目录，并提醒构建树变化后重新生成配置/身份。不把参数化路径和目录说明当成又一轮已运行结果。按整章顺序复验时，应固定 Poky/meta-tiger、保留持久源码，重新核对配方来源与图/签名，调用正式脚本冷启动/reset→部署负向→恢复，而不是绕过真实 runqemu。
- 跨章只读扫描确认 chapter 5 的暂停/tag 开场、旧 4.6 数据流引用；chapter 6 的三 patch/fork/目录叙事；chapter 7 的 PN 子目录约定；chapter 10 的旧节义引用；chapter 14 的三 QEMU 配方回顾等仍需 T-017 统一。它们未在本轮改写，T-017 未启动。
- uninative 被禁用、跨宿主缓存/跨树搬移未验证；锁定启动器可能在子 QEMU 拒绝后返回 0。正文已在相关操作位置告知，不能从本轮静态交付免除后续独立审校或用户确认。

## 4. 阶段出口

writer 候选稿及本人自查已交付；接收/版本/检查写回 TASKS 本人阶段和本人索引。本轮 OPC 普通提交后停止，交 PM 读取固定版本组织后续审校和用户通读。不开始评审轮、不发送跨会话消息，不修改 A-005/A-006、工程原报告/失败/复核、其他章节、WORKSPACE 或 PM 总体摘要；不合并、推送、删分支或启动其他任务。
