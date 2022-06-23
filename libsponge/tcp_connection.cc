#include "tcp_connection.hh"

#include "parser.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return time_since_last_segment_received_; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!active())
        return;

    time_since_last_segment_received_ = 0;

    // 如果收到了RST
    if (seg.header().rst) {
        receive_rst_ = true;
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        return;
    }

    _receiver.segment_received(seg);
    // 当发现将对面的FIN接收了，且还没有发送FIN，则表示自己是passive_close
    // 如果接收了FIN之后接收到了重复的包，则这个地方不会有问题。
    if (_receiver.stream_out().input_ended() && !send_fin_) {
        _linger_after_streams_finish = false;
    }
    // 一定要发送包的两类情况
    bool must_send_package =
        seg.length_in_sequence_space() != 0 || (seg.length_in_sequence_space() == 0 && _receiver.ackno().has_value() &&
                                                !_receiver.legal_seqno(seg.header().seqno));
    // 只有在有ACK的情况下才需要发送给_sender.
    // 第一个SYN包没有ACK，但是通过调用_receiver,就可以触发
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    } else {
        // 否则的话，表示收到了第一个建立连接的SYN包
        // 需要手动触发发送一个SYN+ACK的包
        // 其中的ACK和对应的window_size由sendAllData填充
        // 所以需要先将这个包传递给_receiver，然后获取对应的ack
        _sender.fill_window();  // 发送一个SYN的包，ACK由sendAllData填充
    }
    sendAllData(false, must_send_package);
}

bool TCPConnection::active() const {
    if (receive_rst_)
        return false;
    if (_sender.stream_in().canWrite())
        return true;
    if (!_sender.stream_in().input_ended() || _sender.bytes_in_flight() != 0)
        return true;
    if (!_receiver.stream_out().eof())
        return true;
    if (_linger_after_streams_finish && time_since_last_segment_received_ < 10 * _cfg.rt_timeout)
        return true;
    return false;
}

size_t TCPConnection::write(const string &data) {
    if (!active())
        return 0;

    // 如果sender已经不能发送数据了，则直接返回
    if (!_sender.stream_in().canWrite()) {
        return 0;
    }

    // 将数据写到sender的Stream中
    size_t res = _sender.stream_in().write(data);
#ifdef DEBUG
    std::cout << "Client Write Data: " << data.substr(0, res);
#endif  //
    // 调用fill_window从stream中取出数据
    _sender.fill_window();
    // 将包填充ack和seq，并发送出去
    sendAllData();
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!active())
        return;
    time_since_last_segment_received_ += ms_since_last_tick;
    if (!active())
      return;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        _sender.stream_in().end_input();
        _receiver.stream_out().end_input();
        receive_rst_ = true;
        sendAllData(true, true);
    } else {
        sendAllData();
    }
}

void TCPConnection::end_input_stream() {
    if (!active())
        return;
    if (!_sender.stream_in().canWrite())
        return;
    _sender.stream_in().end_input();
    _sender.fill_window();
    sendAllData(false, true);
}

void TCPConnection::connect() {
    if (!active())
        return;

    // 确保sender当前还可以发送数据
    if (!_sender.stream_in().canWrite()) {
        std::cerr << "Try connect when sender cannot be write" << std::endl;
        return;
    }
    // 确保sender没有发送过SYN包
    if (_sender.next_seqno_absolute() != 0) {
        // 当seqno_abs不是0，表示之前发送过SYN了，不能在发起连接
        std::cerr << "Try connect when sender is not closed" << std::endl;
        return;
    }
    // 到这里可以确定第一个绝对序列号为0，且sender可以写东西。
    // 调用fill_window从而触发发送出SYN包
    _sender.fill_window();
    sendAllData();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            sendAllData(true, true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
