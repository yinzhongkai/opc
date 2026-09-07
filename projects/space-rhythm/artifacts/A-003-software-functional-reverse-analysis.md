# 安装包软件功能逆向报告

- 项目：space-rhythm
- 成果 ID：A-003
- 负责人：developer-reverse-01
- 关联任务：T-003
- 版本：0.1
- 更新日期：2026-09-07
- 状态：draft
- 适用范围：`package/` 中由 A-001 0.1 固定哈希的 5 个 ZIP；覆盖 `osci-render`、`sosci`、对应独立程序/VST3 形态和 Blender 插件在当前样本中可静态恢复的功能面。
- 来源及输入版本：本会话用户于 2026-09-07 直接指派；样本身份、SHA-256、平台结构和安全基线沿用 [A-001 0.1](A-001-package-static-analysis.md)。
- 批准依据：尚无；本任务要求负责人自查形成候选报告。
- 版本记录：2026-09-07，0.1，完成第二阶段功能面逆向和产品间差异核对。

## 1. 结论摘要

当前 5 个安装包实际包含 3 个产品族、6 种主要交付形态：

| 产品族 | 交付形态 | 版本/标识 | 主要用途 |
|---|---|---|---|
| `osci-render` | Linux/Windows 独立程序 | Premium 2.8.10.3 | 把文件、图形、Lua、Blender 场景或音频变成可听、可显示的 XY/XYZ 示波器图形，并进行合成、效果、调制、显示和录制 |
| `osci-render` | VST3 效果器 | 2.8.10.3，`Fx` | 在宿主内处理音频，并复用 osci-render 的文件、效果、调制和显示界面 |
| `osci-render` | VST3 乐器 | 2.8.10.3，`Instrument/Synth` | 在宿主内由 MIDI 驱动图形声音合成 |
| `sosci` | Linux/Windows 独立程序 | 1.3.5.3 | 播放或采集音频并以软件示波器显示，带基础音频整形、显示仿真、录制和音频转视频 |
| `sosci` | VST3 效果器 | 1.3.5.3，`Fx` | 在 DAW/宿主内把输入音频显示成示波器画面并输出/录制 |
| Blender `osci-render` 扩展 | Blender add-on | manifest 1.0.3；`bl_info` 1.1.0 | 把 Grease Pencil 动画保存为 GPLA，或经本机 TCP 实时发送给 osci-render |

两款主程序共享同一套 JUCE 音频、OpenGL 软件示波器、项目状态、录制和 FFmpeg 后端。差异集中在输入和创作层：

- `osci-render` 是完整的“内容生成与声音合成器”。它有文件解析器、3D/矢量/分形/Lua/图像输入、MIDI 多复音、DAHDSR 包络、26 种内置几何/声音效果、一个可编辑 Lua 效果，以及 LFO/随机/包络/侧链调制。
- `sosci` 是轻量的“音频示波器”。它以音频文件、插件宿主输入或系统音频输入为主，提供音量、阈值、单声道立体化和平滑处理，但没有发现 osci-render 的文件生成器、MIDI 音符合成、完整效果链、Lua 或 Blender 对象服务器。
- Blender 插件只负责几何导出和传输，本身不合成音频、不显示示波器，也不录制视频。

本报告中的“全部功能”指当前样本能通过源码、VST 元数据、产品类/符号、参数字符串、帮助文本和内嵌项目交叉确认的功能面，不包含通用 JUCE/系统库中未被产品接入的能力。除 Blender 源码外，本轮没有运行任何样本，因此仍不能保证每项功能在每个平台、每种授权状态和每个插件宿主中都可实际进入。

## 2. 证据等级与分析方法

后文使用四种证据等级：

- **D1（直接）**：Blender Python 源码、manifest 或 VST `moduleinfo.json` 明确声明。
- **D2（强静态）**：产品专属类/函数符号与相邻的参数、菜单或帮助文本相互印证。
- **S（静态推断）**：实现后端或入口存在，但平台、授权、宿主条件或 UI 可达性尚未运行确认。
- **V（待验证）**：只能通过隔离动态测试回答。

本轮对 Linux 未剥离 ELF 的 `.symtab`、`.dynsym` 和可打印字符串做了二次提取，并比较独立程序与 VST3 的编译单元集合；Windows 功能以相同版本号、安装载荷、资源/字符串和 A-001 的 PE 结果作相关性核对。所有二进制地址均是对应 Linux ELF 符号表中的虚拟地址，字符串地址均是文件偏移，不是运行时绝对地址。

## 3. 功能架构与数据流

### 3.1 osci-render

```text
文本 / Lua / SVG / OBJ / 分形 / 图像与视频 / GPLA / Blender 实时帧
                                │
                                ▼
                    文件解析与逐帧几何生成
                                │
              MIDI 音符/包络 ──┼── 外部音频输入
                                ▼
             26 种效果 + 自定义 Lua 效果 + 参数调制
                                │
                 ┌──────────────┴──────────────┐
                 ▼                             ▼
            立体声音频输出                OpenGL 示波器画面
                                               │
                               弹出/全屏、共享纹理、录制/导出
```

