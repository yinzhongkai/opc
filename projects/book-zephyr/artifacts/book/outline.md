# 全书目录与依赖

> v0.2，已经用户确认（D-003、D-004）。章节标识稳定；阅读顺序和显示章号只在本表映射，不用章号推算任务或成果编号。

- 项目：book-zephyr / 成果 ID：A-002 / 关联任务：T-001（确认落实：T-002）
- 负责人：planner / 版本：0.2 / 更新时间：2026-09-13
- 成果状态：approved
- 适用范围：首本书的阅读顺序、章节划分与依赖关系；各章细节待章节蓝图
- 来源及输入版本：图书设计 A-001 v0.2；任务 T-001；用户 2026-09-13 在 planner 会话逐条确认（D-001～D-005）；[PROJECT.md](../../PROJECT.md) 暂定书名与目标
- 批准依据：D-003、D-004（对应 v0.2 全篇）
- 版本记录：2026-09-12 v0.1 初版草案；2026-09-13 v0.2 按用户确认修订：新增第 10 章 Zephyr 系统服务、新增 FLPR 进阶篇、第 16 章增设分区布局小节、外设章按已定物料落到具体器件、收官项目形态定稿

## 阅读路径

| 顺序 | 显示章号或边界名 | 稳定章节标识 | 标题与作用 | 前置章节标识 | 蓝图与正文位置 |
|---|---|---|---|---|---|
| 1 | 导读 | ch-intro | 本书目标、读者画像、学习路径；**物料清单与采购要点**；硬件、资料与反馈方式说明 | 无 | 待创建 |
| 2 | 第 1 章 | ch-env-setup | **nRF54L15 SoC 架构导览**（M33 + VPR 协处理器、存储地图、电源域）；认识 DK；**Windows 下从零搭建 NCS/Zephyr 环境**；构建并烧录第一个程序上板运行（**建议的首个闭环章**） | ch-intro | 待创建 |
| 3 | 第 2 章 | ch-build-system | Zephyr 工程结构、CMake 构建、devicetree 与 Kconfig（借力读者 Linux 背景，重点讲 Zephyr 特有用法） | ch-env-setup | 待创建 |
| 4 | 第 3 章 | ch-kernel | 线程、调度、同步、中断与定时等内核服务（与 Linux 内核概念对照讲解） | ch-build-system | 待创建 |
| 5 | 第 4 章 | ch-gpio | GPIO 输出与输入、按键中断（板上 4 键 4 灯）；驱动模型首次实操 | ch-kernel | 待创建 |
| 6 | 第 5 章 | ch-uart-log | UART 与 Zephyr Logging；建立调试与观察手段 | ch-gpio | 待创建 |
| 7 | 第 6 章 | ch-timer-pwm | 硬件定时器与 PWM：LED 调光；**DRV2605L PWM 输入驱动 LRA 振动** | ch-gpio | 待创建 |
| 8 | 第 7 章 | ch-i2c-sensor | I2C 总线实战：**ICM-42688-P IMU**（寄存器地图、WHO_AM_I、数据就绪中断回接 GPIO）；**同总线挂 DRV2605L 教多从机** | ch-uart-log | 待创建 |
| 9 | 第 8 章 | ch-spi-flash | SPI 总线实战：板载 64 Mb Flash（Board Configurator 引脚切换）；IMU 的 SPI 模式对比 | ch-i2c-sensor | 待创建 |
| 10 | 第 9 章 | ch-adc-power | ADC 采样与电源管理入门；P6 电流测量；**DPPI 实验（外设联动、CPU 休眠）**；电池供电路径 | ch-gpio | 待创建 |
| 11 | 第 10 章 | ch-zephyr-services | **Zephyr 系统服务进阶**：k_work 工作队列、线程栈水位与内存管理、Shell、settings/NVS 持久化、电源管理子系统概览 | ch-kernel | 待创建 |
| 12 | 第 11 章 | ch-ble-basics | BLE 协议核心概念与 Zephyr BLE 协议栈结构（从零讲起，全书主线重点） | ch-kernel | 待创建 |
| 13 | 第 12 章 | ch-ble-broadcast | 广播与扫描实战：Beacon 与观察设备 | ch-ble-basics, ch-zephyr-services | 待创建 |
| 14 | 第 13 章 | ch-ble-gatt | 建立连接；自定义 GATT 服务与特征实战 | ch-ble-broadcast | 待创建 |
| 15 | 第 14 章 | ch-ble-security | 配对、绑定与连接参数；安全概念落地（绑定持久化呼应 settings；CRACEN/TrustZone 概念） | ch-ble-gatt | 待创建 |
| 16 | 第 15 章 | ch-ble-power | BLE 应用的低功耗测量与优化实战（电池供电下实测） | ch-ble-gatt, ch-adc-power | 待创建 |
| 17 | 第 16 章 | ch-dfu | 固件升级：MCUboot 与 BLE DFU；**分区布局独立小节（内部 RRAM + 外部 Flash 完整分区地图）**；外部 Flash 作升级槽（呼应第 8 章） | ch-ble-gatt, ch-spi-flash | 待创建 |
| 18 | 第 17 章 | ch-production | 走向产品：量产烧录、测试与认证路径概览 | ch-dfu | 待创建 |
| 19 | 第 18 章 | ch-final-project | **收官实战：磷酸铁锂电池供电的低功耗蓝牙 IMU 运动传感节点**（传感器 + BLE 上报 + 功耗优化；DRV2605L 振动反馈为可选扩展） | ch-ble-power, ch-i2c-sensor | 待创建 |
| — | 进阶篇（可选，无下游依赖） | ch-flpr-adv | **FLPR RISC-V 协处理器实战**：sysbuild 双核工程、M33 与 FLPR 间 IPC、FLPR 处理时序敏感任务 | ch-zephyr-services | 待创建 |
| — | 附录 | appendix-resources | 排错索引、术语表、资料与在线资源清单 | 无 | 待创建 |

