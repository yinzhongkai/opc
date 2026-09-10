# C++/Qt 测试策略、需求追踪与可复现规则

- 项目：space-rhythm
- 成果 ID：A-016
- 负责人：tester-cpp-qt-01
- 关联任务：T-020
- 版本：0.1
- 更新日期：2026-09-10
- 状态：draft
- 适用范围：第一阶段 Windows x64 独立桌面应用的测试分层、需求与门槛追踪、可复现执行、证据状态和独立验证边界；本版本是 T-021/T-022 的执行计划，不执行测试、不修改被测实现、不批准产品效果、性能、硬件兼容或发布。
- 来源及输入版本：[A-004 0.5](A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.4](A-005-mvp-technology-stack-proposal.md)、[A-006 0.1](A-006-domain-work-packages.md)、[A-007 0.1](A-007-four-engineer-execution-plan.md)、[A-012 0.1](A-012-core-domain-contract-0x.md)、[A-014 0.2](A-014-media-time-buffer-and-golden-contract.md)、[A-015 0.1](A-015-ffmpeg-media-pipeline.md)；D-001～D-008 confirmed；T-013～T-018 completed；H-005；用户于 2026-09-10 明确要求接收 H-005、启动并完成 T-020。
- 批准依据：尚无。T-020 只要求负责人完成覆盖与可执行性自查，无独立评审要求；本计划及后续测试结果不替代产品验收或发布批准。
- 计划版本：`testPlanVersion = 0.1.0`
- 版本记录：2026-09-10，0.1，首次建立 TR-A-001～006、TR-F-001～018、TR-Q-001～010 与 G0～G4 的逐项追踪，定义八层测试体系、可复现规则、证据状态和 T-021/T-022 执行边界。

## 1. 结论与当前证据边界

第一阶段采用八层测试体系：单元、契约、媒体黄金样例、集成、Qt/QML、性能、故障注入和兼容性。统一入口沿用 D-003 已确认的 GoogleTest + CTest + Qt Test/Qt Quick Test，Windows x64 工具链沿用 T-013 的固定 preset；测试层不建立第二套时间、事件、媒体或错误契约。

T-013～T-018 的开发自测证明工程底座、核心、系统和媒体实现已有可消费证据，但这些证据由各实现负责人产生。本计划把它们登记为 `developer-evidence-available`，不冒充 tester-cpp-qt-01 的独立执行结论。T-021/T-022 未启动，因此本文件所有独立测试项当前均为 `not-evaluated`。

当前可直接消费的基线如下：

| 基线 | 固定版本与已有证据 | 本计划中的用途 |
|---|---|---|
| T-013 / A-013 0.1 | CMake 3.31、Ninja、MSVC 19.44、Qt 6.11.2 shared、Windows x64、Debug/Release/CI presets、CTest/GoogleTest/Qt Test/Qt Quick Test 骨架 | T-021 统一入口、标签、headless 和环境清单起点 |
| A-012 0.1 | `coreContractVersion=0.1.0`、`schemaVersion=1`、`vectorSetVersion=1`，72 个稳定 `TV-*` 向量 | 契约独立断言必须保留全部原始 ID 与预期 |
| T-015 | 72/72 向量已由实现方接入 GoogleTest，并有事务、并发与融合开发自测 | T-021 复核测试发现、向量映射和独立证据，不降低预期 |
| T-016 | 作业、IPC、schema 迁移、原子保存、恢复、素材重定位和缓存故障开发自测 | T-021 契约层与 T-022 故障/集成层输入 |
| A-014 0.2 | `mediaContractVersion=0.1.0`、`schemaVersion=1`、`vectorSetVersion=1`、`goldenManifestVersion=1`，12 个 CC0 样例和 30 个时间向量 | 媒体黄金样例、时间、流、seek、格式 epoch、所有权和背压 oracle |
| A-015 0.1 / T-018 | FFmpeg 8.1.2#3、固定 vcpkg baseline、12 个样例实际哈希/ffprobe 证据、16 项媒体开发自测 | T-021 独立复现媒体基线；T-022 接入真实媒体链路 |

