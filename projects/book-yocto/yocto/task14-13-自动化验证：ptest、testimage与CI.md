## 13 自动化验证：ptest、testimage 与 CI

周五上午，工位。阿凯刚到，老周拎着笔记本过来，屏幕上挂着公司内部 GitLab 的 CI 面板——tiger 项目的 job 红着一片。

"QA 昨天手工测出一个回归，倒查回来是咱们周二合进去的改动。"老周把笔记本转过来，"这条流水线从建起来那天就是裸奔——只构建，不验证。红是迟早的事。"

"我每天不是都在跑镜像吗？"阿凯有点不服气，"上一章那串验收命令我敲过不止一遍。"

"手动跑一次镜像、登录进去敲几个命令，这不算测试。"老周摇头，"IoT 产品要长期推补丁，每次推都必须自动验证没回归。Yocto 自己就带了一套测试框架，你今天把它用起来。"

他在白板上列了四件事：**包级测试（ptest）、镜像级测试（testimage）、变更追踪（buildhistory）、进流水线（CI）**。

"我补一件。"阿凯举手——上一章他给自己布置过第五件事，今天故技重施，"通用的测试验的是别人家的东西。tiger 自己的服务、主机名、时区，谁验？"

"问得好，这件归你立项。"老周点头，"开工前先翻目录——测试框架长什么样，自己看，别问我。"

### 13.1 先把地图摊开：oeqa 一家三兄弟

```bash
# 看 Poky 内置测试框架的目录结构
ls ~/workspace/poky/meta/lib/oeqa/
```

输出（关键行，行序按相关度重排）：

```text
buildperf  controllers  core  files  manual  runtime  sdk  sdkext  selftest  utils  oetest.py  runexported.py
```

**oeqa（OpenEmbedded QA）** 本章正式引入：Yocto 内置的测试框架，构建时与运行时测试都归它管。目录即地图——`core/` 是框架本体（loader 装载用例、runner 执行、decorator 贴条件、target 抽象被测对象）；`runtime/` 是跑到目标板上的用例库；`selftest/` 是构建时测试的用例库（`oe-selftest` 命令驱动，本章点名存在即可，不展开）。

阿凯关心的三兄弟，分工一句话一个：

- **ptest**：包级——支持它的配方附带上游自带的测试套件，测试在目标板上跑。
- **testimage**：镜像级——把整个镜像在 QEMU 里拉起来，跑一整套运行时测试。
- **oe-selftest**：构建时测试——验构建系统自身的行为，不碰目标板。

ptest 和 testimage 的关系要早点说透，因为今天会来回用到：ptest 的测试跑在板上，但"驱动它跑、把结果收回来"的那条通道，常常就是 testimage 铺的——`runtime/cases/ptest.py` 里的 PtestRunnerTest 用 `self.target.run('ptest-runner ...')` 在板上驱动 ptest-runner（`self.target` 是被测板的抽象，13.4 认；这条 SSH 通道怎么搭，13.3 铺开讲）。

一句话记住分工：ptest 的执行现场在板上，驱动通道与 testimage 共用同一条 QEMU+SSH 通道——机制两套，通道一条。所以今天的顺序是：先让 ptest 上车（13.2），再把 testimage 的通道铺好（13.3），最后写 tiger 自己的考题（13.4）。

### 13.2 ptest：包自己的考试

#### 13.2.1 机制：ptest.bbclass 实物

**包测试（ptest）** 本章正式引入：Yocto 的包级测试框架——配方 `inherit ptest` 之后，只要发行版特性里带着 `ptest` 这个词，构建系统就会给这个配方多打出一个 `-ptest` 包，里面装着测试套件和入口脚本，测试在目标板上执行。机制实物在 ptest.bbclass，关键四行：

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/ptest.bbclass（节选，11/22/29/31 行）
PTEST_PATH ?= "${libdir}/${BPN}/ptest"
PTEST_ENABLED = "${@bb.utils.contains('DISTRO_FEATURES', 'ptest', '1', '0', d)}"
RRECOMMENDS:${PN}-ptest += "ptest-runner"
PACKAGES =+ "${@bb.utils.contains('PTEST_ENABLED', '1', '${PN}-ptest', '', d)}"
```

逐行认。第 22 行是总开关：**PTEST_ENABLED** 就地解释——`DISTRO_FEATURES` 含 `ptest` 时为 `1`，否则为 `0`；第 31 行按它决定要不要把 `${PN}-ptest` 追加进 PACKAGES——开关一关，-ptest 包根本不产生（ptest.bbclass 尾部还有一段匿名函数把 `do_*_ptest_base` 任务整组摘掉）。

第 11 行 **PTEST_PATH** 就地解释：测试在板上的落点目录，默认 `${libdir}/${BPN}/ptest`。第 29 行把 **ptest-runner** 就地解释带出来：板上执行器，按 RRECOMMENDS 随 -ptest 包推荐带入（推荐安装依赖——装得上就进镜像，装不上不阻塞）——它逐个目录找到入口脚本、执行、汇总结果。

入口脚本叫 **运行包测试（run-ptest）**，本章正式引入：ptest 框架执行测试套件的脚本入口，落盘在 PTEST_PATH 下，输出格式是三段式——每行 `PASS: <用例名>` / `FAIL: <用例名>` / `SKIP: <用例名>`，ptest-runner 按行首词统计。

顺手纠一个容易想当然的点：ptest 没有 `-c ptest` 这样的构建期任务——构建期它只负责把测试打进 -ptest 包，执行现场永远在板上。busybox 就有现成的（`meta/recipes-core/busybox/files/run-ptest`，九行脚本，进 testsuite 目录跑 runtest 再把 SKIPPED/UNTESTED 改写成 SKIP:），而且它被官方标在快测试名单里（`meta/conf/distro/include/ptest-packagelists.inc` 第 14 行，PTESTS_FAST 在册）——拿它做第一次板上实测，正合适。

配方侧产出了 -ptest 包，镜像侧怎么批量装？**ptest-pkgs** 就地解释：IMAGE_FEATURES 的合法特性词，一个词装进所有 ptest 包。证据链三级，都在手边：

```text
# 三级证据链（行号均为本地 Scarthgap 实测）
~/workspace/poky/meta/classes-recipe/core-image.bbclass:41
    # - ptest-pkgs          - ptest packages for all ptest-enabled recipes
~/workspace/poky/meta/classes-recipe/populate_sdk_base.bbclass:25
    COMPLEMENTARY_GLOB[ptest-pkgs] = '*-ptest ${MLPREFIX}ptest-runner'
~/workspace/poky/meta/classes-recipe/image.bbclass:66,69-70
    IMAGE_INSTALL_COMPLEMENTARY = '${@complementary_globs("IMAGE_FEATURES", d)}'
    valid_features = (d.getVarFlag('IMAGE_FEATURES', 'validitems') or "").split()
    valid_features += d.getVarFlags('COMPLEMENTARY_GLOB').keys()
```

core-image.bbclass 的注释清单把 `ptest-pkgs` 列进可用特性词；populate_sdk_base.bbclass 给它定义通配规则——匹配所有 `*-ptest` 包外加 ptest-runner；image.bbclass 第 66 行的补装通道按 IMAGE_FEATURES 展开这些通配去包仓库里捞包装进 rootfs，第 69-70 行把 COMPLEMENTARY_GLOB 的键名并入合法词表（12.5 节 check_image_features 的解析期校验认它）。

回指 12.5 的口诀：先查有没有现成的特性词，有就用词——ptest 安装通道就是"有词"的情形。

> **⚠️ 注意**：`ptest-pkgs` 装的是**所有** ptest 包——通配规则不认亲疏，镜像里任何一个配方只要产出了 -ptest 包都会被捞进来，体积可观。

> **💡 提示**：只想要个别包的测试，就别用特性词，显式把 `<包名>-ptest` 列进 IMAGE_INSTALL——跟 12.5 口诀的另一半（"没有就列包"）是同一个姿势。

#### 13.2.2 落点裁决与启用：ptest 开在哪张桌子

开关找到了，落到哪张桌子？阿凯自己推了一遍：`DISTRO_FEATURES` 是发行版策略，归 DISTRO 桌；base 还是 delta？prod 出货镜像带不带测试包？——体积是一笔钱（12.7.2 刚交过学费），测试套件还多开攻击面。他把结论说给老周听："落 dev delta，prod 不带，base 不动。"

顺嘴认一笔旧账：11.3 的裁剪清单把 ptest 归进过"砍掉"堆，落纸理由是"硬件事实和调试策略都不沾"——那刀砍出的是 base 清单，效果上两态默认都不带；今天 dev delta 把它明确加回，裁剪清单管产品画像，测试姿态是开发态的增量，两笔账不打架。

"对。"老周就补了一句，"**测试的姿态不是出货的姿态。**"

落盘：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro-dev.conf（追加）
# 测试姿态（chapter 13）：ptest 只开 dev 态（落点裁决见正文）
DISTRO_FEATURES:append = " ptest"

# ptest 包批量进镜像（ptest-pkgs 特性词，机制见 13.2.1）
EXTRA_IMAGE_FEATURES:append = " ptest-pkgs"
```

