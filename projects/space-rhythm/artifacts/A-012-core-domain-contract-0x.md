# 核心时间、事件、修订与事务契约 0.x

- 项目：space-rhythm
- 成果 ID：A-012
- 负责人：core-systems-engineer-cpp-01
- 关联任务：T-014
- 版本：0.1
- 更新日期：2026-09-09
- 状态：draft
- 适用范围：定义第一阶段核心领域的规范时间、轨道、事件、分析候选、时间线修订、原子事务、撤销/重做、错误分类和逻辑 DTO 兼容规则，并提供 T-017/T-020 可直接消费的测试向量；不定义媒体 PTS/DTS 解释、QML/Qt 类型、具体序列化编码、IPC 传输或持久化介质。
- 来源及输入版本：[A-004 0.5](A-004-mvp-technical-feasibility-and-requirements.md) 第 4 节、TR-A-002～005、TR-F-004/006/007，[A-006 0.1](A-006-domain-work-packages.md) WP-02，[A-007 0.1](A-007-four-engineer-execution-plan.md) C-01～C-03，[A-010 0.1](A-010-cpp-core-systems-execution-plan.md)，T-014；D-001/D-002/D-004～D-008 confirmed，D-003 proposed。
- 批准依据：尚无。T-014 只要求负责人自查并形成候选契约，不要求独立评审或用户批准；后续实现及消费者反馈可触发 0.x 修订。
- 契约版本：`coreContractVersion = 0.1.0`。
- 逻辑 DTO schema：`schemaVersion = 1`。
- 测试向量集：`vectorSetVersion = 1`。
- 版本记录：2026-09-09，0.1，首次定义 C-01～C-03 语义、事务与历史规则、错误 taxonomy、DTO 兼容边界及正常/边界/错误测试向量。

## 1. 规范性约定与责任边界

本文中的“必须”“不得”“应”是规范要求；“可以”“建议”是非强制实现选择。实现只有在满足全部相关规范和测试向量时，才能声明兼容 `coreContractVersion 0.1.0`。

核心领域拥有：

- C-01：项目规范时间、精确换算目标、舍入与溢出语义。
- C-02：轨道、事件、候选 envelope、修订、锁定与人工编辑语义。
- C-03：命令、原子事务、撤销/重做、失败与取消语义。

核心领域不拥有：

- 媒体层对 PTS、DTS、`time_base`、start time、VFR、seek、音频采样时间的解释；T-017/T-018 负责把这些事实映射为本文定义的 `TimeNs`。
- QML 页面、`QObject`、Qt 容器、FFmpeg 结构体、帧/PCM 缓冲、音频设备、视觉渲染或算法私有特征。
- DTO 的 JSON/二进制编码、IPC 传输和项目文件扩展名；这些具体技术选择受 D-003 和后续任务约束。

任何消费者不得持久化浮点秒、帧号或媒体 PTS 来替代 `TimeNs`，不得复制出语义不同的时间、事件或修订模型。

## 2. 基础类型与标识符

### 2.1 整数类型

| 类型 | 逻辑范围 | 规则 |
|---|---:|---|
| `Int64` | `[-9223372036854775808, 9223372036854775807]` | 有符号 64 位整数；跨边界不得先转换为浮点数。 |
| `UInt64` | `[0, 18446744073709551615]` | 无符号 64 位整数；DTO 编码必须保持精确整数语义。 |
| `UInt32` | `[0, 4294967295]` | 用于有界序号、计数和 `NormPpm`。 |
| `NormPpm` | `[0, 1000000]` | 百万分比定点值；`0` 对应 0，`1000000` 对应 1。 |

`strength` 与 `confidence` 在稳定 DTO 中使用 `NormPpm`，避免把 NaN、无穷或平台浮点差异带入持久化真值。算法内部可使用浮点，但进入契约前必须按版本化算法明确量化；量化方法属于产生者算法版本的一部分。

### 2.2 OpaqueId

`ProjectId`、`TrackId`、`EventId`、`AnalysisRevision`、`TransactionId`、`DiagnosticId` 和候选 ID 均是强类型 `OpaqueId`，不能相互替换。

- DTO 表示必须匹配 ASCII 正则 `[A-Za-z0-9][A-Za-z0-9._:-]{0,127}`。
- 比较采用 ASCII 字节的区分大小写字典序；不做 Unicode、大小写或路径规范化。
- 同类 ID 在其作用域内必须唯一，生成后不得因排序、移动、保存或重分析而改变。
- 本契约不冻结 UUID、ULID 或其他生成算法；生成器必须证明在项目作用域内不会复用 ID。
- 用户可见名称与 ID 分离；名称允许 UTF-8，但不得参与身份判断或稳定排序。

## 3. C-01：规范时间与换算

### 3.1 TimeNs、DurationNs 与 ProjectTimeNs

- `TimeNs` 是 `Int64` 纳秒值，允许负值，用于偏移、源映射中间结果及显式预卷语义。
- `ProjectTimeNs` 是受约束的 `TimeNs`，有效范围为 `[0, INT64_MAX]`。事件在已提交项目时间线上的 `timeNs` 必须是 `ProjectTimeNs`。
- `DurationNs` 有效范围为 `[0, INT64_MAX]`；点事件的持续时间为 0。
- 非法/未知时间不得使用 `INT64_MIN`、`-1` 或其他哨兵表示，必须使用显式可选状态或 `ErrorInfo`。
- 所有算术必须检查结果；不得依赖 C/C++ 有符号整数溢出、截断或环绕行为。

### 3.2 区间

`TimeRange` 使用半开区间 `[startNs, endNs)`：

