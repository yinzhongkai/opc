# 终稿 — 《Yocto BSP 实战：从零打造一块虚拟开发板》

逐 task 写作的终稿存放于此。文件名遵循 `configs/numbering.md` 双轨编号：
`taskNN-<chapter号或边界名>-<标题>.md`（第一段 task 编号溯源工作流，第二段 chapter 号/边界名定位内容）。

| task | chapter | 文件 | 标题 |
|------|---------|------|------|
| 00 | 前言 | `task00-前言.md` | 前言 |
| 01 | 序章 | `task01-序章-项目启动会.md` | 项目启动会 |
| 02 | 1 | `task02-1-把Yocto跑起来.md` | 把 Yocto 跑起来 |
| 03 | 2 | `task03-2-读懂这个项目.md` | 读懂这个项目 |
| 04 | 3 | `task04-3-搭起meta-tiger的骨架.md` | 搭起 meta-tiger 的骨架 |
| 05 | 4 | `task05-4-让QEMU长出tiger这块板.md` | 让 QEMU 长出 tiger 这块板 |
| 06 | 5 | `task06-5-写第一份MACHINE配置.md` | 写第一份 MACHINE 配置 |
| 07 | 6 | `task07-6-集成TF-A：让芯片"上电".md` | 集成 TF-A：让芯片"上电" |
| 08 | 7 | `task08-7-集成U-Boot：让板子"会启动".md` | 集成 U-Boot：让板子"会启动" |
| 09 | 8 | `task09-8-集成LinuxKernel：让板子"活过来".md` | 集成 Linux Kernel：让板子"活过来" |
| 10 | 9 | `task10-9-NAND+UBI：把"硬盘"建好.md` | NAND + UBI：把"硬盘"建好 |
| 11 | 10 | `task11-10-打通启动链：第一次开机成功.md` | 打通启动链：第一次开机成功 |
| 12 | 11 | `task12-11-创建DISTRO：定义发行版策略.md` | 创建 DISTRO：定义发行版策略 |
| 13 | 12 | `task13-12-让镜像有用起来：工具与服务.md` | 让镜像有用起来：工具与服务 |
| 14 | 13 | `task14-13-自动化验证：ptest、testimage与CI.md` | 自动化验证：ptest、testimage 与 CI |
| 15 | 14 | `task15-14-交付SDK：从生产到使用.md` | 交付 SDK：从生产到使用 |

> 完整章节规划见 `outline/yocto/index.md`，写作进度见 `state/yocto/progress.md`。
