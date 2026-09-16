# T-042 迁移后个人未签名交付验证

- 项目：space-rhythm
- 成果 ID：A-038
- 负责人：release-engineer-windows-01
- 关联任务：T-042
- 版本：0.1
- 更新日期：2026-09-16
- 状态：draft
- 适用范围：依据 D-012/D-013/D-015/D-016，从 `projects/space-rhythm/workspace/` 重建确定性 Windows x64 `unsigned-engineering` 包，并在当前 `TIGER`、`VerifiedAndReputablePolicyState=0` 下验证个人交付链和新工程根隔离；不验证或宣称 SAC/WDAC、其他 Windows、产品效果、公开分发或生产发布。
- 来源及输入版本：[A-033 0.3](A-033-windows-unsigned-deployment-and-transaction-pipeline.md)、[A-035 0.1](A-035-windows-personal-unsigned-delivery-validation.md)、[A-036 0.1](A-036-product-workspace-root-migration.md)、[A-037 0.1](A-037-t041-post-migration-independent-regression.md)；T-040/T-041 completed；受控包与验证器提交 `24dd28e7c80ea85cb53558592d705f5dbb1c9645`。
- 批准依据：D-012/D-013 只确认个人未签名范围和当前主机 SAC-off 条件；D-015 延期产品效果评价；D-016 固定唯一产品工程根。本成果不是兼容或发布批准。
- 证据：[T-042 验证摘要](../evidence/T-042/verification-summary.md)与[机器可读证据索引](../evidence/T-042/tiger-workspace-delivery-20260916.json)。
- 版本记录：2026-09-16，0.1，完成新 workspace 双次确定性重建、路径隔离及当前 TIGER 完整个人交付链验证。

## 1. 完成结论

T-042 的完成条件全部满足。精确包 `space-rhythm-0.1.0-dev-t042-unsigned.zip` 从新 workspace 连续两次重建得到相同 SHA-256，并在当前 `TIGER` 实际通过：

1. bundle/payload、供应链清单与个人范围元数据校验；
2. install、拒绝含未登记文件的 repair、repair、rollback、installed validate、uninstall；
3. 无参数、非 offscreen 的正常集成 GUI 首次启动；
4. 安装后的 App/Worker smoke 和回滚后 App smoke；
5. Release 核心工作流；
6. 卸载清除安装根并保留显式根外用户数据；
7. CMake、构建、vcpkg、组包、证据和包清单的新 workspace 路径隔离。

结论为 `pass(personal-unsigned,current-TIGER,SAC-off,post-migration-workspace)`。包仍为 `candidateEligible=false`，不是签名包、发布候选或生产发布。

## 2. 冻结交付物与确定性

| 项 | 冻结值 |
|---|---|
| 包版本/类型 | `0.1.0-dev-t042` / `unsigned-engineering` |
| 范围/候选资格 | `personal-unsigned` / `candidateEligible=false` |
| 源提交 | `24dd28e7c80ea85cb53558592d705f5dbb1c9645` |
| ZIP | `projects/space-rhythm/workspace/out/release/T-042/unsigned/space-rhythm-0.1.0-dev-t042-unsigned.zip` |
| 大小 / SHA-256 | `44,740,452` 字节 / `52E0BED1C7342995591D5F33335EF059BB6204BDCA4386E98F2A1A5E133C56B4` |
| App / Worker SHA-256 | `2B72E6E18777477D2BDF35C5C972A869AC1A5BBC3A1C4B5B8BBD07BAB68182F4` / `068097EC6CE611DB2A402BF417EE8D1B0580F9864B757C03C9408C33DB907A3F` |
| PE / Authenticode 观察 | 81；78 个 `NotSigned`，3 个 Microsoft 文件 `Valid` |

相同源提交、版本、工具链和 OutputRoot 连续两次生成的大小、ZIP SHA-256 以及组包摘要 SHA-256 完全一致。生成时 `sourceWorktreeClean=false` 仅因仓库根既存未跟踪 `package/` 和 `scripts/__pycache__/`；二者位于产品 workspace 外，不属于 CMake、部署或运行时输入，并保持未修改、未提交。

## 3. 新 workspace 隔离

- `productSourceRoot` 与 Release cache 的 `CMAKE_HOME_DIRECTORY` 均为 `projects/space-rhythm/workspace/`。
- 构建、vcpkg installed、发布包、测试和证据均位于该目录的 `out/`。
- 仓库根旧 `src/tests/tooling/docs/cmake`、`CMakeLists.txt`、`CMakePresets.json` 和 vcpkg 入口均不存在；没有观察到旧工程根产品依赖。
- 相对 T-041 受测提交 `616307cfc50cb448915358bf1d2a5e6836346eba`，`src/`、产品 CMake、核心测试 CMake 和 vcpkg 定义的差异为空。安装 App/Worker 哈希与本轮 Release 构建完全一致。
- 输入清单中的 `repositoryRoot` 只用于 Git 提交和路径关系审计，不是产品 source/build/output 根。

## 4. 当前 TIGER 执行结果

验证窗口为 `2026-09-16T04:01:26.6010225Z` 至 `2026-09-16T04:03:11.8959332Z`。正常 GUI 首启 3000 ms 后仍存活并观察到 QML root/event loop，随后由 harness 有界终止；`controlledExitCode=-1` 是清理结果。App、Worker 与回滚后 App smoke 均 exit 0 并输出精确 marker。

完整包事务和独立手工链均通过。卸载后安装根不存在，外部用户数据哨兵存在，审计为 `installed=false/lastAction=uninstall/backupPath=""`。核心函数 `realImportAnalysisEditPreviewSaveAndExportPath` 为 3 passed、0 failed、0 skipped、906 ms；覆盖导入合成视频、分析、编辑、预览、保存、testOnly NUT 导出、音频探测/解码及 worker 断连/重连。T-041 另提供三套 preset 各 166/166 的补充工程证据。

SAC 前后均为 0；Code Integrity Operational 和 AppLocker EXE and DLL 查询成功，匹配 0 条。因为策略未开启，成功运行和 0 条事件都不能证明 SAC/WDAC 兼容；结构化证据固定 `sacWdacCompatibilityValidated=false`。

## 5. 未覆盖边界

- `productEffectEvaluation=not-evaluated(deferred-to-personal-use-feedback)`；自然度、真实 VFR、正式产品性能同样延期未评估。
- 最低 Windows、干净系统/多主机、正式 H.264/AAC/容器、VC Runtime 分发和真实设备/GPU 未评估。
- 不使用签名凭据，不修改策略或白名单，不批准公开/第三方交付或生产发布。
- T-038 的失败诊断继续由 A-035 保留；本轮 T-042 最终验证没有发生需保留的新失败尝试。

T-042 已满足负责人自查完成条件。后续个人使用反馈若发现问题，应新建缺陷或任务，不得把本证据扩写成产品效果验收或兼容性证明。
