Lab 6 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the Router:

这个lab比较简单，就实现路由器的内部逻辑，而且不要求实现路由器的维护算法。

一个路由器的每个规则维护了四个内容：
1. IP 
2. 前缀长度
3. 转发的端口
4. 下一跳的IP(可选，如果没有的话，则表示目标IP就在这个端口的子网中，则将下一条的IP设置为目标IP即可。否则的话，这个IP就表示要到达的下一个路由器再这个端口所在子网的IP)

加入一个规则比较简单，直接放进去即可。

转发一个包的话，找到最长前缀的方法暴力是移位之后比较是否相等。
其实可以使用Trie来优化，在常数时间就可以找到最佳匹配。

同时还需要维护一下ttl。

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
