# T-038 当前 TIGER 个人未签名交付验证摘要

- 执行日期：2026-09-15
- 执行成员：`release-engineer-windows-01`
- 成果：[A-035 0.1](../../artifacts/A-035-windows-personal-unsigned-delivery-validation.md)
- 包/验证器提交：`bd9eb97a3b538550790d340b6117362184ee65f2`
- 适用决定：D-012、D-013、D-015
- 结论：`pass(personal-unsigned,current-TIGER,SAC-off)`；不构成 SAC/WDAC、其他 Windows、产品效果、公开分发或生产发布结论。

## 冻结包

| 项 | 结果 |
|---|---|
| ZIP | `out/release/T-038/unsigned/space-rhythm-0.1.0-dev-t038-unsigned.zip` |
| 大小 / SHA-256 | `44,740,342` 字节 / `2CC3F8B4AC5A6BC4B3EE8CC42C8305094F372A9E16F9DCAF937B59872FF8C3E6` |
| 类型 / 范围 | `unsigned-engineering` / `personal-unsigned` / `candidateEligible=false` |
| App / Worker SHA-256 | `67D01EBF70B6DA0E12C6AB624658D9E1BC1CC3892F6A5BAA43410178B32560A6` / `D87FC84ECA91B506B3D7ADAB515A60008A3338CEA16E35054DBFE4E670C316DB` |
| PE / Authenticode | 81；78 `NotSigned`，3 Microsoft `Valid` |
| 确定性 | 相同 OutputRoot 连续两次生成的大小和 SHA-256 一致 |

包生成时记录的 dirty 项只有既有项目协调文档和未跟踪 `package/`、`scripts/__pycache__/`；它们不属于构建或运行时输入。包因此继续为非候选。

## 复现命令

```powershell
./tooling/windows/Invoke-UnsignedRelease.ps1 `
  -Version 0.1.0-dev-t038 `
  -OutputRoot ./out/release/T-038/unsigned `
  -EvidenceRoot ./out/evidence/T-038/packaging `
  -AllowDirtySource

./tests/release/Test-PersonalDelivery.ps1 `
  -BundleRoot ./out/release/T-038/unsigned/space-rhythm-0.1.0-dev-t038-unsigned `
  -ZipPath ./out/release/T-038/unsigned/space-rhythm-0.1.0-dev-t038-unsigned.zip `
  -ReleaseBuildRoot ./out/build/windows-msvc-x64-release `
  -TestRoot ./out/tests/T-038-personal-delivery-final `
  -EvidenceRoot ./out/evidence/T-038/final
```

## 主机、策略与执行结果

- 主机：`TIGER` / `AMD64`；注册表 `Windows 10 Home China`、25H2、build `26200.9457`。
- 时间窗：`2026-09-15T12:13:23.3939922Z` 至 `2026-09-15T12:14:45.2316574Z`。
- `VerifiedAndReputablePolicyState` 前后均为 0；Code Integrity Operational 与 AppLocker EXE and DLL 均查询成功、匹配 0 条。
- 完整事务和独立手工链均通过：install、正常 GUI 首启、App/Worker smoke、核心工作流、repair、rollback、installed validate、回滚后 App smoke、uninstall、保留根外数据。
- 正常 GUI 首启无参数且非 offscreen；3000 ms 后仍存活，随后由 harness 有界终止。App、Worker、回滚后 App smoke 均 exit 0 并输出精确 marker。
- 卸载后安装根不存在，外部数据哨兵存在；审计为 `installed=false/lastAction=uninstall/backupPath=""`。

SAC 为关闭状态，故 0 条策略事件和成功运行都不能证明兼容 SAC/WDAC；`sacWdacCompatibilityValidated=false`。

## 核心工作流

受控 T-022 Release harness 执行 `realImportAnalysisEditPreviewSaveAndExportPath`：3 passed、0 failed、0 skipped、778 ms。路径覆盖导入合成视频、分析、编辑/锁定/撤销重做、预览/seek、保存、testOnly NUT 导出、导出音频探测/解码和 worker 断连/重连。

`Qt6Test.dll` 只供外部测试 runner 使用且不在交付包中。安装 App/Worker 哈希与当前 Release 构建一致。相对 T-022 提交 `be61c71e9803`，唯一源差异为 `src/worker/main.cpp` 的 smoke stdout 显式刷新；App/核心工作流源不变。A-034 另提供保存项目重开、故障恢复矩阵和三套 preset 各 166/166 的补充证据。

## 保留诊断

| 尝试 | 结果 | 处理 |
|---|---|---|
| 核心 harness 未带测试专用 Qt6Test | `-1073741515` / `0xC0000135` | 未向包添加测试 DLL；改在受控测试环境运行 |
| 外部测试 EXE 强制解析安装布局 | 196 秒后终止；有界复现 `-1073740791` | 定位非交付 EXE 的 qoffscreen plugin discovery 冲突，包 App 自身正常启动 |
| 修复前 Worker 重定向 smoke | 三次 exit 0、stdout/stderr 空 | 显式 `fflush(stdout)`，重建包；最终无重试通过 |

## 原始证据哈希

原始文件位于被忽略的 `out/evidence/T-038/final/`，生成物位于 `out/release/T-038/`。机器可读索引见 [tiger-personal-delivery-20260915.json](tiger-personal-delivery-20260915.json)。

| 文件 | SHA-256 |
|---|---|
| `final/result.json` | `75FC824B3A22E01883B13F4F9A1A9E528E4A3A62EFABC59695A7DBCD6B123B21` |
| `final/core-workflow.qt.txt` | `FFEFB0FEC711F66AD6C8B1B21DE9AF3420003BF55CF2625F7DA52D85C986264F` |
| `final/core-workflow.wrapper.log` | `3B4EB109D5551E9731207517F148D8DBDF8BD830155671DD2E7D718B6D631663` |
| `final/first-launch.stdout.log` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` |
| `final/first-launch.stderr.log` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` |
| `payload/manifest/build-inputs.json` | `04C0021DEC0B954E0F581229E026890D589F5E670D72CFB6DF5DB4432A66E096` |
| `payload/manifest/runtime-files.sha256.csv` | `82FDE6B781B1437F148887EE665AF9CD83E3CF78F8EBBEC3ED2C32D3CD85FDDB` |
| `payload/manifest/payload-files.sha256.csv` | `33AB6A1D385F02ED03138055B288B0EEB1C393A10E5F7E7D430CC7CC25B0C64D` |
| `manifest/bundle-files.sha256.csv` | `90760DDCD8417312D0DBCBAEF695AF9A649FB3BFC2B9230A69E2CAA454928C45` |
| `payload/sbom/space-rhythm.spdx.json` | `37DE1E1EB2650EA89A17998934171FB6E6FEAAD07A16F09E20C59D968B9F244A` |

## 未覆盖边界

- `productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`；自然度、真实 VFR、正式产品性能同样延期未评估。
- 最低 Windows、干净系统/多主机、正式 H.264/AAC/容器、VC Runtime 分发和真实设备/GPU 未评估。
- 不使用签名凭据，不改变策略，不批准公开/第三方交付或生产发布。

## 仓库级自查

```powershell
python ./scripts/validate_framework.py
python -m unittest discover -s scripts -p 'test_*.py'
git diff --check
```

结果：框架校验通过，覆盖 17 个岗位、24 份知识、1 个实际项目、1 套模板和 724 处本地链接；单元测试 37/37 通过；Git 空白检查通过。