先说清代价：这一改动的是 DISTRO_FEATURES——它进几乎所有配方的任务签名，dev 态重建一次是大面积翻转，数小时级（11.9.2 量过同款）。接受它：这种全量重建本来就是 CI 该干的活，13.7 会回来兑现这句话。

变量级先验（对照组目录，零构建成本）：

```bash
# 对照组环境（build-distrotest，MACHINE=qemuarm64 + DISTRO=tiger-distro-dev）
cd ~/workspace/poky
source oe-init-build-env ../build-distrotest

# 总开关与包清单的终值
bitbake -e busybox | grep -E "^(PTEST_ENABLED|PACKAGES)="
```

输出（关键行）：

```text
PTEST_ENABLED="1"
PACKAGES="busybox-ptest busybox-httpd ... busybox-locale busybox"
```

`PTEST_ENABLED` 翻成 `1`，`busybox-ptest` 排进了 PACKAGES 首位（主包 `busybox` 按默认规则收尾）——机制全通。重建对照组镜像（数小时级，实测回填），启动，板上实测 busybox 的 ptest：

```bash
# 重建后启动（dev 态密码登录，12.6.2 起的新常态）
runqemu qemuarm64 tiger-image nographic slirp

# 板上执行（login: root，密码 tiger）：
ptest-runner busybox
```

输出（关键行示意，逐字以实测为准）：

```text
START: ptest-runner
BEGIN: /usr/lib/busybox/ptest
PASS: basename
PASS: bunzip2
# ... (数百行省略)
END: /usr/lib/busybox/ptest
STOP: ptest-runner
```

行首的 `PASS:`/`FAIL:`/`SKIP:` 三段式逐行认过去——busybox 几百个用例在板上真跑了一遍。包级测试这条腿，立住了。

<!-- 【事实核查注记】ptest-runner busybox 的逐行输出与总用例数随对照组实测回填；上文为示意形态，仅担保格式（START/BEGIN/PASS/END/STOP 与三段式行首）。 -->

#### 13.2.3 给 tiger-sysinfo 写 ptest：自写用例全闭环

现成的跑通了，该给自己写的服务配考试了。tiger-sysinfo 是 12.3 的交付物，ptest 化只加两样东西：入口脚本、配方里一行 inherit。

```bash
#!/bin/sh
# 文件路径：~/workspace/meta-tiger/recipes-core/tiger-sysinfo/tiger-sysinfo/run-ptest（新建）
# ptest 入口：在目标板上执行，只引用板上存在的文件与命令

. /etc/os-release
case "$PRETTY_NAME" in
    *"tiger IoT Linux"*) echo "PASS: os-release-pretty-name" ;;
    *)                   echo "FAIL: os-release-pretty-name" ;;
esac

if [ "$(cat /etc/hostname)" = "tiger-iot" ]; then
    echo "PASS: hostname"
else
    echo "FAIL: hostname"
fi
```

校验对象很眼熟——`/etc/os-release` 的 PRETTY_NAME 是 11.2.2 的发行版身份三件套落盘，`/etc/hostname` 是 12.4.1 的主机名。注意脚本用 `case` 和 `[ ]`，没用任何 bash 扩展：板上 `/bin/sh` 是 busybox 的 ash，写脚本时就按板上有的东西写。

提交前一件事别漏：`chmod +x run-ptest`——ptest-runner 在板上直接 exec 这个入口脚本，可执行位缺不得。装包这条路上其实有保底：ptest.bbclass 用裸 `install -D`（第 57 行）复制它，不带 -m 的 GNU install 恒落 0755；但仓库里就带上可执行位仍是好习惯——板上手工调试、或者别的工具不经 install 直接搬这个文件的场景，都用得上。

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-core/tiger-sysinfo/tiger-sysinfo_1.0.bb（编辑）
# SRC_URI 追加一项（与原两项同段）：
SRC_URI = "file://tiger-sysinfo.sh \
           file://tiger-sysinfo.service \
           file://run-ptest \
           "

# inherit 行旁追加：
inherit systemd
inherit ptest
```

`do_install` 一行不用动——ptest.bbclass 的 `do_install_ptest_base`（第 55-58 行）发现 `${WORKDIR}/run-ptest` 存在就自动装进 `${PTEST_PATH}`。落点展开是 `${libdir}/${BPN}/ptest`：tiger-distro 带 usrmerge（11.5.2 随行带入），板上实物是 `/usr/lib/tiger-sysinfo/ptest/run-ptest`；老规矩，写配方用变量，不背字面路径（12.3.2 的纪律）。

第一稿的阿凯多写了一段——他想"板上万一文件被改过，跟构建时的原件对一遍"：

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-core/tiger-sysinfo/tiger-sysinfo_1.0.bb（错误示范，请勿模仿）
# 第一稿的 run-ptest 里有一行：cmp @REFERENCE@ /usr/bin/tiger-sysinfo || echo "FAIL: script-mismatch"
do_install_ptest() {
    # 把"参照原件"的路径填进脚本
    sed -i "s#@REFERENCE@#${WORKDIR}/tiger-sysinfo.sh#" ${D}${PTEST_PATH}/run-ptest
}
```

构建风平浪静，板上 `ptest-runner tiger-sysinfo` 却报 `FAIL: script-mismatch`——三段式行首词后跟的是用例名，正是脚本里 echo 的那个词。阿凯盯着输出愣住——构建期什么都没说。

"ptest 在哪里跑？"老周问了一句。

"……板上。"阿凯自己反应过来：`${WORKDIR}` 展开的是构建主机的绝对路径（`build-distrotest/tmp/work/...` 一长串），板上根本没这个文件，`cmp` 打不开参照物，整条用例 FAIL。**ptest 的运行现场是目标板，run-ptest 里只能引用板上存在的文件与命令。** 官方也在擦同一个屁股——ptest.bbclass 第 65-78 行有一段 sed，专门把装进 -ptest 包的 Makefile 里的主机路径（HOSTTOOLS_DIR、WORKDIR）剥掉（PTEST_BUILD_HOST_FILES 机制，认得即可）。

修正：撤掉 `do_install_ptest`，run-ptest 删掉参照校验那一行，只留板上实物（/etc/os-release、/etc/hostname）。重建（增量，分钟级），重跑：

```text
START: ptest-runner
BEGIN: /usr/lib/tiger-sysinfo/ptest
PASS: os-release-pretty-name
PASS: hostname
END: /usr/lib/tiger-sysinfo/ptest
STOP: ptest-runner
```

自写 ptest 全闭环。这段发作与复盘收进 13.8 踩坑实录。

<!-- 【待验证·阻塞级】C-W25：tiger 组 ptest 实测——ptest-pkgs 把所有 -ptest 包捞进镜像后的总体积 vs rootfs_a 的 64 MiB 卷预算（10.2.2 的 vol_size）处置，以及 tiger 板上 ptest-runner 实跑输出；随 tiger 组仓库就位批次回填，续 C-W17~C-W24 同口径。对照组（qemuarm64）无卷预算约束，本地可验。 -->

### 13.3 testimage：整个镜像的考试

#### 13.3.1 机制：骑在 runqemu 背上的测试任务

ptest 管单个包，谁管整个镜像？**testimage** 本章正式引入：镜像级运行时测试机制——构建系统把镜像在 QEMU 里拉起来，通过 SSH 连进去，按用例清单逐条在板上执行命令、断言结果。它不是一个单独的程序，而是镜像的一个任务：挂钩方式和跑法，官方注释就写在类文件头上：

