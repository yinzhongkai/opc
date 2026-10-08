# Chapter 3 Yocto 真实环境复验报告

## 1. 结论

本次复验在运行中的 Docker 容器 `books` 内实际执行，未修改书稿。九项必测范围均已覆盖，最终环境已恢复为：`meta-tiger` 正确注册、collection ID 为 `tiger`、探针文件清零、Poky 与 `meta-tiger` Git 工作树干净，`bblayers.conf` 和 `local.conf` 与复验前哈希一致。

重点裁决如下：

1. `bitbake-layers show-recipes` 的 provider 显示为 **`meta-tiger`**，不是 collection ID `tiger`。
2. 空 layer 查询**并非静默**；会输出 `WARNING: No bb files in default matched BBFILE_PATTERN_tiger ...`。即使指定不存在的 `tiger-probe`，命令仍返回 `RC=0`。
3. 完整 `yocto-check-layer` 实际运行 8 项测试；2 个非适用的检查类跳过，`test_patches_upstream_status` 为 `unexpected success`，最终为 `OK`、`meta-tiger ... PASS`，`RC=0`。
4. `LAYERSERIES_COMPAT_tiger = "Scarthgap"` 会精确报错为 collection `tiger` 与 core 的 `scarthgap` 不兼容；失败的 `add-layer` 前后 `bblayers.conf` 哈希和内容完全相同，确认自动回滚。
5. 正文使用的 `recipes-bsp` 路径得到与先前 `recipes-core` 实验一致的结论：少一级目录时不匹配，正确层级的 provider 显示为 `meta-tiger 0.1`。
6. 隔离副本中的首次 `git init -b main` 确实创建空仓库并产生 root commit；当前真实 `meta-tiger` 验证提交的 author/committer 为 `Validation Bot <validator@fruitpie.example>`。

## 2. 审计资料

- 执行日期：2026-10-03（Asia/Shanghai）
- 原始日志：[raw.log](raw.log)，688 行，SHA-256 `52F8F528A36B4A1BA6BCDD9A86131C24332313BE16F297B03B3C41355B1EADCC`
- 补证日期：2026-10-04（Asia/Shanghai）
- 补充日志：[supplemental.log](supplemental.log)，378 行，SHA-256 `6746C31D5029C62685598B9A0CE0107A12A608639D1E6408A407AE43C6D349BF`
- 主执行脚本：[run-in-container.sh](run-in-container.sh)
- 完整 layer 检查补充脚本：[run-check-layer-in-container.sh](run-check-layer-in-container.sh)
- 补证脚本：[run-supplemental-in-container.sh](run-supplemental-in-container.sh)
- 所有关键命令均在两份日志中以 `BEGIN/END` 标记，并紧随 stdout/stderr 与 `RC=<值>`。

## 3. 环境与起始基线

| 项目 | 实测值 |
|---|---|
| 容器用户 | `uid=1001(oops) gid=1001(oops)` |
| 内核 | `Linux tiger 6.18.33.2-microsoft-standard-WSL2 x86_64` |
| Poky 路径 | `/home/oops/workspace/poky` |
| Poky HEAD | `cbd62bb2a9f2ab3466a0f72f4289bc86ca20a019` |
| BitBake | `2.8.1` |
| 发行版 | Poky `5.0.20` / Scarthgap |
| MACHINE | `qemuarm64` |
| build | `/home/oops/workspace/build` |
| 测试 layer | `/home/oops/workspace/meta-tiger` |
| 起始 layer HEAD | `586f0d7b507e782ca54e8e3ef0685c4a14333c98` |
| 起始分支/状态 | `main`，干净 |
| 起始 `bblayers.conf` SHA-256 | `33fb66dc05ed9803e4f5af150fb3642c42ad2bd8b1220428d39ab87320f99e86` |
| 起始 `local.conf` SHA-256 | `44e5057ac0c35c469e23bf29681655ce855a9c61abf3151aa30afeed008f8c25` |

起始盘点未发现 `tiger-probe` 残留；`meta-tiger` 已注册一次；`conf/layer.conf` 已使用 `BBFILE_COLLECTIONS += "tiger"` 和小写 `scarthgap`。因此无需清除未知残留，直接以该干净状态为复验基线。

