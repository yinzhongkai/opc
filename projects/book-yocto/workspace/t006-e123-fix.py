# -*- coding: utf-8 -*-
"""T-006 定稿前修订：E-2 show-layers 输出块按 T-008 实测原文替换（字节级取自核查记录），
并在输出块后补层集合名与目录名对应说明。带命中断言，未命中不写盘。"""
import re, sys

BOOK = r"C:\Users\YinZh\Desktop\book-yocto\projects\book-yocto\workspace\yocto\task02-1-把Yocto跑起来.md"
REC = r"C:\Users\YinZh\Desktop\book-yocto\projects\book-yocto\workspace\t008-chapter1-env-check.md"

rec = open(REC, encoding="utf-8").read()
# 提取 E-2 节中缩进的实测输出块（记录内为两空格缩进的 ```text 围栏）
m = re.search(r"实测原文：\n\n  ```text\n((?:  .*\n)+?)  ```", rec)
assert m, "未在核查记录中定位 E-2 实测原文块"
measured = "\n".join(line[2:] for line in m.group(1).split("\n") if line.startswith("  "))
assert measured.count("\n") == 4 and measured.split("\n")[1].count("=") == 104, "实测块形态异常"
print("实测块提取成功，分隔线", measured.split("\n")[1].count("="), "个 =")

text = open(BOOK, encoding="utf-8").read()
old = """```text
layer                 path                                     priority
==================================================================
meta                  /home/oops/workspace/poky/meta           5
meta-poky             /home/oops/workspace/poky/meta-poky      5
meta-yocto-bsp        /home/oops/workspace/poky/meta-yocto-bsp 5
```

> **💡 提示**：本书示例中的用户名和路径取自配套的固化构建环境"""
assert text.count(old) == 1, "旧 show-layers 块未唯一命中"

new = "```text\n" + measured + "\n```" + """

第一列是层的集合名（来自各层 `conf/layer.conf`），不是目录名——对应关系：`core` 即 `meta`，`yocto` 即 `meta-poky`，`yoctobsp` 即 `meta-yocto-bsp`。

> **💡 提示**：本书示例中的用户名和路径取自配套的固化构建环境"""
text = text.replace(old, new)
assert "meta-yocto-bsp        /home/oops" not in text
assert "yoctobsp" in text
open(BOOK, "w", encoding="utf-8", newline="\n").write(text)
print("E-2 已写盘：show-layers 输出块替换为实测原文，并补集合名对应说明")
