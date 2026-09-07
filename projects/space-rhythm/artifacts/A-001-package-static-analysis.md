# package 安装包静态分析报告

- 项目：space-rhythm
- 成果 ID：A-001
- 负责人：developer-reverse-01
- 关联任务：T-001
- 版本：0.1
- 更新日期：2026-09-07
- 状态：draft
- 适用范围：`package/` 中截至 2026-09-07 可见的 5 个 ZIP；覆盖无执行的包结构、二进制格式、依赖、安装目标、关键功能线索与安全基线。
- 来源及输入版本：本会话用户于 2026-09-07 直接提供并授权分析；样本版本以本报告 SHA-256 为准。
- 批准依据：尚无；本任务仅要求负责人自查形成候选报告。
- 版本记录：2026-09-07，0.1，完成首轮静态分析。

## 1. 结论摘要

这批文件由两个产品系列和一个 Blender 扩展组成：

- `osci-render` Premium 2.8.10.3：Linux 独立程序、VST3 效果器、VST3 乐器插件，以及 Windows Inno Setup 安装器。
- `sosci` 1.3.5.3：Linux 独立程序和 VST3 效果器，以及 Windows Inno Setup 安装器。
- Blender 扩展：把 Grease Pencil 几何序列化为 GPLA 2.0.0，通过本机 TCP 发送给 osci-render，或保存为 `.gpla` 文件。

已观察到的总体特征：

1. 主程序和插件由 C++/JUCE 构建；二进制字符串显示 JUCE 8.0.12，VST 元数据显示 VST SDK 3.8.0。Linux 构建为 x86-64 ELF，Windows 实际载荷为 x86-64 PE；Windows 外层安装器自身是 32 位 Inno Setup 6.7.0 引导程序。
2. 程序面向音频/MIDI、示波器可视化、项目/预设管理、视频录制与音频转视频。FFmpeg 不随当前包提供；程序包含从固定 GitHub Release 基址下载 FFmpeg、启动兼容性检查以及选择 H.264/H.265/VP9 编码器的代码线索。
3. Windows 安装器包含主程序、VST3 和 `SpoutLibrary.dll`。安装目标中既有应用目录，也有 `{sys}\SpoutLibrary.dll`；后者可能形成系统级共享 DLL 的版本冲突和管理员权限风险。
4. Windows 安装器及抽出的 EXE/DLL/VST3 均未发现 Authenticode 签名。现有文件可用本报告哈希进行完整性比对，但仅凭这些包不能验证发布者身份。
5. 未发现能够仅凭静态证据确认的恶意持久化、凭据窃取或进程注入逻辑；同时，静态检查不能证明样本安全。网络下载、安装写入、插件宿主行为和运行时许可逻辑仍需在隔离环境动态验证。

## 2. 分析边界与方法

本轮没有运行安装器、主程序、插件或 Blender 扩展，也没有向外部服务上传样本。执行的操作仅包括：

- 计算文件大小和 SHA-256；
- 读取 ZIP 中央目录、Unix 权限和文本清单，并解压到独立临时工作目录；
- 解析 PE/ELF 头、节区、导入、动态依赖、导出、符号、安全标志、版本资源和调试路径；
- 解析 Inno Setup 6.7.0 的压缩元数据及载荷，不执行其 Pascal Script 或安装逻辑；
- 提取 ASCII/UTF-16 字符串并记录关键文件偏移；
- 阅读 Blender 插件 Python 源码和 manifest。

Windows 载荷的拆取结果已通过 `MZ`/PE 格式、Inno 元数据声明的文件长度、SHA-256 和版本资源交叉核对。行为能力主要来自导入、符号、字符串和源码；除 Blender 源码可直接确认的逻辑外，不把“存在字符串/导入”单独等同于“运行时必然执行”。

## 3. 外层样本清单

