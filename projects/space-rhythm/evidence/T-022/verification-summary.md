# T-022 工程端到端、故障恢复与质量门禁验证摘要

- 证据版本：1
- 执行日期：2026-09-15
- 执行人：tester-cpp-qt-01
- 最终受测提交：`be61c71e9803`
- 原始证据：`out/evidence/T-022/<run-id>/`（按 A-016 忽略，不纳入 Git）
- 机器可读索引：[runs-v1.json](runs-v1.json)

## 最终结果

| preset | CTest | fail | skip | 结论 |
|---|---:|---:|---:|---|
| `windows-msvc-x64-debug` | 166/166 | 0 | 0 | 工程范围 `pass` |
| `ci-windows-msvc-x64`（RelWithDebInfo） | 166/166 | 0 | 0 | 工程范围 `pass` |
| `windows-msvc-x64-release` | 166/166 | 0 | 0 | 当前个人 SAC-off 主机工程范围 `pass` |

入口以 `-Clean -Parallel 4` 从干净跟踪工作树运行。每次归档环境、发现清单、标签、CTest 日志、JUnit、构建日志、四份结构化测量和逐文件 SHA-256；固定 seed 为 `0x5350414345524859`，Qt 使用 offscreen，软件渲染，TEMP/TMP 独立。9 个真实 Qt UI/worker 场景分进程串行执行，各自产生 Qt Test 报告，避免用例间进程状态污染。

## 工程端到端与故障恢复

完整路径实际覆盖导入、分析、编辑/锁定、试听、保存/重开、测试格式导出与 worker 重连。公开契约还覆盖取消确认、worker 崩溃/断连、损坏媒体、不可打开媒体的 `unsupported_media`、缺流、损坏缓存、素材丢失后的指纹重定位、磁盘不足、编码失败、异常恢复和并发输出冲突。所有失败路径均验证不覆盖最近成功项目或已有导出目标；并发争用者不删除持有者临时文件。

预览/导出合成时序最大误差 `33,000,000 ns`，一帧上限 `33,333,334 ns`；该工程 oracle 为 pass。seek、A/V 选择差、恢复延迟、分析/渲染吞吐和峰值内存均只登记 `measured`，数值及哈希见 `runs-v1.json`，没有已批准阈值，不能写成性能通过。

## 样例与历史诊断

媒体 13、视频 10、音频 10 个清单项均为项目合成/项目自有确定性内容，许可证 `CC0-1.0`；生成配方、输入/输出 SHA-256、期望字段和精确/版本化容差由各 manifest/oracle 固定。它们不是用户真实 VFR 或产品主观样本。

前置失败运行保留在原始证据目录和 `runs-v1.json`：包括测量 harness 缺陷、未公开 message key 的过度断言、Windows GUI Qt Test 空 stdout，以及单进程多场景的 worker 生命周期污染。最终方案只改测试入口/调度和公开契约测试，没有修改业务实现或既有媒体 oracle；没有用静默重试替代失败记录。

## 判定边界

- `productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `naturalnessEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `realVfrEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `formalProductPerformanceEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- 历史 `T021-ENV-001` 在 T-021 记录中继续为 `blocked`；本轮严格 Release 通过是 D-015/当前个人 SAC-off 条件下的新 T-022 证据，不回写旧结果，也不是 SAC/WDAC 兼容或正式发布证明。
- `package/` 与 `scripts/__pycache__/` 未参与输入或提交；未执行 T-038。