已有运行次数、耗时和 20 秒合成素材的 1024-byte 单帧工作缓冲只作为既有观测。它们没有得到产品侧性能阈值、基准硬件矩阵或正式样例规模授权，不能升级为性能 `pass`。

## 2. 状态与门禁判定

每个测试项和门槛必须使用以下状态之一，不允许用“看起来正常”“基本通过”替代：

| 状态 | 判定规则 |
|---|---|
| `pass` | 已在声明版本和环境实际执行；输入、oracle 与阈值均已确认；所有强制断言满足；日志和环境清单可复核。 |
| `fail` | 已执行且实际结果违反已确认的功能契约、阈值或安全不变量；必须保存最小复现、实际值和诊断。 |
| `blocked` | 测试本应执行，但被缺失构建、权限、设备、依赖或可操作外部条件阻断；记录解除条件和下一行动人。 |
| `not-run` | 已列入当前执行批次但尚未启动；不得汇总成通过。 |
| `measured` | 在完整记录的环境中取得数值或主观评分，但没有已确认阈值，或环境不属于已确认基准矩阵；只报告观测与分布。 |
| `not-evaluated` | 缺少实现、输入、确认阈值、目标硬件/Windows 版本，或任务尚未进入执行阶段，无法形成合格/不合格结论。 |

执行型测试可把“功能不变量”与“性能/效果指标”拆成两个结论。例如流式解码不越过显式内存上限可按已确认上限判 `pass/fail`；同一次运行测得的解码耗时在产品预算未确认时只能是 `measured`，对应性能门槛仍为 `not-evaluated`。

门槛只能在以下条件同时满足时标为 `pass`：范围内全部强制测试均为 `pass`、没有 `fail/blocked/not-run/not-evaluated`、所需阈值已由有权来源确认、环境属于已承诺矩阵、证据包完整。`measured` 不能参与“通过率”分母，也不能被绿色展示替代 `not-evaluated`。

## 3. 测试分层

| 层级 ID | 层级 | 主要范围与 oracle | 运行与证据要求 |
|---|---|---|---|
| `UT` | 单元测试 | 纯函数、边界值、状态转换、排序、舍入、缓存键、算法局部不变量；oracle 来自已确认契约或稳定向量 | GoogleTest；无 GUI、无真实网络、默认无真实时钟；每次提交与 CI 必跑 |
| `CT` | 契约测试 | A-012 的 72 个 `TV-*`、A-014 的 30 个时间向量、C-01～C-08、错误码、schema/协议/版本兼容 | 每个向量保留原 ID、契约版本、输入、预期与实际；消费者与生产者两侧都可执行 |
| `GM` | 媒体黄金样例 | CFR/VFR、负起点、旋转/SAR/颜色、多流、44.1/48 kHz、损坏/缺流、长素材、动态格式；后续扩展事件、音频峰值和渲染语义黄金值 | 先验证清单、许可证、配方和 hash，再执行；实际二进制不从 `package/` 取得 |
| `IT` | 集成测试 | core/system/media/worker/storage/Qt bridge 的真实边界，进程、文件和固定媒体可参与；不以 mock 替代被测集成点 | 唯一临时沙箱、确定性超时、进程树清理、结构化日志；T-021 覆盖契约集成，T-022 覆盖端到端 |
| `QQ` | Qt/QML 测试 | Qt/C++ 属性、模型角色、信号顺序、线程亲和性、QML 绑定/交互、快捷键、可访问错误、offscreen 与受控有界面路径 | Qt Test/Qt Quick Test；记录 QPA、RHI/GPU、缩放、字体、locale；不以无条件 sleep 等待信号 |
| `PF` | 性能测试 | 延迟、吞吐、峰值内存、缓存/磁盘、seek、音画漂移、UI 响应、分析与导出耗时 | 先过功能正确性；记录预热、重复、原始样本、分布和环境签名；无阈值只记 `measured` |
| `FI` | 故障注入 | 取消、超时、worker 崩溃、消息乱序/重复、损坏媒体/缓存、素材丢失、磁盘不足、部分写入、权限拒绝、并发输出冲突 | 使用显式注入点或隔离资源，不填满主机磁盘、不改系统配置；验证最近成功保存和既有输出不被破坏 |
| `CO` | 兼容性测试 | schema/协议/契约版本、Debug/Release/CI ABI、Windows 版本、GPU/RHI、DPI/字体、音频设备、编码器和安装/升级 | 只对已承诺矩阵给 pass；未承诺 Windows/GPU/设备组合列为 `not-evaluated`，不得以本机单点代表矩阵 |

