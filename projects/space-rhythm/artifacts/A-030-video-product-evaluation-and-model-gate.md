# 视频产品效果、性能与可选模型门禁评估

- 项目：space-rhythm
- 成果 ID：A-030
- 负责人：video-algorithm-engineer-cv-01
- 关联任务：T-029
- 版本：0.3
- 更新日期：2026-09-14
- 状态：draft
- 适用范围：代表产品视频、人工标注、经典视频算法效果/性能和“卡点自然”评估，以及可选模型门禁；不批准产品口径，不把合成样本替代产品数据，不直接引入 ONNX 或修改核心时间线。
- 来源及输入版本：用户于 2026-09-14 明确授权继续 T-029 的替换来源获取与配额复核，但暂不运行正式 classic 门禁；D-001～D-010 confirmed；A-002 0.2、A-016 0.1、A-028 0.2、A-029 0.1、A-031 0.3 approved；T-028 提交 `0513aa3`。
- 批准依据：尚无。
- 版本记录：2026-09-14，0.1，登记 T-029 启动、输入就绪审计、评估协议草案和模型门禁边界；没有产品效果或性能通过结论。2026-09-14，0.2，按 A-031 0.2 实际审计 40 个来源并冻结 36 个可用代理的媒体/probe hash；4 个批准时间窗超出源时长、实际 VFR 配额为 0、独立真人证据和合规 Windows 原生运行条件未就绪，故记录真实阻塞和 `not-evaluated`。2026-09-14，0.3，按 D-010/A-031 0.3 获取并 probe 4 个替换来源，完整数据恢复 40 条和结构配额；但 3 个 VFR 目标的所选平台流均实测为 CFR，语义 slice 又须等待真人裁决，故 H-013 不关闭且未运行 classic。

## 1. 输入与执行就绪结论

首次缺输入状态保留在 [`input-readiness-v1.json`](../evidence/T-029/input-readiness-v1.json)。
随后 D-009/A-031 0.2 确认产品基线；D-010/A-031 0.3 又批准 4 个同类别/同分区替换来源。
负责人实际取得并 probe 全部 4 个替换项，保留原 36 个代理，现有 40 个静音、最大 720p 的
内部测试代理均已冻结且没有把视频字节提交到 Git。当前机器可读收口见
[`execution-readiness-v3.json`](../evidence/T-029/execution-readiness-v3.json)。

| 输入/前置 | 实际结果 | T-029 判定 |
|---|---|---|
| 来源与权限 | D-009 允许本项目内部测试，不对外分发；不构成第三方权利法律结论 | `confirmed_boundary` |
| 来源时间窗 | A-031 0.3 的 40 项均成功取得，4 个替换来源和时间窗由 D-010 批准 | `pass` |
| 媒体与 probe | 40 项已冻结逐 clip 媒体、probe、帧时间 hash；合计 1,359,437,599 bytes / 1,815.044 s，40/40 SHA 复核一致 | `pass` |
| 分区/类别 | 实际 `4/16/20`，舞蹈/游戏/旅行/生活各 10 | `pass` |
| VFR slice | D-010 的 3 个 VFR 目标所选平台流与代理均为 CFR；全数据实际 CFR 40、VFR 0 | `fail` |
| slow_motion 等语义 slice | 静态预期标签满足 `slow_motion=4/final=4`，其余声明 slice 也满足计数；但 A-031 要求真人裁决确认 | `not-evaluated` |
| 人工初标/复核/裁决/盲评 | 尚无 5 名独立真人及相应记录/hash | `missing` |
| rubric 与阈值 | A-031 0.3 继承 D-009 已确认口径 | `confirmed` |
| Windows 基准 | TIGER 硬件已确认；当前活动电源方案是“平衡”，Release PE 又在 `main` 前被 WDAC/SAC 以 `0xC0E90002` 拒绝 | `blocked` |

A-031 明确要求先冻结裁决标注，才能运行 classic；本轮用户也明确禁止越过该前置。当前裁决标注 hash 为空，所以负责人没有
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

A-031 0.3 继承已确认的随机化盲评、固定 clip/时间线/试听映射、逐评审者原始记录和 classic/
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

A-031 0.3 第 6～7 节继承已确认的全部比较符、单位和聚合方式：shot/motion/action 的 overall
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

1. `H-013`：A-031 0.3 的 4 个替换项已全部获取，但 3 个 VFR 目标实测仍为 CFR；依据 D-010，
   product-manager-01 必须再次在相同类别/分区提交版本化替换及新决定，H-013 不能关闭。
2. `H-014`：用户需组织 5 名独立真人（至少 3 名有经验）完成初标、复核/裁决和盲评，并让
   TIGER 在接通电源 + Windows 最佳性能模式下允许同一 Release PE 原生启动。

VFR 来源、真人证据与 Windows 输入都就绪后，负责人才能冻结完整媒体、probe、初标/复核/裁决 hash，按一次预热 + 五次
正式测量运行 classic，再输出效果、自然度、人工修正量和性能门禁。

## 7. 真实证据与本轮门禁输出

- 来源审计：[`source-availability-audit-v1.json`](../evidence/T-029/source-availability-audit-v1.json)，
  文件 SHA-256 `d6f9a4711cbc91cad90215f675d39bd25f7039ddc71cca3ed99547da901301db`。
- A-031 0.2 的历史数据 manifest 仍保留为 [`product-dataset-manifest-v1.json`](../evidence/T-029/product-dataset-manifest-v1.json)。
- 当前数据 manifest：[`product-dataset-manifest-v2.json`](../evidence/T-029/product-dataset-manifest-v2.json)，
  文件 SHA-256 `2032a00762364e696066298b91534eb2157bedb2f1f2c19b407ba9f628dee68b`，内容摘要
  `225e8eafbfaf1ee4624b682ba8700d34ee3e185803bdd775a129a7f027b51959`；40/40 媒体 hash 复核一致。
- A-031 0.3 SHA-256：`4f703ac8b9d6a2e83f0157a9840c431436c619bccd0f3e8656cc628b1efd9d88`。
- 4 个替换来源的所选平台流和代理 PTS 保持检查全部通过，但 `GAME-004`、`TRAVEL-001`、
  `TRAVEL-007` 三个 VFR 目标都实测为 CFR；页面描述或预期标签不能覆盖 probe。
- 原始/代理媒体位于被 `.gitignore` 排除的 `out/evaluation/T-029/media`；manifest 未保存临时签名 URL。
- 来源时间窗、40 条结构、分区和类别配额：`pass`；VFR 配额：`fail`；slow_motion 等语义
  slice：`not-evaluated`（等待裁决标注）。产品效果、自然度、人工修正、性能、总体和模型资格：`not-evaluated`。
- classic：`not-run`；模型方案：`not-created`；ONNX：`not-introduced`。
