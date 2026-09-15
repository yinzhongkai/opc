# T-022 工程端到端、故障恢复与质量门禁证据

- 项目：space-rhythm
- 成果 ID：A-034
- 负责人：tester-cpp-qt-01
- 关联任务：T-022
- 版本：0.1
- 更新日期：2026-09-15
- 状态：draft
- 适用范围：依据 A-016 执行 Windows x64 工程端到端、故障恢复、黄金样例、Qt/QML、性能测量和当前可测 G0～G4 证据；不修改业务实现，不执行 T-038，不把个人试用反馈、未确认阈值或跳过项写成通过。
- 来源及输入版本：[A-004 0.5](A-004-mvp-technical-feasibility-and-requirements.md)、[A-016 0.1](A-016-cpp-qt-test-strategy-and-traceability.md)、[A-017 0.2](A-017-windows-headless-contract-test-entry-and-evidence.md)、D-015 confirmed、T-029 cancelled、受测提交 `be61c71e9803`。
- 批准依据：尚无。本成果是测试负责人执行记录，不是产品效果、硬件性能、兼容矩阵、可信分发或发布批准。
- 证据：[T-022 验证摘要](../evidence/T-022/verification-summary.md)与[机器可读运行索引](../evidence/T-022/runs-v1.json)。

## 1. 结论

统一入口在干净跟踪工作树上分别完成 Debug、CI/RelWithDebInfo 和严格 Release；三套均为 166/166 pass、0 fail、0 skip。这里的 `pass` 只表示已执行的工程 oracle 满足，不能外推为正式产品性能、产品效果、真实素材或所有 Windows/硬件兼容通过。

四项延期结论在所有结构化测量和本成果中固定为：

- `productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `naturalnessEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `realVfrEvaluation=not-evaluated(deferred-to-personal-use-feedback)`
- `formalProductPerformanceEvaluation=not-evaluated(deferred-to-personal-use-feedback)`

## 2. 可复现入口与终态运行

```powershell
./tooling/windows/Invoke-HeadlessTests.ps1 -Task T-022 -Preset windows-msvc-x64-debug -Clean -Parallel 4
./tooling/windows/Invoke-HeadlessTests.ps1 -Task T-022 -Preset ci-windows-msvc-x64 -Clean -Parallel 4
./tooling/windows/Invoke-HeadlessTests.ps1 -Task T-022 -Preset windows-msvc-x64-release -Clean -Parallel 4
```

| preset | run ID | CTest/JUnit | 结果 |
|---|---|---:|---|
| Debug | `20260915T103339Z-be61c71e9803-windows-msvc-x64-debug-01` | 166/166，0 skip | `pass(engineering-scope)` |
| CI/RelWithDebInfo | `20260915T103555Z-be61c71e9803-ci-windows-msvc-x64-01` | 166/166，0 skip | `pass(engineering-scope)` |
| Release | `20260915T103750Z-be61c71e9803-windows-msvc-x64-release-01` | 166/166，0 skip | `pass(engineering-scope,current-personal-SAC-off-host)` |

固定 seed 为 `0x5350414345524859`；契约状态机使用手动时钟，Qt 事件循环使用有界真实时钟；QPA 为 offscreen，RHI 为软件路径，每次 TEMP/TMP 指向独立 `work/`。环境、输入 SHA-256、CTest 发现/标签、JUnit、stdout/stderr、构建日志、Qt Test 报告、结构化测量和逐文件哈希全部保存在对应原始证据目录；机器可读索引固定关键文件哈希。

Qt GUI 子系统程序成功时不保证标准输出，故入口强制生成独立 Qt Test 文本报告并检查退出码与非空报告。9 个 UI/worker 场景各用独立进程、串行执行，消除测试用例间 worker 生命周期污染；各场景 oracle 未改动。

## 3. 测试分层与工程闭环

| 层 | 本轮实际证据 |
|---|---|
| UT | 核心事件/事务、媒体、视频、音频、渲染、项目/缓存、IPC、同步与导出事务 |
| CT | 时间/溢出、修订/锁定、schema、公开 PTS/VFR/旋转/颜色、渲染 DTO 与错误码 |
| GM | 70 个带 `golden` 标签的媒体生成/审计、音视频算法 oracle 与确定性字节结果 |
| IT | 23 个带 `integration` 标签的真实 bridge、worker、媒体、存储、渲染与导出路径 |
| QQ | Qt Test、Qt Quick Test、QML shell、高 DPI、默认/软件渲染与 9 个独立真实工作流场景 |
| PF | 音频分析、音频渲染、视频分析、seek、A/V 选择差、恢复时延与峰值内存，仅 `measured` |
| FI | 取消、worker 断连、损坏/不可打开媒体、缓存损坏、素材丢失、磁盘/编码失败、异常恢复、并发输出 |
| CO | 三 preset、x64、动态 CRT、固定 Qt/vcpkg/FFmpeg/OpenCV、x86/ARM64 拒绝、当前 offscreen/软件路径 |

