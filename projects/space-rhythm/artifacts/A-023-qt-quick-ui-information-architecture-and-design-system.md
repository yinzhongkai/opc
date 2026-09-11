# Qt Quick/QML 信息架构、交互线框与基础设计系统

- 项目：space-rhythm
- 成果 ID：A-023
- 负责人：ui-engineer-qt-quick-01
- 关联任务：T-024
- 版本：0.1
- 更新日期：2026-09-11
- 状态：draft
- 适用范围：一期 Windows x64 独立桌面应用的导入、分析、编辑、试听、保存和导出 UI 设计，以及 T-025/T-026 所需的 Qt Quick/C++ ViewModel 与 mock 边界；不包含 QML 实现、媒体/算法计算、离屏导出实现或产品视觉批准。
- 来源及输入版本：D-001～D-008 confirmed；A-002 0.2、A-004 0.5、A-005 0.4、A-006 0.1 WP-03、A-011 0.1、A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1、A-018 0.2、A-019 0.2、A-020 0.1、A-021 0.1、A-022 0.1；当前公开核心、系统、媒体、音频和图形接口。
- 批准依据：尚无。任务完成不自动批准本成果；第 10 节列出的产品未知项继续待确认。
- 版本记录：2026-09-11，0.1，首次形成完整信息架构、工作区线框、状态矩阵、交互规则、可访问性与设计令牌，以及 ViewModel/mock API 清单。

## 1. 设计结论与边界

一期使用一个持续存在的桌面编辑工作区，不把六个阶段拆成会丢失上下文的逐页向导。导入—分析—编辑—试听—保存—导出是同一项目快照上的阶段状态；工具栏负责发起动作，左右面板负责选择和参数，中央预览与时间线持续保留，底部任务抽屉承载后台进度与诊断。

设计遵守以下边界：

- QML 只负责布局、绑定、输入、轻量转场和可访问语义；媒体遍历、PTS 换算、FFT、混音、几何生成、文件保存和 worker 协调均在 C++/worker。
- `core::TimeNs`、`TimelineRevision`、稳定 ID、事务和不可变快照是权威值。QML 不以 JavaScript `Number` 保存或计算纳秒与 64 位修订号。
- 大量事件、波形、频谱和视觉模板由 `SceneGraphRenderItem` 批量绘制，不创建“一事件一个 QML Item”。
- 分析、预览渲染和导出必须绑定输入指纹、参数摘要和时间线修订；旧结果不静默覆盖新编辑。
- 取消是请求而不是瞬时完成；界面在 worker 确认前保持“正在取消”。失败必须说明阶段、项目是否安全、可恢复动作和诊断 ID。
- T-019 尚未实现预览同步与安全导出事务，T-035 尚缺 T-025。本成果只定义 UI 契约，不宣称这两条运行路径已经可用。

## 2. 信息架构与导航

### 2.1 主流程

```text
启动/无项目
  ├─ 新建项目 ─┐
  ├─ 打开项目 ─┼─> 恢复检查 ─> 工作区
  └─ 最近项目 ─┘                  │
                                  ├─ 导入素材 ─> 探测/选流/代理准备
                                  ├─ 分析 ─────> 候选结果 ─> 审阅/合并到时间线
                                  ├─ 编辑 ─────> 事务提交 ─> 撤销/重做/保存点
                                  ├─ 试听 ─────> 混音 PCM + 同修订视觉预览
                                  ├─ 保存 ─────> 原子保存/自动保存/另存为
                                  └─ 导出 ─────> 参数检查/后台任务/结果定位
```

分析和模板调整可以反复进行；用户不必按固定顺序完成每个阶段。依赖不满足时动作保持可见但禁用，并给出原因，例如“请先导入含音轨的素材”。

### 2.2 顶层结构

| 层级 | 内容 | 导航规则 |
|---|---|---|
| 应用级 | 文件、编辑、播放、视图、帮助；窗口与最近项目 | 原生菜单可由键盘进入；危险关闭动作先处理未保存状态 |
| 项目级 | 导入、分析、编辑、试听、保存、导出六阶段状态 | 是状态/动作入口，不切走工作区，不隐藏正在运行任务 |
| 工作区左侧 | 素材、轨道；流选择、素材缺失与重定位入口 | 单选决定预览对象，多选只用于批量素材操作 |
| 工作区中央 | 视觉预览、运输控制、时间尺、轨道与播放头 | 预览和时间线共享当前修订、可见时间范围和选择 |
| 工作区右侧 | 选择检查器、视觉模板、音色映射、导出设置 | 随选择切换内容；没有选择时显示可执行说明 |
| 工作区底部 | 后台任务、警告、失败、诊断 ID | 默认折叠为一行；有失败/需操作状态时自动展开一次 |
| 模态层 | 导入、恢复、素材重定位、另存为、导出确认、未保存关闭 | 打开时限制焦点；关闭后恢复到触发控件 |

### 2.3 路由与深链接标识

T-025 的应用状态只需三条顶层路由：`start`、`workspace`、`fatal`。工作区内部使用 `activePane` 和稳定选择 ID，不为每个面板创建页面历史。诊断和自动化可使用以下稳定位置标识：

