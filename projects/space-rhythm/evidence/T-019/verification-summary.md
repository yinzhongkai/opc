# T-019 验证摘要

- 执行日期：2026-09-12
- 执行成员：multimedia-engineer-ffmpeg-01
- 成果：A-026 0.1
- 工具链：Windows x64，MSVC 19.44.35228，CMake 3.31.6-msvc6，Ninja 1.12.1，Qt 6.11.2，动态 CRT
- 依赖：固定 vcpkg baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`；既有 `media;audio-analysis` feature；FFmpeg 8.1.2（vcpkg 8.1.2#3）；未改 default/GPL/nonfree 配置

## 构建与测试结果

| preset | 构建 | `ctest -L t019` | 结果 |
|---|---:|---:|---|
| `windows-msvc-x64-debug` | pass | 9/9 pass | 实际执行完成 |
| `ci-windows-msvc-x64` | pass | 0/9 run；9 个进程均返回 `0xC0E90002` | host Code Integrity/WDAC blocked；无 fallback |
| `windows-msvc-x64-release` | pass | 0/9 pass；9 个进程均返回 `0xC0E90002` | 与 `T021-ENV-001` 同类 host policy blocked；无 fallback |

构建入口使用 `tooling/windows/Invoke-ProjectBuild.ps1 -Stage Build -UseExistingDependencies`，分别指定三个既有 preset；测试入口为对应 build tree 的：

```text
ctest --test-dir <preset-build-dir> --output-on-failure -L t019
```

原始日志位于忽略的本机目录：

- `out/evidence/T-019/debug-clean-configure-final/`
- `out/evidence/T-019/debug-clean-build-final-2/`
- `out/evidence/T-019/debug-clean-t019-final/ctest.log`
- `out/evidence/T-019/ci-build-final-3/`
- `out/evidence/T-019/ci-t019-final-3/ctest.log`
- `out/evidence/T-019/release-build-final-3/`
- `out/evidence/T-019/release-t019-final-3/ctest.log`

Debug 的 9 项测试名称及覆盖内容记录在 A-026 0.1 第 6 节。测试格式产物由 FFmpeg 库 API 写出后，通过公开 `MediaSource::open` 实际复核 video/audio 流；本机 `ffprobe.exe` 本轮同样被 `0xC0E90002` 阻止，未伪造 CLI 证据。

另执行 Debug `ctest -L media` 回归：T-019 的 9 项再次全部通过；既有 `media.golden_generate` 在启动 FFmpeg CLI 时受同一主机策略阻断，导致依赖该 fixture 的 26 项未运行。该次非 T-019 失败原样保存在 `out/evidence/T-019/debug-media-regression-final/ctest.log`，未替换 T-018 已有 27/27 三 preset 证据，也未写成媒体全套通过。

## SHA-256

| 对象 | SHA-256 |
|---|---|
| Debug `space_rhythm_playback_export_tests.exe` | `075bd079f3a2be96a3523e596044fbb4e0eb475d5671c0829223e3ff3959dea3` |
| CI/RelWithDebInfo `space_rhythm_playback_export_tests.exe` | `b0ad6404a8b2d6e691dff5d1f8a595e19c03e1aa17d53fb52cc45487f5208212` |
| Release `space_rhythm_playback_export_tests.exe` | `2bf319180c7175d6251bbac99cd4b318956b6cfa35630ab317e9c9a5571762f7` |
| Debug 测试 NUT（2×2 RGBA，2 帧，48 kHz stereo f32_le，3,840 frames） | `31b3aead0a927df3abf5b7884ec4f17c637986b306ab4729b55356e7b14891e8` |

## 判定

T-019 的要求矩阵已由 Debug 实际运行覆盖，三个配置均通过编译和链接。CI/Release 的未运行项如实登记为主机策略阻断，不归因于 T-019 断言失败，也不构成发布或 G0～G4 通过结论。H.264 与发布容器继续未确认；唯一编码 descriptor 明确为 `testOnly`。

提交前 `git diff --check` 无内容错误；框架只读校验通过（17 个岗位、24 份知识、1 个实际项目、1 套模板、595 处本地链接）。
