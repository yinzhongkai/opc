# T021-DEFECT-001 颜色范围规范化修复证据

- 执行日期：2026-09-10
- 执行人：multimedia-engineer-ffmpeg-01
- 缺陷：FFmpeg `AVCOL_RANGE_MPEG` 的原始名称 `tv` 泄露到公开 `ColorDescription.range`，违反 A-014 的 `limited` 规范值。
- 修复：媒体适配层使用单一封闭映射；MPEG/TV → `limited`，JPEG/PC → `full`，UNSPECIFIED、未来及非法值 → `unknown`。探测 codec parameters、解码帧事实和 full-range BGRA 输出均复用该映射，公开 DTO 不再输出 `tv`/`pc`。
- 测试边界：没有修改 `tests/contract/media_public_contract_test.cpp` 的独立 `limited` oracle；没有处理 WDAC，没有运行 Release，没有启动 T-019，没有读取或修改 `package/`。

## 回归结果

| 范围 | preset | 结果 | 原始证据目录 |
|---|---|---|---|
| `ctest -L media` | Debug | 22/22 pass，含 17 项媒体单测、3 项公开媒体契约、生成与审计 | 控制台执行；同项完整记录包含在下行 Debug headless 日志 |
| T-021 headless | Debug | 62/62 pass，6.11 s CTest | `out/evidence/T-018/T021-DEFECT-001/debug-final/` |
| T-021 headless | CI/RelWithDebInfo | 首轮 61 pass、1 `BAD_COMMAND`；颜色相关全部 pass | `out/evidence/T-018/T021-DEFECT-001/ci-final/` |
| T-021 headless | CI/RelWithDebInfo | 未修改二进制完整重试 62/62 pass，3.86 s CTest | `out/evidence/T-018/T021-DEFECT-001/ci-final-retry/` |

CI 首轮唯一异常是 `MediaResource.LongMaterialKeepsWorkingBuffersBounded` 在并行启动时报告 `Process not started`，没有执行到产品断言。未修改代码、测试、回退或主机策略；同一构建的完整重试通过。Debug/CI 均沿用 T-021 既有、仅作用于 Qt/QML smoke 的显式受管主机回退，本修复未改变该行为。

## 可复核哈希

| 文件 | SHA-256 |
|---|---|
| `src/media/ffmpeg_color_range.hpp` | `f068fd4e63de1cc7f2bc8b8e866e767dd06797f770eb2d39a23706171688eadd` |
| `src/media/ffmpeg_media.cpp` | `d3b3461c9c61ac7e2794824d3013abb0a5a8da55d62c3f6b6ee6a675a2768eed` |
| `tests/unit/media_golden_test.cpp` | `e3a044ee7b2b38206a1a9491b031c44448e461e4d18cd45c2c5e330699d25d87` |
| Debug `environment.json` | `21d8050b406f149f6c82566676cb43fd4f6bb4b7accda6884b2521be5ad35e5b` |
| Debug `ctest-junit.xml` | `1ea5a06e5a3aff96fbed59487101c14999fe7fbfc13fe18248bbfa45113288b8` |
| Debug `ctest.log` | `d2deba106deaf7b55b8b4f6e0c80858c0b1faa3dcdf5f134b0cd1d49acfdb851` |
| Debug `result.json` | `bc700589afebbb2bd10464e7a991afebfd9afd48cdd553451eabd097deb927ec` |
| CI 首轮 `result.json` | `80d99ef71535ad02cb97010f1dd4e27090e309a3a961ac8bf858628b6df889e9` |
| CI 重试 `environment.json` | `665aab418d3e6ce6d5fd7508c3bb5174d8145c11f97905e6daa278bea65788b3` |
| CI 重试 `ctest-junit.xml` | `6fc687a264ef38d6a174102d89ce13bf1166d77459b0617fc50159e724203986` |
| CI 重试 `ctest.log` | `a473229f0ed0cad7bcaef7f59e087b34a4409fbcae7320f3a72bc72c1bdb1b50` |
| CI 重试 `result.json` | `6be646d327a18809e9e6a07d5967694e529b154dee7ebc3208e605ab4fba32dd` |

原始运行资料位于 Git 忽略的 `out/evidence/T-018/T021-DEFECT-001/`；以上哈希把可提交结论绑定到实际环境、JUnit、CTest 日志和结果文件。
