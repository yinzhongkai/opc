# T-015 chapter 3 命令、配置与输出真实环境核验记录

## 1. 核验范围与环境

- 执行者：`reviewer`
- 执行日期：2026-09-27
- 受评正文：`workspace/yocto/task04-3-搭起meta-tiger的骨架.md`，Git `37116d9`，656 行；执行前确认当前正文相对该提交无差异。
- 执行环境：远程主机 `tiger`，用户 `oops`；Ubuntu 24.04.4 LTS，x86_64，内核 6.8.0-101-generic；8 个 CPU，内存约 31 GiB。
- Yocto 基线：Poky `scarthgap`，提交 `77d1feb37e280733684ae8a9449fb031d5d7ff40`（`yocto-5.0.20-82-g77d1feb37e`）；BitBake 2.8.1；`MACHINE="qemuarm64"`，`DISTRO="poky"`；构建目录 `/home/oops/workspace/build`。
- 前置状态：Poky 工作树干净；`/home/oops/workspace/meta-tiger` 不存在；`bblayers.conf` 只含 `meta`、`meta-poky`、`meta-yocto-bsp` 三个官方 layer，SHA-256 为 `35eb048d12216306217c2304563e9d2e481536e0652063824bd06f201cfe46b2`。
- 授权与副作用：用户于 2026-09-27 明确指示“你叫 review 执行吧”，授权创建并提交 `meta-tiger`、修改 `bblayers.conf`、执行构建与 `yocto-check-layer`、注入并恢复两组错误。未修改受评正文。
- 原始证据：`workspace/t015-chapter3-raw.log`，545 行、23,932 字节，SHA-256 `89097abc9c4f9199fd6a634ecb5edfe209bec852020fc7a3e3dfbf52cb8266b7`。下文行号均指向该日志。

## 2. 恢复策略与最终状态

- 执行前保存 `bblayers.conf`、`local.conf` 哈希，并在错误实验前另存配置副本。
- 大小写错误实验使用 `bitbake-layers add-layer` 触发解析失败；工具自动恢复 `bblayers.conf`，随后核对哈希、layer 列表，再修正并重新注册。
- recipe 层级实验只创建未提交的探针文件；核验后删除整个探针目录并执行 `git status`。
- 最终 `meta-tiger` 已正确注册，priority 为 6；Poky 与 `meta-tiger` 两个 Git 工作树均干净；`local.conf` 哈希与执行前一致；未残留探针配方。详见原始日志 L497-L545。

## 3. 逐项核验结果

