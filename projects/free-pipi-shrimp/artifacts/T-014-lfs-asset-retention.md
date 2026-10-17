# T-014：过程图清退与 LFS 素材保留

- 日期：2026-10-18，Asia/Shanghai；执行与协调记录：project-manager。
- 用户授权：在已说明保留最终图、头像、封面模板及SVG依赖、备份后清退过程图的方案后，用户明确“按这个方案搞一下吧，然后把远程也同步下”。这是既有仓库存储维护，不新增业务开发轮次、不推进T-011状态。
- 成果物仓库：`git@github.com:yinzhongkai/free-pipi-shrimp.git`；旧 main `5313c0d67854b02316c47854c55039009adc63e7`、旧T-011 `4501fad6c07030b885d1a8d107499c049cfebdcc`；新 main `ee79bd840a361e806cb0371c603a40dafdd3fb2d`、新T-011 `696ac1d4c372101da875b3297e2431395d042809`。T-011继续独立保留，未合入。
- OPC原项目头 `372a12226f1277a39d992299a673157cf65b67b0`、原T-011 `a12bb0c79b551581a64db7eb982dbd01b2c92b70`；在更新当前引用后迁移原16条治理提交，保留原说明、专业记录与任务状态。最终维护提交以Git历史及执行回执定位。

## 素材集合

| 范围 | 图片路径数 | 唯一LFS对象 | 唯一字节数 |
|---|---:|---:|---:|
| 整理前 | 253 | 162 | 269324901 |
| 保留当前成品及工程依赖 | 33 | 31 | 80802395 |
| 清退并独立备份 | 220 | 131 | 188522506 |

- 保留：两篇成品16PNG及note、已批准头像、当前封面模板、当前16页SVG及其背景依赖；33路径中的2份相同素材复用同一对象，SVG复用/封面关系不改。18份当前SVG的全部相对图像依赖仍存在。
- 原图字节、路径、SHA-256、最终文字与批准范围保持；没有重绘、有损压缩或替专业角色新增评审结论。
- 原始生成、试错/淘汰图、旧版本位图、旧导出、QA缩略图及拼图从当前树及两条分支的可达历史移出。提示词、工具/参数、来源、评审/自查记录、原manifests和小体积SVG历史快照保留。
- 老清单和旧SVG里的图片引用属于历史来源；完整旧版本应在恢复仓库读取，不把它们的旧文件数/检查结果当作精简版当前检查。当前可编辑入口和后续入库规则以产品 `ASSET_STORAGE.md`、`manifests/asset-retention-20261018.json` 为准。
- `.gitignore` 防止过程位图、QA图和本次清退路径误入库；新增成品/必要依赖需更新保留清单并运行 `python tools/verify_asset_retention.py`。每篇2–4MB只是后续压缩验证目标，本次成品不改。

## 完整备份和恢复

本机目录：`D:\free-pipi-shrimp\.git\asset-archives\asset-retention-20261018-190dc16b`。

- `product-before.bundle`：原成果物main和T-011全部Git对象与历史；`opc-before.bundle`：原项目主线和T-011治理历史。两个bundle均在改写前验证。
- `lfs-objects.zip`：162个唯一素材、269324901原始字节，逐项大小/SHA-256全部核对；归档实际大小265697451字节，SHA-256 `775db3b7027b058fac9ba0526d9733d8996c0eaafa4f4632bb565f3630b731e3`。
- `README-RESTORE.txt`：恢复步骤；`plan.json`：原头、新头、保留/清退清单和原始文件哈希。备份在.git下，仅在本机保留，不上传GitHub/LFS。
- 远程旧对象被清除后，不能依赖GitHub恢复旧版资产；历史来源应由此完整恢复包读取和验证。

## 验证及远程边界

