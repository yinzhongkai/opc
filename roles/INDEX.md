# 角色目录

本目录只包含可供项目成员绑定的角色，按共同职责划分。技术方向和业务领域通过项目角色补充技能表达。每个文件包含职责、边界、产出与基础技能引用。超级管理员是根 [框架管理入口](../SUPER_ADMIN.md)，不属于项目角色。底层目录和配置字段仍使用 `roles`、`role`、`knowledge` 与 `roleKnowledge`。

| 角色 ID | 名称 |
|---|---|
| [project-manager](project-manager.md) | 项目经理 |
| [product-manager](product-manager.md) | 产品经理 |
| [researcher](researcher.md) | 研究员 |
| [planner](planner.md) | 规划师 |
| [architect](architect.md) | 系统架构师 |
| [core-systems-engineer](core-systems-engineer.md) | 核心系统工程师 |
| [developer](developer.md) | 研发工程师 |
| [multimedia-engineer](multimedia-engineer.md) | 多媒体工程师 |
| [build-engineer](build-engineer.md) | 构建工程师 |
| [ui-engineer](ui-engineer.md) | 用户界面工程师 |
| [visual-designer](visual-designer.md) | 视觉设计师 |
| [video-algorithm-engineer](video-algorithm-engineer.md) | 视频算法工程师 |
| [audio-dsp-engineer](audio-dsp-engineer.md) | 音频 DSP 工程师 |
| [graphics-engineer](graphics-engineer.md) | 实时图形工程师 |
| [release-engineer](release-engineer.md) | 发布工程师 |
| [tester](tester.md) | 测试工程师 |
| [writer](writer.md) | 作者 |
| [social-media-operator](social-media-operator.md) | 社交媒体运营 |
| [reviewer](reviewer.md) | 独立评审者 |
| [yocto-engineer](yocto-engineer.md) | Yocto 工程师 |
| [qemu-engineer](qemu-engineer.md) | QEMU 工程师 |

## 使用与维护

职责和基础技能以各角色文件为准，本索引不重复技能配置。字段与引用规则见 [配置规范](../CONFIG_SCHEMA.md)，身份与技能加载见 [会话协议](../SESSION_PROTOCOL.md)，任务、交接和评审流程见 [项目运行协议](../PROJECT_PROTOCOL.md)。

reviewer 是可选的独立检查角色；日常专业交叉评审使用现有角色。

前后端成员复用 developer；软件与旅行项目经理复用 project-manager；旅行和图书规划成员复用 planner。仅当共同职责、边界或产出确实不同，才考虑新增角色。

visual-designer 的公共基础知识是 visual-design；只有采用 AI 生成角色图片的项目，才通过 `TEAM.roleKnowledge` 按需补充 ai-character-image-consistency。补充知识提供方法，不扩大岗位职责或工具权限。

writer 负责内容创作与修订；social-media-operator 负责内容日历、发布准备、人工发布协作、规则核对以及数据和反馈闭环。运营岗位不因承担发布协作而自动获得账号或发布权限。

共享定义的维护与影响检查遵循 [超级管理员入口](../SUPER_ADMIN.md)。
