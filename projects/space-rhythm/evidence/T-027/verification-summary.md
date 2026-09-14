# T-027 验证摘要

- 日期：2026-09-14
- 负责人：video-algorithm-engineer-cv-01
- 任务：T-027
- 成果：[A-028 0.1](../../artifacts/A-028-video-analysis-contract-sample-metrics.md)
- 结论：T-027 契约成果和机器可读样本规格自查通过；实际 OpenCV 算法、媒体生成和效果/性能测量属于 T-028/T-029，未在本任务中冒充完成。

## 验证结果

| 检查 | 结果 | 证据 |
|---|---|---|
| 样本 manifest schema、版本和必需覆盖 | pass | `GOLDEN_VIDEO_MANIFEST=PASS fixtures=10 coverage=9 contract=0.1.0` |
| 10 项配方规范串 SHA-256 | pass | `Test-GoldenVideoManifest.ps1` 逐项重新计算并比较 |
| 10 项标注规范串 SHA-256、排序和一对一身份 | pass | 同一校验器逐项重新计算；fixture/event/output ID 无重复 |
| 时间、窗口、kind、正/负类和媒体状态约束 | pass | 同一校验器检查非负纳秒、样本范围、允许 kind 及未生成媒体不得伪造 hash |
| JSON 可解析 | pass | PowerShell `ConvertFrom-Json` 返回成功 |
| A-028 本地链接 | pass | `A028_LOCAL_LINKS=PASS count=3` |
| 补丁空白检查 | pass | `git diff --check` 退出码 0；仅报告工作副本 LF/CRLF 提示 |
| 框架 YAML 校验器 | unavailable | 系统 `python` 是 Windows Store 占位符；工作区 Python 可启动，但缺少仓库声明的 PyYAML。未联网改动宿主运行时，也未把 unavailable 写成 pass。 |

## 复核入口

```powershell
& tests/golden/video/Test-GoldenVideoManifest.ps1
Get-Content -Raw tests/golden/video/fixtures-v1.json | ConvertFrom-Json | Out-Null
git diff --check
```

校验器覆盖：`videoAnalysisContractVersion=0.1.0`、core 0.1/schema 1、media
1.0/schema 2、9 个必需场景类别、许可、规范配方/标注 hash、事件边界、正负类冲突和
`specified_not_generated` 状态。实际媒体 SHA-256 与 ffprobe 证据必须由 T-028 固定生成器产生。

## 门禁语义

本次 `pass` 只用于可判定的契约/完整性检查。尚未产生检测效果、性能或“卡点自然”通过
结论；后续数值在产品阈值未确认前只能标为 `measured`，相应 gate 为 `not-evaluated`。