工程垂直切片实际执行“导入合成视频 → 分析 → 编辑/锁定/撤销重做 → 开发测试音色试听 → 保存/重开 → testOnly NUT 合成导出”，并由公开 `MediaSource` 重新探测导出流、解码音轨信号。当前导出不是正式 H.264/AAC 产品格式，测试音色不是产品默认音色。

## 4. 故障恢复矩阵

| 故障 | 公开观察点/oracle | 结果 |
|---|---|---|
| 取消 | UI 等待 worker 终态确认；媒体/音频/视频/渲染不发布部分结果；导出不覆盖已有目标 | 三 preset `pass` |
| worker 崩溃/断连 | IPC 转成结构化终态；活动修订不回写；重连后可继续预览 | 三 preset `pass` |
| 损坏媒体 | `GM-CORRUPT-001` 返回 `media/corrupt_media` 且无部分成功 | 三 preset `pass` |
| 不可打开媒体 | 不存在的固定负向路径经公开 `MediaSource::open` 返回 `media/unsupported_media` | 三 preset `pass`；不是受 DRM 样例 |
| 缺流/素材丢失 | required stream 返回稳定错误；素材重定位必须匹配大小与 SHA-256，不能仅凭同名绑定 | 三 preset `pass` |
| 缓存损坏 | 内容校验失败返回 `cache_corrupt/rebuild_required` 并隔离坏条目 | 三 preset `pass` |
| 磁盘不足/保存失败 | 公开 `SaveFault` 和导出故障保持最近成功项目/既有输出 | 三 preset `pass` |
| 编码失败/缺帧/PCM 错序 | 导出 fail closed，临时文件清理，既有目标不变 | 三 preset `pass` |
| 并发输出冲突 | 同 job/目标第二持有者返回 conflict；不能删除首持有者临时文件 | 三 preset `pass` |
| 异常恢复 | 优先恢复较新 autosave；autosave 损坏时回退 primary 并带诊断 | 三 preset `pass` |

## 5. 测量结果

以下全部是当前合成负载的 `measured(no-approved-threshold)`，不作性能通过判定。

| 指标 | Debug | CI/RelWithDebInfo | Release |
|---|---:|---:|---:|
| 音频分析 median / P95 | 100469 / 114572 µs | 12742 / 13744 µs | 12735 / 14007 µs |
| 音频分析吞吐 median | 951710 frame/s | 7514088 frame/s | 7522331 frame/s |
| 音频分析取消 P95 | 406 µs | 781 µs | 673 µs |
| 5760000-frame 音频渲染 | 311.833 ms | 45.856 ms | 40.802 ms |
| 音频渲染吞吐 | 18471431 frame/s | 125609238 frame/s | 141168167 frame/s |
| 音频渲染进程峰值工作集 | 326987776 B | 243044352 B | 242855936 B |
| 视频分析 median / P95 | 1666889 / 1733103 µs | 162971 / 173722 µs | 163340 / 170460 µs |
| 视频分析帧率 median | 53 frame/s | 548 frame/s | 544 frame/s |
| 视频取消 P95 | 17544 µs | 2680 µs | 1892 µs |
| seek 调用 | 66 µs | 16 µs | 6 µs |
| autosave 恢复 / primary 回退 | 3667 / 1239 µs | 1604 / 911 µs | 1124 / 1103 µs |

音频渲染三套结果的 PCM SHA-256 均为 `f1f26ab93fe29e58944919a15eca47314f7fc4a638b99e15de0daeb73345d3f8`。合成同步样例中 seek 请求 `200000000 ns` 选择 `180000000 ns` 帧，选择差 `20000000 ns`；音频播放头 `250000000 ns` 对该选择帧的 lateness 为 `70000000 ns`，只记录观测。预览/导出事件最大误差 `33000000 ns`，小于一输出帧 `33333334 ns`，该明确工程 oracle 为 pass。

## 6. 样例来源、许可、哈希与容差