<!-- 【写作注记】testimage 在 glossary.md 无条目，本章为事实首次正式引入；glossary 补条目裁决留门控（循 task12 先例）。既有 ptest/run-ptest 条目（glossary 第 136-137 行）"由 bitbake -c ptest 执行"的措辞在 Scarthgap 无此任务（ptest.bbclass 只注册 do_*_ptest_base 一族任务），修正裁决一并留门控。 -->

```bitbake
# 文件路径：~/workspace/poky/meta/classes-recipe/testimage.bbclass（节选，8-19/74-78/91-96 行）
# testimage.bbclass enables testing of qemu images using python unittests.
# Most of the tests are commands run on target image over ssh.
# ...
# - first add IMAGE_CLASSES += "testimage" in local.conf
# ...
# - then bitbake core-image-sato -c testimage. That will run a standard suite of tests.
# ...
# The tests can be run automatically each time an image is built if you set
# TESTIMAGE_AUTO = "1"

TESTIMAGE_AUTO ??= "0"

BASICTESTSUITE = "\
    ping date df ssh scp python perl gi ptest parselogs \
    logrotate connman systemd oe_syslog pam stap ldd xorg \
    kernelmodule gcc buildcpio buildlzip buildgalculator \
    dnf rpm opkg apt weston go rust"
# ...（83-89 行：libc-musl 与 qemumips 的删减规则，略）

TEST_SUITES ?= "${DEFAULT_TEST_SUITES}"
QEMU_USE_KVM ?= "1"
TEST_QEMUBOOT_TIMEOUT ?= "1000"
# ...（第 95 行 TEST_OVERALL_TIMEOUT ?= ""，略）
TEST_TARGET ?= "qemu"
```

逐段认。**IMAGE_CLASSES** 就地解释：镜像级 inherit 的挂接变量——`IMAGE_CLASSES += "testimage"` 把 testimage 任务挂到所有镜像上（头部注释第 12 行的官方接法）；跑法是 `bitbake <镜像> -c testimage`，想在每次构建后自动跟跑就置 `TESTIMAGE_AUTO = "1"`（第 19 行，本章用手动命令，自动档知道即可）。

**TEST_TARGET ?= "qemu"**（第 96 行）是最关键的一行：被测对象的默认形态是 QEMU。而启动 QEMU 这件事 testimage 没有另起炉灶——它直接调 `runqemu`（oeqa/utils/qemurunner.py 第 166/178 行拼出 `runqemu snapshot ... <machine>` 命令行），读的还是同一份 qemuboot.conf、同一套 QB_* 变量（回指 4.6 节与 10.3.3 节）。**testimage 是骑在 runqemu 背上的**——前十章在 runqemu 上投的资，这里全部兑现。`snapshot` 参数顺带说一句：测试在可丢弃的快照上跑，怎么折腾都不伤镜像本体。

测试命令怎么进板？SSH。slirp 用户态网络下（第 276-278 行按 TEST_RUNQEMUPARAMS 判定），目标地址回落到 `127.0.0.1` 加端口转发，主机侧服务地址默认 `10.0.2.2`（第 323-329 行，**TEST_SERVER_IP** 就地解释）——和 1.5 节以来手动 `runqemu ... slirp` 用的是同一种网络。

**TEST_SUITES** 就地解释：跑哪些用例的清单，默认 BASICTESTSUITE 三十个词（第 74-78 行实物在上方）——从 python、gcc 到 weston、xorg 全家桶。注意头部注释第 38 行的立场：显式列进 TEST_SUITES 的每个词都是"必考科目"，产品镜像应当裁剪到跟自己有关的子集，否则结果里全是与本镜像无关的 skip/fail 噪音。**TEST_QEMUBOOT_TIMEOUT** 就地解释（第 94 行）：等登录提示符出现的秒数上限，慢机器要调它。**QEMU_USE_KVM ?= "1"**（第 93 行）先认半句：有 KVM 就用硬件加速跑 QEMU，没有会自动回落纯模拟——本书环境就属于后者，13.3.2 末尾的 ⚠️ 框细说。

落点裁决，阿凯这次直接给了答案：挂钩、清单、SSH 的门，全部落 `tiger-distro-dev.conf`——测试挂钩只挂 dev 态，prod 镜像保持无门。这跟 12.4.4 空密码退役是同一条逻辑的两端。

#### 13.3.2 首次跑通：先装门，再配钥匙

挂钩落盘，用例清单先裁剪到七个词（tiger 自己的用例 13.4 再加进来）：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro-dev.conf（追加）
# testimage 挂钩（13.3）：镜像级运行时测试，只挂 dev 态（裁决见正文）
IMAGE_CLASSES += "testimage"

# 用例清单裁剪：默认 BASICTESTSUITE 三十词大半与本镜像无关
TEST_SUITES = "ping date df ssh scp systemd ptest"
```

清单七个词的选择各有一句理由：ping/ssh/scp 验网络与通道本身；date/df/systemd 是任何镜像都该有的基本盘；ptest 就是 13.2 铺好的那条——经 testimage 的通道在板上驱动 ptest-runner，结果自动收回。parselogs（扫内核日志找可疑行）被刻意排除：

> **⚠️ 注意**：parselogs 对自定义板型和自定义镜像常有误报噪音——它的忽略清单是按 qemu 系官方机器维护的。先不进门禁，哪天要收编，先攒一份 tiger 自己的忽略清单再说。

QEMU 参数是会话级的，落 local.conf（不进仓库，CI 里的注入方式 13.7 见）：

```bash
# 在 ~/workspace/build-distrotest/conf/local.conf 末尾追加（不进仓库）：
TEST_RUNQEMUPARAMS = "slirp"
```

先泼一小盆冷水：`-c testimage` **不会替你构建镜像**——`addtask testimage`（类文件第 127 行）没有挂任何镜像任务依赖；镜像没构建过时，do_testimage 在第 220 行直接 bb.fatal（找不到 testdata.json——镜像构建期落盘的测试描述文件）。这里能直接开跑，是因为 13.2.2 刚重建过镜像。记住这条，13.7 进 CI 时它会回来咬人。

开跑：

```bash
# 镜像级测试首跑（镜像须已构建——13.2.2 重建过；起 QEMU 跑用例，无 KVM 容器里较慢，实测回填）
bitbake tiger-image -c testimage
```

结果：系统起来了，测试一色倒下。输出尾部（关键行示意）：

```text
RESULTS - ping.PingTest.test_ping: PASSED (0.01s)
RESULTS - ssh.SSHTest.test_ssh: FAILED - ... Connection refused ... (exit code 255)
RESULTS - date.DateTest.test_date: SKIPPED - ... depends on ssh.SSHTest.test_ssh ...
RESULTS - ptest.PtestRunnerTest.test_ptestrunner_expectsuccess: SKIPPED - ... depends on ssh.SSHTest.test_ssh ...
# ... (其余依赖 SSH 的用例同样 SKIPPED，省略)
SUMMARY: tiger-image () - Ran 17 tests ... FAILED (failures=1, skipped=15)
```

先认一笔账再往下走：清单里是七个**模块词**，17 是装载器把模块展开成 `test_` 方法后的用例数——systemd 一个词就带十个方法。数用例不数词，这条 13.4 的 ⚠️ 框还会回来讲。

ping 过了——它从主机侧 ping 目标，不走 SSH（而且 slirp 下目标是 127.0.0.1，用例检测到 localhost 会提前空跑返回，ping.py 里写得明白）；ssh 一倒，date/df/ptest/systemd 一整片跟着 SKIPPED——这些用例都声明了"SSH 先通我才考"，通道不通就跳过不考（这套依赖机制 13.4 讲装饰器时回来认）。伤口都指向同一个地方：SSH 连不上。阿凯第一反应是"系统没起来"，老周拦住他："boot log 看了吗？系统起来没有？"

测试结果目录在 `tmp/log/oeqa/`（结果 JSON 加按配方名分的子目录），类里把 boot log 以符号链接收进 `tmp/log/oeqa/tiger-image/`（testimage.bbclass 第 395-399 行）：

```bash
# 看 QEMU 启动日志（符号链接，指向镜像 workdir 下的 testimage/qemu_boot_log.<时间戳>）
tail -20 tmp/log/oeqa/tiger-image/qemu_boot_log.*
```

日志末尾 `tiger-iot login:` 端端正正躺在那儿——**系统起来了，只是没人应门**。这就是 13.8 要记下的排查纪律：先分清"没起来"还是"没应门"，两者的修法天差地别。没人应门的原因很直接：镜像里压根没有 SSH 服务。修，dev delta 补门：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro-dev.conf（编辑，testimage 段追加一行）
# SSH 测试通道的门（13.3.2）：测试命令经 SSH 进板；dev 态装门，prod 保持无门
EXTRA_IMAGE_FEATURES:append = " ssh-server-dropbear"
```

