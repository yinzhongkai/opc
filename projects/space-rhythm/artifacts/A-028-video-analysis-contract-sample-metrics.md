# 视频分析输入/输出契约、代表样本与效果指标 0.x

- 项目：space-rhythm
- 成果 ID：A-028
- 负责人：video-algorithm-engineer-cv-01
- 关联任务：T-027
- 版本：0.1
- 更新日期：2026-09-14
- 状态：draft
- 适用范围：视频镜头切换、运动峰值和动作峰值的分析边界、候选数据、确定性样本与测量规则；不包含 T-028 算法实现、T-029 学习模型、核心融合、产品效果批准或发布门禁。
- 来源及输入版本：用户于 2026-09-14 明确要求执行 T-027 并按 T-027 → T-028 → T-029 顺序推进；D-001～D-008 confirmed；A-002 0.2、A-004 0.5、A-005 0.4、A-006 0.1 WP-05、A-011 0.1、A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1。
- 批准依据：尚无。
- 版本记录：2026-09-14，0.1，首次冻结输入/输出、三类候选、真实 `timeNs`、置信度/版本/低质量原因、合成样本配方及指标口径；“卡点自然”仍无已确认通过阈值。

## 1. 结论与版本基线

T-027 冻结以下逻辑协议；T-028 应在不改变语义的前提下实现：

| 项 | 冻结值 |
|---|---|
| `videoAnalysisContractVersion` | `0.1.0` |
| 视频分析 schema | `1` |
| 参数 schema | `1` |
| 样本 manifest | `1` |
| 指标 schema | `1` |
| 上游核心契约 | `coreContractVersion=0.1.0`、schema `1` |
| 上游媒体契约 | `mediaContractVersion=1.0.0`、schema `2` |

媒体层拥有探测、解码、PTS/VFR 映射、像素归一化及缓冲区生命周期；核心层拥有
`ProjectTimeNs`、事件轨道、修订、锁定、撤销重做和融合。视频分析只消费媒体层公开的
真实时间帧并产生 `AnalysisCandidate`，不得自行重建时间、直接改写用户事件或改变核心
融合规则。

`package/`、用户媒体和第三方视听作品不是契约样本。T-027 也不引入学习模型；是否执行
T-029 必须先完成 T-028 并取得可复核的经典算法基线证据。

## 2. 输入契约

### 2.1 `VideoAnalysisRequest`

请求的逻辑字段如下；大块像素数据通过进程内只读 frame lease 或已批准的共享传输交付，
不嵌入 JSON：

| 字段 | 类型/约束 | 语义 |
|---|---|---|
| `schemaVersion` | `uint32=1` | 本请求 schema。未知必填字段或不支持版本须 fail closed。 |
| `videoAnalysisContractVersion` | UTF-8，`0.1.0` | 视频分析契约版本。 |
| `jobId` | 非空稳定 ID | 作业关联，不用于算法随机性。 |
| `sourceFingerprintSha256` | 64 位小写十六进制 | 输入媒体内容身份；写入每个候选的来源。 |
| `streamKey` | 媒体层稳定流键 | 只分析已选视频流。 |
| `mediaContractVersion` / `mediaSchemaVersion` | `1.0.0` / `2` | 消费边界声明。 |
| `algorithmId` | `space-rhythm.video-analysis.classic` | 算法族；T-029 若引入模型必须使用新 ID/版本。 |
| `algorithmVersion` | 非空语义版本 | 精确标识产生结果的实现；不得写 `latest`。 |
| `parameterSetId` | 非空稳定 ID | 人可读参数集身份。 |
| `parameterSchemaVersion` | `uint32=1` | 参数字段 schema。 |
| `parametersDigestSha256` | 64 位小写十六进制 | 规范化参数全文的 SHA-256。 |
| `seed` | `uint64` | 所有允许的随机过程的唯一 seed；确定性算法也要记录。 |
| `analysisRevision` | `uint64` | 与同次输出和核心应用操作关联；只允许递增的新分析替换旧分析。 |
| `inputRangeNs` | 可选 `[startNs,endNs)` | 两端均为真实项目纳秒；`0 <= start < end`。缺省为整段流。 |
| `samplingPolicy` | 版本化对象 | 时间域步长、首帧选择、最大 gap 和格式 epoch 行为。 |
| `resourceLimits` | 版本化对象 | 队列项/字节、线程、内存和取消检查粒度的硬上限。 |

