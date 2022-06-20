#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <assert.h>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , timer_(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return bytes_in_flight_; }

void TCPSender::fill_window() {
    uint64_t right_abs_seq = last_ack_ + last_receive_window_size_;
    if (segment_no_ack_.empty() && last_receive_window_size_ == 0) {
        // 如果所有的包都被ack了，且对面说window_size为零，则这个时候需要将
        // window_size调整为1，这是因为需要保证自己这边发包来触发对面更新自己的window_size
        // 不然的话，整个协议就hang住了。
        // 也就是说要保证要不现在还有没有ack的包，要不要保证现在还有空间让我发包。
        // 如果所有的包都被ack了，且没有空间让我发包了，那么sender就没法想receiver发包，
        // 那么他也就没法收到对面的ack，没法更新window_size，之后就再也没法发包了。
        // 这里需要判断segment_no_ack_为空是为了判断是否之前已经发了一个大小为1的包，
        // 如果已经发过了的话，就不用再发了
        assert(right_abs_seq == _next_seqno);
        right_abs_seq += 1;
    }
    while (!fin_send_ && _next_seqno < right_abs_seq) {
        TCPSegment seg;
        auto &header = seg.header();
        auto &payload = seg.payload();
        header.seqno = wrap(_next_seqno, _isn);
        if (_next_seqno == 0) {
            header.syn = true;
        }
        uint64_t max_data_len = min(TCPConfig::MAX_PAYLOAD_SIZE, right_abs_seq - _next_seqno - (header.syn ? 1 : 0));
        std::string data = _stream.read(max_data_len);
        payload = Buffer(std::move(data));
        _next_seqno += seg.length_in_sequence_space();
        // check whether send FIN
        // 如果Payload装满了，但是没有达到window，那么还是可以发送FIN
        if (_stream.eof() && _next_seqno < right_abs_seq) {
            header.fin = true;
            fin_send_ = true;
            _next_seqno += 1;
        }
        if (seg.length_in_sequence_space() == 0) {
            // no data and SYN FIN, break
            return;
        }
        segment_no_ack_.push(seg);
        bytes_in_flight_ += seg.length_in_sequence_space();
        // if the tiemr is closed, need to start and reset.
        sendSegment(seg, timer_.Closed());
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 先根据ackno清空发送队列
    uint64_t abs_seq = unwrap(ackno, _isn, checkpoint_);
    // 确认序列号查过了下一个要发送的，非法
    if (abs_seq > _next_seqno) {
        return;
    }
    // 根据ack将发送队列中已经被确认的包去除
    removeQueue(abs_seq);
    update(abs_seq, window_size);
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (timer_.Closed())
        return;
    timer_.ExpireTime(ms_since_last_tick);
    if (timer_.Expired()) {
        sendSegment(segment_no_ack_.front(), false);
        if (last_receive_window_size_ != 0) {
            timer_.DoubleRtxTimeout();
            timer_.Reset();
            ++num_consecutive_retx_;
        }
        timer_.Reset();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return num_consecutive_retx_; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
    sendSegment(seg, true);
}

void TCPSender::resetTimer() {
    timer_.ResetTimeout();
    timer_.Reset();
    timer_.Start();
    num_consecutive_retx_ = 0;
}

void TCPSender::sendSegment(const TCPSegment &seg, bool reset_timer) {
    _segments_out.push(seg);
    if (reset_timer) {
        resetTimer();
    }
}

void TCPSender::removeQueue(uint64_t abs_ack) {
    while (!segment_no_ack_.empty()) {
        auto &t = segment_no_ack_.front();
        uint64_t abs_t_seqno = unwrap(t.header().seqno, _isn, checkpoint_) + t.length_in_sequence_space();
        if (abs_t_seqno <= abs_ack) {
            checkpoint_ = abs_t_seqno;
            bytes_in_flight_ -= segment_no_ack_.front().length_in_sequence_space();
            segment_no_ack_.pop();
            // 如果有新的包被ack了，则需要重置定时器
            resetTimer();
        } else {
            break;
        }
    }
    // 如果所有的包都被ACK了，则需要关闭定时器
    if (segment_no_ack_.empty()) {
        timer_.Close();
    }
}

void TCPSender::update(uint64_t abs_ack, uint16_t window_size) {
    if (abs_ack > last_ack_) {
        last_receive_window_size_ = window_size;
        last_ack_ = abs_ack;
    } else if (abs_ack == last_ack_) {
        last_receive_window_size_ = max(last_receive_window_size_, window_size);
    }
}