- `workspace/assets`、`workspace/tracks`
- `workspace/preview`、`workspace/timeline`
- `workspace/inspector/selection`、`workspace/inspector/template`、`workspace/inspector/export`
- `workspace/jobs`

## 3. 页面布局与关键线框

### 3.1 设计参考尺寸

参考画布为 1440 × 900 逻辑像素：顶部菜单 28、阶段工具栏 48、状态栏 24；左栏 260、右栏 320；中央预览与时间线最小宽度 520；任务抽屉展开高度建议 160～240。所有数值均为逻辑像素而非物理像素。

```text
┌ 文件 编辑 播放 视图 帮助 ───────────────────── 项目名 * ─ 窗口控制 ┐
├ [导入] [分析] [编辑] [试听] [保存] [导出] ────────────── 修订 r42 ┤
│ 素材 / 轨道       │ 视觉预览（当前模板、当前修订）       │ 检查器      │
│ ┌───────────────┐ │ ┌───────────────────────────────┐ │ 事件/模板/   │
│ │ video.mp4  就绪│ │ │                               │ │ 音色/导出    │
│ │ audio.wav 分析中│ │ │            画面               │ │ 参数         │
│ │ missing  需定位│ │ └───────────────────────────────┘ │              │
│ └───────────────┘ │  |<  ◀  [播放]  ▶  >|  00:12.340 │              │
│                    ├───────────────────────────────────┤              │
│                    │ 时间尺  - 100% +  [吸附] [循环]    │              │
│                    │ 视频  ──◆────◇────────│────────── │              │
│                    │ 节拍  ─◆──◆──◆──◆────│────────── │              │
│                    │ 手动  ─────■──────────│────────── │              │
├────────────────────┴───────────────────────────────────┴──────────────┤
│ 任务：音频分析 42%  阶段：谱变化     [取消]        诊断/展开          │
├ 项目已修改 · 自动保存成功 │ 素材 2 │ 事件 128 │ 00:00—02:41 ───────┤
```

### 3.2 无项目与空项目

```text
┌ Space Rhythm ────────────────────────────────────────────────────────┐
│                                                                     │
│                  从素材开始创建节奏项目                              │
│                  [ 导入视频或音频 ]                                  │
│                  [ 打开项目 ]                                        │
│                                                                     │
│                  最近项目（若存在）                                  │
└─────────────────────────────────────────────────────────────────────┘
```

空项目进入 `workspace` 后保留完整工作区骨架，中央显示“导入素材后可分析”，主动作是“导入”；不得以空白画布或无限加载占位。

### 3.3 恢复、失败和导出对话框

```text
恢复项目                            导出
┌──────────────────────────────┐   ┌──────────────────────────────┐
│ 检测到较新的自动保存         │   │ 范围：全部 / 选区            │
│ 自动保存：09:42，修订 r42    │   │ 视觉模板：当前模板            │
│ 主文件：  09:36，修订 r38    │   │ 音频：当前混音                │
│ [恢复自动保存] [打开主文件]  │   │ 格式：待产品/媒体确认          │
│ 原文件不会被覆盖。           │   │ 输出路径：[选择…]              │
└──────────────────────────────┘   │ [开始导出] [取消]              │
                                   └──────────────────────────────┘

任务失败
┌─────────────────────────────────────────────────────────────────────┐
│ 音频分析失败 · 项目和原素材安全                                     │
│ 阶段：decode_audio   诊断 ID：D-7F42   [重试] [查看详情] [复制 ID]  │
└─────────────────────────────────────────────────────────────────────┘
```

恢复选择不得覆盖主文件；恢复成功后项目标记为 dirty，用户明确保存才写入主文件。导出格式未确认时 mock 只显示“格式待确认”，不得默认选择 MP4/H.264。

## 4. 状态模型与矩阵

### 4.1 全局与项目状态

| 状态 | 页面表现 | 可用动作 | 禁止/保护 | 离开或恢复 |
|---|---|---|---|---|
| `noProject`（空闲） | 启动页或空工作区，明确主动作 | 新建、打开、导入 | 分析、编辑、试听、保存、导出禁用 | 打开后进入加载 |
| `loadingProject`（加载） | 工作区骨架 + 阶段文案；不显示伪进度 | 取消打开（若可中断） | 全部写操作、再次打开 | 成功进入 ready；失败回到安全起点 |
| `readyClean` | 完整工作区，“已保存” | 导入、分析、编辑、试听、导出 | 无 | 首次语义变更进入 dirty |
| `readyDirty` | 标题和状态栏显示 `*`/“未保存” | 保存、另存为、撤销、常规编辑 | 无 | 语义等同保存点后可恢复 clean；不以修订号判断 dirty |
| `saving` | 保存动作显示忙；其余读取保持响应 | 取消关闭、查看任务 | 并发保存同一路径、覆盖式关闭 | 原子成功后 clean；失败仍 dirty |
| `recoverable`（恢复） | 恢复对话框列主文件/自动保存时间与修订 | 恢复、打开主文件、取消 | 静默覆盖、自动选择损坏副本 | 恢复后 dirty；原文件不动 |
| `readOnly`（只读） | 全局只读条、控件保留但禁写 | 浏览、定位、试听现有结果、导出当前快照、另存为 | 导入、合并、移动、删除、参数提交、原路径保存 | “另存为”成功后进入可写副本 |
| `fatal` | 独立错误页，提供诊断 ID | 返回启动页、复制诊断、退出 | 在未知数据状态继续编辑 | 不破坏已存在项目/自动保存 |

