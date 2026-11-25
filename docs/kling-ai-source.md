# 可灵创作知识：来源、阅读覆盖与角色使用

## 请求与维护范围

用户在本超级管理员会话提供[可灵 AI 使用指南](https://docs.qingque.cn/d/home/eZQBGvsEtlGCz5-IhhQP3S-eM)，要求在 Codex 中阅读文档及各链接，形成供后续 AI 视频制作使用的知识和角色支持。本轮于 2026-11-25 开始，以下记录截至 2026-11-26。

本次是当前 OPC 克隆的共享定义维护。来源分支 `feature/framework/kling-video-knowledge`，目标 `opc-v0.0.6-dev`，合入前基线 `fb161ec0915efabe3f601b2e56c2b828205359fb`。当前克隆没有实际 `projects/` 目录；框架知识写在根目录，不涉及产品工作区。

## 读取方式与证据边界

在 Codex 内置浏览器读取目录、渲染正文和目录链接；对视频 4.0、3.0、3.0 Omni、O1、2.6 使用逐屏读取，对其他长文按相关章节读取。部分读取中断和超时后，辅以网页读取核对可访问的正文。网页抓取只返回开头或报错时，不能据此认定全文已读。

入口共发现 **25 个指南/功能链接，去重为 13 篇指南**；视频指南另引出主体库 3.0。因此清单包含入口页、13 篇主指南及 1 篇关联主体库指南。正文中的邮箱、站点首页、社群、GitHub、HuggingFace 和图片/视频素材不是新增模型指南；不将它们无限递归抓取。2.6 内的部分场景跳转指向企业内部文档，本次直接阅读公开页中的对应正文，没有进入企业内部站点。

正文已读不代表完成案例复现。没有逐一播放嵌入音视频，也没有逐张检查全部图库、界面截图或小字；没有上传素材、连接可灵账号、提交生成或消费灵感值。以下知识不使用官方“始终一致”等宣传语作质量保证。

## 来源清单与实际覆盖

“核心已读”限表内明确列出的内容，不是整篇及其全部媒体均已检查；未通读项保留为缺口。

| 编号 | 官方来源 | 实际覆盖与限制 |
|---|---|---|
| S00 | [可灵 AI 使用指南](https://docs.qingque.cn/d/home/eZQBGvsEtlGCz5-IhhQP3S-eM) | 目录正文和 25 个目标地址已读取 |
| S01 | [Kling Image 4.0](https://docs.qingque.cn/d/home/eZQDsQeh8FByxBZbkC6EPOt6v) | 更新说明、核心参数和单图/组图开头已读；后续创作案例与提示词章节未通读 |
| S02 | [Kling 4.0](https://docs.qingque.cn/d/home/eZQBwtYbPcQQkDUf3zQ56oqeW) | 概览、参数、Flash、入口、提示词、全能参考、编辑、声音、文字、关键帧与 FAQ 已读至文末；媒体未逐一回放 |
| S03 | [视频 3.0](https://docs.qingque.cn/d/home/eZQCqDGoymg61UKgMckSB2oMh) | 升级、多镜头、主体绑定、语言音色、文字、长镜头、计费说明已读至文末；不以所列旧价报价 |
| S04 | [视频 3.0 Omni](https://docs.qingque.cn/d/home/eZQDPQ5RCKYKpTbz1poE88YSp) | 参考、角色主体、声音、自定义分镜、输入限制与 FAQ 已读至文末；计费表与概览口径不一致处不自行外推 |
| S05 | [视频 O1](https://docs.qingque.cn/d/home/eZQCg5xHvxDaE-jP2GcnaNc6O) | 核心技能中的参考、增加/删除/修改、风格、前后镜头、动作运镜、首尾帧、文生视频、输入与计费已读；开篇亮点与全部应用示例未完整覆盖 |
| S06 | [视频 2.6](https://docs.qingque.cn/d/home/eZQBMUXCmLjb57bpfsVk2jNvx) | 概览、场景示例、提示词、多人对白、音色、费用及 FAQ 分段读至文末；媒体未回放 |
| S07 | [基础视频生成](https://docs.qingque.cn/d/home/eZQDKi7uTmtUr3iXnALzw6vxp) | 文生视频公式、图生视频方法、多图参考、首尾帧、运镜的版本条件与对口型正文已读；运动笔刷/场景应用及部分示例未通读。长文读取多次超时后按章节补读 |
| S08 | [数字角色 2.0](https://docs.qingque.cn/d/home/eZQCNHbAH5WUzp1SCYw0uTUcQ) | 概览、目录、角色图来源、配音、表演描述、单人提示和计费段落已读；部分输入规格及示例未完整覆盖 |
| S09 | [动作控制](https://docs.qingque.cn/d/home/eZQAl5y8xNSkr0iYUS8-bpGvP) | 网页读取到 3.0 升级、人物图与动作视频、面部主体、朝向限制和面部参考建议；未复现功能，旧版后续内容未完整核验 |
| S10 | [图片 3.0](https://docs.qingque.cn/d/home/eZQBLMR-3rTRKlMTF9U14ptYa) | 网页读取到概览、多图参考、身份保持和细节编辑示例；后续章节未通读 |
| S11 | [图片 3.0 Omni](https://docs.qingque.cn/d/home/eZQAit38w9iv2rOZ3k0CXYYn-) | 浏览器读取概览、组图入口、叙事及非叙事组图提示词，网页交叉核对概览；中间图库与其他应用章节未通读 |
| S12 | [图片 O1](https://docs.qingque.cn/d/home/eZQAOaXS_vSJtC2ykMjNfYSaa) | 浏览器读取概览、参考能力及编辑/人脸/方位提示词；影视等案例及后续词表未通读 |
| S13 | [基础图片生成](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD) | 文生图方法、角色特征、人物长相、风格转绘、通用垫图、多图主体注意事项、局部重绘、扩图、AI 模特操作与提示已读；部分案例、截图和参数未全部核对 |
| S14 | [主体库 3.0](https://docs.qingque.cn/d/home/eZQCXlb985uYAZ-c8NgyTv11X) | 浏览器读取概览、复用范围、视频角色创建和部分多图/首帧示例；主体类型细则和 FAQ 未通读 |

## 入口的 25 个链接定位

下面保留入口中的功能名及锚点，以便补读时直接定位。同页多个锚点不重复算独立文档。“首尾帧能力”和“多图参考”在原目录指向相同锚点，阅读首尾帧时另使用了正文目录定位。

| 序号 | 入口链接 | 来源编号 |
|---|---|---|
| 1 | [Kling Image 4.0](https://docs.qingque.cn/d/home/eZQDsQeh8FByxBZbkC6EPOt6v) | S01 |
| 2 | [Kling 4.0](https://docs.qingque.cn/d/home/eZQBwtYbPcQQkDUf3zQ56oqeW) | S02 |
| 3 | [视频 3.0](https://docs.qingque.cn/d/home/eZQCqDGoymg61UKgMckSB2oMh) | S03 |
| 4 | [视频 3.0 Omni](https://docs.qingque.cn/d/home/eZQDPQ5RCKYKpTbz1poE88YSp) | S04 |
| 5 | [视频 O1](https://docs.qingque.cn/d/home/eZQCg5xHvxDaE-jP2GcnaNc6O#section=h.dyawluwtx2d9) | S05 |
| 6 | [视频 2.6](https://docs.qingque.cn/d/home/eZQBMUXCmLjb57bpfsVk2jNvx) | S06 |
| 7 | [文生视频](https://docs.qingque.cn/d/home/eZQDKi7uTmtUr3iXnALzw6vxp#section=h.odox0hgboutk) | S07 |
| 8 | [图生视频](https://docs.qingque.cn/d/home/eZQDKi7uTmtUr3iXnALzw6vxp#section=h.xlh3s431laj1) | S07 |
| 9 | [多图参考](https://docs.qingque.cn/d/home/eZQDKi7uTmtUr3iXnALzw6vxp#section=h.ou68xj7jobn1) | S07 |
| 10 | [首尾帧能力](https://docs.qingque.cn/d/home/eZQDKi7uTmtUr3iXnALzw6vxp#section=h.ou68xj7jobn1) | S07 |
| 11 | [数字角色 2.0](https://docs.qingque.cn/d/home/eZQCNHbAH5WUzp1SCYw0uTUcQ#section=h.yymwhetkx2ld) | S08 |
| 12 | [动作控制](https://docs.qingque.cn/d/home/eZQAl5y8xNSkr0iYUS8-bpGvP#section=h.bh1md16n3e2l) | S09 |
| 13 | [对口型](https://docs.qingque.cn/d/home/eZQDKi7uTmtUr3iXnALzw6vxp#section=h.k5f06yqdz8gt) | S07 |
| 14 | [运镜控制](https://docs.qingque.cn/d/home/eZQDKi7uTmtUr3iXnALzw6vxp#section=h.f472l774twz6) | S07 |
| 15 | [图片 3.0](https://docs.qingque.cn/d/home/eZQBLMR-3rTRKlMTF9U14ptYa) | S10 |
| 16 | [图片 3.0 Omni](https://docs.qingque.cn/d/home/eZQAit38w9iv2rOZ3k0CXYYn-) | S11 |
| 17 | [图片 O1](https://docs.qingque.cn/d/home/eZQAOaXS_vSJtC2ykMjNfYSaa) | S12 |
| 18 | [文生图](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.n9d5hcmfp6ed) | S13 |
| 19 | [角色特征参考](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.7jfxmbvnlv3s) | S13 |
| 20 | [人物长相参考](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.u175rtxa5oub) | S13 |
| 21 | [风格转绘](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.dso79n8hmo4l) | S13 |
| 22 | [通用垫图](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.pfrxa6vh4kux) | S13 |
| 23 | [多图参考](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.k1d7z2mb2y4o) | S13 |
| 24 | [图片编辑](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.thjo6aesrzkb) | S13 |
| 25 | [AI 模特](https://docs.qingque.cn/d/home/eZQApd0EZqQHWXBEi7lL16_lD#section=h.l8danmk2iyee) | S13 |

原链接中的 `identityId`、`via` 为访问参数，引用清单保留文档 ID 和章节锚点；这不代表对所有账号的可访问性作保证。

## 已形成的知识与角色使用

| 工作 | 共享角色 | 建议项目补充技能 |
|---|---|---|
| 选创作方式、拆镜、设计表演与声音意图、评审样片 | [导演](../roles/director.md) | [模型与入口选择](../knowledge/kling-model-selection.md)、[分镜与提示词](../knowledge/kling-storyboard-prompts.md) |
| 设计人物/道具/场景、准备关键帧和一致性参考 | [视觉设计师](../roles/visual-designer.md) | [主体与关键帧素材](../knowledge/kling-reference-assets.md)、[AI 角色图片一致性](../knowledge/ai-character-image-consistency.md) |
| 整理生成输入、镜头制作、声音、剪辑、检查与返工 | [视频制作与后期](../roles/video-producer.md) | [模型与入口选择](../knowledge/kling-model-selection.md)、[分镜与提示词](../knowledge/kling-storyboard-prompts.md)、[主体与关键帧素材](../knowledge/kling-reference-assets.md)、[制作检查与迭代](../knowledge/kling-production-iteration.md) |

当前已有三个角色的共同职责足以承接创作链，故复用它们并补充可灵方法导航，不新增一个职责重叠的“可灵导演”或“提示词工程师”。角色基础技能继续保持通用。项目实际采用时，由获授权的配置维护者写入 `TEAM.yaml.roleKnowledge`；这里只提供组合建议，不创建项目、成员或平台会话，不代表已有成员已加载。

先用一句话明确作品目的、时长、画幅和已有素材，再由相应成员在其项目与 scope 内承接。只写提示词、整理最终设置、提交生成和检查已有任务是不同工作，按实际请求停止在对应交付点。

## 待补读与待验证

目录中的全部入口已有概览或相关功能正文覆盖，已形成可用的核心视频创作知识，但**尚未完成全部链接目标的全文阅读**。进一步精读的缺口包括 S09 后续动作控制细则、S01 和 S10–S12 的剩余案例、S08/S14 的规格和 FAQ，以及 S07 的非目录入口章节和 S13 的其余示例。已读范围不能随工具自动恢复而自动扩大。

实际创作前还须核对账号可用模型与实时 schema、上传规格、报价、版权使用范围及代表性样片效果。公开文档和已安装的 Kling AI 插件技能共同提供方法；产品页面与插件能力不互相保证。此处未将本机插件路径写成共享依赖，也未复制其 OAuth 或接口契约。

## 维护验证

作者自查范围：来源覆盖声明、版本差异、四项知识之间的分工及三个角色的导航。2026-11-26 运行 `python -B scripts/validate_framework.py` 通过：25 个角色、57 份技能、0 个实际项目、1 套模板、466 处本地链接；`git diff --check` 通过。这不构成独立成员评审，也不构成模型效果实测或用户批准。最终合入与提交以本次会话结果及 Git 历史为准。
