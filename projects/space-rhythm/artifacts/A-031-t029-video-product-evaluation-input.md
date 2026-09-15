# T-029 视频产品评估输入与用户确认清单

- 项目：space-rhythm
- 成果 ID：A-031
- 负责人：product-manager-01
- 关联任务：T-039；供 T-029、T-022 使用
- 版本：0.5
- 更新日期：2026-09-15
- 状态：approved
- 适用范围：第一阶段“视频到可编辑节奏音轨”主流程的代表产品数据、人工标注、自然度主观评估及经典视频算法效果/性能门禁输入；不执行算法评估，不批准学习模型或生产发布。
- 来源及输入版本：本会话用户于 2026-09-14～2026-09-15 的直接授权与逐项确认；D-001～D-014 confirmed；A-002 0.2 approved；A-011 0.1、A-016 0.1、A-028 0.2、A-029 0.1、A-030 0.3；H-013、H-017、`product-dataset-manifest-v2.json`、`execution-readiness-v3.json`。
- 批准依据：0.3 由 D-009、D-010 confirmed 批准；0.4 由 D-011 confirmed 批准独立真实 VFR 技术集和双 gate。0.5 依据用户于 2026-09-15 确认的 D-014，将本阶段主观评审改为 `personal-single-user-acceptance`；只替换多人角色、评分份数和独立复核/裁决要求，不修改真实 VFR、E-*/P-* 数值或 Windows 基准门禁。
- 协议版本：`productEvaluationInputVersion=0.4.0`
- 版本记录：2026-09-14，0.2，按 D-009 写入已确认的 B 站来源清单与分区、基准机实测元组、运行条件、逐项门槛和总 gate 规则；0.1 为首次方法稿。2026-09-14，0.3，按 H-013 的实际获取/probe 审计，在同类别和分区内替换 4 个超窗来源，补足声明层 `slow_motion` 配额并指定 3 个 VFR 探测目标；同日由 D-010 确认。2026-09-14，0.4，根据 execution-readiness-v3 已证明三个目标平台流均为 CFR 的事实，保留已确认的 40 条 B 站产品主集，另建真实 VFR 原始素材技术鲁棒性集；同日由 D-011 确认。2026-09-15，0.5，按 D-014 将五人独立评审改为用户本人 `USER-01` 的个人单用户验收，固定人工参考与盲评隔离、逐片完整性、单用户聚合和不可外推标记；同日由用户授权落实。

## 1. 输入状态与不可替代边界

本文件把产品侧能够先固定的方法与必须由用户确认的事实分开：

| 输入 | 本版本状态 | 可供 T-029 使用的范围 |
|---|---|---|
| 代表视频集要求与 manifest 字段 | `product-defined` | 可据此收集和审计真实产品视频 |
| 个人单用户人工参考与修正计量协议 | `confirmed-protocol` | D-014 固定 `USER-01`，取消独立复核/裁决前置；可据此制作工具和记录文件 |
| “卡点自然” rubric、1～5 尺度和播放条件 | `product-defined` | 可据此构造盲评流程并保存原始评分 |
| 个人单用户评分完整性 | `confirmed-missing-actual-records` | 主集和 VFR 技术集每条 final 均须 `USER-01` 的 1 份有效评分且不允许缺失；当前尚未执行 |
| 40 条 B 站代表产品主集 | `confirmed-and-probed` | D-010 的 4 个替换来源及原 36 条均已获取，结构配额通过；实际为 CFR 40、VFR 0，不能承担 VFR gate |
| 独立真实 VFR 原始素材技术集 | `confirmed-missing-actual-media` | 第 8.2 节的准入、配额、分区和证据已由 D-011 确认；尚无实际原始素材/hash/probe，`vfrRobustnessGate=not-evaluated` |
| 基准 Windows 硬件与运行条件 | `confirmed` | 按第 8.3 节复核实际测量签名 |
| 效果、自然度和性能数值阈值 | `confirmed` | 按第 6～8 节判定；未运行 T-029 前仍为 `not-evaluated` |

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

### 2.2 必须覆盖的切片与数据集归属

每个片段可以有多个标签。D-009 已确认全部切片均须验证，任何切片没有达到已确认最低配额时，该切片及总 gate 均为 `not-evaluated`。D-011 确认的 0.4 只改变切片由哪套真实数据承载，不删除切片、不降低数量或阈值：40 条 B 站主集承载产品类别、CFR、慢动作及视觉/语义切片；另建的真实 VFR 原始素材技术集承载 VFR 切片。两套数据各自冻结 manifest/hash，结果不得混为一个平均数。

| 维度 | 必须覆盖的切片 | 主要产品风险 |
|---|---|---|
| 剪辑结构 | `fast_cut`、`slow_cut_or_transition`、`long_take_or_no_cut` | 漏切、闪光误切、长镜头过度打点 |
| 运动来源 | `global_camera_motion`、`local_subject_motion`、`mixed_global_local` | 运镜被误当动作、局部动作被全局运动淹没 |
| 动作语义 | `impact`、`stop`、`reversal`、`ambiguous_action` | 重音位置不对、模糊动作被高置信度输出 |
| 节奏形态 | `repetitive_motion`、`sparse_free_rhythm`、`tempo_or_energy_change` | 节奏单调、密度跳变、变化段不连贯 |
| 时间与速度 | `cfr`、`vfr`、`slow_motion` | 使用名义 fps 重建时间、慢动作峰值漂移 |
| 负例与质量 | `near_static`、`flash_or_exposure_change`、`motion_blur_or_occlusion`、`low_quality_compression` | 伪候选、低质量输入仍伪装高置信度 |
| 产品场景 | 用户确认的真实短视频/独立音视频内容类别 | 技术切片覆盖但不代表目标用户工作流 |