### 3.2 sosci

```text
音频文件 / 系统音频 / DAW 插件输入
                 │
                 ▼
音量 + 阈值 + 单声道立体化 + 平滑
                 │
        ┌────────┴────────┐
        ▼                 ▼
    音频输出         OpenGL 示波器画面
                           │
             弹出/全屏、共享纹理、录制/导出
```

### 3.3 Blender 插件

```text
可见 Grease Pencil 对象 + 场景相机 + 当前/全部动画帧
                       │
                       ├─ 保存为 GPLA 2.0.0
                       └─ Base64 + 换行，经 localhost TCP 实时发送
```

## 4. osci-render 完整功能面

### 4.1 内容输入、文件解析和内置示例

`FileParser` 统一调度多类输入，符号中可直接看到 `getLua/getObject/getImg/getLineArt/getSvg/getText/getFractal/getWav`。可恢复的解析器如下：

| 输入族 | 已确认能力 | 证据等级 |
|---|---|---|
| 文本 | `TextParser` 把普通/格式化文本和字体转换成可绘制路径；有文本编辑组件 | D2 |
| Lua | `LuaParser` 执行逐采样形状脚本；可作为文件生成器，也可作为效果脚本 | D2 |
| 3D 模型 | OBJ 示例、`tiny_obj_loader`、`WorldObject`、相机/视锥类共同确认 OBJ 网格路径 | D2 |
| SVG | `SvgParser::pathToShapes` 把 SVG 路径转换成形状 | D2 |
| L-system 分形 | `FractalParser` 支持 axiom、rules、迭代深度、规则应用、JSON 导入/导出和重建 | D2 |
| Line Art | `LineArtParser` 支持 GPLA 2.0.0 二进制帧、JSON 帧、逐帧选择、顶点重排和帧装配 | D2 |
| 图像/GIF | `ImageParser` 支持静态图像和 GIF；按阈值/步幅寻找像素并生成扫描路径 | D2 |
| 视频 | `ImageParser::processVideoFile/loadAllVideoFrames` 经 FFmpeg 解码视频帧；样本直接出现 `.mp4`、`.mov` | D2/S |
| 音频 | `WavParser` 支持解码、重采样、循环、暂停和按块播放；离线文件选择器接受 WAV/AIFF/FLAC/OGG/MP3 | D2 |
| Blender 实时流 | `ObjectServer` 接收对象帧；About 页面显示 Blender 端口，菜单可随机化端口 | D2 |

图像/视频大文件会先显示 `Large File` 警告，确认后再加载。静态字符串明确出现 GIF、MP4、MOV、JPEG、WAV、AIFF、MP3；通用图像解码代码还包含 PNG/JPEG/GIF 后端，但实际文件选择器对全部扩展名的白名单需要动态确认。

文件区支持：

- 打开一个或多个文件，并以网格显示；
- 关闭当前文件、锁定网格项目、切换选中项；
- `k` 切换上一个文件、`j` 切换下一个文件；
- 用 `File Select` 参数选择当前文件，因而可被宿主自动化或 MIDI CC 控制；
- 对多帧文件启用/关闭动画、循环、从指定起始帧播放；
- 动画速率按 `Frames per Beat` 或 `Frames per Second` 设置，并可同步 DAW BPM；
- 图像可反相、设置像素阈值和扫描步幅；
- 分形可设置递归深度；
- 时间线支持播放状态、位置和重复。

内置选择页直接列出的示例包括：

- 文本：Hello World、Greek、Paperclip；
- Lua：Spiral、Shape Generator、Squiggles、Donut、Gravity Well、Helix、Human、Hypercube、Mushroom、Planet；
- 3D：Cube、Diamond、Dodecahedron、Humanoid Quad、Icosahedron、Lamp、Shuttle、Suzanne、Teapot、Tetrahedron；
- SVG：Air Horn、Alien、Bicycle、Card、Cash、Clippy、Puzzle、Skull、Yin Yang；
- 分形：Koch Snowflake、Sierpinski Triangle、Dragon Curve、Binary Tree、Hilbert Curve。

资源中还存在 `osci-render.txt`、`sosci.txt` 等文本素材，但未把仅存在的资源文件误计为可见菜单项。

### 4.2 声音生成、音频输入和 MIDI

osci-render 的核心处理函数 `OscirenderAudioProcessor::processBlock` 同时接收音频缓冲区和 MIDI 缓冲区。用户可在两种声源路径间切换：

- 关闭 MIDI 时，以 `Frequency` 参数连续重复绘制当前图形；频率决定每秒绘制次数，也直接决定音高和闪烁/绘制精度的权衡；
- 打开 MIDI 时，由音符触发 `ShapeVoice`。`VoiceManager` 实现 note-on、note-off、复音分配、voice stealing、sostenuto 和逐声部渲染；
- 打开 `Audio Input Enabled` 时，用输入音频替代生成音频；独立程序可从音频设备/系统音频采集，插件由宿主提供输入；
- 可显示内置 MIDI 键盘。