### 4.2 素材、分析、预览与任务状态

| 对象/状态 | 可见反馈 | 主动作与规则 | 终态/异常 |
|---|---|---|---|
| 素材 `empty` | 空态与导入入口 | 导入文件 | 不创建伪素材 |
| 素材 `probing`/`loading` | 行内忙指示 + “正在读取媒体信息” | 可取消本次导入 | 保持其他素材可用 |
| 素材 `ready` | 时长、流类型、缩略图状态 | 选流、分析、预览 | VFR 只显示事实，不在 QML 重算 PTS |
| 素材 `missing` | “素材已移动”及原路径 | 定位、搜索已授权目录、移除引用 | 指纹不符不得自动替换 |
| 素材 `failed` | 错误类别、阶段、诊断 ID | 重试、移除、详情 | 损坏/不支持要区分 |
| 分析 `idle` | 显示可分析对象和参数摘要 | 开始分析 | 参数无效时解释禁用原因 |
| 分析 `queued` | 任务排队和顺序 | 取消 | 未收到 accepted 也可发取消请求 |
| 分析 `running`（处理） | 确定进度或不确定忙指示、当前阶段 | 取消、后台继续 | 不冻结工作区；结果绑定 base revision |
| 分析 `cancelling`（取消中） | 文案“正在取消”，按钮禁用 | 等待 worker 确认 | 不提前显示“已取消”或 100% |
| 分析 `success` | 候选数量、置信摘要、审阅入口 | 审阅并以事务合并 | 旧修订结果进入 conflict review |
| 分析 `lowConfidence` | 警告 + 原因码，不伪造稳定节拍 | 审阅候选、调整已允许参数后重试 | 可无候选；不是技术失败 |
| 分析 `noSignal` | “未检测到有效信号” | 换流/素材、返回 | 不生成假 BPM 或假事件 |
| 分析 `failed` | 阶段/安全性/重试性/诊断 ID | 可重试时重试、详情 | 已有时间线保持不变 |
| 分析 `cancelled` | 终态摘要 | 重新开始 | 丢弃未完成结果引用 |
| 预览 `priming` | 显示正在准备同修订音画 | 停止 | 未就绪时不移动成“播放中” |
| 预览 `playing`/`paused` | 播放头、时间码、播放/暂停状态 | 播放、暂停、停止、定位 | 设备错误不破坏离线 PCM |
| 预览 `seeking` | 幽灵播放头 + 目标时间 | 松开/Enter 提交，Escape 取消 | 失败回到已确认时间 |
| 导出 `queued/running/cancelling` | 使用统一任务行 | 取消、查看输出目录（成功后） | 同 JobStatus 语义 |
| 导出 `failed` | 临时文件处理说明、目标是否安全、诊断 ID | 重试、另选路径 | 不把半成品改名为最终文件 |
| 导出 `succeeded` | 完成、路径、摘要 | 打开所在位置、再次导出 | 只有 worker `succeeded` 才显示 100% |

### 4.3 并发和状态优先级

同一对象只能有一个主状态，但提示可叠加。优先级为：`fatal/error requiring action` > `recoverable/conflict` > `readOnly` > `cancelling/running/loading` > `dirty` > `ready/idle`。例如只读项目中的预览设备失败，顶部保留只读条，预览面板显示设备错误，不能用一个红色全屏遮罩覆盖仍可浏览的数据。

## 5. 关键交互规则

### 5.1 时间线

1. 时间真值始终是 C++ `TimeNs`；QML 只接收格式化时间文本、当前可见范围和用于绘制的不可变快照。像素↔时间换算调用 A-021 的整数 subpixel 坐标契约。
2. 单击选事件；`Ctrl` 单击增减选择；空白单击清除；框选只对当前可见轨道和时间范围生效。选择 ID 稳定，不以 model 行号保存。
3. 双击轨道空白添加 `manual` 事件；预览标记先显示为 ghost，提交 `AddEvent` 成功后变为实体。只读、锁定轨道或旧修订时禁止提交。
4. 拖动事件时 C++ 保存基准修订和初始时间，QML 只上报 pointer x；C++ 返回精确候选时间和吸附说明。松开提交一次 `MoveEvent` 事务，`Escape` 取消，不在每个鼠标移动事件写领域模型。
5. 默认吸附目标的产品选择待确认。能力必须支持“可见网格、相邻事件/候选、帧边界、关闭吸附”；按住 `Shift` 临时反转当前吸附状态。键盘左右键移动一个当前网格步，`Shift+左右` 移动十步。
6. 锁定事件显示锁图标和不同形状，不只依赖颜色。移动、删除和批量偏移前必须解锁；“解锁并移动”如由一次意图触发，应在同一事务按顺序提交。
7. 多选批量偏移使用一个 `BatchOffset` 事务；越界或锁定冲突按核心 fail-closed 规则整体拒绝，UI 不做部分成功猜测。
8. 连续键盘微调可使用稳定 `coalescing_key` 合并为一个撤销步；鼠标一次按下—释放也是一个撤销步。撤销/重做传当前修订，失败后重新取快照。
9. 分析候选与已合并事件视觉分层；用户先审阅，再触发 `MergeCandidates`。人工/锁定事件被保护时显示 suppression report，而不是让用户误以为候选丢失。
10. 拖动期间若收到新修订，取消 ghost、取新快照、恢复选择并播报“时间线已更新，请重试”；不得把旧基准请求自动重放。
11. 缩放以指针或键盘焦点时间为锚；`Ctrl+滚轮`/`+`/`-` 缩放，普通滚轮纵向滚轨，`Shift+滚轮` 横向平移。Home/End 定位项目首尾，不改变选择。
12. 高密度标记、波形、频谱使用 A-022 批量几何和 LOD。QML overlay 只绘制播放头、交互 ghost、焦点和少量选中对象。