D-011 已确认以上拆分。非 VFR 的 18 个视觉/语义切片仍须在 40 条产品主集中各至少 3 条且至少 1 条属于主集 `final_evaluation`；VFR 技术集仍须实际 `vfr>=3` 且至少 1 条属于该技术集的 `final_evaluation`。主集通过不能抵消 VFR 技术集失败，反之亦然。当前独立技术集尚无实际素材，故 `vfrRobustnessGate=not-evaluated(reason=missing_actual_media)`，T-029 overall 不得为 pass。

### 2.3 每个片段的 manifest 最小字段

| 字段 | 要求 |
|---|---|
| `datasetVersion`、`clipId` | 版本化数据集与稳定匿名片段 ID；不得使用会泄露私人路径的名称 |
| `readOnlyLocationToken` | 用户控制只读位置的脱敏标识；实际路径不进入公开证据 |
| `mediaSha256`、`byteLength` | 对实际媒体字节计算，分析前冻结 |
| `sourceAndUsagePermission` | 来源、权利人/提供人、内部评估或可再分发边界、确认日期 |
| `durationNs`、`geometry` | 真实时长、宽高、旋转和像素宽高比 |
| `frameRateMode` | `cfr|vfr`，同时记录探测到的时间基与帧率摘要 |
| `datasetPurpose` | `product_representative|vfr_technical_robustness`；两个目的的样本、分区和汇总不得混淆 |
| `sourceOriginalityEvidence` | VFR 技术集记录原始采集设备/软件、容器/编码信息和“未经平台转码或人为造 VFR”的证据；产品主集可写 `platform_transcode` |
| `sourceFrameTimesSha256` | 对实际输入源流完整解码后的帧时间序列计算 SHA-256；VFR 准入必须先看原始源流，再验证代理保持 |
| `streamKey` | A-014/A-028 约定的稳定视频流键 |
| `contentCategories`、`qualityAttributes` | 使用第 2.2 节稳定 token，可多选 |
| `datasetPartition` | `calibration|tuning|final_evaluation` |
| `probeVersionAndDigest` | FFmpeg/探测器版本及规范探测摘要 hash |
| `privacyAndDistribution` | 是否含人脸、私人场所或敏感信息；证据能否公开 |

## 3. 个人单用户人工参考与修正协议

### 3.1 角色与顺序

1. 执行人先冻结媒体 manifest、分区和窗口，建立 `reference_authoring` 会话；不得向 `USER-01` 展示当前算法候选、置信度、音色结果或任何历史评分。
2. `USER-01` 在静音视频上制作人工参考，标注可见事件、节奏作用和明确负例。允许在同一参考制作会话内自查并执行 `accept|modify|reject`，但必须保留初始版本、修改日志和最终版本，不得覆盖来源记录。
3. D-014 取消独立复核者与独立裁决者前置。最终人工参考标记为 `single_user_reference`，不能命名为独立复核或多人裁决结果；对应角色字段按第 3.3 节明确记为不适用。
4. `single_user_reference` 的媒体、标注协议、事件时间线和 hash 冻结后，算法才能在对应分区运行。`final_evaluation` 的参考在参数和映射冻结后才用于匹配与汇总。
5. 人工参考制作、自然度盲评和人工修正使用不同会话 ID。`USER-01` 完成参考制作并关闭其界面后才能进入随机化盲评；同一片段须先完成盲评，再进入会暴露 `classic` 事件来源的人工修正会话。

正式结果必须记录 `acceptanceScope=personal-single-user-acceptance`、固定匿名角色 ID `USER-01`、是否属于目标用户型创作者、培训/校准版本、各会话起止时间和利益冲突。`USER-01` 同时承担参考制作、主观盲评和人工修正，因此存在 D-014 已接受的记忆、偏好与确认偏差；会话隔离和随机化只能降低而不能消除该风险，结果不得表述为独立多人证据。

### 3.2 事件语义

- `shot`：硬切标在新镜头首个显示帧；渐变以可复核的 `[timeNs,timeNs+durationNs)` 区间标注。闪光或曝光变化不是默认切镜。
- `motion_peak`：标在视觉运动强度局部峰值，附 `global|local|mixed|ambiguous`；它不自动表示应当配重音。
- `action_peak`：标在主体冲击、急停或反转最明确的可见时刻，附 `impact|stop|reversal|ambiguous`；不要求物体识别或动作名称。
- `negative_span`：显式标记某个时间区间内哪些 kind 不应出现，用于计算误报；近静止、闪光、恒速运镜和压缩噪声必须优先检查负例。

每个事件再标注节奏作用：`primary_accent|secondary_accent|do_not_accent|uncertain`。这用于自然度和人工修正分析，不改变 A-028 的三类算法 `kind`。`uncertain` 事件不进入强制命中门槛，但必须单独报告，不能静默丢弃。

### 3.3 标注字段与时间规则

每项至少包含 A-030 所需字段：`annotationId`、`clipId`、`kind`、`timeNs`、`durationNs`、`matchWindowBeforeNs`、`matchWindowAfterNs`、`annotationVersion`、`annotatorAnonymousId=USER-01`。另增加：

- `boundaryOrMotionOrActionClass`；
- `rhythmRole`；
- `uncertaintyReasonToken`；
- `selfCheckDecision=accept|modify|reject` 与 `referenceStatus=single_user_reference`；
- `referenceAuthorAnonymousId=USER-01`、`referenceAuthoringSessionId`；
- `reviewerAnonymousId=null(reason=single_user_scope)`、`adjudicatorAnonymousId=null(reason=single_user_scope)`；
- `acceptanceScope=personal-single-user-acceptance`；
- `sourceFrameTimeNs`、`sourceDecodeOrdinal`；
- `annotationProtocolVersion=0.2.0`。

所有时间来自媒体层真实 `timeNs`，标注工具吸附到实际显示帧并保存 `decodeOrdinal`；禁止用 `frameIndex/nominalFps` 反推。前后匹配窗口由画面本身的可定位精度确定并写理由，不得根据算法误差事后放宽。标注规范串按 A-028 的 kind/time/ID 规则排序并计算 SHA-256。

### 3.4 人工修正量

盲评完成后，由 `USER-01` 在单独的正常产品时间线会话中修正经典算法结果，系统记录原始候选、最终事件和完整操作日志：

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