| ZIP | 字节数 | SHA-256 | 包内概况 |
|---|---:|---|---|
| `osci-render-blender-plugin.zip` | 3,388 | `6F8C41849100A9AB52C04471A55C45BC747357674E7107798F0AA3714C9BFED0` | 1 个 Python 文件、1 个 manifest；解压后 12,109 字节 |
| `osci-render-premium-linux.zip` | 70,582,030 | `1EE9E2B6E304D713D08EE33CB91DFBD82C0FAD7F39AF12719678E6997FC55E87` | 独立程序、VST3 效果器、VST3 乐器插件及两份模块元数据；解压后 112,828,459 字节 |
| `osci-render-premium-windows.zip` | 58,414,448 | `40F630D9FFFA316D00628BBCC81C444662D94DE996ABFFC590801629112A0F9C` | 单个 Inno Setup 安装器；解压后 58,958,822 字节 |
| `sosci-linux.zip` | 45,418,115 | `71BA1D863EF4E690BDD7884DF694D24EFFA3DEEED008DAF22B69B65FBCEF45C5` | 独立程序、VST3 效果器及模块元数据；解压后 67,133,312 字节 |
| `sosci-windows.zip` | 39,843,544 | `80621D7CFDF94F85A77AA14DAF7A45DA5827E66B341FD23F45460681879F265B` | 单个 Inno Setup 安装器；解压后 40,396,196 字节 |

所有 Linux ELF/VST3 文件在 ZIP 中保留了 `0755` 执行权限，JSON 和 Blender 文本文件为 `0644`。分发包中没有单独的 README、校验清单或完整安装说明；Blender manifest 是唯一直接声明许可证的文件（GPL-3.0-or-later）。

## 4. Linux 二进制

### 4.1 组成、格式与哈希

| 文件 | 类型 | 字节数 | SHA-256 |
|---|---|---:|---|
| `osci-render` | ELF64 x86-64 PIE 独立程序，动态链接，未剥离符号 | 37,137,880 | `13AA747AD42F0B4C997E065E57CB78662788628814D4473FC3CEB64A3047D5AF` |
| `osci-render.vst3/.../osci-render.so` | ELF64 x86-64 共享对象，未剥离符号 | 37,844,200 | `91BE835E1022EB4C24269584EF5F2EDBC00994EB2281454948158D67EE6CB370` |
| `osci-render-instrument.vst3/.../osci-render-instrument.so` | ELF64 x86-64 共享对象，未剥离符号 | 37,844,200 | `FCB7384F5387A4C9AA036E3C81939EDB976AA022187CF5F7199158F9D0705E03` |
| `sosci` | ELF64 x86-64 PIE 独立程序，动态链接，未剥离符号 | 33,240,568 | `66A5A332A28FCD11E3A2DFE8966FFC1CDCB5053283BD82BB4AB5BC0DF427B81F` |
| `sosci.vst3/.../sosci.so` | ELF64 x86-64 共享对象，未剥离符号 | 33,891,720 | `73D20E4A458DB02F9184EF9ADD8954F1C4F84DC441EB03C4CCE152FFA28AC53A` |

两套 VST 元数据分别给出 `osci-render 2.8.10.3` 和 `sosci 1.3.5.3`。`osci-render` 效果器分类为 `Fx`，乐器插件分类为 `Instrument/Synth`，`sosci` 分类为 `Fx`。

### 4.2 直接动态依赖

五个 ELF 的 `DT_NEEDED` 集合一致：

```text
libasound.so.2
libfontconfig.so.1
libfreetype.so.6
libGL.so.1
libcurl.so.4
libstdc++.so.6
libm.so.6
libgcc_s.so.1
libc.so.6
ld-linux-x86-64.so.2
```

由此可直接确认 ALSA 音频/MIDI、字体渲染、OpenGL、HTTP/网络传输及 GNU C/C++ 运行时依赖。没有 RPATH/RUNPATH，动态库将按系统加载器规则解析。

编译标记显示 GCC 13.3.0（Ubuntu 24.04 工具链）。`osci-render` 系列还带有 DynASM 1.5.0 标记。所有 ELF 都保留 `.symtab`；例如独立 `osci-render` 约 60,121 个静态符号，独立 `sosci` 约 45,017 个。这显著提高可调试性和逆向可读性，但也扩大了发布物的实现信息暴露和体积。

### 4.3 加固状态

| 类型 | PIE | 栈不可执行 | RELRO |
|---|---|---|---|
| 两个独立程序 | 是 | 是 | Full RELRO（含 BIND_NOW） |
| 三个 VST3 `.so` | 不适用为可执行 PIE | 是 | Partial RELRO |

这是可接受的基础加固，但插件没有 Full RELRO。入口指令包含 `endbr64`，说明至少部分构建启用了面向 CET/IBT 的编译支持；本轮未在目标运行系统验证 CET 是否实际生效。

## 5. Windows 安装器与载荷

### 5.1 外层安装器

两个 ZIP 中的 EXE 均为 32 位 PE GUI 程序，版本资源明确标记为 Inno Setup：