开发成员负责其模块的白盒自测、注入点和诊断；tester-cpp-qt-01 负责跨模块 oracle、追踪清单、独立执行及证据判定。测试失败由原实现负责人修复，测试成员不静默修改被测实现。

## 4. 目录、命名、标签与统一入口

现有 `tests/unit`、`tests/qt`、`tests/qml`、`tests/golden/media` 保留。T-021/T-022 按实际实现增设 `tests/contract`、`tests/integration`、`tests/performance`、`tests/fault`、`tests/compatibility`；本任务不提前创建目录或代码。

测试显示名采用 `<layer>.<domain>.<behavior>`，例如 `contract.core.TV-TIME-007`、`golden.media.GM-VFR-001`、`fault.store.disk_full_preserves_primary`。每个测试元数据至少记录：测试 ID、关联 TR/G、被测目标、契约/清单版本、fixture ID、预期类别、负责人和默认超时。测试代码引用 A-012/A-014 向量时必须在失败输出中打印原始向量 ID。

CTest 标签至少使用：`unit`、`contract`、`golden`、`integration`、`qt`、`qml`、`performance`、`fault`、`compatibility`、`smoke`、`gate-g0`～`gate-g4`。需求追踪以本文件矩阵和测试清单为事实源，不依赖从测试名猜测。

统一入口继续使用 `tooling/windows/Invoke-ProjectBuild.ps1` 和 `ctest --preset <preset>`。T-021 应提供以下选择语义：

- 默认提交门禁：`UT + CT + GM(manifest/短样例) + Qt/QML headless smoke`。
- 夜间/受控环境：全部 GM、IT、FI、CO，以及不带产品通过结论的 PF 测量。
- 发布候选：按已确认 Windows/硬件/格式矩阵执行 G0～G4 所需集合；缺少矩阵项时门槛为 `not-evaluated`。
- `--output-on-failure`、JUnit/XML 与原始 stdout/stderr 同时保存；零测试、发现失败、测试进程崩溃或超时均视为失败，不得返回成功。

## 5. 可复现执行规则

### 5.1 随机种子

- 规范回归默认基础种子为无符号 64 位 `0x5350414345524859`；测试报告同时保存十六进制和十进制形式。
- 使用随机排列、属性数据或渲染随机数的测试必须记录 PRNG 名称/版本、基础种子、从 suite/test ID 派生子种子的算法和实际子种子；不得使用 `std::random_device`、进程时间、PID、容器地址或未版本化 `std::hash` 作为不可复现输入。
- 探索运行可以显式指定其他种子，但失败必须打印可直接重放的完整命令和种子。重试不得更换种子后把首轮失败写成通过。
- 产品算法和渲染若声明确定性，素材指纹、算法版本、参数摘要、随机种子、`analysisRevision`/`timelineRevision` 必须一起进入结果身份和证据清单。

### 5.2 时钟、区域与等待