D-009 已确认聚合和通过阈值；D-014 只把评审人数改为单用户，不修改任何评分锚点或数值门槛。必须保存 `USER-01` 的逐片段、逐版本原始值；overall 与 slice 的中位数、四分位数、低分率、`N/A` 比例和直接导出占比均在有效 final 片段上聚合，不对一个人的评分复制、加权或伪造成多人票。评审者间一致性及任何需要两个以上真人的统计固定报告为 `unavailable(reason=single_reviewer_scope)`，不进入通过补偿。

## 5. 播放与评审条件

### 5.1 自然度盲评

- 产品主集与真实 VFR 技术集的每条 `final_evaluation` 片段都生成 `classic` 和 `human_reference` 两个试听版本；后者来自该片段已冻结的 `single_user_reference`，不是合成 golden。两者使用完全相同的固定音色组、事件到音色映射、混音、响度和导出链，只改变事件时间线。
- 盲评工具隐藏算法名称、置信度、时间线、事件标记、原始文件名、参考制作记录和先前评分；版本顺序和片段顺序在会话开始前随机化，随机种子写入证据。`USER-01` 不得在盲评会话中打开人工参考或答案。
- 画面完整播放，音频开启，不允许拖动或逐帧查看。每个版本先播放一次，`USER-01` 可主动重播最多两次；实际播放次数必须记录。
- 同一评审会话使用同一设备、系统音量、应用音量、显示模式和环境。不得在两版本之间改变音量、耳机/扬声器或显示刷新设置。
- 评分紧随播放完成，不显示版本身份、参考制作记录或先前答案。校准片段只用于理解尺度，不计入正式结果。
- `USER-01` 对每个版本分别填写五维评分、总体自然度、直接导出意愿和可选短原因；成对呈现时再记录 `A|B|tie` 偏好，偏好不覆盖单版本评分。
- 每条 final 必须形成 1 组且仅 1 组正式有效评分，不允许缺失或以均值插补。技术故障或中断导致无效时，旧记录须保留 `invalidReasonToken`，并以新 `trialId` 重新完整播放该片段的一对版本；不得从多次有效结果中挑选有利评分。

每次会话必须记录：`acceptanceScope=personal-single-user-acceptance`、应用/算法/参数/音色映射/数据集/rubric 版本及 hash，Windows edition/build，显示器刷新率与缩放，音频设备与驱动/采样率，系统和应用音量，是否耳机，房间环境，`reviewerAnonymousId=USER-01`、目标用户画像、随机种子、开始结束时间、中断、`trialId` 与有效性。评分完整性按第 8.4 节执行；实际显示和音频设备在首次会话前冻结为运行证据。

### 5.2 人工修正评估

人工修正与盲评分开进行。`USER-01` 只有在对应片段的正式盲评提交后，才能进入正常产品工作区查看事件来源并使用冻结的 `classic` 结果从头修到自己认为可直接导出的状态。开始前重置项目，不得加载人工参考时间线或先前修正结果；结束时保存最终 revision、操作日志、活跃编辑时间和项目 hash。没有达到可直接导出时也必须保存当前事实并选择 `major_edit|unusable`，不得强迫完成。

## 6. 效果门槛结构

以下指标、比较符、单位、聚合方式和数值已经 D-009 确认。每个指标同时按总体、`kind` 和第 2.2 节关键 slice 报告；总体平均不能掩盖任一已确认必过切片。分母为零时写 `unavailable(reason=undefined_denominator)`，不得填 0 或 pass。

| 门槛 ID | 指标 | 比较符 | 单位/聚合 | 数值状态 |
|---|---|---|---|---|
| `E-SHOT-PRECISION` | shot precision | `>=` | overall micro + slice；macro 同报 | overall `0.90`；单个必测 slice `0.80` |
| `E-SHOT-RECALL` | shot recall | `>=` | overall micro + slice；macro 同报 | overall `0.85`；单个必测 slice `0.70` |
| `E-MOTION-PRECISION` | motion_peak precision | `>=` | overall micro + slice；macro 同报 | overall `0.80`；单个必测 slice `0.65` |
| `E-MOTION-RECALL` | motion_peak recall | `>=` | overall micro + slice；macro 同报 | overall `0.70`；单个必测 slice `0.55` |
| `E-ACTION-PRECISION` | action_peak precision | `>=` | overall micro + slice；macro 同报 | overall `0.75`；单个必测 slice `0.60` |
| `E-ACTION-RECALL` | action_peak recall | `>=` | overall micro + slice；macro 同报 | overall `0.65`；单个必测 slice `0.50` |
| `E-FP-MIN` | false positives per minute | `<=` | 每 kind、每 slice P95 与总体 | overall：shot `0.5`、motion `1.0`、action `1.0`、三类合计 `2.0`；任一必测 slice 单类 P95 `2.0` |
| `E-TIME-P95` | 匹配事件绝对时间误差 P95 | `<=` | ms；每 kind + overall | shot `50 ms`、motion `100 ms`、action `80 ms`、overall `100 ms` |
| `E-GRADUAL-IOU` | 渐变转场区间 IoU | `>=` | 每片段 + macro median | overall macro median `0.60`；必测 slice median `0.50` |
| `E-EDIT-ADD` | 人工新增事件数 | `<=` | 次/视频分钟 | overall `1.5` |
| `E-EDIT-DELETE` | 人工删除事件数 | `<=` | 次/视频分钟 | overall `1.0` |
| `E-EDIT-MOVE` | 人工移动事件数及移动量 | `<=` | 次/视频分钟 + 绝对 ms P95 | `1.5`；移动量 P95 `100 ms` |
| `E-EDIT-RECLASSIFY` | 人工改类事件数 | `<=` | 次/视频分钟 | overall `0.5` |
| `E-EDIT-TOTAL` | 总修正操作 | `<=` | 次/视频分钟 | overall `3.0`；任一必测 slice P95 `5.0` |
| `E-EDIT-ACTIVE-TIME` | 活跃编辑时间 | `<=` | 秒/视频分钟 | median `30 s`；P95 `60 s` |
| `E-NATURALNESS` | `overallNaturalness` | `>=` | 逐片原始分布 + median；低分率单列 | overall median `4/5`；必测 slice median `3.5/5`；1～2 分占比 `<=10%` |
| `E-RUBRIC-DIMENSIONS` | 五个 rubric 维度 | `>=` | 每维 overall + slice median | overall 各 `3.5/5`；任一 slice 各 `3/5` |
| `E-DIRECT-EXPORT` | `direct_export|minor_edit` 占比 | `>=` | overall + 关键 slice | `0.80`；`unusable <=0.05` |
| `E-PAIR-PREFERENCE` | classic 相对 human_reference 的偏好 | `>=` | win/tie/loss | classic `win+tie >=0.60`；tie 作为未输，辅助净得分按 `0.5` 票 |
| `E-NA-RATE` | 自然度有效评分的 `N/A` 占比 | `<=` | overall + 逐片 | `0.05`；超过时该项 `not-evaluated` |