- 33个保留图像路径的原始Git指针及实体哈希、31个唯一对象总量、18份当前SVG依赖、未清退的专业/来源文本保持；两个分支全部可达历史均无清退对象的LFS指针，message一行、零merge commit。框架/链接、测试及实际远端核对见本次执行回执。
- 推送使用主线及T-011各自显式旧SHA保护，两条引用原子更新；其他项目、分支、标签和权限不变。产品先同步并验证，再同步OPC引用。
- 后续处理：GitHub Support工单[#4870933](https://help.github.com/ticket/personal/0/4870933)已提交，但因账号仅提供自助支持而关闭。2026-10-18用户明确要求删除重建并在最终按钮处确认，远端已删除并按原名称重建为私有仓库；仅上传31个保留对象，并从空缓存重新下载、逐项核对大小/SHA-256通过。重建恢复时main和T-011原SHA保持，GitHub后台显示LFS存储`0.1 GB used / 10 GB included`（账户汇总舍入显示），131个旧对象未重新上传。
- 精确OID清单仍在本机 `support-cleanup-request.txt`；已校验的当前Git恢复包为 `product-cleaned-before-recreate.bundle`，重建和后台用量回执为 `recreate-receipt.json`，均位于上列完整备份目录。仓库元数据已核查，无Issues、PR、Stars、Forks、Releases、协作者、部署密钥、Webhook或Actions记录需要迁移。

## 原成果物功能提交映射

| 原提交 | 精简后提交 |
|---|---|
| `4d023fdcb6190d710512e98d7c03c790c715b234` | `4d023fdcb6190d710512e98d7c03c790c715b234` |
| `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` | `7b227f4a3354df50c9c8c80ef48dbbf2c348eea8` |
| `9f5cb8e8a50a161583bc72b5f321e4875aed6fc0` | `147224e430061be3915c1df070c2903eaf888560` |
| `5404a33758b83fd5aff00ced55a4d893bf700c08` | `d7070010b3684795aa3906f3b01934c2a6d5b6e1` |
| `b915bd69586cddf992cbb6e6a4220bcb3d9184b5` | `4b53d0834c7869592ba1d579f74102fb2a4edf4a` |
| `fc68e731eed685a0c36073d3b23fd44449e48e35` | `a11d0da0cfdefa5e579584966c06163b7dd6cceb` |
| `433391754589c43cb9f81e55360bcef958a7374c` | `33e90ceca12ebc5d70b908031f6a8380debd60e5` |
| `5313c0d67854b02316c47854c55039009adc63e7` | `b93ffbfd4d99d2e956641cbc85fc876322743ff9` |
| `1fa6a4078116a41b8b3090a600244d110c193066` | `604fbc8abaf0be0e92a1813115533728b4b51a68` |
| `4501fad6c07030b885d1a8d107499c049cfebdcc` | `696ac1d4c372101da875b3297e2431395d042809` |

## 后续过程文档不进入LFS（2026-10-18）

- 用户要求：“后续过程文档也不要进入lfs了”。LFS改为按33个成品及必要图像依赖的精确路径列出，取消按图片/设计文件后缀自动跟踪；当前31个对象/80802395字节及其原始内容不变。
- 提示词、参数、生成/选择、评审、自查和其他轻量文字由普通Git保存；过程图片、设计文件以及`work/`、`qa/`中的PDF/Office等二进制过程文档和压缩包仅留授权本地工作区，不进入LFS，也不把过程图片改存普通Git绕过约束。
- 新增成品/必要依赖须同步产品保留清单、`.gitattributes`精确路径和`.gitignore`例外。`python tools/verify_asset_retention.py --staged`验证待提交内容与有效过滤规则，拒绝清单外的任意LFS指针和意外过滤；已验证误入LFS的Markdown指针和泛化Markdown过滤规则都会失败。
- 当前产品main `b46e262f0abc34cd19044342668b33d17d3ebcb4`、T-011 `dfc2ddfa606dfc44a63308bef955b30c5a16d5f5`；两条分支均仅追加一条单行维护提交，不新增merge、不重放原业务提交。T-011未合入，业务/专业评审/批准状态不变。
- 本轮变更前的双仓库Git恢复包和执行记录在 `D:\free-pipi-shrimp\.git\asset-archives\process-docs-no-lfs-20261018`；完整旧素材仍在上一节原归档，不再重复上传。
