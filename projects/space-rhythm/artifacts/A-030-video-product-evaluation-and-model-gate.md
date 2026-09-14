# 视频产品效果、性能与可选模型门禁评估

- 项目：space-rhythm
- 成果 ID：A-030
- 负责人：video-algorithm-engineer-cv-01
- 关联任务：T-029
- 版本：0.2
- 更新日期：2026-09-14
- 状态：draft
- 适用范围：代表产品视频、人工标注、经典视频算法效果/性能和“卡点自然”评估，以及可选模型门禁；不批准产品口径，不把合成样本替代产品数据，不直接引入 ONNX 或修改核心时间线。
- 来源及输入版本：用户于 2026-09-14 明确授权执行 T-029；D-001～D-009 confirmed；A-002 0.2、A-016 0.1、A-028 0.2、A-029 0.1、A-031 0.2 approved；T-028 提交 `0513aa3`。
- 批准依据：尚无。
- 版本记录：2026-09-14，0.1，登记 T-029 启动、输入就绪审计、评估协议草案和模型门禁边界；没有产品效果或性能通过结论。2026-09-14，0.2，按 A-031 0.2 实际审计 40 个来源并冻结 36 个可用代理的媒体/probe hash；4 个批准时间窗超出源时长、实际 VFR 配额为 0、独立真人证据和合规 Windows 原生运行条件未就绪，故记录真实阻塞和 `not-evaluated`，没有运行 classic 或引入 ONNX。

## 1. 输入与执行就绪结论

首次缺输入状态保留在 [`input-readiness-v1.json`](../evidence/T-029/input-readiness-v1.json)。
随后 A-031 0.2 与 D-009 已确认来源、标注/盲评协议、自然度 rubric、Windows 基准元组和全部
`E-*`/`P-*` 阈值。负责人据此实际审计 40 个来源并取得 36 个静音、最大 720p 的内部测试代理，
没有把视频字节提交到 Git。当前机器可读收口见
[`execution-readiness-v2.json`](../evidence/T-029/execution-readiness-v2.json)。

| 输入/前置 | 实际结果 | T-029 判定 |
|---|---|---|
| 来源与权限 | D-009 允许本项目内部测试，不对外分发；不构成第三方权利法律结论 | `confirmed_boundary` |
| 来源时间窗 | 40 项中 36 项有效；4 项批准结束时间超出源时长 | `fail` |
| 媒体与 probe | 36 项已冻结逐 clip 媒体、probe、帧时间 hash；合计 1,141,653,591 bytes / 1,635.045 s | `partial` |
| 分区/类别 | 实际 `3/15/18`，类别 `10/9/8/9`，未达到 `4/16/20` 与每类 10 | `fail` |
| 技术 slice | probe 为 CFR 36、VFR 0；A-031 要求 VFR 至少 3 且 final 至少 1 | `fail` |
| 人工初标/复核/裁决/盲评 | 尚无 5 名独立真人及相应记录/hash | `missing` |
| rubric 与阈值 | A-031 0.2 已由 D-009 确认 | `confirmed` |
| Windows 基准 | TIGER 硬件已确认；当前活动电源方案是“平衡”，Release PE 又在 `main` 前被 WDAC/SAC 以 `0xC0E90002` 拒绝 | `blocked` |

A-031 明确要求先冻结裁决标注，才能运行 classic；当前裁决标注 hash 为空，所以负责人没有
越序运行算法，也没有用合成 golden、AI 评分或 Wine 测量冒充产品结果。

## 2. 代表产品集和人工标注输入契约

产品侧提供的每个 clip 至少登记：稳定匿名 `clipId`、只读位置、内容 SHA-256、来源/使用权限、
时长、分辨率、真实帧率模式、视频流键、内容类别、质量属性和数据分区。覆盖类别至少应从已
确认主流程中选择快切、慢切、全局运镜、局部动作、慢镜头、VFR、近静止、低质压缩及混合
事件；具体 clip 数、时长和各类配额必须由用户/产品侧确认，算法侧不自行补数。

事件标注继续使用 A-028 0.2 的真实 `timeNs` 语义。每项包含 `annotationId`、`clipId`、
`kind=shot|motion_peak|action_peak`、`timeNs`、`durationNs`、前后匹配窗口、标注版本、匿名标注人
和复核人；明确的无事件区间登记 negative span。还需逐次保存新增、删除、移动、改类和锁定
操作，才能计算每分钟人工修正量。媒体、标注和分区 hash 必须在分析前冻结，调参集与最终
评估集不得混用。

产品视频可以保持在用户控制的外部只读目录，证据只登记脱敏位置标识和 hash；不得把私人
素材、`package/` 内容或未知许可作品复制进可分发 golden。

## 3. 已确认的“卡点自然”rubric 基线

