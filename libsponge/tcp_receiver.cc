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
        }
    }

    WrappingInt32 seq = seg.header().seqno;
    // if this is not a sync package, the absoluted seqno of data need minus 1.
    if (!seg.header().syn)
        seq = seq - 1;
    uint64_t abs_seqno = unwrap(seq, syn_num_, _reassembler.get_ackno());
    _reassembler.push_substring(seg.payload().copy(), abs_seqno, seg.header().fin);
}

std::optional<size_t> TCPReceiver::get_abs_ackno() const {
  if (!receive_syn_) return {};
  size_t abs_ackno = _reassembler.get_ackno();
  // the SYN 
  abs_ackno += 1;
  if (_reassembler.input_ended()) 
    abs_ackno += 1;
  return abs_ackno;
}


optional<WrappingInt32> TCPReceiver::ackno() const {
    auto abs_ackno = get_abs_ackno();
    if (!abs_ackno.has_value()) return {};
    return wrap(abs_ackno.value(), syn_num_);
}


size_t TCPReceiver::window_size() const { return _reassembler.get_window_size(); }