参数规范串采用 UTF-8、字段名按字节升序、整数十进制、布尔 `true/false`、枚举固定 token、
数组保持声明顺序且不使用浮点文本；SHA-256 覆盖完整规范串。影响候选语义的参数或实现改变
必须改变 `algorithmVersion`、`parametersDigestSha256`，必要时提升 schema。

### 2.2 帧格式和生命周期

输入帧必须是 A-014 0.4 / 媒体 schema 2 的 `VideoFrame`：

- `timeNs` 和 `durationNs` 是媒体层根据真实 PTS、time base、start offset 和编辑语义映射出的
  项目时间；分析器直接使用，不得用 `frameIndex / nominalFps` 重建。
- 像素为显示变换已应用、像素宽高比 `1:1`、8-bit full-range BGRA；宽高、stride、plane 和
  format epoch 来自公开 DTO。分析器不得从文件名或编码器标签猜测像素格式。
- 输入稳定排序键为 `(timeNs, decodeOrdinal)`。重复 PTS 合法；同一 `timeNs` 的帧仍以
  `decodeOrdinal` 区分。时间向后跳或 epoch 改变必须结束当前分析 segment、清空跨帧状态并
  产生诊断，禁止排序掩盖或静默 clamp。
- 使用 `MediaSource::proxy_frame` 时，调用方保留 `scheduledTimeNs`，并把返回帧的真实
  `timeNs`、`decodeOrdinal`、`sourcePts` 和 geometry 组成代理映射记录；候选时间只能来自
  返回帧或由相邻返回帧限定的证据区间，不能把请求时间当成命中时间。顺序解码的缩放代理
  同样保留媒体 DTO 的原始时间字段，空间缩放不改变时间映射。
- 核心候选要求非负 `ProjectTimeNs`。若上游帧映射仍为负，记录
  `timestamp_discontinuity`/输入错误并跳过候选，禁止擅自改为零。
- frame lease 只在约定调用期内有效。异步保留时必须在资源上限内显式复制；释放 lease 后
  不得持有 plane 指针。背压时停止索取新帧，不得无界缓存。

### 2.3 时间域采样、取消和失败

固定步长采样以项目纳秒调度：每个调度点选择首个 `timeNs >= scheduledTimeNs` 的可用帧，
并记录实际选择的 `timeNs` 与 `decodeOrdinal`；VFR 不插帧、不假设恒定 fps。超过版本化
`maxSamplingGapNs` 时断开跨 gap 曲线并记录 `sampling_gap`。seek、流/格式切换和时间回跳
均新建 segment。

读取、颜色转换、特征提取、光流批次及输出序列化都必须检查取消。取消返回可判定状态，
不得提交半成品候选；失败返回稳定错误 token 和诊断，不用异常文本充当公共协议。

## 3. 输出契约

### 3.1 `VideoAnalysisResult`

| 字段 | 类型/约束 | 语义 |
|---|---|---|
| `schemaVersion` / `videoAnalysisContractVersion` | `1` / `0.1.0` | 输出协议。 |
| `status` | `completed`、`low_quality`、`cancelled`、`failed` | `low_quality` 可携带候选；后两者不得提交候选。 |
| 输入身份字段 | 与请求逐项相同 | `jobId`、流、输入 hash、范围、seed、revision。 |
| 算法身份字段 | 与请求逐项相同 | ID、版本、参数集/schema/hash；结果不得覆盖请求值。 |
| `coreContractVersion` / `coreSchemaVersion` | `0.1.0` / `1` | 下游 DTO 目标。 |
| `candidates` | 有序 `AnalysisCandidate[]` | 排序和 ID 规则见下文。 |
| `diagnostics` | 有序结构数组 | token、segment/range、计数及可选详情；不承载必须字段。 |
| `curveArtifacts` | 描述符数组 | 大曲线的版本、时间范围、点数、采样策略、内容 hash 和受控位置；不把全曲线塞入候选。 |

候选必须直接映射 A-012 的实际 `AnalysisCandidate`：

- `id`：规范串 SHA-256 导出的稳定 ID，格式 `va:<kind>:<64-lower-hex>`；规范串至少包含
  契约/schema、输入 hash、流键、segment、`timeNs`、`durationNs`、kind、算法 ID/版本、
  参数 hash、seed 和证据窗口。相同输入与参数重跑 ID 相同；任何语义输入变化产生新 ID。
