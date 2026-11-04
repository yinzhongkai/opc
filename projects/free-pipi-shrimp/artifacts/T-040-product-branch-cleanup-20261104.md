# T-040 产品库分支清理与历史保全

2026-11-04（Asia/Shanghai）；project-manager；状态completed。

用户要求产品库“只保留主分支”。本次已删除本地3个及远端2个非main分支，本地与GitHub均只保留main。8个现有历史标签及全部102条提交保留，无需下载历史bundle即可浏览。

## 仓库与固定版本

- 本机：`C:\Users\YinZh\Documents\workspace\free-pipi-shrimp`。
- 远端：`git@github.com:yinzhongkai/free-pipi-shrimp.git`。
- main／origin/main／WORKSPACE保持`5061a76052e19d393988c1932f92912d60c9e745`，main仍32条提交。
- Git tree保持`0faa665195635f3fd8501ecd1cdd75e78a9a50db`，索引摘要未变，工作树干净。未产生产品内容提交，最终图文及发布记录保持。

## 删除的分支

| 分支 | 删除前提交 | 本地 | 远端 |
| --- | --- | --- | --- |
| feature/t-023-station02-first-two-visual-assets | 9d175ee41410b2abfa62abceb31217a4f2b6abc1 | 已删除 | 已删除 |
| archive/t-038-full-history-main-20261104 | 9d175ee41410b2abfa62abceb31217a4f2b6abc1 | 已删除 | 原本不存在 |
| archive/local-before-sync-20261019/main-104b533 | 66b61c3671b5fef117255751a1cf93c149899b4a | 已删除 | 已删除 |

前两分支均已合入main，本地使用`git branch -d`。最后一个历史归档分支尚非main祖先，但其全部历史已由本地／远端现有标签`archive/t-038-history-before-rebuild-20261104/d39a76cb1177`覆盖，标签仍指向`6c1538942bdeb0110240d676a4a5904e9fdf2262`；核对后使用显式旧SHA条件删除归档引用。

远端两分支采用显式旧头lease和原子推送删除，随后清理失效跟踪引用。最终实时`git ls-remote --heads origin`只返回main；本地仅main，origin跟踪仅main及HEAD。

## 历史保全核验

清理前全部102个可达提交，与main加现有8个标签的完整提交集合逐项一致；清理后再次比较无遗漏、无新增，8个标签名称及目标均未变化。提交集合的SHA-256为`8fdb6a0d57086cce442b31cca3e499b480d7fc03e667c52d6b37430c2ba2119c`。

历史读取仍采用[T-039旧新SHA查询表](T-039-commit-map-20261104.tsv)，可在[GitHub主线提交](https://github.com/yinzhongkai/free-pipi-shrimp/commits/main/)及保留的历史标签中浏览，也可本地执行`git log --all --graph --oneline`。本次只删除分支引用，未改写提交或标签，也未改变素材集合。

[结构化回执](T-040-product-branch-cleanup-20261104.json)保存实际核验结果。产品本机`.git/branch-cleanup-t040-20261104/`下保存清理前提交集合、索引摘要、远端删除日志及结果。OPC自身分支不在本次产品清理范围内。