- UT/CT/FI 使用可注入手动单调时钟；状态机通过推进虚拟时间触发 deadline、取消和重试，不用无条件 `Sleep` 掩盖竞态。
- IT/QQ 的真实事件循环等待使用信号、条件或有界轮询，并记录超时值和最后状态。墙上时间只用于证据时间戳，不参与功能 oracle。
- 序列化时间使用 UTC 和明确单位；每次运行记录 Windows 时区、locale、Qt locale、代码页。规范回归固定为 UTC、稳定 locale 和项目固定字体；时区/DST、本地化及高 DPI 另在 CO 层显式参数化。

### 5.3 临时目录和进程隔离

- 每次运行使用 `out/test-work/<run-id>/<test-id>/<attempt>/` 下的唯一绝对目录；测试不得写入源码树、用户主目录、`package/` 或其他测试的沙箱。
- 输入只读挂载或复制，输出、缓存、项目、autosave、IPC endpoint 和临时导出均位于当前测试沙箱。路径测试使用专门构造的数据，不复用用户真实目录。
- `pass` 可清理工作目录；`fail/blocked` 默认保留并登记 hash。进程退出时必须有界终止本次创建的 worker 子进程并清理 endpoint，不杀死无关进程。
- 磁盘不足、部分写入和权限拒绝使用注入式文件系统或受控小配额卷；禁止通过填满系统盘或修改全局 ACL 制造故障。

### 5.4 日志与环境归档

每次运行生成唯一 `run-id = <UTC时间>-<短提交>-<preset>-<序号>`。原始证据保存在被忽略的 `out/evidence/T-021|T-022/<run-id>/`，可提交摘要保存在 `projects/space-rhythm/evidence/T-021|T-022/`。环境清单至少包含：

- Git commit、工作树是否干净及相关输入文件 SHA-256；不得把未跟踪 `package/` 或 `scripts/__pycache__/` 纳入输入/提交。
- Windows edition/build、CPU、内存、磁盘、GPU/驱动、显示缩放、字体、音频设备、电源策略和虚拟化事实；缺失字段显式为 unknown。
- preset/build type、MSVC/Windows SDK/CMake/Ninja/Qt/GoogleTest/FFmpeg/vcpkg baseline 与已解析 ABI/feature；Debug/Release CRT。
- 时区、locale、QPA 平台、RHI/图形后端、PATH 中实际解析的运行库、测试 seed/clock 模式、fixture manifest/hash 和 capability 探测结果。
- 每个测试的开始/结束 UTC、单调耗时、状态、退出码、诊断 ID、首次失败及每次重试；日志中的绝对用户路径按稳定 token 脱敏，媒体内容和 PCM/帧字节默认不进入日志。

已知 WDAC 回退只能按 T-013 已声明的显式参数使用并单独标注。它不得替代 GoogleTest、媒体实现或产品二进制的独立执行；任何回退覆盖到被测行为时，该行为为 `not-evaluated` 或 `blocked`，不能为 `pass`。

### 5.5 跳过与重试

- 只有运行前可判定且已登记的能力条件、未进入当前阶段、缺少已确认产品输入，或明确不属于当前承诺矩阵的组合可以跳过。每次跳过必须记录测试/需求/门槛 ID、原因码、探测证据、解除条件、下一行动人和失效日期或适用版本。
- 跳过原因码限定为 `missing_capability`、`unsupported_host`、`missing_confirmed_input`、`not_in_stage`、`outside_committed_matrix` 或 `known_environment_policy`；自由文本只能补充上下文，不能取代原因码。
- 预期必跑测试的编译失败、发现失败、fixture/hash 不符、进程崩溃、超时、权限异常或依赖缺失不得改写为 skip：被测结果不符为 `fail`，环境使测试无法开始为 `blocked`。跳过项在需求和门槛汇总中一律为 `not-evaluated`，从不计作 `pass`。
- 默认提交门禁中的测试不得长期 disable；临时隔离必须有缺陷/任务引用、责任人和恢复条件。过滤后零测试、所需标签没有发现测试或清单数量下降均为失败。
- 重试只用于诊断不稳定性或已登记环境策略，必须保持同一提交、二进制、输入 hash、seed 和环境清单，并保存首轮及全部 attempt。后续通过不能覆盖首轮失败；稳定性结论仍为 `fail` 或 `blocked`，直至根因修复并完成明确复测。

