# USER-01 个人单用户验收界面

本界面落实 A-031 0.5 / D-014 的三阶段隔离：`reference_authoring`、`blind_rating`、
`manual_correction` 使用不同会话定义和记录。所有实际媒体、浏览器预览、草稿、隐藏盲评答案和
冻结输出都放在被 Git 忽略的 `out/evaluation/T-029/user01/`；仓库只提交工具、协议和不含媒体的
执行证据。

当前只允许制作人工参考。盲评会话必须等 `single_user_reference` 冻结、classic 对应分区运行完毕，
并生成 `classic`/`human_reference` 两个同链路试听版本后再创建；人工修正会话必须等对应片段的
正式盲评提交后再创建。不能通过修改 URL 越过阶段。

## 执行人准备参考会话

从产品 workspace 运行以下命令。它会再次核对 40 个产品代理的媒体 SHA-256，用 FFprobe 完整读取逐帧 PTS，
校验 `frameTimesSha256`，并生成浏览器可播放的本地 H.264 静音预览。原媒体和预览均不提交 Git。

```powershell
Set-Location projects/space-rhythm/workspace
$python = 'C:\Users\YinZh\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $python tests/evaluation/video/prepare_user01_workflow.py prepare-reference
```

仅验证媒体、PTS 和会话结构而暂不转码时加 `--skip-previews`。这种会话的
`previewStatus=pending_generation`，不能交给 USER-01 正式操作。

随后启动只监听本机回环地址、且只暴露验收 UI 与 `out/evaluation/T-029/user01/` 工作区的窄范围服务器：

```powershell
& $python tests/evaluation/video/serve_user01_app.py
```

在 Edge 或 Chrome 打开：

```text
http://127.0.0.1:8765/index.html?session=/out/evaluation/T-029/user01/reference-session-v1.json
```

服务器会拒绝非 loopback 地址、仓库源码路径和目录遍历；不要复制 `out/` 媒体或会话记录到 Git。

## USER-01 制作人工参考

1. 关闭能显示 classic 候选、置信度、音色结果或历史评分的其他窗口；确认页面顶部阶段为
   “1 人工参考”。本阶段播放器固定静音。
2. 按左侧顺序逐条查看完整视频。可拖动、暂停、重播和逐帧定位；会话冻结预览帧与源帧的逐帧
   presentation-order 映射，界面先定位实际显示的预览帧，再保存该帧对应的源真实 PTS
   `timeNs`/`sourceDecodeOrdinal`，不按平均帧率反推。预览转码发生时间量化时也不能拿预览秒数
   直接冒充源 `timeNs`。
3. 在可见落点使用“镜头切换 / 运动峰值 / 动作峰值”按钮；负例先点起点，再移动到终点并点终点。
   按 A-031 语义选择子类和 `rhythmRole`。`uncertain` 必须把占位原因改成实际稳定 token。
4. 前后匹配窗按画面可定位精度填写，不要查看算法误差后放宽。渐变和负例填写实际持续时间。
5. 即使某类没有事件，也要勾选镜头、运动、动作、负例四项“已检查”，再点“完成本片段自查”。
   新增、修改和删除都会保留原始操作日志，不覆盖初始记录。
6. 左侧进度到 `40 / 40` 后点“完整性检查并导出”。把下载的
   `USER01-REFERENCE-PRODUCT-V1-submission.json` 保留在本机并告诉执行人其路径；不要手改 JSON。

执行人用以下命令校验、冻结和计算 `singleUserReferenceSha256`：

```powershell
& $python tests/evaluation/video/prepare_user01_workflow.py freeze `
  --session out/evaluation/T-029/user01/reference-session-v1.json `
  --submission '<USER-01 导出的 JSON 路径>' `
  --frozen-out out/evaluation/T-029/user01/single-user-reference-v1.json
```

冻结成功后必须关闭参考界面。执行人随后才能运行对应分区的 classic；final 参数与映射冻结前，
不得打开 final 人工参考做结果调参。

## 随机化盲评（参考冻结后才提供）

执行人将创建不含答案的 `blind-session-v1.json` 和单独保存的隐藏答案表。正式开始前，USER-01
固定显示器/刷新率/缩放、音频设备/驱动/采样率、系统音量、应用音量、耳机状态和房间环境；这些
字段与随机种子、版本 hash 一并冻结。

实际操作时：

1. 打开执行人提供的盲评 URL，确认页面顶部阶段为“2 随机化盲评”，页面不显示文件名、算法名、
   置信度、时间线、参考记录或先前答案。
2. 对每条 final，A/B 各完整播放至少 1 次；需要时各最多再播 2 次。界面禁止拖动和逐帧查看，
   并自动记录实际完整播放次数。
3. A、B 分别填写五维 1～5 分、`overallNaturalness` 和 `directExportReadiness`，再填
   `A|B|tie`。只能用整数；选 `N/A` 必须填写稳定原因 token。
4. 每条只提交 1 组有效结果。技术故障时保留旧试次为 invalid，由执行人发新 `trialId`，不能从
   多次有效结果中挑有利的一次。
5. 产品主集 20 条 final 和真实 VFR 技术集全部 final 都不得缺失；VFR 素材未就绪时整体仍为
   `not-evaluated`。

## 人工修正（对应盲评提交后才提供）

执行人将为已完成盲评的片段创建 `manual-correction-session-v1.json`。界面此时才显示冻结的
classic 事件；不载入人工参考时间线或此前修正结果。

1. 从 classic 初始时间线开始，用“新增、删除、移动、改类、锁定”完成真实编辑；每次操作、原值、
   新值和时间都会保留。
2. 达到自己认为可直接导出的状态时选择 `direct_export|minor_edit`；无法少量修正时也如实选择
   `major_edit|unusable`，不要为了通过强行完成。
3. 提交后保存最终事件、revision、完整操作日志、活跃编辑时间与项目 hash。原始 UI 操作数和按
   事件归一化的最终修正数分别汇总。

以上三个阶段的 USER-01 是同一人，结果必须标记 `personal-single-user-acceptance`；独立复核、独立
裁决和评审者间一致性分别写 `not_applicable(reason=single_user_scope)` /
`unavailable(reason=single_reviewer_scope)`，不得描述为独立多人产品验证。