- `startNs`、`endNs` 均为 `ProjectTimeNs`，且 `endNs >= startNs`。
- `durationNs = endNs - startNs` 必须可表示为 `DurationNs`。
- 点事件不使用空区间表达，它由 `RhythmEvent.timeNs` 与 `durationNs = 0` 表达。
- 相邻区间 `[a,b)` 与 `[b,c)` 不重叠。
- 从 `timeNs + durationNs` 构造端点时必须先做溢出检查。

### 3.3 精确比例换算

通用时间基表示为：

```text
TimeBase {
  secondsNumerator: Int64  // 必须 > 0
  secondsDenominator: Int64 // 必须 > 0
}
```

将整数刻度 `ticks` 转为纳秒的数学真值为：

```text
exactNs = ticks * secondsNumerator * 1_000_000_000 / secondsDenominator
```

实现必须按数学上的无限精度中间值判断，或使用等价的约分/checked 算法；不得让中间乘法溢出后再检查结果。最终取整结果必须落入 `Int64`。

### 3.4 舍入模式

每次非整除换算必须显式传入下列模式之一，不设隐式默认值：

| 模式 | 规则 |
|---|---|
| `floor` | 向负无穷取整。 |
| `ceil` | 向正无穷取整。 |
| `toward_zero` | 向 0 截断。 |
| `nearest_ties_to_even` | 取最近整数；恰好一半时取偶数，负数规则对称。 |

边界所有者必须记录选择原因。建议区间起点使用 `floor`、覆盖型区间终点使用 `ceil`，显示/最近采样选择使用 `nearest_ties_to_even`；这只是调用约定建议，具体媒体语义由 T-017 定义。

### 3.5 时间 API 的规范行为

下列是语义签名，不是已冻结的 C++ 头文件：

```text
Result<TimeNs, ErrorInfo> scaleTicks(Int64 ticks, TimeBase base, RoundingMode mode)
Result<TimeNs, ErrorInfo> checkedAdd(TimeNs lhs, TimeNs rhs)
Result<TimeNs, ErrorInfo> checkedSubtract(TimeNs lhs, TimeNs rhs)
Result<TimeRange, ErrorInfo> makeRange(ProjectTimeNs start, DurationNs duration)
```

`scaleTicks` 不解释 ticks 是 PTS、采样索引还是帧号；调用方拥有该语义。结果若用于项目事件，还必须通过 `ProjectTimeNs` 非负校验。

## 4. C-02：轨道、事件与候选

### 4.1 Track

```text
Track {
  schemaVersion: 1
  id: TrackId
  orderIndex: UInt32
  label: optional UTF-8 string
  extensions: ExtensionMap
}
```

- `Track.id` 在项目内稳定唯一。
- 已提交快照内 `orderIndex` 必须从 0 开始连续且不重复；轨道顺序先按 `orderIndex`，再以 `TrackId` 作为防御性最终比较，但重复 `orderIndex` 仍是无效快照。
- `label` 只用于显示，不参与相等性之外的身份判断；修改 label 是可撤销的语义修改。
- 删除非空轨道必须显式选择 `reject_non_empty` 或 `delete_events`，禁止隐式级联。

### 4.2 EventKind

schema 1 的已知 `EventKind` token 为：

`shot`、`motion_peak`、`action_peak`、`beat`、`onset`、`band_energy`、`manual`。

token 使用小写 snake_case。未知 token 不得被静默映射成 `manual` 或其他已知值；执行模式必须返回 `unknown_enum`。只有兼容规则允许的 opaque passthrough 可以原样保留未知扩展数据。

### 4.3 EventSource

```text
EventSource {
  schemaVersion: 1
  origin: "user" | "analysis" | "import"
  producerId: OpaqueId
  producerVersion: non-empty ASCII string
  inputFingerprint: optional non-empty ASCII string
  parametersDigest: optional non-empty ASCII string
  analysisRevision: optional AnalysisRevision
  candidateIds: ordered unique list<CandidateId>
  extensions: ExtensionMap
}
```

- `origin = analysis` 时 `analysisRevision`、`inputFingerprint`、`parametersDigest` 必须存在。
- `origin = user` 时 `analysisRevision` 必须为空，`confidence` 必须是 `not_applicable`。
- `origin = import` 必须记录可追踪的 `producerId` 与 `producerVersion`；能获得时应记录输入指纹。
- `candidateIds` 只记录实际贡献或被聚合的候选，顺序必须稳定且不得重复。
- `producerVersion` 与 `parametersDigest` 共同决定结果复现边界；不得只依赖日志文本。

### 4.4 RhythmEvent

```text
RhythmEvent {
  schemaVersion: 1
  id: EventId
  trackId: TrackId
  timeNs: ProjectTimeNs
  durationNs: DurationNs
  kind: EventKind
  source: EventSource
  strengthPpm: NormPpm
  confidencePpm: optional NormPpm // empty 表示 not_applicable
  locked: bool
  userEdited: bool
  soundAssignment: optional VersionedOpaqueObject
  extensions: ExtensionMap
}
```

不变量：

1. `id` 在项目内唯一，重分析不得把旧 ID 分配给另一事件。
2. `trackId` 必须引用同一快照中的现有轨道。
3. `timeNs + durationNs` 必须可表示，且不得超过 `INT64_MAX`。
4. `strengthPpm` 必须在 `[0,1000000]`；`confidencePpm` 存在时也必须在该范围。
5. 人工来源事件的 `confidencePpm` 必须为空；不得伪造模型置信度。
6. 分析来源事件必须携带 `analysisRevision`、输入指纹、算法版本和参数摘要。
7. `locked = true` 时，分析事务不得移动、删除、改型、改轨、改强度、改声音分配或替换该事件。
8. 显式用户事务若要修改已锁定事件，必须先在同一事务中将其解锁，或只执行解锁；操作按事务顺序在临时状态上校验。
9. 用户完成移动、改型、改强度、改轨或声音分配后，结果事件的 `userEdited` 必须为 true；撤销必须恢复修改前的值。
10. `soundAssignment` 由音频领域解释；核心只校验其版本化 envelope、稳定 ID 与大小限制并原样保存，不解释音色参数。

