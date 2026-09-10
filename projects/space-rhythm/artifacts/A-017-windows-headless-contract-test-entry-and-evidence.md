# Windows headless 契约测试入口与证据

- 项目：space-rhythm
- 成果 ID：A-017
- 负责人：tester-cpp-qt-01
- 关联任务：T-021
- 版本：0.1
- 更新日期：2026-09-10
- 状态：draft
- 适用范围：依据 A-016 0.1 建立 T-021 Windows x64 headless 统一入口、CTest 标签、失败诊断归档、独立公开契约测试及合法媒体黄金样例审计；不修改被测业务实现，不执行 T-022，不给未确认的性能、效果、硬件、Windows 或发布门槛通过结论。
- 来源及输入版本：[A-012 0.1](A-012-core-domain-contract-0x.md)、[A-014 0.2](A-014-media-time-buffer-and-golden-contract.md)、[A-015 0.1](A-015-ffmpeg-media-pipeline.md)、[A-016 0.1](A-016-cpp-qt-test-strategy-and-traceability.md)；D-003 confirmed；T-013、T-014、T-017、T-020 completed；T-015、T-016、T-018 实际实现；用户于 2026-09-10 明确启动 T-021。
- 批准依据：尚无。T-021 的测试基础设施交付完成不等于被测产品、G0～G4 或发布批准；当前实际结果含 fail 与 blocked。
- 测试入口版本：`headlessTestEntryVersion = 0.1.0`
- 版本记录：2026-09-10，0.1，首次实现统一入口、标签与诊断证据，新增 11 项独立公开契约测试和 12 样例审计，并记录 Debug/Release/CI 实际结果。

## 1. 实现结果

统一入口为 `tooling/windows/Invoke-HeadlessTests.ps1`。它接受 `windows-msvc-x64-debug`、`windows-msvc-x64-release`、`ci-windows-msvc-x64`，调用固定工具链完成 configure/build，再以 `ctest -L t021 --output-on-failure --no-tests=error --output-junit` 执行。运行环境固定 `QT_QPA_PLATFORM=offscreen`、软件 RHI、UTC、稳定 seed，并把 TEMP/TMP 指向本次证据目录的 `work/`。

每次运行保存 `environment.json`、`ctest-discovery.json`、`ctest-labels.log`、`ctest.log`、`ctest-junit.xml`、`result.json`、构建日志以及存在时的 `LastTest.log`/`LastTestsFailed.log`。配置/构建/发现/零测试/进程启动/断言错误均返回非零；失败和阻断目录默认保留。原始证据置于忽略的 `out/evidence/T-021/<run-id>/`，可提交摘要见 [T-021 验证摘要](../evidence/T-021/verification-summary.md)。

GoogleTest 采用 CMake 静态测试注册，以便即使受管主机阻止某一测试可执行文件，CTest 仍会运行并归档其他测试，而不是在 PRE_TEST 发现阶段丢失整个批次。Qt Test、Qt Quick Test、媒体 golden 生成/验证、应用/worker smoke 和构建保护同属 `t021`。标签体系使用 `unit`、`contract`、`golden`、`integration`、`qt`、`qml`、`compatibility`、`headless`、`smoke`、`gate-g0`～`gate-g4`；门槛标签只是追踪输入，不表示门槛已通过。

## 2. 独立公开契约覆盖

| 独立测试 | 公开 API 与 oracle | 覆盖 | 三 preset 结果 |
|---|---|---|---|
| `T021CoreTimeContract` | `scale_ticks`、`checked_add/subtract`；A-012 固定字面值，不实现第二套算法 | 正负 half-tie、NTSC 分数、非法 time base、Int64 溢出 | pass |
| `T021CoreTransactionContract` | `Timeline::submit/snapshot`；A-012 状态与错误码 | 锁定保护、用户原子解锁移动、幂等重放、stale revision、失败事务不部分提交 | pass |
| `T021CoreRevisionContract` | 加载公开 snapshot 后提交 | `UINT64_MAX` 修订耗尽不回绕 | pass |
| `T021IpcContract` | `JobCoordinator`、protocol frame API | queued/running/cancelling/cancelled、乱序拒绝、终态重放、帧往返、版本拒绝 | pass |
| `T021SchemaContract` | `ProjectStore::deserialize` | schema 1→2 迁移、未来 schema 拒绝 | pass |
| `T021StorageContract` | `ProjectStore::save/load` + 公开 `SaveFault` | disk full/partial/before commit 均保留最后成功提交 | pass |
| `T021CacheContract` | `RebuildableCache::put/get` | 内容损坏返回 `cache_corrupt`、要求重建并移除坏条目 | pass |
| `T021MediaContract` | `map_presentation_time`、`MediaSource` | 时间溢出、CFR PTS、VFR 非帧序号重建、旋转/SAR/BT.709/range | 2 pass，1 fail |

