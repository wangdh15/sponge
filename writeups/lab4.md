Lab 4 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

## TCP如何决定何时关闭

TCP一共有两个方向，一个是发送，一个是接收。一方可以安全地关闭TCP连接需要有如下的条件：
1. 自己完全接收完了对方发送过来的数据。
2. 自己发送的数据全部被对方完全接收了。

对于第一个条件是比较容易满足的。因为如果自己收到了对方发送过来的带有FIN的包，就知道自己完整接收了对方发送过来的数据。
但是第二个条件就不是那么容易就搞清楚了。

1. 自己先收到对面发送过来的FIN，这个时候自己后面发的包肯定会携带上对对方的FIN的ACK，所以如果自己后面收到了对方给自己的FIN发来的ACK，则可以确定的是对方也一定收到了自己给对方的FIN发送的ACK。所以自己就可以确定自己收到了对方发送来的全部数据，且对方也收到了自己给对方发送的全部数据，所以就可以安心关闭自己了。(自己不会给对方的ACK发送ACK)
2. 如果自己在收到对方发送过来的FIN之前，自己率先发送了FIN，这个时候对方还会给自己发送数据。
  - 对面先给自己的FIN发了ACK，那么自己就可以确定自己的数据被对面全部接收了。然后对面发动了他的FIN，自己给这个FIN发送了一个ACK。但是这个ACK我是不知道对面是否已经收到了。这是因为按照第一条，对面如果这个时候收到了我发送的ACK，他就可以确定他的两个方向都ok，他就直接关闭了，而且他不会对我的ACK再发送一个ACK，所以我是不能够知道他有咩有收到我给他发的ACK的。这个时候我需要等待一个超时时间，然后把连接关闭（认为对面已经关闭了）。

如果自己收到了对面发来的


Program Structure and Design of the TCPConnection:

每个TCPConnection的公共接口有如下:
## 修改状态的

1. connect
  - 检查当前是否是CLOSE状态。
  - 直接调用sender的fill_window方法，发送一个SYN包。
  - 然后将sender的发送队列的包拷贝到connection的发送队列。（这个时候receiver应该还没有ack和window_size。）
  
2. write 

  - 检查当前状态是否能够发送数据。（是否已经完成了三次握手的过程）
  - 将数据写入sender的Stream中。
  - 调用sender的fill_window函数，发送数据。
  - 将sender的发送队列拷贝到connection的发送队列，并填充好对应的ack和windows_size


3. end_input_stream
  - 调用sender的Stream的end_input方法。
  - 调用sender的fill_windw的方法，触发发送FIN的包。

4. segment_received

  - 检查当前状态是否能够接收包
  - 如果收到了以RST的包，则
    + 将sender和receiver的流关闭
    + 确保active返回false
  - 将包发送给receiver，供更新ack和window_size
  - 将包发送给sender
    + 如果收到的包是对面发来的第一个SYN的包，则不包含ACK的信息，这个时候需要发送给sender，只需要调用sender的fill_window，触发sender发送一个SYN的包，然后利用receiver的ack和window_size填充，就相当于实现了第二次握手。
    + 如果收到的包包含了ACK的信息，则调用sender的ack_received函数即可。如果可以继续发数据的话，则其内部会触发调用fill_window函数。
  - 将sender的发送队列去除，填充上ack和window_size，发送给对方

5. tick

  - 状态检查？
  - 调用sender的tick函数。
  - 查看sender的连续重传次数，超过阈值的话就全部关闭。
  - 维护距离上次接收到包的时间，超过阈值的话就设置linger_after_streams_finish的状态。
  - 将sender的发送队列清空，并填充好对应的ack和win。


## 只读状态的

1. remaining_outbound_capacity

2. inbound_stream

3. bytes_in_flight

4. unassembled_bytes

5. time_since_last_segment_received

6. active 

7. segments_out


Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