## 6. 黄金样例规则

每个黄金样例必须在版本化 manifest 中具有：稳定 ID、语义类别、来源、许可证/SPDX、生成器和固定工具版本、规范配方、配方 SHA-256、实际字节 SHA-256、字节长度、探测摘要/hash、契约与 schema 版本、期望语义、容差单位/理由/批准来源、可重建性和维护人。

- 只使用项目生成、项目原创固定字节或取得明确可再分发许可的非敏感小样例。A-014 的 12 个样例均为 CC0-1.0 起点。
- `package/` 中的逆向样本、未知许可内容、用户媒体和仅允许内部查看的附件不得成为可分发 golden，也不得为获取测试输入而读取或复制。
- 生成前先校验配方 hash 和工具链；生成后校验实际 hash 与 ffprobe/结构摘要。固定工具链下实际 hash 不匹配时失败并保留两份事实，不自动更新期望。
- 期望值变更必须引用需求、契约或算法版本变更，并与被测实现变更分开复核。禁止在同一未经复核的修改中重生成 golden 后据新期望宣告通过。
- 像素、浮点音频或性能若需要容差，必须写明单位、比较方法、平台范围和批准来源；没有批准容差时只保存原始值并标 `measured/not-evaluated`。

## 7. 性能、故障和兼容性规则

### 7.1 性能

先验证功能，再测性能。微基准至少 5 次预热、30 次记录；完整场景至少 1 次预热、5 次记录；高成本长场景在相同冷热缓存条件下至少记录 3 次。每次保存原始样本，不只保留平均数，摘要至少给出中位数、P95、最小/最大和峰值资源。

指标包括 UI 输入到可见反馈、首次预览、seek 落点/耗时、分析耗时与吞吐、导出实时比、预览/导出漂移、工作集/私有字节峰值、缓存和磁盘 I/O、线程/进程峰值。只有环境签名、输入 hash、构建、依赖、缓存状态和电源策略一致时才做趋势比较；否则建立新基线并标不可直接比较。

最低 Windows、基准/推荐硬件、典型/最大素材、UI P95、分析/导出预算、像素容差和“卡点自然”阈值尚未全部确认。确认前数值只能为 `measured`，对应 TR-Q-003/004/008 及 G1/G3/G4 的性能或效果结论为 `not-evaluated`。

### 7.2 故障注入

故障点覆盖：事务提交点前/后取消、worker 握手前后崩溃、进度乱序/重复/缺号、IPC 截断/超限/版本冲突、媒体损坏/缺流/时间戳缺失/格式切换、缓存损坏、素材丢失/指纹冲突、磁盘不足/部分写入/权限拒绝、导出目标并发占用和应用异常退出。每项同时断言错误 code/diagnosticId、终态、最近成功项目、既有输出、临时文件和可重试性。

FI 的注入位置和次数是版本化输入。测试必须证明故障实际命中；未命中注入点不得报告通过。首次失败之后的诊断重试单独记录，不能覆盖首轮结果。

### 7.3 兼容性

兼容矩阵分为：核心 DTO/schema/contract/vectorSet、媒体 DTO/schema/contract/goldenManifest、IPC protocol、项目 schema/迁移、Debug/Release/CI ABI/CRT、Windows build、Qt/FFmpeg 精确版本、GPU/RHI、DPI/字体、音频设备和编解码能力。

旧版本样例必须只读保存并验证迁移/拒绝/round-trip 语义；未知 schema、required feature 和 enum 必须按契约拒绝，不得静默降级。当前只确认 Windows x64 与现有固定工具链，最低 Windows 版本和设备矩阵未确认，因此本机 Windows 11 结果不能代表 G4 兼容通过。