### 4.5 AnalysisCandidate

```text
AnalysisCandidate {
  schemaVersion: 1
  id: CandidateId
  proposedTrackId: optional TrackId
  timeNs: ProjectTimeNs
  durationNs: DurationNs
  kind: EventKind // 不得为 manual
  source: EventSource // origin 必须为 analysis
  strengthPpm: NormPpm
  confidencePpm: NormPpm
  payload: VersionedOpaqueObject
  extensions: ExtensionMap
}
```

- 候选是不可变输入，不直接出现在已提交时间线中。
- 候选进入融合前必须已经映射到规范项目时间；核心不接受 PTS、浮点秒或帧号候选。
- payload 的领域所有者是视频或音频算法；核心可保留并追踪，但不得据未知 payload 改变公共融合语义。
- 同一候选集的稳定身份由 `analysisRevision`、输入指纹、算法版本、参数摘要、随机种子（若使用）及按 ID 排序后的候选共同决定。

### 4.6 稳定排序与等价性

已提交快照中的轨道按 `orderIndex` 升序排列。事件使用以下全序键升序排列：

```text
(timeNs, track.orderIndex, EventId ASCII bytes)
```

事件 kind、强度、来源和插入顺序不得作为隐式 tie-breaker。因为 EventId 唯一，上述键能够形成全序。消费者即使收到不同输入顺序，也必须得到相同顺序。

两个事件集合“等价”是指：按上述全序排列后，所有 schema 1 规范字段及已声明参与语义的扩展逐项相等；诊断日志、内存地址、容器迭代顺序和未声明缓存不参与等价判断。

## 5. 修订与快照

### 5.1 TimelineRevision

- 新项目的初始 `timelineRevision` 为 0。
- 每个产生语义状态变化的成功提交生成 `current + 1`；到达 `UINT64_MAX` 后必须拒绝新的语义提交并返回 `revision_exhausted`。
- 撤销和重做也是新提交，因此生成新修订；不得把修订号倒退到历史值。
- `no_change` 不生成新修订，不写入历史。
- 保存/重新打开后保留当前修订号，下一次提交继续递增；加载和 schema 迁移本身不伪装成用户时间线编辑。

### 5.2 AnalysisRevision

`AnalysisRevision` 是不可变分析结果的 OpaqueId，不要求数值单调。不同输入指纹、算法版本、参数摘要或随机种子必须产生不同修订身份；同一修订身份不得对应两组不等价候选。

### 5.3 TimelineSnapshot

```text
TimelineSnapshot {
  schemaVersion: 1
  coreContractVersion: "0.1.0"
  projectId: ProjectId
  timelineRevision: UInt64
  tracks: ordered list<Track>
  events: ordered list<RhythmEvent>
  extensions: ExtensionMap
}
```

- 对外发布的快照不可变；消费者不得通过保留引用修改核心状态。
- 所有引用和不变量必须在发布前整体校验。
- 读取者可以长期持有旧快照；旧快照不会因新提交在原地改变。
- 后台结果只有在事务基准修订仍满足前置条件时才可合入，过期结果不得覆盖新状态。

## 6. C-03：命令与原子事务

### 6.1 TransactionRequest

```text
TransactionRequest {
  schemaVersion: 1
  transactionId: TransactionId
  baseTimelineRevision: UInt64
  origin: "user" | "analysis" | "system"
  operations: ordered non-empty list<Operation>
  coalescingKey: optional OpaqueId
  extensions: ExtensionMap
}
```

schema 1 支持以下语义操作：

- `AddTrack`、`RemoveTrack`、`ReorderTracks`、`RenameTrack`。
- `AddEvent`、`RemoveEvent`、`MoveEvent`、`PatchEvent`、`SetEventLocked`。
- `BatchOffsetEvents`：显式 EventId 集合与 `deltaNs`，全部成功或全部失败。
- `MergeAnalysisCandidates`：输入不可变候选集、分析修订和版本化融合参数，作为一个可撤销事务提交。

操作 DTO 必须使用显式字段；不得依赖函数闭包、指针、UI 对象或进程内地址。

### 6.2 提交算法

提交必须表现为以下不可分割序列：

1. 校验并规范化请求，先查询 `transactionId`：等价重放直接返回首次结果，不等价重用返回 `idempotency_conflict`。
2. 在提交串行化点读取当前快照。
3. 比较 `baseTimelineRevision`；不相等则返回 `stale_revision`，不执行任何 operation。
4. 检查取消状态；已取消则返回 `transaction_cancelled`，不修改状态。
5. 在当前快照的临时工作副本上按列表顺序执行 operation。
6. 校验每个 operation 的前置条件及最终全局不变量；任一失败都丢弃整个临时结果。
7. 再次检查可观察取消状态并进入唯一提交点。
8. 若语义状态未变化，返回 `no_change`，保持当前修订且不写历史。
9. 否则原子发布新快照，修订加一，并生成一条可撤销历史记录。

进入第 9 步的原子提交点后收到取消请求，事务结果必须是 `committed`；不得返回 cancelled 却已改变状态。实现可用提交锁、单线程协调器或等价机制，但外部行为必须一致。

### 6.3 TransactionResult

```text
TransactionResult {
  schemaVersion: 1
  transactionId: TransactionId
  status: "committed" | "no_change" | "rejected" | "cancelled"
  beforeRevision: UInt64
  afterRevision: UInt64
  error: optional ErrorInfo
  changeSummary: optional ChangeSummary
  suppressionReport: optional SuppressionReport
}
```

