# 全书内容与配套资料盘点

- 项目：book-bitcoin
- 成果 ID：A-001
- 负责人：project-manager
- 关联任务：T-001
- 版本：0.1
- 更新日期：2026-10-11
- 状态：draft
- 适用范围：现有 13 篇书稿的内容、结构、资源引用和后续审校输入。
- 来源及输入版本：用户要求先盘点再规划；书稿仓库 `git@github.com:yinzhongkai/book-bitcoin.git`，提交 `0811b23eb19d79ceb55a3c64a81944312eec0f2c`，目录 `bitcoin/`；定位确认见 [D-004](../../DECISIONS.md)。服务器位置查找依据本会话用户“先使用ssh tiger登录到服务器，然后在服务器的workspace目录下找找看”。
- 存储位置：OPC `projects/book-bitcoin/artifacts/book/content-inventory.md`；扫描证据见 [inventory-evidence.json](inventory-evidence.json)。
- 批准依据：尚无；盘点完成不代表专业审校或出版批准。
- 版本记录：2026-10-11，0.1，首次覆盖全书并登记服务器配套资料位置。

## 总体判断与盘点方法

全书已有完整的学习路线：先认识数据、共识与脚本，再追踪状态、交易池、网络与区块生产，最后进入钱包、RPC 和索引。尾声收回九章知识，附录 A 用 C++ 链上分析工具综合运用。后续工作的重点是证明这些叙述、源码定位和实验承诺在同一版本下成立，并让配套资料可以被读者取得。

用户已确认沿用前言定位：有 1–3 年经验、能读 C/C++ 并使用 Linux、尚不熟悉 Bitcoin 的开发者。现有方法是“原理、源码、eBPF 追踪”三条线交织。书稿声明的环境为 Bitcoin Core v31.0、Ubuntu 24.04 LTS / Linux 6.8、bpftrace 0.20.x；本盘点登记这些声明，未验证环境兼容性。

本次覆盖全部 task00–task12：全文扫描标题、代码围栏、示例编号与资源引用；全文阅读前言、序章和尾声；正文与附录读取章首、各节概述、要点、交付、承接及重点疑点段落。它是内容与审校输入盘点，未逐句审校全部正文，未逐段验证源码，也未运行书稿命令。下表的审核重点是后续工作入口，不是已成立的专业缺陷。

13 篇合计 **649322 字节、8139 行、229 个代码围栏块**。全文出现 **C-1 至 C-170 共 170 个不同示例编号**；代码块、示例编号和可独立执行的实验不是同一种计数，实验数量须由 T-006 建立矩阵后确认。当前书稿仓库共 15 个跟踪文件，另两份是仓库说明和导航，没有随书图表、配套源代码或实验记录。

## 逐篇内容盘点

文件均位于上述精确提交的 `bitcoin/`。行数和代码块数由全文扫描获得。

| 文件 | 规模：行 / 代码块 | 内容与学习结果 | 主要源码 / 运行入口 | 后续审核重点 |
|---|---:|---|---|---|
| task00-前言.md | 31 / 0 | 读者、三轨方法、阅读收益、环境门槛 | 版本与读者约定 | 与全书实际深度和交付承诺一致；环境要求可取得 |
| task01-序章-把地图摊开.md | 321 / 16 | 全景、三阶段路线、源码地图、构建环境与 regtest 入门 | Core 构建、bpftrace hello、钱包与出块 | 从空环境可走通；配套 URL、版本清单和标签能定位 |
| task02-1-解码Bitcoin的数据模型.md | 770 / 19 | 序列化、CompactSize、字节序、交易 / SegWit、区块头、Merkle、nBits；C-1–14 | serialize / streams / primitives / merkle；序列化观测 | 表示与语义分清；数据往返、哈希和字节序示例可核对 |
| task03-2-验证共识规则.md | 822 / 20 | 验证分层、交易与区块检查、上下文、PoW、难度调整；C-15–28 | consensus/tx_check、validation、pow；栈与耗时观测 | 共识与策略边界；检查顺序、时间 / 难度口径、性能数字来源 |
| task04-3-拆解脚本引擎.md | 971 / 28 | 栈执行、P2PKH / P2SH / SegWit / Taproot、验签与成本；C-29–47 | script/interpreter、solver、pubkey、secp256k1；验签直方图 | 脚本执行规则、升级差异、观测单位；示意数据与实测结论一致 |
| task05-4-追踪UTXO与链状态.md | 690 / 19 | Coin、CoinsView 缓存、连接 / 回滚、undo、重组；C-48–63 | coins、txdb、validation、undo；缓存与状态窗口 | 状态生命周期与回滚不变量；触发条件、命中率定义与证据 |
| task06-5-潜入交易池.md | 686 / 17 | 本地策略、费率、TxGraph、接收、RBF、淘汰、包；C-64–76 | mempool_entry、txmempool、txgraph、validation、policy/rbf | v31.0 具体接口与策略默认值；构造案例及事件统计口径 |
| task07-6-接入P2P网络.md | 589 / 18 | 节点发现、连接、握手、inv/getdata、BIP 324 v2；C-77–91 | net、net_processing、protocol、bip324、AddrMan；双节点观测 | 双节点前置、v1/v2 分界、消息与系统调用统计含义 |
| task08-7-构建区块模板.md | 715 / 21 | GBT、交易选择、coinbase、补贴 / 成熟度、PoW、紧凑区块；C-92–109 | node/miner、rpc/mining、blockencodings；模板计时与栈 | 承接交易池和网络；选包、收益、BIP 152、阶段回顾一致性 |
| task09-8-发起一笔交易.md | 707 / 24 | descriptor、SQLite、HD 钱包、选币、费率、签名、PSBT；C-110–128 | wallet、scriptpubkeyman、spend、coinselection；签名与选币观测 | 钱包状态前置、RPC 参数、找零 / 费率 / PSBT 流程能复现 |
| task10-9-查询与监听——RPC与索引.md | 844 / 26 | HTTP / JSON-RPC、认证、注册、查询 / 估费、索引、ZMQ；C-129–151 | httpserver、httprpc、rpc、index、zmq；延迟、同步与通知 | 索引配置与重启前置、查询可用条件、线程与通知语义 |
| task11-尾声-拼合全景.md | 174 / 1 | 六层全景、跨层接缝、线程视角、脚本总览、附录交棒 | 九章分图合成全景图；18 脚本总览 | 回收九章而不引入未解释前提；数量、标签、图与正文一致 |
| task12-附录A-BTC金融数据分析工具.md | 819 / 20 | C++ `onchain` 的 stats / scripts / utxo / graph；C-152–170；链上取数、指标、聚类、CSV 与图 | libcurl / JSON / CMake、RPC 批量遍历、自建 UTXO、gnuplot | 从节选能否完成工具；网络与数据口径、UTXO 对账、价格输入、聚类结论边界 |