这个词哪来的？core-image.bbclass 头部那张可用特性词清单——12.1.2 指过路的那张表（FEATURE_PACKAGES 映射，第 46-59 行）——里就有它（第 25 行注释、第 57 行映射 packagegroup-core-ssh-dropbear）。落点与 12.4.4 的密码逻辑自洽：dev 态有密码、有 SSH，root 用密码登录。重建镜像（增量，分钟级），重跑 `-c testimage`——门是装上了，可 SSH 还是被挡在门外：

```text
RESULTS - ssh.SSHTest.test_ssh: FAILED - ... Permission denied (publickey,password) ...
```

这次的伤口阿凯认得——**这是自己 12.4.4 埋的回旋镖**：两态 root 都设了实密码，空密码便利退役了。

物证有两条。其一，oeqa 的 SSH 通道递不了密码——它组装的 ssh 命令行就这几个选项（oeqa/utils/sshcontrol.py 第 107-113 行：UserKnownHostsFile、StrictHostKeyChecking、LogLevel，完了），没有密码、没有 `-i`。其二，串口日志里 qemurunner 尝试用空密码登串口失败后打了一句 "Couldn't login into serial console as root using blank password"（第 542 行附近）——**官方通道的出厂假设就是空密码**。我们把空密码退役了，就得给测试通道配一把正式的钥匙。

钥匙走公钥，这也是 CI 的标准姿势。三件事：

```bash
# 第一件：构建主机上把私钥挂进 ssh-agent（没有 key 就 ssh-keygen 生成一把；
# 用你日常那把也行——公钥进门、私钥永不出主机）
eval $(ssh-agent -s)
ssh-add

# 第二件：公钥拷进 layer，做成一个极小的配方
mkdir -p ~/workspace/meta-tiger/recipes-core/tiger-testkey/tiger-testkey
cp ~/.ssh/id_ed25519.pub ~/workspace/meta-tiger/recipes-core/tiger-testkey/tiger-testkey/authorized_keys
```

```bitbake
# 文件路径：~/workspace/meta-tiger/recipes-core/tiger-testkey/tiger-testkey_1.0.bb（新建）
SUMMARY = "SSH public key for the test channel (dev images only)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://authorized_keys"

do_install() {
    install -d -m 0700 ${D}${ROOT_HOME}/.ssh
    install -m 0600 ${WORKDIR}/authorized_keys ${D}${ROOT_HOME}/.ssh/authorized_keys
}

FILES:${PN} = "${ROOT_HOME}/.ssh/authorized_keys"
```

`${ROOT_HOME}` 在 systemd 发行版下是 `/root`（init-manager-systemd.inc 第 9 行）。这份文件就是门上的锁芯——authorized_keys 里躺着谁的公钥，谁就进得了这扇门。第三件，把包挂进 dev 态的 tiger-image：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro-dev.conf（编辑，testimage 段追加一行）
# 测试通道的钥匙：root 公钥只进 dev 态镜像（机制见正文；prod 无门也无钥匙）
IMAGE_INSTALL:append:pn-tiger-image = " tiger-testkey"
```

为什么 ssh-agent 这头零配置：ssh 客户端默认会尝试 ssh-agent 里的钥匙；testimage.bbclass 第 308-318 行有一段 `export_ssh_agent`，显式把 `SSH_AUTH_SOCK`/`SSH_AGENT_PID` 往测试环境递——官方留的钥匙通道就是它。

白名单这一头更省事：BitBake 对环境变量有过滤白名单（`BB_ENV_PASSTHROUGH_ADDITIONS`），但 `source oe-init-build-env` 时它已经默认放行这两个变量——`scripts/oe-buildenv-internal` 第 110-119 行导出的那份默认名单，第 112 行就躺着 `SSH_AGENT_PID SSH_AUTH_SOCK`（1.6.2 的代理变量走的正是同一份名单）。所以钥匙通道即插即用，要做的只有开头那两条 shell 命令，配置文件一行不用动。

重建、重跑：

```text
RESULTS - ping.PingTest.test_ping: PASSED (0.01s)
RESULTS - df.DfTest.test_df: PASSED (1.02s)
RESULTS - date.DateTest.test_date: PASSED (2.41s)
RESULTS - ssh.SSHTest.test_ssh: PASSED (1.18s)
RESULTS - scp.ScpTest.test_scp_file: PASSED (1.33s)
RESULTS - systemd.SystemdBasicTests.test_systemd_basic: PASSED (0.87s)
RESULTS - systemd.SystemdServiceTests.test_systemd_status: SKIPPED (requires avahi-daemon)
# ... (其余用例行省略)
RESULTS - ptest.PtestRunnerTest.test_ptestrunner_expectsuccess: PASSED (312.55s)
SUMMARY: tiger-image () - Ran 17 tests in 342.101s
OK (skipped=6)
```

（输出形态以实测为准。）ptest 用例那三百多秒是在板上真跑全套 ptest——busybox 几百个用例加 tiger-sysinfo 两条，结果经 SSH 收回，自动汇进 SUMMARY。至此，13.2 在板上手敲的 `ptest-runner`，被镜像级考试收编了。

> **⚠️ 注意**：前言推荐的那套 Docker 环境里没有 KVM——`QEMU_USE_KVM ?= "1"` 写着 1，但 `oe.types.qemu_use_kvm`（oe/types.py 第 176-188 行）只在目标架构等于主机架构（或双 x86）时才真启用；x86_64 主机跑 qemuarm64 注定回落到纯模拟。慢是常态，`TEST_QEMUBOOT_TIMEOUT` 不够用就调它。

<!-- 【事实核查注记·V13】TEST_SUITES 七个词（ping/date/df/ssh/scp/systemd/ptest）+ 13.4 的 tiger_sysinfo 在 qemuarm64 + tiger-image（dropbear + root 公钥 + systemd）对照组上的逐词通过情况、dropbear 公钥认证实测形态（含串口 blank-password 警告行的逐字措辞），待真实环境实测回填，续 task13 的 V8~V12 编号。正文输出为示意形态，但用例 ID 已按源码实物落字（scp.ScpTest.test_scp_file、systemd.SystemdBasicTests.* 等）；scp 用例的通过依据已源码核实——OEHasPackage 集合参数是"至少装其一"语义（oeqa/runtime/decorator/package.py 第 55-60 行），packagegroup-core-ssh-dropbear 第 6 行 RRECOMMENDS openssh-sftp-server 在册即可跑；SKIPPED 明细（SystemdServiceTests 一族、ptest expectfail）与 SUMMARY 实际用例数随实测回填。 -->

<!-- 【事实核查注记·V14】无 KVM 容器（前言推荐的 Docker 环境）中 `bitbake tiger-image -c testimage` 的实际耗时、TEST_QEMUBOOT_TIMEOUT 是否需调大，待实测回填。QEMU_USE_KVM 回落行为已源码级确证（oe/types.py:176-188）。 -->

> **📖 深入阅读**：结果攒多了要纵向对比（这次比上次多挂了哪些用例、回归率趋势），官方工具是 `resulttool`（poky/scripts/ 下，按 tmp/log/oeqa 的 JSON 结果做存储、diff 与回归分析）。本章点到名字，chapter 16 的证据链还会回来用它。

<!-- 【待验证·阻塞级】C-W26：tiger 组的 testimage 接线需要额外配置，本章在对照组验证机制。tiger 组回填要点：其一，machine conf 的 `QB_DEFAULT_KERNEL = "none"`（10.3.3 接线）与 testimage 的直通内核启动冲突——testimage.bbclass 第 263-265 行按 `${KERNEL_IMAGETYPE}-${MACHINE}.bin` 拼内核路径递给 runqemu，而 tiger 全链要求从 BL1 经 NAND 起（-bios + NAND 后端参数经 QB_OPT_APPEND）；其二，slirp 网络接线；其三，NAND 全链下 boot pattern 与超时（BL1→BL31→U-Boot→kernel 全链跑完才见 login banner）。随仓库就位批次回填，续 C-W17~C-W25 同口径。 -->

### 13.4 tiger 自己的考题：自定义 oeqa/runtime 用例

"通用的测试替你验了 ssh 和 systemd。"老周说，"tiger 自己的东西，谁验？"

这正是阿凯早上在白板上给自己立的第五件。素材现成——12.6.2 他在板上手敲的那串验收命令：`systemctl status tiger-sysinfo`、`hostname`、`date`、`ls -l /etc/localtime` 一族。**自动化验证在这一节第一次有了具体含义：把那串手敲的命令，一条条对进测试代码。**（`which` 那条属于工具清点，不进考题；`date` 有官方 DateTest 在清单里罩着，也不自写。）

先找"自定义用例放哪"的机制答案。testimage.bbclass 里有个函数：

```python
# 文件路径：~/workspace/poky/meta/classes-recipe/testimage.bbclass（节选，406-418 行）
def get_runtime_paths(d):
    """
    Returns a list of paths where runtime test must reside.

    Runtime tests are expected in <LAYER_DIR>/lib/oeqa/runtime/cases/
    """
    paths = []

    for layer in d.getVar('BBLAYERS').split():
        path = os.path.join(layer, 'lib/oeqa/runtime/cases')
        if os.path.isdir(path):
            paths.append(path)
    return paths
