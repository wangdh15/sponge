Lab 2 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:

从64位序列号处理为32位序列号的逻辑是，直接将其和初始系列号进行相加，溢出不需要
处理。

从32位序号处理为64位序号的逻辑是：
1. 首先利用该32位序号和初始序列号得到代表的最小的64位序号x。
2. 如果x比checkpoint大，则直接返回x。
3. 如果x比checkpoint小，则需要找到k使得
  $$x + k \times 2^{32} \leq checkpoint < x + (k + 1) \times 2^{32}$$
  $$ \frac{checkpoint - x}{2^{32}} - 1 < k \leq \frac{checkpoint - x}{2^{32}} $$
  $$ k = [\frac{checkpoint - x}{2^{32}}]$$
4. 然后看$x + k \times 2^{32} 和 x + (k +1) \times 2^{32}$那个和checkpoint相近，就用哪一个。

Receiver需要处理的序号有三类：
1. 32位SeqNo，在TCP的header中携带的，需要使用checkpoint和初始的32位的SeqNo处理成绝对的序列号
2. 64位绝对的序列号，以第一个SYN开始，以FIN结束，同时Sync和FIN也都占据一个位置。
3. 64位流序号。不包含和FIN。

在收到一个包的时候：
1. 如果SYN为true，则记录下初始化的SeqNo，将checkpoint置为0。这个时候数据段的绝对系列号为1，流序号为0.
2. 如果SYN为false，则负荷的流序号是对应包的绝对序列号-1.
3. 求ACK的时候，首先从streamAssembler中获取对应的流序号。
  - 首先将其加1，获得绝对序列号。
  - 然后判断是否数据全部接收完毕，如果是的话，则还需要将其加1，从而将FIN算上去。

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