A-031 0.2 已确认随机化盲评、固定 clip/时间线/试听映射、逐评审者原始记录和 classic/
human_reference 成对呈现；正式评审须有 5 名独立真人且至少 3 名有短视频剪辑或卡点制作经验，
算法实现者不进入正式评审。rubric 维度为：

1. 事件与可见镜头/动作落点的时间贴合；
2. 被选视觉事件的显著性与节奏重音是否相称；
3. 密度是否造成漏拍或多余干扰拍；
4. 连续片段中的节奏是否稳定且不过度跳变；
5. 达到可用结果所需的人工修正负担。

评分采用 1～5 级并报告 `direct_export|minor_edit|major_edit|unusable`、N/A、逐片分布和成对
win/tie/loss。每条 final 至少 3 份有效评分。上述口径已经确认，但实际真人评审尚未发生，
因此所有自然度结果仍为 `not-evaluated`，算法实现者或 AI 不得代填。

## 4. 已确认门槛与本轮判定

A-031 0.2 第 6～7 节已确认全部比较符、单位和聚合方式：shot/motion/action 的 overall
precision 下限为 `0.90/0.80/0.75`，recall 下限为 `0.85/0.70/0.65`，并另有 slice 下限；
同时约束 FP/min、时间误差 P95、渐变 IoU、人工新增/删除/移动/改类/总修正/活跃编辑时间、
五维 rubric、总体自然度、直接导出率、成对偏好和 N/A 率。性能要求包括典型/最大 RTF P95
`<=1.0/2.0`、analyzed fps P05 `>=30`、60 秒/180 秒 wall P95 `<=60/360 s`、private bytes
`<=2048 MiB`、working bytes `<=1536 MiB`、进程/新增线程 `<=32/12`、取消 P95 `<=500 ms`。

这些是确认的通过标准，不是本轮实测值。由于 final 数据集、裁决标注和合规 Windows 环境
尚未同时冻结，效果、自然度、人工修正量和性能均为 `not-evaluated`，不能写作 pass 或 fail。

## 5. 可选模型门禁

当前 `modelEvaluationStarted=false`、`onnxIntroduced=false`。只有在以下条件同时满足后才允许提出模型方案：

1. 产品数据、标注、rubric、基准机和阈值均有确认来源与 hash；
2. classic 1.0.0 在冻结最终评估集上实际违反至少一项已确认门槛；
3. 失败不是输入质量、标注分歧、媒体映射或运行环境造成；
4. 候选模型能单独报告相对 classic 的效果收益、时间误差、人工修正量、性能、内存、CPU/GPU、
   包体、确定性、模型来源与许可证影响。

满足上述条件也只形成模型引入建议，实际增加 ONNX Runtime、模型文件或发布依赖仍须新决定。
目前没有 classic “未达门槛”的合法结论，故未启动模型评估。

## 6. 当前阻塞与下一步

1. `H-013`：product-manager-01 需在相同类别和分区内替换 4 个超窗来源并升级 A-031；新清单
   还要让实际 probe/裁决覆盖满足每个技术 slice 至少 3 条且至少 1 条 final，尤其补足 VFR。
2. `H-014`：用户需组织 5 名独立真人（至少 3 名有经验）完成初标、复核/裁决和盲评，并让
   TIGER 在接通电源 + Windows 最佳性能模式下允许同一 Release PE 原生启动。

两个输入都就绪后，负责人才能冻结完整媒体、probe、初标/复核/裁决 hash，按一次预热 + 五次
正式测量运行 classic，再输出效果、自然度、人工修正量和性能门禁。

## 7. 本轮真实证据与门禁输出

- 来源审计：[`source-availability-audit-v1.json`](../evidence/T-029/source-availability-audit-v1.json)，
  文件 SHA-256 `d6f9a4711cbc91cad90215f675d39bd25f7039ddc71cca3ed99547da901301db`。
- 数据 manifest：[`product-dataset-manifest-v1.json`](../evidence/T-029/product-dataset-manifest-v1.json)，
  文件 SHA-256 `a97443151c8b1dc1894227ade465ffbd532c4498eca554fe3dd6513724bb6a05`，内容摘要
  `2046cb6055784029ca04d6a79114fbdf074385a79dbc9dc0b852531c750ed571`；36/36 媒体 hash 复核一致。
- A-031 0.2 SHA-256：`88467ff4ccbb07148fdbfc944b42fb52bec647cf7e1c4cabe8a35434ef15f7ff`。
- 4 个失效时间窗：`SR-BILI-GAME-004`、`SR-BILI-TRAVEL-001`、`SR-BILI-TRAVEL-007`、
  `SR-BILI-LIFE-007`。不得由算法负责人静默缩短或替换。
- 原始/代理媒体位于被 `.gitignore` 排除的 `out/evaluation/T-029/media`；manifest 未保存临时签名 URL。
- 数据与配额门禁：`fail`。产品效果、自然度、人工修正、性能、总体和模型资格：`not-evaluated`。
- classic：`not-run`；模型方案：`not-created`；ONNX：`not-introduced`。