P1 包含 chapter 1–3，提供后续状态与运行观测的基础；P2 包含 chapter 4–7，依次形成状态、待确认交易、传播与出块的链路；P3 包含 chapter 8–9，面向读者的实际交互。附录依赖九章，不宜在正文接口与实验基线稳定前先定稿。

## 逐章配套资源

下表登记书稿所引用的资源名，重复引用去重后为 **18 个 `.bt` 脚本、10 个 `.drawio` 图和 10 个 `*-prompt.md` 图稿提示**。序章内联 hello 探针未计入 18 个脚本。图稿提示的文件名与同一图的 stem 对应。

| 章节 | `.bt` 脚本 | 图 stem（`.drawio` / `-prompt.md`） | 书稿指称的参考标签 |
|---|---|---|---|
| 1 | serialize_count | ch1-data-model | chapter1 |
| 2 | validation_ustack、pow_duration | ch2-consensus-validation | chapter2 |
| 3 | sigverify_hist、script_exec_count | ch3-script-engine | chapter3 |
| 4 | utxo_cache_hit、chainstate_window | ch4-utxo-chainstate | chapter4 |
| 5 | mempool_events、mempool_feerate_hist | ch5-mempool | chapter5 |
| 6 | p2p_messages、p2p_sendmsg | ch6-p2p | chapter6 |
| 7 | blocktemplate_timing、miner_ustack | ch7-blocktemplate | chapter7 |
| 8 | wallet_sign、wallet_coinselect | ch8-wallet-tx | chapter8 |
| 9 | rpc_handler_latency、index_sync、zmq_notify_count | ch9-rpc-index | chapter9 |
| 尾声 | 汇总以上 18 个脚本 | epilogue-panorama | epilogue |

序章另引用 `prologue`。这 11 个参考标签的可取得性尚未建立。附录 `bitcoin-appendix-a` 是正文要求读者为自己的实现创建的标签，不能当作已发布的参考快照。

其他引用包括 `configs/bitcoin/versions.md`、`bitcoin-tools/bpf/bpftrace/`、`bitcoin-tools/onchain/` 及其 `plot/`、`out/`。正文图稿仍引用 `content/bitcoin/resource/diagram/`、`resource/prompts/` 一类路径；它们与当前书稿根目录 `bitcoin/` 的关系需在资料清单中说明。

## tiger 服务器查找结果

2026-10-11 使用 `ssh tiger` 只读查找；登录目录 `/home/oops`，工作区 `/home/oops/workspace`。未切换提交、修改远端文件或运行书稿实验。