| 编号 | 正文位置与前置条件 | 命令/操作 | 正文预期 | 实际结果摘要 | 状态、证据与恢复 |
|---|---|---|---|---|---|
| V-1 | L31-66；固定 Poky 提交可读 | `find`、`sort`；查 Poky 本地文档 | 官方 layer 顶层目录与节选一致，并据此归纳惯例 | 三个目录实物与展示一致；但 `meta-`、全小写、连字符及 `recipes-*` 类别属于通行约定，不是 BitBake 强制且不是封闭分类 | 通过；原始日志 L20-L92；表述见 R-1 |
| V-2 | L139-202；`meta-tiger` 尚未注册 | 查看官方配置，写入原文 `layer.conf`，再用 BitBake 解析 | 官方关键行和完整新层配置可用 | 官方关键行与正文一致，新建配置可解析；`BBFILE_COLLECTIONS` 并不带 `_meta-tiger` 后缀 | 通过；原始日志 L57-L121；机制说明见 R-2 |
| V-3 | L74-137；目标路径原先不存在 | 原文 `mkdir`、`touch`、`find` | 目录、五个 `.gitkeep` 与 Fig-3-1 一致 | 骨架按预期创建；Fig-3-1 结构与最终实物一致，README 等文件在后续步骤补齐 | 通过；原始日志 L93-L106、L527-L543；无额外恢复 |
| V-4 | L149、L570-620；新层已注册 | 在错误/正确路径各运行一次 `show-recipes tiger-probe` | 错误路径不收录，正确路径显示版本 0.1 | 匹配逻辑成立；错误路径另发出空 layer 警告。目录要求来自本层选定的 `BBFILES` 模式，不是 BitBake 的普遍目录法则 | 局部不符；原始日志 L429-L447；见 R-3/R-4；探针按 V-11 清理 |
| V-5 | L204-344；骨架已创建 | 写入三份文档；运行 `test_readme`；核对 MIT 文本哈希 | 文档三件套满足本章和检查工具要求 | 三个文件被 Git 收录；README 检查通过；MIT md5 为 `0835ade698e0bcf8506ecda2f7b4f302` | 通过；原始日志 L122-L169、L318-L345；文件保留为交付物 |
| V-6 | L349-402；正确配置已写入 | 原文 `add-layer`、`show-layers`、`show-recipes | 四层列表含 priority 6；空 layer 标准输出为空 | 注册和列表符合预期；空 layer 查询实际打印 `WARNING: No bb files ...` | 局部不符；原始日志 L174-L211；见 N-1/R-4；layer 后续按检查步骤暂移并加回 |
| V-7 | L404-428；新层已注册，既有构建缓存可用 | `bitbake core-image-minimal` | 4073 个任务全部成功且无需重跑 | rc=0，任务汇总逐字相符；仍打印空 layer 警告；Git 尚未初始化使新层版本显示为 `<unknown>:<unknown>` | 通过；原始日志 L212-L303；观察 O-1；未改 `local.conf` |
| V-8 | L430-471；按正文先移除被测 layer | Git 初始化前后各运行一次 `yocto-check-layer ../meta-tiger` | 识别为 SOFTWARE，列出的关键测试为 `ok`，最终 PASS | 两次均 rc=0、PASS；实际运行 8 项，并出现不改变 PASS 的 `unexpected success`，格式和测试集合与预期块不同 | 局部不符；原始日志 L304-L361、L453-L496；见 N-2/R-5；检查后 layer 加回 |
| V-9 | L473-502；文档与骨架已完成，Git 身份已配置 | 原文 `git init -b main`、`add`、`status`、`commit`、`log` | 创建独立 main 仓库并提交全部交付文件 | 9 个文件、76 行，提交 `0e3da48 Initial meta-tiger layer skeleton`，文件集合一致 | 通过；原始日志 L362-L401；提交保留 |
| V-10 | L506-568；错误前保存配置哈希 | 临时改为 `Scarthgap`，执行 `add-layer`，比较哈希和列表，再改回小写并注册 | 解析失败，`bblayers.conf` 自动回滚；修正后成功 | rc=1，错误文本相符；失败后哈希未变且仅有三层；修正后四层列表恢复 | 通过；原始日志 L403-L428；错误配置已恢复 |
| V-11 | L570-633；Git 首次提交已完成 | 创建错误层级探针，移动到正确层级，查询后删除并执行 `git status` | 错误路径不可见，正确路径显示三行结果，删除后干净 | 正确路径结果逐字相符；错误路径有 N-1 警告；清理后 `nothing to commit, working tree clean` | 通过；原始日志 L429-L451；探针目录已删除 |
| V-12 | 全章；所有实验与恢复步骤完成 | `show-layers`、配置哈希、两个 Git 状态、日志、目录树 | 正确 layer 保留，临时错误和探针无残留 | `meta-tiger` 正确注册；Poky 和新层均干净；`local.conf` 未变，只留下授权的正确 layer 注册 | 通过；原始日志 L497-L545；最终恢复确认完成 |

统计：共 12 组；通过 9 组，局部不符 3 组，阻塞 0 组，未执行 0 组。

## 4. 不符项与观察项

### N-1：空 layer 和错误 recipe 层级并非“标准输出为空”

- 位置：L395-402、L587-596。
- 实测：空 layer 查询和错误层级查询都打印：

```text
WARNING: No bb files in default matched BBFILE_PATTERN_meta-tiger '^/home/oops/workspace/meta-tiger/'
```

- 证据：原始日志 L199-L211、L429-L436。`bitbake-layers show-recipes | grep -i tiger` 的 rc=0 还会被这条含 `tiger` 的警告满足，因此它不能证明“查询结果为空”。
- 影响：正文把实际警告描述成空输出，并把管道成功误当成正向验证；读者无法区分“没有配方”和“命令静默成功”。
- 建议：二选一并重新实测。其一，保留当前 `layer.conf`，展示并解释警告，同时用精确 recipe 名查询判断未发现配方；其二，加入 `BBFILE_PATTERN_IGNORE_EMPTY_meta-tiger = "1"` 抑制空层警告，并同步更新完整配置、Table-3-1 和错误层级实验的判定方式。若采用后者，须避免掩盖错误层级实验本来要展示的信号。

### N-2：`yocto-check-layer` 预期输出与固定环境实际输出不一致

- 位置：L442-464。
- 实测：两次检查均运行 8 项测试；单项名称与 `... ok` 分行显示，另有 `test_show_environment`、`test_world`、`test_world_inherit_class` 等。`test_patches_upstream_status` 显示 `unexpected success`，但套件最终为 `OK`，汇总为 `meta-tiger ... PASS`，rc=0。
- 证据：原始日志 L304-L346、L453-L494。
- 影响：预期块被正文标为将由实测回填；若不更新，读者会把格式和测试集合差异误认为环境异常。
- 建议：按本次实测替换关键输出；保留版本相关省略标记，并说明 `unexpected success` 在该固定版本中不改变 rc=0 和最终 PASS。

### O-1：Git 初始化顺序会影响构建配置中的 layer 版本展示

- 位置：L404-428、L473-502。
- 实测：正文先构建、后初始化 Git；因此构建成功时的 Build Configuration 把 `meta-tiger` 显示为 `<unknown>:<unknown>`。首次提交后再运行 layer 检查仍通过。
- 评价：不影响本章构建结论，也不是 Git 的功能性前置条件；建议在构建输出或相邻正文说明该顺序导致的可见版本信息，避免读者把 `<unknown>` 当成 layer 配置失败。

## 5. 机制性源码核查

- `bitbake/lib/bblayers/action.py`：固定版本的 `add-layer` 在解析失败时会用临时备份恢复 `bblayers.conf`；V-10 的实际哈希和 layer 列表也验证了这一行为。
- `bitbake/lib/bb/cooker.py`：无文件匹配时打印 N-1 的警告；集合可通过 `BBFILE_PATTERN_IGNORE_EMPTY_<collection>` 显式忽略空匹配。
- `scripts/lib/checklayer/__init__.py`：没有可识别 machine/distro 配置的本层被判为 `LayerType.SOFTWARE`；空目录中的 `.gitkeep` 不改变类型。
- `scripts/lib/checklayer/cases/common.py`：README 必须存在且非空，并检查维护者/补丁联系信息等；本章 README 内容通过了 `test_readme`。
- Poky `documentation/dev-manual/layers.rst`：`meta-` 是常用前缀但并非严格强制，用于 R-1 的约定边界判断。

## 6. 结论

结论：`revise`。本章的目录创建、`layer.conf` 实体、layer 注册、构建、Git 提交、兼容系列失败与自动回滚、recipe 正确层级和最终清理均通过真实环境验证；Fig-3-1 和 Table-3-1 的技术映射成立。N-1、N-2 必须回填真实输出，R-1～R-3 的机制边界也须由 writer 在 T-014 中统一修订。T-015 的远程执行和证据采集已经完成，待 project-manager 复核记录后关闭。
