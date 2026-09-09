# T-015 可复核验证摘要

- 项目：space-rhythm
- 任务：T-015
- 负责人：core-systems-engineer-cpp-01
- 验证日期：2026-09-09
- 输入基线：A-012 0.1、A-013 0.1，D-003 confirmed
- 实现目标：`SpaceRhythm::Core` 纯 C++20 静态库；无 Qt Quick、媒体、CV 或 DSP 依赖

## 1. 实现范围

- 实现 `TimeNs` 精确比例换算、四种显式舍入、checked 加减和半开时间范围；MSVC x64 使用 192 位无符号中间量，最终结果按 `Int64` 范围判溢出。
- 实现强类型 ID、轨道、事件、分析候选、来源、不可变已提交快照和规范排序 `(timeNs, track.orderIndex, EventId ASCII bytes)`。
- 实现增删/重排/重命名轨道，增删/移动/修补/锁定事件，批量偏移和候选融合操作；事务按单锁全序执行，使用基准修订做乐观冲突检查，在临时快照完整校验后一次发布。
- 实现事务 ID 幂等重放、不同请求复用冲突、提交前取消、无变化提交、修订耗尽、结构化错误和逻辑 DTO/schema 门禁。
- 实现有界 1,024 条的 undo/redo 历史、分支编辑清空 redo、同 key/同实体用户操作合并、保存点语义比较和重新打开后空历史。
- 实现 `deterministic-fusion-v1`：候选规范化、版本/参数/种子参与确定性排序，保护人工、用户编辑和锁定事件，执行最小间隔与固定密度窗限制，稳定生成事件 ID，并为每个抑制候选输出唯一主原因。
- 同一 `AnalysisRevision` 在已打开会话内绑定规范候选及融合参数摘要；不等价复用返回 `analysis_revision_conflict`。

## 2. A-012 向量接入

`tests/unit/core_contract_vectors_test.cpp` 按 A-012 第 10 节分成六个 GoogleTest 组，组内每个断言使用原始向量 ID 作为 `SCOPED_TRACE`：

| 向量组 | 数量 | GoogleTest |
|---|---:|---|
| `TV-TIME-*` | 15 | `A012TimeVectors.TV_TIME_001_015` |
| `TV-EVENT-*` | 14 | `A012EventVectors.TV_EVENT_001_014` |
| `TV-TXN-*` | 12 | `A012TransactionVectors.TV_TXN_001_012` |
| `TV-MERGE-*` | 7 | `A012MergeVectors.TV_MERGE_001_007` |
| `TV-HISTORY-*` | 12 | `A012HistoryVectors.TV_HISTORY_001_012` |
| `TV-DTO-*` | 12 | `A012DtoVectors.TV_DTO_001_012` |
| 合计 | 72 | 六组全部进入 CTest |

以下命令对 A-012 与测试源码中的向量 ID 去重后比较，结果为 `CONTRACT=72 TESTS=72` 且 `Compare-Object` 无差异：

```powershell
$contract = rg -o 'TV-(TIME|EVENT|TXN|MERGE|HISTORY|DTO)-[0-9]{3}' projects/space-rhythm/artifacts/A-012-core-domain-contract-0x.md | Sort-Object -Unique
$tests = rg -o 'TV-(TIME|EVENT|TXN|MERGE|HISTORY|DTO)-[0-9]{3}' tests/unit/core_contract_vectors_test.cpp | Sort-Object -Unique
Compare-Object $contract $tests
```

另有三项开发自测：两个线程以同一基准修订竞争时仅一个提交、轨道生命周期与旧快照不可变、重复/最小间隔/密度抑制和旧分析事件替换。

## 3. Windows x64 构建与 CTest

实际工具链为 MSVC 19.44.35228.0、CMake 3.31.6-msvc6、Ninja 1.12.1、Qt 6.11.2 shared、动态 CRT，以及固定 vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`。三套目标均以 `/W4 /WX /permissive- /std:c++20` 编译。

| preset | 最新源码构建 | CTest | 结果 |
|---|---:|---:|---|
| `windows-msvc-x64-debug` | 干净重配置后 71/71 | 16/16 | 通过 |
| `windows-msvc-x64-release` | 12/12 增量重编译 | 16/16 | 通过 |
| `ci-windows-msvc-x64` | 12/12 增量重编译 | 16/16 | 通过 |

最终命令入口：

```powershell
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Configure -UseExistingDependencies -EvidenceRoot ./out/evidence/T-015/windows-msvc-x64-debug-clean
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Build -EvidenceRoot ./out/evidence/T-015/windows-msvc-x64-debug-clean
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Test -AllowWdacFallback -EvidenceRoot ./out/evidence/T-015/windows-msvc-x64-debug-clean
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-release -Stage Build -EvidenceRoot ./out/evidence/T-015/windows-msvc-x64-release
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-release -Stage Test -AllowWdacFallback -EvidenceRoot ./out/evidence/T-015/windows-msvc-x64-release
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset ci-windows-msvc-x64 -Stage Build -EvidenceRoot ./out/evidence/T-015/ci-windows-msvc-x64
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset ci-windows-msvc-x64 -Stage Test -AllowWdacFallback -EvidenceRoot ./out/evidence/T-015/ci-windows-msvc-x64
```

最终原生日志位于：

- Debug build：`out/evidence/T-015/windows-msvc-x64-debug-clean/native-20260909-193304.log`
- Debug CTest：`out/evidence/T-015/windows-msvc-x64-debug-clean/native-20260909-193334.log`
- Release build：`out/evidence/T-015/windows-msvc-x64-release/native-20260909-193037.log`
- Release CTest：`out/evidence/T-015/windows-msvc-x64-release/native-20260909-193126.log`
- CI build：`out/evidence/T-015/ci-windows-msvc-x64/native-20260909-193054.log`
- CI CTest：`out/evidence/T-015/ci-windows-msvc-x64/native-20260909-193200.log`

本机在新生成可执行文件首次运行时数次返回 A-013 已登记的 WDAC “应用程序控制策略已阻止此文件”或无输出超时。最终 Debug 使用干净重配置/重编译后，三套 GoogleTest 均由最新二进制实际运行；三套 CTest 均为 16/16。`-AllowWdacFallback` 只允许既有 Qt 冒烟脚本在已识别策略拒绝时使用固定 Qt SDK 工具，不会替代、跳过或吞掉 GoogleTest 失败。

## 4. 自查结论

- A-012 的 72 个唯一向量 ID 全部存在于测试源码，没有遗漏或额外伪造编号。
- 事务失败、取消、过期修订、锁定冲突和分析版本冲突均保持当前快照及历史不变；批量偏移和整次融合可由一次 undo 恢复。
- 相同候选事实、算法版本、参数和种子在不同输入排列下产生语义等价且规范排序一致的事件集合。
- 公共头文件和核心实现没有 Qt/QML、FFmpeg、OpenCV、DSP 或媒体 include；现有 QML、媒体、CV、DSP 与 `package/` 未修改。
- 框架自测 37/37 通过；只读配置校验通过 17 个岗位、24 份知识、1 个实际项目、1 套模板和 435 处本地链接。
- T-016 未启动；本任务没有实现 IPC、worker 作业协议或持久化。
