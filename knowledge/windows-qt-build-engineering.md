---
id: windows-qt-build-engineering
name: Windows x64 与 Qt 构建工程
status: active
---

# Windows x64 与 Qt 构建工程

用于把已确认的 Windows x64、Qt、CMake/Ninja 和依赖约束转成可复现的本地构建与 CI。具体 Qt 版本、编译器路线、Windows SDK、许可证路径、依赖版本和发布方式以项目决定为准；知识配置本身不安装工具、不确认候选方案，也不授权签名或发布。

## 固定工具链身份

构建前先形成工具链元组，至少记录目标架构、编译器及完整版本、Windows SDK、C/C++ 标准、运行库模型、构建类型、Qt 源码版本或提交、CMake/Ninja 版本和依赖清单。实际探测环境并保存输出，不仅依赖 PATH 或机器上的默认版本。

把源码、工具链、配置和产物的变化分开。需要比较 MSVC 与 MinGW-w64 等路线时，依据项目目标评估支持范围、第三方二进制可用性、调试与 CI 条件；待确认项保持待确认，不能由一次本地构建结果代替项目决定。

## Qt 源码构建

从可追溯的官方源码版本构建，保持源码、构建和安装目录分离。Windows 路径保持较短并避免空格或特殊字符；在配置前验证受支持编译器及必需工具。Qt 当前官方说明将 CMake、Ninja 和 Python 3 列为 Windows 源码构建工具，并给出配置、构建与安装流程，实际版本要求须按项目选定的 Qt 版本重新核对：[Qt for Windows—Building from Source](https://doc.qt.io/qt-6/windows-building.html)。

保存完整 `configure.bat` 参数、环境与配置摘要，明确 Qt 模块、功能、第三方库来源、shared/static、Debug/Release、安装前缀和许可证影响。使用 shadow build 维护独立构建树；面向应用交付的 Qt SDK 不使用仅适合开发 Qt 自身的 `-developer-build`。配置方式与选项以相应版本的 [Qt Configure Options](https://doc.qt.io/qt-6/configure-options.html) 为准。

Qt 安装产物作为版本化 SDK 管理，生成文件清单与哈希，记录配置摘要、编译器和依赖来源。不同工具链、架构、运行库或配置使用不同安装前缀，不能靠覆盖同一目录切换变体。

## CMake、Ninja 与依赖

用受版本控制的 `CMakePresets.json` 表达团队共享的 configure、build、test、install 或 package 入口；个人路径和本机覆盖放在不提交的 `CMakeUserPresets.json`。预设的能力与 schema 按项目最低 CMake 版本选择，依据见 [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)。生成器、配置类型和输出目录要显式，命令行与 CI 调用同一组预设。

Ninja 负责执行生成的构建图，不把复杂环境判断隐藏在临时命令中；并行度、失败日志和增量行为需要在本地及 CI 核对。命令与行为以 [Ninja Manual](https://ninja-build.org/manual.html) 为准。

依赖清单记录来源、版本或提交、哈希、许可证、构建选项及二进制变体。对同进程原生库至少核对架构、编译器工具链、运行库、Debug/Release、静态/动态链接、导出约定和相关编译宏；缺少兼容证据时从固定源码以同一工具链重建，不把“链接成功”当作 ABI 已兼容。稳定跨边界接口应遵循已确认架构契约，不能由构建层擅自改写。

## CI 与可复现性

CI 从干净的 Windows x64 环境执行配置、构建、测试、安装和必要的打包检查。缓存键应覆盖工具链元组、源码锁定信息与构建选项；缓存只用于提速，命中缓存不能成为正确性证据。定期执行无缓存构建，避免开发机或旧 SDK 隐式补齐依赖。

Qt SDK 构建和应用构建可以分层，但下游必须验证 SDK 清单和哈希。CI 产物记录提交、预设、工具版本、依赖清单、配置摘要、测试结果和符号文件；失败时保存足以定位的日志，不在日志或产物中泄露签名密钥、访问令牌和其他凭据。

至少验证应用与测试目标能够从声明的环境完成全新构建。针对发布候选再检查运行时 DLL、Qt 插件、第三方许可证、安装布局和目标 Windows 环境启动情况；这些检查不代替功能与产品验收。

## 交付与协作

构建交接应给出一条可复现入口、前置环境、预设名称、预期产物、实际验证结果和已知限制。问题报告同时记录失败阶段、完整命令、工具版本、首个有效错误及环境差异，避免只交付截图或最后一行报错。

涉及架构接口、依赖版本或发布约束的变化，分别交由对应负责人确认；构建工程师负责说明可构建性、ABI 和 CI 影响，不用构建配置替代产品、架构、测试或许可证决定。