## 依赖检查与替代路径

- 本版未发现循环依赖；各前置关系见上表。
- 术语引入顺序：devicetree/Kconfig（第 2 章）先于各外设章使用；Zephyr 内核对象（第 3 章）先于 GPIO 中断与 BLE 回调使用；k_work 与 settings（第 10 章）先于 BLE 实战章（第 12～15 章）使用；BLE 概念（第 11 章）先于全部 BLE 实战章；分区布局（第 16 章内小节）先于该章 DFU 实战。
- 第 10 章的定位：它不是外设也不是 BLE，而是 BLE 实战章的隐形前提（回调下推迟工作、绑定信息持久化、电源管理框架），故置于外设之后、BLE 实战之前。
- 可灵活阅读的路径：第 6～9 章（外设各章）在第 4、5 章之后大体可互换顺序；第 11 章起与第 6～9 章并行，仅依赖第 3 章，赶进度的读者可在外设章之前先进入 BLE（但第 12 章起需先读第 10 章）。
- 进阶篇 ch-flpr-adv 为可选独立章：多镜像构建与双核 IPC 属进阶内容，不作任何主线章的前置。
- 首个闭环建议：第 1 章（ch-env-setup）先走通"蓝图 → 草稿 → 用户照做 → 反馈修订"全流程，再批量展开后续章节。
- 进度只引用 TASKS 与成果索引，本表不维护第二份状态。

## 调整影响

- 2026-09-13 v0.1 → v0.2（用户确认，D-003、D-004）：
  - 新增 ch-zephyr-services（第 10 章），原 BLE 各章显示章号顺延 1 位；稳定标识不变，已有任务、成果、反馈不重编号。
  - ch-dfu 增设"分区布局"独立小节（内部 RRAM + 外部 Flash 分区地图），作为该章 DFU 实战的前置基础。
  - 新增 ch-flpr-adv 进阶篇（FLPR 支持情况经 Nordic 官方资料核实：cpuflpr 构建目标、sysbuild 多镜像、SoftPeripherals 均可用）。
  - 外设章落到已定物料：ch-i2c-sensor 主线传感器定为 ICM-42688-P（替代 v0.1 的泛化"传感器"）；ch-timer-pwm 增加 DRV2605L + LRA 实验；ch-adc-power 增加 DPPI 实验；ch-spi-flash 明确板载 64 Mb Flash 主线。
  - 收官项目形态定稿：磷酸铁锂电池供电的低功耗蓝牙 IMU 运动传感节点（振动反馈可选）。
  - I3C 话题移除（数据手册确认 nRF54L15 无 I3C 硬件）。
  - 受影响范围：均为 draft 期新增与细化，无既有正文、验证或反馈需要改写；后续调整将在此继续列出原因、相关决定或反馈、受影响章节及需改写或复核的范围。
