# T-016 可复核验证摘要

- 项目：space-rhythm
- 任务：T-016
- 负责人：core-systems-engineer-cpp-01
- 验证日期：2026-09-09
- 输入基线：T-013、T-014、T-015 completed，D-003 confirmed，A-012 0.1、A-013 0.1
- 实现边界：系统/可靠性骨架与 mock Worker；未实现媒体、UI、CV、DSP 任务，未修改 `package/`

## 1. 交付内容

- 新增 `SpaceRhythm::System` C++20 静态库。公共头文件只依赖标准库和 `SpaceRhythm::Core`；纯 `JobCoordinator` 不包含 Qt 类型，`QLocalSocket`/JSON/文件系统细节收敛在私有实现，`MessageTransport` 保持传输层可替换。
- 作业状态机覆盖 `queued/running/cancelling/succeeded/failed/cancelled`；请求 ID 绑定完整请求指纹，等价重复提交/取消可重放，不等价复用返回冲突；Worker 更新使用严格递增序号，区分等价重复、同序号冲突、缺口/回退；超时、断连和过期 `timelineRevision` 均转成带诊断 ID 的结构化终态错误。
- IPC 采用 4 字节大端长度前缀、schema 1/protocol 1 的 UTF-8 JSON envelope 和同用户 `QLocalServer::UserAccessOption`；握手协商成功前拒绝作业。帧上限 64 KiB，payload 仅允许小型命令/进度元数据，显式拒绝帧、PCM、采样、像素和 blob 类内联字段；大数据只通过含长度和 SHA-256 的 file/cache `DataReference` 传递。
- `space-rhythm-worker --mock-worker <server>` 提供 echo、delay、timeout、crash 四种确定性作业；覆盖握手、进度、带外结果引用、幂等冲突、重复取消和进程异常退出，不持有 UI `QObject`。
- 新增项目 schema 2（JSON Schema draft 2020-12）、schema 1→2 迁移和完整核心时间线/轨道/事件/扩展 round-trip；`TimeNs`、`TimelineRevision` 和文件长度使用十进制字符串，避免 JSON double 精度损失。
- 项目主文件、自动保存、会话标记和缓存均使用 `QSaveFile` 临时文件原子提交，禁止 direct-write fallback；恢复 API 返回 primary/autosave 来源、来源修改时间和被忽略的损坏副本诊断。
- 素材重定位要求文件大小与 SHA-256 同时匹配，不凭同名文件误绑；缓存键覆盖素材指纹、算法版本、参数摘要和工具版本，缓存文件自带 magic/格式版本/长度/内容 SHA-256，缺失或损坏均返回“需要重建”，损坏项可安全移除，配额按最旧项裁剪。

## 2. 故障与边界覆盖

| 场景 | 自动化断言 |
|---|---|
| 等价重复请求、不同内容复用请求 ID | 等价请求返回 `replayed`；冲突返回 `request_conflict`，mock Worker 同样拒绝冲突复用 |
| 重复、同序号冲突、缺号及进度回退 | 精确重复更新幂等；其他情况分别返回 `message_conflict` / `out_of_order_message`，已提交状态不变 |
| 取消 | queued 直接 cancelled；running→cancelling→cancelled；重复取消及 mock 重复取消保持幂等 |
| 超时 | 协调器按 deadline 失败为 `request_timeout`；本地 IPC 读超时返回同类诊断 |
| Worker 崩溃 | mock 进程以非零码退出；IPC 报 `worker_crashed`，协调器将所有非终态作业置为 failed |
| 版本不兼容/未握手 | 不兼容范围返回 `protocol_version_mismatch`；成功握手后才接受作业 |
| JSON 内联大数据 | `frameData`、`pcmBytes` 等被 `forbidden_inline_data` 拒绝；超限元数据被 `message_too_large` 拒绝；带外引用正常 round-trip |
| 过期修订 | 结果基准或当前 timeline revision 不一致时作业失败为 `stale_revision`，不发布结果引用 |
| schema 迁移/未来版本 | schema 1 可迁移并报告来源版本；未知未来 schema 以 compatibility 错误拒绝 |
| 磁盘不足、部分写入、提交前失败 | 故障注入返回 `disk_full` / `partial_write`，最近成功保存的项目和缓存内容保持不变 |
| 异常恢复 | 未清洁会话优先有效 autosave；损坏 autosave 回退 primary 并保留诊断；损坏 primary 可恢复有效 autosave |
| 素材丢失/误匹配 | 递归搜索只接受大小与 SHA-256 同时匹配；同名错误文件返回 `fingerprint_mismatch` |
| 缓存损坏/缺失/配额 | 校验失败和 miss 均要求重建；损坏条目删除；配额裁剪不接触项目持久数据 |

