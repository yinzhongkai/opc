# 项目成果索引

## 首站补齐与实际发布记录（2026-10-18）

- [T-011实际操作接收记录](T-011-publication-observation-20261018.md)：PM只读复核两篇定时及前序用户反馈；运营已交A-011 v0.4并判准备范围completed，PM独立核验通过；不预填公开或手机结果。
- [T-015～T-020串行生产计划](T-015-station01-completion-plan.md)：用户确认首站共5篇、补齐后三篇；规划/作者/设计/审校/产品/运营依次执行，未启动的任务不计为存量。

## 当前素材存储定位（2026-10-18，过程图清退）

产品当前主线 `main@ef5399b0ec6dc89bc657a65968346f62ad3e4434`，T-011已快进合入并完成目标验证/远端核对，WORKSPACE一致；OPC目标验证/远端核对及两仓T-011开发分支清理完成，当前延续双仓T-015同后缀feature；planner已交付A-012并经PM接收；writer已交A-013准备和输入方法验证，当前因Kimi登录条件阻塞，完整新稿/图片仍未产出，新增记录均仅本机。保留成品/已批准头像/封面模板/当前SVG依赖共33图片路径、31唯一对象、80802395字节，原路径和内容哈希保持。

当前物料入口：产品 `ASSET_STORAGE.md`、`manifests/asset-retention-20261018.json`。以下作者条目及既有阶段SHA保留历史时点；已清退图像和老版本来源由本机完整备份读取，当前物料集合与恢复定位以 [LFS素材清退记录](T-014-lfs-asset-retention.md) 为准。远端已删除并按同名私库重建，31个对象重新上传及下载校验通过；GitHub后台当时显示0.1GB/10GB。LFS只按清单精确路径选择成品/必要图像依赖，过程文档不进入LFS；过程文字普通Git，过程二进制材料本地保存。


## 成果物当前版本定位（2026-10-18，T-014 续）

成果物仓库 `git@github.com:yinzhongkai/free-pipi-shrimp.git`，主线为 `main@5313c0d67854b02316c47854c55039009adc63e7`，与 WORKSPACE.revision 一致。以下原负责人条目及阶段小节保留其登记时点；当前固定版本以本节和 [历史整理记录](T-014-history-consolidation.md) 为准。所有原路径、内容哈希和评审／批准范围保持。

| 模块 | 当前固定提交 | 范围 |
|---|---|---|
| A-006 视觉资源 | `b915bd69586cddf992cbb6e6a4220bcb3d9184b5` | 已进入 main；原路径 `deliverables/T-012-first-two-visual-assets/v0.2-draft/` |
| A-008 文案与来源／套版输入 | `fc68e731eed685a0c36073d3b23fd44449e48e35` | 已进入 main；原 T-006 文案及来源路径 |
| A-009 文字套版 | `433391754589c43cb9f81e55360bcef958a7374c` | 已进入 main；原 `deliverables/T-006-first-two-buffer-plogs/v0.1-typeset-draft/` |
| A-010 v0.1 完整稿 | `5313c0d67854b02316c47854c55039009adc63e7` | 已进入 main；原 `deliverables/T-006-first-two-buffer-plogs/v0.1-integrated-draft/` |
| T-011 的 A-010 v0.2 AI 说明 | `1fa6a4078116a41b8b3090a600244d110c193066` | 仍在 `feature/t-011-first-publication-package`；`deliverables/T-011-first-publication-package/v0.2-copy-disclosure-draft/` |
| T-011 的 A-011 运营与检查包 | `4501fad6c07030b885d1a8d107499c049cfebdcc` | 同一 T-011 分支，未合入 main；`deliverables/T-011-first-publication-package/` |


每位成果负责人维护自己的条目，协调记录维护人检查索引一致性。元信息与批准规则见根 [项目运行协议](../../../PROJECT_PROTOCOL.md)。

