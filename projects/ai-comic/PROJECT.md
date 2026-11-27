# AI 漫剧创作

项目 ID：`ai-comic`

## 建项信息

- 建立日期：2026-11-27。
- 用户请求来源：建项用户要求以 OPC `v0.0.7` 建立 AI 漫剧创作项目，保留项目成员并使用独立成果物库。
- 当前阶段：筹备。
- 框架基线：`v0.0.7`，精确提交 `91d084606f710740ed21ab5dcfec89f33fa8c4a6`。
- 项目长期分支：`project/ai-comic`。

## 目标与约束

- 目标：根据用户提供的故事制作 AI 漫剧，组织内容需求、剧本、分镜、关键帧、视频制作和验收。
- 范围与非目标：导演将指定故事拆解为分镜和关键帧说明，视觉设计师按需制作帧图，视频制作成员通过可灵 MCP 生成镜头并完成剪辑。具体选题和试作范围待用户确认。
- 交付物与验收要求：项目资料、成员与工作区配置须通过框架校验。作品时长、画幅、交付规格及验收标准待确认。
- 时间、资源与其他约束：未设定。
- 成果物工作区：独立 Git 仓库 `git@github.com:yinzhongkai/ai-comic.git`，本机 checkout 为 `../ai-comic`。目标分支为 `main`，精确版本见 [WORKSPACE.yaml](WORKSPACE.yaml)。

未知项保持待确认，已确认的范围变更链接 [DECISIONS.md](DECISIONS.md) 中的决定，不从模板预设业务任务。

## 确认与记录责任

- 默认最终确认人：建项用户，以本文件记录的请求来源识别；用户可明确指定其他确认人。
- 协调记录维护人：`project-manager`。
- 框架与成员配置维护：由根目录超级管理员入口负责，不属于本项目成员或长期协调记录职责。
- 计划协调：`project-manager` 在已确认范围内协调成员、任务和依赖，超出已确认范围或资源约束的变更交用户确认。
- 专业职责与成员分工：以 [TEAM.yaml](TEAM.yaml) 登记的[成员文件](members/README.md)和共享角色为准；记录整理不授予其他角色职责。

## 交付约定

- 需求、方案、计划、评审、验收等治理与协作成果位于 OPC 的 `artifacts/`；产品代码、可直接交付的业务内容及其中间文件位于 [WORKSPACE.yaml](WORKSPACE.yaml) 声明的成果物仓库。默认 `none` 不创建工作目录；独立产品源码优先使用外部 git。`artifacts/README.md` 同时索引两类成果，并为工作区成果记录仓库、路径和精确版本。声明本身不授权联网或修改外部仓库。
- 用户保证当前仓库每次只有一个会话执行，前一会话完成或停止后再启动下一会话；框架不实现文件锁或自动调度。
- 每份文档默认维护一个当前版本，记录版本及批准依据；历史通过用户维护的 Git 追溯。
- 评审与批准要求在任务中明确。专业交叉评审由现有成员在原会话执行，不默认新建评审会话；用户保证受评版本可读取且不变，本轮全部评审者提交意见后作者再统一修订。采用根 [项目运行协议](../../PROJECT_PROTOCOL.md) 的评审、完成与状态规则。
- 默认在相关任务内记录轻量评审，需独立跟踪或正式报告时再拆分；具体已确认的验收要求不能自行降低。

## 成员配置记录

- 2026-11-27，框架超级管理员依据建项请求登记项目经理 [project-manager](members/project-manager.yaml)、产品经理 [product-manager](members/product-manager.yaml)、编剧 [writer](members/writer.yaml)、导演 [director](members/director.yaml)、视觉设计师 [visual-designer](members/visual-designer.yaml) 和视频制作与后期 [video-producer](members/video-producer.yaml)。具体分工及项目技能以成员文件和 [TEAM.yaml](TEAM.yaml) 为准。

## 当前资料

成员见 [TEAM.yaml](TEAM.yaml)，任务见 [TASKS.md](TASKS.md)，摘要见 [STATUS.md](STATUS.md)，交接见 [HANDOFFS.md](HANDOFFS.md)，成果见 [索引](artifacts/README.md)。