F1 作为 precision/recall 的派生诊断同时报告，但不允许只用一个 F1 门槛掩盖高误报或高漏报。置信度分桶只做校准观测，`confidencePpm` 不解释为自然度概率。

## 7. 性能门槛结构与模型门禁

性能运行遵循 A-016：完整场景至少一次预热、五次记录；保存全部原始样本并报告中位数、P95、最小/最大和峰值。只有硬件、Windows、构建、线程、代理/采样、缓存冷热和输入 hash 相同才允许比较。

| 门槛 ID | 指标 | 比较符 | 单位/聚合 | 数值状态 |
|---|---|---|---|---|
| `P-ANALYSIS-RTF` | 分析 real-time factor | `<=` | wall seconds / media seconds；P95 | 典型 `1.0`；最大 `2.0` |
| `P-ANALYZED-FPS` | analyzed fps | `>=` | frames/s；P05 与 median | P05 `30 fps` |
| `P-WALL-P95` | 典型/最大素材 wall time | `<=` | seconds；P95 | 60 秒典型素材 `60 s`；180 秒最大素材 `360 s` |
| `P-PRIVATE-BYTES` | 峰值 private bytes | `<=` | MiB；最大值 | `2048 MiB` |
| `P-WORKING-BYTES` | 峰值 working bytes | `<=` | MiB；最大值 | `1536 MiB` |
| `P-THREADS` | 峰值线程数 | `<=` | count；最大值 | 进程总数 `32`；分析期间相对 idle 新增 `12` |
| `P-CANCEL-P95` | 取消请求到稳定 cancelled | `<=` | ms；P95 | `500 ms` |

每个性能场景同时只运行 1 个分析任务，FFmpeg 解码线程为 2，OpenCV 计算线程最多 8。超过 720p 的输入等比缩小至不超过 `1280x720`，竖屏为 `720x1280`，不跳帧且保留真实时间戳。先预热 1 次，再保存 5 次正式测量；冷启动结果单列，不进入正式 gate。

经典算法只有在冻结的 `final_evaluation` 上实际违反至少一个已确认效果门槛、且已排除输入质量、标注分歧、媒体时间映射或错误基准环境后，才满足“可以提出可选模型方案”的必要条件。即使满足，也只允许提交收益、性能、许可、CPU/GPU、包体和部署影响比较；引入 ONNX Runtime 或模型文件仍需新决定。

## 8. D-009 已确认基线、H-013 实测、D-011 收口与 D-014 个人验收

用户已于 2026-09-14 对 C-1～C-3 逐项确认，并以 D-010 批准 0.3 的 4 个来源替换。execution-readiness-v3 随后证明 40 条来源全部为 CFR；三个所谓 VFR 目标的平台流和 PTS 保持代理均为 CFR。D-011 据此批准 0.4：不再根据标题寻找 VFR，改用独立真实原始素材技术集。2026-09-15，D-014 在个人自用范围内把主观评审改为 `USER-01` 的个人单用户验收。该变化不修改 D-009/D-011 的素材数量、真实 VFR、效果、性能或 Windows 基准门禁；实际素材、评分和测量缺失时不得改判为通过。

### 8.1 C-1 代表产品视频

- 交付方式：由 product-manager-01 从 B 站公开页面筛选真实来源，T-029 执行人按下表获取片段并放入用户控制的外部只读目录。
- 使用边界：用户确认仅用于本项目内部测试，不用于其他用途或对外分发；该确认是产品测试边界，不冒充对第三方权利的法律结论。
- 分区：`calibration=4`、`tuning=16`、`final_evaluation=20`，每个产品类别 10 条。
- 时长与配额：每条 20～60 秒，总时长预计 20～30 分钟；D-011 确认由本主集继续承担全部非 VFR 视觉/语义 slice，每项至少 3 条且至少 1 条属于 `final_evaluation`，VFR 改由第 8.2 节独立技术集承担。
- 证据边界：允许保存 B 站链接、BV 号、脱敏位置、SHA-256、探测摘要、标注和评分；不得将视频原文件提交到 Git。

来源页面元数据由 product-manager-01 于 2026-09-14 从 B 站公开搜索结果读取。所有片段在自然度评估中静音原音；下表时间窗是相对源视频的播放时间，实际获取后由执行成员用真实时间戳校正并计算媒体 hash。不得以 T-028 合成 golden、`package/` 或其他内容代替。