MIDI/声部参数包括：

| 参数 | 功能 |
|---|---|
| Voices | 复音声部数 |
| Bend / Pitch Bend Range | 弯音范围 |
| Velocity Tracking | 力度对输出/声部的跟踪 |
| Glide Time | 滑音时间 |
| Glide Slope / Octave Scale | 滑音按音程缩放 |
| Always Glide | 即使断奏也始终滑音 |
| Legato | 单声部连续音符不重新触发包络 |
| DAHDSR | Delay、Attack、Hold、Decay、Sustain、Release，以及 Attack/Decay/Release 曲线形状 |

Linux 构建会尝试加载 `/usr/local/lib/libMTS.so`，并包含 MTS-ESP 客户端注册、主调音器检测、音符过滤、调音表、音阶名和周期比接口，界面可显示 `MTS-ESP Connected`。因此微分音/MTS-ESP 接入属于已编译能力；没有 MTS 主服务时的完整回退行为仍需运行确认。

参数右键菜单支持学习、取消或移除 MIDI CC 分配，并持久化通道、CC 编号和目标参数。VST3 乐器版在元数据中明确标为 `Instrument/Synth`；效果器版是否仍对音符事件开放输入总线，不能仅凭共享源码单元确定。

### 4.3 效果链：26 种内置效果和 1 个自定义效果

参数注册块、效果类和帮助文本共同恢复出以下完整内置效果面：

| # | 效果 | 参数/行为 |
|---:|---|---|
| 1 | Delay | `Decay` 控制回声衰减/音量，`Length` 控制回声间隔 |
| 2 | Perspective | 3D 透视强度与相机 Field of View |
| 3 | Bit Crush | 点坐标量化、分辨率和干湿比；画面像素化，声音更数字化/失真 |
| 4 | Polygonizer | 强度、边数、条纹尺寸、旋转和条纹相位，把点约束到多边形图案 |
| 5 | Spiral Bit Crush | 强度、密度、扭曲、缩放和旋转，把点约束到螺旋图案 |
| 6 | Distort | 分别在 X/Y/Z 方向抖动当前音频采样/点 |
| 7 | Unfold | 按段数绕中心旋转重复；小数值可平滑形变 |
| 8 | Multiplex | X/Y/Z 网格尺寸、网格间平滑和样本延迟 |
| 9 | Wobble | 对当前突出频率叠加轻微失谐正弦波；有 Amount 和 Phase |
| 10 | Duplicator | Copies、Spread、Angle Offset；多副本同时形成可听和声 |
| 11 | Dash | 虚线数量、偏移和每段宽度 |
| 12 | Trace | 起始位置和每周期绘制长度，可动画化为从点逐步描线 |
| 13 | Scale | X/Y/Z 三轴缩放 |
| 14 | Rotate | X/Y/Z 三轴旋转 |
| 15 | Translate | X/Y/Z 三轴平移，包括水平、垂直和远离相机 |
| 16 | Ripple | 波纹深度、相位和数量；动画化相位可产生移动波纹 |
| 17 | Swirl | 把图形卷成螺旋 |
| 18 | Bounce | 物体大小、运动速度和运动方向/角度 |
| 19 | Skew | X/Y/Z 剪切 |
| 20 | Kaleidoscope | 段数、隔段镜像、径向展开和按段裁剪 |
| 21 | Vortex | 强度、角度倍数/涡旋数量和旋转 |
| 22 | God Ray | 噪声光束强度/尺寸与向内或向外的位置方向 |
| 23 | Vector Cancelling | 每隔若干采样反相以抵消音频；降低声音同时扭曲图形 |
| 24 | Twist | 软木塞式空间扭曲 |
| 25 | Bulge | 放大中心、压缩边缘，同时对音频产生失真 |
| 26 | Smoothing | 低通滤波，去除高频，让图形更平滑、声音更柔和 |
| 27 | Custom Lua Effect | 可编辑 Lua 处理函数和 Strength；与普通 Lua 形状文件共用编辑器但可切换编辑目标 |

资源中的 `fixed_rotate.svg` 是图标/历史资源；当前参数注册只恢复出一个 `Rotate` 效果，因此不把它额外计作第 27 种内置效果。

效果链还支持：添加、移除、启用/禁用、拖拽重排；预览鼠标悬停的效果；锁定同一效果内参数；链接参数使改动联动；重置默认值、直接输入值和编辑范围；一键随机化全部效果参数、启用状态与顺序；按默认动画设置把新效果自动关联 LFO。

### 4.4 调制系统

osci-render 的效果参数可绑定四类全局调制源，状态中会分别保存 `lfos`、`randoms`、`envelopes` 和 `sidechain` 分配：

