# 工作记录与核验证据

本目录保存 book-yocto 的修订辅助脚本、逐章真实环境核验记录和原始日志。这些文件用于任务追溯与复核，不是书稿仓库内容，也不因迁入 `artifacts/` 自动成为已批准成果。

文件于 2026-09-28 从原 `projects/book-yocto/workspace/` 迁回 OPC，迁移依据见 [D-012](../../DECISIONS.md)。旧任务中的 `workspace/<file>` 对应本目录同名文件；原始版本与完整修改历史仍可从 OPC 归档标签 `archive/2026-09-28/book-yocto` 追溯。

原始日志保持原内容，不为消除告警而改写。记录中的 SHA-256 以 Git 保存的 LF 内容为准；Windows 检出时若启用了 CRLF 转换，直接对工作区文件计算的字节校验值会不同。