| clipId | partition | 类别 | B 站来源 | 时间窗 | 预期覆盖（须以实际媒体/标注复核） |
|---|---|---|---|---|---|
| SR-BILI-DANCE-001 | calibration | 舞蹈/运动 | [BV1oq4y1E7co](https://www.bilibili.com/video/BV1oq4y1E7co/) 舞蹈混剪 | 00:15～01:00 | fast_cut、local_subject_motion、repetitive_motion |
| SR-BILI-DANCE-002 | tuning | 舞蹈/运动 | [BV1nE411f7NK](https://www.bilibili.com/video/BV1nE411f7NK/) 换装舞蹈混剪 | 00:15～01:00 | fast_cut、flash_or_exposure_change、mixed_global_local |
| SR-BILI-DANCE-003 | final_evaluation | 舞蹈/运动 | [BV1AE411e7Tb](https://www.bilibili.com/video/BV1AE411e7Tb/) 高燃舞蹈混剪 | 00:15～01:00 | fast_cut、impact、tempo_or_energy_change |
| SR-BILI-DANCE-004 | tuning | 舞蹈/运动 | [BV1E8VBzWEDv](https://www.bilibili.com/video/BV1E8VBzWEDv/) 双人舞片段 | 00:15～01:00 | local_subject_motion、repetitive_motion、ambiguous_action |
| SR-BILI-DANCE-005 | final_evaluation | 舞蹈/运动 | [BV1RE41197xT](https://www.bilibili.com/video/BV1RE41197xT/) 舞台高燃混剪 | 00:15～01:00 | fast_cut、impact、flash_or_exposure_change |
| SR-BILI-DANCE-006 | tuning | 舞蹈/运动 | [BV1qxYd6BEZo](https://www.bilibili.com/video/BV1qxYd6BEZo/) 舞蹈开幕式 | 00:15～01:00 | slow_cut_or_transition、mixed_global_local、stop |
| SR-BILI-DANCE-007 | final_evaluation | 舞蹈/运动 | [BV1NwY76rEMH](https://www.bilibili.com/video/BV1NwY76rEMH/) 4K 直拍混剪 | 00:15～01:00 | slow_motion、local_subject_motion、motion_blur_or_occlusion |
| SR-BILI-DANCE-008 | tuning | 舞蹈/运动 | [BV17aYU6HEdh](https://www.bilibili.com/video/BV17aYU6HEdh/) 舞蹈混剪 | 00:15～01:00 | impact、reversal、tempo_or_energy_change |
| SR-BILI-DANCE-009 | final_evaluation | 舞蹈/运动 | [BV12Y4y147Ld](https://www.bilibili.com/video/BV12Y4y147Ld/) 舞蹈群像 | 00:15～01:00 | slow_cut_or_transition、motion_blur_or_occlusion、mixed_global_local |
| SR-BILI-DANCE-010 | final_evaluation | 舞蹈/运动 | [BV1aT411R77K](https://www.bilibili.com/video/BV1aT411R77K/) 水袖舞混剪 | 00:10～00:55 | repetitive_motion、reversal、sparse_free_rhythm |
| SR-BILI-GAME-001 | calibration | 游戏/动作高光 | [BV1bGYE6XEoF](https://www.bilibili.com/video/BV1bGYE6XEoF/) 原神战斗混剪 | 00:15～01:00 | fast_cut、impact、mixed_global_local |
| SR-BILI-GAME-002 | tuning | 游戏/动作高光 | [BV17BT2zbEND](https://www.bilibili.com/video/BV17BT2zbEND/) 三角洲高光混剪 | 00:15～01:00 | fast_cut、impact、flash_or_exposure_change |
| SR-BILI-GAME-003 | final_evaluation | 游戏/动作高光 | [BV14v4y1p7Ki](https://www.bilibili.com/video/BV14v4y1p7Ki/) 1080P60 高光 | 00:15～01:00 | fast_cut、slow_motion、tempo_or_energy_change |
| SR-BILI-GAME-004 | tuning | 游戏/动作高光 | [BV1jeYq6nEC4](https://www.bilibili.com/video/BV1jeYq6nEC4/) 移动端赛车实况 | 00:15～01:00 | long_take_or_no_cut、global_camera_motion、low_quality_compression |
| SR-BILI-GAME-005 | final_evaluation | 游戏/动作高光 | [BV1z4411K7Jx](https://www.bilibili.com/video/BV1z4411K7Jx/) 游戏高燃混剪 | 00:15～01:00 | fast_cut、stop、reversal |
| SR-BILI-GAME-006 | tuning | 游戏/动作高光 | [BV1akYZ6vEHz](https://www.bilibili.com/video/BV1akYZ6vEHz/) 4K60 运动游戏实况 | 00:15～01:00 | long_take_or_no_cut、global_camera_motion、repetitive_motion |
| SR-BILI-GAME-007 | final_evaluation | 游戏/动作高光 | [BV1LkYD6pEVv](https://www.bilibili.com/video/BV1LkYD6pEVv/) 4K60 跑酷实况 | 00:15～01:00 | long_take_or_no_cut、repetitive_motion、tempo_or_energy_change |
| SR-BILI-GAME-008 | tuning | 游戏/动作高光 | [BV1Kqbw6oEZE](https://www.bilibili.com/video/BV1Kqbw6oEZE/) 4K60 骑行实况 | 00:15～01:00 | global_camera_motion、reversal、motion_blur_or_occlusion |
| SR-BILI-GAME-009 | final_evaluation | 游戏/动作高光 | [BV1Zxju6PE2c](https://www.bilibili.com/video/BV1Zxju6PE2c/) FPS 无解说实况 | 00:15～01:00 | long_take_or_no_cut、mixed_global_local、impact |
| SR-BILI-GAME-010 | final_evaluation | 游戏/动作高光 | [BV1dJ411q7f8](https://www.bilibili.com/video/BV1dJ411q7f8/) 1080P60 战场实况 | 00:15～01:00 | global_camera_motion、stop、low_quality_compression |
| SR-BILI-TRAVEL-001 | calibration | 旅行/风景混剪 | [BV1Ex411H7he](https://www.bilibili.com/video/BV1Ex411H7he/) iPhone 1080p60 扫街 | 00:15～01:00 | slow_cut_or_transition、global_camera_motion、tempo_or_energy_change |
| SR-BILI-TRAVEL-002 | tuning | 旅行/风景混剪 | [BV19VYe6MEJs](https://www.bilibili.com/video/BV19VYe6MEJs/) 风景旅行片段 | 00:15～01:00 | slow_cut_or_transition、near_static、sparse_free_rhythm |
| SR-BILI-TRAVEL-003 | final_evaluation | 旅行/风景混剪 | [BV1TsYR6nEk6](https://www.bilibili.com/video/BV1TsYR6nEk6/) 旷野风景短片 | 00:15～01:00 | global_camera_motion、sparse_free_rhythm、ambiguous_action |
| SR-BILI-TRAVEL-004 | tuning | 旅行/风景混剪 | [BV1u7411P7Vx](https://www.bilibili.com/video/BV1u7411P7Vx/) 环球旅行高燃片 | 00:15～01:00 | fast_cut、tempo_or_energy_change、mixed_global_local |
| SR-BILI-TRAVEL-005 | final_evaluation | 旅行/风景混剪 | [BV1va411y7MC](https://www.bilibili.com/video/BV1va411y7MC/) 机车旅途混剪 | 00:15～01:00 | fast_cut、global_camera_motion、motion_blur_or_occlusion |
| SR-BILI-TRAVEL-006 | tuning | 旅行/风景混剪 | [BV1MT411V7Cg](https://www.bilibili.com/video/BV1MT411V7Cg/) 园林航拍长镜头 | 00:15～01:00 | long_take_or_no_cut、near_static、sparse_free_rhythm |
| SR-BILI-TRAVEL-007 | final_evaluation | 旅行/风景混剪 | [BV1F6b86bELD](https://www.bilibili.com/video/BV1F6b86bELD/) Action 5 Pro 海边与乡景混合帧率片段 | 00:30～01:15 | slow_cut_or_transition、near_static、slow_motion |
| SR-BILI-TRAVEL-008 | tuning | 旅行/风景混剪 | [BV1HiYC6VEVC](https://www.bilibili.com/video/BV1HiYC6VEVC/) 风景长镜头 | 00:15～01:00 | long_take_or_no_cut、ambiguous_action、low_quality_compression |
| SR-BILI-TRAVEL-009 | final_evaluation | 旅行/风景混剪 | [BV1zDbV62Ea1](https://www.bilibili.com/video/BV1zDbV62Ea1/) 4K 纽约城市空镜 | 00:15～01:00 | global_camera_motion、slow_cut_or_transition、flash_or_exposure_change |
| SR-BILI-TRAVEL-010 | final_evaluation | 旅行/风景混剪 | [BV1714y1B7ba](https://www.bilibili.com/video/BV1714y1B7ba/) 4K 公路长镜头 | 00:15～01:00 | long_take_or_no_cut、global_camera_motion、repetitive_motion |
| SR-BILI-LIFE-001 | calibration | 生活 Vlog/长镜头 | [BV1huYz6LE5J](https://www.bilibili.com/video/BV1huYz6LE5J/) 晚餐日常 Vlog | 00:15～01:00 | slow_cut_or_transition、local_subject_motion、sparse_free_rhythm |
| SR-BILI-LIFE-002 | tuning | 生活 Vlog/长镜头 | [BV1epYS6jEgn](https://www.bilibili.com/video/BV1epYS6jEgn/) 晚间日常 Vlog | 00:10～00:55 | slow_cut_or_transition、near_static、ambiguous_action |
| SR-BILI-LIFE-003 | final_evaluation | 生活 Vlog/长镜头 | [BV1GyYq6CE51](https://www.bilibili.com/video/BV1GyYq6CE51/) 生活日记 Vlog | 00:15～01:00 | mixed_global_local、sparse_free_rhythm、tempo_or_energy_change |
| SR-BILI-LIFE-004 | tuning | 生活 Vlog/长镜头 | [BV1kqYi6bEUi](https://www.bilibili.com/video/BV1kqYi6bEUi/) 画师生活碎片 | 00:15～01:00 | fast_cut、local_subject_motion、ambiguous_action |
| SR-BILI-LIFE-005 | final_evaluation | 生活 Vlog/长镜头 | [BV18nYC6WEKH](https://www.bilibili.com/video/BV18nYC6WEKH/) 宅家日常 Vlog | 00:15～01:00 | near_static、local_subject_motion、low_quality_compression |
| SR-BILI-LIFE-006 | tuning | 生活 Vlog/长镜头 | [BV1PEjPzyECh](https://www.bilibili.com/video/BV1PEjPzyECh/) 一镜到底片段 | 00:15～01:00 | long_take_or_no_cut、mixed_global_local、repetitive_motion |
| SR-BILI-LIFE-007 | final_evaluation | 生活 Vlog/长镜头 | [BV1Ts41117Hv](https://www.bilibili.com/video/BV1Ts41117Hv/) 1500fps 猫咪慢镜头 | 00:15～01:00 | slow_motion、local_subject_motion、impact、ambiguous_action |
| SR-BILI-LIFE-008 | tuning | 生活 Vlog/长镜头 | [BV1KnYX6LEKS](https://www.bilibili.com/video/BV1KnYX6LEKS/) 房间一镜到底 | 00:00～01:00 | long_take_or_no_cut、near_static、global_camera_motion |
| SR-BILI-LIFE-009 | final_evaluation | 生活 Vlog/长镜头 | [BV1rrYi6oEhQ](https://www.bilibili.com/video/BV1rrYi6oEhQ/) Room Tour 一镜到底 | 00:15～01:00 | long_take_or_no_cut、global_camera_motion、stop |
| SR-BILI-LIFE-010 | final_evaluation | 生活 Vlog/长镜头 | [BV1d3Yi6jEri](https://www.bilibili.com/video/BV1d3Yi6jEri/) 第一视角开箱 Vlog | 00:15～01:00 | long_take_or_no_cut、local_subject_motion、ambiguous_action |

#### H-013 的 0.3 替换与实际 probe 结论

product-manager-01 于 2026-09-14 从 B 站公开页面核验了 4 个候选的标题和公开时长；这只能证明页面存在且候选源时长覆盖所选窗口，不能证明下载流的帧率模式或人工 slice 标签。

| clipId | 0.2 失效来源 | 0.3 已批准来源 | 同类/同分区 | 公开源时长与新窗口 | execution-readiness-v3 结果 |
|---|---|---|---|---|---|
| `SR-BILI-GAME-004` | `BV1FCXhY8ESH`，49.421s 小于批准终点 50s | `BV1jeYq6nEC4` | game / tuning | 135s；15～60s | 源流 CFR；代理 CFR；PTS 保持 pass |
| `SR-BILI-TRAVEL-001` | `BV1YAYz6nEcC`，51.366s 小于批准终点 52s | `BV1Ex411H7he` | travel / calibration | 109s；15～60s | 源流 CFR；代理 CFR；PTS 保持 pass |
| `SR-BILI-TRAVEL-007` | `BV1C8YX6uE8p`，55.402s 小于批准终点 56s | `BV1F6b86bELD` | travel / final_evaluation | 370s；30～75s | 源流 CFR；代理 CFR；PTS 保持 pass |
| `SR-BILI-LIFE-007` | `BV16hrhBoEp7`，21.640s 小于批准终点 22s | `BV1Ts41117Hv` | life / final_evaluation | 144s；15～60s | 源流 CFR；代理 CFR；PTS 保持 pass |

dataset 0.2.0 已实际恢复 40 条、`calibration/tuning/final_evaluation=4/16/20`、四类各 10 条，40/40 媒体 hash 与 4/4 PTS 保持检查通过；但实际为 CFR 40、VFR 0。`slow_motion=4/final=4` 及其余视觉/语义 slice 仍只是声明标签，须由 `USER-01` 按第 3 节形成并冻结 `single_user_reference` 后才能作为本阶段人工参考。B 站页面中的“手机录屏”“混合帧率”“60fps”等标题或描述不再作为 VFR 选源依据。

### 8.2 D-011 确认的 C-1B 独立真实 VFR 技术鲁棒性集

本节已由 D-011 确认生效。技术集是 40 条 B 站产品主集之外的附加数据，不替换主集样本、不计入四类各 10 条，也不允许其结果与主集平均后掩盖失败。

#### 8.2.1 最低组成与分区

- 至少 3 个互不近重复的真实 VFR 原始媒体，来自至少 2 种采集设备或录制软件族；同一原始文件的转码、裁切或不同窗口只计 1 个来源。
- 至少 2 条为 `tuning`、至少 1 条为冻结的 `final_evaluation`；final 原始文件、窗口、标注、代理和参数在解封前分别冻结 hash。
- 内容须来自目标用户实际可能处理的拍摄或录屏，至少覆盖相机实拍和屏幕/游戏录制两种采集路径，并在集合内包含可裁决的 `shot`、`motion_peak`、`action_peak` 与明确负例；不得只收纯测试图案。
- 每个效果评估窗口为 20～60 秒。性能仍执行 D-009 的 VFR 典型 1080p30/60 秒与最大 4K60/180 秒场景；性能窗口可以来自同一批原始文件，但不能重复计入 3 个来源配额。

| 技术 clipId | 分区 | 实际来源 | 准入目标 | 当前状态 |
|---|---|---|---|---|
| `SR-VFR-TECH-001` | tuning | 待提供真实原始文件 | 相机实拍；产品化画面事件 | `missing_actual_media` |
| `SR-VFR-TECH-002` | tuning | 待提供真实原始文件 | 屏幕或游戏录制；负载变化下帧间隔波动 | `missing_actual_media` |
| `SR-VFR-TECH-003` | final_evaluation | 待提供真实原始文件 | 与前两条非近重复；含可裁决动作/镜头事件 | `missing_actual_media` |

#### 8.2.2 VFR 准入证据

- 必须是采集设备或录制软件直接产生的原始容器/视频流；不得使用 B 站、YouTube 等公开平台转码流，不得把 CFR 人工丢帧、复制帧、改时间戳或变速后伪造成 VFR，不得使用 T-028 合成 golden。
- 在任何代理生成前先对原始源流完整解码并保存帧 PTS 序列 hash、正帧间隔数量、不同间隔分布和 probe digest。只有存在至少两种有效正帧间隔，且超过 5% 的正帧间隔相对中位数偏差大于 1 ms，才标为 `vfr`；标题、扩展名、`avg_frame_rate`/`r_frame_rate` 单项差异均不能单独证明 VFR。
- 代理仍按不跳帧、真实 PTS/timeNs 和 720p 上限生成；逐帧数量、时长、帧率模式、中位帧间隔及原始/代理时间戳偏差必须通过现有 PTS 保持检查。原始源流不是 VFR 或代理保持失败时 fail closed，不计入配额。
- 每条保存 `mediaSha256`、只读位置 token、采集设备/软件族、采集日期、原始/代理 probe 版本与 digest、`sourceFrameTimesSha256`、代理 hash 和分区；原始媒体及代理均不提交 Git。

#### 8.2.3 对 D-009 总 gate 的确认修改

1. 将 D-009 的数据门禁拆为 `productRepresentativeGate` 与 `vfrRobustnessGate`。前者使用 40 条 B 站主集；后者使用本节至少 3 条真实 VFR 原始媒体。T-029 总 gate 采用 AND：两者都通过且其余人工/Windows 前置齐全才可判定。
2. 不修改 `vfr>=3`、其中 final `>=1` 的最低数量，不修改第 6～7 节任何 `E-*`、`P-*` 数值、比较符或聚合方式。VFR 技术集按相同标注、人工修正和盲评协议执行，并单独报告所有有有效分母的 E-* slice 指标；VFR 典型/最大性能场景继续使用原 P-* 门槛。
3. 主集的产品效果、自然度或性能通过不能补偿 VFR 技术集失败；VFR 技术集通过也不能补偿主集失败。VFR 实际素材不足、probe 不满足或必选证据缺失时，`vfrRobustnessGate=not-evaluated|fail`，T-029 overall 不得为 pass。

### 8.3 C-2 Windows 基准机与运行条件

- 基准 ID 与机型：`TIGER`，ASUSTeK `TX Air FA401KM_FA401KM`。
- Windows：Microsoft Windows 11 家庭版中文版，64 位，Version `10.0.26200`，Build `26200`。
- CPU：AMD Ryzen AI 7 H 350 with Radeon 860M，8 物理核 / 16 逻辑处理器。
- 内存：`33,413,771,264 bytes`，名义 32 GB（约 31.1 GiB）。
- GPU：NVIDIA GeForce RTX 5060 Laptop GPU，driver `32.0.15.7297`；AMD Radeon 860M，driver `32.0.22032.6002`。
- 存储：SK hynix `HFS001TEM9X174N`，NVMe SSD，约 1 TB；选定时 C 盘可用 `592,158,224,384 bytes`，可用空间必须每次运行重新记录。
- 供电：探测时电池 100% 且接通电源；正式评估固定为接通电源 + Windows 最佳性能模式，运行前保存实际活动电源方案证据。
- 典型素材：1920x1080、30 fps、60 秒，CFR 与 VFR 分别测量。
- 最大素材：3840x2160、60 fps、180 秒，CFR 与 VFR 分别测量。
- 输入代理：最大 1280x720，竖屏 720x1280，等比缩放；不跳帧，保留真实 PTS/timeNs。
- 并发/线程：同时 1 个分析任务；FFmpeg 解码线程 2；OpenCV 计算线程最多 8。
- 缓存/重复：每场景预热 1 次后测量 5 次；正式 gate 使用预热后样本，首次冷启动单列报告。

上述基准机信息来自本机 Windows 原生只读探测，不使用 Wine 观测替代。正式测量时仍须重新保存 OS、驱动、电源、温度/降频诊断、构建和输入 hash。

### 8.4 C-3 评审、数值阈值与总 gate

- 验收范围：`personal-single-user-acceptance`，仅用于 D-012/D-013 的用户本人个人自用一期范围；不得宣称独立多人产品验证通过，也不得外推到其他用户、公开分发或第三方交付。
- 评审者：固定匿名 ID `USER-01`，不再要求 5 名独立评审者或至少 3 名有剪辑经验者；人工参考制作、盲评和人工修正的角色重合必须如实记录。
- 有效评分：产品主集和真实 VFR 技术集的每条 `final_evaluation` 视频均须取得 `USER-01` 的 1 组正式有效评分，不允许缺失、复制或插补。任一片段缺失时，对应数据集 gate 及 overall 为 `not-evaluated`。
- 独立性统计：独立复核、独立裁决与评审者间一致性不再是本阶段前置；相关字段固定为 `not_applicable(reason=single_user_scope)` 或 `unavailable(reason=single_reviewer_scope)`，不得伪造多人统计，也不得把不可用统计当作通过证据。
- tie：成对盲评允许 tie，tie 计入“未输”，辅助净得分按 0.5 票计算。
- 数值阈值：完整保留第 6～7 节已确认的全部 `E-*` 与 `P-*` 数值、单位、比较方向和聚合方式；单用户范围不构成降低阈值、删除 slice 或平均补偿的理由。
- 判定分区：产品主集只在冻结的 20 条 `final_evaluation` 上判定代表性产品 gate；按 D-011，VFR 技术集另在其冻结的 `final_evaluation` 上判定 VFR slice gate，两个结果不混合平均。
- 总 gate：所有必选 `E-*` 与 `P-*` 必须同时通过，不得用平均分抵消失败项；任一必测 slice 未达门槛则总体不通过。
- 真实 VFR：继续要求至少 3 条真实原始 VFR，且 final 至少 1 条；原始源流逐帧 PTS、代理保持和双数据集 AND gate 完全按第 8.2 节执行。
- Windows 性能：继续使用第 8.3 节 `TIGER`、接通电源与 Windows 最佳性能模式、固定代理/线程、一次预热加五次正式测量；个人验收不允许用其他环境或 Wine 数值替代。
- `unavailable`：除本节明确列为非 gate 的单评审者独立性统计外，任一必选指标因数据不足为 `unavailable` 时，总体为 `not-evaluated`；CPU-only 路径的 GPU 时间和显存允许 `unavailable` 且不影响总 gate。
- 模型边界：经典算法失败后只能提出模型方案，不自动引入模型。

本轮按 D-014 修订并批准 A-031 0.5，没有运行 T-029，没有代替 `USER-01` 生成评分，也没有实际 VFR 原始素材。任何 VFR 结果在 video-algorithm-engineer-cv-01 保存原始源流和代理的完整 probe/hash、`single_user_reference` 及个人盲评证据前仍为 `not-evaluated`。

## 9. 交付给 T-029 的证据包

D-009、D-011、D-014 已确认；T-029 执行人应分别冻结并引用：

1. 本文件版本与 SHA-256；
2. 40 条产品主集与独立 VFR 技术集各自的数据 manifest、媒体 hash、权限边界和分区 hash；
3. `USER-01` 的初始参考、自查修改日志、冻结 `single_user_reference` 及各自 hash，并保存独立复核/裁决字段的不适用原因；
4. rubric、评分尺度、播放条件、音色映射、随机化 seed、`trialId`、无效试次原因和参考制作/盲评分离的会话记录；
5. 基准硬件与软件环境签名；
6. 已确认的 `E-*`、`P-*` 数值门槛和来源；
7. classic 的算法/参数/build hash 与逐次原始结果；
8. `USER-01` 的人工修正日志、逐片段逐版本原始评分、逐片评分完整性检查、`personal-single-user-acceptance` 标记和汇总脚本版本。

任何缺项都必须保留为 `missing_confirmed_input`、`measured` 或 `not-evaluated`，不得使用合成 golden、CFR 人工造 VFR、复制单用户评分、建议值或当前实现观测补齐后宣称产品效果通过。当前真实 VFR 原始素材和 `USER-01` 实际记录均缺失，因此相应门禁仍为 `not-evaluated`，不能因 0.5 已批准而视为通过。