- `committed`：`afterRevision = beforeRevision + 1`，`error` 为空。
- `no_change`：两个 revision 相等，`error` 为空，不产生历史。
- `rejected`：两个 revision 相等，必须有 error。
- `cancelled`：两个 revision 相等，error code 为 `transaction_cancelled`。
- `suppressionReport` 记录融合中因锁定、人工保护、窗口、密度或重复候选而未进入结果的 CandidateId 及稳定原因码；抑制不是静默丢弃。

### 6.4 幂等与并发

- 在同一已打开项目会话中，相同 `transactionId` 和规范等价请求重复提交，必须返回第一次提交结果，不得再次修改状态。
- 相同 `transactionId` 配合不等价请求必须返回 `idempotency_conflict`。
- 所有写提交按单一全序串行化；读取通过不可变快照并发进行。
- 过期 `baseTimelineRevision` 不自动 rebase。调用方必须取得新快照、重新生成显式意图并使用新 TransactionId。

### 6.5 锁定与分析合并

`MergeAnalysisCandidates` 必须遵守：

1. 新候选先在临时状态中规范化和稳定排序。
2. 人工事件完整保留；锁定事件完整保留。
3. 与锁定事件进入版本化合并窗的候选按参数规则抑制或保留，但不得改写锁定事件；处理结果写入 suppression report。
4. 自动事件只有在融合参数明确允许且未受人工/锁定保护时才可替换。
5. 参数、算法版本、输入指纹和随机种子（若使用）必须写入来源与分析修订。
6. 整次合并只产生一个事务和一个时间线修订，可由一次 undo 撤销。
7. 取消、验证失败或修订冲突保持当前快照不变。

schema 1 的 suppression reason token 至少包括 `protected_manual`、`protected_locked`、`duplicate_candidate`、`merge_window` 和 `density_limit`；每个被抑制候选必须精确记录一个主原因，可附带其他原因扩展。

具体权重、窗口、密度、优先级及种子算法由 T-015 实现并版本化，不属于本 0.1 契约的固定数值。

## 7. 撤销、重做与保存点

### 7.1 历史条目

每个成功且非空的领域事务产生一条逻辑历史条目：

```text
HistoryEntry {
  transactionId: TransactionId
  origin: user | analysis | system
  semanticForwardDelta: immutable value
  semanticInverseDelta: immutable value
  beforeStateIdentity: value
  afterStateIdentity: value
  coalescingKey: optional OpaqueId
}
```

历史不得持有 UI 对象、裸指针、临时缓冲或生命周期不受控的外部资源。实现可使用结构化差异、共享不可变快照或检查点，但外部语义必须等价。

### 7.2 Undo

- `undo(expectedCurrentRevision)` 必须先校验当前修订；不匹配返回 `stale_revision`。
- 无可撤销条目时返回 `no_change`，不增加修订。
- 成功 undo 原子应用栈顶 inverse delta，生成新 `timelineRevision`，并把该条目移入 redo 栈。
- 若 inverse delta 的身份前置条件不成立，返回 `history_conflict`，状态与两个栈均不改变。
- 分析合并与批量命令按单条历史撤销，不拆成用户不可预测的多步。

### 7.3 Redo

- `redo(expectedCurrentRevision)` 使用与 undo 相同的修订和原子性规则。
- 成功 redo 应产生与原成功提交语义等价的状态，但生成新的 timeline revision。
- undo 后发生任何新的非空领域提交，必须清空 redo 栈。

### 7.4 连续操作合并

默认不合并相邻事务。只有同时满足以下条件才可合并为一个可撤销历史单元：

- origin 都是 `user`；
- `coalescingKey` 非空且相同；
- 作用的实体集合相同；
- 中间没有其他事务、保存点或外部修订；
- 合并后的 inverse delta 仍精确恢复第一笔事务之前的状态。

每笔中间事务仍生成自己的 timeline revision；合并只影响 undo 粒度。调用方不希望合并时必须不提供 `coalescingKey`；核心收到满足上述条件的相同 key 时必须合并，不能再以实现差异或隐式时间窗口改变结果。

### 7.5 保存点与持久化边界

- dirty 判断基于当前时间线的规范语义状态是否等价于最近成功保存状态，而不是比较 revision 数值；undo/redo 回到相同内容时应恢复 clean。
- 最近成功保存状态使用版本化语义摘要或等价机制标识；摘要算法与编码由存储任务定义，不能用不稳定容器遍历结果。
- undo/redo 历史在 schema 1 中不持久化。重新打开项目后 undo/redo 栈为空，当前已加载状态成为保存点并标记 clean。
- 自动保存不改变用户显式保存点，除非后续产品契约明确改变该语义。

## 8. 错误分类与 ErrorInfo

### 8.1 ErrorInfo

```text
ErrorInfo {
  schemaVersion: 1
  category: ErrorCategory
  code: ErrorCode
  stage: non-empty ASCII token
  diagnosticId: DiagnosticId
  retryable: bool
  messageKey: non-empty ASCII token
  context: map<ASCII key, bounded scalar>
  cause: optional ErrorInfo
  extensions: ExtensionMap
}
```

- `code` 是机器接口，`messageKey` 用于本地化映射；人类日志文本不得参与程序分支。
- 同一次失败跨层传播时应保留 `diagnosticId`；包装 cause 不得伪造新的根因。
- context 只包含定位所需的有界标量，不默认记录媒体内容、完整用户路径或隐私数据。
- 未知异常必须映射为 `internal_error` 并保留诊断 ID，不能伪装成 validation error。

### 8.2 分类与稳定错误码

