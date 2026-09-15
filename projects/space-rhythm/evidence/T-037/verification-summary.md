# T-037 个人未签名流水线验证摘要

- 执行日期：2026-09-15
- 执行成员：`release-engineer-windows-01`
- 成果：[A-033 0.2](../../artifacts/A-033-windows-unsigned-deployment-and-transaction-pipeline.md)
- 源提交：`02c65ce4b596675d102ed3c82459528b60f63297`
- 适用决定：D-012、D-013
- 结论：T-037 在“用户本人、自有 Windows、个人未签名工程包”范围内完成。当前 `TIGER` 在 SAC 已关闭的前提下，最新受控包通过 App/Worker smoke 及完整事务验证；本结果不证明、也不宣称兼容 SAC/WDAC。

## 最新受控包

| 项 | 结果 |
|---|---|
| 包名 | `space-rhythm-0.1.0-dev-unsigned.zip` |
| ZIP 大小 / SHA-256 | `44,725,623` 字节 / `CD94BC9CABF1B0AD29062EE39DD14DEBCBF2AAEB6B777D69036874221D8C634C` |
| 输入状态 | 由已提交源 `02c65ce4...` 生成；`sourceWorktreeClean=false` 仅记录既有未跟踪 `package/`、`scripts/__pycache__/`，这些文件不属于构建或运行时输入 |
| 范围元数据 | `build-inputs.json` schema 3；`deliveryScope.mode=personal-unsigned`；禁止公开分发和第三方交付；`sacWdacCompatibilityClaim=none` |
| `windeployqt` | 固定 Qt 6.11.2 工具及其 SHA-256；dry-run 来源门禁与实际部署均成功 |
| 运行闭包 | 81 个 PE、103,338,584 字节；全部 PE32+ x64；递归导入无未解析项 |
| 签名状态 | 78 个 `NotSigned`、3 个 Microsoft 文件 `Valid`；未访问或使用签名凭据 |
| 供应链材料 | runtime/payload/bundle hash、schema 3 build inputs、SPDX 2.3、上游 SPDX、许可证/notices、Qt 替换说明、known limitations 和无凭据 signing request 均包含在包内 |
| 可重复归档 | 相同已提交输入连续两次生成，ZIP 大小与 SHA-256 完全一致 |
| 发布资格 | `unsigned-engineering`、`candidateEligible=false`；仅用于 D-012/D-013 指定范围 |

App SHA-256 为 `7391BE3B859E41A2567AC03842C13D57C9FE7D9314F15C3EEAC0D64AB3EBC082`；Worker SHA-256 为 `5C12BB3F390544314BDC3B633CEE13E463FF728C959663FC66695916EFF15260`。

## TIGER App/Worker smoke

- 主机：`TIGER`
- 注册表系统信息：`Windows 10 Home China`，`DisplayVersion=25H2`，`Build=26200.9457`，`AMD64`
- 验证窗口：`2026-09-15T07:19:05.0834419Z` 至 `2026-09-15T07:19:37.6414752Z`
- 前置与结束状态：`VerifiedAndReputablePolicyState=0`
- 命令：

  ```powershell
  ./tests/release/Test-UnsignedPackage.ps1 `
    -BundleRoot ./out/release/T-037/unsigned/space-rhythm-0.1.0-dev-unsigned `
    -RunSmoke
  ```

| 进程 | 退出码 | 精确标记 |
|---|---:|---|
| App | 0 | `SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64` |
| Worker | 0 | `SPACE_RHYTHM_WORKER_SMOKE_OK Qt=6.11.2 arch=x64` |

在上述时间窗内，以 `space-rhythm|Qt6|T-037-unsigned-transaction` 过滤 Code Integrity Operational 和 AppLocker EXE and DLL，相关事件数为 0。由于本次主机的 SAC 状态为 0，这个“无事件”结果只描述当前执行环境，不能外推为 SAC/WDAC 开启时可运行。

## 安装事务与清理结果

同一次正式验证依次通过：

1. bundle manifest 校验；
2. install；
3. App smoke；
4. Worker smoke；
5. 拒绝包含未登记文件的 repair；
6. repair；
7. rollback；
8. installed payload 再校验；
9. uninstall；
10. 根外用户数据保留。

结束时安装根不存在，外部数据哨兵仍存在；审计状态为 `installed=false`、`lastAction=uninstall`、`backupPath` 为空。

## 边界与历史证据

- 历史 SAC/WDAC 开启期间的 Qt DLL 拒绝证据继续保留；D-013 改变的是个人验证主机条件，不是证明阻断已经被签名或兼容性方案解决。
- H-015 已取消，H-016 已关闭；代码签名、GUI 安装器、受信任发布者与公开渠道不是 T-037 的完成门禁。
- 不修改或绕过策略，不改白名单/ACL，不使用 `-AllowWdacFallback`。
- T-022、T-038、最低 Windows 版本、正式媒体格式/H.264/AAC 和产品默认资产仍按各自任务处理，不改变 T-037 的个人未签名范围结论。
- 完整生成物位于被忽略的 `out/release/T-037/`；本次可审计机器证据见 [tiger-sac-off-smoke-20260915.json](tiger-sac-off-smoke-20260915.json)。

## 仓库级复核

```powershell
python ./scripts/validate_framework.py
python -m unittest discover -s scripts -p 'test_*.py'
git diff --check
```

结果：框架校验覆盖 17 个岗位、24 份知识、1 个实际项目、1 套模板和 690 处本地链接；单元测试 37/37 通过；Git 空白检查通过。
