# 项目状态摘要

- 汇总日期与信息截至点：2026-10-22；首版两主线交付完成后，按用户追加要求将本轮产品17条、OPC项目27条提交各压缩为一条单行提交。当前产品main为95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0，已重新验证并核对GitHub；OPC当前提交通过Git历史和远端核对定位。旧交付头、归档标签及压缩映射见[A-015 v0.4](artifacts/git-delivery-handoff.md)。
- 维护人：`project-manager`。
- 当前阶段：原生首版的本轮Git交付完成；AppID配置差异已修正并独立通过，两主线已合入验证推送。正式账号由用户准备，本次未执行微信提审或发布。
- 任务进展：T-001至T-016共16项completed；本次仅按用户要求整理已交付历史，未开始新业务任务。任务完成不代替成果批准。
- 本轮分支：交付主线为产品main和OPC project/eating-everyday，原feature已安全删除；本轮压缩保留更早基线，每仓库新增范围各一条普通提交，无merge commit。旧头先以归档标签和本机bundle保留，再用显式旧SHA的--force-with-lease同步；用户本次明确要求把project.config.json也纳入原提交，已按原内容合入；私有配置继续忽略。
- 关键成果：需求A-001 v0.9、选项A-007 v0.3保持；[A-006 v0.5](artifacts/system-architecture.md)/[A-008 v0.3](artifacts/accounts-access-checklist.md)已解除原B-01至B-04。[A-010 v0.1](artifacts/core-test-plan.md)为测试计划，[A-011 v0.3](artifacts/developer-core-handoff.md)/[A-012 v0.3](artifacts/core-test-report.md)为核心闭环，[A-013 v0.3](artifacts/miniprogram-preview-handoff.md)/[A-014 v0.4](artifacts/miniprogram-test-report.md)为原生/配置开发交接和独立验证，A-015记录Git交付。成果仍draft，三个既有缺陷均独立复测closed。
- 现行范围：全部候选由用户输入或从其已保存菜单加载，首次为空；保留独立等概率转盘、24 项上限、多份命名菜单及本机原样留存。定位、距离、附近搜索、餐厅数据源与林萃桥覆盖已撤销，旧技术失败和未验证事实保留为历史。
- 工作区：WORKSPACE仍为git，repository为git@github.com:yinzhongkai/eating-everyday.git、branch为main、checkout为相对路径../eating-everyday；revision更新为压缩后95173d4eb44f9cf3002ca6e0a546b7c0c46d26c0。GitHub main与本地同SHA，旧8fa87fe基线保留为初始化历史；原交付f642dcad及旧被测SHA保留于归档标签。
- 测试进展与入口：本次纳入配置后，AppID校验同步允许占位符或规范wx AppID，非法值和工程/打包保护继续拒绝；主线与无.git、无私有配置的干净导出分别完整299/299。该结果为project-manager实际收尾验证，不冒充新增独立测试或平台验收。以下为原T-014/T-015历史：配置修正ac9ae935仅三份test源码；开发与独立复测分别在保留本机配置和干净导出各299/299，tester另有85/85独立配置检查及六类错误拒绝验证。产品目标main再跑完整299/299，失败/取消/跳过/todo均0。历史291/292、292/292及平台结果按原时点保留；手机用户自测结论不变，本轮无需GUI或重建二维码。
- 证据限制：手机设备/系统/微信版本及构建指纹未提供，手机结果来自用户自测；真实满配额、多设备、强杀/设备清库及旧平台GUI附件缺口按A-014保留。A-006 v0.5已判原约定范围无剩余技术阻塞，本次Git交付不扩展为全部设备、全部AC独立真机通过或微信正式发布。
- 交接状态：H-002/H-004 已因 D-016 取消，H-003 为 closed；H-001 保持 accepted，待发起人核对。H-005 已由 architect 实际接收并提交设计反馈，仍为 accepted；H-006 已由 project-manager 接收并同步摘要，仍为 accepted，均待发起人 product-manager 核对关闭，不因本次 Git 提交自动关闭。
- 团队与下一步：architect、developer、tester均已提交停止，project-manager仅处理本次提交整理与远端核对。用户准备正式账号，之后按实际发布入口另行安排，无新增手机补测、账号开通或费用确认前置。
- 工作区与边界：治理文档仍在OPC artifacts，代码/测试/中间文件只在声明产品仓库；本机配置公开SHA97f3a135…e2db、私有SHA988e8a38…cff4已验证保留，原始备份及主线TAP仅在产品.local忽略目录。当前测试号AppID已随用户明确授权的project.config.json提交，AppSecret未纳入、私有文件仍忽略；旧数据工作不恢复，不办理账号或发布。

本文件是摘要，原始事实以 [TASKS.md](TASKS.md)、[DECISIONS.md](DECISIONS.md)、[HANDOFFS.md](HANDOFFS.md) 和 [成果索引](artifacts/README.md) 为准。汇总后注明实际信息范围，过期摘要不能覆盖原始记录。