- `proposedTrackId`：版本化默认轨道映射；只建议目标轨道，不创建或覆盖用户轨道。
- `timeNs` / `durationNs`：真实项目纳秒，规则见第 4 节。
- `kind`：只允许 `shot`、`motion_peak`、`action_peak`。
- `source.origin=analysis`；`producerId=space-rhythm.video-analysis`；
  `producerVersion=algorithmVersion`；同时填写 `inputFingerprint`、`parametersDigest`、
  `analysisRevision` 和可追溯的 `candidateIds`。
- `strengthPpm`、`confidencePpm`：闭区间 `[0,1000000]` 的整数，见第 5 节。
- `payload.owner=space-rhythm.video-analysis`，`payload.schemaVersion=1`；
  `requiredFeatures` 只列真正必需的稳定 token。`payload` 继续遵循核心的
  `map<string,string>`，数值写十进制整数、枚举写固定 token。

公共 payload 必填键为 `evidenceWindowStartNs`、`evidenceWindowEndNs`、`detectorId`、
`detectorVersion`、`normalizationScope`、`qualityReasonTokens`。原因 token 去重后按字节升序以
逗号连接；没有原因时为空串。结果按 `(timeNs, kindRank, id)` 稳定排序，其中
`shot=0`、`motion_peak=1`、`action_peak=2`。ID 全长碰撞必须 fail closed，不得截短后递增编号。

## 4. 三类候选的时间与数据语义

### 4.1 镜头切换 `shot`

- 硬切：`timeNs` 是新镜头首个显示帧的真实项目时间，`durationNs=0`。
- 渐变/叠化：`timeNs` 是转场证据区间起点，`durationNs` 是 `[timeNs,timeNs+durationNs)`
  的实际区间长度。没有可复核的起止证据时不得伪造持续时间。
- kind payload：`boundaryKind=hard_cut|gradual_transition`、`histogramDistancePpm`、
  `structuralChangePpm`、`flashLikelihoodPpm`、`transitionStartNs`、`transitionEndNs`。
- 单帧闪光、曝光突变或编码噪声不得自动等同硬切；歧义必须体现在置信度和原因 token。

### 4.2 运动峰值 `motion_peak`

- `timeNs` 是版本化运动强度曲线的局部极大值对应的真实帧时间，`durationNs=0`；平台区按
  版本化规则选择确定性代表点并记录规则。
- 它描述视觉运动幅度峰，不等价于语义动作，也不等价于全局相机运动。
- kind payload：`curveValuePpm`、`prominencePpm`、`globalMotionPpm`、
  `localResidualPpm`、`motionClass=global|local|mixed|ambiguous`、`curveSchemaVersion`。
- 全局平移/缩放/旋转必须显式估计并登记；不能把所有光流能量都宣称为局部动作。

### 4.3 动作峰值 `action_peak`

- `timeNs` 是局部主体冲击、急停或反转证据最强的真实帧时间，`durationNs=0`。
- 它是编辑候选，不声称完成物体识别或动作语义分类。T-028 经典算法可用局部残差运动、
  速度变化、加速度、反转和短时静止组合，但必须保留来源字段。
- kind payload：`actionClass=impact|stop|reversal|ambiguous`、`accelerationPpm`、
  `reversalPpm`、`stillnessAfterPpm`、`localResidualPpm`、`globalSuppressionPpm`。
- 相机运动占主导、遮挡、运动模糊或采样不足时须降置信度并给出原因，不能把缺证据写成
  高置信度动作。

## 5. 强度、置信度和低质量原因

`strengthPpm` 是同一 `kind`、同一 analysis segment、同一算法/参数下的相对证据强度。
它不是概率，不允许跨 `shot`/`motion_peak`/`action_peak` 或跨参数版本直接比较。
`normalizationScope` 必须说明归一化范围，常量曲线的强度按实现契约处理而非除零。

`confidencePpm` 表示输入质量、证据一致性、时序定位稳定性及歧义惩罚后的可靠度；它不是
“用户会觉得卡点自然”的概率，也不是产品通过标记。T-028 必须记录由哪些可复算整数项
组成置信度，并用参数版本固定组合方式。

稳定低质量原因 token 如下；可多选、排序并同时出现在候选和结果诊断中：