## 8. TR-A 架构需求追踪

| 需求 | 主要测试层 | 计划 oracle 与证据 | 当前独立状态 |
|---|---|---|---|
| TR-A-001 | QQ/CT/IT | QML 只做视图/绑定/交互；Qt/C++ bridge 提供模型/命令；长任务不在 UI 线程；依赖与事件循环证据 | `not-evaluated`，等待 UI 实现 |
| TR-A-002 | UT/CT/CO | core/分析可在 headless Windows 单独构建运行；公共核心无 Qt Widgets/QML/场景图依赖 | `not-evaluated`；T-015 开发证据可用 |
| TR-A-003 | CT/IT/FI | 依赖方向单向；进度/结果/取消/错误只经版本化接口或消息；过期结果不回写 UI/核心 | `not-evaluated`；T-016 开发证据可用 |
| TR-A-004 | CT/CO | Int64/UInt64 精确往返、UTF-8、显式长度和版本；Qt/FFmpeg 类型不进入稳定 schema/ABI | `not-evaluated`；A-012/T-016 oracle 可用 |
| TR-A-005 | UT/IT/FI/QQ | lease 生命周期、创建/释放方、只读快照、线程亲和性、取消/flush 后不悬空 | `not-evaluated`；T-016/T-018 开发证据可用 |
| TR-A-006 | CO/UT/QQ | 固定 CMake presets、x64/MSVC/CRT/Qt/FFmpeg 门禁，x86/ARM64 拒绝，headless core 与独立 Qt 测试 | `not-evaluated`；T-013 开发证据可用 |

## 9. TR-F 功能需求追踪

| 需求 | 主要测试层 | 计划 oracle 与证据 | 当前独立状态 |
|---|---|---|---|
| TR-F-001 | GM/IT/FI | 只读探测容器/流/时长/几何/采样；不支持、损坏、受保护输入返回结构化错误且无部分成功 | `not-evaluated`；A-014/A-015 开发证据可用 |
| TR-F-002 | GM/IT/PF/FI | 代理/缩略图/波形流式且可取消；队列和内存有界；缓存键覆盖指纹/版本/参数并正确失效 | `not-evaluated`；T-016/T-018 开发证据可用 |
| TR-F-003 | QQ/IT/GM | 预览、波形、事件、播放头和缩放共享真实 `timeNs`；拖动/播放/定位不建立第二套时钟 | `not-evaluated`，等待 UI/同步实现 |
| TR-F-004 | UT/CT/QQ/IT | 增删移选锁、强度/类型/音色、吸附/偏移、撤销重做均为可回滚事务 | `not-evaluated`；A-012/T-015 开发证据可用 |
| TR-F-005 | UT/GM/IT/PF | 分别输出 shot/motion_peak/action_peak，包含来源、强度、置信度和版本；静止/运镜/闪烁等样例 | `not-evaluated`，等待视频算法实现及代表样例 |
| TR-F-006 | UT/CT/GM | 权重/间隔/密度/窗口版本化；相同输入/参数/种子等价；候选来源与抑制原因可追踪 | `not-evaluated`；A-012/T-015 开发证据可用 |
| TR-F-007 | UT/CT/QQ/FI | 重分析和参数变化保持人工/锁定事件；失败、取消、过期修订不修改当前时间线 | `not-evaluated`；A-012/T-015 开发证据可用 |
| TR-F-008 | UT/CT/QQ/GM | 不超过三组一期音色的映射、替换和试听；声音 ID/参数版本化；随包音色来源/许可证/hash 完整 | `not-evaluated`，等待音频实现和合法音色输入 |
| TR-F-009 | UT/GM/PF/FI | 固定 revision 确定性 PCM；重叠/尾音/峰值/削波有 oracle 与告警；失败不产生伪完整音轨 | `not-evaluated`，等待音频渲染实现 |
| TR-F-010 | UT/GM/PF | 节拍、瞬态、响度、频段能量及置信度；自由节奏/弱瞬态/噪声不伪造稳定 BPM | `not-evaluated`，等待音频分析实现与样例 |
| TR-F-011 | QQ/GM/PF/CO | 三类视觉模板及灵敏度/密度/强度/偏移；高密度不采用一点一 QML Item | `not-evaluated`，等待渲染实现和风格输入 |
| TR-F-012 | CT/QQ/GM/CO | 屏上/离屏共享 RenderRecipe、seed、timelineRevision；时序一致；像素差只按批准容差判定 | `not-evaluated`，像素容差未确认 |
| TR-F-013 | CT/IT/FI/CO | 新建/保存/另存/重开/重定位/autosave/恢复；状态完整；旧 schema 迁移或明确拒绝 | `not-evaluated`；T-016 开发证据可用 |
| TR-F-014 | IT/GM/CO/PF | 三类输出按已确认格式矩阵探测流、时长、PTS、同步和可播放性；编码器/许可证随证据固定 | `not-evaluated`，T-019 与发布格式矩阵未完成 |
| TR-F-015 | CT/IT/FI | 冻结 revision 后写临时目标并校验提交；取消/磁盘不足/编码失败/崩溃不覆盖既有文件 | `not-evaluated`，等待 T-019 |
| TR-F-016 | QQ/IT/FI | 阶段、进度、取消、低质量标识、可行动错误和 diagnosticId 对用户可见且与底层一致 | `not-evaluated`，等待 UI/同步实现 |
| TR-F-017 | CO/FI | 所有外部依赖固定来源、版本、许可证、hash；无隐式联网下载/执行；完整性失败可诊断 | `not-evaluated`；T-013/A-015 提供部分输入 |
| TR-F-018 | CO/IT/FI | 安装包/EXE 发布者、私有依赖、版本/hash/许可证/SBOM、升级卸载和 DLL 搜索路径 | `not-evaluated`，等待发布实现与签名输入 |