| 清单 | 数量 | 来源/许可 | 清单 SHA-256 | oracle/容差 |
|---|---:|---|---|---|
| media | 13 | FFmpeg lavfi、数字静音或项目固定字节；`CC0-1.0` | `f267c6d2e98d6a35279ff049ec306de3ab13c42c986b8608c033b507a4682704` | stream/PTS/time/rational/error 精确相等；每项配方和媒体 hash |
| video | 10 | 项目确定性合成帧；`CC0-1.0` | `126dc37487809c92710ed969fd432bdadea9dee0f4bf5d904189b2f39fb1a575` | 版本化算法 oracle/固定参数；不代表产品效果 |
| audio | 10 | 项目确定性 PCM；`CC0-1.0` | `400b222482de2b26a747dd33e741c0e7a91b63b238d3157fa7827eb0c0eb5b97` | sample/time/tempo 精确；strength mantissa 1、feature mantissa 4 的既有版本化容差 |

媒体审计清单 SHA-256 为 `4b4c7f06112aba87ae66e31c9c59b566e244c4882f0bc35a06d2179bfaeb4cae`。生成器先校验许可证/配方，再校验实际媒体 SHA-256、ffprobe/公开 API 期望和容差依据。不可打开路径没有媒体字节，不作为黄金样例；它只验证公开错误类别/码。现有样例均为 synthetic，不得称为真实 VFR 或真实产品数据。

## 7. TR-A/F/Q 追踪结论

`pass(engineering-scope)` 仅表示本轮可执行公开契约满足；带 `not-evaluated` 的未确认部分不得被同行中的局部 pass 覆盖。

| 需求 | 本轮证据 | 判定 |
|---|---|---|
| TR-A-001 | QML 只做视图/交互；真实导入/分析异步，GUI heartbeat 可响应 | `pass(engineering-scope)` |
| TR-A-002 | core、媒体、音视频算法和渲染均可脱离 GUI 在 headless 三 preset 运行 | `pass(engineering-scope)` |
| TR-A-003 | Qt bridge→application service→IPC/core 单向消息，过期/断连结果不回写 | `pass(engineering-scope)` |
| TR-A-004 | Int64/UInt64、UTF-8、显式 schema/协议版本和公开 DTO 往返 | `pass(engineering-scope)` |
| TR-A-005 | buffer lease、只读快照、队列配额、取消/flush 生命周期 | `pass(engineering-scope)` |
| TR-A-006 | CMake x64 Debug/CI/Release、固定 ABI/依赖、Qt 与算法分层，x86/ARM64 拒绝 | `pass(engineering-scope)` |
| TR-F-001 | 探测、CFR/VFR/旋转/多流、损坏/不可打开/缺流结构化错误 | `not-evaluated(protected-media-not-covered)`；其余工程路径 pass |
| TR-F-002 | 流式代理/波形、取消、背压/配额、缓存键/损坏/淘汰 | `pass(engineering-scope)` |
| TR-F-003 | 预览/波形/时间线/seek 共用 `timeNs` 和 C++ 播放头 | `pass(engineering-scope)` |
| TR-F-004 | 增删移选锁、批量偏移、吸附、撤销/重做和 stale revision | `pass(engineering-scope)` |
| TR-F-005 | synthetic 快切/慢镜/静止/运镜/局部动作，候选来源/强度/置信度/版本 | 工程 oracle pass；`productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)` |
| TR-F-006 | 融合权重/间隔/密度/抑制来源与固定 seed 确定性 | `pass(engineering-scope)` |
| TR-F-007 | 人工/锁定保护、失败/取消/旧修订不提交 | `pass(engineering-scope)` |
| TR-F-008 | 三组以内开发测试音色映射、试听、来源/许可/hash | 工程 oracle pass；产品默认音色 `not-evaluated` |
| TR-F-009 | 固定 revision PCM、重叠/尾音/削波、取消无部分结果 | `pass(engineering-scope)` |
| TR-F-010 | 节拍/瞬态/频段/置信度；自由节奏/弱瞬态/噪声不伪造 BPM | 工程 oracle pass；自然度/真实效果 deferred |
| TR-F-011 | 三类视觉模板、参数、Qt Scene Graph 高密度路径 | 工程 oracle pass；产品视觉效果 deferred |
| TR-F-012 | 屏上/离屏共享 recipe/seed/revision，时序 oracle 一致 | 时序 `pass(engineering-scope)`；跨 GPU 像素容差 `not-evaluated` |
| TR-F-013 | 新建/保存/另存/重开/重定位/autosave/恢复/schema | `pass(engineering-scope)` |
| TR-F-014 | WAV/PCM 与 testOnly NUT 公开探测/解码 | `not-evaluated(unconfirmed-formal-H264-AAC-format-matrix)` |
| TR-F-015 | 导出冻结、临时目标、取消/磁盘/编码/崩溃/并发冲突 fail closed | `pass(engineering-scope)` |
| TR-F-016 | 阶段/进度/取消/错误/diagnosticId 和恢复操作经 UI 可见 | `pass(engineering-scope)` |
| TR-F-017 | 固定 vcpkg/FFmpeg/OpenCV/KissFFT/Qt 版本、许可、hash，无 shell 媒体命令 | `pass(engineering-scope)` |
| TR-F-018 | 本轮只验证构建产物和 headless 启动，不消费 `package/` | `not-evaluated(personal-unsigned-formal-distribution-deferred)` |
| TR-Q-001 | synthetic CFR/VFR 与同 revision 一帧误差 oracle | synthetic 工程 oracle pass；`realVfrEvaluation=not-evaluated(deferred-to-personal-use-feedback)` |
| TR-Q-002 | 固定输入/版本/参数/seed；三 preset PCM hash 一致 | 工程确定性 pass；跨 GPU 像素容差 `not-evaluated` |
| TR-Q-003 | GUI heartbeat 与异步长任务实际测试；未确认 P95 门槛 | `measured(no-approved-threshold)` |
| TR-Q-004 | 流式/配额/线程限制及分析/渲染/内存测量 | `measured(no-approved-threshold)`；formal performance deferred |
| TR-Q-005 | 取消、worker 崩溃、磁盘不足、素材丢失、迁移/恢复均保留成功提交 | `pass(engineering-scope)` |
| TR-Q-006 | 不可信媒体失败路径、worker 边界、参数 API、默认离线证据 | `pass(engineering-scope)`；正式安全评估不在本任务 |
| TR-Q-007 | 本轮核对固定依赖/许可/hash；不测试签名、恶意软件扫描或安装事务 | `not-evaluated(formal-release-deferred)` |
| TR-Q-008 | 当前 Windows 11 x64、offscreen/软件/默认图形、高 DPI、x86/ARM64 拒绝 | `not-evaluated(unconfirmed-windows-device-codec-matrix)` |
| TR-Q-009 | 媒体/分析/事件/渲染/项目接口和参数版本可追踪 | `pass(engineering-scope)` |
| TR-Q-010 | 确定性/边界/基准、时间/锁定/保存/导出与端到端自动化 | `pass(engineering-scope)` |

