# T-042 新 workspace 个人未签名交付验证摘要

- 执行日期：2026-09-16
- 执行成员：`release-engineer-windows-01`
- 成果：[A-038 0.1](../../artifacts/A-038-t042-post-migration-personal-unsigned-delivery.md)
- 包/验证器提交：`24dd28e7c80ea85cb53558592d705f5dbb1c9645`
- 适用决定：D-012、D-013、D-015、D-016
- 结论：`pass(personal-unsigned,current-TIGER,SAC-off,post-migration-workspace)`；不构成 SAC/WDAC、其他 Windows、产品效果、公开分发或生产发布结论。

## 冻结包与双构建

| 项 | 结果 |
|---|---|
| ZIP | `workspace/out/release/T-042/unsigned/space-rhythm-0.1.0-dev-t042-unsigned.zip` |
| 大小 / SHA-256 | `44,740,452` 字节 / `52E0BED1C7342995591D5F33335EF059BB6204BDCA4386E98F2A1A5E133C56B4` |
| 两轮归档 | 相同提交、版本、工具链和 OutputRoot；SHA-256 均为上述值 |
| 类型 / 范围 | `unsigned-engineering` / `personal-unsigned` / `candidateEligible=false` |
| App / Worker SHA-256 | `2B72E6E18777477D2BDF35C5C972A869AC1A5BBC3A1C4B5B8BBD07BAB68182F4` / `068097EC6CE611DB2A402BF417EE8D1B0580F9864B757C03C9408C33DB907A3F` |
| PE / Authenticode | 81；78 `NotSigned`，3 Microsoft `Valid` |

两轮 `unsigned-package-summary.json` 也逐字节相同。生成时记录的 dirty 项只有仓库根既有未跟踪 `package/` 与 `scripts/__pycache__/`；均位于产品 workspace 外且不是构建/运行时输入，因此包继续非候选。

## 复现命令

```powershell
cd projects/space-rhythm/workspace
./tooling/windows/Invoke-UnsignedRelease.ps1 `
  -Version 0.1.0-dev-t042 `
  -OutputRoot ./out/release/T-042/unsigned `
  -EvidenceRoot ./out/evidence/T-042/packaging-first `
  -AllowDirtySource

# 使用相同 OutputRoot，改用 packaging-second EvidenceRoot 重复一次。
./tests/release/Test-PersonalDelivery.ps1 `
  -TaskId T-042 `
  -BundleRoot ./out/release/T-042/unsigned/space-rhythm-0.1.0-dev-t042-unsigned `
  -ZipPath ./out/release/T-042/unsigned/space-rhythm-0.1.0-dev-t042-unsigned.zip `
  -ReleaseBuildRoot ./out/build/windows-msvc-x64-release `
  -TestRoot ./out/tests/T-042-personal-delivery-final `
  -EvidenceRoot ./out/evidence/T-042/final
```

## 路径与执行结果

- 产品 source、output、Release build、vcpkg installed、包和证据路径全部位于 `projects/space-rhythm/workspace/`；Release `CMAKE_HOME_DIRECTORY` 与 package `productSourceRoot` 一致。
- 仓库根旧产品目录/入口不存在；相对 T-041 受测提交的产品与核心工作流源差异为空；没有观察到旧工程根产品依赖。
- 当前主机为 `TIGER`/AMD64，注册表 `Windows 10 Home China`、25H2、build `26200.9457`。验证窗口为 `2026-09-16T04:01:26.6010225Z` 至 `2026-09-16T04:03:11.8959332Z`。
- 完整事务和独立手工链均通过：install、正常 GUI 首启、App/Worker smoke、核心工作流、repair、rollback、installed validate、回滚后 App smoke、uninstall、保留根外数据。
- 正常 GUI 首启无参数且非 offscreen，3000 ms 后仍存活；App、Worker、回滚后 App smoke 均 exit 0。核心工作流为 3 passed、0 failed、0 skipped、906 ms。
- SAC 前后均为 0；两个策略日志查询成功且匹配 0 条。`sacWdacCompatibilityValidated=false`，不宣称 SAC/WDAC 兼容。

## 原始证据哈希

原始文件位于被忽略的 `workspace/out/evidence/T-042/`，生成物位于 `workspace/out/release/T-042/`。机器可读索引见 [tiger-workspace-delivery-20260916.json](tiger-workspace-delivery-20260916.json)。

| 文件 | SHA-256 |
|---|---|
| `packaging-first/unsigned-package-summary.json` | `F8F71822AF3A1EB1A9C4EF81953BAC129BFA6CD0DB3F991A91CB04A54794C895` |
| `packaging-second/unsigned-package-summary.json` | `F8F71822AF3A1EB1A9C4EF81953BAC129BFA6CD0DB3F991A91CB04A54794C895` |
| `final/result.json` | `14DB8FADECB16CE9B6FEC54A9990DCD218692113E2BE3A4C82CCDF4EF0B86C51` |
| `final/core-workflow.qt.txt` | `C0ED85DD49665AC5255A61E99E4E44FEEFEB9489F9DDBA4407D8FB54DC5D6E61` |
| `final/core-workflow.wrapper.log` | `9887A4DD0473BCB19102449A2CFF71FCC7A4B64E7CB2F8959A5742523114664F` |
| `final/first-launch.stdout.log` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` |
| `final/first-launch.stderr.log` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` |
| `payload/manifest/build-inputs.json` | `8ADF077DF1E32C92FCFF852952FEE38CFB1521E2F364856EA6EAF313EC3B661D` |
| `payload/manifest/runtime-files.sha256.csv` | `D34E1A8D25B5627B018E4B75458C4A1694FC9461748F9A4081C40830F724A70B` |
| `payload/manifest/payload-files.sha256.csv` | `85F8D5760A02689C9CF2F1EB5AB27952575DAB556E3228534090F29C5FB73880` |
| `manifest/bundle-files.sha256.csv` | `D921E29D7DB9BF7ED15AC192B0F9CBC1D12577B4F9A0EAAAFF92F4C0A6CA68E5` |
| `payload/sbom/space-rhythm.spdx.json` | `9B7BACDBC5BEF497E268C07B398D25F7E8A7B7ED9582A43A2A5303D7F0F1F06C` |

## 未覆盖边界

`productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`；自然度、真实 VFR 和正式产品性能同样延期未评估。最低 Windows、干净系统/多主机、正式 H.264/AAC/容器、VC Runtime 分发和真实设备/GPU未评估。不使用签名凭据，不修改策略，不批准公开/第三方交付或生产发布。

## 仓库级自查

框架校验通过，覆盖 17 个岗位、24 份知识、1 个实际项目、1 套模板和 769 处本地链接；单元测试 37/37 通过；T-042 边界 JSON、两个发布 PowerShell 脚本语法和 `git diff --check` 均通过。
