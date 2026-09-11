# T-025 验证摘要

- 日期：2026-09-11
- 执行人：ui-engineer-qt-quick-01
- 成果：A-024 0.1；`SpaceRhythm::UiBridge`；启动页、工作区、致命错误页；11 个 QML 组件；3 个 headless 测试入口
- 环境：Windows x64；MSVC 19.44；Qt 6.11.2 shared；CMake 3.31.6-msvc6；Ninja；Debug；手写 C++ 目标使用项目 `/W4 /WX` 策略。

## 最终验证

1. 定向构建：在 VS 2022 x64 developer environment 中执行 `cmake --build out/build/windows-msvc-x64-debug --config Debug --target space_rhythm_app space_rhythm_ui_bridge_tests space_rhythm_ui_qml_tests`，通过。
2. QML 静态检查：构建目标 `space_rhythm_app_qmllint`，通过，无诊断。
3. C++ bridge 直接结果：`UiBridgeTest` 最终 9 passed、0 failed，包含实际 `SceneGraphRenderItem` 挂载检查。
4. QML 直接结果：`qmltestrunner` 最终 6 passed、0 failed，覆盖工作区组件、关键对象名/可访问性、loading/running/cancelling/failed/readOnly、恢复和致命页。
5. CTest headless：`ctest --test-dir out/build/windows-msvc-x64-debug -C Debug -L '^t025$' --output-on-failure`，最终 3/3 通过：`qt.ui_bridge`、`qml.ui_shell`、`app.qml_smoke`。
6. 环境变量：三项 CTest 均使用 `QT_QPA_PLATFORM=offscreen`；当前企业签名策略主机使用 `QT_QUICK_CONTROLS_STYLE=Basic`，避免加载被策略拒绝的 `Qt6QuickEffects.dll`，没有使用测试成功回退替代新增测试断言。
7. 框架一致性：`scripts/validate_framework.py` 通过，检查 17 个岗位、24 份知识、1 个实际项目、1 套项目模板和 582 处本地链接。

开发中第一次 C++ bridge 运行发现只读状态未允许导出当前快照，与 A-023 状态矩阵不符；已统一修正 ViewModel 能力判断和 mock 命令语义，最终 9/9 通过。第一次应用 smoke 还发现默认启动页不存在预览宿主；入口已改为随 Loader/状态变化延迟挂载，并在工作区 smoke 中验证实际 Scene Graph item。两项均属于本任务内已解决缺陷，不是保留阻塞。

## 完成条件追溯

| T-025 完成条件 | 验证证据 |
|---|---|
| 启动/工作区/致命错误应用外壳 | `Main.qml`、三个页面；`qml.ui_shell`、`app.qml_smoke` |
| 素材/预览/时间线/检查器/任务抽屉拆分 | 五个独立 QML 组件及 `WorkspacePage.qml`；QML 组件断言 |
| 版本化 QObject/QAbstractItemModel | UI bridge 0.1.0/schema 1；一个 QObject、三个列表模型；descriptor 测试 |
| QML 不持有 64 位权威值 | 元对象负向断言；仅字符串属性/角色；QML 源码扫描 |
| mock/真实 service 同接口和语义 | `WorkspaceService`、共享 command/snapshot/error DTO；构造注入；mock 异步状态测试 |
| 嵌入 SceneGraphRenderItem | C++ attachment；unit test 和应用 smoke；QML 无几何输入/循环 |
| 七类页面状态 | mock scenario、状态条、恢复页、只读门控、致命页；Qt/QML 双层测试 |
| objectName 与可访问性 | A-024 第 7 节清单；QML 稳定名称和 Accessible 断言 |
| Qt Quick/C++/headless 测试入口 | 三项 CTest，最终 3/3 pass |

## 保留项

视觉风格、产品文案、默认音色、发布导出格式、产品默认模板参数以及可访问性/性能门槛均保持待确认。T-026 保持 `todo` 且未启动；本验证不宣称完成时间线编辑、六阶段端到端业务逻辑、媒体/DSP、导出事务或离屏渲染。H-006 只追加 T-025 阶段证据并保持 `accepted`。
