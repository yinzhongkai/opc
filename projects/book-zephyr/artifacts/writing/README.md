# book-zephyr：写作规划与验证资料

按 [D-011](../../DECISIONS.md)，本目录在 OPC 中维护图书设计、章节规划、蓝图和验证基线，每份成果保留一份当前正文。书稿正文、配图及官方参考资料位于独立 book-zephyr 仓库，统一登记见 [成果索引](../README.md)。

- [图书设计](design.md)：读者定位、内容边界、物料与写作约定。
- [全书目录与依赖](outline.md)：全书章节规划、显示章号、阅读顺序及前置关系；规划条目不代表正文已创建。
- [技术与实验环境基线](environment.md)：开发环境、复现条件与实验安排。
- [第 1 章蓝图](chapters/ch-env-setup/plan.md)：学习目标、内容结构与示例设计，稳定章节标识为 `ch-env-setup`。

当前书稿阅读入口为 [zephyr/index.md](https://github.com/yinzhongkai/book-zephyr/blob/08092e0a5af5d2895e44911b7282bb3b29d3b64e/zephyr/index.md)，该单条初始提交已同步远端，且已由 [WORKSPACE.yaml](../../WORKSPACE.yaml) 锁定为当前基线；[H-001](../../HANDOFFS.md) 的集成与关闭证据见 T-007，历史重建与恢复依据见 D-012。

本次迁回仅修正存放位置和必要引用，原成果 ID、负责人、版本、专业内容及批准依据保留。任务进展、评审、批准与实测结果以 OPC 原记录为准。