## 10. TR-Q 质量需求追踪

| 需求 | 主要测试层 | 计划 oracle 与证据 | 当前独立状态 |
|---|---|---|---|
| TR-Q-001 | CT/GM/IT/PF | CFR/VFR 下预览可见事件与导出误差不超过一个输出帧；音轨/视频同零点和同 revision | `not-evaluated`，等待 T-019 及端到端实现 |
| TR-Q-002 | UT/CT/GM/CO | 相同指纹/版本/参数/seed/平台产生等价事件和离线音频；渲染差按批准容差 | `not-evaluated`；核心/媒体部分开发证据可用，像素容差未确认 |
| TR-Q-003 | QQ/PF/FI | 长任务不阻塞 UI；编辑与播放反馈测量 P50/P95/最大值；事件循环故障可诊断 | `not-evaluated`，基准硬件和 P95 阈值未确认 |
| TR-Q-004 | PF/GM/FI | 流式/分块、显式内存/缓存/线程/作业上限；首次预览、分析、导出和资源峰值 | `not-evaluated`；已有数值仅 `measured`，素材上限/硬件/预算未确认 |
| TR-Q-005 | FI/IT/CO | 取消、worker 崩溃、磁盘不足、素材丢失、迁移失败不破坏最近成功保存；恢复来源/时间明确 | `not-evaluated`；T-016 开发证据可用 |
| TR-Q-006 | FI/CO/IT | 不可信媒体在受限 worker；路径不经 shell；默认离线；日志/附件不泄露媒体或隐私 | `not-evaluated`，需 T-022 安全边界验证 |
| TR-Q-007 | CO/FI | 固定依赖、许可证、SBOM、hash、签名、恶意软件检查、干净安装/卸载 | `not-evaluated`，等待发布工作流 |
| TR-Q-008 | CO/GM/QQ | 已承诺 Windows 版本共享 schema/golden；GPU、设备、编码器、DPI/字体能力矩阵明确 | `not-evaluated`，最低 Windows 与设备矩阵未确认 |
| TR-Q-009 | CT/CO/IT | 媒体/分析/事件/渲染/项目接口版本化；参数/算法进入项目和诊断；旧项目变更可追踪 | `not-evaluated`；A-012/A-014/T-016 提供部分 oracle |
| TR-Q-010 | UT/CT/GM/IT/QQ/PF/FI/CO | 分析器确定性/边界/基准样例；时间、锁定融合、原子保存、导出事务自动化；端到端比较事件/峰值/帧时间戳 | `not-evaluated`；T-021/T-022 尚未执行 |

