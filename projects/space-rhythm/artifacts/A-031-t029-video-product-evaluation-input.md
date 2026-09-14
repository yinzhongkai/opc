# T-029 视频产品评估输入与用户确认清单

- 项目：space-rhythm
- 成果 ID：A-031
- 负责人：product-manager-01
- 关联任务：T-039；供 T-029、T-022 使用
- 版本：0.1
- 更新日期：2026-09-14
- 状态：draft
- 适用范围：第一阶段“视频到可编辑节奏音轨”主流程的代表产品数据、人工标注、自然度主观评估及经典视频算法效果/性能门禁输入；不执行算法评估，不批准学习模型或生产发布。
- 来源及输入版本：本会话用户于 2026-09-14 的直接授权；D-001～D-008 confirmed；A-002 0.2 approved；A-011 0.1、A-016 0.1、A-028 0.2、A-029 0.1、A-030 0.1；T-029 blocked；H-001 项目经理整理结果。
- 批准依据：尚无。标注、rubric、评分与播放协议由 product-manager-01 按本次授权形成；实际素材、基准硬件和数值阈值等待 D-009 确认。
- 协议版本：`productEvaluationInputVersion=0.1.0`
- 版本记录：2026-09-14，0.1，首次形成代表产品集要求、标注协议、自然度 rubric、评分尺度、播放条件、门槛结构和最小确认清单。

## 1. 输入状态与不可替代边界

本文件把产品侧能够先固定的方法与必须由用户确认的事实分开：

| 输入 | 本版本状态 | 可供 T-029 使用的范围 |
|---|---|---|
| 代表视频集要求与 manifest 字段 | `product-defined` | 可据此收集和审计真实产品视频 |
| 人工标注、复核与修正计量协议 | `product-defined` | 可据此制作标注工具/文件并培训标注者 |
| “卡点自然” rubric、1～5 尺度和播放条件 | `product-defined` | 可据此构造盲评流程并保存原始评分 |
| 实际代表产品视频、许可/hash、配额与分区 | `missing-confirmed-input` | D-009 确认并冻结前不得运行产品效果门禁 |
| 基准 Windows 硬件 | `missing-confirmed-input` | D-009 确认前性能数字只能标 `measured` |
| 效果、自然度和性能数值阈值 | `missing-confirmed-input` | D-009 确认前所有对应 gate 为 `not-evaluated` |

T-028 的 10 项 CC0 合成 golden 用于契约、确定性、时间映射和回归测试，不属于目标用户的真实创作素材，禁止计入产品数据规模、自然度评分、人工修正量或产品通过率。Wine 8.0 隔离环境的性能数值也不是已承诺 Windows 基准机结果。

产品视频可以保存在用户控制的外部只读目录。项目证据只登记稳定脱敏位置、内容 SHA-256、权限、标注版本和分区，不复制私人素材，也不把内部产品集转成可分发 golden。

## 2. 代表产品视频集要求

### 2.1 目标与分层

产品集必须来自第一阶段目标用户会实际制作的短视频或独立音视频内容，并能够回答：经典算法产生的镜头、运动和动作候选，是否能以合理人工修正量形成自然的节奏音轨。素材不能只挑算法容易命中的片段。

数据分为三个互斥分区：

- `calibration`：标注者理解协议和评审者熟悉评分尺度；不得用于算法调参或最终门禁。
- `tuning`：经典算法参数分析、失败定位和有限调参；可以查看标注。
- `final_evaluation`：参数、音色映射和播放条件冻结后才解封；不得反向用于调参。

同一原始视频、连续裁切片段、同一拍摄事件或只做转码/裁剪的近重复内容不得跨 `tuning` 与 `final_evaluation`，以避免内容泄漏。每次评估先冻结 manifest、媒体和标注 hash；发生任何变化须建立新数据集版本，旧结果不覆盖。

### 2.2 必须覆盖的切片

每个片段可以有多个标签。最终产品集必须覆盖全部下列切片；具体片段数、总时长和配额由 D-009 确认，任何切片没有达到已确认最低配额时，该切片及总 gate 均为 `not-evaluated`。