### 5.2 试听与视觉预览

- 预览请求必须绑定同一个 timeline revision、模板参数摘要、音色参数摘要和时间范围；音频或视觉任一仍基于旧修订时，播放按钮显示原因并禁止开始“同步试听”。
- C++ 播放器维护 `stopped/priming/playing/paused/seeking/error`；QML 不用动画时钟驱动播放头。目标方案以已消费音频样本为主时钟，实际同步和 seek 行为由 T-019 实现及验证。
- 拖动播放头只更新 seek ghost 和时间文本；释放后发一次定位请求。收到确认后才切换真实播放头；`Escape` 恢复原位置。
- 播放期间编辑会产生新修订。默认停止旧修订播放并提示“内容已更改，正在准备预览”；是否允许延迟到暂停再切换属于待确认产品策略。
- 循环区使用半开时间范围 `[start,end)`；区间为空或超出项目范围时不开始循环。
- `QtAudioPreview` 只播放 `DeterministicMixer` 已生成的 PCM。`device_unavailable`、`format_unsupported` 或 sink 初始化失败只影响设备试听，不改写离线混音结果。
- 设备恢复、Scene Graph invalidation 或窗口换屏时保持项目与播放位置；视觉节点按 device generation 重建。未完成 T-035 前不承诺屏上/离屏像素一致性或 GPU 降级性能。

### 5.3 模板参数

| 模板 | 稳定 ID/版本 | UI 分组 |
|---|---|---|
| 波形/示波器 | `space-rhythm.waveform-oscilloscope` / `1.0.0` | 波形幅度、线宽、主辅色、事件标记、时间线区域、LOD/预算高级项 |
| 频谱几何 | `space-rhythm.spectrum-geometry` / `1.0.0` | 幅度、柱间距、最小柱高、主辅色、事件标记、LOD/预算高级项 |
| 节奏线条脉冲 | `space-rhythm.rhythm-line-pulse` / `1.0.0` | 抖动、寿命、每事件数量、运动距离、线宽、颜色、LOD/预算高级项 |

- 参数模型提供类型、整数最小值、最大值、工程默认值、单位、当前值和验证信息；QML 不复制范围常量。
- A-022 的默认值是模板 1.0.0 的工程默认值，不是产品批准值。UI 动作为“恢复模板工程默认值”，不得写成“推荐风格”。
- 输入先在编辑缓冲验证；合法变更由 C++ 规范化、计算 `parametersDigestSha256`、创建新 `RenderRecipe`/`RenderSnapshot`。新快照就绪前继续显示旧快照并标记“正在更新”。
- Slider 拖动可本地预览数值，但 C++ 应节流/合并快照请求；释放、Enter 或失焦提交一个可撤销参数意图。预算类高级参数只允许 SpinBox/文本精确输入。
- 模板切换保留各模板各自的最后合法参数，但项目保存必须记录模板 ID、版本和完整参数块；未知版本 fail closed，不能套用当前默认值猜测迁移。

### 5.4 任务与进度

- 统一映射 `queued/running/cancelling/succeeded/failed/cancelled`，任务行包含操作、素材、阶段、进度、开始时间、取消/重试、诊断入口。
- `progressPpm` 显示为 0～100%，但只在 worker 提供可信分母时使用确定进度；否则显示不确定指示和阶段文本，不生成假百分比。
- 点击取消只调用 `JobCoordinator.cancel(requestId)`，状态进入 `cancelling`。只有 `cancellation_acknowledged`/`cancelled` 更新后才进入终态。
- 任务完成结果若 `resultBaseRevision` 与当前修订不一致，标记“结果基于旧版本”，提供审阅/重新运行，禁止自动合并。
- worker 断开时运行任务进入失败并显示重连/重试；项目和已提交时间线保持可用。重复 request ID 采用 coordinator 的幂等/重放语义，UI 不自行复制任务。
- 成功任务可折叠保留本会话摘要；失败、冲突、取消中任务不可被“全部清除”隐藏。进度无变化时不向屏幕阅读器高频播报。

## 6. 快捷键、焦点、高 DPI 与可访问性

### 6.1 快捷键基线

