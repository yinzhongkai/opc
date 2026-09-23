# nRF54L15 DK 资料包

全部来自 Nordic Semiconductor 官方（docs.nordicsemi.com / nordicsemi.com），2026-09-12 下载。

## 文件清单

| 文件 | 说明 |
|---|---|
| `nRF54L15_nRF54L10_nRF54L05_Datasheet_v1.0.pdf` | **芯片数据手册**（Product Specification，940 页），nRF54L15/L10/L05 三合一，寄存器、电气参数、封装引脚全在这里 |
| `nRF54L15_DK_HW_User_Guide_v1.0.0.pdf` | **开发板用户指南**（31 页），板载资源、供电、按键/LED、电流测量、RF 测量、调试接口说明 |
| `PCA10156_Schematic_And_PCB.pdf` | **开发板原理图 + PCB 图**（从硬件包中单独抽出，方便直接看） |
| `PCA10156_nRF54L15-DK_硬件设计文件_1_0_0.zip` | **完整硬件设计包**：Altium 原理图/PCB 源文件、Gerber、BOM、钻孔、装配图 |
| `nRF54L15_Rev_2_Errata_v1.1.pdf` | **Rev 2 芯片勘误表**。DK 1.0.0 板上焊的是 Rev 2 芯片（nRF54L15-QFAAC00），看这个版本 |
| `ngl_001.pdf` | nRF54L 系列硬件设计指南（自己画板时的参考设计、RF 布局、电源去耦） |
| `nan_047.pdf` | nRF54L 系列量产烧录指南（批量生产时用） |

## 上手建议

1. 先看 DK 用户指南，了解板子布局和跳线/焊桥配置。
2. 软件开发用 **nRF Connect SDK**（基于 Zephyr），这颗芯片不支持老的 nRF5 SDK。
3. 查寄存器/电气参数用 Datasheet；自己设计板子时配合硬件设计指南（ngl_001）。

## 在线资源

- 在线文档主页：https://docs.nordicsemi.com （可在线翻阅以上所有文档的最新版）
- DK 快速上手：https://www.nordicsemi.com/Products/Development-hardware/nRF54L15-DK/GetStarted
- 硬件文件下载页（含历史版本）：https://www.nordicsemi.com/Products/Development-hardware/nRF54L15-DK/Hardware-files
- nRF Connect SDK 文档：https://docs.nordicsemi.com/bundle/ncs-latest/page/index.html
- Nordic DevAcademy 免费课程（nRF Connect SDK Fundamentals）：https://academy.nordicsemi.com
- 技术问答社区 DevZone：https://devzone.nordicsemi.com