| 维度 | 必须覆盖的切片 | 主要产品风险 |
|---|---|---|
| 剪辑结构 | `fast_cut`、`slow_cut_or_transition`、`long_take_or_no_cut` | 漏切、闪光误切、长镜头过度打点 |
| 运动来源 | `global_camera_motion`、`local_subject_motion`、`mixed_global_local` | 运镜被误当动作、局部动作被全局运动淹没 |
| 动作语义 | `impact`、`stop`、`reversal`、`ambiguous_action` | 重音位置不对、模糊动作被高置信度输出 |
| 节奏形态 | `repetitive_motion`、`sparse_free_rhythm`、`tempo_or_energy_change` | 节奏单调、密度跳变、变化段不连贯 |
| 时间与速度 | `cfr`、`vfr`、`slow_motion` | 使用名义 fps 重建时间、慢动作峰值漂移 |
| 负例与质量 | `near_static`、`flash_or_exposure_change`、`motion_blur_or_occlusion`、`low_quality_compression` | 伪候选、低质量输入仍伪装高置信度 |
| 产品场景 | 用户确认的真实短视频/独立音视频内容类别 | 技术切片覆盖但不代表目标用户工作流 |

### 2.3 每个片段的 manifest 最小字段

| 字段 | 要求 |
|---|---|
| `datasetVersion`、`clipId` | 版本化数据集与稳定匿名片段 ID；不得使用会泄露私人路径的名称 |
| `readOnlyLocationToken` | 用户控制只读位置的脱敏标识；实际路径不进入公开证据 |
| `mediaSha256`、`byteLength` | 对实际媒体字节计算，分析前冻结 |
| `sourceAndUsagePermission` | 来源、权利人/提供人、内部评估或可再分发边界、确认日期 |
| `durationNs`、`geometry` | 真实时长、宽高、旋转和像素宽高比 |
| `frameRateMode` | `cfr|vfr`，同时记录探测到的时间基与帧率摘要 |
| `streamKey` | A-014/A-028 约定的稳定视频流键 |
| `contentCategories`、`qualityAttributes` | 使用第 2.2 节稳定 token，可多选 |
| `datasetPartition` | `calibration|tuning|final_evaluation` |
| `probeVersionAndDigest` | FFmpeg/探测器版本及规范探测摘要 hash |
| `privacyAndDistribution` | 是否含人脸、私人场所或敏感信息；证据能否公开 |

## 3. 人工事件标注与复核协议

### 3.1 角色与顺序

1. 标注负责人先冻结媒体 manifest，不运行当前算法结果给标注者看。
2. 初标者在静音视频上独立标注可见事件，不看算法候选、置信度或音色结果。
3. 不同人员担任复核者，逐项执行 `accept|modify|reject`，并检查明确负例区间。
4. 初标与复核不一致时保留双方原始版本，由指定裁决者依据本协议形成 `adjudicated` 版本；不得直接覆盖原始意见。
5. `adjudicated` 标注 hash 冻结后，算法才能在对应分区运行。`final_evaluation` 的标注在参数和映射冻结后才用于匹配与汇总。

正式结果必须记录匿名角色 ID、是否属于目标用户型创作者、培训/校准版本和利益冲突；算法实现者不能独自充当最终标注、复核和自然度评审三种角色。评审人数和目标用户构成由 D-009 与数值门槛一起确认。

### 3.2 事件语义

- `shot`：硬切标在新镜头首个显示帧；渐变以可复核的 `[timeNs,timeNs+durationNs)` 区间标注。闪光或曝光变化不是默认切镜。
- `motion_peak`：标在视觉运动强度局部峰值，附 `global|local|mixed|ambiguous`；它不自动表示应当配重音。
- `action_peak`：标在主体冲击、急停或反转最明确的可见时刻，附 `impact|stop|reversal|ambiguous`；不要求物体识别或动作名称。
- `negative_span`：显式标记某个时间区间内哪些 kind 不应出现，用于计算误报；近静止、闪光、恒速运镜和压缩噪声必须优先检查负例。

每个事件再标注节奏作用：`primary_accent|secondary_accent|do_not_accent|uncertain`。这用于自然度和人工修正分析，不改变 A-028 的三类算法 `kind`。`uncertain` 事件不进入强制命中门槛，但必须单独报告，不能静默丢弃。

### 3.3 标注字段与时间规则

每项至少包含 A-030 所需字段：`annotationId`、`clipId`、`kind`、`timeNs`、`durationNs`、`matchWindowBeforeNs`、`matchWindowAfterNs`、`annotationVersion`、`annotatorAnonymousId`、`reviewerAnonymousId`。另增加：

- `boundaryOrMotionOrActionClass`；
- `rhythmRole`；
- `uncertaintyReasonToken`；
- `reviewDecision` 与 `adjudicatorAnonymousId`；
- `sourceFrameTimeNs`、`sourceDecodeOrdinal`；
- `annotationProtocolVersion=0.1.0`。

