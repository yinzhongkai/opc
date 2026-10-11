# 项目成果索引

每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与位置 | 存储位置 | 负责人 | 关联任务 | 版本/精确提交 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|---|
| A-001 | [全书内容与配套资料盘点](book/content-inventory.md)；[扫描证据](book/inventory-evidence.json) | OPC artifacts/book/ | project-manager | [T-001](../TASKS.md) | 0.1；书稿输入 0811b23eb19d79ceb55a3c64a81944312eec0f2c | draft | 尚无 |
| A-002 | [全书审核与修订计划](book/review-plan.md) | OPC artifacts/book/ | project-manager | [T-001、T-002](../TASKS.md) | 0.2；输入 A-001 0.1、A-003 1.0 与 D-005 | draft | 具体标准 / 边界已由 D-005 确认；书稿最终批准尚无 |
| A-003 | [审校标准与配套交付边界](book/review-criteria.md) | OPC artifacts/book/ | project-manager | [T-002](../TASKS.md) | 1.0；获确认草案 0.1 位于 OPC 提交 07e2bdcc23625d2e7f2499d7bdcc7242a50d995f | approved | D-005；2026-10-11 用户采用整体建议，范围仅标准与交付边界 |

OPC 治理与协作成果使用当前项目 `artifacts/` 内的正文链接；产品代码和直接业务交付物使用“成果物仓库 + 仓库内路径”，并记录其最终合并提交。可移动分支名不能代替精确提交。

## 工作区当前输入位置

依据 [D-001 至 D-003](../DECISIONS.md) 和 [WORKSPACE.yaml](../WORKSPACE.yaml)，本节登记管理员配置工作引入的用户原稿与当前导航。具体审核任务、成果 ID、负责人及批准记录由后续获授权成员按项目协议登记。

| 输入 | 仓库 | 仓库内位置 | 精确提交 | 当前事实 |
|---|---|---|---|---|
| 用户原稿 task00–task12，共 13 篇 | `git@github.com:yinzhongkai/book-bitcoin.git` | `bitcoin/`；文件索引 `bitcoin/index.md` | `0811b23eb19d79ceb55a3c64a81944312eec0f2c` | 已推送 main；与原输入逐字节一致，尚未审校或批准 |
| 仓库说明与文件导航 | 同上 | `README.md`、`bitcoin/index.md` | `0811b23eb19d79ceb55a3c64a81944312eec0f2c` | 已推送 main；目录清理说明及导航链接核对通过 |

当前版本已推送并核对远端。初始提交 `b3e147f539c3ba298e6be6fcaf0d16c3e79b7bdd` 中的 `docker/dockerfile` 与 `.gitattributes` 已按 D-002 删除；本地历史整理后早期版本仅在本机 Git 恢复记录中，远端采用上表的现有 main 版本。原稿来源与指纹、默认本机位置及配置授权见 [PROJECT.md](../PROJECT.md)。

## 正文元信息样式（不是真实成果）

```text
项目：<project-id>
成果 ID：A-001
负责人：<member-id>
关联任务：<T-编号>
版本：0.1
更新日期：<实际日期>
状态：draft
适用范围：<本成果覆盖的内容和边界>
来源及输入版本：<授权来源、所依赖成果及版本>
存储位置：<OPC artifacts 路径 | 成果物仓库、仓库内路径与最终合并提交>
批准依据：尚无
版本记录：<日期、版本、主要变化和替代关系>
```

任务完成不自动批准成果。重要交接或批准须引用实际版本及可用的 Git 提交或内容摘要；修改批准内容时更新版本并重新说明评审状态。