| 快捷键 | 动作 | 生效条件 |
|---|---|---|
| `Ctrl+N` / `Ctrl+O` | 新建 / 打开项目 | 模态层关闭；先处理未保存状态 |
| `Ctrl+I` | 导入素材 | 项目可写 |
| `Ctrl+S` / `Ctrl+Shift+S` | 保存 / 另存为 | 有项目；只读只允许另存为 |
| `Ctrl+Z` / `Ctrl+Y` | 撤销 / 重做 | 时间线有对应历史且非只读 |
| `Space` | 播放 / 暂停 | 非文本输入且预览同修订就绪 |
| `K` | 停止 | 预览非 stopped |
| `Home` / `End` | 项目开始 / 结束 | 预览或时间线有焦点 |
| `Delete` | 删除选择 | 选择可写且未锁定 |
| `L` | 锁定 / 解锁选择 | 时间线有焦点；非文本输入 |
| `←` / `→` | 按当前网格步微调 | 单/多选可移动事件 |
| `Shift+←` / `Shift+→` | 十个网格步微调 | 同上 |
| `+` / `-` / `0` | 放大 / 缩小 / 适合项目 | 时间线有焦点 |
| `F6` / `Shift+F6` | 下一个 / 上一个工作区面板 | 非模态状态 |
| `Ctrl+J` | 展开/折叠任务抽屉 | 工作区 |
| `Esc` | 取消 ghost/拖动/定位，或关闭顶层弹窗 | 先取消局部交互，不直接丢弃项目 |

菜单中显示快捷键；与文本输入冲突时文本编辑优先。所有动作经同一 C++ command enablement，快捷键不得绕过按钮的只读、锁定或忙状态。

### 6.2 焦点顺序

默认顺序为：菜单栏 → 阶段工具栏 → 左侧素材/轨道 → 预览运输控制 → 预览画布 → 时间线工具栏 → 时间线画布 → 右侧检查器 → 任务抽屉 → 状态栏可操作项。`F6` 在这些区域间循环，区域内部保持可预测的 Tab 顺序；列表进入后用方向键导航，Tab 离开列表，避免数百行进入全局 Tab 链。

模态对话框限制焦点在内部，首焦点放在标题后的第一个安全动作而非破坏性动作；关闭后恢复触发控件。事件拖动、模板切换、模型刷新和错误条出现不得无故抢焦点。删除选中项后焦点落到相邻项或轨道本身。

### 6.3 高 DPI、缩放与窗口重排

- 尺寸、间距、命中和画笔宽度使用逻辑像素；场景图输出按实际 device pixel ratio 创建，不在 QML 手工乘缩放因子。
- 字体使用 Qt/系统字体度量，不以固定像素高度裁剪；支持 Windows 文本缩放 100%、125%、150%、200%。图标使用矢量或每 DPR 资源，1 逻辑像素分隔线对齐物理像素。
- 响应式规则按窗口逻辑宽度而非屏幕分辨率：`>=1280` 显示三栏；`960～1279` 右侧检查器变为可切换抽屉；`<960` 左右面板互斥抽屉并保留中央预览/时间线。最低受支持窗口尺寸仍待产品确认，任何尺寸都不得让保存/取消/关闭等安全动作被裁掉。
- 换屏或 DPR 变化时保存时间范围、选择、滚动和播放位置，重建图形资源，不重新分析素材。
- UI 自动化记录 QPA、RHI、GPU、DPR、文本缩放、字体和 locale；DPI 截图基线与像素容差需在 T-026/T-035 输入明确后评估。

### 6.4 可访问性

- 每个按钮、字段、列表、时间线、进度和错误提供稳定 `objectName`、可访问角色、名称、描述和状态；图标按钮必须有可见标签或 accessible name。
- 颜色不单独表达含义：选中同时有轮廓/形状，锁定有锁图标与文本，错误有图标/标题，轨道/候选来源有标签或线型。
- 焦点环对所有键盘可操作控件可见；不得因 hover、selected 或 error 覆盖焦点环。Windows 高对比模式优先系统颜色并移除依赖背景图的语义。
- 动态播报使用克制的 live region：任务开始、阶段变化、每跨越约 10% 或完成/失败/取消时播报；拖动播放头和每帧渲染不播报。
- 时间线提供非画布等价路径：选中事件可在检查器读取/编辑精确时间、类型、强度、置信度和锁定；键盘可完成添加、移动、锁定、删除、撤销和重做。
- 非必要转场建议 120～180 ms，并服从系统减少动态效果设置；播放头、进度等表达真实状态的运动可以保留但不闪烁。
- 细指针最低有效命中 32 × 32 逻辑像素；触摸/粗指针约 44 × 44。最终 WCAG/组织级门槛、屏幕阅读器矩阵和最小字体要求待确认，T-026 只能按已确认门槛给出通过结论。

## 7. 基础设计系统

### 7.1 语义颜色（工程草案）

视觉风格尚未获产品批准。以下颜色只为 T-025 可替换的语义令牌基线，组件不得直接绑定业务含义到十六进制值。