所有时间来自媒体层真实 `timeNs`，标注工具吸附到实际显示帧并保存 `decodeOrdinal`；禁止用 `frameIndex/nominalFps` 反推。前后匹配窗口由画面本身的可定位精度确定并写理由，不得根据算法误差事后放宽。标注规范串按 A-028 的 kind/time/ID 规则排序并计算 SHA-256。

### 3.4 人工修正量

标注冻结后，由目标用户型评估者在正常产品时间线中修正经典算法结果，系统记录原始候选、最终事件和完整操作日志：

- `add`：最终保留的人工事件没有同 kind 可匹配候选；
- `delete`：算法候选在最终时间线被移除；
- `move`：匹配候选的最终 `timeNs` 发生变化，同时记录绝对移动纳秒；
- `reclassify`：最终 kind 与候选 kind 不同；
- `lock`：用户确认并锁定事件，单独计数，不当作错误修正。

按片段和每分钟分别报告新增、删除、移动、改类、锁定、总操作数、活跃编辑时间和移动绝对误差分布。多次拖动同一事件同时保留原始 UI 操作数和归一化后的最终一次 `move`，避免微调动作夸大或掩盖负担。

## 4. “卡点自然”rubric 与评分尺度

每个冻结试听版本对五个维度分别评分，并另给总体自然度和直接导出意愿。使用 1～5 的离散整数尺度，不使用半分；无法判断只能选 `N/A` 并填写稳定原因 token，不得把 `N/A` 当 0 或 3。

| 维度 | 1 分 | 3 分 | 5 分 |
|---|---|---|---|
| 时间贴合 `temporalAlignment` | 多数重音明显早于或晚于可见落点，形成音画错位 | 主要落点大致对齐，但存在数个可察觉偏移 | 重音与关键镜头/动作落点稳定贴合，没有有意义的错位 |
| 显著性匹配 `salienceAccentMatch` | 次要变化被重击，关键事件反而缺少重音 | 大部分重要事件权重合理，少数轻重关系不当 | 重音强弱持续反映画面事件的视觉显著性 |
| 密度与干扰 `densityAndExtraBeats` | 明显过密或过稀，多余拍/漏拍持续干扰观看 | 整体可用，但局部仍需删减或补点 | 密度与内容变化匹配，没有造成干扰的多余拍或明显漏拍 |
| 连续性 `continuity` | 节奏频繁无因跳变，段落之间断裂 | 基本连贯，个别转场或能量变化处理生硬 | 节奏在段落内稳定，并随画面能量变化自然过渡 |
| 编辑就绪度 `editReadiness` | 无法通过少量修正达到可用，需要重做 | 可作为草稿，但需要若干明确人工修正 | 可直接使用，或只需不影响结构的细微偏好调整 |

2 分表示介于 1 与 3 之间，4 分表示介于 3 与 5 之间。`overallNaturalness` 使用同一 1～5 尺度，回答“作为目标用户，我认为这条卡点节奏整体听起来有多自然”；它不由五个维度自动平均生成。`directExportReadiness` 另记录 `direct_export|minor_edit|major_edit|unusable`。

在 D-009 确认聚合和通过阈值前，必须保存逐评审者、逐片段、逐版本原始值，只能报告分布、中位数、四分位数和 `N/A` 比例，不能计算产品“通过率”。

## 5. 播放与评审条件

### 5.1 自然度盲评

- 每个片段生成 `classic` 和 `human_reference` 两个试听版本；后者来自同一真实产品视频的裁决后人工事件时间线，不是合成 golden。两者使用完全相同的固定音色组、事件到音色映射、混音、响度和导出链，只改变事件时间线。
- 隐藏算法名称、置信度、时间线、事件标记和文件名；版本顺序和片段顺序随机化，随机种子写入证据。评审者不得在评分前查看答案或互相讨论。
- 画面完整播放，音频开启，不允许拖动或逐帧查看。每个版本先播放一次，评审者可主动重播最多两次；实际播放次数必须记录。
- 同一评审会话使用同一设备、系统音量、应用音量、显示模式和环境。不得在两版本之间改变音量、耳机/扬声器或显示刷新设置。
- 评分紧随播放完成，不显示他人答案。校准片段只用于理解尺度，不计入正式结果。
- 每个版本分别填写五维评分、总体自然度、直接导出意愿和可选短原因；成对呈现时再记录 `A|B|tie` 偏好，偏好不覆盖单版本评分。