1. **LFO**：基础波形字符串包括 Static、Sine、Square、Seesaw、Reverse Sawtooth；支持起点/终点、相位、单极/双极、频率/秒、Tempo 和 Trigger 速率模式，以及 dotted/triplet 拍号细分。
2. **可绘制 LFO 图形**：节点图可按 Step、Half、Paint 方式编辑，带 Smooth 开关、复制/粘贴、撤销记录和预设浏览器。
3. **LFO 预设**：可浏览工厂/用户预设、前后切换、保存、删除、设为默认/清除默认；可读取 Vital 的 `*.vitallfo` JSON，并扫描 `.local/share/vital`。静态字符串可见的工厂形状/门控预设包括 Saw Up、Saw Down、Staircase Down、Growing Oscillations、Nervous Groove、Pulse Series、Random Pulses、Shuffle Gate、Side Chain 1/2、Split Gate、Trance Gate、Custom、Freeze。
4. **Random**：Perlin、Sample & Hold、Sine Interpolate 三种随机风格，并有独立速率配置。
5. **Envelope**：完整 DAHDSR 和曲线节点编辑；还包含 Sustain Envelope、Loop Point、Loop Hold 状态。
6. **Sidechain**：输入音量通过可编辑传递曲线驱动参数，带 Attack/Release；没有输入时显示 `NO SIDECHAIN INPUT`。

每个分配包含目标参数、深度和双极性；可添加/移除调制。参数 UI 会显示当前调制位置和来源颜色。

### 4.5 Lua 创作环境

二进制包含 LuaJIT 和 Lua 5.1 兼容环境，并提供代码编辑器、Lua Console、暂停/清空控制、内置文档和 Lua 5.1 参考手册链接。用户可在“当前 Lua 文件”和“Custom Lua Effect”之间切换编辑对象，并用 A-Z 共 26 个滑杆向脚本传入 0..1 控制值。

脚本逐采样返回：

- `{x, y}`：二维位置；
- `{x, y, z}`：三维位置；
- `{x, y, z, r, g, b}`：位置和逐点 RGB 颜色。

可用上下文变量包括：

| 类别 | 变量 |
|---|---|
| 基础 | `step`、`phase`、`sample_rate`、`frequency`、`cycle_count`、A-Z slider |
| 外部音频 | `ext_x`、`ext_y` |
| MIDI/声部 | `midi_note`、`velocity`、`voice_index`、`note_on` |
| DAW transport | `bpm`、`play_time`、`play_time_beats`、`is_playing`、`time_sig_num`、`time_sig_den` |
| 包络 | `envelope`、`envelope_stage` |
| 仅效果脚本 | 当前点 `x`、`y`、`z` |

注册的几何和 DSP API 包括：

- 形状：`osci_line`、`osci_rect`、`osci_square`、`osci_circle`、`osci_ellipse`、`osci_arc`、`osci_polygon`、`osci_bezier`、`osci_lissajous`；
- 波形：`osci_square_wave`、`osci_saw_wave`、`osci_triangle_wave`、`osci_pulse_wave`；
- 组合/变换：`osci_chain`、`osci_mix`、`osci_translate`、`osci_scale`、`osci_rotate`；
- 数学/DSP：`osci_noise`、`osci_lerp`、`osci_smoothstep`、`osci_clamp`、`osci_map`、`osci_dot`、`osci_cross`、`osci_length`、`osci_normalize`。

Lua 状态支持全局变量持续存在、`print` 输出和清空控制。执行器带最大指令数保护、错误报告、栈清理和 fallback 脚本，避免明显的无限脚本持续占用音频线程；保护阈值和实时压力仍需动态测试。

### 4.6 Blender 对象服务器

osci-render 内部包含 `ObjectServer::run/reload`，默认与 Blender 插件使用端口 51677。About 页可显示当前端口，菜单可执行 `Randomize Blender Port`。接收到的 GPLA 帧进入与本地 Line Art 相同的几何/动画路径。

服务只应绑定本机是基于配套插件的 `HOST = "localhost"` 推断；服务端实际监听地址、并发连接、帧上限和恶意输入健壮性需要隔离网络测试。

## 5. sosci 完整功能面

### 5.1 音频源和播放

sosci 的主流程是把音频直接作为 X/Y 示波器输入：

- 打开本地音频文件；`WavParser` 具备解码、重采样、进度、循环、暂停和按块处理；
- 文件格式后端直接列出 WAV/BWF、AIFF/AIF、FLAC、Ogg Vorbis、MP3；离线视频选择器使用 `*.wav;*.aiff;*.flac;*.ogg;*.mp3`；
- 时间线有 Play、Pause、Stop、Repeat、拖动位置和当前播放位置；
- 独立程序可从音频设备或系统/进程回环音频采集；启用音频输入会关闭已打开的文件，并以红色图标提示；
- VST3 版按 `Fx` 注册，由宿主提供音频输入和时间/采样率环境。

### 5.2 基础音频处理

| 功能 | 行为 |
|---|---|
| Volume | 调节输出音量 |
| Threshold | 把音频裁剪到最大值；阈值越严，失真越强 |
| Stereo | 把不适合显示的单声道信号变成立体声，使 XY 轨迹更有变化 |
| Smoothing | 低通滤波，减少高频，平滑画面并让声音不那么尖锐 |
| Mute | 静音音频输出；音频输入模式还会用静音避免反馈回路 |