```

约定一句话：每个 layer 把自己的运行时用例放在 `lib/oeqa/runtime/cases/` 下，装载器按 BBLAYERS 逐个收集。这个目录不在 BBFILES 通配里（它装的是 Python 用例不是配方），layer.conf 零改动。装载机制说清楚：它不走 Python 的模块搜索路径——get_runtime_paths 按 BBLAYERS 在文件系统上逐层拼出 cases/ 目录，loader 的 discover（oeqa/core/loader.py 第 273-279 行）直接按路径做 unittest discover。用例文件里 `from oeqa...` 能 import，靠的是 oe-core 自己的 layer.conf 第 135 行写了 `addpylib ${LAYERDIR}/lib oe`，把 poky/meta/lib 放上 sys.path——那是 poky 自带的动作，meta-tiger 不用管。落位：

```bash
# meta-tiger 的用例目录
mkdir -p ~/workspace/meta-tiger/lib/oeqa/runtime/cases
```

用例全文——本章的核心交付物。Python 语法就地认：`class` 声明用例类，`@` 开头的是装饰器（给方法贴条件的标记）——不熟也没关系，下面逐个讲：

```python
# 文件路径：~/workspace/meta-tiger/lib/oeqa/runtime/cases/tiger_sysinfo.py（新建）
# tiger 自己的运行时考题：把 12.6.2 手敲的验收命令搬进测试代码（13.4）

from oeqa.runtime.case import OERuntimeTestCase
from oeqa.core.decorator.depends import OETestDepends
from oeqa.runtime.decorator.package import OEHasPackage


class TigerSysinfoTest(OERuntimeTestCase):

    @OETestDepends(['ssh.SSHTest.test_ssh'])
    @OEHasPackage(['tiger-sysinfo'])
    def test_sysinfo_service_ran(self):
        # 12.3 的 oneshot 服务：跑完即退，证据留在 journal 里
        status, output = self.target.run('journalctl -u tiger-sysinfo --no-pager')
        self.assertEqual(status, 0, msg='journalctl failed: %s' % output)
        self.assertTrue('tiger-sysinfo: board=' in output,
                        msg='tiger-sysinfo journal line not found: %s' % output)

    @OETestDepends(['ssh.SSHTest.test_ssh'])
    @OEHasPackage(['tiger-sysinfo'])
    def test_hostname(self):
        # 12.4.1：产品主机名是 tiger-iot
        status, output = self.target.run('cat /etc/hostname')
        self.assertEqual(status, 0, msg='cat /etc/hostname failed: %s' % output)
        self.assertEqual(output, 'tiger-iot',
                         msg='hostname mismatch: %s' % output)

    @OETestDepends(['ssh.SSHTest.test_ssh'])
    @OEHasPackage(['tiger-sysinfo'])
    def test_timezone(self):
        # 12.4.2：/etc/localtime 指向 Asia/Shanghai
        status, output = self.target.run('readlink -f /etc/localtime')
        self.assertEqual(status, 0, msg='readlink failed: %s' % output)
        self.assertTrue(output.endswith('Asia/Shanghai'),
                        msg='localtime points to %s' % output)
```

逐段讲。**OERuntimeTestCase** 就地解释：运行时测试用例的基类（oeqa/runtime/case.py 第 10 行），装载器会给每个用例注入 `self.target`——被测板的抽象，`self.target.run('命令')` 经 SSH 在板上执行，返回 `(状态码, 输出)` 二元组。

三个测试方法对应 12.6.2 手敲的那串命令，但不是照抄——写法换了三处，各有理由：`systemctl status tiger-sysinfo` 换 `journalctl -u tiger-sysinfo`，因为 oneshot 服务跑完即退，`status` 看不到活着的进程，证据在 journal；`hostname` 换 `cat /etc/hostname`，拿到的字符串直接比对；`ls -l /etc/localtime` 换 `readlink -f`，要的是链接指向本身，不是一行的显示文本。

方法头上的两行装饰器：**OETestDepends** 就地解释——声明用例依赖，`['ssh.SSHTest.test_ssh']` 表示"SSH 通道的用例先通，不通则本用例直接跳过"——13.3.2 第一跑那片 SKIPPED 就是它干的（跑不通 SSH，后面任何板上命令都是空谈）；**OEHasPackage** 就地解释——镜像里没装所列包就自动 skip 而非 fail（oeqa/runtime/decorator/package.py 第 62-63 行），同一个用例文件将来喂给没装 tiger-sysinfo 的镜像也安全。

接入清单，重跑：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro-dev.conf（编辑，清单行换为八词）
TEST_SUITES = "ping date df ssh scp systemd ptest tiger_sysinfo"
```

```bash
# 重跑镜像级测试（tiger_sysinfo 三用例应在列）
bitbake tiger-image -c testimage
```

输出（关键行示意，以实测为准，V13 同批回填）：

```text
RESULTS - tiger_sysinfo.TigerSysinfoTest.test_hostname: PASSED (0.92s)
RESULTS - tiger_sysinfo.TigerSysinfoTest.test_sysinfo_service_ran: PASSED (1.05s)
RESULTS - tiger_sysinfo.TigerSysinfoTest.test_timezone: PASSED (0.88s)
```

> **⚠️ 注意**：模块名必须**全小写**。装载器按正则认名字，假设就是"包名和模块名不含大写、类名才有"（oeqa/core/loader.py 第 47-49 行的注释与正则）；写成大写不会报错，只会被警告后**静默跳过**（第 50-54 行）——测试根本没跑，SUMMARY 里用例数对不上才能察觉。同族的静默还有一种：清单里写了不存在的模块名，同样不报错、悄悄缺席。这门功课的结账姿势只有一个——数 SUMMARY 里的用例数。

### 13.5 buildhistory：谁在改我的镜像

12.7.2 的 💡 框留过一句话："构建历史机制专门治这个……chapter 13 正式启用，今天先记住名字。"今天收账。

**构建历史（buildhistory）** 本章正式启用：Yocto 的构建产物变更追踪机制——每次构建结束，自动把包版本、包清单、镜像内容与体积落盘存档，配合 git 提交，任何两次构建之间"谁变了"都有据可查。机制三行：

```bitbake
# 文件路径：~/workspace/poky/meta/classes/buildhistory.bbclass（节选，14-15/44 行）
BUILDHISTORY_FEATURES ?= "image package sdk"
BUILDHISTORY_DIR ?= "${TOPDIR}/buildhistory"
# ...（19-37 行 BUILDHISTORY_RESET 注释段，见下方 ⚠️ 框）
BUILDHISTORY_COMMIT ?= "1"
```

**BUILDHISTORY_FEATURES** 就地解释：记录范围（镜像/包/SDK 三类，默认全记）；`BUILDHISTORY_DIR` 默认落在构建目录下的 `buildhistory/`；**BUILDHISTORY_COMMIT** 就地解释——默认 `1`，每次构建自动把变更 git 提交进存档目录（存档目录自己就是一个 git 仓库）。落点裁决：`INHERIT` 写进 base 而不是 dev delta——构建记录是项目级的工程行为，开发态和量产态都要记账，进仓库：

```bitbake
# 文件路径：~/workspace/meta-tiger/conf/distro/tiger-distro.conf（追加）
# 构建历史（13.5）：两态都记录，存档目录自成 git 仓库、每次构建自动提交
INHERIT += "buildhistory"
```