## 4. 必测项覆盖

| # | 必测项 | 状态 | 实测结果与证据标记 |
|---|---|---|---|
| 1 | 目录骨架与官方目录对照 | 通过 | `T1-01`：存在 `conf/{distro,machine,layer.conf}`、`recipes-{bsp,core,kernel}`、`README`、`COPYING.MIT`、`MAINTAINERS`。补证 `S1-01`～`S1-05` 实际执行正文第 3.1 节三个 `ls` 和 `cat meta-poky/conf/layer.conf`；全部 `RC=0`。 |
| 2 | collection `tiger` 的 add-layer、配置与 show-layers | 通过 | `T2-01`～`T2-06`：移除后 `add-layer` 成功；`bblayers.conf` 加入 layer；`show-layers` 显示 `tiger ... priority 6`；展开的 `BBFILE_COLLECTIONS` 为 `core yocto yoctobsp tiger`；相关命令 `RC=0`。 |
| 3 | 空 layer 的 show-recipes | 通过，有警告 | `T3-01`/`T3-02`：输出 `No bb files ... BBFILE_PATTERN_tiger`；不存在匹配 recipe，但命令 `RC=0`，不是静默空输出。 |
| 4 | 未注册/已注册构建对照 | 通过 | `T4-03` 与 `T4-06` 均构建 `core-image-minimal` 成功，均为 4073/4073 tasks、无需重跑；未注册配置不出现 `meta-tiger`，已注册配置出现 `meta-tiger = "main:586f0d7..."` 并附空 layer 警告。两次 `RC=0`。 |
| 5 | 完整 yocto-check-layer | 通过 | 首次 `T5-01` 因 layer 已注册只提示前置条件，未计作完整检查；补充 `T5B-01`～`T5B-05` 先移除 layer，再运行完整测试并恢复注册。实际 `Ran 8 tests in 85.637s`，最终 `PASS`、`RC=0`。 |
| 6 | 独立 Git 初始化、提交与状态 | 通过 | 起始真实目录已是独立 Git 仓库，`T6-01`～`T6-05` 验证原地 re-init、提交与干净状态。补证 `S3-01`～`S3-11` 在不含 `.git` 的隔离副本中实际完成首次 `git init -b main`、本地身份配置、add、root commit、log 与干净状态，并删除副本。`S4-01`～`S4-03` 另行读取当前真实提交的 author/committer。 |
| 7 | 大小写错误与自动回滚 | 通过 | `T7-03` 将兼容值改为 `Scarthgap`；`T7-05` 精确报错并 `RC=1`；失败前后 `bblayers.conf` 均为 `35eb048d...`，`cmp` 与 `diff` 均证明内容一致；随后恢复小写值并成功注册。 |
| 8 | 探针少一级与正确层级 | 通过 | 原始 `T8` 在 `recipes-core` 验证。补证 `S2-02`～`S2-05` 按正文路径重演：`recipes-bsp/tiger-probe_0.1.bb` 不匹配，只出现空 layer 警告；移入 `recipes-bsp/tiger-probe/tiger-probe_0.1.bb` 后匹配成功，provider 精确显示 `meta-tiger 0.1`。 |
| 9 | 清理与最终状态 | 通过 | `T9-01` 无探针残留；`T9-02`/`T5B-05` 确认 layer 已注册且 `show-layers` 为 `tiger`；两个 Git 工作树干净；配置哈希与起始值一致。 |

## 5. 关键原始输出

### 5.1 collection ID 与 layer 展示

命令：`bitbake-layers show-layers`（`T2-05`）

```text
layer                 path                                                                    priority
========================================================================================================
core                  /home/oops/workspace/poky/meta                                          5
yocto                 /home/oops/workspace/poky/meta-poky                                     5
yoctobsp              /home/oops/workspace/poky/meta-yocto-bsp                                5
tiger                 /home/oops/workspace/meta-tiger                                         6
RC=0
```

`bitbake -e core-image-minimal` 展开结果（`T2-06`）：