菜单包含 `Force Disable RGB Input`，帮助文本的同一设置却写成 `Force Disable Brightness Input`。这证明 sosci 有一条可强制关闭的附加颜色/亮度输入路径，但当前样本仅凭字符串无法可靠恢复多通道到 RGB/亮度的确切映射，列为 V 级待验证项。

### 5.3 参数调制和 MIDI CC

sosci 没有 osci-render 的全局 LFO/Random/Envelope 编辑页面，但通用参数框架保留了逐参数调制：

- LFO 波形：Static、Sine、Square、Seesaw、Reverse Sawtooth；
- LFO Rate、Start、End；
- 以输入音量驱动参数的 Sidechain 开关；
- 参数启用、锁定、选择、优先级和数值平滑；
- MIDI CC 学习、取消和移除，并把参数映射保存到项目状态。

VST 元数据没有 MIDI 乐器分类，`SosciAudioProcessor` 也没有发现音符声部/合成器类。因此 MIDI CC 参数控制不能解释为 MIDI 音符发声功能。

### 5.4 项目和内置示例

sosci 提供 Open/Save/Save As/Create New Project、Recent Projects 和 `.sosci` 文件关联。包内有 5 个完整 XML 项目示例：

- Clean
- Default
- Rainbow
- Real Oscilloscope
- Vector Display

这些项目保存视觉参数、逐参数 LFO/Sidechain、布尔/整数设置和录制参数。`Rainbow` 项目直接使用 Sawtooth LFO 调制色相，说明基础 LFO 不是残留的未用代码。

## 6. 两款程序共有的软件示波器

### 6.1 显示模式与窗口操作

产品字符串恢复出五个明确命名的屏幕外观：Empty、Smudged、Smudged Graticule、Real Oscilloscope、Vector Display。内嵌项目的 `screenOverlay` 范围为 1..6，资源还区分屏幕/反射纹理，因此存在第六个枚举值或内部组合，但其用户可见名称无法仅凭当前静态证据无歧义映射。

窗口和交互功能包括：

- 软件示波器实时渲染；
- 全屏、双击全屏、退出全屏；
- 暂停/恢复视觉渲染；
- 弹出到独立窗口；
- 打开视觉设置页；
- 垂直翻转、水平翻转；
- Goniometer 模式，把画面旋转为相位关系显示；
- Sweep 模式，按时间从左到右扫描，并用 Trigger Value 稳定波形；
- 音频上采样，使画面更接近真实模拟示波器，但增加性能开销；
- Shutter Sync，使单帧亮度在绘制频率与视频帧率同步时保持稳定。

### 6.2 视觉参数

| 分组 | 参数 | 作用 |
|---|---|---|
| 线条颜色 | Line Hue、Line Intensity、Line Saturation | 电子束色相、亮度和饱和度；逐点 RGB 数据可覆盖统一线色 |
| 屏幕颜色 | Screen Hue、Screen Saturation | 屏幕底色的色相偏移和饱和度 |
| 光效 | Persistence、Afterglow、Glow、Ambient Light、Overexposure | 余辉持续、亮点衰减、泛光、环境光和过曝白化阈值 |
| 线条效果 | Focus、Noise/Grain、Visualiser Smoothing | 电子束聚焦、屏幕噪点和轨迹平滑 |
| 图像变换 | X/Y Scale、X/Y Offset、Flip、Goniometer | 缩放、偏移、镜像和旋转 |
| Sweep | Sweep milliseconds、Trigger Value | 扫描周期和触发电平 |

sosci 内嵌项目可直接恢复的典型合法范围包括：Persistence 0..6、Hue 0..359、Line Intensity 0..10、Line/Screen Saturation 0..5、Noise/Glow 0..1、Ambient 0..5、Sweep 0..1000 ms、Trigger -1..1、Visualiser Smoothing 0..1、Volume 0..3、Threshold 0..1。不同项目中 `Focus` 最小值出现 0.01 与 0.3 两种记录，不能据此断言所有版本/项目使用同一最小值。

渲染后端使用 OpenGL、多级纹理、余辉、紧/宽泛光、屏幕纹理、反射纹理、环境光和噪声 shader。存在保存渲染纹理为 PNG 的内部函数，但没有找到对应明确用户菜单，所以不把“截图导出 PNG”列为已开放功能。

### 6.3 共享纹理

Quick Controls 有 `Shared Texture` 开关和自定义 Syphon/Spout 服务名，作用是把示波器画面发送给外部接收器。Windows 载荷直接携带并导入 `SpoutLibrary.dll`，因此 Windows Spout 是 D2 级能力。

Linux 二进制同样编译了 `SharedTextureManager` 和按钮，但也含 `Content sharing not available on this platform!` 回退文本。Syphon 是 macOS 技术，而当前包没有 macOS 构建。因此“所有当前平台都能共享纹理”不能成立：Windows 可确认，Linux 是否有有效后端为 V 级。