| Category | ErrorCode | 触发条件 | 是否建议重试 |
|---|---|---|---|
| `validation` | `invalid_identifier` | ID 为空、格式或长度无效。 | 否 |
| `validation` | `invalid_time_base` | 时间基分子/分母非正或缺失。 | 否 |
| `validation` | `time_overflow` | 时间算术或端点不可表示。 | 否 |
| `validation` | `invalid_dto` | DTO 缺少/重复字段、类型错误或出现未知顶层字段。 | 否 |
| `validation` | `invalid_track` | 轨道字段或顺序不满足不变量。 | 否 |
| `validation` | `invalid_event` | 事件字段、范围、来源或置信度不满足不变量。 | 否 |
| `validation` | `duplicate_track_id` | 同一快照存在重复 TrackId。 | 否 |
| `validation` | `duplicate_event_id` | 同一项目存在重复 EventId。 | 否 |
| `validation` | `track_not_found` | Event 或 operation 引用不存在轨道。 | 取得新输入后 |
| `validation` | `event_not_found` | operation 引用不存在事件。 | 取得新输入后 |
| `validation` | `track_not_empty` | 删除非空轨道但未显式允许级联。 | 否 |
| `validation` | `invalid_transaction` | operation 为空、顺序或字段不合法。 | 否 |
| `conflict` | `stale_revision` | base/expected revision 不是当前 revision。 | 是，重新取快照 |
| `conflict` | `locked_event` | 非法修改锁定事件。 | 用户显式解锁后 |
| `conflict` | `history_conflict` | undo/redo 身份前置条件不成立。 | 否，需诊断 |
| `conflict` | `idempotency_conflict` | 同 TransactionId 对应不同规范请求。 | 否 |
| `conflict` | `analysis_revision_conflict` | 同 AnalysisRevision 对应不等价候选集或来源事实。 | 否，需重新生成身份 |
| `conflict` | `revision_exhausted` | timeline revision 已为 UINT64_MAX。 | 否 |
| `cancelled` | `transaction_cancelled` | 提交点前观察到取消。 | 由用户决定 |
| `compatibility` | `unsupported_schema` | schemaVersion 不受支持。 | 升级软件/迁移后 |
| `compatibility` | `unsupported_feature` | required feature 不受支持。 | 升级软件后 |
| `compatibility` | `unknown_enum` | 必需语义枚举 token 未识别。 | 升级软件后 |
| `resource_limit` | `resource_limit` | 有界解析或事务规模超过明确上限。 | 缩小输入后 |
| `internal` | `invariant_violation` | 内部结果违反已声明不变量。 | 否，需诊断 |
| `internal` | `internal_error` | 未分类内部失败。 | 依据诊断决定 |

`no_change` 是成功结果状态，不是错误码。媒体的 `unsupported_media`、`corrupt_media` 等错误仍由媒体领域产生，可通过同一 ErrorInfo envelope 传递，但不在本文重定义。

## 9. 逻辑 DTO 与兼容规则

### 9.1 编码中立

本文描述逻辑 DTO，不确认 JSON、CBOR、Protocol Buffers 或其他编码。任何编码适配必须满足：

- `Int64`/`UInt64` 精确往返，不得经过只有 53 位整数精度的浮点通道。
- UTF-8 字符串必须验证编码；ID 进一步受 ASCII 规则约束。
- 数组顺序按字段定义保留；无序 map 在计算摘要前必须采用版本化规范顺序。
- 解码必须先检查长度、计数、整数范围、schema 和资源上限，再构造领域对象。

### 9.2 Schema envelope

所有公共 DTO 必须包含：

```text
schemaVersion: 1
requiredFeatures: ordered unique list<ASCII token> // 可为空
extensions: map<namespaced ASCII key, VersionedOpaqueObject> // 可为空
```

`coreContractVersion` 只在顶层快照、契约交接或诊断清单中出现，不代替每个 DTO 的 `schemaVersion`。

### 9.3 Reader 规则

schema 1 reader 必须：

1. schemaVersion 不是 1 时返回 `unsupported_schema`，不得猜测或静默降级。
2. 缺少必需字段、字段重复、类型不匹配或整数越界时返回稳定 validation/compatibility error。
3. `requiredFeatures` 中含未知 token 时返回 `unsupported_feature`。
4. 对必需语义字段中的未知 enum 返回 `unknown_enum`，不得映射为默认值。
5. 拒绝未知顶层字段；未来不破坏 schema 的可选扩展必须放入 `extensions`。
6. 对未知 extension namespace 原样保留并忽略其语义；只要它未列入 requiredFeatures，就不得阻止读取。
7. 在执行领域操作前把逻辑 DTO 完整转换并校验为领域值对象，禁止“部分有效对象”。

### 9.4 Writer 与 round-trip 规则

- schema 1 writer 只能写 schema 1 已定义字段和 namespaced extensions，不得自创顶层字段。
- 读—改—写已知对象时，未知但允许的 extension 必须原样保留；显式删除 extension 必须由拥有者命令完成。
- 新增必需语义、改变字段含义/单位/范围、改变排序或事务行为必须发布新的 schemaVersion；不得靠同版本 silently reinterpret。
- 新增可选、可忽略、可原样保存的数据可以使用 extensions；若核心行为依赖它，必须同时加入 requiredFeatures。
- 规范等价请求的幂等摘要必须包含所有已知语义字段、requiredFeatures 及声明参与语义的 extensions，并排除字段排列、map 迭代顺序和非语义诊断数据。

### 9.5 VersionedOpaqueObject

```text
VersionedOpaqueObject {
  owner: namespaced ASCII token
  schemaVersion: positive integer
  requiredFeatures: ordered unique list<ASCII token>
  payload: bounded logical object
}
```

核心只校验 envelope、大小和所有权，不解释其他领域 payload。payload 需要影响核心排序、锁定或事务前置条件时，必须先升级核心契约，不能通过 opaque 对象偷偷改变核心语义。

## 10. 规范测试向量

### 10.1 约定

- 所有向量以空白、有效的新项目开始，除非 setup 另有说明。
- `S0` 表示初始快照：revision 0，轨道 `track-0`（orderIndex 0），无事件。
- `manual-1` 是有效人工事件：track-0、timeNs 100、durationNs 0、kind manual、origin user、strengthPpm 500000、confidence 为空、locked false、userEdited true。
- 预期 error 必须同时匹配 category/code，revision 与状态保持要求也必须断言。
- 测试框架尚受 D-003 约束；向量本身与框架无关。