| 令牌 | 浅色基线 | 深色基线 | 用途 |
|---|---|---|---|
| `color.canvas` | `#F6F8FA` | `#0D1117` | 应用背景 |
| `color.surface` | `#FFFFFF` | `#161B22` | 面板、菜单、对话框 |
| `color.surfaceRaised` | `#EAEEF2` | `#21262D` | 悬浮/分组表面 |
| `color.border` | `#D0D7DE` | `#30363D` | 结构边界 |
| `color.textPrimary` | `#24292F` | `#F0F6FC` | 主文本 |
| `color.textSecondary` | `#57606A` | `#A8B3C0` | 次要文本 |
| `color.accent` | `#0969DA` | `#35C2FF` | 主操作、激活播放头 |
| `color.onAccent` | `#FFFFFF` | `#071018` | accent 上文字 |
| `color.selection` | `#DDF4FF` | `#143D52` | 选区背景，仍需轮廓 |
| `color.focus` | `#0969DA` | `#6ED5FF` | 2 px 焦点环 |
| `color.success` | `#1A7F37` | `#56D364` | 成功文字/图标 |
| `color.warning` | `#9A6700` | `#E3B341` | 警告/低置信 |
| `color.error` | `#CF222E` | `#FF7B72` | 错误/破坏性动作 |

颜色对比必须针对实际背景、字号和控件状态自动测试，表中颜色不代替验收。事件类别优先用中性色 + 图形/标签；只有持续且确有必要的类别映射才增加色板。

### 7.2 排版、间距与几何

| 类别 | 工程基线 |
|---|---|
| 字体 | Windows UI 首选 Segoe UI/系统 sans；时间码和数值启用 tabular figures，不强制打包字体 |
| 字号 | 辅助 12、正文/控件 14、强调 16、面板标题 20、空态标题 28 逻辑 px；随系统文本缩放 |
| 字重 | 400 正文，600 标题/强调；不以极细字重表达次要内容 |
| 行高 | 正文约 1.4，控件由字体度量和最小命中共同决定 |
| 间距 | 4 px 基线：`space.1=4`、`2=8`、`3=12`、`4=16`、`6=24`、`8=32` |
| 圆角 | 小控件 4，面板/弹窗 6；时间线与数据区优先直边，避免装饰性卡片泛滥 |
| 边框 | 常规 1，焦点环 2；禁用态不可只降低到难读的不透明度 |
| 动效 | 状态转场 120～180 ms；任务、播放和渲染不使用无意义循环动效 |

### 7.3 组件状态

| 状态 | 视觉 | 行为 |
|---|---|---|
| default | 中性表面、清晰标签 | 可操作 |
| hover | 轻微表面/边界变化 | 不承载 hover-only 必要信息 |
| pressed | 明确按下表面和位移不超过 1 px | 释放后触发；拖出可取消 |
| focus | 2 px `color.focus` 外环 | 键盘操作完整，始终可见 |
| selected | selection 表面 + 实线轮廓/勾选或形状 | 与 focus 可叠加 |
| disabled | 仍可读的次级文本，保留禁用原因入口 | 不接收激活动作 |
| busy | 控件标签变为具体进行态，必要时有进度 | 防重复提交，不阻塞无关控件 |
| error | error 图标、标题、字段说明 | 焦点可移至首个错误；不清空输入 |
| readOnly | 只读标签/锁图标，数值保持正常可读 | 可复制/浏览，不可提交 |
| locked | 锁图标 + 标记形状变化 | 时间线移动/删除拒绝，允许显式解锁 |

首批共享组件清单：`SrActionButton`、`SrIconButton`、`SrTextField`、`SrComboBox`、`SrSpinBox`、`SrSliderRow`、`SrTabBar`、`SrPaneHeader`、`SrStatusBanner`、`SrProgressRow`、`SrEmptyState`、`SrInspectorRow`、`SrDialog`、`SrTooltip`、`SrTimelineView`、`SrPreviewSurface`、`SrTransportBar`。名称为 T-025 建议，不代表代码已经存在。

## 8. ViewModel 与 mock API 清单

### 8.1 桥接原则

- 建议桥接契约从 `uiBridgeSchemaVersion=1`、`uiBridgeContractVersion=0.1.0` 起步；T-025 实现时固化到头文件和契约测试。
- 所有 `QObject`/`QAbstractItemModel` 在 UI 线程发布只读属性和 signal；worker 结果通过队列回到 UI，视图永不持有 worker、FFmpeg、PCM lease、QSG node 或领域可变对象。
- 64 位 `timeNs`、revision 和 frame index 在 QML 层以只读格式化文本、opaque string 或注册的强类型值承载；可显示 `double seconds`，但不得将其作为保存/事务输入。
- 命令返回“已受理/立即拒绝”，异步结果由状态和带 request ID 的 signal/model 更新；QML 不自行推断作业成功。
- mock 和真实 service 实现同一 C++ interface，DTO/schema、异步顺序、错误码和线程语义一致；mock 不执行媒体、DSP 或几何算法。

### 8.2 QML-facing 对象