阿凯落完盘叹了口气："13.2 把几百个 ptest 包装进镜像那笔账，记不上了——历史只能从开启那天记起。"

"所以演示换一个素材。"老周说，"把 12.7.2 那颗雷再点一次。"

演示闭环四步：开启后先构建一次留基线；给 tiger-image 临时加回 `python3`（坑 2 的原凶）再构建；diff；撤掉。

```bash
# 第一步：基线构建（ptest 姿态，分钟级，sstate 大部分命中）
bitbake tiger-image

# 第二步：tiger-image.bb 的 IMAGE_INSTALL 临时追加 python3（演示用，验完即撤），再构建
bitbake tiger-image

# 第三步：最近两次构建的差异
# （依赖 GitPython——没有先 sudo apt install -y python3-git；scripts/buildhistory-diff 第 15-20 行，缺它直接报错退出）
buildhistory-diff

# 顺手看一眼存档目录的自动提交形态
cd ~/workspace/build-distrotest/buildhistory && git log --oneline -3 && cd ..
```

`buildhistory-diff` 输出（关键行示意，以实测为准）：

```text
Changes to images/qemuarm64/glibc/tiger-image (files-in-image.txt):
  /usr/bin/python3 was added
  ...
Changes to images/qemuarm64/glibc/tiger-image (installed-package-names.txt):
  python3-core was added
  libpython3.12-1.0 was added
images/qemuarm64/glibc/tiger-image: IMAGESIZE changed from 36864 to 54528 (+48%)
```

12.7.2 那个"哪个包把镜像喂胖"的问题，第一次有了不用人肉翻日志的自动答案——文件与包清单的增项（`was added` 一族）、体积的涨跌（IMAGESIZE 按 KiB 记、涨跌给百分比），一翻就到。演示完把 python3 从清单里撤掉。

> **⚠️ 注意**：diff 不是读起来都那么痛快。阿凯第一回把全量 diff 从头读到尾，半小时没读完——存档是"全都要"的，读的人得会过滤。三个治理手段：
>
> 1. 不需要的类别裁掉：`BUILDHISTORY_FEATURES` 按 "image package sdk" 三个词裁剪；
> 2. 时间戳类噪音认得就好：镜像构建时间、DATETIME 段文件名每次必变，不是回归；
> 3. CI 全量重建的场景，知道 `BUILDHISTORY_RESET` 的存在即可（类文件第 19-37 行注释段：置位则清掉旧存档只留本次，包"消失"也能被记录，专门给全量构建用）。

### 13.6 瞭望两则：checkpkg/AUH 与 yocto-check-layer

两件今天不做实操、但必须知道存在的事。

第一则是上游哨兵。**版本检查（checkpkg）** 本章轻量引入：检查所有配方的上游有没有新版本。先纠正一个容易复活的老印象：checkpkg 原本挂在 distrodata.bbclass 的任务上，而 Scarthgap 里这个类已经退役——poky 的 scripts/ 下没有这条命令：grep scripts/ 命中的只是 resulttool 里对残留用例名的四处字符串引用（scripts/lib/resulttool/regression.py 第 62/70/122/124 行），用例本体则只剩 oe-selftest 里一个同名残留（selftest/cases/distrodata.py 第 13 行）。现行姿势两路。其一是 Scarthgap 原生实物：

```bash
# 检查单个（或全部）配方的上游新版本状态（scripts/lib/devtool/upgrade.py 第 645/688 行）
devtool check-upgrade-status <配方名>      # 省略配方名则扫全部
```

输出是每个配方的上游版本核对表（新版可用/已是最新/无法判定一族状态）。其二是 **AUH（Automatic Upgrade Helper）**，本章随之一并轻量引入：Yocto 社区的自动升级助手，独立仓库（auto-upgrade-helper），把这套检查做成无人值守——自动检测上游新版本并生成升级补丁；其 README 只记载 `upgrade-helper.py <配方名>|all` 的用法，社区旧文档里"checkpkg 是 AUH 子命令"的说法未经现行 master 证实（挂账 V16）。AUH 依赖外网扫上游，本书环境不实测，输出形态以仓库文档为准。

今天记住名字就够——它们的戏在后面：chapter 15 修上游 bug、chapter 16 合规发布固定版本时，这个哨兵是情报来源。

<!-- 【事实核查注记·V16】checkpkg 作为 AUH 子命令的说法未经现行 auto-upgrade-helper master 证实（可查 README 仅载 upgrade-helper.py 用法），正文已降级为待核口径；主口径 devtool check-upgrade-status 为本地实物（upgrade.py:645/688），已核实。AUH 确切命令形态与输出文案依赖外网与独立仓库，待人工验证回填，续 task13 的 V8~V12 编号。 -->

第二则是老熟人换身份。yocto-check-layer 在 3.4 节就正式引入过，机制不重讲；今天它完成一次身份转换——从"手动体检"变成"流水线门禁的第一步"。实操不再单跑，直接并入下一节的 CI YAML。一个细节随 layer 的成长变了：meta-tiger 从 chapter 6 起声明了 `LAYERDEPENDS = "core meta-arm"`，检查时要按 `--dependency` 把依赖层指给它（3.4 节时 meta-tiger 只依赖 core，用不到这个参数）。

### 13.7 接入 CI：把今天敲的命令排成流水线

回到早上那块报红的面板。老周先定编排思路："别想复杂——**把今天敲过的命令按顺序排一遍，就是流水线。**"对照一下：13.2 重建镜像是"构建"，13.3 的 `-c testimage` 是"测试"，13.5 的 buildhistory-diff 是"报告"，3.4 的 yocto-check-layer 顶在最前当"门禁"。四级流水线，每一级的每条命令今天都已在本机手跑通过。

场景设定与两处裁决先交代。公司内部 GitLab（`<internal-git-server>` 一族，前文惯例）；CI 配置随 BSP layer 走——`.gitlab-ci.yml` 放 meta-tiger 仓库根，跟代码一起 review、一起打 tag。runner 用 Docker executor，镜像按 chapter 1 的依赖清单烤制（ubuntu:24.04 一族），读者已有心智模型。两态裁决落给老周："**出货镜像没留测试的门，测试镜像不是出货镜像；两个都建，只测有门的那个。**"——dev、prod 都进 build 级（保证 prod 不因测试配置腐坏），test 级只有 dev 一个 job。

全文落盘，读者第一次接触 CI YAML，逐段讲：