### 10.2 TimeNs 与区间

| ID | 输入 | 预期 |
|---|---|---|
| `TV-TIME-001` | `scaleTicks(0,{1,1},nearest_ties_to_even)` | `0`。 |
| `TV-TIME-002` | `scaleTicks(30,{1,30},nearest_ties_to_even)` | `1000000000`。 |
| `TV-TIME-003` | `scaleTicks(48000,{1,48000},nearest_ties_to_even)` | `1000000000`。 |
| `TV-TIME-004` | `scaleTicks(1,{1001,30000},nearest_ties_to_even)` | `33366667`。 |
| `TV-TIME-005` | `scaleTicks(1,{1,2000000000},floor/ceil/toward_zero/nearest_ties_to_even)` | 分别为 `0/1/0/0`。 |
| `TV-TIME-006` | `scaleTicks(3,{1,2000000000},floor/ceil/toward_zero/nearest_ties_to_even)` | 分别为 `1/2/1/2`。 |
| `TV-TIME-007` | `scaleTicks(-1,{1,2000000000},floor/ceil/toward_zero/nearest_ties_to_even)` | 分别为 `-1/0/0/0`。 |
| `TV-TIME-008` | `scaleTicks(-3,{1,2000000000},floor/ceil/toward_zero/nearest_ties_to_even)` | 分别为 `-2/-1/-1/-2`。 |
| `TV-TIME-009` | `scaleTicks(1,{0,1},floor)` 或 denominator 为 0/负数 | `validation/invalid_time_base`。 |
| `TV-TIME-010` | `checkedAdd(INT64_MAX,1)`；`checkedSubtract(INT64_MIN,1)` | 均为 `validation/time_overflow`，不得环绕。 |
| `TV-TIME-011` | `makeRange(0,0)` | 有效 `[0,0)`；作为空范围，不等同于点事件对象。 |
| `TV-TIME-012` | `makeRange(10,5)` | 有效 `[10,15)`，包含 10、不包含 15。 |
| `TV-TIME-013` | `[0,10)` 与 `[10,20)` | 不重叠。 |
| `TV-TIME-014` | `makeRange(INT64_MAX,1)` | `validation/time_overflow`。 |
| `TV-TIME-015` | 构造已提交事件 `timeNs=-1` | `validation/invalid_event`；通用 TimeNs 可为负但 ProjectTimeNs 不可。 |

### 10.3 轨道、事件与排序

| ID | 输入 | 预期 |
|---|---|---|
| `TV-EVENT-001` | S0 添加 `manual-1` | committed，revision 1，事件字段原样保留。 |
| `TV-EVENT-002` | 两个轨道使用相同 TrackId | `validation/duplicate_track_id`，快照不发布。 |
| `TV-EVENT-003` | 两个事件使用相同 EventId | `validation/duplicate_event_id`，快照不发布。 |
| `TV-EVENT-004` | 事件引用 `track-missing` | `validation/track_not_found`。 |
| `TV-EVENT-005` | `strengthPpm=1000001` 或 `confidencePpm=1000001` | `validation/invalid_event`。 |
| `TV-EVENT-006` | origin=user 且 confidencePpm 存在 | `validation/invalid_event`。 |
| `TV-EVENT-007` | origin=analysis 但缺少 analysisRevision/inputFingerprint/parametersDigest 任一项 | `validation/invalid_event`。 |
| `TV-EVENT-008` | kind=`future_kind` | `compatibility/unknown_enum`，不得映射为 manual。 |
| `TV-EVENT-009` | event timeNs=INT64_MAX 且 durationNs=1 | `validation/time_overflow`。 |
| `TV-EVENT-010` | tracks：`track-0` order 0、`track-1` order 1；同在 timeNs 100 的事件按输入顺序 `event-b@track-0,event-a@track-1,event-a0@track-0` | 输出固定为 `event-a0@track-0,event-b@track-0,event-a@track-1`。 |
| `TV-EVENT-011` | 将 TV-EVENT-010 输入任意排列重复 100 次 | 每次输出顺序与规范序列相同。 |
| `TV-EVENT-012` | 删除含事件轨道，policy=`reject_non_empty` | `validation/track_not_empty`，轨道和事件均保留。 |
| `TV-EVENT-013` | 删除含事件轨道，policy=`delete_events` | 单事务删除轨道及其事件；一次 undo 全部恢复。 |
| `TV-EVENT-014` | 两个轨道的 orderIndex 为 0 和 2，缺少 1 | `validation/invalid_track`，快照不发布。 |

### 10.4 修订、事务与取消

| ID | Setup / 输入 | 预期 |
|---|---|---|
| `TV-TXN-001` | S0，base 0，AddEvent(manual-1) | committed；before 0、after 1；一条历史。 |
| `TV-TXN-002` | TV-TXN-001 后，再以 base 0 提交新事件 | `conflict/stale_revision`；保持 revision 1 和原事件集。 |
| `TV-TXN-003` | revision 1，对 manual-1 Patch 为与当前完全相同值 | no_change；before/after 都为 1；历史数不变。 |
| `TV-TXN-004` | S0 单事务：先 AddEvent(manual-1)，再 MoveEvent(event-missing) | `validation/event_not_found`；revision 保持 0；manual-1 不得部分加入。 |
| `TV-TXN-005` | 有效快照中含两个事件；单事务批量偏移后其中一个端点溢出 | `validation/time_overflow`；两个事件均保持原位。 |
| `TV-TXN-006` | 提交点前取消 | cancelled + `cancelled/transaction_cancelled`；revision 和历史不变。 |
| `TV-TXN-007` | 原子发布完成后才到达取消 | committed；不得返回 cancelled；新 revision 可读取。 |
| `TV-TXN-008` | 相同 TransactionId 与规范等价请求重放 | 返回首次结果；不增加 revision、不重复历史。 |
| `TV-TXN-009` | 相同 TransactionId、不同事件内容 | `conflict/idempotency_conflict`；状态不变。 |
| `TV-TXN-010` | 当前 revision=UINT64_MAX，提交非空变化 | `conflict/revision_exhausted`；状态不变。 |
| `TV-TXN-011` | revision 1 的锁定事件，origin=analysis 直接 Move/Patch/Remove | `conflict/locked_event`；状态不变。 |
| `TV-TXN-012` | revision 1 的锁定事件，origin=user，操作序列先 SetLocked(false) 再 Move | committed；两个变化同一 revision 原子生效。 |