## 7. 录制、视频编码和离线导出

两款程序共有以下录制能力：

- 实时录制示波器画面和音频，可分别打开/关闭 Record Video、Record Audio；
- 音频录制保存为 WAV；
- 设置视频分辨率、帧率和 Video Quality；录制进行中不允许改变分辨率/帧率；
- Lossless Audio、Lossless Video；界面会提示无损视频并非所有播放器都支持；
- 压缩速度预设至少包含 ultrafast、superfast、veryfast、faster、fast、medium、slower、veryslow；越慢通常文件越小；
- 用户可见编码器选项为 H.264 和 H.265/HEVC；`FFmpegEncoderManager` 还编译了 VP9 命令生成和硬件编码器探测，但未发现 VP9 的用户选择字符串，故只把它列为后端能力；
- 编码失败、OpenGL 初始化失败、帧尺寸异常、写管道失败和录制采样率无效都有专门错误路径。

`Render Audio File to Video` 是另一条离线流程：

1. 选择 WAV/AIFF/FLAC/OGG/MP3；
2. 选择输出视频文件；
3. 解码和重采样音频；
4. 离屏 OpenGL 按录制设置渲染每帧，并把帧写入 FFmpeg；
5. 用 `copy` 保留视频流，并在不同录制分支使用 `pcm_s16le` 或 `-b:a 384k` 等音频参数完成 mux；
6. 显示进度，允许取消，并清理临时文件。

帮助文本 `This feature is available in the premium version.` 与该入口相邻，说明至少某些构建会对离线转视频做 Premium 门控；当前 osci-render 样本明确为 Premium，sosci 包名未标 Premium。确切授权矩阵必须以合法授权状态动态验证，不能从字符串推导出绕过方式。

FFmpeg 不随当前包提供。程序会检查本地 FFmpeg，不兼容或缺失时提示下载；A-001 已记录固定 GitHub Release 基址。编码器列表查询、可用性测试和硬件编码器选择均有实现。下载文件是否做固定哈希/签名校验仍未确认。

## 8. 项目、预设、设置和恢复

### 8.1 项目管理

两款程序都有：

- Open Project、Save Project、Save Project As、Create New Project；
- 最近项目列表和 `No Recent Projects` 状态；
- 命令行打开项目、非法类型和文件不存在错误；
- Windows 文件关联：osci-render 使用 `.osci`，sosci 使用 `.sosci`；
- 项目状态 XML/XML-like 序列化，保存效果、参数、LFO/侧链、布尔/整数值、录制设置和属性。

osci-render 额外保存打开文件、当前文件、Custom Lua function、全局 LFO/Random/Envelope/Sidechain 分配、动画和 MIDI 合成状态。旧 1.x 状态有明确忽略/迁移分支，并有“免费版每参数 LFO转全局 LFO 分配”的兼容代码。

### 8.2 崩溃恢复和撤销

两个处理器都会保存 `lastHeartbeatTime`。如果上次未正常退出，会显示 `Possible Crash Detected`，让用户选择 `Continue` 或 `Reset to New Project`。项目状态解析失败、未知效果 ID、无 effects/files 节点等均有日志路径。

菜单有 Undo/Redo；LFO 图编辑、效果/参数更改至少部分进入可撤销状态。静态证据不足以列出每一种可撤销操作。

### 8.3 设置与诊断

共有或可见的设置入口包括：

- 独立程序的音频/MIDI 设备选择器和采样率/缓冲区配置；
- Standalone BPM；插件版由宿主 transport 提供 BPM/拍号/播放时间；
- Mute audio output；
- 旋钮拖动方式：圆周、左右或上下；
- 特殊按键监听、快捷键映射；
- 打开日志、应用设置文件和全局设置文件；
- About、官网、Discord、问题反馈链接和贡献者信息。

## 9. Blender 插件完整功能面

### 9.1 用户操作

插件在 Blender Render 属性中增加 `osci-render settings` 面板，包含：

- 端口设置，范围 51600..51699，默认 51677；
- `Connect to osci-render instance`；
- 连接后显示 `Close osci-render connection`；
- `Save line art to file`，文件扩展名强制为 `.gpla`；
- 连接失败、保存成功/失败和扩展名异常提示。

连接建立后立即发送当前帧。插件注册 `frame_change_pre` 和 `depsgraph_update_post` 回调，因此换帧和依赖图变化都会自动重新发送当前场景；退出时发送 `CLOSE\n` 并关闭 socket。

### 9.2 收集和序列化的数据

插件只导出可见 Grease Pencil 对象：Blender 4.3+ 使用 `GREASEPENCIL` 和新数据 API，旧分支使用 `GPENCIL`。每帧写入：

- 场景帧数和 `scene.render.fps`；
- 第一台数据相机镜头值，经 `-0.05 * lens` 转为 `focalLen`；
- 每个对象相对于场景相机的 4×4 矩阵；
- 每一图层当前/活动帧中的 strokes；
- 每条 stroke 的顶点数和所有 XYZ double 坐标。