```yaml
# 文件路径：~/workspace/meta-tiger/.gitlab-ci.yml（新建）
# tiger BSP 流水线（chapter 13）：check → build → test → report
# runner：Docker executor；镜像按 chapter 1 的依赖清单烤制（ubuntu:24.04 一族）

stages:
  - check
  - build
  - test
  - report

default:
  image: "<internal-registry>/yocto-build-env:ubuntu-24.04"

# MACHINE / DISTRO / IMAGE 参数化为 CI 变量：他日复用，改值即可
variables:
  MACHINE: "qemuarm64"
  IMAGE: "tiger-image"
  DISTRO_DEV: "tiger-distro-dev"
  DISTRO_PROD: "tiger-distro-prod"
  BUILD_DIR: "build-ci"

# 下载缓存与 sstate 跨 job 复用（poky / meta-arm 一并缓存，省得每个 job 重拉）
cache:
  key: tiger-scarthgap
  paths:
    - poky/
    - meta-arm/
    - build-ci/downloads/
    - build-ci/sstate-cache/

# 公共准备（YAML 锚点，各 job 引用）：外部仓库克隆（命中缓存则跳过）、
# 构建目录初始化、layer 注册——与 11.8.2 建对照组同一条依赖链顺序
.setup: &setup |
  set -e
  if [ ! -d poky ]; then git clone -b scarthgap git://git.yoctoproject.org/poky.git poky; fi
  if [ ! -d meta-arm ]; then git clone -b scarthgap git://git.yoctoproject.org/meta-arm.git meta-arm; fi
  cd poky
  source oe-init-build-env ../$BUILD_DIR
  bitbake-layers add-layer ../meta-arm/meta-arm-toolchain
  bitbake-layers add-layer ../meta-arm/meta-arm
  bitbake-layers add-layer "$CI_PROJECT_DIR"

# 公共 local.conf 注入（dev 态）：策略都在仓库里，CI 只补机器名/路径/并行度
.configure-dev: &configure-dev |
  {
    echo "MACHINE = \"$MACHINE\""
    echo "DISTRO = \"$DISTRO_DEV\""
    echo "DL_DIR = \"$CI_PROJECT_DIR/$BUILD_DIR/downloads\""
    echo "SSTATE_DIR = \"$CI_PROJECT_DIR/$BUILD_DIR/sstate-cache\""
  } >> conf/local.conf

# 第一级 check：layer 规范体检当门禁（3.4 节工具的流水线身份）；
# 注意 meta-tiger 不进 bblayers，检查器自带环境；
# --dependency 是 nargs="+" 会吞掉后面所有参数——被检层必须放它前面
check:layer:
  stage: check
  script:
    - if [ ! -d poky ]; then git clone -b scarthgap git://git.yoctoproject.org/poky.git poky; fi
    - if [ ! -d meta-arm ]; then git clone -b scarthgap git://git.yoctoproject.org/meta-arm.git meta-arm; fi
    - cd poky && source oe-init-build-env ../build-check
    - yocto-check-layer "$CI_PROJECT_DIR" --dependency ../meta-arm/meta-arm-toolchain ../meta-arm/meta-arm

# 第二级 build：两态都建（裁决见正文），buildhistory 随构建自动记账
build:dev:
  stage: build
  script:
    - *setup
    - *configure-dev
    - bitbake $IMAGE
  artifacts:
    paths:
      - build-ci/buildhistory/
    expire_in: 1 week

build:prod:
  stage: build
  script:
    - *setup
    - |
      {
        echo "MACHINE = \"$MACHINE\""
        echo "DISTRO = \"$DISTRO_PROD\""
        echo "DL_DIR = \"$CI_PROJECT_DIR/$BUILD_DIR/downloads\""
        echo "SSTATE_DIR = \"$CI_PROJECT_DIR/$BUILD_DIR/sstate-cache\""
      } >> conf/local.conf
    - bitbake $IMAGE
  artifacts:
    paths:
      - build-ci/buildhistory/
    expire_in: 1 week

# 第三级 test：只测 dev（prod 无门）；私钥走 masked CI 变量，永不进仓库
test:dev:
  stage: test
  script:
    - *setup
    - *configure-dev
    # 13.3.2 的会话级接线，CI 注入版：slirp 网络
    # （ssh-agent 变量的白名单 oe-init-build-env 默认已放行，无需额外动作）
    - echo 'TEST_RUNQEMUPARAMS = "slirp"' >> conf/local.conf
    - eval $(ssh-agent -s)
    # masked 变量只能存单行：私钥以 base64 编码进变量，这里解码喂给 ssh-add
    - echo "$TIGER_TEST_SSH_KEY" | base64 -d | ssh-add -
    # -c testimage 不替你建镜像（13.3.2）：全新容器里必须先建，缺 testdata.json 会当场 fatal
    - bitbake $IMAGE
    - bitbake $IMAGE -c testimage
  artifacts:
    # 失败也要收日志：结果与 boot log 都在这里（13.3.2）
    when: always
    paths:
      - build-ci/tmp/log/oeqa/
    expire_in: 1 week

# 第四级 report：测试结果为 artifacts，buildhistory 差异留档（chapter 16 证据链伏笔）
report:
  stage: report
  script:
    - *setup
    - *configure-dev
    # 恢复构建现场（sstate 命中，分钟级），让 buildhistory 记上本次
    - bitbake $IMAGE
    - cd ../$BUILD_DIR/buildhistory && git log --oneline -5 && cd -
    - cd ../$BUILD_DIR && buildhistory-diff | tee $CI_PROJECT_DIR/buildhistory-diff.txt
  artifacts:
    paths:
      - build-ci/buildhistory/
      - buildhistory-diff.txt
    expire_in: 1 week
```

逐段过。`stages` 声明四级；`default.image` 是 runner 拉起的容器镜像；`variables` 把 MACHINE/DISTRO/IMAGE 提成参数；`cache` 声明跨 job 复用的目录。`.setup` 和 `.configure-dev` 是 YAML 锚点（`&名字` 定义、`*名字` 引用）——每个 job 开头都把 11.8.2 建对照组的手工动作重演一遍：克隆外部仓库、初始化构建目录、按依赖链注册 layer。注意 local.conf 里只写机器名、缓存路径这类环境配置——策略一行没有，策略全在仓库里（11.10 节立下的规矩在 CI 里自动兑现）。

> **⚠️ 注意**：流水线里**每个 job 都是一台全新容器**——job 之间不共享 shell 环境、不共享工作目录，只有显式声明的 cache 和 artifacts 会传递。所以 `source oe-init-build-env` 和 `bitbake` 必须呆在同一个 job 里（同一个 job 内，script 各行共享同一个 shell，`cd` 和 `source` 的效果持续到 job 结束）；想让产物跨 job，走 artifacts。新人第一次写 CI 最常踩的就是"上一个 job source 过，这个 job 直接用"。

`test:dev` 里有三处值得停留。其一，`TIGER_TEST_SSH_KEY` 是 GitLab 的 masked CI 变量——masked 变量要求值是单行，私钥是多行文本存不进去，所以变量里躺的是私钥的 base64 单行编码，job 里 `base64 -d` 解码喂给 ssh-add；公钥早已随 tiger-testkey 进了仓库，私钥永不进仓库。其二，`bitbake $IMAGE` 先行——13.3.2 埋的那条伏笔在这里兑现：`-c testimage` 不替你建镜像，本地手跑时镜像早建好了没感觉，CI 每个 job 都是全新容器，缺了 testdata.json 会当场 bb.fatal（testimage.bbclass 第 220 行）。其三，`when: always` 保证测试挂了日志也照收，不然报告级收到的只有一片绿时的祥和。

> **💡 提示**：variables 里 MACHINE/DISTRO 都是参数——falcon 项目来的时候，复制这份 YAML 改几个值就是它的流水线（呼应上一章末"配一个新 MACHINE 的事"）。到那天再展开。

验证口径交代清楚：这份 YAML 里流水线的每条命令，本章前面都已在本机手跑通过（13.2/13.3/13.5 逐节即逐步骤，check 级是 3.4 节的老动作）；YAML 本体做语法级校验：

```bash
# YAML 语法校验（无 PyYAML 则先 sudo apt install -y python3-yaml）
python3 -c "import yaml; yaml.safe_load(open('$HOME/workspace/meta-tiger/.gitlab-ci.yml'))" && echo "YAML OK"
```

远端 runner 的真实执行（runner 注册、registry 里的构建镜像、缓存盘挂载策略）依赖公司内部环境，本书无法本地验证，挂账 V15；GitLab 网页端的 CI lint 校验器可以替代本地做更严格的检查，知道它的存在即可。

<!-- 【事实核查注记·V15】.gitlab-ci.yml 远端真实执行（runner 镜像 <internal-registry>/yocto-build-env、DL_DIR/SSTATE_DIR 缓存策略、SSH 私钥 base64 单行 masked 变量注入、artifacts 体积）待公司内部 GitLab 环境实测回填，续 task13 的 V8~V12 编号。本地证据链：流水线每条命令本章已手跑通过 + YAML 语法校验通过。 -->

叙事收个尾：下周三傍晚，阿凯把 `.gitlab-ci.yml` 合进 meta-tiger，面板上流水线四级依次转绿——check、build×2、test、report。早上那片红，收摊了。

### 13.8 踩坑实录

#### 13.8.1 坑 1：自写的 ptest 在板上跑挂——构建期的东西不在板上

发作过程见 13.2.3：第一稿 run-ptest 想用 `cmp` 跟"构建时的原件"对一遍，`do_install_ptest` 里 sed 把 `${WORKDIR}` 展开的主机绝对路径填进脚本——构建期风平浪静，板上 `ptest-runner` 报 `FAIL: script-mismatch`。

机理一句话：**ptest 的运行现场是目标板**。run-ptest 里能引用的只有板上存在的文件与命令；任何构建期路径（${WORKDIR}、${S}、${B}）展开的都是主机侧的字面，板上不存在。官方也踩在同一处——ptest.bbclass 第 65-78 行的 PTEST_BUILD_HOST_FILES sed 段，专职剥离装进 -ptest 包的 Makefile 里的主机路径。

