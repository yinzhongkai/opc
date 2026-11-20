# 第 4 章配套版本发布与获取核对

- 日期及执行人：2026-11-20，book-yocto / project-manager。
- 关联任务及问题：T-016、PM-C4-01。
- 授权来源：用户了解“配套版本获取”预期后，在本会话明确要求“执行下吧”，授权固定工程标签、核对正文获取步骤及对应文件，并回写结果。
- 结论：工程 `chapter4` 注释标签已发布并从 GitHub 获取核对，书稿与配套工程对应检查通过，PM-C4-01 closed。T-016 保持 in_progress，H-002/H-003 仍待发起人核对关闭。

## 固定版本

| 对象 | 仓库、路径与精确版本 |
|---|---|
| 书稿 | git@github.com:yinzhongkai/book-yocto.git，main@619c4efcd57a08818572f360b61b97abe2462ed0 |
| 第 4 章 | yocto/task05-4-让QEMU长出tiger这块板.md，获批 blob b1311b25af54c4b0f03b5bdada1b193de67b487e，与 D-016 一致 |
| 工程 | git@github.com:yinzhongkai/meta-tiger.git，main@79d9696c1d37fcbf65724eb05a253b3168d2eab7 |
| 工程文件树 | 4d41c859b9ba3c167fdb4f0ab51e7291d04ff929 |
| 注释标签 | chapter4，标签对象 8f3da6ea761049c2fe83e87a4a25bbe0c99d8940，指向上述工程提交 |

发布入口：[meta-tiger / chapter4](https://github.com/yinzhongkai/meta-tiger/tree/chapter4)。本次没有新增工程或书稿提交，两个 main 及 WORKSPACE revision 均不变。

## 获取步骤与发布结果

已批准正文 4.1 已包含以下步骤，继续使用上一章的同一仓库和构建目录，无需改动正文：

```sh
git -C /home/oops/workspace/meta-tiger fetch origin tag chapter4
git -C /home/oops/workspace/meta-tiger switch --detach chapter4
```

正文已提示切换前先提交需保留的修改。读者使用的 origin 对应上述 GitHub 工程仓库；标签固定本章内容，后续 main 演进不改变该版本。

发布前核对本机既有工程副本和 GitHub main 均为上述提交，工作树干净，本机及远端无 chapter4 标签。新建注释标签后，仅推送 refs/tags/chapter4，未强推或替换已有标签。远端查询得到：

```text
79d9696c1d37fcbf65724eb05a253b3168d2eab7  refs/heads/main
8f3da6ea761049c2fe83e87a4a25bbe0c99d8940  refs/tags/chapter4
79d9696c1d37fcbf65724eb05a253b3168d2eab7  refs/tags/chapter4^{}
```

实际执行 `git fetch --no-tags origin tag chapter4` 成功，并再次核对本机标签对象及解引用提交。`--no-tags` 限制自动获取其他标签，显式请求的 chapter4 正常获取。本轮不切换本机或服务器工作树，标签内容通过精确提交读取核对。

## 书稿与工程对应核对

50 项静态检查全部通过，明细见[结构化回执](t016-chapter4-release-20261120.json)。

- 书稿 checkout 为声明的 main，正文 blob 与 D-016 完全一致，两个 GitHub main 与 WORKSPACE 相同。
- 章节目录树要求的 machine、QEMU append/patch、minboot 固件源、Makefile、许可及镜像配方齐全。工程已按前序授权移除 tests/，当前正文采用手工 QEMU 监控器；旧历史中测试脚本的取件要求不再适用于当前正文。
- patch 的 Kconfig、Meson、tiger.c 三文件及四组模型 C 节选对应；完整 main.c、Makefile、启动汇编指令、链接脚本节选及五组 BitBake 节选对应。按片段语义保留正文注释、节选及排版差异，不把讲解中的片段当成完整源码。
- 固件配方的本地 SRC_URI 文件齐全，MIT checksum 一致；MACHINE=tiger、minboot-0.1-tiger 及 minboot-image-tiger.qemuboot.conf 命名、构建和启动命令对应。
- 正文 49 个代码块围栏成对，8 个 bash 块经 bash -n 检查通过。
- 本次标签固定的七份固件、配方和 machine blob 与[既有工程构建回执](remove-tests-minboot-result.json)一致，原回执记录 740 项任务成功及冷启动/两次复位通过。原始运行日志见[监控器记录](manual-monitor-minboot-20261120.log)。本次没有重新执行 BitBake、QEMU、复位或独立审校。

治理校验：`python -B scripts/validate_framework.py` 通过（24 个角色、50 份技能、1 个实际项目、724 处本地链接），`git diff --check` 通过。范围核对确认仅八份本项目治理文件变化，T-016 之外的任务、HANDOFFS、WORKSPACE、公共框架及历史原始证据保持不变；T-016 保持 in_progress。

## 工作区与流程边界

正式工程工作区仍为 WORKSPACE 声明的 `tiger:/home/oops/workspace/meta-tiger`。本次 SSH 尝试返回 connection refused，未核验或改变服务器 checkout，未删除、移动或修改持久 QEMU 源码。

本次使用既有本机同级工程副本 `C:/Users/YinZh/Documents/workspace/meta-tiger` 管理 GitHub 标签引用。其仓库地址、checkout、GitHub main、文件树及构建回执 blob 已核对；未在该副本开发工程内容、克隆新副本或修改 WORKSPACE 声明。标签发布只固定已合入内容，不依赖服务器当前运行状态。

OPC 治理分支为 `feature/book-yocto/t-016-chapter4-release`，从 `project/book-yocto@a7f47470dcd761a3f120b39a046f66d057935ea8` 建立，属于 T-016 的配套发布收尾。原已删除开发分支不恢复。记录经验证后按根分支策略以 --ff-only 合入项目线并普通推送，再安全清理本次治理分支；最终提交由任务记录、来源分支、合入前目标及 Git 历史定位。