### 10.5 重分析保护与来源追踪

| ID | Setup / 输入 | 预期 |
|---|---|---|
| `TV-MERGE-001` | 时间线含 manual 事件；候选位于同一时间；参数指定受保护冲突时保留现有事件 | manual 事件完整保留，候选被抑制，主原因 `protected_manual`；不得替换 manual ID。 |
| `TV-MERGE-002` | 时间线含 locked 自动事件；候选落入合并窗；参数指定受保护冲突时保留现有事件 | locked 事件所有规范字段保持，候选被抑制，主原因 `protected_locked`。 |
| `TV-MERGE-003` | 同一候选集、输入指纹、算法版本、参数摘要和种子，以不同输入排列执行 | 产生规范等价事件集合和相同稳定排序。 |
| `TV-MERGE-004` | 候选集中一项 `strengthPpm=1000001`，融合已在临时状态处理过其他候选 | `validation/invalid_event`；当前 timelineRevision 和事件集合完全不变，无历史条目。 |
| `TV-MERGE-005` | 候选引用 revision A，但提交时 timeline 已从 N 变为 N+1 | `conflict/stale_revision`，过期结果不得覆盖。 |
| `TV-MERGE-006` | 成功接受一次分析候选集 | 新事件 source 能追踪 analysisRevision、producerVersion、inputFingerprint、parametersDigest 和实际 candidateIds。 |
| `TV-MERGE-007` | 两组不等价候选宣称同一 AnalysisRevision | `conflict/analysis_revision_conflict`，第二组不得接纳。 |

### 10.6 Undo、redo 与保存点

| ID | Setup / 输入 | 预期 |
|---|---|---|
| `TV-HISTORY-001` | rev0 添加 manual-1 得 rev1；undo(rev1) | committed 到 rev2；事件为空；条目进入 redo。 |
| `TV-HISTORY-002` | TV-HISTORY-001 后 redo(rev2) | committed 到 rev3；状态与 rev1 语义等价，但 revision 为 3。 |
| `TV-HISTORY-003` | rev1 undo 到 rev2，随后提交新 AddEvent 得 rev3，再 redo | redo 栈已清空；返回 no_change。 |
| `TV-HISTORY-004` | 空历史调用 undo 或 redo | no_change；revision 不变。 |
| `TV-HISTORY-005` | 批量偏移 20 个事件成功后一次 undo | 20 个事件全部恢复，且只增加一个 revision。 |
| `TV-HISTORY-006` | 分析合并一次提交后 undo | 整次合并恢复到提交前状态；不得逐事件分步撤销。 |
| `TV-HISTORY-007` | expectedCurrentRevision 过期 | `conflict/stale_revision`；状态和两个历史栈不变。 |
| `TV-HISTORY-008` | 保存 rev1 语义状态 S；undo 到 rev2 再 redo 到 rev3，内容回到 S | dirty=false，不能因 revision 3 != 1 判定 dirty。 |
| `TV-HISTORY-009` | 保存后修改成 T，再 undo 回 S | dirty=false。 |
| `TV-HISTORY-010` | 保存并关闭，再重新打开 | 当前状态为 clean；undo/redo 栈为空；revision 从文件值继续。 |
| `TV-HISTORY-011` | 两笔相邻 user 事务具有相同 coalescingKey 和实体集，且无中间事务/保存点 | 两笔形成一个历史单元；一次 undo 恢复第一笔前状态。 |
| `TV-HISTORY-012` | 相同 coalescingKey 之间插入其他事务或保存点 | 不得合并。 |

### 10.7 ErrorInfo 与 DTO 兼容

| ID | 输入 | 预期 |
|---|---|---|
| `TV-DTO-001` | schemaVersion=1 的最小有效 Track/Event/Snapshot | 完整读取并规范化，写回后语义等价。 |
| `TV-DTO-002` | schemaVersion=2 | `compatibility/unsupported_schema`，不得按 schema 1 猜测。 |
| `TV-DTO-003` | 缺少必需字段、重复字段或字段类型错误 | `validation/invalid_dto`，不产生部分领域对象。 |
| `TV-DTO-004` | 未知顶层字段 `futureField` | `validation/invalid_dto`；未来可选数据必须放入 extensions。 |
| `TV-DTO-005` | `extensions["vendor.future"]` 未知，且不在 requiredFeatures | reader 忽略语义但原样保留；round-trip 后仍存在。 |
| `TV-DTO-006` | requiredFeatures 含 `future.required` | `compatibility/unsupported_feature`。 |
| `TV-DTO-007` | 必需 enum 是未知 token | `compatibility/unknown_enum`。 |
| `TV-DTO-008` | TimeNs=`9223372036854775807` 精确往返 | 数值逐位相同；任何经浮点导致精度损失的适配失败测试。 |
| `TV-DTO-009` | EventId 含空格、斜杠、非 ASCII 或长度 129 | `validation/invalid_identifier`。 |
| `TV-DTO-010` | ErrorInfo 缺 diagnosticId 或 code | `validation/invalid_dto`；不得只返回自由文本错误。 |
| `TV-DTO-011` | 同一失败跨两层包装 cause | 顶层与根因使用同一 diagnosticId，稳定根因 code 可追踪。 |
| `TV-DTO-012` | 未知运行时异常 | `internal/internal_error`，diagnosticId 非空；不得伪装成用户输入错误。 |