| 成果 ID | 名称与正文链接 | 负责人 | 关联任务 | 版本 | 状态 | 批准决定 |
|---|---|---|---|---|---|---|
| A-001 | [自由的皮皮虾需求说明](A-001-需求说明.md) | product-manager | T-001、T-002、T-007、T-008 | 0.4 | approved | D-001 ~ D-008（2026-09-16、2026-09-28 确认） |
| A-002 | [自由的皮皮虾视觉基线包](A-002-自由的皮皮虾视觉基线包.md) | visual-designer | T-003 | 0.4 | approved | 2026-10-06 用户确认头像“我觉这个头像可以”；product-manager 已关闭 PM-001～PM-004；外部 `main` 功能提交 `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` |
| A-003 | [三星堆首站连载规划](A-003-三星堆首站连载规划.md) | planner | T-004 | 0.3 | draft | D-009（仅确认首站目的地；方案正文仍将在 T-012 与 T-006 中验证） |
| A-004 | [皮皮虾 3:4 封面视觉模板](A-004-封面视觉模板.md) | visual-designer | T-005 | 0.2 | approved | D-010（2026-10-12 批准模板及三星堆示例）；产品最终 `main@5404a33758b83fd5aff00ced55a4d893bf700c08`，原路径/哈希不变；完整封面 pass、问题为空；非发布授权 |
| A-005 | [三星堆第1站封面文案](A-005-三星堆第1站封面文案初稿.md) | writer | T-013、T-005 文案修订 | 0.2 | draft | 全文未批准；产品最终 `main@5404a33758b83fd5aff00ced55a4d893bf700c08`；RV-COPY-001／002 pass / closed、K3 原稿保留；D-010 仅采用示例中的候选 A 短句 |
| A-006 | [三星堆首批两篇视觉资源](A-006-三星堆首批两篇视觉资源.md) | visual-designer | T-012 | 0.2 | draft | 产品 `feature/t-012-first-two-visual-assets@d1f42cea3230374640d8f2efdc7329d0f2c7fff5` 不变，fixed-git-lfs-1 清单196项匹配；reviewer `4f283652…` 第2轮限定17项视觉／现有文字 pass，RV-VIS-001／002关闭；writer `a908fdf9…` 实际接收可用，发起人核对后 H-009 closed、负责人登记 T-012 completed（draft资源交付）；未合入main，填词／完整稿／全部AC／批准及发布未覆盖 |
| A-007 | [plog 文字结构与文案槽位规范](A-007-plog文字结构与文案槽位规范.md) | writer | T-010 | 0.1 | draft | 草拟及 SC-01～SC-05 作者自查完成，仅覆盖 AC-2／AC-3 文字结构部分及 AC-6 骨架自查；首篇八图、17 SVG／18 种实际节点映射可对接已合入视觉资源。没有新 K3 原稿、整篇／填词后验收或批准 |
| A-008 | [三星堆前两篇文案与来源包](A-008-三星堆前两篇文案与来源包.md) | writer | T-006 | 0.4 | draft | 原 reviewer 第 2 轮文案／来源限定 pass、001～004 closed；writer 已选套版草稿并交付输入包，产品 `feature/t-006-first-two-buffer-plogs@5f58227db0b340056f1dbbab8752039a72740be2`，10 新文件（manifest SHA-256 `3db1705b35eb98fbcdb491f4d75b006d39bb0b3bf10655a215596c54923e45cd`）；两篇 16 页／60 节点、15 启用必填有值、2 可选有值、11 可选留空，备用未启用。新选择值仅供套版，旧 35 文件／315 blob 及评审历史保持；待设计师实际套版及整篇验证，全文未批准 |
| A-009 | [三星堆前两篇文字套版视觉包](A-009-三星堆前两篇文字套版视觉包.md) | visual-designer | T-006 专业协作 | 0.1 | draft | 产品 `feature/t-006-first-two-buffer-plogs@5af5615d83b59fff85bdbdb50583f825cb5f82ba`，16 SVG／16 PNG（14 新导出、首封／事实卡 2 原字节复用），60 节点对应、逐页原尺寸／270 拼版自查和工程重开完成；96 文件固定核验，manifest SHA-256 `cbcc02ab8848a24b9ad9151baa8f126ddb02aab3c8db56f08a71337e52804266`。系统650字重的3页文字像素差异已披露，以固定PNG为成品；待完整图文独立复核／作者整合，未批准或发布 |
| A-010 | [三星堆前两篇完整稿件](A-010-三星堆前两篇完整稿件.md) | writer | T-006原稿、T-011说明修订协作 | 0.2 | draft | 两篇独立AI说明各末尾追加“文字含 AI 辅助创作。”并同步新note／JSON；T-011产品 `c589870894cc379ad98689f7e8e79d94dfb6a2e3`，12新文本文件，manifest SHA-256 `074bfd70fd53f7505a712ab65e18e1ce7e3e59050f146b3290d73c1b4b364b68`。474旧blob、v0.1完整稿／所有原限定／标题／正文／16图及T-006 completed范围保持；OP-T011-001已形成新稿待复核，原运营问题和T-011 blocked不改，不代原生AI声明、独立pass或用户批准 |
| A-011 | [首期运营准备与发布包检查](A-011-首期运营准备与发布包检查.md) | social-media-operator | T-011 | 0.4 | draft | 产品ef5399b0ec6dc89bc657a65968346f62ad3e4434；v0.4实际接收13新文本，manifest SHA-256 4d401638e2b09bfa1b12a19d6829ec3a94702d6988b955752bc0a8db13403826；原三条件满足、T-011仅准备范围completed。前2篇10月19/22日20:30 scheduled，后3待制作/待采用及proposed，15数据窗口null。原第3轮限定pass/RV003 closed保持，v0.4无新独立审校；平台保存后逐页/标识/审核及公开数据未验，H-003仅PM处理，未批准或代发布 |
| A-012 | [三星堆后三篇故事线与事实输入](A-012-三星堆后三篇故事线与事实输入.md) | planner | T-015 | 0.1 | draft | 3篇各封面＋7页，共24页；逐页故事线、事实／虚构边界、视觉禁区、替代与文字槽位齐备。2026-10-18实际核验一手结构／修复记录和具名论文；当前开放／在展及图像许可仍未知，采用原创意象替代。产品只读固定ef5399b0ec6dc89bc657a65968346f62ad3e4434；PM已核对接收为T-016规划输入，T-015阶段completed，成果仍draft；不等于K3原稿、最终图文、独立审校、用户采用或发布 |
| A-013 | [三星堆后三篇文案与来源包（具体展品说明）](A-013-三星堆后三篇文案与来源包.md) | writer | T-016 | 0.5 | approved | D-013（2026-10-27 用户最终采用第03～05篇固定图文；writer 2026-10-28 按 H-010 回写 approved，批准范围限 v0.5 固定文案本身，不含发布／账号／定时授权）。固定产品`40d190d4302546ef3f937c8efe437a1ee9311691`，三篇标题／正文／每篇5标签／独立AI与虚构说明／来源栏与配图参考以 T-020 copy.md 与固定 note 六字段一致记录为证；文案6项清单SHA-256 fac5a2079046bda930e3fe2e39a074bcc8345bb6dd514582cb9b82cb1c7e4ce4。全部固定文案、来源、历史候选与审校记录原样保留。v0.5 登记时点原文：产品本地3960731e7ba5eb2322d2cf82ebc3cb9e14b7efb7；三篇仅AI说明及新增配图参考变化，固定v0.2视觉782cd870d6fba40a43ca017daaafcd9b89ea7c6f；原主体／学术来源、83原槽＋21辅助槽、24PNG／SVG保持，选择JSON SHA-256 d10a6972c971138f7fc80c8247077afabe0be872ce89fd2e67fa9a8efd8f072a，606旧路径／字段差异／来源／11链接／JSON／普通Git／7提交字节通过，debug.log保留未入库 |
| A-014 | [三星堆后三篇视觉与套版](A-014-三星堆后三篇视觉与套版.md) | visual-designer | T-017 | 0.3 | approved | D-013（2026-10-27 用户最终采用第03～05篇固定图文；visual-designer 2026-10-28 按 H-011 回写 approved，批准范围限 v0.3 登记的 24 张 PNG 及正确图序（含 RV-T018-001 更正后来源登记），不含发布／账号／定时授权）。固定产品`40d190d4302546ef3f937c8efe437a1ee9311691`，24 图位于`deliverables/T-017-station01-remaining-plogs/v0.2-exhibits-draft/`各篇 images/，自 v0.3 固定提交 3c1c05f 至当前 25294db 该范围 Git diff 为空；视觉92项清单修正版本体SHA-256 c22865546a2f7b47f5ed02b8cf0a6207bad4335d9bcd85deb70ccc515b81751c。v0.3 登记时点原文：2026-10-27按RV-T018-001补齐S10作者登记为7人原序，两处reference-register.json同字节同步更正（新Git字节10421、SHA-256 a94942786458afd5493ec7fa9c2c03fa39daa4d67b00cfc7156b480dc9d95df9），产品固定3c1c05f3a1ba02ce264ca55be339db7d6dc0d508已推送并核对远端；24图／24 SVG／三篇note／83原槽＋21辅助槽及其他登记不变，更正前字节保留于Git历史3960731e…。v0.2基线：按D-011交新24张1080×1440展品图文／24 SVG，原7纯字页全部修订；13次真实调用／11选中及3必要CC0原参考；逐页原尺寸／缩略本人自查、字体边界／图文分区／暂存哈希／旧业务及原debug.log保全、LFS提交前后实测通过。面具藏品号unknown；RV-T018-001复核pass／closed；旧v0.1保留历史，未合主线／发布 |
| A-015 | [首站后三篇运营总包](A-015-首站后三篇运营总包.md) | social-media-operator | T-020、T-021 | 0.2 | draft | 产品固定`40d190d4302546ef3f937c8efe437a1ee9311691`及D-013图文不变；v0.2按D-014／H-012登记网页草稿与逐篇授权流程。三篇草稿保存、重开、24页截图与像素核验完成后，用户于2026-10-28 01:01确认按候选档期分别发布；笔记管理已逐篇核验第03／04／05篇定时发布2026-10-29／11-02／11-05 20:30（Asia/Shanghai），公开可见，最终草稿箱0。第04篇首次显示审核中、最终不再显示该标识，第05篇最终显示审核中，未显示不推断通过；第05篇标题误命中已在提交前恢复并复核。实际记录位于产品`work/T-021-station01-web-publication/batch-20261028/execution-record.json`。T-021 in_progress，三篇实际公开时间／链接／稳定ID／实际公开可见范围仍null，定时或审核中不等于公开成功；前两篇及PAUSED监控不变，A-015整体未批准 |
| A-016 | [峨眉山第二站连载规划](A-016-峨眉山第二站连载规划.md) | planner | T-022 | 0.1 | draft | D-015仅确认第二站目的地峨眉山与看猴子主题；5篇第一人称连载（出发—初见—边界—山上—轻轻离开），前2篇为缓冲候选并含封面＋7图逐图素材卡，第3～5篇含8图骨架与备选。2026-10-28实际读取E01～E06六份官方／权威页面：藏酋猴国家二级保护、不投喂不挑逗不抚摸等安全硬边界、人猴分离、猴伤处置、高山区观光车分时预约；景区官网未获可读正文已登记缺口，时效规则列复核表。图像许可边界明确：新闻与网页图片只读不复制，无许可用原创剪影／意象并标注AI情境。仅规划候选稿，不启动制作、不新建产品分支、不发布；产品只读，未覆盖最终文案／图像／独立审校／用户采用 |
| A-017 | [峨眉山首批两篇视觉资源](A-017-峨眉山首批两篇视觉资源.md) | visual-designer | T-023 | 0.1 | draft | 尚无批准决定；产品固定 `feature/t-023-station02-first-two-visual-assets@051203883c11ad37eae45716bf1ff84cf806c667`（已推送核对），17 PNG／17 SVG／15 背景，30 空槽 pending-writer-k3，manifest 68 项 SHA-256 `627f5064617121b524b444b5d9bfc200d636343ee21638b3a252776a1b3d29f6`；导出器逐页安全区／规格自查与保留校验（73 路径／73 对象／161634870 字节）提交前后均 passed；含保留清单 b708c85 后续同步更正（71 移 archived）；野生动物／预约／事实边界按 A-016 §6 执行，猴形象仅原创剪影标 AI 情境；待 PM 核对与 reviewer 独立评审，未合 main、未批准、未发布 |
| A-018 | [峨眉山前两篇文案候选包](A-018-峨眉山前两篇文案候选包.md) | writer | T-024 | 0.2 | draft | 尚无批准决定；产品固定 `feature/t-023-station02-first-two-visual-assets@615190d10a5212b948eb541338a17858e8b2cfab`（已推送核对）。v0.1 第1轮独立审校 `pass`、无返修；两篇各选候选1“慢慢去见山里的邻居”／“远远看见，就很好”，正文、独立AI／虚构说明及逐页配文原句不变。v0.2 正式16页29槽（17必填全有值；12可选中7有值、5空值附理由），`p1-alt` 备用且 `selected_value: null`；17份真实SVG／节点／固定哈希与 `selected_value` 映射齐备。manifest 10个新文件＋23个只读来源，SHA-256 `cfd8d6511bc8b2b6b7a351bc36bf397448d7835b12e08753c994c82eaac0454f`，提交前后校验 `pass`。原v0.1产品`184512485dc5998e0185790d80719fac722830b8`、30槽候选、K3／ChatGPT来源链及22项manifest保留；A-017视觉`051203883c11ad37eae45716bf1ff84cf806c667`未改。`rendered: false`、`user_final_adoption: null`，待PM核对，未套版、未整合、未批准、未发布 |
| A-019 | [峨眉山前两篇文字套版视觉包](A-019-峨眉山前两篇文字套版视觉包.md) | visual-designer | T-024 专业协作 | 0.1 | draft | 尚无批准决定；产品固定 `feature/t-023-station02-first-two-visual-assets@67ed39349cda7b6d1fae7f27e2cd6e8adcd374a8`（含计数与固定来源哈希两项一致性修正，单行 message，已推送核对）。按 A-018 v0.2 selected-slot-mapping 将正式 16 页 29 槽套入真实节点：24 有值槽逐字等于已评候选、5 可选空槽保留空白附理由；16 SVG／16 PNG（1080×1440 RGB/sRGB）＋14 背景（与 T-023 逐字节一致）＋2 份 note 原字节随包；`p1-alt` 未启用未导出。导出器逐页规格／字体／安全区／逐字断言与两次截图一致性全过；修正后 `verify-typeset.py` 两次实跑 144 项全 pass 且字节稳定（新增固定来源 blob 断言），保留校验提交前后均 passed（103 路径／89 对象／194651874 字节／59 SVG／55 依赖），manifest 56＋33 项 SHA-256 `cdb1d32e1ba2c8bd84313d93b03debdb0755a3bffee7e1fcd595fc54e25022aa`。未改字、未新增事实、未重绘或改图，A-017／A-018 旧文件未覆盖；待 PM 核对后 reviewer 完整图文独立审校与 writer 整合，未合 main、未批准、未发布 |