| 对象 | 关键只读属性/模型 | 命令 | 对齐来源 |
|---|---|---|---|
| `ApplicationViewModel` | `route`、`activePane`、`globalBanner`、`modalState`、`canClose` | `newProject()`、`openProject()`、`requestClose()`、`activatePane(id)` | 应用壳层与 ProjectStore |
| `ProjectViewModel` | `projectId`、`displayName`、`path`、`state`、`dirty`、`readOnly`、`recoveryAvailable`、`timelineRevisionText` | `save()`、`saveAs()`、`recover(source)`、`openPrimary()`、`discardSessionChanges()` | `Timeline::is_dirty/mark_saved`、`ProjectStore` |
| `AssetListModel` | roles：`assetId`、`displayName`、`kind`、`state`、`durationText`、`streamSummary`、`fingerprint`、`thumbnailState`、`error` | 由 `ImportViewModel` 变更，不暴露行号命令 | `AssetReference`、`MediaInfo` |
| `ImportViewModel` | `selectedPaths`、`state`、`probeProgress`、`availableStreams`、`error` | `chooseFiles()`、`probe()`、`selectStreams()`、`cancel()`、`commitImport()`、`relocate(assetId)` | `MediaSource::open/info/select/proxy_frame/thumbnail` |
| `TimelineViewModel` | `snapshotId`、`revisionText`、`viewport`、`playheadText`、`selectionIds`、`canUndo/Redo`、`canEdit`、`snapMode` | `select()`、`beginEventDrag()`、`updateEventDrag(pointerX)`、`commitEventDrag()`、`cancelGesture()`、`addManualAt(pointerX,trackId)`、`removeSelected()`、`setLocked()`、`offsetSelected()`、`undo()`、`redo()` | A-012 transactions、A-021 坐标/命中 |
| `TimelineEventListModel` | 稀疏/检查器 roles：ID、轨道、时间文本、kind、source、strength、confidence、locked、userEdited | 无领域写方法 | `TimelineSnapshot.events`；密集绘制不走此模型 |
| `AnalysisViewModel` | `state`、`requestId`、`assetId`、`stage`、`progressPpm`、`confidence`、`reasonCodes`、`candidateCount`、`baseRevisionText`、`error` | `start(assetId,parameterSetId)`、`cancel()`、`review()`、`mergeSelected()`、`retry()` | `Analyzer`、`AnalysisResult`、`MergeCandidates`、JobCoordinator |
| `PreviewViewModel` | `state`、`positionText`、`durationText`、`activeRevisionText`、`loopRange`、`audioDevice`、`error` | `play()`、`pause()`、`stop()`、`beginSeek(pointerX)`、`commitSeek(pointerX)`、`cancelSeek()`、`setLoopRange()` | `DeterministicMixer`、`QtAudioPreview`；同步待 T-019 |
| `TemplateViewModel` | `templateId/version`、`parametersDigest`、`state`、`parameterModel`、`renderSnapshotId` | `selectTemplate(id)`、`setDraftValue(name,value)`、`commitParameter(name)`、`resetEngineeringDefaults()` | RenderRecipe、TemplateParameterBlock、A-022 definitions |
| `TemplateParameterListModel` | roles：`name`、`labelKey`、`type`、`minimum`、`maximum`、`engineeringDefault`、`value`、`unit`、`advanced`、`validationError` | 无；写入经 TemplateViewModel | A-022 整数参数契约 |
| `TaskListModel` | roles：`requestId`、`operation`、`subject`、`status`、`stage`、`progressPpm`、`retryable`、`diagnosticId` | `cancel(requestId)`、`retry(requestId)`、`dismissTerminal(requestId)` | `JobSnapshot`/`JobUpdate`/JobCoordinator |
| `ExportViewModel` | `state`、`range`、`outputPath`、`formatOptions`、`validationErrors`、`requestId`、`error` | `chooseOutput()`、`validate()`、`start()`、`cancel()`、`retry()` | RenderedFrame/PCM + T-019；格式矩阵待确认 |
| `ErrorPresentationModel` | `category`、`code`、`stage`、`messageKey`、`diagnosticId`、`retryable`、`projectSafety`、`actions` | `copyDiagnosticId()`、`invokeAction(id)` | `core::ErrorInfo`、`system::SystemError`、媒体/预览/渲染错误 |

`TimelineViewModel` 的 pointer 命令由 C++ 接收 item-local 逻辑坐标并调用 `item_x_sp_to_time()`；不得把 QML 计算的浮点秒重新转换为权威 `TimeNs`。命中结果必须同时匹配 snapshot ID 和 revision。

### 8.3 统一 mock 场景

| 场景 ID | 要验证的 UI |
|---|---|
| `empty-project` | 无素材空态、动作 enablement、焦点起点 |
| `media-probing-vfr` | 加载、取消、VFR/流摘要、代理缩略图占位 |
| `media-missing-relink` | 素材缺失、指纹校验和重定位失败 |
| `analysis-running-42` | 42% 确定进度、阶段文本、后台仍可编辑 |
| `analysis-cancelling` | 取消请求等待确认，防重复提交 |
| `analysis-low-confidence` | 原因码、候选审阅、不显示伪 BPM |
| `analysis-no-signal` | 非技术失败空结果 |
| `timeline-dense-selected` | Scene Graph 密集绘制、少量选择 overlay、键盘编辑 |
| `timeline-stale-revision` | 拖动/合并被旧修订拒绝、刷新与焦点恢复 |
| `worker-disconnected` | 运行任务失败、项目仍安全、重试入口 |
| `recovery-autosave-newer` | 主文件/自动保存比较、恢复后 dirty |
| `read-only-project` | 所有写动作一致禁用、另存为可用 |
| `preview-device-unavailable` | 设备试听失败而离线 PCM/项目不受损 |
| `render-device-lost` | 预览占位、generation 重建、状态保留 |
| `export-disk-full` | 安全目标说明、临时结果不冒充完成、另选路径 |