```text
BBFILE_COLLECTIONS=" core yocto yoctobsp tiger"
RC=0
```

### 5.2 空 layer 查询

命令：`bitbake-layers show-recipes tiger-probe`（`T3-02`）

```text
WARNING: No bb files in default matched BBFILE_PATTERN_tiger '^/home/oops/workspace/meta-tiger/'

Summary: There was 1 WARNING message.
RC=0
```

因此不能把该场景描述为“标准输出为空”或“完全静默”。

### 5.3 构建对照

未注册（`T4-03`）与已注册（`T4-06`）均得到：

```text
NOTE: Tasks Summary: Attempted 4073 tasks of which 4073 didn't need to be rerun and all succeeded.
RC=0
```

差异是：已注册构建会在 Build Configuration 中增加：

```text
meta-tiger           = "main:586f0d7b507e782ca54e8e3ef0685c4a14333c98"
```

并因 layer 尚无 `.bb` 文件而产生 `BBFILE_PATTERN_tiger` 警告。空 layer 的注册不改变本次镜像任务结果，但会改变配置可见性与诊断输出。

### 5.4 完整 yocto-check-layer

完整运行前已确认 layer 不在 `BBLAYERS` 中。实际测试（`T5B-03`）：

- `test_layerseries_compat`：ok
- `test_parse`：ok
- `test_patches_upstream_status`：unexpected success
- `test_readme`：ok
- `test_show_environment`：ok
- `test_signatures`：ok
- `test_world`：ok
- `test_world_inherit_class`：ok
- `BSPCheckLayer`：skipped，layer 不是 BSP layer
- `DistroCheckLayer`：skipped，layer 不是 Distro layer

汇总原文：

```text
INFO: Ran 8 tests in 85.637s
INFO: OK
INFO:  (skipped=2, unexpected successes=1)
INFO: meta-tiger ... PASS
RC=0
```

### 5.5 兼容系列大小写错误

失败配置：

```text
LAYERSERIES_COMPAT_tiger = "Scarthgap"
```

`bitbake-layers add-layer /home/oops/workspace/meta-tiger` 的完整错误核心（`T7-05`）：

```text
ERROR: Layer tiger is not compatible with the core layer which only supports these series: scarthgap (layer is compatible with Scarthgap)
ERROR: Parse failure with the specified layer added, exiting.
RC=1
```

失败前后 `bblayers.conf` SHA-256 均为：

```text
35eb048d12216306217c2304563e9d2e481536e0652063824bd06f201cfe46b2
```

`cmp` 返回 0，`diff` 无差异且返回 0，确认失败加入被自动回滚。

### 5.6 探针 provider

错误层级 `recipes-core/tiger-probe_0.1.bb` 不匹配当前 `BBFILES` 的 `recipes-*/*/*.bb`，仍只得到空 layer 警告。

修正为 `recipes-core/tiger-probe/tiger-probe_0.1.bb` 后（`T8-05`）：

```text
=== Matching recipes: ===
tiger-probe:
  meta-tiger           0.1
RC=0
```

结论：collection ID 是 `tiger`，但 `show-recipes` provider 列显示 `meta-tiger`。两者不是同一个展示字段，正文不应互换。

### 5.7 第 3.1 节官方目录与 `meta-poky/conf/layer.conf`

补充日志 `S1-01`～`S1-03` 实际执行正文中的三个目录查看命令。关键结果如下：

- `meta-poky/`：`classes`、`conf`、`README.poky.md`、`recipes-core`。
- `meta-yocto-bsp/`：`conf`、`lib`、`README.hardware.md`、`recipes-bsp`、`recipes-graphics`、`recipes-kernel`、`wic`。
- `meta/`：除正文列出的关键目录外，实际还包含 `classes-global`、`classes-recipe`、`COPYING.MIT`、`recipes.txt`；完整输出以补充日志为准。

`S1-04`/`S1-05` 完整读取并提取 `meta-poky/conf/layer.conf` 的相关行，实测包含：