## T-005 工作区集成定位（2026-10-12）

产品仓库 `git@github.com:yinzhongkai/free-pipi-shrimp.git`，当前线性版本 `5404a33758b83fd5aff00ced55a4d893bf700c08`；当前工作区基线见 WORKSPACE.yaml。A-004 的模板/说明/示例位于 `deliverables/T-005-cover-visual-template/v0.2-draft/`，完整清单位于 `manifests/T-005-cover-visual-template/manifest-v0.2-draft.md`；A-005 位于 `deliverables/T-013-cover-copy-draft/v0.2-draft/cover-copy-draft.md`，来源与修订证据位于对应 work/T-013 目录。全部原路径和已登记哈希保持不变。

两仓库原始生成、作者、评审及批准提交由各自远端标签 `archive/t005-pre-squash-20261012` 保留；成果正文中的原固定 feature SHA 属于来源/受批版本证据，不作为当前 main 定位。合并不改变 A-004 的批准范围、A-005 全文 draft 状态或发布权限。

## T-012 工作区集成定位（2026-10-14）

产品仓库 `git@github.com:yinzhongkai/free-pipi-shrimp.git` 已按用户本轮授权快进合入、验证并推送 `main@d1f42cea3230374640d8f2efdc7329d0f2c7fff5`，WORKSPACE 仅 revision 同步到该版本。A-006 视觉包位于 `deliverables/T-012-first-two-visual-assets/v0.2-draft/`，完整清单位于 `manifests/T-012-first-two-visual-assets/manifest-v0.2-draft.json`，原批次及修订／核验来源位于 `work/T-012-first-two-visual-assets/`；原路径及 A-006 登记哈希不变，仍 v0.2 draft。