GoogleTest 新增 16 个系统测试：2 个协议编解码、3 个进程级 mock Worker、4 个作业状态机、4 个项目存储/恢复、1 个素材重定位、2 个缓存完整性/配额测试，连同既有契约和工程冒烟共进入 32 个 CTest 项。

## 3. Windows x64 Presets/CTest

工具链为 MSVC 19.44.35228.0、CMake 3.31.6-msvc6、Ninja 1.12.1、Qt 6.11.2 shared、动态 CRT，以及固定 vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`。三套配置均使用 `/W4 /WX /permissive- /std:c++20`。

| preset | 最新源码构建 | CTest | 结果 |
|---|---:|---:|---|
| `windows-msvc-x64-debug` | 17/17 增量重编译 | 32/32 | 通过 |
| `windows-msvc-x64-release` | 17/17 增量重编译 | 32/32 | 通过 |
| `ci-windows-msvc-x64` | 17/17 增量重编译 | 32/32 | 通过 |

最终命令入口：

```powershell
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Build -EvidenceRoot ./out/evidence/T-016/windows-msvc-x64-debug
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage Test -AllowWdacFallback -EvidenceRoot ./out/evidence/T-016/windows-msvc-x64-debug
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-release -Stage Build -EvidenceRoot ./out/evidence/T-016/windows-msvc-x64-release
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-release -Stage Test -AllowWdacFallback -EvidenceRoot ./out/evidence/T-016/windows-msvc-x64-release
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset ci-windows-msvc-x64 -Stage Build -EvidenceRoot ./out/evidence/T-016/ci-windows-msvc-x64
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset ci-windows-msvc-x64 -Stage Test -AllowWdacFallback -EvidenceRoot ./out/evidence/T-016/ci-windows-msvc-x64
```

最终原生日志位于被忽略的本地证据目录：

- Debug build/test：`out/evidence/T-016/windows-msvc-x64-debug/native-20260909-204828.log`、`native-20260909-204847.log`
- Release build/test：`out/evidence/T-016/windows-msvc-x64-release/native-20260909-204907.log`、`native-20260909-204922.log`
- CI build/test：`out/evidence/T-016/ci-windows-msvc-x64/native-20260909-204907.log`、`native-20260909-204923.log`

本机 WDAC 对刚链接的可执行文件曾在 GoogleTest discovery 或首次子进程启动时短暂返回“应用程序控制策略已阻止此文件”；最终三套测试均由最新二进制实际执行。`-AllowWdacFallback` 只作用于既有 Qt 冒烟脚本，不替代或跳过 GoogleTest；mock Worker 启动只对该已知瞬态启动失败执行有界重试，协议及断言失败仍直接失败。

## 4. 自查结论

- JSON schema 可由 PowerShell `ConvertFrom-Json` 解析为 title `Space Rhythm project`、schemaVersion const 2。
- `src/system/include` 与 `src/domain` 不包含 Qt/QML、FFmpeg 或 OpenCV include；IPC 适配和领域/作业接口保持单向依赖。
- `git diff --check` 无空白错误；框架自测 37/37 通过，只读配置校验通过 17 个岗位、24 份知识、1 个实际项目、1 套模板和 438 处本地链接。
- 未启动媒体、UI、CV 或 DSP 任务，未修改 `package/`；提交仅包含 core-systems-engineer-cpp-01 的 T-016 相关变更。