| token | 触发事实 |
|---|---|
| `decode_errors` | 证据窗口存在媒体层公开的解码错误/损坏。 |
| `timestamp_discontinuity` | PTS 映射回跳、缺失或不连续导致 segment 重置。 |
| `sampling_gap` | 相邻实际采样时间超过参数所声明的最大 gap。 |
| `format_epoch_boundary` | 证据窗口跨越分辨率/像素格式 epoch。 |
| `insufficient_frames` | 窗口内有效帧不足以计算对应特征。 |
| `insufficient_spatial_detail` | 分辨率或纹理不足。 |
| `low_contrast` | 对比度不足使结构/运动证据不稳定。 |
| `color_metadata_assumed` | 上游明确标记色彩元数据为 assumed。 |
| `compression_noise` | 块效应/振铃等压缩噪声占主要变化。 |
| `flash_ambiguous` | 瞬时全局亮度变化与硬切不可可靠区分。 |
| `global_motion_dominant` | 全局相机运动压过局部残差。 |
| `occlusion_ambiguous` | 遮挡使局部轨迹不连续。 |
| `motion_blur` | 模糊使结构或运动定位不稳定。 |
| `near_static` | 动态范围接近噪声底。 |
| `threshold_edge` | 证据靠近当前算法参数边界，结果对微扰敏感。 |

低质量候选可以保留以供人工编辑，但不得通过 `confidencePpm=0` 或自由文本暗示原因；至少
一个稳定 token 必须存在。结果只要出现影响解释的系统性低质量事实，就用 `low_quality`
而非伪装为 `completed`。

## 6. 代表样本矩阵与完整性

机器可读清单为 [`tests/golden/video/fixtures-v1.json`](../../../tests/golden/video/fixtures-v1.json)，
许可声明见 [`LICENSE.md`](../../../tests/golden/video/LICENSE.md)，结构和规范串校验入口为
[`Test-GoldenVideoManifest.ps1`](../../../tests/golden/video/Test-GoldenVideoManifest.ps1)。

| 样本 | 代表切片 | 正向标注 | 主要负向约束 |
|---|---|---|---|
| `VV-HARD-CUT-001` | 单次快硬切 | 1 shot | 无运动/动作峰 |
| `VV-DISSOLVE-001` | 慢叠化 | 1 区间 shot | 无伪运动/动作峰 |
| `VV-FLASH-001` | 单帧全局闪光 | 无 | 不得报 hard cut/运动/动作 |
| `VV-GLOBAL-PAN-001` | 恒速全局平移 | 1 motion peak | 不得报 shot/action；峰应标 global |
| `VV-LOCAL-IMPACT-001` | 静态背景中的局部冲击 | motion + action | 不得报 shot |
| `VV-STATIC-001` | 静态低变化 | 无 | 三类均为负例 |
| `VV-SLOW-MOTION-001` | 慢动作冲击 | motion + action | 不得报 shot |
| `VV-VFR-REVERSAL-001` | VFR 局部反转 | motion + action | 使用真实 `timeNs`，不得 fps 重建 |
| `VV-COMPRESSION-NOISE-001` | 低质压缩噪声 | 无 | 三类均为负例，报告低质量事实 |
| `VV-FAST-CUT-ACTION-001` | 快切与局部动作邻接 | 3 shot + motion + action | 检验同刻排序和切镜隔离 |

上述 10 项为第一层“契约合成集”：全部是 CC0 项目自制配方，覆盖
fast/slow cut、flash、全局/局部运动、static、slow motion、VFR 和 compression noise。
清单已经冻结每项规范配方 SHA-256、标注规范串 SHA-256、真实纳秒位置和容差窗口。
T-027 不生成媒体字节，因此 `mediaStatus=specified_not_generated` 且
`actualMediaSha256=null` 是有意状态，不冒充可运行 golden。

T-028 必须用项目固定 FFmpeg 基线实现生成器，生成实际字节，登记媒体 SHA-256 和探测
证据，并保持已冻结的配方/标注 hash 不变。产品真实评估集是第二层，当前尚未获提供；
纳入前每个素材必须记录来源、使用权限、许可证、内容 SHA-256、标注人/复核人、标注版本、
训练/调参/验证分区。不得自行从 `package/` 或用户目录取材。

## 7. 标注与确定性匹配

每个 ground-truth 事件包含 `id`、`kind`、真实 `timeNs`、`durationNs`、
`matchWindowBeforeNs` 和 `matchWindowAfterNs`；显式 `negativeKinds` 表示整个样本对该 kind
为负例。标注规范串按 `(kind,timeNs,id)` 排序，负类按 token 排序后计算 SHA-256。

点事件只在同 kind 内一对一匹配。候选须落入
`[gt.timeNs-matchWindowBeforeNs, gt.timeNs+matchWindowAfterNs]`；全局分配优先最小化总绝对
时间误差，仍并列时按候选 ID 字节序决定。一个候选和一个真值最多匹配一次；未匹配候选为
FP，未匹配真值为 FN。渐变 shot 另报告预测/真值区间 IoU、起点误差和终点误差；本版本不
规定 IoU 通过阈值。