## 11. 消费者交接

### 11.1 T-017 媒体契约

T-017 应直接消费：

- `TimeNs`/`ProjectTimeNs` 的整数和范围规则；
- `TimeBase` 与四种显式舍入模式；
- TV-TIME-001～015，特别是负值、半值、VFR 常用分数和溢出；
- 媒体层输出 ProjectTimeNs 前必须完成 PTS/start time/VFR 解释，核心不接收未解释 PTS。

T-017 可以定义 `PtsMapping`、采样索引和 seek 规则，但不得建立另一种项目规范时间或改变本文舍入结果。

### 11.2 T-020 测试策略

T-020 应把全部 `TV-*` ID 原样纳入追踪，允许增加独立测试但不得降低预期。框架未确认前可以先以表格或无依赖 harness 表达，最终自动化应报告向量 ID、契约版本、实际值和差异。

### 11.3 后续 UI、算法、Worker 与存储

- UI 只通过快照、命令与 TransactionResult 工作，不直接修改领域容器。
- 算法只产生 AnalysisCandidate；融合与锁定保护归核心。
- Worker/IPC 使用同一 ErrorInfo、修订和 DTO envelope；过期结果必须拒绝。
- 存储精确保存 Int64/UInt64、schemaVersion、事件来源和当前 revision；undo/redo 历史不持久化。

## 12. 需求追踪与自查

### 12.1 追踪矩阵

| 输入要求 | 本契约位置 | 证据 |
|---|---|---|
| C-01 规范时间、换算与舍入 | 第 3 节 | TV-TIME-001～015 |
| C-02 事件/轨道/修订/锁定 | 第 4～5 节 | TV-EVENT、TV-MERGE |
| C-03 命令、事务、撤销/重做 | 第 6～7 节 | TV-TXN、TV-HISTORY |
| TR-A-002 无 GUI C/C++ 核心 | 第 1 节边界 | 无 QML/Qt/FFmpeg 稳定类型 |
| TR-A-003 单向依赖与消息结果 | 第 1、6、11 节 | 快照/命令/结果边界 |
| TR-A-004 跨边界值对象与版本 | 第 2、9 节 | 强类型、schema、扩展规则 |
| TR-A-005 所有权与线程边界 | 第 5～7 节 | 不可变快照、无裸引用、串行提交 |
| TR-F-004 编辑、偏移、锁定、撤销 | 第 6～7 节 | TV-TXN、TV-HISTORY |
| TR-F-006 版本化确定性融合 | 第 4.5、6.5 节 | TV-MERGE-003/006/007 |
| TR-F-007 人工和锁定保护 | 第 4.4、6.5 节 | TV-TXN-011/012、TV-MERGE-001/002 |
| TR-Q-001 统一时间 | 第 3、5 节 | 时间向量和固定 timelineRevision |
| TR-Q-002 确定性 | 第 4.6、6.5 节 | 排序与合并等价向量 |
| TR-Q-009 可维护性 | 第 8～9 节 | 稳定错误码、schema/feature/extension |
| TR-Q-010 可测试性 | 第 10 节 | 72 个稳定向量 ID |

### 12.2 自查结果

- **范围**：已覆盖用户要求的 TimeNs、事件/轨道、修订、事务、撤销重做、错误分类和 DTO 兼容规则；未进入 T-015 实现、媒体 PTS 解释、Qt/QML、IPC 或存储编码。
- **时间正确性**：明确 int64 纳秒、非负项目时间、半开区间、四种舍入、无限精度概念中间值和溢出拒绝；负数与 tie-to-even 有精确预期。
- **确定性**：事件全序只依赖规范字段；输入排列、容器顺序和日志不影响结果；分析来源和参数可追踪。
- **原子性**：事务、批量操作、分析合并、undo/redo 均定义全成或全败；提交点前后取消语义无歧义。
- **保护性**：人工和锁定事件在重分析中保持，直接非法修改返回稳定冲突错误，抑制候选具有原因报告。
- **兼容性**：schema、contract、analysis、timeline revision 分离；未知 schema/feature/enum 显式拒绝，未知可选扩展可保留。
- **消费者可用性**：测试向量具有稳定 ID、具体输入和可判定预期；T-017 与 T-020 的责任边界和消费清单已列出。
- **未确认项隔离**：D-003 仍为 proposed；本文没有确认 C++20、GoogleTest、Qt 本地 IPC、JSON 或具体依赖版本。
- **静态与算术检查**：脚本统计 72 个向量行、72 个唯一 ID，分组为 TIME 15、EVENT 14、TXN 12、MERGE 7、HISTORY 12、DTO 12；本地 Markdown 引用缺失数和尾随空白数均为 0。使用 bundled Python 标准库 `Fraction` 独立复算 TV-TIME-002～008 的整数、分数、正负 half-tie 舍入，结果为 `TIME_VECTOR_ARITHMETIC=PASS`。
- **验证性质**：本轮完成文档级规范检查、追踪检查和向量预期复算；仓库尚无 T-013 业务构建骨架，因此没有执行 C++ 编译或自动化测试，也未把设计推演表述为运行证据。

### 12.3 后续变更规则

消费者发现歧义或反例时，应引用契约版本与 `TV-*` ID 提交。0.x 修订必须保留版本记录；改变字段单位、排序、锁定保护、事务原子性或兼容行为属于破坏性变化，必须升级逻辑 schema 并说明迁移。T-015 只能实现本文契约或先完成受控修订，不得用代码既成事实反向覆盖契约。
