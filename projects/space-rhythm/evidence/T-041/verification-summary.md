# T-041 新 workspace 独立回归验证摘要

- 执行日期：2026-09-16
- 执行成员：`tester-cpp-qt-01`
- 受测提交：`616307cfc50cb448915358bf1d2a5e6836346eba`
- 工程根：`projects/space-rhythm/workspace/`
- 成果：[A-037 0.1](../../artifacts/A-037-t041-post-migration-independent-regression.md)
- 机器可读索引：[runs-v1.json](runs-v1.json)
- 原始证据：被忽略的 `projects/space-rhythm/workspace/out/evidence/T-041/<runId>/`

## 结论

从受测提交建立全新本地 clone，在干净跟踪工作树中从迁移后的工程根执行统一入口。最终 Debug、CI/RelWithDebInfo、Release 三套 preset 均完成 166/166、0 fail、0 skip；CTest 发现、JUnit、Qt Test/Qt Quick Test 文本报告、构建/运行日志、四类结构化测量、环境输入哈希和逐文件 SHA-256 均已归档。三套最终运行的 34 项证据哈希逐项复算均为 0 mismatch。

本结论只表示最终工程 oracle 通过。首轮 Debug 曾出现 1 项取消→重连场景超时后状态为 `failed`，其原始 JUnit 和日志完整保留；随后在没有修改 oracle、测试或被测业务实现的独立 Debug 运行中 3.30 秒通过。该现象记录为“未复现的瞬态观察”，不从历史中删除，也不外推为稳定性问题已经证明消失。

## 三套最终运行

统一命令均从 `R:/projects/space-rhythm/workspace` 执行；`R:` 只把物理 clone 映射为短路径，以避免 MSVC 生成对象路径超限。

```powershell
./tooling/windows/Invoke-HeadlessTests.ps1 -Task T-041 -Preset windows-msvc-x64-debug -Clean -Parallel 4
./tooling/windows/Invoke-HeadlessTests.ps1 -Task T-041 -Preset ci-windows-msvc-x64 -Clean -Parallel 4
./tooling/windows/Invoke-HeadlessTests.ps1 -Task T-041 -Preset windows-msvc-x64-release -Clean -Parallel 4
```

| preset | run ID | JUnit | 路径审计 | JUnit SHA-256 |
|---|---|---:|---|---|
| Debug | `20260916T033437Z-616307cfc50c-windows-msvc-x64-debug-01` | 166/166，0 skip | pass | `508d718c41a4d79aed92cd39cad7314a50e5b7eba07d5ac3fd5d4670a8e53472` |
| CI/RelWithDebInfo | `20260916T032958Z-616307cfc50c-ci-windows-msvc-x64-01` | 166/166，0 skip | pass | `675aadfe57873ab32100b942f0780bfe574ef062573008d6caac6f7724a199a1` |
| Release | `20260916T033222Z-616307cfc50c-windows-msvc-x64-release-01` | 166/166，0 skip | pass | `2fd1a549475d1a985537e6088c9be05bb3f9a3aca4265bb5cfbaf8c7077d6baf` |

测试总数与迁移前 T-022 基线一致，均为 166；没有通过删减、skip 或更换标签获得终态结果。

## CTest/JUnit 与覆盖核对

最终发现清单包含 166 个 `headless` 测试，标签计数为：108 `unit`、33 `contract`、70 `golden`、23 `integration`、16 `qt`、4 `qml`、9 `ui`、1 `worker`、47 `media`、43 `audio`、10 `video`。G0～G4 标签分别覆盖 34、141、32、121、53 项；标签会重叠，不相加为测试总数。

- Qt/QML：`qt.core_smoke`、`qml.quick_smoke`、`qml.ui_shell`、`qml.ui_shell_high_dpi`、`app.qml_smoke` 以及 9 个 `qt.ui_real_workflow.*` 场景均实际执行。
- UI/Worker：真实 UI/worker 双进程、版本化 out-of-band 结果、后台导入、取消终态确认、取消后重连、worker 断连恢复、事务编辑、保存重开与大项目失败重试均执行。
- 媒体：CFR/VFR/负 PTS、旋转/SAR、`limited/full/unknown` 颜色范围、多流、动态格式、seek、损坏/缺流、PCM segment/lease 和资源边界均执行。
- 存储：schema 迁移、future schema 拒绝、精确整数往返、原子保存、autosave 恢复、素材重定位、缓存损坏与配额淘汰均执行。
- 取消/恢复：媒体、音频、视频、渲染和导出取消，worker 崩溃/断连、磁盘/编码失败、并发输出、autosave 损坏回退均执行。