每次会话必须记录：应用/算法/参数/音色映射/数据集/rubric 版本及 hash，Windows edition/build，显示器刷新率与缩放，音频设备与驱动/采样率，系统和应用音量，是否耳机，房间环境，评审者匿名 ID、目标用户画像、随机种子、开始结束时间和中断。具体设备型号、评审人数和构成由 D-009 确认。

### 5.2 人工修正评估

人工修正与盲评分开进行。评估者看到正常产品工作区、时间线和事件来源，使用冻结的 `classic` 结果从头修到自己认为可直接导出的状态。开始前重置项目，禁止复制另一位评估者结果；结束时保存最终 revision、操作日志、活跃编辑时间和项目 hash。没有达到可直接导出时也必须保存当前事实并选择 `major_edit|unusable`，不得强迫完成。

## 6. 效果门槛结构

以下指标、比较符、单位和聚合方式已经固定，数值列必须由 D-009 确认。每个指标同时按总体、`kind` 和第 2.2 节关键 slice 报告；总体平均不能掩盖任一已确认必过切片。分母为零时写 `unavailable(reason=undefined_denominator)`，不得填 0 或 pass。

| 门槛 ID | 指标 | 比较符 | 单位/聚合 | 数值状态 |
|---|---|---|---|---|
| `E-SHOT-PRECISION` | shot precision | `>=` | overall + slice；micro 与 macro | `TBD_USER_CONFIRM` |
| `E-SHOT-RECALL` | shot recall | `>=` | overall + slice；micro 与 macro | `TBD_USER_CONFIRM` |
| `E-MOTION-PRECISION` | motion_peak precision | `>=` | overall + slice；micro 与 macro | `TBD_USER_CONFIRM` |
| `E-MOTION-RECALL` | motion_peak recall | `>=` | overall + slice；micro 与 macro | `TBD_USER_CONFIRM` |
| `E-ACTION-PRECISION` | action_peak precision | `>=` | overall + slice；micro 与 macro | `TBD_USER_CONFIRM` |
| `E-ACTION-RECALL` | action_peak recall | `>=` | overall + slice；micro 与 macro | `TBD_USER_CONFIRM` |
| `E-FP-MIN` | false positives per minute | `<=` | 每 kind、每 slice 的 P95 与总体 | `TBD_USER_CONFIRM` |
| `E-TIME-P95` | 匹配事件绝对时间误差 P95 | `<=` | ms；每 kind + overall | `TBD_USER_CONFIRM` |
| `E-GRADUAL-IOU` | 渐变转场区间 IoU | `>=` | 每片段 + macro median | `TBD_USER_CONFIRM` |
| `E-EDIT-ADD` | 人工新增事件数 | `<=` | 次/分钟；每 slice P95 + overall | `TBD_USER_CONFIRM` |
| `E-EDIT-DELETE` | 人工删除事件数 | `<=` | 次/分钟；每 slice P95 + overall | `TBD_USER_CONFIRM` |
| `E-EDIT-MOVE` | 人工移动事件数及移动量 | `<=` | 次/分钟 + 绝对 ms P95 | `TBD_USER_CONFIRM` |
| `E-EDIT-RECLASSIFY` | 人工改类事件数 | `<=` | 次/分钟；每 slice P95 + overall | `TBD_USER_CONFIRM` |
| `E-NATURALNESS` | `overallNaturalness` | `>=` | 逐片原始分布 + overall median；低分率单列 | `TBD_USER_CONFIRM` |
| `E-DIRECT-EXPORT` | `direct_export|minor_edit` 占比 | `>=` | overall + 关键 slice | `TBD_USER_CONFIRM` |
| `E-PAIR-PREFERENCE` | classic 相对 human_reference 的偏好 | `>=` | win/tie/loss，tie 规则随阈值确认 | `TBD_USER_CONFIRM` |

F1 作为 precision/recall 的派生诊断同时报告，但不允许只用一个 F1 门槛掩盖高误报或高漏报。置信度分桶只做校准观测，`confidencePpm` 不解释为自然度概率。

## 7. 性能门槛结构与模型门禁

性能运行遵循 A-016：完整场景至少一次预热、五次记录；保存全部原始样本并报告中位数、P95、最小/最大和峰值。只有硬件、Windows、构建、线程、代理/采样、缓存冷热和输入 hash 相同才允许比较。

