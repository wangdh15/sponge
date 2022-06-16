#include "stream_reassembler.hh"
#include <cassert>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :
    _output(capacity), _capacity(capacity), all_data_(), range_begin_(0),
    unassembled_bytes_(0), receive_eof_(false), eof_index_(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (!receive_eof_ && eof) {
        receive_eof_ = true;
        eof_index_ = index + data.size();
        if (range_begin_ == eof_index_) {
            _output.end_input();
        }
    }
    // firstly, delete the string outof range
    size_t len_byteStream = _output.buffer_size();
    size_t range_end = _capacity - len_byteStream + range_begin_;
    // without overlap
    if (index >= range_end || index + data.size() <= range_begin_) {
        return;
    }
    // get the data in the range
    std::string data_in_range = data;
    size_t l_in_range = index;
    if (index < range_begin_) {
        data_in_range = data_in_range.substr(range_begin_ - index, data_in_range.size());
        l_in_range = range_begin_;
    }
    if (l_in_range + data_in_range.size() > range_end) {
        data_in_range = data_in_range.substr(0, range_end - l_in_range);
    }
    if (data_in_range.size() == 0) {
        return;
    }
    size_t r_in_range = l_in_range + data_in_range.size() - 1;
    auto iter = all_data_.lower_bound({l_in_range, 0});
    // overlap with left range
    if (iter != all_data_.begin()) {
        --iter;
        if (iter->first.second + 1 >= l_in_range) {
            if (iter->first.second >= r_in_range) {
                data_in_range = iter->second;
            } else {
                data_in_range = iter->second + data_in_range.substr(iter->first.second + 1 - l_in_range, data_in_range.size());
            }
            l_in_range = iter->first.first;
            r_in_range = max(iter->first.second, r_in_range);
            unassembled_bytes_ -= iter->second.size();
            iter = all_data_.erase(iter);
        } else {
            ++iter;
        }
    }
    // check other range
    while (iter != all_data_.end() && r_in_range + 1 >= iter->first.first) {
        if (r_in_range < iter->first.second) {
            data_in_range += iter->second.substr(r_in_range + 1 - iter->first.first, iter->second.size());
            r_in_range = iter->first.second;
        }
        unassembled_bytes_ -= iter->second.size();
        iter = all_data_.erase(iter);
    }
    assert(data_in_range.size() == r_in_range - l_in_range + 1);
    all_data_[{l_in_range, r_in_range}] = data_in_range;
    unassembled_bytes_ += data_in_range.size();
    if (all_data_.begin()->first.first == range_begin_) {
        _output.write(all_data_.begin()->second);
        unassembled_bytes_ -= all_data_.begin()->second.size();
        range_begin_ = all_data_.begin()->first.second + 1;
        all_data_.erase(all_data_.begin());
    }
    if (receive_eof_ && range_begin_ == eof_index_) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return unassembled_bytes_;
 }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
