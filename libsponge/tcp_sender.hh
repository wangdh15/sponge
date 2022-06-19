#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

class Timer {
  private:
    unsigned int init_rtx_timeout_;
    unsigned int cur_rtx_timeout_;
    unsigned int time_expired_;
    bool stop_;

  public:
    Timer(unsigned int init_rtx_timeout)
        : init_rtx_timeout_(init_rtx_timeout), cur_rtx_timeout_(init_rtx_timeout), time_expired_(0), stop_(true) {}

    void Start() {
        stop_ = false;
    }

    void Reset() { time_expired_ = 0; }

    void ResetTimeout() { cur_rtx_timeout_ = init_rtx_timeout_; }

    void Close() { stop_ = true; }

    bool Closed() { return stop_; }

    void DoubleRtxTimeout() {
        cur_rtx_timeout_ <<= 1;
    }

    bool Expired() { return time_expired_ >= cur_rtx_timeout_; }

    void ExpireTime(unsigned int t) { time_expired_ += t; }
};

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    // 控制超时重传的时钟
    Timer timer_;

    // 还未被ack的下一个绝对序列号
    uint64_t left_abs_seq_{0};

    // 可以发送的最大绝对系列号的后一个，和left_abs_seq_组成左闭右开的区间
    // 初始化为1保证第一次发送一个SYN
    uint64_t right_abs_seq_{1};

    // 用于32位序列号和64位绝对序列号之间的转换
    uint64_t checkpoint_{0};

    // 是否已经发送过FIN，这是需要记录是因为根据其他的状态无法推断出这个内容
    bool fin_send_{false};

    // 连续重传次数
    unsigned int num_consecutive_retx_{0};
    
    // 上次收到的window size
    uint16_t last_receive_window_size_{1};

    // 还没有被ACK的segment
    std::queue<TCPSegment> segment_no_ack_{};
    
    // 还没有被ack的byte数
    uint64_t bytes_in_flight_{0};

  private:
    void removeQueue(uint64_t abs_ack);

    void update(uint64_t abs_ack, uint16_t window_size);
  
    void sendSegment(const TCPSegment& seg, bool reset_timer);
    
    void resetTimer();

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