| 门槛 ID | 指标 | 比较符 | 单位/聚合 | 数值状态 |
|---|---|---|---|---|
| `P-ANALYSIS-RTF` | 分析 real-time factor | `<=` | wall seconds / media seconds；P95 | `TBD_USER_CONFIRM` |
| `P-ANALYZED-FPS` | analyzed fps | `>=` | frames/s；P05 与 median | `TBD_USER_CONFIRM` |
| `P-WALL-P95` | 典型/最大素材 wall time | `<=` | seconds；P95 | `TBD_USER_CONFIRM` |
| `P-PRIVATE-BYTES` | 峰值 private bytes | `<=` | MiB；最大值 | `TBD_USER_CONFIRM` |
| `P-WORKING-BYTES` | 峰值 working bytes | `<=` | MiB；最大值 | `TBD_USER_CONFIRM` |
| `P-THREADS` | 峰值线程数 | `<=` | count；最大值 | `TBD_USER_CONFIRM` |
| `P-CANCEL-P95` | 取消请求到稳定 cancelled | `<=` | ms；P95 | `TBD_USER_CONFIRM` |

经典算法只有在冻结的 `final_evaluation` 上实际违反至少一个已确认效果门槛、且已排除输入质量、标注分歧、媒体时间映射或错误基准环境后，才满足“可以提出可选模型方案”的必要条件。即使满足，也只允许提交收益、性能、许可、CPU/GPU、包体和部署影响比较；引入 ONNX Runtime 或模型文件仍需新决定。

## 8. 用户最小确认清单

为解除 T-029 的 `missing_confirmed_input`，用户只需提供下列三组事实；未列的协议细节按本文件 0.1 执行。建议按原字段直接回复，未知项不要填默认值。

### C-1 真实代表产品视频

- 产品视频只读目录或交付方式：`<待确认>`
- 每个视频的来源/使用权限是否允许本项目内部评估：`<逐项确认>`
- `calibration / tuning / final_evaluation` 的实际片段清单：`<待提供>`
- 每个分区的片段数、总时长及第 2.2 节各切片最低配额：`<待确认>`
- 是否允许保存脱敏位置、SHA-256、探测摘要、标注和评分证据：`<是/否及边界>`

提供后由执行成员计算实际媒体 hash；用户不需要手工计算，但必须确认这些实际文件就是获准评估的版本。不得以 T-028 合成 golden、`package/` 或未知许可下载内容代替。

### C-2 基准 Windows 硬件

- 基准 ID 与实际可用机器：`<待确认>`
- Windows edition/build：`<待确认>`
- CPU、物理/逻辑核心数：`<待确认>`
- 内存容量：`<待确认>`
- GPU 与驱动版本：`<待确认；无独显也需明确>`
- 存储类型与可用空间：`<待确认>`
- 电源方案：`<待确认>`
- 评估时的线程上限、代理尺寸/采样策略、冷/热缓存规则：`<待确认>`
- 典型素材和最大素材的时长、分辨率、帧率/VFR边界：`<待确认>`

本机受限探测和 Wine 容器不得替代上述实际 Windows 基准元组。

### C-3 数值阈值

- 第 6 节所有 `E-*` 门槛的数值、必过 slice、评审人数/目标用户构成、缺失票和 tie 处理：`<逐项确认>`
- 第 7 节所有 `P-*` 门槛的数值，以及典型/最大素材分别适用的预算：`<逐项确认>`
- 总 gate 规则：是否要求全部必选 `E-*`、`P-*` 同时满足，以及允许的 `unavailable` 项：`<待确认>`

确认回复必须保留单位和比较方向。例如只写“自然度 4”仍不完整，至少应说明是 `overall median >= 4/5`、适用哪些 slice、低分是否另设上限及需要多少独立目标用户型评审者。

## 9. 交付给 T-029 的证据包

D-009 确认后，T-029 执行人应冻结并引用：

1. 本文件版本与 SHA-256；
2. 产品数据 manifest、媒体 hash、权限边界和分区 hash；
3. 原始/复核/裁决标注文件及各自 hash；
4. rubric、评分尺度、播放条件、音色映射和随机化 seed；
5. 基准硬件与软件环境签名；
6. 已确认的 `E-*`、`P-*` 数值门槛和来源；
7. classic 的算法/参数/build hash 与逐次原始结果；
8. 人工修正日志、逐评审者原始评分和汇总脚本版本。

任何缺项都必须保留为 `missing_confirmed_input`、`measured` 或 `not-evaluated`，不得使用合成 golden、建议值或当前实现观测补齐后宣称产品效果通过。