## 8. 效果、校准、编辑与性能指标

所有统计必须同时报告算法/参数/输入/标注 hash、seed、构建 preset、OpenCV/FFmpeg 精确
版本和样本切片。不得只报汇总均值。

### 8.1 检测效果

每个 kind 分别报告 TP、FP、FN、precision、recall、F1、FP/min、预测与标注事件密度。
分母为零时相应比率写 `unavailable(reason=undefined_denominator)`，不得填零。匹配项报告有符号
时间误差与绝对误差的 median、P95、max；渐变 shot 再报告 IoU、起止误差。总体同时给出
micro 与按样本/类别 macro，且必须保留 fast_cut、slow_cut、flash、global/local、static、
slow_motion、VFR、compression_noise 切片，避免平均值掩盖失效模式。

### 8.2 置信度观测

按固定 PPM 桶报告候选数、匹配精度、平均 `confidencePpm` 与经验精度差；样本不足的桶原样
登记。未经独立校准，`confidencePpm` 仍只是版本化可靠度分数，不宣称概率。低质量 token
按 kind 和样本类别报告出现率、对应 precision/recall 与时间误差，便于定位降级是否有效。

### 8.3 人工修正与“卡点自然”

在未来已批准的人工评估协议下记录每分钟新增、删除、移动、改类数量，移动绝对纳秒，
以及修正前后的候选来源；锁定/manual 事件仍由核心层保护。人工偏好、节奏贴合和“卡点自然”
只保存原始评分、成对选择、评审者/片段匿名 ID、播放条件和 rubric 版本。

当前没有产品确认的“卡点自然”rubric、样本量、评分尺度或阈值。因此这些结果只能标记
`measured`，对应效果 gate 必须是 `not-evaluated`；不得计算“通过率”，不得自行设定平均分、
胜率、误差或置信度门槛，也不得用本合成集替代真实用户判断。

### 8.4 性能与资源

报告 wall time、输入视频时长、real-time factor、decoded/analyzed fps、峰值 working/private
bytes、峰值队列 items/bytes、线程数和取消延迟；同时记录 CPU/GPU/内存、OS、构建 preset、
缓存冷热、输入 hash、帧数/像素数和采样参数。未确认预算时，数字只标 `measured`，性能 gate
为 `not-evaluated`。无法读取的 GPU 时间或显存必须写
`unavailable(reason=<stable-token>)`，不能用 CPU/working set 代替。

## 9. 状态判定与 T-028 交接条件

结构、hash、排序、版本和固定 oracle 可形成可判定的功能契约测试；这类测试真实执行成功
时可以写 `pass`。检测效果、性能和“卡点自然”在没有已确认阈值时只能写 `measured`，项目
gate 保持 `not-evaluated`，符合 A-016。

T-028 的最小输入已经具备，但其完成必须补齐：

1. 经典 OpenCV 实现及公共 C++ DTO/adapter，不修改媒体/core 所有权；
2. 确切 OpenCV 版本、算法版本、参数规范串/hash 和所有低质量计算来源；
3. 合成媒体生成器、实际媒体 SHA-256/ffprobe 证据、单元/契约/golden/取消/资源测试；
4. 三类候选真实 `timeNs`、稳定 ID/排序、置信度和原因 token 的端到端断言；
5. 效果与性能数值按本文件记录为 `measured/not-evaluated`，除非届时已有明确批准门槛。

只有 T-028 完成并保留经典基线、失败样本与可复核测量后，才能开始 T-029 的模型必要性评估。

## 10. T-027 自查

- [x] 已绑定 `video-algorithm-engineer-cv-01` 并接收 H-007。
- [x] 已与实际 core 0.1/schema 1 和 media 1.0/schema 2 对齐。
- [x] 已冻结输入、输出、三类候选、真实 `timeNs`、算法/参数版本和确定性顺序。
- [x] 已区分强度、置信度、低质量事实和产品通过结论。
- [x] 已建立 10 项代表样本配方、标注、hash、许可及机器校验入口。
- [x] 已定义按 kind 的匹配、效果、校准、人工修正、自然度与性能测量。
- [x] 未自行设定“卡点自然”或性能通过阈值，状态保持 `measured/not-evaluated`。
- [x] 已明确 T-027 → T-028 → T-029 顺序及 T-028 待生成实际媒体的边界。