上表／作者正文的 feature 与尚未合入描述保存其受评和负责人收尾时点，当前集成位置以本段、WORKSPACE 及 T-012 最新项目经理记录为准。OPC 最终项目版本为 `5ee097df6252272ed786b9915d185da750e29d60`，两仓快进合入、目标验证与远端同步通过，本地／远端 T-012 feature 已安全删除，原标签及全部已合入来源可追溯。合入不批准 A-006 全文或完整稿件，也不授予发布权限。

## T-010 治理集成定位（2026-10-14）

A-007 v0.1 draft 的作者交付为 `2973f0691d91b64e7cde6502a864952b23da379a`，正文原字节已进入 OPC 项目主线 `project/free-pipi-shrimp@25421a5d2a8b3df3e9f95780fcc82d221b5fffe0`。本轮只有 OPC 治理交付，产品 main／WORKSPACE 保持 `d1f42cea3230374640d8f2efdc7329d0f2c7fff5`；快进集成、目标验证及本地／远端 T-010 feature 清理完成，原标签和来源提交保留。A-007 作为 T-006 结构输入使用，不等于新 K3 文案、完整稿件、独立审校或成果批准。

## T-006 输入包远端同步与阶段交接（2026-10-14）

用户已明确要求先推送两仓本地提交，再由用户转交 Kimi 作者的执行指令。产品输入包已正常推送至 `feature/t-006-first-two-buffer-plogs@db81ecb78c1e3430cc30e7b7cfad4c091dd3ffbb`，19 文件的大小／SHA-256 与原清单匹配，远端／跟踪头一致，main／WORKSPACE 不变。OPC 本人成果登记及后续协调记录沿 `feature/free-pipi-shrimp/t-006-first-two-buffer-plogs` 同步，实际固定入口见本轮执行回执；任务最新阶段安排已登记。

