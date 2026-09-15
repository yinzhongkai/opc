# Windows 个人未签名交付验证

- 项目：space-rhythm
- 成果 ID：A-035
- 负责人：release-engineer-windows-01
- 关联任务：T-038
- 版本：0.1
- 更新日期：2026-09-15
- 状态：draft
- 适用范围：依据 D-012/D-013/D-015，在用户本人当前 `TIGER`、`VerifiedAndReputablePolicyState=0` 条件下，验证精确哈希的 Windows x64 个人未签名工程包、正常首次启动、App/Worker smoke、核心工作流及安装/修复/回滚/卸载事务；不验证或宣称 SAC/WDAC、其他 Windows/干净系统、产品效果、真实 VFR、正式 H.264/AAC/容器、公开分发或生产发布。
- 来源及输入版本：[A-033 0.2](A-033-windows-unsigned-deployment-and-transaction-pipeline.md)、[A-034 0.1](A-034-t022-engineering-e2e-fault-recovery-quality-gates.md)、T-022/T-037 completed、D-012/D-013/D-015 confirmed、受控包源提交 `bd9eb97a3b538550790d340b6117362184ee65f2`。
- 批准依据：D-012/D-013 仅确认个人未签名范围及当前主机 SAC-off 条件；D-015 将产品效果评价延期到个人试用反馈。本成果不构成兼容或发布批准。
- 证据：[T-038 验证摘要](../evidence/T-038/verification-summary.md)与[机器可读证据索引](../evidence/T-038/tiger-personal-delivery-20260915.json)。
- 版本记录：2026-09-15，0.1，冻结修复 Worker 重定向 smoke 输出后的个人未签名包，在当前 TIGER 完成交付链验证并保留失败诊断。

## 1. 完成结论

T-038 在授权范围内完成。精确包 `space-rhythm-0.1.0-dev-t038-unsigned.zip` 在当前 `TIGER` 实际通过：

1. bundle/payload 哈希与范围元数据校验；
2. install、拒绝含未登记文件的 repair、repair、rollback、installed validate、uninstall；
3. 无参数、非 offscreen 的正常集成 GUI 首次启动；
4. 安装后 App/Worker smoke 及回滚后 App smoke；
5. 真实导入—分析—编辑—预览—保存—测试导出—音频探测/解码—worker 断连/重连的 Release 核心工作流；
6. 卸载后安装根清除、审计状态收口，显式根外用户数据哨兵保留。

任务结论是 `pass(personal-unsigned,current-TIGER,SAC-off)`，不是签名包或发布候选结论。产品评价固定为：

`productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`

## 2. 冻结交付物

| 项 | 冻结值 |
|---|---|
| 包版本/类型 | `0.1.0-dev-t038` / `unsigned-engineering` |
| 范围/候选资格 | `personal-unsigned` / `candidateEligible=false` |
| 源提交 | `bd9eb97a3b538550790d340b6117362184ee65f2` |
| ZIP | `out/release/T-038/unsigned/space-rhythm-0.1.0-dev-t038-unsigned.zip` |
| 大小 | `44,740,342` 字节 |
| ZIP SHA-256 | `2CC3F8B4AC5A6BC4B3EE8CC42C8305094F372A9E16F9DCAF937B59872FF8C3E6` |
| App SHA-256 | `67D01EBF70B6DA0E12C6AB624658D9E1BC1CC3892F6A5BAA43410178B32560A6` |
| Worker SHA-256 | `D87FC84ECA91B506B3D7ADAB515A60008A3338CEA16E35054DBFE4E670C316DB` |
| 运行闭包/签名观察 | 81 个 PE；78 个 `NotSigned`，3 个 Microsoft 文件 `Valid` |

相同源提交、版本和同一 OutputRoot 连续两次生成的 ZIP 大小与 SHA-256 完全一致。使用不同 OutputRoot 的诊断包因 `build-inputs.json` 如实嵌入不同绝对路径而具有不同归档哈希；逐文件比对确认二进制相同，该跨路径结果不冒充确定性对照。

生成时 `sourceWorktreeClean=false`：清单记录项目经理既有的 `PROJECT.md`、`STATUS.md`、`HANDOFFS.md` 修改，以及未跟踪 `package/`、`scripts/__pycache__/`。这些内容均不是 CMake、部署或运行时输入；由于工作树不干净，包仍强制为非候选。

## 3. 当前 TIGER 与策略边界

