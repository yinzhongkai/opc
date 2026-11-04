# T-036 首站发布后独占工程归档与本地清退

项目free-pipi-shrimp；负责人project-manager；2026-11-04（Asia/Shanghai）；状态completed。

## 用户确认及固定版本

用户确认首站内容已经发布，并明确接受“保留5篇最终图文、40图及发布记录，共用头像／封面模板；首站独占背景、参考图及旧工程归档后清退”的方案，D-020 confirmed。该发布情况按用户直接反馈记录，未新增平台回采、稳定ID或精确公开时间。

产品git@github.com:yinzhongkai/free-pipi-shrimp.git，本机C:/Users/YinZh/Documents/workspace/free-pipi-shrimp，main／WORKSPACE保持771c9d2be5427cfa59decfed53acf26dffa93066，Git tree 69c5a1f5a2c048bf850e7a46ab09526fa0218634不变；本轮无产品内容提交。归档标签archive/station01-editing-assets-20261104先创建并推送，ls-remote回读标签对象与本地一致，解引用为上述main，成功后才清退本地文件。

[完整归档计划和保留快照](T-036-station01-archive-plan-20261104.json)与[实际验证结果](T-036-station01-archive-result-20261104.json)记录路径、大小／SHA、Git引用和对象集合、本地检出规则及容量。

## 实际范围与保留内容

- 使用Git本地非锥形sparse-checkout，从当前工作目录移出T-017首站14份背景／参考图片及24份SVG源工程，共38个受跟踪文件。仅改变本机检出范围，Git中的全部664条文件记录及tree／索引对象不变，源工程仍在main与归档标签中。
- 删除T-012旧目录中13份ignored背景工作副本，大小45109861字节；其原Git路径／指针来自ef5399b0ec6dc89bc657a65968346f62ad3e4434（main祖先），本机LFS对象逐一SHA核对完整，归档缓存继续保留。未使用旧D盘归档假设。
- 合计51个本地工作文件82189376字节；仅38个文件的本地检出被关闭，13个冗余工作副本被删除。手工删除用同一PowerShell逐文件LiteralPath，先验证绝对路径在授权目录、无链接越界、ignored状态、大小／SHA和完整缓存副本，只移除空父目录。
- 完整保留首站40张发布原图、54748708字节：前两篇16张与T-011实际定时输入哈希一致，后三篇24张与已交付包一致。最终文字、发布记录和所有其他未排除跟踪文件均原字节保持。
- 共用角色头像、2份封面相关PNG／2份SVG模板及第二站全部工作文件保持；扫描第二站SVG未发现对本次归档目录的图像依赖。

## 校验与容量

- main／Git tree和完整索引mode／object摘要不变；原99个Git提交和全部旧对象／引用仍在，仅新增上述归档标签。git fsck --full --no-dangling实际通过，产品工作树干净。
- 保留的626个受跟踪工作文件逐项大小／SHA保持；当前165个图片路径、59份SVG及55项工作区图像依赖实际核验通过；另40张首站最终图逐项验证。
- tools/verify_asset_retention.py --git-only实际通过：Git层仍有179图片路径／125唯一素材289094702字节、83 SVG和79依赖。本机38工程已按规则归档不检出，未冒称179路径当前全部实体在工作区，未运行会要求归档工程当前存在的旧完整工作区检查。
- 原245个LFS缓存对象保持，无新增缓存，13首站旧背景归档实体完整保留；14个归档T-017图片的远端可恢复性已由本日T-035的逐对象远端验证回执证明。本轮未进一步删除历史缓存或远端素材。
- 产品目录1012087483→929899093字节，实际净释放82188390字节，约82.2MB；清理后约929.9MB（约0.87GiB）。差额包含Git标签、索引与检出规则的轻量元数据。

## 恢复方法

当前main仍包含全部38个跟踪工程，执行以下命令即可恢复正常完整检出，LFS缓存缺失时Git会按现有认证方式从远端取回已核验素材：

```powershell
git -C C:/Users/YinZh/Documents/workspace/free-pipi-shrimp sparse-checkout disable
git -C C:/Users/YinZh/Documents/workspace/free-pipi-shrimp lfs checkout
```

13份旧背景不在当前main中，需要按计划JSON的ignored_backgrounds_to_remove记录恢复：对应原Git版本为ef5399b0ec6dc89bc657a65968346f62ad3e4434、原路径在original_historic_path，SHA及工作副本路径明确保存。完整数据保留于本机.git/lfs/objects/<SHA前2位>/<第3至4位>/<SHA>，复制到清单原工作副本路径并核对SHA即可。不要将本地检出范围调整误认为Git历史删除。

本地规则只排除T-017的assets目录和article-03／04／05各自source目录，其余跟踪文件保持检出。需要再次做全工作区实体检查时，先恢复上述归档工程；保持归档状态时使用Git层检查与保留文件实际核验。

OPC记录在原feature保存并同步；本轮不改变WORKSPACE、角色分工、内容采用／专业评审或第二站发布安排，不触发账号操作。