族谱认门：这个坑既不是 10.8 节归过档的"什么都不发生"（它明确报了 FAIL），也不完全是"参数放行型"（工具不是放行错参数，是执行现场换了）——它是新一族：**构建期不拦、运行时才炸**。跟 8.11.1 直接改工作区文件那个坑是近亲：都是"构建现场与运行现场傻傻分不清"。

纪律落纸：写完 run-ptest，逐行问自己"这条命令板上有没有、这个文件板上在不在"；板上 `/bin/sh` 是 busybox ash，bash 扩展语法也别用。

#### 13.8.2 坑 2：testimage 裸跑全灭——系统起来了，没人应门

发作过程见 13.3.2，两连击。第一击：挂上 IMAGE_CLASSES 就跑，结果 SSH 用例 FAILED、依赖它的一整片 SKIPPED（Connection refused）——镜像里根本没有 SSH 服务，测试命令进不了板。第二击：装上 dropbear（ssh-server-dropbear 特性词装进镜像的轻量 SSH 服务端）再跑，换成 Permission denied——12.4.4 把空密码退役了，而 oeqa 的 SSH 通道没有任何递密码的机制（sshcontrol.py 第 107-113 行的选项清单为证；qemurunner 那句 "using blank password" 警告是官方出厂假设的旁证）。

两击之间藏着本章最值钱的一个排查动作：老周没有给答案，只问"boot log 看了吗？"阿凯从 `tmp/log/oeqa/tiger-image/qemu_boot_log.*` 里看到 login 提示符端端正正躺在末尾，才把"系统没起来"和"系统起来了但没应门"分开——前者查启动链，后者查门。**超时和失败的第一动作都是看 boot log**；`TEST_QEMUBOOT_TIMEOUT ?= "1000"` 是"没起来"一族的调参位。

修正动作的回指链也值得复盘：门（ssh-server-dropbear）与钥匙（tiger-testkey 公钥 + ssh-agent）全部只落 dev 态——**测试的门是 dev 态的明确表态**，prod 镜像无门无钥匙，跟 12.4.4 空密码退役是同一条逻辑的两端。而环境变量透传白名单这条通道，1.6.2 代理变量走过的那条，本章第二次用上——这次连放行动作都免了：oe-init-build-env 的默认名单（oe-buildenv-internal 第 112 行）早就替我们放行了。

### 13.9 本章小结

白板五件事回顾：

- **13.1 地图**：oeqa 一家——ptest（包级、板上跑）、testimage（镜像级、QEMU+SSH 通道）、oe-selftest（构建时，点名）；ptest 的结果常借 testimage 的通道收回。
- **13.2 ptest**：机制实物（ptest.bbclass：PTEST_ENABLED 总开关、PACKAGES =+ -ptest、PTEST_PATH 落点、RRECOMMENDS ptest-runner）；run-ptest 三段式，ptest 无 `-c ptest` 任务、执行现场永远在板上；ptest-pkgs 特性词（三级证据链，回指 12.5 口诀）；落点裁决"测试的姿态不是出货的姿态"——DISTRO_FEATURES ptest + ptest-pkgs 全落 dev delta（11.3 砍掉的旧账今日 dev 态加回）；对照组板上 `ptest-runner busybox` 实测；tiger-sysinfo 自写 ptest 全闭环（坑 1 在此发作）。
- **13.3 testimage**：IMAGE_CLASSES 挂钩、`-c testimage` 手动档（不替你建镜像，testdata.json 缺失当场 fatal）；TEST_TARGET=qemu 骑 runqemu 背上（qemurunner 拼 runqemu snapshot 命令行，QB_* 老本全兑现）；SSH 命令通道与 slirp；TEST_SUITES 裁剪到七词（parselogs 排除）；坑 2 两连击——装门（ssh-server-dropbear）+ 配钥匙（公钥 + ssh-agent，白名单 oe-init-build-env 默认已放行）；结果落 tmp/log/oeqa，resulttool 点名。tiger 组接线挂 C-W26。
- **13.4 自定义用例**：`<layer>/lib/oeqa/runtime/cases/` 约定（get_runtime_paths 按 BBLAYERS 在文件系统收集，layer.conf 零改动）；tiger_sysinfo.py 三用例把 12.6.2 手敲的验收命令逐条对进测试代码（写法换三处，各有理由）；⚠️ 模块名全小写、不存在的模块名静默缺席——数 SUMMARY 用例数结账。
- **13.5 buildhistory**：正式启用（还 12.7.2 的账）；INHERIT 落 base（两态都记账）；BUILDHISTORY_COMMIT 默认自动 git 提交；用 python3 重演坑 2 做 diff 演示——"哪个包把镜像喂胖"第一次有自动答案；⚠️ 噪音治理三手段（BUILDHISTORY_FEATURES 裁剪、时间戳噪音识别、BUILDHISTORY_RESET 点名）。
- **13.6 瞭望**：上游哨兵——checkpkg 已随 distrodata.bbclass 消亡，现行姿势 devtool check-upgrade-status（本地实物）/ AUH（独立仓库，挂 V16，伏笔 chapter 15/16）；yocto-check-layer 身份转换为流水线门禁（回指 3.4，注意 --dependency 吞参、被检层前置）。
- **13.7 CI**：`.gitlab-ci.yml` 四级流水线（check/build×2/test/report），"把今天敲过的命令按顺序排一遍"；两态裁决——都建、只测有门的；test:dev 先行 `bitbake $IMAGE`（全新容器无 testdata.json）；⚠️ job 间环境不共享；本地证据链 + YAML 语法校验，远端执行挂 V15；💡 falcon 参数化复用一句。

本章产出清单：

- `meta-tiger` 新增：`lib/oeqa/runtime/cases/tiger_sysinfo.py`；`.gitlab-ci.yml`；`recipes-core/tiger-sysinfo/tiger-sysinfo/run-ptest`；`recipes-core/tiger-testkey/`（`tiger-testkey_1.0.bb` + `tiger-testkey/authorized_keys`）。
- `meta-tiger` 编辑：`recipes-core/tiger-sysinfo/tiger-sysinfo_1.0.bb`（SRC_URI 加 run-ptest、`inherit ptest`）；`conf/distro/tiger-distro-dev.conf`（ptest 段 + testimage 段，含 ssh-server-dropbear 与 tiger-testkey 两笔）；`conf/distro/tiger-distro.conf`（INHERIT buildhistory 一行）。

提交并打 tag：

```bash
# 提交本章产出并标记终点（在 meta-tiger 仓库操作）
cd ~/workspace/meta-tiger
git add -A
git status
git commit -m "Add automated testing: ptest, testimage, buildhistory and CI pipeline"
git tag chapter13
```

> **⚠️ 注意**：tag 归属——`meta-tiger` 打 `chapter13`，它是本章唯一有改动的我方仓库；build-distrotest 的 local.conf 会话级追加（TEST_RUNQEMUPARAMS）不进仓库，沿用惯例；poky、meta-arm 与四个开发态仓库本章无改动，不打。另：tiger-testkey 里的 authorized_keys 是**公钥**，可以进仓库；私钥永远只在构建主机和 CI 的 masked 变量（base64 单行编码）里。

后续任务清单：

- **task 15 / chapter 14 SDK 交付**：`populate_sdk` 产出的 SDK 与镜像同一条流水线兜底——每次改动都有自动验证。
- **task 16 / chapter 15 修上游 bug**：checkpkg/AUH 的上游情报伏笔回收；补丁合入有流水线验证没回归。
- **task 17 / chapter 16 合规交付**：CI report 级的 artifacts 与 buildhistory 存档是"交付清单 + 构建日志"证据链的底稿。

---

**延伸阅读**

1. Yocto 测试手册（ptest、testimage、oeqa 框架与 resulttool）：https://docs.yoctoproject.org/5.0/test-manual/index.html
2. Yocto 参考手册 class 章节（ptest.bbclass、testimage.bbclass、buildhistory.bbclass）：https://docs.yoctoproject.org/5.0/ref-manual/classes.html
3. Yocto 变量术语表（TEST_SUITES、TEST_RUNQEMUPARAMS、BUILDHISTORY_FEATURES 等条目）：https://docs.yoctoproject.org/5.0/ref-manual/variables.html
4. auto-upgrade-helper 仓库（AUH 用法与输出形态，以其 README 为准）：https://git.yoctoproject.org/auto-upgrade-helper/
5. GitLab CI YAML 参考（stages/cache/artifacts/anchors 语法与 CI lint 校验器）：https://docs.gitlab.com/ee/ci/yaml/