| 安装器 | 产品版本 | Inno 数据版本 | Setup header 文件偏移 | Payload 文件偏移 | Authenticode |
|---|---|---|---:|---:|---|
| `osci-render-premium.exe` | 2.8.10.3 | 6.7.0 | `0x3712B95` | `0xDA400` | 未签名 |
| `sosci.exe`（Windows ZIP 内层安装器） | 1.3.5.3 | 6.7.0 | `0x255ED94` | `0xDA400` | 未签名 |

安装器引导代码启用 ASLR 和 DEP，但未启用 CFG；大部分文件内容位于高熵压缩 overlay 中。两者使用同一 Inno stub 时间戳（2026-02-11 UTC），这不是产品载荷自身的编译时间。

### 5.2 osci-render Windows 载荷

Inno 元数据包含 5 个逻辑安装目标，指向 4 份唯一数据：

| 目标 | 字节数 | SHA-256 |
|---|---:|---|
| `{code:GetExeInstallDir}\osci-render.exe` | 29,347,328 | `BC3BDF36ED3808D00BDD9637BF3F57EF7E66CB7DC0D1038D527F35CF06F60987` |
| `{code:GetVstInstallDir}\osci-render.vst3` | 28,839,424 | `BD0ADA44E34CE5022BE3B5AB0EAD4D12971A71EBB6B0CF0728914884E03B1FA8` |
| `{code:GetVstInstallDir}\osci-render-instrument.vst3` | 28,839,424 | `8AB1796CD85B41D944BEF88E24CD65723F2EB747400C881344A847C44F47FBD2` |
| `{code:GetExeInstallDir}\SpoutLibrary.dll` | 255,488 | `85905D0072D018611459F3125CF6300A541138C3874A36DCA14189376D220F8F` |
| `{sys}\SpoutLibrary.dll` | 同一份数据 | 同上 |

### 5.3 sosci Windows 载荷

Inno 元数据包含 4 个逻辑安装目标，指向 3 份唯一数据：

| 目标 | 字节数 | SHA-256 |
|---|---:|---|
| `{code:GetExeInstallDir}\sosci.exe` | 26,828,800 | `6EB6FDB6104CFAD5FFF83B4325DB68EE15A0F6FCD351B6E9FC8A62929CB163EA` |
| `{code:GetVstInstallDir}\sosci.vst3` | 26,320,896 | `08ABA0EF4F32D79C4AC0921E176148A4318642EEF98D298F2FD8701FB4C77375` |
| `{code:GetExeInstallDir}\SpoutLibrary.dll` | 255,488 | `85905D0072D018611459F3125CF6300A541138C3874A36DCA14189376D220F8F` |
| `{sys}\SpoutLibrary.dll` | 同一份数据 | 同上 |

实际主程序和 VST3 均为 PE32+ AMD64。版本资源与 Linux VST 元数据一致。它们启用了 ASLR、高熵 VA 和 DEP，但未启用 CFG；安装器、主程序、VST3 和 `SpoutLibrary.dll` 均为 `NotSigned`。

VST3 导出 `GetPluginFactory`/`InitDll`/`ExitDll`，符合 VST3 插件入口。`osci-render.vst3` 和 `sosci.vst3` 直接导入 `SpoutLibrary.dll`，用于 Windows GPU 纹理共享；`SpoutLibrary.dll` 的导出包含 DirectX 11/OpenGL 共享纹理、共享内存、sender/receiver 和注册表辅助接口。

### 5.4 安装行为线索

安装元数据显示：

- 应用默认路径由 `{autopf}`（自动选择 32/64 位 Program Files）派生，VST3 默认路径为 `{cf}\VST3`。
- 可创建开始菜单和可选桌面快捷方式。
- `osci-render` 注册 `.osci` 项目文件关联，`sosci` 注册 `.sosci` 项目文件关联，并把打开命令指向对应主程序。
- 安装界面含“Remove any existing settings (Clean installation)”选项；关联的设置路径分别为 `{userappdata}\osci-render\osci-render.settings` 和 `{userappdata}\sosci\sosci.settings`。静态元数据只证明存在该清理分支，未确认它是否默认选中。
- 文件表包含向应用目录和系统目录各部署一份相同 `SpoutLibrary.dll` 的目标。是否受安装条件控制仍需动态验证；若确实写入系统目录，卸载、升级和其他 Spout 应用之间可能出现所有权或版本冲突。
- Inno header 还包含大量通用依赖下载模板（多个 .NET、VC++、DirectX、SQL Server 等 URL）。这些字符串来自安装器依赖辅助代码，不能据此断言本产品会下载所有这些运行库；必须观察实际安装分支。