## 11. G0～G4 门槛追踪

| 门槛 | 必需证据与判定 | 当前状态和解除条件 |
|---|---|---|
| G0 | 代表视频/音频、最低 Windows、基准硬件、典型/最大素材、人工修正量、自然度评分与阈值、音色/视觉风格及许可均有确认来源、版本和 hash；构建基线可复现 | `not-evaluated`。工具链已就绪，但产品样例、硬件、规模、效果和内容输入仍未全部确认 |
| G1 | 样例覆盖稳定/自由节奏、快切、慢镜头、静止、运镜和动作；候选可解释并生成可试听节奏轨；记录耗时、密度、人工修正和盲评 | `not-evaluated`。算法与音频任务未完成，且效果阈值未确认；后续数值只能先记 `measured` |
| G2 | 导入→分析→编辑/锁定→试听→保存重开→导出闭环；重分析保护、撤销重做、取消/失败原子性；同 revision 下预览/导出误差不超过一帧 | `not-evaluated`。等待 UI、算法、音频、渲染、T-019、T-021/T-022 集成 |
| G3 | 音频特征和三类视觉；屏上/离屏同配方；损坏媒体、无节奏、静止、素材丢失、磁盘不足、worker 崩溃、异常恢复；受控性能记录 | `not-evaluated`。相关实现与基准输入未齐；性能结果无阈值时只为 `measured` |
| G4 | 每个已承诺 Windows 版本的干净安装/首次启动/导入/预览/保存/导出/升级/卸载；无隐式下载、私有依赖、签名、hash、许可证/SBOM、格式/GPU 矩阵 | `not-evaluated`。最低 Windows、发布格式、安装器和签名尚未全部确认或实现 |

## 12. T-021、T-022 执行边界

T-021 才实现统一测试入口、独立契约测试和黄金样例库：核对 A-012 72/72、A-014 30/30 的 ID 映射，接入短媒体 golden、core/system/media 契约、headless 与 Qt/QML smoke，生成首次 tester 独立证据。T-020 不创建这些代码，也不沿用实现方的“通过”作为独立结论。

T-022 才执行完整端到端、故障恢复、性能测量和 G0～G4 汇总；其开始受 T-019、T-021、UI、视频、音频、渲染和发布输入约束。未确认阈值继续保持 `measured/not-evaluated`，不通过降低完成条件推进门槛。

## 13. 自查结果

- 追踪表逐项列出 TR-A-001～006、TR-F-001～018、TR-Q-001～010 和 G0～G4，无编号遗漏或重复。
- 已定义 UT/CT/GM/IT/QQ/PF/FI/CO 八层、目录、命名、CTest 标签、统一入口和开发自测/独立验证边界。
- 已定义 seed、PRNG、时钟、locale、临时目录、进程清理、日志、环境清单、重试、WDAC 回退、黄金样例来源/许可证/hash/期望更新规则。
- 已定义 `pass/fail/blocked/not-run/measured/not-evaluated`，并明确无已确认性能、效果、硬件或兼容阈值时不得报告通过。
- 本轮只形成计划和台账，没有执行 T-021/T-022、没有运行测试、没有修改被测实现或 `package/`，也没有把 `scripts/__pycache__/` 纳入成果。