## 黄金样例与可复现规则

固定 seed 为 `0x5350414345524859`；契约状态机使用手动时钟，Qt 事件循环使用有界真实时钟；QPA 为 `offscreen`，RHI 为 `software`；每个运行的 `TEMP/TMP` 指向其证据目录下独立 `work/`。

| 样例 | 数量 | 来源/许可证 | fixtures SHA-256 | generated SHA-256 |
|---|---:|---|---|---|
| media | 13 | FFmpeg lavfi、数字静音或项目固定字节；CC0-1.0 | `ff30af091a1200baeff3871cd3df15b085609507eab14eecd021ed0c71301cd3` | `aaac8ca1582732898821fffb734dd9b9e569240d21022e87df27ece01310797c` |
| video | 10 | 项目确定性合成帧；CC0-1.0 | `46cedfeebc0158c9baf090103cbeb13ddd7724520e040b51c752c85fa1991073` | `9e5a2dd8953873c8522c361bada4b23567cf5615e95df4cc9ef908ef359f2354` |
| audio | 10 | 项目确定性 PCM；CC0-1.0 | `080a83d150727f4a1db4b66da1b6aae37f741f2e0cc61f08fba916ed30ec9bd4` | `552604abe80a95adab839b9b0c959ea44464cb785e28d007d291777cd6c8e40f` |

干净 clone 首次暴露被忽略的开发音色 PCM 没有构建规则。测试入口现于构建前调用既有确定性生成器，再检查跟踪工作树仍为 clean；媒体 ffprobe 记录中的样例路径也标准化为清单内 `outputFile`。这些变化只修复测试准备和证据可迁移性，没有修改业务实现或已有 oracle。

## 路径隔离

三套最终 `path-isolation.json` 均确认：

- Git 相对工程根为 `projects/space-rhythm/workspace`；CMake home 与该工程根一致。
- build、vcpkg installed、证据与临时目录均位于新 workspace 的 `out/` 下。
- 独立 clone 的仓库根没有旧产品 `out/`，也没有旧根产品入口。
- 配置前和测试后跟踪工作树均 clean；最终 clone `git status --short --untracked-files=no` 无输出。
- 根 `package/` 与 `scripts/__pycache__/` 不属于输入、输出或提交范围。

## 保留的诊断运行

| run ID | 事实 | 处理 |
|---|---|---|
| `20260916T031217Z-d198c09221f6-windows-msvc-x64-debug-01` | 干净 clone 缺少被忽略的开发音色生成物，构建 blocked | 在测试入口构建前生成；原始 blocked 证据保留 |
| `20260916T031726Z-e8448a3b4571-windows-msvc-x64-debug-01` | 深 checkout 路径触发 MSVC C1083 | 使用短 `subst` 执行路径；物理 clone 仍在 workspace/out；原始证据保留 |
| `20260916T031910Z-e8448a3b4571-windows-msvc-x64-debug-01` | 166/166 通过，但 `subst` 与物理路径比较使路径审计 blocked | 由 Git prefix 解析相对工程根；该轮不计作终态 |
| `20260916T032410Z-616307cfc50c-windows-msvc-x64-debug-01` | 165/166；`cancelledWorkerJobDoesNotPoisonPreviewAfterReconnect` 约 113 秒后实际 `failed`，期望 `idle` | 未改 oracle/测试/产品；后续干净 Debug 166/166，原始失败继续保留 |

## 判定边界与停止点

5 个带 `measured`/`not-evaluated` 标签的工程测量均已执行；无已批准门槛的性能数值只记为 `measured(no-approved-threshold)`。以下四项保持：

- `productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `naturalnessEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `realVfrEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `formalProductPerformanceEvaluation=not-evaluated(deferred-to-personal-use-feedback)`

因此 T-041 完成的是迁移后工程复验，不是“产品质量全绿”、真实 Windows/硬件兼容矩阵或发布批准。T-042 未启动；本任务只把已完成的测试结果交给其负责人作为后续输入。
