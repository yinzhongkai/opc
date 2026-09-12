# 项目任务

## T-038：验证干净 Windows 环境并形成发布候选证据
- 负责人：release-engineer-windows-01
- 状态：todo
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：在功能、质量和安装流水线就绪后验证发布候选的安装、首次启动、升级、卸载、回滚、用户数据保留和诊断，形成可追溯证据；不批准或执行生产发布。
- 输入与依赖：T-022、T-037；D-004～D-008；最低 Windows 版本、安装器、签名主体与发布矩阵待确认；A-011 0.1。
- 优先级：未设定（架构建议：发布候选阶段必需）。
- 完成条件与确认方式：在确认的干净 Windows x64 兼容矩阵上复核哈希、签名状态、依赖、安装/升级/卸载/回滚、首次启动和核心工作流；记录环境、命令、结果、失败与未覆盖项，输出发布候选证据包；负责人自查，测试通过不等于最终发布批准。
- 进展：任务已登记，尚未由负责人会话接收。
- 成果与验证证据：[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；实际证据暂无。
- 阻塞与下一位行动人：等待 T-022/T-037 及发布输入确认；release-engineer-windows-01 先执行 T-036。
- 更新日期：2026-09-09。

## T-037：建立 Windows 部署、安装器与签名工程流水线
- 负责人：release-engineer-windows-01
- 状态：todo
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：从受控构建产物建立应用私有部署、安装/升级/卸载、回滚和隔离签名工程流程；不自行选择安装器、使用签名凭据或发布产品。
- 输入与依赖：T-013、T-019、T-032、T-035、T-036；D-004～D-008；安装器、最低 Windows 版本和签名输入待确认；A-011 0.1。
- 优先级：未设定（架构建议：功能闭环后启动）。
- 完成条件与确认方式：部署清单仅使用受控产物；应用、Qt、运行库、插件和原生依赖采用私有布局；安装、升级、卸载和失败回滚边界明确；签名凭据与普通构建隔离，未授权时使用 unsigned 流程；生成可复现脚本、日志、哈希、SBOM 和许可证包，负责人自查。
- 进展：任务已登记，尚未由负责人会话接收。
- 成果与验证证据：[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；实际流水线暂无。
- 阻塞与下一位行动人：等待功能产物及发布决定；T-036 可立即开展。
- 更新日期：2026-09-09。

## T-036：制定 Windows 发布输入、许可证与 SBOM 计划
- 负责人：release-engineer-windows-01
- 状态：todo
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：定义发布候选输入、部署布局、依赖/SBOM/许可证材料、安装事务、签名隔离和干净环境验证计划；不作法律结论，不接触凭据，不执行生产发布。
- 输入与依赖：D-004～D-008，A-004 0.5、A-005 0.3、A-006 0.1 WP-10、A-009 0.1、A-011 0.1；可与实现任务并行。
- 优先级：未设定（架构建议：立即启动，以提前暴露分发风险）。
- 完成条件与确认方式：列出源提交、构建预设、工具链、依赖、测试和版本冻结输入；定义私有部署、安装/升级/卸载/回滚、用户数据、缓存和日志边界；建立 Qt/FFmpeg/OpenCV/FFT/音色/模型/安装器许可证与 SBOM 字段；列出签名、最低 Windows 和渠道待决定项；形成版本化计划并自查。
- 进展：任务及 H-010 已登记，等待负责人接收。
- 成果与验证证据：[A-011 0.1 第 2～6 节](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；实际发布计划暂无。
- 阻塞与下一位行动人：无技术前置阻塞；release-engineer-windows-01 接收 H-010 后执行。
- 更新日期：2026-09-09。

## T-035：验证屏上/离屏一致性、GPU 降级与渲染性能
- 负责人：graphics-engineer-qt-scenegraph-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：让预览与导出复用同一渲染配方和核心数据，建立 GPU 能力探测、降级、设备差异和性能证据；不负责媒体编码或发布批准。
- 输入与依赖：T-025、T-032、T-034；基准硬件和像素容差待确认；A-011 0.1。完成后的离屏帧由媒体导出任务消费，该下游关系不是本任务前置。
- 优先级：未设定（架构建议：视觉模板基础完成后启动）。
- 完成条件与确认方式：相同事件修订、特征、参数、尺寸、帧时间和种子产生可解释的屏上/离屏等价结果；输出帧接口可由媒体层消费；记录帧时间、CPU/GPU、显存和上传量；低能力设备有明确降级与诊断；负责人自测并保存兼容/性能证据。
- 进展：2026-09-11，完成 offscreen schema/contract 0.1.0 和 `SpaceRhythm::OffscreenRendering`：默认 D3D11 GPU 使用公共 `QQuickGraphicsDevice/QQuickRenderControl/QQuickRenderTarget`，Qt Software 使用公共 paint-device fallback；两者固定复用同一 immutable snapshot、recipe、`build_geometry_frame()`、帧时间和 seed，输出 A-021 RGBA8/sRGB/top-down `RenderedFrame/FrameLease` 并沿用 lease 级有界背压。实现能力探测、进程级后端边界、结构化 render/device-lost diagnostic、旧 generation/session 终止和新 generation 重建；未使用 Qt 私有 API。
- 成果与验证证据：[A-025 0.1](artifacts/A-025-qt-offscreen-rendering-and-measurements.md)、[公共接口](../../src/rendering/include/space_rhythm/rendering/offscreen_renderer.hpp)、[实现](../../src/rendering/offscreen_renderer.cpp)、[7 项 GoogleTest](../../tests/unit/offscreen_rendering_test.cpp)、[机器可读测量](evidence/T-035/measurements-v1.json)及[验证摘要](evidence/T-035/verification-summary.md)。本机 software/GPU 各三模板屏上/离屏 exact hash 均相等，六组 max diff=0、diff pixels=0；帧/CPU/geometry/readback、上传顶点/bytes、working set 已实测，GPU frame time/动态显存占用明确 unavailable。最终 T-035 2/2 CTest 进程、内部 7/7 case 通过；T-033～T-035 合并回归 31/31 通过。
- 阻塞与下一位行动人：T-035 无实现阻塞并按负责人完成条件结束。基准 GPU、像素容差和性能阈值尚未确认，全部结果仅为 `measured/not-evaluated`，不作性能/视觉批准。H-009 仍保持 accepted，下一行动人为 architect-01，验收 T-033～T-035 后决定关闭。
- 更新日期：2026-09-11。

## T-034：实现高密度时间线、波形与三类视觉模板
- 负责人：graphics-engineer-qt-scenegraph-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：使用 Qt Quick Scene Graph 实现高密度事件/波形绘制和波形/示波器、频谱几何、粒子或线条脉冲三类基础模板；不实现产品工作流、音频分析或媒体编码。
- 输入与依赖：T-012、T-014 和 D-003 已完成/确认；仍依赖 T-013、T-030、T-033；A-011 0.1。
- 优先级：未设定（架构建议：工程骨架和契约就绪后启动）。
- 完成条件与确认方式：高密度数据采用批量几何/纹理/等价方案而非一点一个 QML Item；三类模板参数、范围、默认值和随机种子版本化；线程与 GPU 资源生命周期符合契约；具备裁剪、空输入、极密数据和设备恢复自测；形成可供 UI 和离屏路径消费的模块。
- 进展：2026-09-11，完成 geometry schema 1/contract 0.1.0 的纯 C++ 几何核心和公共 Qt Scene Graph 适配：事件时间线按像素聚合，波形按像素 min/max envelope，频谱按宽度分组取峰值，节奏脉冲由版本化 seed 稳定生成；三模板均为 1.0.0，参数范围、工程默认值、摘要、批次/总顶点上限版本化。`SceneGraphRenderItem` 仅在 `updatePaintNode()` 创建/复用 `QSGGeometryNode`，使用动态批量顶点和 device generation 重建；屏上与后续离屏共用 `build_geometry_frame()`。未执行 T-035。
- 成果与验证证据：[A-022 0.1](artifacts/A-022-batched-scene-graph-visual-templates.md)、[几何公共接口](../../src/rendering/include/space_rhythm/rendering/geometry_core.hpp)、[Scene Graph 公共接口](../../src/rendering/include/space_rhythm/rendering/scene_graph_render_item.hpp)、[实现](../../src/rendering/geometry_core.cpp)、[Qt 适配](../../src/rendering/scene_graph_render_item.cpp)、[9 项几何 GoogleTest](../../tests/unit/render_geometry_test.cpp)、[2 项 QSG GoogleTest](../../tests/unit/render_scene_graph_test.cpp)及[验证摘要](evidence/T-034/verification-summary.md)。Windows x64 Debug `/W4 /WX` 全目标构建通过，最终 T-034 专项 11/11 通过。
- 阻塞与下一位行动人：T-034 无阻塞并按完成条件结束。视觉风格、效果/性能门槛、基准 GPU、像素容差和设备矩阵仍未确认，不在本任务宣称通过。T-035 保持 todo，须由用户另行明确启动；H-009 在 T-035 完成前保持 accepted。
- 更新日期：2026-09-11。

## T-033：定义 RenderRecipe、渲染线程与离屏接口契约
- 负责人：graphics-engineer-qt-scenegraph-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：定义版本化渲染配方、数据快照、界面/渲染线程边界、GPU 资源生命周期、命中接口及离屏帧契约；不冻结后端私有 API，不改变 UI 或核心事件语义。
- 输入与依赖：A-004 0.5、A-005 0.4、A-006 0.1 WP-07、A-011 0.1，D-003 confirmed；与 T-014/T-024/T-030/T-017 对齐，可先用接口草案和测试向量。
- 优先级：未设定（架构建议：立即启动）。
- 完成条件与确认方式：RenderRecipe 含修订、特征、模板参数、尺寸、时间范围、帧率、颜色和种子；明确不可变快照、节点/资源创建更新释放、设备丢失、错误和版本兼容；离屏输出含帧时间、格式、所有权和背压；形成版本化契约及边界测试向量，负责人自查。
- 进展：2026-09-11，完成 `renderContractVersion 0.1.0/schema 1` 的纯 C++ 契约：版本化 RenderRecipe、深拷贝不可变 RenderSnapshot、精确帧时间、整数坐标/命中、Scene Graph device generation 生命周期、RGBA8/sRGB/top-down 离屏帧、FrameLease 与按在途 lease 计数的有界背压/取消/设备丢失语义。明确 QQuickItem/QSGGeometryNode 只在 `updatePaintNode()` 渲染线程同步点更新、UI 线程只交换快照、资源经 render job/invalidation 清理；仅使用 Qt 公共 API，未实现 T-034。
- 成果与验证证据：[A-021 0.1](artifacts/A-021-render-recipe-thread-offscreen-contract.md)、[公共接口](../../src/rendering/include/space_rhythm/rendering/render_contract.hpp)、[实现](../../src/rendering/render_contract.cpp)、[GoogleTest 契约向量](../../tests/contract/render_public_contract_test.cpp)及[验证摘要](evidence/T-033/verification-summary.md)。Windows x64 Debug `/W4 /WX` 全目标构建通过，`RenderContractVectors.*` 18/18 通过。
- 阻塞与下一位行动人：T-033 无阻塞并按完成条件结束。T-034/T-035 均未启动；视觉风格、实际模板参数、基准 GPU 与像素/性能容差仍留给后续任务。须由用户另行明确启动 T-034，H-009 在 T-033～T-035 全部完成前保持 accepted。
- 更新日期：2026-09-11。

## T-032：实现事件音色触发、确定性混音、试听与离线输出
- 负责人：audio-dsp-engineer-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：实现事件到音色映射、采样精确触发、重叠/尾音、增益/削波控制、确定性混音、Qt 设备试听及 PCM/WAV 输出；不负责媒体容器封装或音色许可决定。
- 输入与依赖：T-012 和 D-003 已完成/确认；仍依赖 T-013、T-015、T-016、T-030、T-031及合法音色输入；A-011 0.1。完成后的混音 PCM 由媒体导出和离屏图形任务消费，这些下游关系不是本任务前置。
- 优先级：未设定（架构建议：节奏特征和核心事件就绪后启动）。
- 完成条件与确认方式：相同事件、音色、参数和种子产生等价 PCM；试听与离线复用混音核心，设备失败不破坏离线输出；重叠、尾音、非有限值、峰值与削波受控；合法测试音色有来源/许可/哈希；提交自动化测试、WAV/PCM 黄金结果和性能记录。
- 进展：2026-09-11，用户明确启动 T-032；前置与 A-018 三个 CC0 测试音色已满足。已实现纯 C++ Q23 确定性混音、EventKind→音色映射、nearest-even 采样触发、重叠/尾音/增益/声像/硬削波、PCM/WAV 输出及只传输同一核心 PCM 的 QAudioSink 适配。测试音色继续只用于测试，不登记产品默认音色。
- 成果与验证证据：[A-020 0.1](artifacts/A-020-audio-rendering-and-preview-implementation.md)、[T-032 验证摘要](evidence/T-032/verification-summary.md)、[render oracle](../../tests/golden/audio/render-oracles-v1.json)。Debug/CI 功能与 golden 有 13/13 同源成功记录，Release 同源专项曾 14/14；最终复跑时 Release 单元进程也受 WDAC/SAC 阻断，最新 Release 专项不宣称通过。Release 实测 140,724,290 frames/s、峰值工作集 243,499,008 bytes、取消延迟 0.085 ms，均为 measured/not-evaluated。
- 阻塞与下一位行动人：任务实现无阻塞。T021-ENV-001/WDAC-SAC 阻止最新 Release 单元门禁；产品默认音色、效果/性能阈值、声卡矩阵与完整发布门禁仍待对应责任人和确认人，全仓 Release 另有非 T-032 Qt smoke 缺口。H-008 由发起人按流程关闭；本成员停止，不启动后续任务。
- 更新日期：2026-09-11。

## T-031：实现音频特征、瞬态和节拍候选分析
- 负责人：audio-dsp-engineer-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：实现短时能量、频段能量、谱变化、瞬态、节拍和置信度等 C/C++ DSP 分析；不直接修改核心时间线，不把低置信度素材伪装成稳定节拍。
- 输入与依赖：T-013、T-014、T-017、T-018、T-030 已完成，D-003 已确认 C++ DSP + KissFFT 总体路线；A-011 0.1、A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1、A-017 0.2、A-018 0.2。
- 优先级：未设定（架构建议：PCM 契约和媒体输入就绪后启动）。
- 完成条件与确认方式：窗、步长、FFT、平滑、峰值和置信度参数版本化；输出候选含时间、类型、强度、置信度、来源和失败原因；固定 PCM 结果确定性；覆盖稳定节拍、变速、自由节奏、弱瞬态、噪声和静音；记录吞吐、内存、取消延迟和数值容差，负责人提交自测。
- 进展：2026-09-10，用户明确确认全部前置满足并启动；audio-dsp-engineer-01 已完成 A-018 PCM 窄适配、周期 Hann/KissFFT 分析、短时/频段能量、谱变化、瞬态/节拍候选、置信度、稳定排序、错误/取消/资源限制及 Windows 验证。2026-09-11，用户明确授权把已 `completed` 的 T-031 作为 H-011 验收修订重新置为 `in_progress`；修订删除调用者补填媒体事实的 `PcmAdapterContext`，直接消费 `156b19f` 的媒体 schema 2 `PcmBuffer`，完成真实管线和 fail-closed 回归后恢复 `completed`。算法、参数、oracle、固定 baseline 均未改变，T-032 未执行。
- 成果与验证证据：[A-019 0.2：音频特征、瞬态与节拍候选实现和算法 oracle](artifacts/A-019-audio-analysis-implementation-and-oracles.md)、[A-018 0.2](artifacts/A-018-audio-dsp-pcm-feature-candidate-contract.md)、[公共接口](../../src/audio_analysis/include/space_rhythm/audio/analysis.hpp)、[实现](../../src/audio_analysis/analysis.cpp)、[A-018 算法 oracle](../../tests/golden/audio/algorithm-oracles-v1.json)、[GoogleTest](../../tests/unit/audio_analysis_test.cpp)及[验证摘要](evidence/T-031/verification-summary.md)。原三配置专项各 17/17；本次验收修订 Debug、CI/RelWithDebInfo、Release 各 22/22，CI 媒体专项另 27/27。Debug 前两次曾被 WDAC 在断言前阻止，最终重建复跑通过；既有性能实测继续有效，未确认阈值保持 `measured/not-evaluated`。
- 阻塞与下一位行动人：T-031 功能与验收修订完成，无实现阻塞；H-011 已由发起人验收并关闭。T021-ENV-001 的项目级环境问题仍按原任务记录，本次 Debug 瞬态不改写该结论。T-032 保持 `todo`，须由用户另行明确启动。
- 更新日期：2026-09-11。

## T-030：定义 PCM、采样时间、音频特征与测试音色契约
- 负责人：audio-dsp-engineer-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：定义 DSP 输入 PCM、采样时间、缓冲所有权、特征/候选、参数、错误和测试音色边界；不冻结 FFT 后端或随包音色，不重定义媒体 PTS 或核心事件。
- 输入与依赖：A-004 0.5、A-006 0.1 WP-06、A-011 0.1、A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1；T-014/T-017/T-018/T-020 已完成并已对齐。
- 优先级：未设定（架构建议：立即启动）。
- 完成条件与确认方式：明确格式、采样率、声道、交错、有效帧、起点、重采样延迟、样本索引到 timeNs 舍入；定义特征帧、候选、置信度、错误和版本兼容；建立脉冲、节拍、变速、噪声、静音等可生成黄金样例和合法测试音色清单；形成版本化契约及测试向量，负责人自查。
- 进展：2026-09-10，audio-dsp-engineer-01 依据用户明确指令接收 H-008，并完成 `dspContractVersion 0.1.0`。契约已收窄 A-015 实际 interleaved float PCM，冻结 segment/采样索引/重采样证据、精确 TimeNs 舍入、lease 生命周期、特征/候选 DTO、低置信语义、版本/摘要、错误及消费者边界；未实现分析或混音算法。
- 成果与验证证据：[A-018 0.2：音频 DSP PCM、特征、候选与测试音色契约](artifacts/A-018-audio-dsp-pcm-feature-candidate-contract.md)、[向量清单](../../tests/golden/audio/fixtures-v1.json)、[生成/校验器](../../tests/golden/audio/Generate-AudioDspVectors.ps1)、[实际 SHA-256 证据](../../tests/golden/audio/generated/actual-hashes-v1.json)与[许可声明](../../tests/golden/audio/LICENSE.md)。10 项 CC0 合成向量（含 3 个合法测试音色）在生成模式和 `-ValidateOnly` 模式均通过 hash、长度、schema、许可和登记自检；2026-09-11 的 0.2 修订已对齐媒体 schema 2 直接消费边界。
- 阻塞与下一位行动人：T-030 无阻塞并按完成条件结束。原媒体 provenance 缺口已由提交 `156b19f` 补齐并经 T-031 消费方验收，H-011 已关闭；缺字段、旧 schema 或矛盾 trace 继续 fail closed。T-032 保持 `todo`，须由用户另行明确启动。
- 更新日期：2026-09-11。

## T-029：评估视频卡点效果、性能并执行可选模型门禁
- 负责人：video-algorithm-engineer-cv-01
- 状态：todo
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：在代表性数据上评估镜头、运动和动作候选的效果、可解释性、人工修正量与性能，决定是否具备进入可选模型评估的证据；不自行确定产品门槛或引入模型。
- 输入与依赖：T-027、T-028；产品样本、卡点自然口径、基准硬件和阈值待确认；A-011 0.1。
- 优先级：未设定（架构建议：经典算法可运行后启动）。
- 完成条件与确认方式：按样本类别报告命中、误报、时间误差、人工修正量或确认的等价指标；记录吞吐、内存、线程和取消；列出已知失败模式；经典算法未达门槛时提交模型收益、运行时、许可、CPU/GPU 和包体影响，等待新决定；形成可复核效果/性能报告。
- 进展：任务已登记，尚未由负责人会话接收。
- 成果与验证证据：[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；实际评估暂无。
- 阻塞与下一位行动人：等待 T-027/T-028 及产品/测试输入；video-algorithm-engineer-cv-01 先执行 T-027。
- 更新日期：2026-09-09。

## T-028：实现经典镜头、运动与动作峰值分析
- 负责人：video-algorithm-engineer-cv-01
- 状态：todo
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：基于 C++/OpenCV 和代理帧实现镜头切换、全局/局部运动曲线与动作峰值候选；不直接融合或覆盖核心时间线，不默认引入模型。
- 输入与依赖：T-014 已完成，D-003 已确认 OpenCV 经典算法路线；仍依赖 T-013、T-017、T-018、T-027；A-011 0.1。
- 优先级：未设定（架构建议：工程、媒体和契约就绪后启动）。
- 完成条件与确认方式：输出 shot/motion_peak/action_peak 的 timeNs、强度、置信度、来源、算法/参数版本和低质量原因；真实 PTS/VFR 映射正确；相同输入与参数结果可复现；覆盖闪烁、运镜、局部动作、快切、慢切和静止样例；使用有界内存并支持取消；负责人提交单元、黄金和性能自测。
- 进展：任务已登记，尚未由负责人会话接收。
- 成果与验证证据：[A-011 0.1](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；实际代码暂无。
- 阻塞与下一位行动人：仍等待 T-013/T-017/T-018/T-027；video-algorithm-engineer-cv-01 可先完成 T-027。
- 更新日期：2026-09-09。

## T-027：建立视频分析契约、样本矩阵和效果指标
- 负责人：video-algorithm-engineer-cv-01
- 状态：todo
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：定义代理帧输入、视频候选、参数/算法版本、失败诊断、代表性样本和效果/性能测量方法；不设定未经产品确认的门槛，不重定义媒体或核心时间。
- 输入与依赖：A-002 0.2、A-004 0.5、A-006 0.1 WP-05、A-011 0.1；与 T-014/T-017/T-020 对齐，可立即开展。
- 优先级：未设定（架构建议：立即启动）。
- 完成条件与确认方式：输入明确帧格式、尺寸、方向、真实 timeNs、代理映射和生命周期；候选含类型、时间、强度、置信度、来源、版本和低质量原因；样本覆盖快/慢切、闪烁、运镜、局部动作、静止和慢镜头并登记来源/许可/哈希；指标区分效果、人工修正量和性能，未定阈值标明待确认；形成版本化成果并自查。
- 进展：任务及 H-007 已登记，等待负责人接收。
- 成果与验证证据：[A-011 0.1 第 2～4 节](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；实际契约和样本暂无。
- 阻塞与下一位行动人：无技术前置阻塞；video-algorithm-engineer-cv-01 接收 H-007 后执行。
- 更新日期：2026-09-09。

## T-026：完成时间线编辑、作业状态、错误恢复和用户路径集成
- 负责人：ui-engineer-qt-quick-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-12 明确要求执行 T-026、接入现有真实能力、提交自测证据并更新 T-026/H-006；同日用户根据 architect-01 对提交 `f792e64` 的验收结论，明确重新打开 T-026，要求修复 `T026-DEFECT-001`～`004`、补针对性测试并重新提交；architect-01 随后在提交 `b6f00ed` 登记 `T026-DEFECT-005`、`006`，同日用户再次明确重开 T-026 修复两项问题并补回归测试，要求 H-006 暂不关闭且修复完成前不启动 T-022；原任务由 2026-09-09 编制请求登记。
- 目标与范围：完成事件时间线、播放头、参数、作业进度/取消、错误/恢复界面及导入—分析—编辑—试听—保存—导出用户路径；不在 UI 内实现算法或媒体循环。
- 输入与依赖：T-015、T-016、T-018、T-019、T-025、T-032、T-034、T-035 已完成；A-011 0.1、A-024 0.1、A-025 0.1、A-026 0.1，以及当前核心、系统、媒体、音频和渲染公开契约。完成后的用户路径由端到端质量任务验证，该下游关系不是本任务前置。
- 优先级：未设定（架构建议：核心和媒体接口可用后启动）。
- 完成条件与确认方式：支持缩放/滚动/拖动/锁定/偏移、播放头、键盘及撤销重做；作业状态可取消且不阻塞 UI；错误含阶段、项目安全、恢复动作和诊断 ID；高 DPI、窗口缩放、焦点和可访问性具备检查；提交 Qt Quick/UI 集成自测和用户路径记录。
- 进展：2026-09-12，用户明确要求执行 T-026；首版 `f792e64` 接入真实六阶段路径、核心编辑、T-019 时钟、保存、冻结/离屏/安全导出及 UI 自动化。architect-01 随后在 `f3af651` 登记 `T026-DEFECT-001`～`004` 并给出 `revise`，用户明确重开。提交 `8bdfdbb` 将 bridge 提升为 0.3.0/schema 3，完成 pending 手势、真实 UI/worker 双进程、事件音轨和后台保存/分块导出；architect-01 在 `b6f00ed` 复核确认前四项通过，但新登记 `T026-DEFECT-005`（取消令牌泄漏到重连后试听）和 `T026-DEFECT-006`（打开项目仍在 GUI 线程同步读盘），用户再次明确重开。本轮已将作业/试听取消源和试听操作序号分域，取消分析后重连不再污染事件试听；项目打开的 load/可写性检查迁移到后台 IO，以 workspace generation 拒绝迟到结果并支持失败重试。Debug 与 CI/RelWithDebInfo 构建通过，真实集成均 11/11，UI/headless 均 5/5，ProjectStore/取消关联选择集 9/9；固定媒体黄金 fixture 的 WDAC/SAC 限制继续如实保留。T-022 未启动。
- 成果与验证证据：[A-027 0.3：Qt Quick 真实服务、时间线与安全导出集成](artifacts/A-027-qt-quick-real-workflow-integration.md)及 [T-026 验证摘要](evidence/T-026/verification-summary.md)。
- 阻塞与下一位行动人：`T026-DEFECT-005`、`006` 已修复并完成针对性回归，无 T-026 范围内实现阻塞；按用户要求 H-006 暂不关闭并保持 `accepted`，下一位行动人为发起人 architect-01，复核本次修订和证据。视觉风格、产品文案、产品默认音色、发布导出格式、产品默认视觉模板参数、设备矩阵及可访问性/性能门槛继续待确认；A-018 CC0 映射仅能显式作为开发用途，当前导出仍为 T-019 `testOnly`。T-022 保持 `todo`，未启动。
- 更新日期：2026-09-12。

## T-025：实现 Qt Quick 工作区、页面骨架与 C++ ViewModel 桥接
- 负责人：ui-engineer-qt-quick-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：实现应用外壳、项目/素材导入、预览、播放、参数和任务页面骨架，以及 QML 与 C++ ViewModel/模型/作业接口；可使用 mock service 并行，不实现媒体或算法本体。
- 输入与依赖：T-012/T-013/T-014/T-024 已完成，D-002/D-003 confirmed；A-011 0.1、A-023 0.1，以及当前核心、系统、媒体、音频和图形公开契约。
- 优先级：未设定（架构建议：设计与工程骨架就绪后启动）。
- 完成条件与确认方式：QML 仅负责视图/绑定/交互；领域和作业通过版本化 C++ 接口；页面覆盖空闲、加载、处理中、取消、失败、恢复和只读状态；mock 与真实 service 可替换；无长任务阻塞界面线程；组件具备对象名/可访问标识和基础 Qt Quick 自测。
- 进展：2026-09-11，用户明确确认前置条件全部满足并要求执行 T-025；ui-engineer-qt-quick-01 完成启动页、持续工作区和致命错误页，拆分素材、预览、时间线、检查器、任务抽屉等 11 个 QML 组件，新增 UI bridge 0.1.0/schema 1 的 `WorkspaceService`、共享 DTO、一个 QObject ViewModel 和三个 QAbstractItemModel。QML 只接收格式化时间/修订及字符串化模板整数，不保存或计算 `TimeNs`、revision、frame index；实际 `SceneGraphRenderItem` 由 C++ 在 Loader 进入工作区后挂载。确定性 mock 覆盖 idle/loading/running/cancelling/failed/recovery/readOnly/fatal，并与后续真实 service 共用接口、错误和异步快照语义。关键控件具备稳定 `objectName` 与 Accessible 信息；新增 Qt Quick Test、C++ bridge test 和 CTest headless 入口。本轮未执行 T-026。
- 成果与验证证据：[A-024 0.1：Qt Quick 工作区与版本化 ViewModel 桥接实现](artifacts/A-024-qt-quick-workspace-and-viewmodel-bridge.md)及 [T-025 验证摘要](evidence/T-025/verification-summary.md)。Windows x64 Debug 定向构建和 `space_rhythm_app_qmllint` 通过；C++ bridge 9/9、QML 6/6，最终 `ctest -L '^t025$'` 3/3 通过，含实际应用 QML/Scene Graph smoke。
- 阻塞与下一位行动人：T-025 无阻塞并按完成条件结束。T-026 保持 `todo`，须等待用户后续明确启动；T-035 的 T-025 前置现已满足，但本任务未执行 T-035。视觉风格、产品文案、默认音色、发布导出格式、产品默认模板参数和可访问性/性能门槛继续待确认。
- 更新日期：2026-09-11。

## T-024：定义 UI 信息架构、交互原型与基础设计系统
- 负责人：ui-engineer-qt-quick-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 要求补充系统架构师判断的新增成员及成员任务并提交。
- 目标与范围：将已确认的独立桌面应用主辅流程转成信息架构、线框/交互原型、界面状态、组件和基础视觉规范；不改变产品范围，不以原型代替 QML 实现或产品批准。
- 输入与依赖：D-001～D-008 confirmed；A-002 0.2、A-004 0.5、A-005 0.4、A-006 0.1 WP-03、A-011 0.1、A-012 0.1、A-014 0.4、A-015 0.3、A-016 0.1、A-018 0.2、A-019 0.2、A-020 0.1、A-021 0.1、A-022 0.1；T-014/T-017/T-020 已完成并完成契约对齐。
- 优先级：未设定（架构建议：立即启动）。
- 完成条件与确认方式：覆盖导入、分析、编辑、试听、保存、导出及失败恢复；明确导航、布局、空/忙/取消/失败/恢复状态、快捷键、焦点、高 DPI 和可访问性；定义颜色、排版、间距、选中/锁定/错误及组件状态；列出 ViewModel/mock API 需求和未确认产品问题；形成版本化原型/规范，负责人自查。
- 进展：ui-engineer-qt-quick-01 于 2026-09-09 完成成员会话初始化并接收 H-006；2026-09-11 依据用户“继续完成 T-024”授权完成成果。设计采用持续编辑工作区而非强制向导，覆盖导入—分析—编辑—试听—保存—导出、启动/工作区/恢复/失败/只读线框、完整状态矩阵、时间线/预览/模板/任务规则、快捷键/焦点/DPI/可访问性、基础令牌与组件状态，并把 C++ 契约映射为 QML-facing ViewModel/mock 清单。纳秒时间、64 位修订、媒体/音频/几何计算均留在 C++，未修改 QML 或执行 T-025。
- 成果与验证证据：[A-023 0.1：Qt Quick/QML 信息架构、交互线框与基础设计系统](artifacts/A-023-qt-quick-ui-information-architecture-and-design-system.md)。第 11 节已按 T-024 八类完成条件逐项自查；视觉风格、产品文案、默认音色、导出格式、默认模板参数、可访问性/性能门槛均明确保持待确认，没有以工程默认值替代产品批准。
- 阻塞与下一位行动人：T-024 无阻塞并按完成条件结束。T-025 保持 `todo`，本轮按用户要求不启动；T-035 仍须等待 T-025，即使其他前置已完成也不得提前执行。H-006 保持 accepted，等待 T-025/T-026 后续授权与交付。
- 更新日期：2026-09-11。

## T-023：确定完整研发编制并细化新增成员任务
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-09 指出系统架构师应判断各领域开发人员及任务，并明确要求把新增成员和成员任务补充到文档并提交。
- 目标与范围：从完整 MVP 数据流和交付链判断人员覆盖，在现有四位研发基础上提出 UI、视频算法、音频 DSP、实时图形和 Windows 发布岗位，形成 scope、任务、依赖与交接；成员配置由框架超级管理员依据同一用户授权建立。本任务不替项目经理承诺排期，不把配置当成会话已启动，不确认 D-003 或生产发布。
- 输入与依赖：TEAM 和全部成员配置；D-001～D-008；A-002 0.2、A-004 0.5、A-005 0.3、A-006 0.1、A-007 0.1；T-011～T-022；本轮用户直接授权。
- 优先级：高（用户要求本轮完成并提交）。
- 完成条件与确认方式：形成 A-011 0.1；由框架超级管理员建立五个职责无重叠的成员配置及有效岗位/知识组合；登记 T-024～T-038 与 H-006～H-010；更新现有集成任务依赖；校验配置、编号、链接、责任覆盖和 Git 提交边界，不混入其他成员未提交工作。
- 进展：已形成九个研发岗位覆盖 WP-01～WP-10 的完整矩阵；框架超级管理员已按用户授权新增五位成员，architect-01 已登记十五条研发任务和五条启动交接；UI/UX 在 MVP 内与 QML 开发合并，高密度 Scene Graph 与发布工程独立负责。
- 成果与验证证据：[A-011 0.1：一期 MVP 完整研发编制与任务计划](artifacts/A-011-complete-mvp-engineering-staffing-and-task-plan.md)；成员配置、T-024～T-038、H-006～H-010及校验记录。
- 阻塞与下一位行动人：本任务无阻塞。五位成员须在各自会话接收交接后才开始；D-003、产品样本/门槛、最低 Windows、编码/安装/签名输入仍由用户或对应流程确认。
- 更新日期：2026-09-09。

## T-022：建立端到端集成、故障恢复与质量门禁证据
- 负责人：tester-cpp-qt-01
- 状态：todo
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 C++/Qt 测试工程师执行。
- 目标与范围：在工程骨架、核心/系统和媒体基础实现就绪后，独立验证 UI 空壳—worker—核心—媒体—存储链路，覆盖故障恢复、兼容和性能测量，并汇总 A-004 G0～G4 证据；不替开发修复缺陷，不虚构产品或性能阈值，不批准成果或发布。
- 输入与依赖：T-013、T-015、T-016、T-018、T-019、T-021、T-026、T-029、T-032、T-035；D-001、D-002、D-004～D-008 confirmed；A-004 0.5、A-007 0.1、A-011 0.1；基准硬件、素材和阈值由用户/产品侧另行确认。
- 优先级：高；按前置条件排队。
- 完成条件与确认方式：形成可重复的端到端测试，至少覆盖取消、worker 崩溃、损坏/不支持媒体、缓存损坏、素材丢失、磁盘不足和并发输出冲突；记录时延、吞吐、峰值内存、seek、音画漂移及恢复数据；按 G0～G4 输出证据、失败项、复现步骤和归属任务，未确认阈值只报告 measured/not-evaluated，不误写 pass；负责人自查并把结果登记为版本化成果。
- 进展：已完成任务拆分和依赖登记，尚未由负责人会话接收。
- 成果与验证证据：[A-007 0.1 第 2、3.4 节](artifacts/A-007-four-engineer-execution-plan.md)定义测试面与门禁证据要求；实际测试结果暂无。
- 阻塞与下一位行动人：等待 T-013、T-015、T-016、T-018、T-019、T-021、T-026、T-029、T-032、T-035 交付；tester-cpp-qt-01 先接收 H-005 并执行 T-020，不应提前宣告本任务通过。
- 更新日期：2026-09-08。

## T-021：实现统一测试入口、契约测试与黄金样例库
- 负责人：tester-cpp-qt-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 C++/Qt 测试工程师执行；同一用户于 2026-09-10 确认 T-013、T-014、T-017、T-020、D-003 前置满足及 T-015、T-016、T-018 已有实现，并明确启动 T-021；同日用户明确要求基于 `40b1734` 独立复测 T021-DEFECT-001，只有 Debug 和 CI/RelWithDebInfo 通过后才标记 resolved。
- 目标与范围：在测试策略和首批核心/媒体契约可用后，依据 D-003 建立 Windows headless 测试入口、契约测试和许可可核对的黄金样例；不把 `package/` 逆向样本作为可分发测试资产。
- 输入与依赖：T-013、T-014、T-017、T-020 completed，D-003 confirmed；T-015、T-016、T-018 实际实现；A-004 0.5、A-005 0.4、A-007 0.1、A-012 0.1、A-014 0.3、A-015 0.2、A-016 0.1；T021-DEFECT-001 修复提交 `40b1734`。
- 优先级：高（用户要求本轮执行）。
- 完成条件与确认方式：提供统一 `ctest` 入口、标签、headless 配置和失败诊断归档；覆盖时间换算边界/溢出、事件锁定/事务/修订、PTS/VFR/旋转、IPC 状态机、schema 迁移及原子保存契约；黄金样例登记来源或生成方式、许可证、哈希、期望值及容差依据；按 D-003 接入 GoogleTest、Qt Test/Qt Quick Test；负责人自查并在 Windows x64 可用环境中保存实际运行证据。
- 进展：tester-cpp-qt-01 于 2026-09-10 完成统一 `Invoke-HeadlessTests.ps1`、CTest `t021` 与分层/门槛标签、JUnit/环境/日志/LastTest 失败归档；首轮新增 11 项仅调用公开 API 的独立契约测试及 12 个 CC0 样例/30 个时间向量审计。用户同日要求基于 `40b1734` 独立复测 T021-DEFECT-001 后，保留 GM-ROT-SAR-001 的 `limited` oracle，并补齐经公开 `MediaSource` 观察的 `full/unknown` 用例，独立契约增至 12 项。Debug 与干净 CI/RelWithDebInfo 各实际执行 63 项并 63 pass、0 fail、0 blocked、0 skip；首次 Release 记录仍为 34 pass、1 个当时的契约 fail、26 blocked。
- 成果与验证证据：[A-017 0.2：Windows headless 契约测试入口与证据](artifacts/A-017-windows-headless-contract-test-entry-and-evidence.md)、[T-021 验证摘要](evidence/T-021/verification-summary.md)、[首次运行索引](evidence/T-021/runs-v1.json)及 [`40b1734` 独立复测索引](evidence/T-021/runs-v2.json)。原始证据位于忽略的 `out/evidence/T-021/`，索引记录输入、环境、JUnit、CTest、发现、标签、LastTest、构建日志及 result SHA-256；所有运行使用固定 seed、UTC、offscreen、软件 RHI 和独立 TEMP/TMP。
- 阻塞与下一位行动人：`T021-DEFECT-001` 已按用户指定条件独立复测为 `resolved`：A-014 0.3 的公开 `limited/full/unknown` 映射及 GM-ROT-SAR-001 在 Debug、CI/RelWithDebInfo 均通过，既有 oracle 与媒体实现未由测试会话修改。`T021-ENV-001` 继续为 `blocked`：首次 Release 的既有 `space_rhythm_core_tests.exe` 被 WDAC 阻止启动，原 26 项记录不得改写；build-engineer-windows-qt-01 已完成专项诊断并以重建产物真实运行 26/26，但严格 Release 全套仍为 53/63，Code Integrity `VerifiedAndReputableDesktop` 对其他同轮生成程序继续给出 `0xC0E90002`/Win32 4551。下一行动人为主机策略管理员，需按 [专项诊断](evidence/T-013/t021-env-001-verification-summary.md)提供最小开发信任路线后，再由 tester-cpp-qt-01 无 fallback 复跑 63/63。G0～G4、性能、效果和硬件兼容均为 `not-evaluated`。T-022 保持 todo，未启动。
- 更新日期：2026-09-10。

## T-020：制定测试策略、需求追踪与可复现规则
- 负责人：tester-cpp-qt-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 C++/Qt 测试工程师执行；同一用户于 2026-09-10 明确要求 tester-cpp-qt-01 接收 H-005、启动 T-020，并基于已完成的 T-013～T-018、A-012、A-014 0.2 和 A-015 形成版本化测试计划后提交。
- 目标与范围：把 A-004 技术需求与 G0～G4 转成单元、契约、集成、Qt/QML、黄金样例、性能、故障和兼容测试计划，定义开发自测与独立验证分工；当前阶段不引入未确认的框架、不编造产品阈值、不代替其他成员实现被测模块。
- 输入与依赖：A-004 0.5、A-005 0.4、A-006 0.1、A-007 0.1、A-012 0.1、A-014 0.2、A-015 0.1，D-001～D-008 confirmed；T-013～T-018 completed；H-005 及用户 2026-09-10 的本轮边界。
- 优先级：高（用户要求本轮启动）。
- 完成条件与确认方式：逐项追踪 TR-A-001～006、TR-F-001～018、TR-Q-001～010 与 G0～G4；定义测试层级、目录、命名、种子/时钟、临时目录、日志与环境归档、跳过规则、失败注入、样例来源/许可/哈希和统一执行入口；列出需用户确认的阈值与基准输入；形成并登记版本化测试计划，负责人完成覆盖和可执行性自查，无独立评审要求。
- 进展：tester-cpp-qt-01 于 2026-09-10 接收 H-005 并按用户要求完成 T-020。已把 T-013～T-018 的开发自测与后续独立验证分开登记，建立 UT/CT/GM/IT/QQ/PF/FI/CO 八层策略，逐项追踪全部 34 条 TR 和 G0～G4，并定义随机种子、时钟、临时目录、日志、环境归档、黄金样例来源/许可/hash、跳过、重试及 `measured/not-evaluated` 判定规则。T-021/T-022 未启动，未执行测试或修改被测实现。
- 成果与验证证据：[A-016 0.1：C++/Qt 测试策略、需求追踪与可复现规则](artifacts/A-016-cpp-qt-test-strategy-and-traceability.md)，状态 draft。负责人完成编号覆盖、输入版本、状态语义、本地链接和边界自查；未确认的性能、效果、硬件、最低 Windows、发布格式与兼容矩阵均保持 `not-evaluated`，已有数值只登记为 `measured` 或开发自测输入，不写成 tester 独立通过。
- 阻塞与下一位行动人：T-020 无阻塞并按完成条件结束。T-021/T-022 保持 `todo`，本轮按用户要求停止；后续只有在用户明确启动且各自前置满足后执行。H-005 保持 accepted，等待发起人 architect-01 核对 A-016/T-020 后关闭。
- 更新日期：2026-09-10。

## T-019：实现预览同步与安全导出事务
- 负责人：multimedia-engineer-ffmpeg-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 FFmpeg 多媒体工程师执行；同一用户于 2026-09-12 明确确认全部前置条件已满足并要求执行 T-019。
- 目标与范围：在时间线、worker 和解码基础可用后实现播放主时钟、帧调度、seek/暂停/恢复、漂移诊断及固定修订导出事务；不决定 H.264 发布后端、格式产品范围或许可证策略，不实现视觉模板与音频 DSP。
- 输入与依赖：T-015、T-016、T-018、T-032、T-035；A-004 0.5、A-006 0.1 WP-04、A-007 0.1、A-011 0.1；H.264/容器发布矩阵仍待确认。
- 优先级：高；按前置条件排队。
- 完成条件与确认方式：明确并验证预览主时钟、视频帧选择、掉帧和漂移处理；导出开始冻结 `timelineRevision`、媒体/渲染/音频参数与随机种子，写临时文件后成功提交，取消/失败不覆盖已有目标；编码器接口可替换，在后端未定时只用已确认测试格式；对测试样例证明预览与导出事件位置误差不超过一个输出帧并保存日志；负责人完成自测并登记证据。
- 进展：multimedia-engineer-ffmpeg-01 于 2026-09-12 完成 T-019。`PreviewSynchronizer` 有音轨时仅按设备实际累计播放 frame 数推进，无音轨时按单调时钟推进；实现 VFR 实际时间戳选帧、暂停/恢复、可重置音频计数的 seek、presentation drop 与漂移诊断，播放头全部由 C++ `TimeNs` 状态产生。导出开始逐项冻结修订、媒体指纹/流选择、`RenderRecipe/RenderSnapshot`、T-032 音频参数/音色 hash、范围/fps/seed；直接接受 T-035 `RenderedFrame` 和 T-032 `RenderedPcm`。可替换编码器当前只允许显式 `testOnly` NUT/rawvideo/PCM，FFmpeg C API 完成写出；同目录临时文件仅在 trailer 成功后原子替换。取消、磁盘不足、编码失败、旧修订、错序和 worker 中断均 fail closed。
- 成果与验证证据：[A-026 0.1：预览同步与安全导出事务](artifacts/A-026-preview-synchronization-and-safe-export.md)、[T-019 验证摘要](evidence/T-019/verification-summary.md)及 `src/playback_export`/`tests/unit/playback_export_test.cpp`。Windows x64 Debug `/W4 /WX` 构建并实际 9/9 pass；CI/RelWithDebInfo 与 Release 均构建成功，但新测试程序均被既有 Code Integrity/WDAC `0xC0E90002` 阻止启动，未使用 fallback、未写成通过。Debug 真实 FFmpeg 产物由公开 `MediaSource` 重新探测到 video/audio 流，事件位置测试在 30 fps 下最大误差 33,000,000 ns，小于一帧上限 33,333,334 ns。
- 阻塞与下一位行动人：T-019 技术范围无剩余实现阻塞。H.264、发布容器/编码器矩阵与许可证仍未确认，因此不形成发布导出结论；`T021-ENV-001` 主机策略问题仍归原责任链处理。H-004 保持 accepted，等待发起人 architect-01 验收 A-026/T-019 后关闭。
- 更新日期：2026-09-12。

## T-018：实现 FFmpeg 探测、解码、时间映射与代理管线
- 负责人：multimedia-engineer-ffmpeg-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 FFmpeg 多媒体工程师执行。
- 目标与范围：依据已确认契约，用 FFmpeg C API 和 C++ RAII 实现只读媒体探测、解复用、视频/音频解码、格式归一、真实时间映射、代理帧/缩略图/波形源数据和 seek；不实现镜头/运动算法、节奏 DSP、QML 页面或最终发布编码矩阵。
- 输入与依赖：T-012/T-014 已完成，D-003 已确认 FFmpeg 库 API 路线；仍依赖 T-013、T-017；A-004 0.5、A-005 0.4、A-007 0.1。
- 优先级：高；按前置条件排队。
- 完成条件与确认方式：FFmpeg 资源全部使用可审计生命周期管理；CFR/VFR、旋转、多流和音频样例的 `timeNs`/定位符合契约；长素材使用有界队列和缓存，不整段加载；取消、损坏、不支持与资源不足返回结构化错误且不泄漏；记录 FFmpeg build configuration、格式能力矩阵、峰值内存和单元/集成自测；产出代码和可复核运行证据。
- 进展：multimedia-engineer-ffmpeg-01 于 2026-09-10 完成原 T-018，同日解决 T021-DEFECT-001；随后依据用户明确指令接收 H-011 并补齐媒体 PCM resampler timing provenance。公共版本提升为 `mediaContractVersion=1.0.0/schemaVersion=2`，`PcmBuffer` 实际填充声道顺序、segment 原点和逐转换 trace；delay 直接来自每次 `swr_convert` 前的 `swr_get_delay`，版本来自运行时 swresample，参数摘要来自版本化实际配置。新增样本精确 audio seek、动态采样率 segment/epoch、取消不排空及旧 schema 拒绝。固定 baseline 与 default/GPL/nonfree 状态未变；未修改 DSP、`package/` 或缓存目录。
- 成果与验证证据：[A-015 0.3](artifacts/A-015-ffmpeg-media-pipeline.md)、[A-014 0.4](artifacts/A-014-media-time-buffer-and-golden-contract.md)、[H-011 验证摘要](evidence/T-018/H-011-resampler-provenance.md)、[原可复核摘要](evidence/T-018/verification-summary.md)、[T021-DEFECT-001 修复证据](evidence/T-018/T021-DEFECT-001.md)、[运行时 DLL 哈希](evidence/T-018/runtime-dlls.sha256.csv)及[13 项实际黄金媒体 SHA-256/ffprobe 证据](../../tests/golden/media/generated/actual-hashes-and-probe-v1.json)。H-011 修订后 Windows x64 Debug、CI/RelWithDebInfo、Release 媒体专项均由最终二进制 27/27 通过；schema 1 独立拒绝 oracle 与非零 delay/连续 `firstSampleIndex` 断言均通过。
- 阻塞与下一位行动人：H-011 与 T021-DEFECT-001 已修复且无剩余媒体实现阻塞；H-011 发起人 audio-dsp-engineer-01 可在后续获授权任务中消费 schema 2 字段，DSP 不得重复补偿 delay。后续获用户明确授权的 T-019 已于 2026-09-12 完成。
- 更新日期：2026-09-12。

## T-017：定义媒体时间、缓冲契约与黄金样例矩阵
- 负责人：multimedia-engineer-ffmpeg-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 FFmpeg 多媒体工程师执行。
- 目标与范围：先于具体 FFmpeg 实现定义媒体信息、流选择、帧/PCM 缓冲、PTS/DTS/time_base 映射、VFR/旋转/颜色/seek 和结构化错误契约，并建立许可可核对的测试样例矩阵；不锁定发布编码器，不重定义核心规范时间或事件模型。
- 输入与依赖：A-004 0.5 第 4 节与 TR-F-001～003、TR-F-014～015，A-005 0.4，A-006 0.1 WP-04，A-007 0.1，A-012 0.1，D-003/D-006 confirmed；根据用户 2026-09-09 本轮指令以 A-012 规范 `timeNs` 为唯一核心时间。
- 优先级：高（用户要求本轮启动）。
- 完成条件与确认方式：定义 `MediaInfo`、旋转/SAR/DAR、流选择、帧/PCM 所有权/背压/生命周期；明确未知/负时间戳、start time、CFR/VFR、seek、采样索引和舍入规则；生成 CFR、VFR、旋转、采样率差异、损坏和缺失流样例或脚本，登记来源/许可/哈希和期望时间向量；与 T-014 契约无同义冲突；形成并登记版本化契约成果，负责人自查，无独立评审要求。
- 进展：multimedia-engineer-ffmpeg-01 于 2026-09-09 接收 H-004 并完成本任务。已定义 `MediaInfo`、显式流选择、PTS/DTS/time_base/start time 与负/未知时间戳处理、CFR/VFR、seek、采样索引、旋转/SAR/DAR/颜色、帧/PCM lease、背压、动态格式 epoch 和结构化错误；全部纳秒结果直接使用 A-012 `TimeNs`，未建立同义时间模型。已建立 10 个 CC0 合成/固定字节样例配方和 28 个精确时间向量，未执行 T-018/T-019。
- 成果与验证证据：[A-014 0.2：媒体时间、流、缓冲与黄金样例契约](artifacts/A-014-media-time-buffer-and-golden-contract.md)，状态 draft；[fixtures-v1.json](../../tests/golden/media/fixtures-v1.json)、[manifest 验证器](../../tests/golden/media/Test-GoldenMediaManifest.ps1)、[可复现生成器](../../tests/golden/media/Generate-GoldenMedia.ps1)和[许可声明](../../tests/golden/media/LICENSE.md)。T-018 使用固定 FFmpeg 后已扩展并实际生成 12 个样例、30 个时间向量及真实媒体 SHA-256/ffprobe 证据；`mediaContractVersion` 仍为 0.1.0，既有向量语义未变。
- 阻塞与下一位行动人：本任务无阻塞且已完成。后续明确授权的 T-018、T-019 亦已完成；H-004 发起人 architect-01 可核对完整交付。
- 更新日期：2026-09-12。

## T-016：实现 worker、版本化 IPC、项目存储与恢复骨架
- 负责人：core-systems-engineer-cpp-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 C++ 核心/系统工程师执行。
- 目标与范围：建立长任务进程隔离、作业状态机、版本化 IPC、取消/诊断、项目 schema/迁移、原子保存、恢复和可重建缓存；先用 mock job 证明系统语义，不实现媒体/算法任务本体，不让 worker 直接持有 UI `QObject`。
- 输入与依赖：T-013～T-015 已完成，D-003 已确认；A-004 0.5、A-006 0.1 WP-08、A-007 0.1。
- 优先级：高；按前置条件排队。
- 完成条件与确认方式：状态机覆盖 queued/running/cancelling/succeeded/failed/cancelled，具备版本握手、幂等请求、进度、取消、超时和诊断 ID；IPC 不用 JSON 复制大帧/PCM；项目保存/迁移/自动恢复/素材重定位/缓存指纹和临时结果提交可测；worker 崩溃、磁盘不足、缓存损坏不破坏最近成功保存；负责人提交开发自测和故障注入证据。
- 进展：2026-09-09，已交付纯作业状态机、schema/protocol 1 长度前缀本地 IPC、同用户权限与强制版本握手、mock Worker、幂等请求/取消、进度/超时/崩溃/诊断 ID、过期修订保护；已交付项目 schema 2、v1→v2 迁移、原子主保存/自动保存、带来源及时间的异常恢复、指纹素材重定位，以及带格式/长度/SHA-256 校验和配额裁剪的可重建缓存。JSON IPC 只承载小型控制元数据，大帧、PCM、采样和像素等通过 file/cache 引用传递；公共系统接口无 Qt 类型。
- 成果与验证证据：实现位于仓库 `src/system`，mock 入口位于 `src/worker/main.cpp`，GoogleTest 位于 `tests/unit/job_system_test.cpp`、`ipc_system_test.cpp` 和 `project_store_test.cpp`；[T-016 可复核验证摘要](evidence/T-016/verification-summary.md)记录故障矩阵和 MSVC 19.44 `/W4 /WX` 下 Debug、Release、CI 三套 Windows x64 CTest 32/32 通过结果。
- 阻塞与下一位行动人：T-016 无剩余阻塞并已按完成条件结束；H-003 由目标成员完成处理记录，保持 accepted，等待发起人 architect-01 核对后关闭。本会话按用户边界停止，不启动媒体、UI、CV 或 DSP 任务。
- 更新日期：2026-09-09。

## T-015：实现事件时间线事务、撤销与确定性融合核心
- 负责人：core-systems-engineer-cpp-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 C++ 核心/系统工程师执行。
- 目标与范围：依据核心 0.x 契约实现事件增删移锁、批量偏移、命令/事务、撤销重做、分析候选融合、密度和修订冲突检查；不实现 QML 控件、媒体解码、CV/DSP 算法或视觉效果。
- 输入与依赖：T-013 的纯 C++ x64 构建预设、T-014；A-004 0.5、A-006 0.1 WP-02、A-007 0.1。
- 优先级：高；按前置条件排队。
- 完成条件与确认方式：人工及锁定事件在重分析后保持；合并按单事务提交并可撤销；失败/取消不改变当前修订；过期 revision 被结构化拒绝；相同输入、算法版本、参数和种子产生等价事件集合；公共 API、关键不变量、边界和并发语义具备开发自测；形成可供 UI/worker/测试消费的无 Qt Quick 依赖库。
- 进展：2026-09-09，已基于 A-012 0.1/A-013 0.1 完成纯 C++20 时间、轨道/事件、不可变快照、原子事务、撤销/重做、稳定排序、锁定/人工保护、确定性融合、密度限制及 timeline/analysis 版本冲突实现；72 个 A-012 唯一向量 ID 已全部接入 GoogleTest，并补充并发提交点、快照不变性和融合策略自测。
- 成果与验证证据：核心公共 API 与实现位于仓库 `src/domain`，GoogleTest 位于 `tests/unit/core_contract_vectors_test.cpp`；[T-015 可复核验证摘要](evidence/T-015/verification-summary.md)记录向量 72/72 对应、MSVC 19.44 `/W4 /WX` 编译及 Debug/Release/CI 三套 Windows x64 CTest 16/16 通过结果。
- 阻塞与下一位行动人：T-015 无剩余阻塞；按本轮用户边界停止，不启动 T-016。后续由用户另行授权 T-016。
- 更新日期：2026-09-09。

## T-014：定义规范时间、事件、修订和事务核心契约 0.x
- 负责人：core-systems-engineer-cpp-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 C++ 核心/系统工程师执行。
- 目标与范围：冻结前先形成无 QML/FFmpeg 依赖的核心 0.x 契约，拥有规范 `timeNs`、事件、轨道、修订、锁定/人工编辑、错误和事务语义；不决定媒体 PTS 的具体解释，不改变 A-004 产品/技术范围，不把 proposed 技术栈写成定案。
- 输入与依赖：A-004 0.5 第 4 节、TR-A-002～005、TR-F-004/006/007，A-006 0.1 WP-02，A-007 0.1；与 T-017 对齐媒体边界，与 T-020 对齐测试向量。
- 优先级：高（用户要求本轮启动）。
- 完成条件与确认方式：定义有符号 64 位 `timeNs`、带检查的换算/溢出/舍入规则；定义 `RhythmEvent`、来源/强度/置信度、锁定/人工编辑、`timelineRevision`、`analysisRevision`、错误 taxonomy 与诊断 ID；定义命令和事务提交、取消/失败、DTO/schema 版本兼容；提供正常、边界和错误测试向量；媒体层能映射进来、测试层能独立断言且无同义模型；形成并登记版本化契约成果，负责人自查，无独立评审要求。
- 进展：core-systems-engineer-cpp-01 已依据 A-010 完成核心契约 0.1，定义 TimeNs/ProjectTimeNs、轨道、RhythmEvent/AnalysisCandidate、timelineRevision/analysisRevision、原子事务、幂等与取消、撤销重做和保存点、稳定错误 taxonomy 及编码中立的 DTO/schema 兼容规则；未启动 T-015，未创建业务源码。
- 成果与验证证据：[A-012 0.1：核心时间、事件、修订与事务契约 0.x](artifacts/A-012-core-domain-contract-0x.md)，状态 draft；包含 72 条具有唯一稳定 ID、具体输入和可判定预期的 TIME/EVENT/TXN/MERGE/HISTORY/DTO 测试向量。负责人已完成需求追踪、术语与责任边界、舍入和负数预期、事务原子性、锁定保护、幂等顺序、错误码、兼容行为、向量唯一性、本地链接及尾随空白自查；72 条向量 ID 无重复，引用文件均存在，bundled Python `Fraction` 对关键正负舍入向量的独立复算结果为 `TIME_VECTOR_ARITHMETIC=PASS`。仓库尚无 T-013 业务构建骨架，本任务为契约成果，因此未执行 C++ 编译或自动化测试。
- 阻塞与下一位行动人：本任务无阻塞并已按完成条件结束。D-003 已在任务完成后由用户确认；T-015 保持 todo，当前只等待 T-013 的纯 C++ x64 工程骨架。A-012 可由 T-017 复用规范时间边界、由 T-020 按原向量 ID 建立独立断言。
- 更新日期：2026-09-09。

## T-013：建立应用、worker、核心库和测试的 CMake/CI 工程骨架
- 负责人：build-engineer-windows-qt-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 Windows/Qt 构建工程师执行。
- 目标与范围：依据已确认的 D-003/D-006 先建立纯 C++ 核心、媒体适配和测试目标，再基于已完成的可复现 Qt SDK 补齐应用、worker 与 Qt 目标的 CMake Presets/Ninja 工程骨架、依赖清单和 Windows CI；不实现业务 UI/算法，不批准签名或发布。
- 输入与依赖：T-011/T-012 已完成；D-003、D-006、D-007、D-008 confirmed；A-005 0.4 第 7、9 节，A-006 0.1 WP-01，A-007 0.1，A-008 0.3，A-009 0.1，A-012 0.1。
- 优先级：高；按前置条件排队。
- 完成条件与确认方式：建立 A-005 建议目录的最小可链接目标；提供开发/CI presets、编译警告、x64/运行库/ABI 保护、统一测试入口和依赖 manifest/baseline；x86/ARM64 配置明确失败；CI 可配置、编译、运行无 GUI 单测和最小 QML 冒烟，并检查运行时依赖；保存构建日志和产物清单；负责人自查，不包含正式安装器/签名批准。
- 进展：build-engineer-windows-qt-01 已完成应用、Worker、纯 C++ 核心、媒体适配和测试的 Windows x64 工程骨架；加入 CMake 3.31/Ninja 的 Debug、Release、CI presets，MSVC 19.44/x64/C17/C++20/动态 CRT/精确 Qt 6.11.2 门禁，固定 vcpkg manifest/baseline 及 FFmpeg/OpenCV/KissFFT 可选入口，GoogleTest/CTest、Qt Test/Qt Quick Test、应用 QML、Worker 进程、x86/ARM64 拒绝测试，Windows CI、部署、PE/依赖核对和逐文件哈希证据。只复用 T-012 SDK，未重新构建 Qt，未实现业务逻辑，未修改 `package/`。2026-09-10 按用户明确授权完成 T021-ENV-001 后续诊断：旧 Release core 哈希稳定触发 Win32 4551/Code Integrity `0xC0E90002`；原路径 clean rebuild 与全新目录 rebuild 后 core 均可运行，当前 26/26 实际通过；严格 Release headless 全套仍为 53/63，未使用 fallback，未把其余策略阻断改写为通过。
- 成果与验证证据：[A-013 0.2：Windows x64 CMake/Ninja 工程与 CI 骨架](artifacts/A-013-windows-x64-cmake-ci-skeleton.md)，状态 draft；[T-013 可复核摘要](evidence/T-013/verification-summary.md)保留 0.1 历史基线；[T021-ENV-001 专项诊断](evidence/T-013/t021-env-001-verification-summary.md)及其 [Code Integrity 事件摘录](evidence/T-013/t021-env-001-code-integrity-events.json)。Debug、Release、CI 的 PE/CRT/导入/签名/路径/启动已对照；完整机器日志位于被忽略的 `out/evidence/T021-ENV-001`。
- 阻塞与下一位行动人：T-013 工程骨架本身无剩余阻塞并保持 completed；T021-ENV-001 不能由测试入口降级关闭，需主机策略管理员依据 Policy GUID `{0283ac0f-fff1-49ae-ada1-8a933130cad6}` 提供组织管理的非生产开发签名路线，或在 ACL 受控、非用户可写的专用构建根上建立最小补充策略，再由 tester-cpp-qt-01 严格复跑 Release 63/63。最低 Windows 版本仍需在后续兼容/发布任务中确认。H-002 保持 accepted，等待发起人 architect-01 核对 T-011～T-013 结果后关闭。
- 更新日期：2026-09-10。

## T-012：从官方源码构建可复现的 Windows x64 Qt SDK
- 负责人：build-engineer-windows-qt-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 Windows/Qt 构建工程师执行；Qt 源码构建和 x64 已由此前用户要求确认。
- 目标与范围：依据已确认的 MSVC 2022 Build Tools x64 和 LGPLv3/shared 路径，在用户确认 Qt 确切版本后，从固定官方源码建立可复现 x64 shared Qt SDK，并验证最小 Qt Quick/QML 程序；不自行改变 D-006/D-007，不使用预编译 Qt 包，不把 `-developer-build` 当交付 SDK。
- 输入与依赖：T-011；D-002、D-004、D-005、D-006、D-007、D-008 confirmed；A-004 0.5、A-005 0.3、A-007 0.1、A-008 0.3。
- 优先级：高；按前置条件排队。
- 完成条件与确认方式：源码、构建和安装目录隔离且路径短；登记源码来源/版本/哈希、编译器/SDK/CMake/Ninja/Python 版本、完整 configure 参数/摘要、模块白名单、产物/符号/运行库清单和哈希；完成 Debug/Release 约定配置及最小 QML x64 编译运行/部署暂存冒烟；证明 Qt 与同进程原生依赖 ABI/运行库一致；形成脚本、说明和实际日志，负责人自查。
- 进展：用户于 2026-09-08 明确要求“启动 T-012”后，已安装并验证 Visual Studio Build Tools 2022 17.14.39、MSVC 19.44.35228.0、Windows SDK 10.0.26100.0、CMake 3.31.6-msvc6、Ninja 1.12.1 和 Python 3.13.15 x64。Qt 6.11.2 官方源码归档的 1,019,661,552 字节及 SHA-256 已与 D-008 完全核对；已在 `C:\sr` 隔离短路径完成 x64 shared Release/Debug 配置、构建和安装。独立 CMake 消费端已完成 Qt Quick/QML + Multimedia 双配置编译、`windeployqt` 部署与 offscreen 运行；Qt 版本、x64 ABI、QML load、`MediaPlayer` 实例化、事件循环和 Release/Debug CRT 均通过实际验证。最终日志和三份 SHA-256 清单已生成，任务完成条件全部满足。
- 成果与验证证据：[A-009 0.1：Windows x64 Qt 6.11.2 源码 SDK 构建与验收记录](artifacts/A-009-windows-qt-6.11.2-source-sdk-build.md)；[T-012 可复核摘要](evidence/T-012/verification-summary.md)；复现入口为仓库 `tooling/qt/Invoke-Qt6112Build.ps1`，完整机器日志和清单位于 `C:\sr\evidence\T-012`，SDK 位于 `C:\sr\q\qt6112`。负责人已逐项自查，最终 configure/build/install/smoke/manifest 均退出 0。
- 阻塞与下一位行动人：T-012 无剩余阻塞。D-003 已确认，T-013 的 Qt SDK 和技术组合前置均已满足；由 build-engineer-windows-qt-01 依据 H-002 启动工程骨架与 CI 基线。最低 Windows 版本仍未确认，不影响 T-013 工程骨架，但会约束后续兼容与发布验收。
- 更新日期：2026-09-08。

## T-011：审计 Windows 构建环境并形成 D-006 决策输入
- 负责人：build-engineer-windows-qt-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确要求把任务细化后交给已创建的 Windows/Qt 构建工程师执行。
- 目标与范围：对当前 Windows x64 构建环境做只读事实盘点，比较 MSVC 2022 Build Tools 与全链路 MinGW-w64 对 Qt 源码、FFmpeg/C++ 依赖、ABI、调试和 CI 的影响，向用户提供 D-006 的可核对决策输入；不安装软件、不下载源码、不编译 Qt、不替用户确认技术决定。
- 输入与依赖：D-002、D-004、D-005 confirmed，D-006 proposed；A-004 0.5、A-005 0.3、A-006 0.1 WP-01、A-007 0.1；当前主机只读环境信息。
- 优先级：高（用户要求本轮启动）。
- 完成条件与确认方式：盘点 Windows/CPU/内存/磁盘、`vswhere`、MSVC/Windows SDK、MinGW、CMake、Ninja、Python、Git 与环境变量；区分已安装、可发现、版本不满足和缺失；从 Qt 支持范围、x64 ABI、依赖可得性、调试/CI、安装体量与许可输入比较两路线，给出推荐、风险、最小安装清单和验证命令；列出 Qt 版本/许可证/最低 Windows 版本等仍需决定项；形成并登记版本化报告，所有结论可由命令输出复核。
- 进展：build-engineer-windows-qt-01 已于 2026-09-08 接收 H-002，完整读取任务输入并完成当前主机只读盘点。已核对 Windows/硬件/磁盘、Visual Studio/MSVC/Windows SDK、MinGW、CMake/Ninja/Python/Git、Qt/vcpkg、相关环境变量、长路径策略及仓库构建骨架；未下载、安装或编译任何组件。已基于 Qt 与 Microsoft 官方资料完成 MSVC/MinGW 比较和后续执行方案。
- 成果与验证证据：[A-008 0.3：Windows 构建环境审计与执行方案](artifacts/A-008-windows-build-environment-audit-and-execution-plan.md)，状态 draft。当前主机为 Windows 11 x64，硬件与磁盘容量可进入构建准备；Git 2.55.0.windows.4 可用，未发现 MSVC/Windows SDK、MinGW、CMake、Ninja、Qt 或 vcpkg，系统 `python.exe` 仅为不可用的 Windows Store 别名。报告保存审计范围、命令复核入口、路线比较、已确认的 MSVC/LGPLv3/Qt 6.11.2 基线、最小安装清单与 T-012/T-013 门禁；负责人已完成覆盖和决定边界自查。
- 阻塞与下一位行动人：本任务无阻塞并已按完成条件形成版本化报告。用户后续已确认 D-003、D-006、D-007 与 D-008，T-012 也已完成；T-013 当前可由 build-engineer-windows-qt-01 启动。
- 更新日期：2026-09-08。

## T-010：细化四位研发成员任务并发起执行交接
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-08 明确列出已创建的 Windows/Qt 构建、C++ 核心/系统、FFmpeg 多媒体、C++/Qt 测试四位研发同事，并要求“把任务拆解的再细些，然后让他们就执行”。
- 目标与范围：核对四位成员实际 ID、岗位 scope 与岗位知识，将 A-006 工作包细化为可执行任务、依赖、产物、验收和契约责任，登记面向各成员的启动交接；不冒充成员接收任务，不把任务登记当成会话已启动，不修改项目经理负责的 STATUS.md，不把当前人员未覆盖领域写成已经有人承担。
- 输入与依赖：TEAM.yaml、四位成员配置与岗位知识；D-001、D-002、D-004、D-005 confirmed，D-003/D-006 proposed；A-004 0.5、A-005 0.3、A-006 0.1；本轮用户直接授权。
- 优先级：高（用户要求本轮启动）。
- 完成条件与确认方式：形成 A-007 0.1，登记 T-011～T-022 和 H-002～H-005；每人具有一个无技术前置阻塞的首任务及两个带明确依赖的后续任务；说明现有四人覆盖范围、未覆盖岗位、共享契约唯一责任人和停止条件；负责人完成成员 ID/scope、编号、链接、状态和决定边界自查，无独立评审要求。
- 进展：已完成四位成员身份与岗位边界核对，将研发工作分成 E0 契约/准备、E1 基础实现和 E2 集成三批；已登记任务与开放交接，等待四位成员分别在自己的项目会话中接收。
- 成果与验证证据：[A-007 0.1：四人研发任务细化与启动计划](artifacts/A-007-four-engineer-execution-plan.md)，状态 draft；T-011～T-022；H-002～H-005。
- 阻塞与下一位行动人：本文档与登记任务无阻塞。下一位行动人为四位目标成员，分别接收 H-002～H-005 并把 T-011/T-014/T-017/T-020 更新为 in_progress；D-003/D-006 等决定仍由用户确认。
- 更新日期：2026-09-08。

## T-009：按专业领域拆分技术需求
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 要求“你先把这些技术需求按不同的领域拆分下，后面我找专人来开发”。
- 目标与范围：把现有一期技术需求拆成可供后续专业人员独立承接、并行开发和验收的领域工作包；每个工作包明确专业画像、范围、输入、输出、接口、验收和依赖，并定义跨领域共享契约与集成顺序。不创建新成员、不替项目经理指派负责人，不把 D-003/D-006 proposed 内容写成已确认决定。
- 输入与依赖：D-001、D-002、D-004、D-005 confirmed，D-003/D-006 proposed，[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.3](artifacts/A-005-mvp-technology-stack-proposal.md)及本轮用户拆分要求。
- 优先级：未设定。
- 完成条件与确认方式：形成 A-006 0.1 draft，覆盖 A-004 的 TR-A-001～006、TR-F-001～018、TR-Q-001～010，给出领域边界、共享契约、依赖阶段、招募组合和待确认门槛；负责人完成需求覆盖、责任无重叠/无遗漏、本地链接与状态自查，无独立评审要求。
- 进展：已将技术需求拆为平台构建、核心领域、Qt UI、媒体同步、视频算法、音频 DSP、图形渲染、worker/存储、测试性能、发布合规十个工作包，并定义跨包契约和推荐启动顺序。
- 成果与验证证据：[A-006 0.1：一期技术需求领域拆分与专业工作包](artifacts/A-006-domain-work-packages.md)，状态 draft；需求覆盖矩阵逐项映射 A-004 技术需求。
- 阻塞与下一位行动人：本文档任务无阻塞。正式启动开发前，用户需确认 D-006 编译器路线并决定是否整体接受 D-003；项目经理或用户可据 A-006 安排人员，新增项目成员仍由超级管理员入口按实际岗位和 scope 创建。
- 更新日期：2026-09-07。

## T-008：确认 Windows x64 与 Qt 源码构建要求
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 明确表示“Qt 匹配的 MSVC 工具链也没有，需要从零开始编译qt源码，架构就x64”。
- 目标与范围：把 Windows x64 和 Qt 官方源码自行构建转成可执行的工程约束，明确源码/构建/安装隔离、构建可追溯性和原生 ABI 一致性；说明源码构建仍需外部编译器，并提出 MSVC 与 MinGW 的待选方案。此任务不安装工具、不实际编译 Qt，也不替用户选择尚未确认的编译器、Qt 版本或许可证。
- 输入与依赖：D-002、D-003 proposed、D-004、[A-004 0.4](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.2](artifacts/A-005-mvp-technology-stack-proposal.md)、本轮用户约束及 Qt 官方 Windows 源码构建文档。
- 优先级：未设定。
- 完成条件与确认方式：登记 D-005 confirmed 和 D-006 proposed；将 A-004 更新到 0.5、A-005 更新到 0.3，明确 x64、Qt 源码构建前置工具、ABI、目录、版本与复现要求；负责人完成状态、版本、链接和冲突自查，无独立评审要求。
- 进展：已确认 Windows x64 和 Qt 源码构建方式；已识别“当前无编译器工具链”为正式构建前置缺口，并形成 MSVC 2022 Build Tools 优先、MinGW-w64 统一 ABI 备选的方案。
- 成果与验证证据：D-005、D-006 proposed；[A-004 0.5](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)；[A-005 0.3](artifacts/A-005-mvp-technology-stack-proposal.md)。
- 阻塞与下一位行动人：本文档任务无阻塞。实际 Qt 构建在 D-006 确认且对应编译器、Windows SDK（MSVC 路线）、CMake、Ninja、Python 3 可用前不能开始；下一位行动人为用户确认 MSVC 2022 Build Tools 路线，或明确禁止 MSVC 并选择 MinGW-w64 路线。
- 更新日期：2026-09-07。

## T-007：确认第一阶段 Windows 平台范围
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 明确表示“首先考虑windows平台，linux平台暂不考虑”。
- 目标与范围：把第一阶段目标系统收敛为 Windows，明确 Linux 不进入本阶段的设计目标、构建、CI、分发和验收范围；同步修正架构与技术栈文档中的跨平台表述。此任务不替用户确认 D-003 的其余技术栈，也不擅自确定最低 Windows 版本、CPU 架构或安装器形式。
- 输入与依赖：D-001、D-002、D-003 proposed、[A-004 0.3](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)、[A-005 0.1](artifacts/A-005-mvp-technology-stack-proposal.md)及本轮用户平台约束。
- 优先级：未设定。
- 完成条件与确认方式：登记 confirmed 决定 D-004；将 A-004 更新到 0.4、A-005 更新到 0.2，删除一期 Linux 编译/CI/分发/验收要求并保留必要的平台解耦边界；负责人完成术语、版本、索引和本地链接自查，无独立评审要求。
- 进展：已确认 Windows 为第一阶段唯一目标操作系统；已从工程和验收范围移除 Linux，并将最低 Windows 版本等具体发布参数保留为待确认项。
- 成果与验证证据：D-004；[A-004 0.4](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)；[A-005 0.2](artifacts/A-005-mvp-technology-stack-proposal.md)。
- 阻塞与下一位行动人：本任务无阻塞。CPU 架构后由 D-005 确认为 x64；最低 Windows 版本、安装器/签名方式仍需后续确认，D-003 其余技术栈继续等待用户确认或调整。
- 更新日期：2026-09-07。

## T-006：确认 Qt Quick/QML 并形成其余技术栈建议
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 回复“OK，我理解了那就使用Qt Quick/QML，其他的技术选择呢”。
- 目标与范围：把 Qt Quick/QML 作为已确认界面形态写入 D-002 和 A-004，并在已确认产品边界内提出一期构建、媒体、视频/音频算法、渲染、并发隔离、持久化、测试、依赖和分发技术栈；区分已确认方向、架构建议和仍需用户/原型确认的许可与版本问题。
- 输入与依赖：D-001、D-002、A-002 0.2、[A-004 0.2](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)，以及 Qt、FFmpeg、OpenCV、CMake/vcpkg 和测试框架的当前官方资料。
- 优先级：未设定。
- 完成条件与确认方式：更新 D-002 和 A-004 0.3，形成 A-005 0.1 技术栈建议，并以 D-003 proposed 汇总等待确认的其余技术基线；负责人自查范围、依赖方向、许可证风险和验证门槛，无独立评审要求。
- 进展：已确认 Qt Quick/QML；已完成一期技术栈分层、组件选择、排除项、目录边界、版本策略、许可证门槛和首个垂直切片建议。
- 成果与验证证据：D-002 更新；D-003 proposed；[A-004 0.3](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)；[A-005 0.1](artifacts/A-005-mvp-technology-stack-proposal.md)。
- 阻塞与下一位行动人：本任务无阻塞。D-003 尚待用户确认或调整；目标平台已由 D-004 收敛为 Windows，在 Qt 许可证路径、H.264 编码后端和最低 Windows 版本未确认前，不冻结发布构建。
- 更新日期：2026-09-07。

## T-005：落实 Qt 界面与 C/C++ 算法技术方向
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 明确提出“我的想法界面使用qt，算法使用c/c++来实现”。
- 目标与范围：把用户给出的界面框架和算法实现语言转成项目技术基线，明确 Qt 界面/应用层与独立 C/C++ 算法核心的依赖边界，更新 A-004；不在本任务中擅自确定 Qt Widgets/Qt Quick/QML 形态、具体版本、编译器、第三方库许可或发布平台顺序。
- 输入与依赖：D-001、A-002 0.2、[A-004 0.1](artifacts/A-004-mvp-technical-feasibility-and-requirements.md) 及本轮用户技术方向。
- 优先级：未设定。
- 完成条件与确认方式：登记已确认技术决定 D-002；将 A-004 更新为 0.2，明确 Qt 与 C/C++ 技术基线、模块依赖方向、跨边界数据/内存/线程契约和待细化项；负责人自查记录与用户原话一致，无独立评审要求。
- 进展：已确认当前身份、岗位和项目资料未发生阻断变化；已登记 D-002，并把 A-004 的候选栈改为已确认基线，新增六项架构实现约束。
- 成果与验证证据：D-002；[A-004 0.2](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)。Qt UI 形态和 C/C++ 具体标准保持为后续技术细化项，没有超出用户本轮方向。
- 阻塞与下一位行动人：无。Qt Quick/QML 已由后续 T-006 与 D-002 确认；原型启动前仍需由架构与研发确定 Qt minor 版本、许可证路径、编译器和构建预设。
- 更新日期：2026-09-07。

## T-004：评估一期产品技术可行性并形成技术需求
- 负责人：architect-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 直接指派“根据产品需求判断技术的可实现性，并将其转成为技术需求”。
- 目标与范围：依据 D-001 已确认方向和 A-002 0.2 已批准产品需求，判断第一阶段独立桌面应用的技术可实现性，将产品能力转换为可实现、可验证的系统边界、模块职责、数据与接口契约、功能和质量属性要求，并列明关键风险、验证门槛与待产品确认项；不擅自扩大产品范围，不把候选技术选型写成已批准决定，也不授权生产部署。
- 输入与依赖：[A-002 0.2](artifacts/A-002-audio-visual-rhythm-product-brief.md)、D-001；[A-001 0.1](artifacts/A-001-package-static-analysis.md) 与 [A-003 0.1](artifacts/A-003-software-functional-reverse-analysis.md) 作为媒体处理、分发风险和参考能力边界的静态证据，二者 draft 状态不作为本项目实现来源或运行验收结论。
- 优先级：未设定。
- 完成条件与确认方式：形成 A-004 0.1 draft，包含总体可行性结论、假设和边界、建议架构、核心技术契约、可追踪的功能与非功能技术需求、验证计划、风险及待确认项；由负责人按 A-002 0.2 与 D-001 自查，本任务不要求独立评审或用户批准，成果是否转为 approved 另按项目确认流程处理。
- 进展：已刷新 architect-01 身份、岗位和有效知识，完整读取 A-002 0.2、D-001、相关项目记录，并核对 A-001 0.1 与 A-003 0.1 的参考能力和分发风险；已完成总体可行性判断、建议模块边界、统一时间与事件契约、18 项 P0 功能需求、10 项质量属性需求、产品需求追踪、风险控制和 G0～G4 验证门槛。
- 成果与验证证据：[A-004 0.1：一期 MVP 技术可行性与技术需求](artifacts/A-004-mvp-technical-feasibility-and-requirements.md)，状态 draft。负责人已按 A-002 0.2 全部 P0 条目自查追踪关系，并将已确认范围、工程假设、候选选型和 8 项待确认输入分开记录；结论为“有条件可实现，允许进入算法原型和端到端垂直切片，不在验证门槛通过前承诺完整 MVP 效果或工期”。
- 阻塞与下一位行动人：本任务无阻塞，已按约定形成候选稿。目标平台已由 D-004 收敛为 Windows；最低 Windows 版本、典型素材与格式/规模边界、基准硬件、离线或云端边界、音色许可及“卡点自然”评分口径仍需用户/产品侧确认。下一位行动人为 project-manager-01，可在接收现有 H-001 后依据 A-004 协调 G0/G1 原型计划，成果批准仍由项目确认流程另行处理。
- 更新日期：2026-09-07。

## T-003：逆向梳理安装包所含软件的完整功能面
- 负责人：developer-reverse-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 直接指派“进一步帮我分析下这几款软件的所有功能”。
- 目标与范围：基于 `package/` 中与 A-001 0.1 同版本的样本，深入梳理 `osci-render`、`sosci`、其独立程序与 VST3 形态以及 Blender 插件的静态可见功能；覆盖用户工作流、输入输出、项目与预设、音频/MIDI、可视化、效果器、录制/视频导出、互联、设置、许可限制和异常路径，并区分直接证据、静态推断和待动态验证。这里的“所有功能”以当前样本可静态恢复的功能面为边界，不把通用 JUCE/系统库能力或孤立字符串误报为产品功能。
- 输入与依赖：`package/` 中 5 个 ZIP；[A-001 0.1](artifacts/A-001-package-static-analysis.md) 的样本哈希、包结构与安全基线。A-002 及 D-001 属于产品候选方向，不作为逆向结论依据。
- 优先级：未设定。
- 完成条件与确认方式：形成 A-003 draft 功能逆向报告，按产品和交付形态给出可核对的功能分类、关键类/符号/字符串或源码证据、产品间差异、置信度与未覆盖项；负责人自查，无独立评审或用户批准要求。
- 进展：已完成 Linux 独立程序与 VST3 的产品符号/字符串二次提取、交付形态编译单元对比、sosci 内嵌项目参数恢复、VST 元数据核对和 Blender 源码逐功能审查；已按产品、工作流和交付形态重建静态可见功能面。
- 成果与验证证据：[A-003 0.1：安装包软件功能逆向报告](artifacts/A-003-software-functional-reverse-analysis.md)。报告列出 osci-render 的输入/生成器、MIDI 合成、26 种内置效果与 Lua 自定义效果、调制和 Blender 互联，sosci 的音频示波器能力，两者共用的显示/录制/离线导出，以及 VST3/平台差异；同时给出 ELF 虚拟地址、文件偏移、VST metadata 和 Blender 源码位置作为复核索引。
- 阻塞与下一位行动人：无。负责人已完成静态自查；报告列出的动态验证不属于本任务完成条件，如需执行应由用户另行授权隔离环境。
- 更新日期：2026-09-07。

## T-002：定义音画双向节奏创作工具的产品方向与 MVP
- 负责人：product-manager-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 直接提出“将声音做成可视化”以及“有视频可以生成对应卡点的音频，有节奏感”的产品设想。
- 目标与范围：澄清声音驱动视觉和视频驱动节奏音频两条用户流程，提炼可共享的产品能力，形成目标用户、核心价值、MVP 范围、非目标、验收标准及待决定事项的候选需求；本任务不决定具体算法、框架或工程实现。
- 输入与依赖：用户本轮产品设想；A-001 0.1 的样本边界、安全与分发风险；A-003 0.1 对 osci-render、sosci、独立程序、VST3 和 Blender 插件功能面的静态重建。两份逆向报告均为参考输入，不等同于获准复刻其实现、协议、素材或品牌。
- 优先级：未设定。
- 完成条件与确认方式：形成一份可供用户评审的候选产品需求，覆盖双向流程、建议的一期切片、可验证验收条件和关键待决定项；由本会话用户确认目标用户与一期主路径后完成，后续技术可行性由相应技术岗位评估。
- 进展：已依据 A-003 0.1 将 A-002 修订为 0.2。逆向输入表明参考软件在音频/MIDI、示波器、效果调制、脚本、VST3、Blender 和专业互联上功能完整，但没有确认“从普通视频的镜头/运动事件生成节奏音轨”的工作流。用户于 2026-09-07 回复“OK，把你生成的内容提交一把”，确认采用独立桌面应用、视频到可编辑节奏音轨为主、音频到基础视觉为辅的第一阶段方向；D-001 已确认。
- 成果与验证证据：[A-002 0.2：音画双向节奏创作工具产品需求](artifacts/A-002-audio-visual-rhythm-product-brief.md)，输入版本为 A-001 0.1 与 A-003 0.1，状态 approved，批准依据为 D-001。
- 阻塞与下一位行动人：无。后续协调请求见 H-001；下一位行动人为 project-manager-01，接收后依据已确认需求协调项目目标摘要和技术可行性评估计划。
- 更新日期：2026-09-07。

## T-001：分析 package 目录中的安装包
- 负责人：developer-reverse-01
- 状态：completed
- 授权来源与日期：本会话用户于 2026-09-07 直接指派“我把待分析的安装包已经放在package目录了，你来帮我分析下”。
- 目标与范围：对 `package/` 中现有安装包开展不执行样本的静态分析，记录样本标识与 SHA-256，识别压缩包结构、目标平台、可执行文件格式与架构、直接相关依赖、关键程序与安装行为线索，并形成可复核报告；不上传样本、不修改原始安装包、不在当前日常主机直接运行未知程序。
- 输入与依赖：`package/` 中 5 个 ZIP 安装包；以任务登记后实际计算的文件清单、大小和 SHA-256 为准。
- 优先级：未设定。
- 完成条件与确认方式：形成一份 draft 静态分析报告，包含样本清单、环境与方法、包结构、主要二进制及格式/架构、依赖和行为线索、证据与复现命令、结论、限制和待动态验证项；负责人自查，无独立评审或用户批准要求。
- 进展：已完成 5 个 ZIP 的哈希与包结构核对、Linux ELF 分析、Windows Inno Setup 6.7.0 元数据及载荷拆取、PE/ELF 安全属性和依赖检查、关键行为字符串取证，以及 Blender 插件源码审查；全程未执行样本。
- 成果与验证证据：[A-001 0.1：package 安装包静态分析报告](artifacts/A-001-package-static-analysis.md)。报告记录了全部外层和主要内层文件 SHA-256、文件偏移证据、复现命令与动态验证边界；Windows 安装器及抽出载荷的 Authenticode 状态为 `NotSigned`。
- 阻塞与下一位行动人：无。动态验证不在本任务授权范围内，已列入 A-001 待办；如需继续，应由用户另行授权在隔离环境执行样本。
- 更新日期：2026-09-07。

状态与登记权限见根 [项目运行协议](../../PROJECT_PROTOCOL.md)。

## 记录样式（不是真实任务）

```text
## T-001：任务标题
- 负责人：<member-id>
- 状态：todo
- 授权来源与日期：<用户直接指派或项目经理安排的可核对依据>
- 目标与范围：<实际授权的结果和边界>
- 输入与依赖：<任务、决定、资料版本；无则写无>
- 优先级：未设定
- 完成条件与确认方式：<可检查的条件；是否需评审，实际确认人>
- 进展：尚未开始
- 成果与验证证据：暂无
- 阻塞与下一位行动人：无
- 更新日期：<实际日期>
```

普通进展更新对应任务。更换负责人、重新打开或取消任务时保留原因、日期和已有成果；历史编号不复用。

## 可选的任务内评审样式（不是真实评审）

需要轻量评审时，将以下小节放到相应任务下；不要求为每位评审者另建任务、交接或报告。使用条件和写入责任见根项目运行协议第 4.1 节。

```text
### 本轮评审：<受评成果 ID、版本、轮次>
- 受评正文：<路径>
- 安排来源与日期：<用户或有权项目经理的明确安排>
- 参与成员、各自范围与完成条件：<member-id 及对应要求>
- 本轮进度与下一位行动人：<依据实际意见记录，不代写结论>

#### <member-id> 的意见
- 日期、评审依据与未覆盖项：<实际信息>
- 结论：<pass / revise / blocked；未提交时写尚未评审>
- 问题：<本轮唯一编号、位置、依据与影响、建议、原稿责任人；无问题则写无>
- 作者处理回复：<作者填写，注明问题编号与修订版本>
- 复核：<实际复核人、问题编号、版本、结果及证据；未复核则写待复核>
```

评审者只更新自己的意见与复核，作者填写处理回复；本轮全部意见提交后才修改受评正文。后续轮次保留前轮记录，不覆盖旧结论。