`Save` 会遍历 `scene.frame_start..scene.frame_end` 的全部帧，结束后恢复原来的当前帧。实时发送只构造当前帧，但 GPLA 文件头仍写入场景总帧数；数据整体先 Base64，再追加换行并经 TCP 发送。

### 9.3 GPLA 2.0.0 协议

源码直接定义：

```text
"GPLA    " + uint64 major/minor/patch
"FILE    "
  "fCount  " + uint64 frame_count
  "fRate   " + uint64 fps
"DONE    "
  one or more FRAME records
"END GPLA"
```

帧内依次为 `FRAME`、`focalLen`、`OBJECTS`、`OBJECT`、`MATRIX`、`STROKES`、`STROKE`、`vertexCt`、`VERTICES`，各层用 8 字节 ASCII 标记和 `DONE` 结束；整数为 8 字节小端，矩阵/坐标为 native `struct.pack("d")` double。在常见 x86-64 Windows/Linux Blender 上是小端 double，但源码未显式指定 `<d`，跨大端平台不具备严格可移植性。

### 9.4 已知缺陷和限制

源码审查确认：

1. manifest 版本 1.0.3 与 `bl_info` 1.1.0 不一致；
2. manifest 最低 Blender 4.2.0 与 `bl_info` 的 3.1.2 不一致；
3. 注册属性写入 `bpy.types.Scene.oscirenderPort`，注销却删除 `bpy.types.Object.oscirenderPort`，会造成注销异常或残留；
4. 直接访问 `bpy.data.cameras[0]`、`scene.camera` 和 current/active frame，没有无相机、无活动帧保护；
5. `depsgraph_update_post` 每次都同步序列化并发送整帧，复杂场景可能卡顿或产生高频本机 IPC；
6. 发送异常只把全局 `sock` 置空，没有显式关闭故障 socket；
7. TCP 没有认证、长度前缀、重连、压缩、节流或完整性校验，但目标被限制为 `localhost`；
8. `osci_render_save.execute()` 的部分失败路径没有显式返回 Blender operator 状态；
9. 只导出可见 Grease Pencil strokes，不导出材质、颜色、线宽、填充、灯光、普通网格或音频。

## 10. 独立程序、VST3 与平台差异

### 10.1 VST3 形态

VST `moduleinfo.json` 直接给出：

- `osci-render.vst3`：`Fx`；
- `osci-render-instrument.vst3`：`Instrument` + `Synth`；
- `sosci.vst3`：`Fx`；
- 三者使用 VST SDK 3.8.0。

编译单元对比显示，osci-render 的两个 VST3 都保留与独立程序相同的全部 154 个产品/支持编译单元；sosci VST3 保留独立程序 32 个单元中的 31 个。两者唯一稳定缺失的产品单元都是 `CustomStandalone.cpp`。因此可以确认插件并非另一个精简 DSP，而是复用主要编辑器、效果、可视化、项目、录制和离线渲染代码；区别主要是宿主包装、总线/MIDI 分类和音频设备所有权。

| 差异点 | 独立程序 | VST3 |
|---|---|---|
| 音频/MIDI 设备 | 自己选择设备、采样率和缓冲区 | 由宿主提供 |
| 时间/BPM | Standalone BPM 和本地播放状态 | DAW transport、BPM、拍号、播放时间 |
| 项目状态 | `.osci`/`.sosci` 和应用设置 | 同时受宿主插件状态保存/恢复影响 |
| osci-render 角色 | 可生成、播放、采集和显示 | Fx 版主角色是处理输入；Instrument 版主角色是 MIDI 合成 |
| sosci 角色 | 文件/系统音频示波器 | 宿主输入音频示波器 |
| 录制/离线入口 | D2 确认 | 相同单元已编译；具体宿主内入口和文件对话框为 S/V |

### 10.2 Windows 与 Linux

- Windows 版本直接部署 Spout；Linux 包不带等价外部共享库，纹理共享可用性待验证。
- Linux 使用 ALSA、OpenGL、libcurl；Windows 使用系统音频、WININET/Winsock、DirectX/OpenGL 和 Spout。
- Windows/Linux 版本号一致，但本轮深度符号分析主要来自 Linux 未剥离构建；不能仅凭同版本号承诺逐功能二进制完全等价。
- Windows 的系统音频回环/默认输出设备路径在字符串和平台导入中存在；具体设备命名和反馈抑制需动态验证。

## 11. 明确未发现的功能

为避免把共用库或素材误报为产品功能，当前样本中没有足够证据支持以下说法：

- sosci 具备 Lua、OBJ、SVG、分形、图像/视频转线稿、Blender 接收、MIDI 音符合成或 osci-render 的 26 种效果链；
- Blender 插件本身进行音频合成、示波器显示、视频编码或网络远程控制；
- Linux 当前构建一定能向外部软件共享纹理；
- VP9 一定出现在用户可选编码器菜单；
- `VisualiserRenderer::saveTextureToPNG` 一定有用户可见截图按钮；
- 所有 JUCE 支持的音频/图像格式都被产品文件选择器开放；
- Premium 字符串能说明每项功能的具体免费/付费授权矩阵；
- 当前样本运行时不会访问网络或下载其他组件。