## 6. 功能与数据流线索

### 6.1 音频、视频和网络

已观察事实：

- Linux 直接依赖 `libasound`、`libGL`、`libcurl`；Windows 载荷导入 WININET、Winsock、Direct3D/Direct2D/OpenGL、WinMM 等接口。
- 字符串与保留符号包括 `CommonAudioProcessor`、`VisualiserComponent::setRecording`、`OfflineAudioToVideoRendererComponent::renderToFile`、`FFmpegEncoderManager`、音频/MIDI 设置、项目文件、预设和效果链。
- FFmpeg 下载基址为 `https://github.com/eugeneware/ffmpeg-static/releases/download/b6.0/`。相关字符串包含 `ffmpeg-linux-x64`、`ffmpeg-win32-x64`、下载确认、启动失败、兼容性检查、编码器探测和重新下载提示。
- `FFmpegEncoderManager` 符号显示 H.264、H.265、VP9 命令构造、可用编码器查询和硬件编码器探测。

分析推断：缺少本地 FFmpeg 时，相关视频功能会询问用户并从上述固定 Release 下载二进制，然后通过创建子进程进行验证和编码。未在本轮静态字符串中确认下载物的固定 SHA-256 或代码签名校验，因此下载链完整性需单独验证。

许可相关静态线索包括 `isLicensed`、`Premium Feature`、`showPremiumSplashScreen` 及 `Effect::setPremiumOnly/isPremiumOnly`。JUCE 的机器标识实现还包含读取 Linux `board_serial` 的通用路径。当前 URL 集合中未发现明确的明文许可服务器端点，但这不能证明不存在编码、间接或运行时生成的许可通信。本轮不包含许可绕过或补丁分析。

### 6.2 Blender 插件与 GPLA

插件源码直接确认以下流程：

```text
Blender scene/Grease Pencil
  -> GPLA 2.0.0 二进制序列化
  -> Base64 编码 + 换行
  -> TCP localhost:51677（端口可在 51600..51699 调整）
  -> osci-render
```

连接目标硬编码为 `localhost`，默认端口 51677。连接后，插件在 `frame_change_pre` 和 `depsgraph_update_post` 处理器中发送当前场景数据；关闭时发送 `CLOSE\n`。也可以把全部帧保存为 `.gpla`。

GPLA 结构由源码直接给出：

- 文件头：8 字节 `GPLA    `，随后三个 8 字节小端整数版本号（2、0、0）。
- 文件信息：`FILE    `、帧数 `fCount  `、帧率 `fRate   `，字段以 `DONE    ` 结束。
- 每帧：`FRAME   `、焦距 `focalLen`（double）、对象集合、每个对象的 4×4 double 矩阵、stroke 数组、每条 stroke 的 8 字节顶点数和连续的 XYZ double 三元组。
- 文件尾：`END GPLA`。

源码检查发现四项明确问题：

1. 版本不一致：`bl_info` 为 1.1.0，而 manifest 为 1.0.3。
2. Blender 最低版本不一致：`bl_info` 声明 3.1.2，manifest 声明 4.2.0。
3. 注册时属性写到 `bpy.types.Scene.oscirenderPort`，注销时却删除 `bpy.types.Object.oscirenderPort`，会导致注销异常或残留 Scene 属性。
4. 序列化直接访问 `bpy.data.cameras[0]`、`scene.camera`、`current_frame()/active_frame`，缺少空对象/空帧检查；场景没有相机、图层没有当前帧时可能抛异常。`depsgraph_update_post` 中同步重建并发送整个帧，也可能在复杂场景造成高频 CPU/内存与本机 IPC 开销。

## 7. 风险与优先级