| 对象 | 实际位置与证据 | 当前结论 |
|---|---|---|
| Bitcoin Core | `/home/oops/workspace/bitcoin`；origin 为 `https://github.com/bitcoin/bitcoin.git`；分支 `tag-v31.0`；HEAD 与 `v31.0^{commit}` 均为 `6574cb40869b96b9ffc79c19dc8f4e467d60f321`；Git 状态为空 | 已定位可供后续核查的精确源码候选；源码正确性与实验环境尚未审查 |
| bitcoin-tools | `/home/oops/workspace/bitcoin-tools`；无 Git 元数据；文件清单如下 | 是零散配套脚本目录，尚不能证明书稿所承诺的版本化配套仓库已就绪 |
| 图、提示、版本清单 | workspace 深度 8 内按文件名查找，跳过 `.git`、node_modules、sstate-cache、downloads、tmp、build | 未找到本书指定的 10 图、10 提示和 `configs/bitcoin/versions.md`；命中 Core 子项目的 versions.md 和其他项目图，不能替代书稿资源 |
| onchain 与实验记录 | bitcoin-tools 全目录只读列出文件；同范围另按 bitcoin 路径查找日志 / CSV / Markdown 等记录候选 | 目录中没有 onchain；未找到独立实验记录候选。不能据此断言服务器其他路径没有资料或以前未做实验 |

bitcoin-tools 的文件清单：

| 相对路径 | 大小（字节） | 与书稿资源名的关系 |
|---|---:|---|
| bpf/bpftrace/serialize_count.bt | 1128 | chapter 1 同名 |
| bpf/bpftrace/validation_ustack.bt | 1390 | chapter 2 同名 |
| bpf/bpftrace/validation_count.bt | 1909 | 不在上述 18 个引用名中 |
| bpf/bpftrace/pow_verify_hist.bt | 1422 | 不在上述 18 个引用名中；不能直接视作 pow_duration.bt |

书稿引用的 18 个脚本中，本次只有两个同名文件可定位，其余 16 个未在该目录找到。同名不证明内容一致；受本次查找授权限制，没有读取脚本内容或计算指纹。该服务器目录未登记为 OPC 产品工作区，本次仅作为用户授权查找的外部输入位置，不自动复制、改写或增加工作区配置。

## 可直接定位的审校入口

位置以书稿精确提交的文件及 1-based 行号为准；内容解释由 researcher / reviewer 后续确认。

| 编号 | 已观察到的文字或资料情况 | 核查方向 | 任务 |
|---|---|---|---|
| I-01 | 序章 task01:137 引用缺席版本清单，217–223 配套 URL 是占位符；本地不含资源，服务器只有上述四脚本 | 建立可取得的资源、精确版本和路径清单；区分随书提供与读者练习产物 | T-002、T-004 |
| I-02 | task02:27 使用作者绝对路径 `/home/oops/workspace/bitcoin/src/...`；序章及各章绑定版本、二进制与探针 | 统一可移植路径；建立源码 commit、构建选项、二进制与观测环境关系 | T-003、T-006、T-007 |
| I-03 | task08:713 把 P2 收束写成“三章拼图”，序章 task01:63 / 73–76 把 P2 定为四章 | 确认是仅回顾交易池 / 网络 / 挖矿三章，还是阶段数量需要改写 | T-005、T-008 |
| I-04 | task04:840 明标数值非实测，854 又以该例作为“实测注脚”；跨章还引用 PoW 耗时 | 每个性能数字登记来源、环境、单位、采样和适用条件；示意与实测标注一致 | T-003、T-006、T-007 |
| I-05 | 尾声 task11:130 使用“一百五十余个代码示例”；全文不同 C 编号为 170 个 | 明确正文 / 附录计数范围，避免把代码示例、脚本与观测点混用 | T-005、T-009 |
| I-06 | 附录 task12:32 指向配套仓库全量代码，805 又说明不预置实现；当前目录确无 onchain | 明确从零练习需要提供的脚手架、完整实现取得方式或可推导程度；缺失不直接认定为违约 | T-002、T-004、T-005 |
| I-07 | 附录 task12:41 / 738 对 testnet 的分析代表性提出较强叙述，565 / 585 / 598 又区分合成价格与经济意义 | 核查所用测试网、历史 / 激活边界、数据解释与统计口径；评估同章措辞一致性 | T-003、T-005、T-009 |
| I-08 | 附录 task12:737–740、744 承诺批量取数、自建 UTXO 对账、四子命令和产出；当前无实现与运行记录 | 验证窗口 / warmup、链高与索引前置、逐聪对账、合成价格标签和聚类启发式边界 | T-003、T-006、T-009 |

TxGraph / RBF、钱包接口、索引、JSON-RPC 字段、线程与 USDT / uprobe 等均需对精确 Core 源码、BIP 与运行环境核查。以上只是版本敏感项清单，本盘点没有凭记忆判断其对错。

## 对计划的影响

先确认审校覆盖与交付边界，同时落实精确源码和配套资料缺口。随后在同一书稿输入上完成领域核查与独立评审，建立实验矩阵；本轮意见齐备后才由 writer 统一修订。修订按 P1、P2、P3 / 尾声 / 附录推进，按变更影响复核，最后进行全书一致性检查与验收准备。详细安排见 [审核与修订计划](review-plan.md)，原始任务状态见 [TASKS.md](../../TASKS.md)。
