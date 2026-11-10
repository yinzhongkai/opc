# 项目成果索引

暂无已登记业务成果。每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与位置 | 存储位置 | 负责人 | 关联任务 | 版本/精确提交 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|---|

OPC 治理与协作成果使用当前项目 `artifacts/` 内的正文链接；产品代码和直接业务交付物使用“成果物仓库 + 仓库内路径”，并记录其合入后的最终提交。快进合并后可直接引用通过最终验证的普通功能提交，可移动分支名不能代替精确提交。

## 成果物工作区基线

- 基础设施配置依据：[D-001](../DECISIONS.md#d-001配置独立-git-成果物工作区) 与 [WORKSPACE.yaml](../WORKSPACE.yaml)，由框架超级管理员按用户授权登记。
- 位置：本机 `C:/Users/YinZh/Documents/workspace/ai-comic`，相对 OPC 根目录的 checkout 为 `../ai-comic`；仓库为 `git@github.com:yinzhongkai/ai-comic.git`，目标分支 `main`。
- 精确提交：`787f5af54367f13c8776fa1279252d24dbee0d12`；[仓库 README](https://github.com/yinzhongkai/ai-comic/blob/787f5af54367f13c8776fa1279252d24dbee0d12/README.md) 记录目录用途与 Git LFS 使用方法。本机与远端该提交已核对，当前仅有基础配置和占位目录。
- 路径：`scripts/`、`storyboards/`、`assets/characters/`、`assets/scenes/`、`assets/props/`、`images/`、`audio/`、`video/`、`records/generation/`、`source-projects/`。后续业务成果按上表逐项登记负责人、任务、仓库内路径及合入后精确版本。

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
存储位置：<OPC artifacts 路径 | 成果物仓库、仓库内路径与合入后的最终提交>
批准依据：尚无
版本记录：<日期、版本、主要变化和替代关系>
```

任务完成不自动批准成果。重要交接或批准须引用实际版本及可用的 Git 提交或内容摘要；修改批准内容时更新版本并重新说明评审状态。
