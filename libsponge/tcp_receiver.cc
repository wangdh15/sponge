#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!receive_syn_) {
        if (!seg.header().syn)
            return;
        else {
            receive_syn_ = true;
            syn_num_ = seg.header().seqno;
            checkpoints_ = 0;
        }
    }

    WrappingInt32 seq = seg.header().seqno;
    // if this is not a sync package, the absoluted seqno of data need minus 1.
    if (!seg.header().syn)
        seq = seq - 1;
    uint64_t abs_seqno = unwrap(seq, syn_num_, checkpoints_);
    _reassembler.push_substring(seg.payload().copy(), abs_seqno, seg.header().fin);
    checkpoints_ = abs_seqno;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!receive_syn_)
        return {};
    size_t abs_ackno = _reassembler.get_ackno();
    // the syn need a seq_num
    abs_ackno += 1;
    // because the fin need a seq_num;
    if (_reassembler.input_ended())
        abs_ackno += 1;
    return wrap(abs_ackno, syn_num_);
}

size_t TCPReceiver::window_size() const { return _reassembler.get_window_size(); }
