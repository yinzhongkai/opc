# T-029 产品视频获取与冻结工具

`acquire_product_evaluation_media.py` 从当前 A-031 的 40 行来源表读取唯一输入，先核对批准时间窗，
再通过 PyAV 解码为静音、最大 720p 的 MPEG-4/Matroska 内部测试代理。工具保留实际解码帧间隔，
将 clip 内第一帧归一为 `timeNs=0`，并在 manifest 中冻结媒体、probe、完整帧时间序列 SHA-256。
新获取的代理还记录所选平台流在批准窗口内的源时间戳摘要，用于核对 VFR 和 PTS 保持；复用代理
则明确标为未重新解码源流，不把代理推断伪装为新的源流 probe。

依赖仅安装到被 Git 忽略的 `out/`，不是产品运行时依赖：

```powershell
Set-Location projects/space-rhythm/workspace
python -m pip install --target out/tools/t029-py -r tests/evaluation/video/requirements-t029.txt
$env:PYTHONPATH = "out/tools/t029-py"
```

只审计来源元数据和批准时间窗：

```powershell
python tests/evaluation/video/acquire_product_evaluation_media.py `
  --metadata-only `
  --manifest-out ../evidence/T-029/source-availability-audit-v1.json
```

获取代理并冻结证据：

```powershell
python tests/evaluation/video/acquire_product_evaluation_media.py `
  --output-dir out/evaluation/T-029/media `
  --manifest-out ../evidence/T-029/product-dataset-manifest-v1.json
```

任一来源失效或时间窗不足时，工具仍保存其他 clip 的证据，但返回非零；不得据此静默缩短时间窗。
临时签名媒体 URL 不写入 manifest，原始/代理视频不得加入 Git。A-031 来源版本变化时应输出新版本 manifest；
`--reuse-existing` 只用于同一来源版本下复核已存在代理，且仍会重新计算 probe 和 hash。
需要只重试指定项时可重复传入 `--clip-id SR-BILI-...`；生成完整新版 manifest 时，可重复传入
`--previous-manifest`，仅在 clipId、BVID 和媒体 SHA-256 全部一致时继承先前保存的源流时间戳摘要。
完整获取返回码 `0` 表示来源和实际配额没有已知失败，`1` 表示获取/探测失败，`2` 表示获取完整但
实际配额失败；语义 slice 在真人裁决前仍会明确保持 `not-evaluated`，不会因声明标签自动通过。

## USER-01 人工验收

A-031 0.5 / D-014 的本地三阶段界面、逐帧 PTS 会话生成器和正式操作步骤见
[`user01-app/README.md`](user01-app/README.md)。人工参考、隐藏随机化盲评和人工修正必须使用不同
会话，且不得在前置 hash 未冻结时解锁下一阶段。