mock 时序必须可控：测试可逐步发出 accepted → progress → cancellation acknowledged → cancelled，或 success with stale base revision；禁止只用定时器永远顺利到 100%。所有对象提供稳定 `objectName` 和 accessible 标识以支持 Qt Quick Test。

## 9. 错误、恢复与数据安全规则

1. 错误先回答三件事：发生在哪个阶段、项目/原素材是否安全、用户下一步能做什么；诊断 ID 可复制，技术堆栈默认折叠。
2. 保存失败保持 dirty，不更新保存点；磁盘不足、partial write 或 commit 前失败不得损坏旧主文件。
3. 自动保存失败不阻塞继续编辑，但状态栏持续警告；关闭前把风险提升为对话框。自动保存周期和保留策略待确认。
4. 恢复结果注明来源 `primary/autosave`；被忽略的恢复副本错误显示为非阻断诊断。恢复后以 dirty 打开，避免误认为已写回。
5. 素材重定位只在 fingerprint 匹配时自动接受；候选过多或超限时让用户缩小目录，不能凭文件名猜测。
6. 缓存损坏显示“将重新生成”，不把可重建缓存错误升级为项目损坏。原始素材和项目 JSON 永不由清缓存动作删除。
7. 只读状态贯穿菜单、按钮、快捷键和拖放；任何入口都调用相同 enablement。另存为创建新写入目标后才解除只读。
8. 导出失败保留项目和已确认快照；输出临时文件由 T-019 的安全导出事务定义清理/恢复，UI 不自行改名。

## 10. 明确待确认项

| 未知项 | 当前处理 | 需要的确认人/后续 |
|---|---|---|
| 产品视觉风格、品牌色、图标与主题策略 | 第 7 节仅为可替换工程令牌；不宣称品牌定稿 | 产品/设计输入后升级 A-023 |
| 产品文案：产品名称、阶段名、错误与空态文案、术语及本地化范围 | 使用功能性中文占位和 `messageKey` | 产品负责人确认文案、语言和术语表 |
| 产品默认音色、音色授权、事件到音色默认映射 | A-018/A-020 的 CC0 音色仅用于测试，UI 不标为产品默认 | 产品与音频负责人确认 |
| 导出容器、视频/音频编码、H.264 后端、预设和默认扩展名 | 导出框架存在，格式显示“待确认” | T-019/发布与许可决策后填充 |
| 默认视觉模板与 A-022 参数产品默认值 | 保留模板工程默认值，不称推荐 | 产品视觉评审后版本化 |
| 默认吸附方式、播放中编辑策略、首次运行引导 | 能力和冲突规则已定义，默认选项不冻结 | 可用性评审/产品确认 |
| 最低窗口尺寸、最低 Windows、屏幕阅读器/输入设备矩阵 | 实现响应式和系统语义，不宣称兼容门禁 | 产品与发布/测试确认 |
| 对比度等级、最小字体、DPI 截图容差、像素一致性容差 | 作为 T-026/T-035 待评估输入 | 产品、测试、图形负责人确认 |
| UI 响应 P95、预览 FPS、任务进度精度、可接受音画漂移 | 只记录 measured/not-evaluated | 基准硬件和性能门槛确认 |
| 自动保存周期、恢复保留数、崩溃后默认选择 | 仅定义非破坏恢复语义 | 产品/系统负责人确认 |
| 代表视频/音频、标注与“卡点自然”门槛 | mock 不代替真实素材或效果评价 | 产品、算法、测试共同确认 |

## 11. T-024 自查与后续边界

| T-024 完成条件 | 覆盖位置 | 自查结果 |
|---|---|---|
| 导入—分析—编辑—试听—保存—导出完整信息架构 | 第 2 节 | 已覆盖主辅路径、依赖与反复迭代 |
| 页面布局、导航和关键线框 | 第 2～3 节 | 已覆盖启动、工作区、恢复、失败、导出 |
| 空闲、加载、处理、取消、失败、恢复、只读状态 | 第 4、9 节 | 已给出动作、安全和终态矩阵 |
| 时间线、预览、模板参数、任务进度规则 | 第 5 节 | 已对齐修订、事务、作业和渲染契约 |
| 快捷键、焦点、高 DPI、可访问性 | 第 6 节 | 已给出行为基线和未确认门槛 |
| 颜色、排版、间距、组件状态 | 第 7 节 | 已形成可替换工程令牌与组件清单 |
| ViewModel/mock API | 第 8 节 | 已映射核心、系统、媒体、音频与图形契约 |
| 未知项明确待确认 | 第 10 节 | 已列出视觉、文案、音色、导出等责任边界 |

T-024 至此具备版本化设计成果并完成负责人自查。下一工程任务仍为 T-025，但本轮按用户要求保持 `todo`，没有修改 `src/app/qml/Main.qml` 或创建 ViewModel 代码。T-035 依赖 T-025，即使 T-032/T-034 已完成也不得提前启动；H-006 在 T-025/T-026 完成前保持 `accepted`。