A-008 上表和正文中的“本机、未推送、待外传授权”保留作者登记时点，不覆盖本轮用户授权和已验证的产品远端位置。原 v0.1-prepared 的全部输入、哈希和历史拒绝记录不变；A-008 仍 v0.1 draft，尚无新 K3 原稿或两篇候选。后续业务原稿及候选在产品新批次／版本保存，Kimi 作者交回后再串行进行 ChatGPT 审核修订及独立审校。

## T-006 K3 初稿交付定位（2026-10-14）

用户在既有 Kimi 作者会话明确指示执行 K3 原稿与两篇文案候选阶段，writer 已完成生成、归档与双仓交付。产品 `feature/t-006-first-two-buffer-plogs@5c5c934ce419c206120a258027dfabb4620eef0e` 已推送并核对远端一致，含批次目录 `work/T-006-first-two-buffer-plogs/batch-20261014-k3-draft/`（5 文件）、`deliverables/T-006-first-two-buffer-plogs/v0.1-draft/copy-candidates.md` 及 `manifests/T-006-first-two-buffer-plogs/manifest-v0.1-draft.json`，共 7 文件，各文件 SHA-256 见 A-008 v0.2 末节；原 v0.1-prepared 19 文件与产品 main／WORKSPACE 不变。

A-008 升 v0.2 仍 draft，T-006 保持 in_progress；本交付不等于 ChatGPT 作者审核修订、reviewer 独立审校、暂定采用、套版、整篇复核或成果批准。下一位 project-manager 固定受评版本并安排 reviewer 审校。OPC 治理记录沿本任务原 feature 同步，不合入主线、不删除分支、不发布。

