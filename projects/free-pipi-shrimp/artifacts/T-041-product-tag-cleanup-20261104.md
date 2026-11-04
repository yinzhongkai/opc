# T-041 产品库全部标签清理与历史恢复

2026-11-04（Asia/Shanghai）；project-manager；状态completed。

用户明确要求把产品库本地和远程tag全部清理。本轮两端各删除8个归档标签，现均仅main，无标签。main的32条提交、成品和素材保持；清理前全部102条提交已保存为本地自包含Git bundle并独立恢复验证，其中70条非main记录需由该备份读取。

## 仓库与核验

- 本机：`C:\Users\YinZh\Documents\workspace\free-pipi-shrimp`；远端`git@github.com:yinzhongkai/free-pipi-shrimp.git`。
- main／origin/main／WORKSPACE：`5061a76052e19d393988c1932f92912d60c9e745`；tree仍`0faa665195635f3fd8501ecd1cdd75e78a9a50db`。
- 清理前本地／远端标签及其目标逐项一致；清理后`git tag --list`及`git ls-remote --tags origin`均为空，远端heads仅main。本地标签采用显式旧SHA事务删除，远端使用显式旧SHA lease原子推送，未创建替代分支或标签。
- 350个受跟踪文件共162975742字节，逐文件SHA-256及索引摘要前后相同，工作树干净。82个LFS缓存共158163528字节保持，未上传或清退素材。本轮未产生产品提交，OPC仅记录治理。

完整删除标签名单、校验摘要和旧头删除日志分别见[结构化回执](T-041-product-tag-cleanup-20261104.json)及产品本机`.git/tag-cleanup-t041-20261104/`内的`before.json`、`remote-deletion.log`。

## 本地历史备份

- 文件：`C:\Users\YinZh\Documents\workspace\free-pipi-shrimp\.git\tag-cleanup-t041-20261104\before-tags.bundle`。
- 大小：1924100字节，约1.9MB。
- SHA-256：`f4d74eb70c670c7ef280e18be1bde8a3c65b9783ec05e1c5d225d75b27ccd6d2`。
- 自包含备份已在独立镜像中恢复：102条提交集合、main及8个标签名称／目标全部一致，`git fsck --full --no-reflogs`通过。临时验证副本已清理，备份保留在本地，未上传远端。

备份包含清退中间图片后的Git历史，不内嵌LFS图片实体；保留的82个最终／共用图片仍在本地缓存及远端。原始未重写历史的T-039备份也保留。标签清理后，GitHub按分支／标签浏览仅main的32条，另外70条不再有该入口，不能继续称全部102条均能通过远端标签浏览。

需要查看额外历史时，可在另一个目录从bundle创建镜像，不给当前产品库恢复标签：

```powershell
git clone --mirror "C:\Users\YinZh\Documents\workspace\free-pipi-shrimp\.git\tag-cleanup-t041-20261104\before-tags.bundle" "<另一个空目录>"
git -C "<另一个空目录>" log --all --graph --oneline
```

旧成果先用[T-039旧新SHA查询表](T-039-commit-map-20261104.tsv)定位新SHA；若不属于main，再到恢复出的独立镜像查询。T-039／T-040记录反映其当时引用状态，现状按本报告读取。产品内T-039历史说明未在本次引用清理中另行修改。

删除标签不会减少保留的158.2MB LFS素材。本地新增约1.9MB历史备份后，产品约360.3MB；远端是否回收不可达Git对象及其时点不由本次引用删除决定，未执行历史重写或仓库重建。