测试只调用公开头文件中的 API，预期来自 A-012/A-014 的稳定字面 oracle 和状态不变量。测试没有读取实现私有状态，没有复制时间换算、JSON、缓存格式或 FFmpeg metadata 解析逻辑。存储故障使用公开注入点；临时文件由独立 `QTemporaryDir` 创建且统一入口将系统临时根限定到本次 `work/`。

## 3. 黄金样例逐项核对

完整生成命令保存在 `fixtures-v1.json` 的 `canonicalRecipe`；下表列出配方身份、实际字节身份和期望域。所有样例许可证均为 `CC0-1.0`，均为项目合成或项目自有固定字节，不读取、不复制 `package/`。T-021 没有批准像素、PCM、主观效果或性能容差；表中整数时间、错误码、枚举、字符串和有理数均按精确相等比较，容差为 0。

| fixture | 生成方式 | 配方 SHA-256 | 媒体 SHA-256 | 期望与容差依据 |
|---|---|---|---|---|
| GM-CFR-001 | FFmpeg lavfi testsrc2 + 静音 | `9a93e92f347e5929263fb72d54206d8f47ba36a0403c8a22da2576ae0a99577c` | `85cd9761dc428617431f53dcc124df766d3813ef6c1671d2b6a2b3c0c7f1297c` | 流类型、5 个 CFR PTS；A-014 精确 ns，0 |
| GM-VFR-001 | lavfi testsrc2 + 显式 PTS | `7453fc778b309b98fac0f596b1f5cb14508e22e59bd211ff90137408e3af9d15` | `cfa0bd714598b7034382c7ca2359ea72183e83f16ccb7dfc30f37a1f72dc43b2` | 流类型、5 个 VFR PTS；A-014 显式 PTS，0 |
| GM-ROT-SAR-001 | lavfi testsrc2 + SAR/BT.709 + display rotation remux | `5e0467cd9ed47cb2249ef09c838a57c8fc978f06d441335a5d1d25298e109fd3` | `20f26854c2489fb40803c8b4073db4990b614bff9c6f59be605d96baa9b412eb` | geometry/SAR/DAR/rotation/color + 2 PTS；A-014 归一化 metadata，0 |
| GM-MULTI-001 | testsrc2 + 两路静音 | `39f68e0b16ff87885c4f4eb64b8d18bd9dfe0ad742ef2da0b8e872728ab6ba3b` | `9ef2430d551f2af3ff22d7af16334fc64736e097afcbec63512dede8f870b549` | 三流、默认/显式选择、2 PTS；A-014 确定性选择，0 |
| GM-AUDIO-44100-001 | 项目生成数字静音 | `21a78fffe12d8fae31cde268be751814362b0962331ae20050245dc5370bd84b` | `f971a9aaa8626894132d1d3d7139e0f06b01f3beb894f680f5033eccfbbcd969` | 44.1 kHz/4410 samples/mono/s16 + 5 时间值；有理采样时间，0 |
| GM-AUDIO-48000-001 | 项目生成数字静音 | `6837e8223eb7178c9569b087ee7a5f26a2102808aaa22e5fe05b2e0d9f200ef7` | `639dad0ac2923f5fe9e9ccfb99aa9b3084048e2e53317d1903e6e899a4f6296a` | 48 kHz/4800 samples/mono/s16 + 5 时间值；有理采样时间，0 |
| GM-CORRUPT-001 | 项目自有截断 EBML 固定字节 | `a8bfb71271547ffd8ba34a1e642c76e219617bdbaf0b0b95db089a727c1b2495` | `ae9dd0845c07d6a876f88162a29130453a3ac4e3999a71296416f926211dffb5` | `media/corrupt_media/probe`；稳定错误契约，0 |
| GM-MISSING-VIDEO-001 | lavfi 静音 | `28fd80c48b8b12675c98ece2827c4000fa14453dee165c8bf626c966ca1077aa` | `a5bace0a9919f277b41bf534f3be64fad14eea07351a179e179265935436b9ec` | 缺必需视频流错误；A-014 选流契约，0 |
| GM-MISSING-AUDIO-001 | lavfi testsrc2 | `57fdc675174150023ccf378665e0676646d0ed020bfa3118921ac241a4ed53e8` | `a609dd9b3a41d8897c2050ef77f87d542115fd0d5bae83611478b2a5d20679b4` | 缺必需音频流错误；A-014 选流契约，0 |
| GM-NEG-START-001 | testsrc2 + 显式负 PTS | `f05d1d88be4457662b284bab2259283a23e3dd6f58f98e96410198f693ba39af` | `ed410c2d20dd29280cb8c4e808b5b1c9c38f9e907b5024745edd2fbfd73bc0fb` | origin -80 ms + 4 映射时间；A-014 presentation origin，0 |
| GM-LONG-001 | testsrc2 + 数字静音 20 秒 | `268d33f6726fb53bcfaf74de3d2db66647c9a0d50034e102dbe99d31fa9796e4` | `a22465b40aeeb91c6771f4346b66ccd1076236d23676d938dd0421d2d4612ffe` | 流类型 + 起止时间；仅正确性精确比较，性能阈值未确认 |
| GM-DYNAMIC-001 | 两段不同尺寸 testsrc2 拼接 | `11b72d19cf3d4f95ab51e005c8dd2d6ba105cf0c253f0349e72acc7b90aedc37` | `34f411f2fef1c71708ad0dc93e53c0839818aa5855bf7e72b18886ba175230e5` | 16×16→32×16 format epoch；A-014 格式变更契约，0 |

