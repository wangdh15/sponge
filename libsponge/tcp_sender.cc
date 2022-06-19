#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

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
    while (!fin_send_ && _next_seqno < right_abs_seq_) {
        TCPSegment seg;
        auto &header = seg.header();
        auto &payload = seg.payload();
        header.seqno = wrap(_next_seqno, _isn);
        if (_next_seqno == 0) {
            header.syn = true;
        }
        uint64_t max_data_len = min(TCPConfig::MAX_PAYLOAD_SIZE, right_abs_seq_ - _next_seqno);
        // 如果上次收到的说window的大小为0的话，则至少要发送一个数据
        if (last_receive_window_size_ == 0) {
            max_data_len = max(max_data_len, static_cast<uint64_t>(1));
        }
        if (header.syn)
            max_data_len -= 1;
        std::string data = _stream.read(max_data_len);
        payload = Buffer(std::move(data));
        _next_seqno += seg.length_in_sequence_space();
        // check whether send FIN
        // 如果Payload装满了，但是没有达到window，那么还是可以发送FIN
        if (_stream.eof() && _next_seqno < right_abs_seq_) {
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
    if (abs_seq > _next_seqno) {
        return;
    }
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
        auto &t = segment_no_ack_.front().header().seqno;
        uint64_t abs_t_seqno = unwrap(t, _isn, checkpoint_);
        if (abs_t_seqno < abs_ack) {
            checkpoint_ = abs_t_seqno;
            bytes_in_flight_ -= segment_no_ack_.front().length_in_sequence_space();
            segment_no_ack_.pop();
        } else {
            break;
        }
    }
}

void TCPSender::update(uint64_t abs_ack, uint16_t window_size) {
    if (abs_ack > left_abs_seq_) {
        last_receive_window_size_ = window_size;
        left_abs_seq_ = abs_ack;
        // 当收到window_size 为零的时候，需要按照1进行处理
        right_abs_seq_ = abs_ack + window_size;
        // reset the timer
        resetTimer();
    } else if (abs_ack == left_abs_seq_) {
        last_receive_window_size_ = max(last_receive_window_size_, window_size);
        right_abs_seq_ = max(right_abs_seq_, abs_ack + last_receive_window_size_);
    }

    // 如果发出的所有的seg都被ack了，则需要停止定时器
    if (segment_no_ack_.empty()) {
        timer_.Close();
    }
}
