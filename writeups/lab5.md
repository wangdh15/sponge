Lab 5 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the NetworkInterface:

ARP协议是根据某个IP获取对应的MAC地址。
不知道某个IP对应的MAC地址的时候，通过发送广播包来请求MAC地址。
其他服务器都可以收到这个广播包，然后更新自己的缓存。但是只有被请求IP的
机器才会返回对应的响应（其他机器有对应的映射关不会响应）.

ARP请求再五秒钟内没有响应则不会重发。
每个缓存的过期时间是30秒。

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