| 级别 | 发现 | 影响 | 建议 |
|---|---|---|---|
| 高 | Windows 安装器和全部载荷未签名 | 无法仅凭文件确认发布者；中间人替换或镜像污染难以及时被系统阻止 | 从可信发布渠道取得官方哈希；发布方应签名安装器和最终 PE/VST3 |
| 高 | 运行时可能下载并执行 FFmpeg，未确认固定哈希/签名验证 | 下载源、重定向、账户或 Release 资产受损时形成供应链风险 | 在隔离网络抓包验证最终 URL、TLS 和校验流程；建议固定哈希或验证签名 |
| 中 | 安装目标含 `{sys}\SpoutLibrary.dll` | 可能覆盖系统共享副本、影响其他程序或导致卸载残留 | 验证条件与覆盖策略；优先使用应用本地 DLL，并明确版本/所有权 |
| 中 | VST3 ELF 仅 Partial RELRO，Windows PE 未启用 CFG | 降低部分内存破坏利用缓解能力 | Linux 插件启用 `-z now`；Windows 构建评估 `/guard:cf` |
| 中 | Blender 注销属性类型错误及空对象边界缺失 | 插件禁用失败、场景触发异常或卡顿 | 修复 `Object`/`Scene` 错配，增加相机/帧检查和发送节流 |
| 低 | Linux 发布物未剥离大量符号 | 增加包体与实现信息暴露 | 对正式发行物剥离符号并单独保留调试符号包 |
| 信息 | ZIP 缺少 README、独立许可文件和校验清单 | 用户难以确认安装方式、依赖与来源 | 随包提供说明、许可证、版本和 SHA-256 清单 |

风险等级反映本轮静态证据和部署影响，不等同于恶意软件判定。

## 8. 待动态验证

建议在可回滚 Windows/Linux 虚拟机中分别验证，且不要使用真实凭据或生产网络：

1. Windows：记录安装前后文件、注册表、服务、计划任务、启动项和子进程差异，确认 `SpoutLibrary.dll` 的实际写入条件、覆盖/卸载策略，以及“Clean installation”默认状态。
2. 网络：在受控代理或抓包环境触发需要 FFmpeg 的功能，记录 DNS、最终 URL、重定向、下载文件哈希、校验逻辑和执行参数。
3. 主程序：验证配置目录、崩溃恢复、项目/预设格式、音视频输入输出、FFmpeg 命令参数和错误处理。
4. VST3：在专用宿主中扫描和加载，观察插件启动时文件、网络、进程、音频设备和 Spout 访问，避免用生产 DAW 首次加载。
5. Blender：在测试工程中覆盖无相机、空 Grease Pencil 图层、大场景、高频 depsgraph 更新、断线重连和注销流程。
6. 许可：仅验证合法授权场景下的状态来源、离线行为和数据外发；不开展绕过分析。

在完成上述动态验证前，本报告不对“安装后无额外下载”“不会写入系统目录”“无网络通信”或“样本安全”作确定承诺。

## 9. 复现要点

在 PowerShell 中可复核外层哈希与包清单：

```powershell
Get-ChildItem package -File | Get-FileHash -Algorithm SHA256
tar.exe -tf package\osci-render-premium-linux.zip
tar.exe -tf package\sosci-linux.zip
Get-AuthenticodeSignature <抽出的-Windows-EXE或DLL>
```

在 Linux 分析环境中对解包后的 ELF 执行：

```bash
file osci-render sosci
readelf -h -l -d -s osci-render
readelf -h -l -d -s osci-render.so
strings -a -t x osci-render | rg -i 'ffmpeg|isLicensed|Premium Feature|Blender Port|GPLA|download'
```

关键字符串证据示例：

| 样本 | 文件偏移 | 字符串 |
|---|---:|---|
| Linux `osci-render` | `0xAB1FEE` | `isLicensed` |
| Linux `osci-render` | `0xAB3200` | `GPLA    ` |
| Linux `osci-render` | `0xAC1B7D` | `Blender Port:` |
| Linux `osci-render` | `0xACAB18` | FFmpeg GitHub Release 基址 |
| Linux `sosci` | `0x882801` | `isLicensed` |
| Linux `sosci` | `0x883C7E` | `FFmpeg executable not found.` |
| Linux `sosci` | `0x891850` | `cat /sys/class/dmi/id/board_serial` |
| Windows `osci-render.exe` 载荷 | `0x76D670` | FFmpeg GitHub Release 基址 |
| Windows `osci-render.exe` 载荷 | `0x7734B8` | `GPLA    ` |
| Windows `osci-render.exe` 载荷 | `0x81DD18` | `isLicensed` |
| Windows `sosci.exe` 载荷 | `0x5ED180` | FFmpeg GitHub Release 基址 |
| Windows `sosci.exe` 载荷 | `0x93CC40` | `isLicensed` |

以上偏移是对应解包文件中的文件偏移，不是 RVA/VA；更换样本版本后不得沿用。
