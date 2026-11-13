# 专业技能目录

本目录保存可组合的专业技能、工作方法和判断依据。每项技能使用一份普通 Markdown 文档，底层仍直接放在 `knowledge/` 下。

| 技能 ID | 名称 |
|---|---|
| [ai-character-image-consistency](ai-character-image-consistency.md) | AI 角色图片一致性 |
| [ai-comic-asset-design](ai-comic-asset-design.md) | AI 漫剧角色场景与道具资产设计 |
| [ai-comic-audio-production](ai-comic-audio-production.md) | AI 漫剧配音音效与音乐设计 |
| [ai-comic-business-iteration](ai-comic-business-iteration.md) | AI 漫剧流程升级与商业迭代 |
| [ai-comic-image-production](ai-comic-image-production.md) | AI 漫剧分镜图像生产与修订 |
| [ai-comic-production-integration](ai-comic-production-integration.md) | AI 漫剧全流程整合与返工判断 |
| [ai-comic-production-planning](ai-comic-production-planning.md) | AI 漫剧定位与制作规划 |
| [ai-comic-script-adaptation](ai-comic-script-adaptation.md) | AI 漫剧故事改编与剧本拆解 |
| [ai-comic-video-production](ai-comic-video-production.md) | AI 漫剧视频生成与连续性控制 |
| [ai-comic-visual-storytelling](ai-comic-visual-storytelling.md) | AI 漫剧分镜与视觉叙事 |
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
| [investment-analysis](investment-analysis.md) | 投资组合分析与跟踪 |
| [opencv-video-rhythm-analysis](opencv-video-rhythm-analysis.md) | OpenCV 视频节奏分析 |
| [planning](planning.md) | 方案规划 |
| [project-management](project-management.md) | 项目管理 |
| [qemu-engineering](qemu-engineering.md) | QEMU 模型开发与验证 |
| [requirements-analysis](requirements-analysis.md) | 需求分析 |
| [reader-feedback](reader-feedback.md) | 读者学习反馈与修订 |
| [research](research.md) | 资料研究与事实核查 |
| [reverse-engineering](reverse-engineering.md) | Linux 与 Windows 可执行文件逆向分析 |
| [securities-analysis](securities-analysis.md) | 证券分析 |
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
| [value-investing](value-investing.md) | 价值投资与企业估值 |
| [value-investing-research](value-investing-research.md) | 价值投资尽调与催化剂 |
| [video-directing](video-directing.md) | 影视导演与分镜设计 |
| [video-production-editing](video-production-editing.md) | 视频制作剪辑与后期 |
| [visual-design](visual-design.md) | 视觉设计 |
| [wechat-miniprogram-development](wechat-miniprogram-development.md) | 微信小程序开发 |
| [wechat-miniprogram-testing](wechat-miniprogram-testing.md) | 微信小程序测试 |
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

video-directing 和 video-production-editing 分别是导演、视频制作与后期岗位的通用基础方法。AI 漫剧的按章知识通过项目角色补充技能选择，沿用各岗位职责，不自动增加工具或发布权限。

social-media-operations 是社交媒体运营岗位的通用基础知识；xhs-content-operations 是该岗位面向小红书的项目补充知识，负责日历、发布协作和数据反馈。xhs-plog-creation 负责单条 plog 的图文创作，可按项目分配给作者等内容成员；运营知识不取代创作职责。

wechat-miniprogram-development 和 wechat-miniprogram-testing 分别提供微信小程序实现与平台测试方法，适合作为 developer、tester 的项目补充技能。网页前端方法不能代替小程序运行环境与真机验证；具体业务规则由项目需求提供，不改变共享角色的基础技能或职责。

### AI 漫剧知识的按章导航

以下九项由《AI漫剧创作一本通》按章独立提炼，按实际任务组合使用。它们不自动加入共享角色基础技能；需要供项目成员持续加载时，由获授权的配置维护者按角色职责配置项目 `TEAM.yaml.roleKnowledge`，成员在新任务开始时按会话协议刷新。临时查阅可从本索引进入；索引不能代替完整技能正文。

| 原书章节 | 可复用知识 | 主要使用时机 |
|---|---|---|
| 第1章 | [定位与制作规划](ai-comic-production-planning.md) | 明确受众、形式、资源与试制范围 |
| 第2章 | [故事改编与剧本拆解](ai-comic-script-adaptation.md) | 从故事设定走向可制作剧本与分镜表 |
| 第3章 | [角色场景与道具资产设计](ai-comic-asset-design.md) | 设计和固定可反复引用的视觉资产 |
| 第4章 | [分镜与视觉叙事](ai-comic-visual-storytelling.md) | 选择景别、视角、运动与剪切方式 |
| 第5章 | [分镜图像生产与修订](ai-comic-image-production.md) | 生成静帧，处理多图参考与局部编辑 |
| 第6章 | [视频生成与连续性控制](ai-comic-video-production.md) | 选择生成模式，检查动作与跨镜头连续性 |
| 第7章 | [配音音效与音乐设计](ai-comic-audio-production.md) | 设计并检查人物声音、动作音效与音乐 |
| 第8章 | [全流程整合与返工判断](ai-comic-production-integration.md) | 组织完整样片，定位问题和修订环节 |
| 第9章 | [流程升级与商业迭代](ai-comic-business-iteration.md) | 评估自动化、生产成本与反馈循环 |

章节来源、阅读覆盖和未验证范围见[来源与阅读记录](../docs/ai-comic-creation-source.md)。这组知识保存工作方法；具体项目的原始媒体、完整指令、生成记录与源工程仍保存在声明的成果物工作区。

## 维护

技能状态为 draft、active、deprecated。新会话要求有效集合中的技能全部存在且 active。

修改或退役正文前，检查当前仓库共享角色的基础引用、全部项目的补充引用及超级管理员入口的直接引用，再确定受影响的会话。退役前迁移仍在使用的引用并保留历史文件。文档更新不代表其他会话已经读取，也不会自动同步到其他克隆；成员开始新任务时按初始化协议刷新当前可访问的资料。