## T-006 工作区集成定位（2026-10-15）

按用户本轮明确授权，产品六个功能提交已快进合入、目标验证并正常推送至`main@104b533b4bd66825918e2f32871268f31179d876`，WORKSPACE仅revision同步，配置独立提交`14e7a51ba72912e614be73e6f96387010cae4bfb`。A-010完整稿入口为`deliverables/T-006-first-two-buffer-plogs/v0.1-integrated-draft/README.md`，清单`manifests/T-006-first-two-buffer-plogs/manifest-v0.1-integrated-draft.json`的Git SHA-256仍为`e136e3c44496604adc761e9be04a5021240645197a90146e363680553ee8b6e5`；A-008／A-009原输入、套版工程及所有来源路径、字节和固定提交不变。

上表及负责人正文的feature／未合入描述保留各自登记时点，当前集成依据本段、WORKSPACE及TASKS最新项目经理记录。产品六清单170文件／63唯一LFS实体合并前后匹配，18读者文件保持第3轮已评原字节；OPC最终项目版本`850a42c66d4ed3efb5babc15ae9b436460371d49`已快进合入、目标验证并正常推送，两仓本地／远端T-006 feature已安全删除，原标签和全部已合入来源保留。收尾后才从两个最新目标主线建立T-011新分支。A-008 v0.4／A-009 v0.1／A-010 v0.1均保留draft及原评审范围，不新增成果批准或发布权限。

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
批准依据：尚无
版本记录：<日期、版本、主要变化和替代关系>
```

任务完成不自动批准成果。重要交接或批准须引用实际版本及可用的 Git 提交或内容摘要；修改批准内容时更新版本并重新说明评审状态。

## 当前线性历史定位（2026-10-13，T-014）

用户本轮授权按功能整理历史并移除 merge commit。成果物当前 main 为 `5404a33758b83fd5aff00ced55a4d893bf700c08`，T-003 视觉基线功能提交为 `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd`；内容、原路径、SHA-256、生成/作者/评审/批准记录与业务状态不变。正文中的旧集成/feature 提交保留为历史证据，当前定位以本段和 WORKSPACE 为准；对应恢复点及功能映射见 [T-014 历史整理记录](T-014-history-consolidation.md).