| 项 | 观测 |
|---|---|
| 主机/架构 | `TIGER` / `AMD64` |
| 注册表系统信息 | `Windows 10 Home China`，`DisplayVersion=25H2`，build `26200.9457` |
| 验证窗口 | `2026-09-15T12:13:23.3939922Z` 至 `2026-09-15T12:14:45.2316574Z` |
| SAC 状态 | 前 `0`，后 `0` |
| 策略日志 | Code Integrity Operational 与 AppLocker EXE and DLL 均成功查询，过滤匹配 0 条 |

SAC 在验证期间处于关闭状态，所以成功启动和 0 条匹配事件只描述当前主机、当前状态及上述精确哈希。`sacWdacCompatibilityValidated=false`；历史 SAC/WDAC 拒绝证据不被改写，也不从本次结果推导兼容性。

## 4. 启动、smoke 与事务结果

正常首次启动没有参数且未设置 offscreen；进程启动后 3000 ms 仍存活，已观察到 QML root/event loop，随后由测试器有界强制终止，因此 `controlledExitCode=-1` 是测试清理结果而非启动失败。stdout/stderr 均为空。

| 检查 | 结果 |
|---|---|
| 完整包事务 | `pass`：validate/install/App smoke/Worker smoke/拒绝未登记 repair/repair/rollback/validate/uninstall/保留外部数据 |
| App smoke | exit 0，`SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64` |
| Worker smoke | exit 0，`SPACE_RHYTHM_WORKER_SMOKE_OK Qt=6.11.2 arch=x64` |
| 回滚后 App smoke | exit 0，同一 marker |
| 最终安装根 | 不存在 |
| 外部用户数据哨兵 | 存在 |
| 审计状态 | `installed=false`、`lastAction=uninstall`、`backupPath=""` |

## 5. 核心工作流与包关联

本轮 Release 函数 `realImportAnalysisEditPreviewSaveAndExportPath` 为 3 passed、0 failed、0 skipped，覆盖合成视频导入、分析、编辑/锁定/撤销重做、预览/seek、保存、testOnly NUT 导出、导出音频探测/解码和 worker 断连/重连。保存项目重开、故障恢复矩阵以及三套 preset 各 166/166 是 A-034/T-022 的补充证据，不伪装成本轮单函数步骤。

Qt Test 可执行文件依赖的 `Qt6Test.dll` 只来自受控 T-022 Release 测试环境，未加入交付包。安装后的 App/Worker 哈希与当前 Release 构建逐字节相同。相对 T-022 受测提交 `be61c71e9803`，唯一业务源差异为 `src/worker/main.cpp` 中对 smoke stdout 的显式 `fflush`；App 和核心工作流源未变化，重建后的 Worker 已由本轮无重试 smoke 独立验证。

## 6. 保留的失败诊断与修正

1. 首次核心工作流尝试未给外部 Qt Test harness 提供测试专用 `Qt6Test.dll`，退出 `-1073741515`（`0xC0000135`）。没有把测试 DLL 添加到交付包。
2. 第二次尝试强制外部 Qt Test 程序通过安装包布局解析运行时，196 秒内未形成报告并被终止；有界复现退出 `-1073740791`，定位为该非交付测试程序的 `qoffscreen` plugin discovery 冲突。最终改为受控 T-022 Release 测试环境运行核心工作流，包自身另做正常首启和 smoke。
3. 修复前 Worker 在重定向 stdout 时三次均 exit 0 但 stdout/stderr 为空。`src/worker/main.cpp` 增加显式 `fflush(stdout)`，重新构建和冻结包；最终 App/Worker smoke 均单次执行、无重试通过。

这些失败保留在结构化结果中，没有用成功重跑覆盖历史诊断。

## 7. 未覆盖项与责任边界

- `naturalnessEvaluation`、`realVfrEvaluation`、`formalProductPerformanceEvaluation` 均为 `not-evaluated(deferred-to-personal-use-feedback)`。
- 最低 Windows 版本、干净系统/多主机矩阵、正式 H.264/AAC/容器、VC Runtime 分发和真实设备/GPU 兼容未评估。
- 不使用签名凭据，不修改策略或白名单，不宣称 SAC/WDAC 兼容，不批准第三方/公开分发或生产发布。
- 后续个人使用反馈若发现问题，应建立新的缺陷或任务；不能追溯把本证据扩写成产品验收。

T-038 已满足负责人自查完成条件。H-010 的最终关闭仍由发起人 architect-01 核对；项目汇总文件由 project-manager-01 刷新。
