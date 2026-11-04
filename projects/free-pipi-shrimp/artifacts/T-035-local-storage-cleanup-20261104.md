# T-035 本地旧稿与LFS缓存清理记录

项目free-pipi-shrimp；负责人project-manager；2026-11-04（Asia/Shanghai）；状态completed。

## 授权、版本与范围

用户明确要求“把不用的旧稿、过程文件和LFS缓存清理一下，保留正式成品、必要的编辑素材和Git历史”。本次仅清理本机C:/Users/YinZh/Documents/workspace/free-pipi-shrimp的ignored旧稿／过程材料与可恢复LFS缓存，不变更产品受跟踪内容，不新开业务开发轮次，不改写或删减Git历史。

产品main／WORKSPACE保持771c9d2be5427cfa59decfed53acf26dffa93066，Git tree保持69c5a1f5a2c048bf850e7a46ab09526fa0218634，功能分支、归档标签、远程跟踪引用和全部Git对象保留。OPC仍在现有feature/free-pipi-shrimp/t-023-station02-first-two-visual-assets保存本轮治理记录。

[逐文件计划及保护快照](T-035-local-cleanup-plan-20261104.json)记录清理前全部受跟踪文件大小／SHA、额外保留素材、删除文件名单及Git引用／对象目录摘要；[实际结果](T-035-local-cleanup-result-20261104.json)记录实际删除的79个LFS对象与全部验证。

## 保护范围

- 全部664受跟踪文件原字节保持，包括正式成品、制作代码、配置、文字／出处／审校／生成记录、83份现用SVG及79项图像依赖。当前179个受跟踪图片路径／125个不同素材共289094702唯一字节校验通过，工作区成品未重压或重绘。
- 核对T-011实际定时接收包的16张首站前两篇原图SHA，确认旧T-006目录中的成品与之相同，额外保留全部16张原尺寸图；同时保留13份必要背景，合计29个ignored文件75489056字节。不能仅以ignored判断无用。
- 首站额外保护素材分别位于deliverables/T-006-first-two-buffer-plogs/v0.1-typeset-draft/与deliverables/T-012-first-two-visual-assets/v0.2-draft/；具体路径／SHA见保护清单。相关历史SVG仍可从Git原版本读取，未删除其Git对象。
- 所有本地Git对象（含不可达旧提交）目录摘要不变，99个提交保留；refs、标签、分支、reflog不删，不运行gc。旧记录中的D盘素材归档当前不存在，本次未把它当作可用备份依据。

## 实际清理结果

| 类型 | 文件／对象数 | 删除字节 |
|---|---:|---:|
| ignored旧稿、QA检查图、原始过程副本、node_modules及过期进程记录 | 634 | 299127889 |
| 远端逐对象核验可恢复的LFS本地缓存 | 79 | 124309534 |
| 已结束传输的incomplete文件及旧进度日志 | 17 | 16789411 |

另仅删除94个空父目录。所有手工删除先核对解析后绝对路径在授权仓库内、没有符号链接／junction越界、文件大小及SHA仍匹配清单，使用同一PowerShell内的LiteralPath逐文件删除；空目录只作非递归删除。

LFS使用官方prune，force仅允许回收当前素材的本地重复缓存，工作区原素材仍保留；同时开启verify-remote、verify-unreachable和when-unverified=continue。正式运行报告324本地对象、245保留、79通过远端验证，并实际删除79对象；未删除未推送／受历史保护对象。实际前后缓存集合与大小独立复核，245个剩余对象均再核对内容SHA等于对象名。

剩余本地LFS缓存245个对象488851796字节，按历史／未推送保护规则保留；不以清空缓存为目标破坏本地历史恢复能力。此次没有上传旧素材、删除远端LFS对象或改变远端仓库。

## 容量变化与校验

- 产品目录总文件字节：1452304202→1012087483，净减少440216719字节，约440.2MB；约1.45GB→1.01GB（清理后约0.94GiB）。净值包含本次少量轻量日志开销。
- 清理后deliverables为516881625字节、.git为491806576字节、work为2637891字节；其余清单／说明／工具不足1MB。
- 664受跟踪文件及29额外保护素材的实际字节／SHA与清理前相同；HEAD、tree、索引mode／对象摘要、refs及全部Git对象目录摘要不变，产品工作树干净。
- git fsck --full --no-dangling实际通过；完整tools/verify_asset_retention.py实际通过：179图片路径、125唯一素材／289094702字节、83当前SVG／79依赖，working_bytes_checked=true。
- 634个旧过程文件及17个旧传输文件均确认不存在，79个删除缓存与官方工具回执一致；Git历史无需恢复。node_modules制作依赖已移除，受跟踪配置／代码保持，需要重新制作时可按配置重装。

本轮不改变图文采用、独立评审、T-032／T-033发布条件、成员scope、WORKSPACE或平台状态；OPC旧temp不处理。治理清单与本记录在原OPC feature以单行message保存并同步，不产生产品代码提交。
