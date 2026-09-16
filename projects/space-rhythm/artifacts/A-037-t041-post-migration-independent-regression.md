# T-041 迁移后独立工程回归与路径隔离证据

- 项目：space-rhythm
- 成果 ID：A-037
- 负责人：tester-cpp-qt-01
- 关联任务：T-041
- 版本：0.1
- 更新日期：2026-09-16
- 状态：draft
- 适用范围：从新 `projects/space-rhythm/workspace/` 工程根独立复验 Windows x64 Debug、CI/RelWithDebInfo、Release 的 CTest/JUnit、Qt/QML、UI/Worker、媒体、存储、取消恢复和路径隔离；不执行发布重建，不形成产品效果、真实 VFR、正式性能或硬件兼容结论。
- 来源及输入版本：D-016 confirmed；T-040 completed；[A-016 0.1](A-016-cpp-qt-test-strategy-and-traceability.md)、[A-017 0.2](A-017-windows-headless-contract-test-entry-and-evidence.md)、[A-034 0.1](A-034-t022-engineering-e2e-fault-recovery-quality-gates.md)、[A-036 0.1](A-036-product-workspace-root-migration.md)；受测提交 `616307cfc50cb448915358bf1d2a5e6836346eba`。
- 批准依据：尚无。本成果是测试执行记录，不是发布批准或产品质量全绿声明。
- 证据：[T-041 验证摘要](../evidence/T-041/verification-summary.md)与[机器可读运行索引](../evidence/T-041/runs-v1.json)。
- 版本记录：2026-09-16，0.1，完成迁移后独立三 preset 回归、路径隔离、诊断保留和未评估边界登记。

## 1. 执行结论

从受测提交创建全新干净 clone，并从其中的 `projects/space-rhythm/workspace/` 调用统一入口。最终结果为：

| preset | CTest/JUnit | skip | 路径隔离 | 判定 |
|---|---:|---:|---|---|
| Windows x64 Debug | 166/166 | 0 | pass | `pass(engineering-scope)` |
| Windows x64 CI/RelWithDebInfo | 166/166 | 0 | pass | `pass(engineering-scope)` |
| Windows x64 Release | 166/166 | 0 | pass | `pass(engineering-scope)` |

测试数量与迁移前 T-022 基线一致。三套最终运行都在配置前、测试后保持跟踪工作树 clean，逐文件证据清单各含 34 项且复算 0 mismatch。

## 2. 覆盖事实

- CTest/JUnit：166 个 `headless` 测试实际执行，包含 108 unit、33 contract、70 golden、23 integration；标签重叠，不把标签数相加作为测试总数。
- Qt/QML：Qt Test、Qt Quick Test、QML shell/high-DPI、App smoke、offscreen/software/default GPU 及 9 个独立真实 UI 工作流场景实际运行。
- UI/Worker：版本化 IPC、out-of-band 引用、后台导入、取消确认、取消后重连、断连恢复、事务编辑、保存重开和失败重试实际运行。
- 媒体：PTS/CFR/VFR/负时间、旋转/SAR、颜色范围、多流、动态格式、seek、损坏/缺流、PCM provenance 和资源边界实际运行。
- 存储：schema 迁移、原子保存、autosave 恢复、素材重定位、缓存损坏与配额实际运行。
- 取消/恢复：媒体/音视频/渲染/导出取消，worker 崩溃或断连、磁盘/编码失败、并发输出及 autosave 损坏回退实际运行。

工程测量仍没有已批准的正式阈值。标签中的局部 `pass` 只能证明公开工程 oracle，不替代产品效果、自然度、真实素材或硬件矩阵。

## 3. 新 workspace 与路径隔离

物理 clone 位于主工程被忽略的 `workspace/out/`，运行时通过短 `R:` 映射规避 MSVC 对深生成路径的限制。三套路径审计均确认：

- Git 相对工程根为 `projects/space-rhythm/workspace`；
- CMake home、build、固定 vcpkg installed、证据和 TEMP/TMP 均属于新 workspace；
- 独立 clone 的仓库根不存在旧产品入口和根 `out/`；
- 最终运行没有向仓库根旧工程位置写入 fixture、生成清单或证据；
- `package/`、`scripts/__pycache__/` 与 T-042 均未触碰。

## 4. 可复现输入与黄金样例

随机 seed 固定为 `0x5350414345524859`；契约状态机使用手动时钟，Qt 事件循环使用有界真实时钟；QPA=`offscreen`、RHI=`software`。环境记录绑定 Qt 6.11.2、MSVC 19.44、Windows SDK 10.0.26100.0、CMake 3.31.6、Ninja 1.12.1、固定 vcpkg baseline 与 FFmpeg 8.1.2#3。

13 个 media、10 个 video、10 个 audio 样例均为项目合成/固定字节来源并声明 CC0-1.0。三类 fixtures 与 generated 清单哈希已写入机器索引；最终运行前后保持一致。干净 clone 缺失的被忽略开发音色由既有确定性生成器在构建前生成，生成后必须保持跟踪工作树 clean。

## 5. 诊断记录与可信边界

本次没有删除失败记录：

1. 干净 clone 首次构建暴露开发音色生成物未准备，测试入口已补前置生成。
2. 深 checkout 首次触发 MSVC C1083，改用短映射路径后构建通过。
3. 首个短路径运行 166/166，但路径审计没有正确解析 `subst`，测试基础设施修正为读取 Git prefix。
4. 受测提交的首轮 Debug 为 165/166：取消→重连用例约 113 秒后观察到 `workspaceState=failed`，期望 `idle`。未修改 oracle、测试或产品实现的后续独立 Debug 为 166/166，该用例 3.30 秒通过。首次失败按未复现瞬态保留，不能因终态通过而回写成从未发生。

测试基础设施修订提交为 `d198c09`、`e8448a3`、`616307c`；没有修改 `src/`、既有测试 oracle 或被测媒体实现。

## 6. 延期项与交接

以下结论保持不变：

- `productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `naturalnessEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `realVfrEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `formalProductPerformanceEvaluation=not-evaluated(deferred-to-personal-use-feedback)`

T-041 已完成“执行并如实记录”的范围。T-042 未启动；后续负责人可以本成果和三套最终原始证据作为新工程根发布重建的测试输入，但不得把本成果扩写为产品或发布全绿。