## 12. 关键静态证据索引

| 样本/位置 | 证据 | 支持的结论 |
|---|---|---|
| `osci-render` ELF VA `0x772a80` | `OscirenderAudioProcessor::processBlock(AudioBuffer, MidiBuffer)` | 主音频/MIDI 处理核心 |
| `osci-render` ELF VA `0x8d6d00` | `FileParser::parse(...)` | 多格式文件入口 |
| `osci-render` ELF VA `0x8ddd50` | `LineArtParser::parseBinaryFrames` | GPLA 二进制帧解析 |
| `osci-render` ELF VA `0x8e7270` | `ImageParser::processVideoFile` | 视频帧输入 |
| `osci-render` ELF VA `0x8bd3f0/0x8bd820` | `LuaParser::parse/run` | Lua 解析和逐样本执行 |
| `osci-render` ELF VA `0x861f30/0x8605e0` | `VoiceManager::noteOn/noteOff` | MIDI 声部管理 |
| `osci-render` ELF VA `0xa3e2e0` | `EffectTypeGridComponent::setupEffectItems` | 效果选择面板 |
| `osci-render` ELF file `0xab11fd..0xab190e` | 连续效果参数注册字符串 | 26 种效果名称和参数边界 |
| `osci-render` ELF file `0xac7b18..0xac8d08` | 连续产品帮助文本 | 各效果实际行为 |
| `osci-render` ELF file `0xab4c45..0xab4d84` | `osci_*` API 名称 | Lua 几何/DSP API |
| `osci-render` ELF VA `0xa2a1c0` | `OfflineAudioToVideoRendererComponent::renderToFile` | 离线音频转视频 |
| `sosci` ELF VA `0x6ea4e0` | `SosciAudioProcessor::processBlock` | sosci 主音频处理核心 |
| `sosci` ELF VA `0x7c7450/0x7c7be0` | `WavParser::parse/processBlock` | 文件解码和播放 |
| `sosci` ELF VA `0x8085a0` | `VisualiserComponent::setRecording` | 实时录制 |
| `sosci` ELF file `0x8833e5..0x883dce` | 参数、菜单和 Quick Controls 字符串 | 音频/显示/录制/项目入口 |
| `sosci` ELF file `0x896828..0x897d38` | 产品帮助文本 | 参数语义和用户工作流 |
| `sosci` ELF file `0x1a23e88` 起的 5 段项目 XML | Clean/Default/Rainbow/Real/Vector 项目 | 预设、范围、LFO/Sidechain 状态 |
| Blender ZIP `osci_render/__init__.py:36..324` | 完整 Python 源码 | 面板、socket、GPLA、handlers 和缺陷 |
| 三个 VST3 `moduleinfo.json` | 类别、版本、SDK | Fx/Instrument/Synth 分类 |

## 13. 仍需动态验证的清单

如果下一阶段获准在可回滚、无真实凭据的隔离虚拟机中执行，建议按以下顺序验证：

1. **逐入口走查**：分别截取 osci-render/sosci 独立程序和三种 VST3 的菜单、面板、右键菜单，核对本报告功能是否可见以及 Premium 禁用态。
2. **格式矩阵**：以最小合法/损坏样本测试 TXT、Lua、OBJ、SVG、L-system、GPLA、JSON line art、PNG/JPEG/GIF、MP4/MOV 和五种音频格式，记录错误和资源上限。
3. **音频/MIDI**：验证声部上限、弯音、力度、滑音、legato、DAHDSR、MTS-ESP、MIDI CC、Fx 插件的 MIDI 事件总线和 instrument 插件的音频输入总线。
4. **效果与调制**：记录 26 种效果的默认值/范围、串联顺序、宿主自动化 ID、LFO 数量、Random/Envelope/Sidechain 数量和实时 CPU 峰值。
5. **视觉与多通道**：恢复六个 `screenOverlay` 值的准确名称，验证 RGB/brightness 通道映射、逐点 RGB、Sweep trigger 和 Shutter Sync。
6. **录制/导出**：记录所有可选容器/编码器、VP9 是否可见、硬件编码器选择、无损输出格式、FFmpeg 命令行和下载物哈希。
7. **纹理共享**：Windows 用 Spout 收发测试；Linux 确认是否明确禁用、是否存在其他后端。
8. **项目兼容**：验证 `.osci/.sosci` round-trip、旧版本迁移、崩溃恢复、宿主状态恢复和丢失外部文件时的处理。
9. **Blender**：覆盖 4.2/4.3、无相机、空图层、多个相机、多个 Grease Pencil 对象、大帧、断线、重连和卸载；抓取 TCP 以确认服务端监听范围和 GPLA 解析边界。

在这些动态步骤完成前，本报告是功能面的高置信静态重建，不是运行验收、性能报告或安全结论。
