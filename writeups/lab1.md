Lab 1 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the StreamReassembler:


1. 可以将问题抽象为实现一个维护区间的数据结构，支持如下的操作：
    - 插入一个区间。（对重复的区间进行去重）
    - 将区间的一开始连续的部分传递给另一个对象
    - 可维护的整个区间的长度是一个有限的值。

那么可以使用经典的树形结构来进行维护。树的每个节点存储三个内容：
    - 区间左端点
    - 区间右端点
    - 字符串

当一个新的区间加入的时候：
1. 首先根据capacity判断可以容纳的字符串的最大范围，然后将超过这个范围的字符串去除。
2. 利用加入的区间的左端点在树中进行二分查找，找到第一个左端点不小于该区间左端点的节点。
    - 首先判断该区间和左边的区是否有交集。
    - 然后判断和右边的若干个区间是否有交集。

在扫描区间的时候，遇到有交集的区间，就需要将该区间从树上删除，并维护好对应的值。

判断何时想output_发送eof，我是将第一次传入eof标志的区间算出去右端点然后存下来，之后每次处理完后就检查
还没有发送的第一个index和eof的最后一个index是否相同，如果相同的话就说明所有数据都接收完毕了，就向output_发送eof。

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
