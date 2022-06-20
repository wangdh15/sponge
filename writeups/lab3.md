Lab 3 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the TCPSender:

四个发送包的函数的逻辑：

### fill_window

1. SYN和FIN要占据绝对序号的空间，也会占据windows的空间。
2. MAX_PAYLOAD_SIZE指的是data的最大大小。SYN和ACK不算data的大小中。
3. 当windows的大小为0时，且当前发送队列没有包的时候，需要按照大小为1进行处理。这是因为虽然可能遭到拒绝，但是可以触发发送方再发送一次ACK。

### ack_received

接收到了发送方发来的ackno和window_size的包，更新自己的状态，并调用fill_window(如果window有新的空间了)
1. 将ackno前面的包从队列中删除。
2. 调整区间为[ackno, ackno + window_size]
3. 还需要对定时器进行重置。

### tick

上层通过调用这个接口来通知Sender距离上次该函数调用已经过去了的时间，需要维护一个状态，来判断是否
触发了超时。
1. 如果超时了，则需要对包进行重发，并将修改下次超时时间。
2. 如果没有超时，则不做什么处理。
3. 如果是window_size为0的情况下发送的一个包，那么如果发生了超时，则不用延长对应的过期时间，保持原样即可。

### send_empty_segment

发送一个空的包，不包含数据，包含序列号，但是不占用序列号。可被上层用于发送空的ACK包。


Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