```text
BBPATH =. "${LAYERDIR}:"
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"
BBFILE_COLLECTIONS += "yocto"
BBFILE_PATTERN_yocto = "^${LAYERDIR}/"
BBFILE_PRIORITY_yocto = "5"
LAYERSERIES_COMPAT_yocto = "scarthgap"
LAYERDEPENDS_yocto = "core"
```

以上命令均为 `RC=0`。

### 5.8 `recipes-bsp` 正文路径探针

补证按正文路径重新执行，而不是沿用先前的 `recipes-core`：

```text
recipes-bsp/tiger-probe_0.1.bb
```

此时 `bitbake-layers show-recipes tiger-probe` 只输出：

```text
WARNING: No bb files in default matched BBFILE_PATTERN_tiger '^/home/oops/workspace/meta-tiger/'
Summary: There was 1 WARNING message.
RC=0
```

移入正确层级：

```text
recipes-bsp/tiger-probe/tiger-probe_0.1.bb
```

再次查询得到：

```text
=== Matching recipes: ===
tiger-probe:
  meta-tiger           0.1
RC=0
```

随后探针已删除；`git status --porcelain=v1 --branch` 只显示 `## main`。

### 5.9 隔离副本首次 Git 初始化与真实提交身份

补证在 `/tmp/chapter3-revalidation-supplemental/meta-tiger-fresh` 创建不含 `.git` 的 skeleton 副本。初始化前 `test ! -e .git` 返回 0；随后：

```text
$ git init -b main
Initialized empty Git repository in .../meta-tiger-fresh/.git/
RC=0
```

隔离副本显式设置的 repo-local 身份为：

```text
user.name=Chapter 3 Validation
user.email=chapter3-validation@example.invalid
```

`git add .` 后状态显示 `No commits yet on main` 及 9 个新增文件；首次提交产生 root commit `35d7803f65478fabe4c3725a1d803258d78d9497`，author/committer 均为上述隔离身份。提交后状态只显示 `## main`，临时副本随后删除并以 `TEMP_COPY_ABSENT_RC=0` 确认不存在。

当前真实 `/home/oops/workspace/meta-tiger` HEAD `75ce208a2477a5bcab1f61fbe2d1b4345d5ffdfe` 的实际提交信息由 `git show -s --format=fuller HEAD` 读取，而非从 README 推断：

```text
Author:     Validation Bot <validator@fruitpie.example>
Commit:     Validation Bot <validator@fruitpie.example>
```

## 6. 异常与限制

- 第一次 `yocto-check-layer` 调用发生在 layer 已注册状态，工具只提示应先从 `BBLAYERS` 移除并返回 0；该调用没有被误算为完整测试。之后以 `T5B` 补跑完整测试并恢复注册。
- 起始真实 `meta-tiger` 已由此前试验初始化为 Git 仓库。原始证据中的原地 re-init 不代表首次初始化；2026-10-04 补证已在不含 `.git` 的隔离副本中完成真正的首次初始化和 root commit，且未破坏真实仓库。
- 两次镜像构建均完全命中既有任务状态（4073 个任务均无需重跑）。证据支持“同一 build 基线下两种注册状态均可成功解析并完成构建”，不代表执行了全量无缓存重建。
- 未执行 QEMU 启动或实机验证，因为本轮明确范围是 chapter 3 layer/metadata 行为。

## 7. 最终环境状态

- `meta-tiger` 已在 `bblayers.conf` 中注册一次。
- `show-layers` 显示 collection `tiger`，priority `6`。
- `conf/layer.conf` 已恢复为 `LAYERSERIES_COMPAT_tiger = "scarthgap"`。
- `tiger-probe` 文件与目录均已删除。
- Poky 工作树干净。
- `meta-tiger` 分支为 `main`，工作树干净；最终 HEAD 为 `75ce208a2477a5bcab1f61fbe2d1b4345d5ffdfe`。
- `bblayers.conf` 最终哈希与起始相同：`33fb66dc05ed9803e4f5af150fb3642c42ad2bd8b1220428d39ab87320f99e86`。
- `local.conf` 最终哈希与起始相同：`44e5057ac0c35c469e23bf29681655ce855a9c61abf3151aa30afeed008f8c25`。
