# 专业技能目录

本目录保存可组合的专业技能、工作方法和判断依据。每项技能使用一份普通 Markdown 文档，底层仍直接放在 `knowledge/` 下。

| 技能 ID | 名称 |
|---|---|
| [ai-character-image-consistency](ai-character-image-consistency.md) | AI 角色图片一致性 |
| [backend-development](backend-development.md) | 后端开发 |
| [audio-dsp-rhythm-engineering](audio-dsp-rhythm-engineering.md) | 音频 DSP 与节奏工程 |
| [book-planning](book-planning.md) | 图书定位与章节规划 |
| [book-production](book-production.md) | 图书生产与统稿 |
| [book-review](book-review.md) | 图书审校与一致性检查 |
| [cpp-core-systems-engineering](cpp-core-systems-engineering.md) | C++ 核心系统工程 |
| [cpp-qt-testing](cpp-qt-testing.md) | C++ 与 Qt 测试工程 |
| [critical-review](critical-review.md) | 独立评审 |
| [ffmpeg-media-engineering](ffmpeg-media-engineering.md) | FFmpeg 多媒体工程 |
| [frontend-development](frontend-development.md) | 前端开发 |
| [opencv-video-rhythm-analysis](opencv-video-rhythm-analysis.md) | OpenCV 视频节奏分析 |
| [planning](planning.md) | 方案规划 |
| [project-management](project-management.md) | 项目管理 |
| [requirements-analysis](requirements-analysis.md) | 需求分析 |
| [reader-feedback](reader-feedback.md) | 读者学习反馈与修订 |
| [research](research.md) | 资料研究与事实核查 |
| [reverse-engineering](reverse-engineering.md) | Linux 与 Windows 可执行文件逆向分析 |
| [social-media-operations](social-media-operations.md) | 社交媒体运营 |
| [qt-quick-ui-engineering](qt-quick-ui-engineering.md) | Qt Quick/QML 界面设计与工程 |
| [qt-scene-graph-engineering](qt-scene-graph-engineering.md) | Qt Scene Graph 实时渲染工程 |
| [software-development-basics](software-development-basics.md) | 软件研发基础 |
| [software-engineering](software-engineering.md) | 软件工程 |
| [system-design](system-design.md) | 系统设计 |
| [team-management](team-management.md) | 团队组织与成员维护 |
| [technical-book-validation](technical-book-validation.md) | 技术书示例与实验验证 |
| [testing](testing.md) | 测试设计与执行 |
| [travel-planning](travel-planning.md) | 旅行规划 |
| [visual-design](visual-design.md) | 视觉设计 |
| [windows-qt-build-engineering](windows-qt-build-engineering.md) | Windows x64 与 Qt 构建工程 |
| [windows-release-engineering](windows-release-engineering.md) | Windows 发布与供应链工程 |
| [writing](writing.md) | 长篇写作与编辑 |
| [xhs-content-operations](xhs-content-operations.md) | 小红书内容运营 |
| [xhs-plog-creation](xhs-plog-creation.md) | 小红书 plog 内容创作 |
| [yocto-engineering](yocto-engineering.md) | Yocto 工程 |

## 文件约定

一个技能条目对应 `knowledge/<id>.md`，字段与有效引用统一见 [CONFIG_SCHEMA.md](../CONFIG_SCHEMA.md)。必需文件头为：

```yaml
---
id: frontend-development
name: 前端开发
status: active
---
```

正文说明专业技能、方法、适用条件与判断依据。保持文件平铺，需要新增条目时直接增加另一个 Markdown 文件并更新本索引。技能 ID 使用小写字母、数字与短横线，保持引用稳定。

技能不定义成员身份、不分配任务、不授予权限。具体技术栈、旅行日期、读者和预算由项目文档提供；补充技能是为了支撑角色职责，不把另一角色的职责一并带入。

## 引用和加载

共享角色以 `knowledge` 定义基础要求。项目 TEAM 以 `roleKnowledge` 为相应角色补充技能，成员文件只记录身份和分工。初始化按[协议](../SESSION_PROTOCOL.md)将二者合并去重，并完整读取对应正文。框架级 [超级管理员](../SUPER_ADMIN.md)直接加载 `team-management`，不使用项目角色技能组合。

专业技能可以被多个角色、多个项目复用。同项目同角色的成员加载相同组合，分工差异放在成员 scope。索引不代替技能正文；这些文档也不负责安装工具或提供执行环境。

visual-design 是视觉设计师的通用基础方法；ai-character-image-consistency 只适用于采用 AI 生成角色图片的项目，应作为 visual-designer 的项目补充知识按需配置。二者分层避免普通视觉设计任务被强制加载特定生成工具流程。

social-media-operations 是社交媒体运营岗位的通用基础知识；xhs-content-operations 是该岗位面向小红书的项目补充知识，负责日历、发布协作和数据反馈。xhs-plog-creation 负责单条 plog 的图文创作，可按项目分配给作者等内容成员；运营知识不取代创作职责。

## 维护

技能状态为 draft、active、deprecated。新会话要求有效集合中的技能全部存在且 active。

修改或退役正文前，检查当前仓库共享角色的基础引用、全部项目的补充引用及超级管理员入口的直接引用，再确定受影响的会话。退役前迁移仍在使用的引用并保留历史文件。文档更新不代表其他会话已经读取，也不会自动同步到其他克隆；成员开始新任务时按初始化协议刷新当前可访问的资料。