## 8. G0～G4 判定

| 门槛 | 已执行工程证据 | 总体判定 |
|---|---|---|
| G0 | 固定工具链、33 个 CC0 fixture、配方/hash/oracle 就绪 | `not-evaluated`：最低 Windows、基准硬件、真实代表素材、规模、默认风格和效果阈值未全部确认 |
| G1 | synthetic 稳定/自由节奏、快切/慢镜/静止/运镜/动作与可试听测试音轨均有确定性结果 | `not-evaluated(deferred-to-personal-use-feedback)`：产品效果、自然度、人工修正和盲评未执行 |
| G2 | 完整工程垂直切片、锁定/撤销、取消/失败原子性、同 revision 一帧误差 | `pass(engineering-scope)` |
| G3 | 三类视觉、屏上/离屏 recipe、全部指定故障与结构化性能记录 | `not-evaluated`：可靠性工程子门禁 pass；真实规模/基准硬件和正式产品性能 deferred |
| G4 | 当前主机三 preset、Qt/QML/App/Worker、x86/ARM64 拒绝和固定依赖通过 | `not-evaluated`：最低 Windows/设备/编码矩阵、正式签名与可信分发不在当前个人未签名范围 |

历史 `T021-ENV-001` 继续在 T-021 记录中为 `blocked`，不回写旧 Release 的 blocked 项。本轮 T-022 Release 是 D-015/当前个人 SAC-off 条件下的新证据，只证明本次工程入口可执行；不构成 SAC/WDAC 兼容或全绿发布声明。

## 9. 失败记录、修正边界与停止点

所有前置失败/超时保留在 `out/evidence/T-022/` 并索引于 `runs-v1.json`。测试会话修复了自身测量 optional 访问、未公开 message key 的过度断言、Qt GUI stdout 误判和跨用例 worker 状态污染；修正均限于测试、CTest 和诊断包装，没有修改测试 oracle 的既有期望或被测媒体/业务实现。最终运行没有 skip、fallback 或静默重试。

T-022 至此完成测试执行与结果登记。T-038 未启动；后续个人使用反馈发现问题时应另建缺陷，不将 deferred 项追溯改写为本轮 pass。
