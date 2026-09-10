# T-018 可复核验证摘要

- 固定 vcpkg baseline：`9e593bb18ea69cc5095e012465dcd675a822ed0d`（未改）。
- FFmpeg：`8.1.2#3`，ABI `0b20ea8b04628351d2acf4b27f5dfbe6ff52836ee735fd5b338c6848ae840253`。
- feature：`core,avcodec,avdevice,avfilter,avformat,ffmpeg,ffprobe,swresample,swscale,version3`；`default-features=false`，无 GPL/nonfree。
- configuration SHA-256：`e952f157587581ba14fe1c26e12f358da5c48b779fb3d3493a56ec936c761259`；运行时报告 LGPL version 3 or later。
- 黄金生成：12 fixtures、30 time vectors，全部为项目合成/固定字节 CC0-1.0；除故意损坏样例 ffprobe exit=1 外均为 0。
- clean Debug CTest：49/49（20.31 s）；clean Release CTest：49/49（17.81 s，首次 discovery 遇主机策略无输出后原二进制重试通过）；clean CI CTest：49/49（19.74 s）。媒体专项 16/16 原生运行。
- 长素材：20 秒、500 帧；16×16 BGRA 单缓冲峰值 1024 bytes，累计 512000 bytes。
- 运行时：Debug/Release 安装各包含 7 个 FFmpeg DLL；逐文件核验分别与 vcpkg `debug/bin`、`bin` 同哈希，且两种配置的对应 DLL 均不同。Release 逐文件证据见 `runtime-dlls.sha256.csv`。
- 环境说明：Qt smoke 使用 T-013 既有 WDAC 显式回退；媒体测试不使用该回退。安装后应用 smoke 在本机策略下超时，未据此宣称发布安装通过。
- 完整本机日志：被忽略的 `out/evidence/T-018/`；可提交的逐帧 ffprobe 与媒体 hash：`tests/golden/media/generated/actual-hashes-and-probe-v1.json`。
- 范围：T-019 未启动；未实现 UI、CV、DSP 或发布编码器。
