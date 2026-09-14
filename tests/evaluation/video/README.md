# T-029 产品视频获取与冻结工具

`acquire_product_evaluation_media.py` 从 A-031 0.2 的 40 行来源表读取唯一输入，先核对批准时间窗，
再通过 PyAV 解码为静音、最大 720p 的 MPEG-4/Matroska 内部测试代理。工具保留实际解码帧间隔，
将 clip 内第一帧归一为 `timeNs=0`，并在 manifest 中冻结媒体、probe、完整帧时间序列 SHA-256。

依赖仅安装到被 Git 忽略的 `out/`，不是产品运行时依赖：

```powershell
python -m pip install --target out/tools/t029-py -r tests/evaluation/video/requirements-t029.txt
$env:PYTHONPATH = "out/tools/t029-py"
```

只审计来源元数据和批准时间窗：

```powershell
python tests/evaluation/video/acquire_product_evaluation_media.py `
  --metadata-only `
  --manifest-out projects/space-rhythm/evidence/T-029/source-availability-audit-v1.json
```

获取代理并冻结证据：

```powershell
python tests/evaluation/video/acquire_product_evaluation_media.py `
  --output-dir out/evaluation/T-029/media `
  --manifest-out projects/space-rhythm/evidence/T-029/product-dataset-manifest-v1.json
```

任一来源失效或时间窗不足时，工具仍保存其他 clip 的证据，但返回非零；不得据此静默缩短时间窗。
临时签名媒体 URL 不写入 manifest，原始/代理视频不得加入 Git。A-031 来源版本变化时应输出新版本 manifest；
`--reuse-existing` 只用于同一来源版本下复核已存在代理，且仍会重新计算 probe 和 hash。