审计由独立 `contract.golden.media_audit` 测试执行，逐项重新计算配方文本 SHA-256 和生成文件 SHA-256，验证 12 个唯一 fixture、30 个唯一时间向量、许可证、来源、期望字段与容差依据。它不重新实现媒体时间算法；时间语义由公开 API 契约测试以固定预期验证。

## 4. 实际运行与问题

| preset | 总数 | 结果 | 原始 run-id |
|---|---:|---|---|
| Debug | 61 | 60 pass，1 fail | `20260910T062656Z-b9ae42a53a67-windows-msvc-x64-debug-01` |
| Release | 61 | 34 pass，1 fail，26 blocked | `20260910T062715Z-b9ae42a53a67-windows-msvc-x64-release-01` |
| CI/RelWithDebInfo | 61 | 60 pass，1 fail | `20260910T062734Z-b9ae42a53a67-ci-windows-msvc-x64-01` |

稳定产品失败 `T021-DEFECT-001`：`GM-ROT-SAR-001` 的公开 `ColorDescription.range` 实际为 `tv`，A-014 0.2 期望 `limited`。三套配置一致失败，不修改测试 oracle，也不修改被测媒体实现。

环境阻断 `T021-ENV-001`：Release 的既有 `space_rhythm_core_tests.exe` 被 WDAC 阻止启动，26 项记为 `blocked`；同 preset 的独立契约可执行文件及媒体、Qt/QML、应用/worker 测试可运行。发现阶段的两次首次/同条件重试证据均保留，未用回退替代 GoogleTest。

## 5. 状态与边界

T-021 的交付物已经完成：入口、标签、诊断归档、独立契约覆盖、黄金审计和三 preset 实际证据均存在。因此任务状态可记为 completed；这不把测试结果写成全绿。被测结果当前为 `fail`，Release 部分范围为 `blocked`。

所有性能耗时仅是执行诊断，没有确认的性能阈值和基准硬件矩阵，故性能、效果、硬件/Windows 兼容及 G0～G4 均保持 `not-evaluated`。T-022 没有启动、没有建立或执行其端到端/故障/性能批次。
