#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : capacity_(capacity), data_(),
    bytes_written_(0), bytes_read_(0), end_(false),
    _error(false){ }


size_t ByteStream::write(const string &data) {
    if (end_ || _error) {
        return 0;
    }
    size_t remain_ = remaining_capacity();
    data_ += data.substr(0, remain_);
    bytes_written_ += min(remain_, data.size());
    return min(remain_, data.size());
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return data_.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    bytes_read_ += min(len, data_.size());
    if (len <= data_.size()) {
        data_ = data_.substr(len, data_.size());
    } else {
        data_ = "";
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() {
    end_ = true;
}

bool ByteStream::input_ended() const { return end_; }

size_t ByteStream::buffer_size() const { return data_.size(); }

bool ByteStream::buffer_empty() const { return data_.size() == 0; }

bool ByteStream::eof() const { return (end_ || _error) && data_.size() == 0; }

size_t ByteStream::bytes_written() const { return bytes_written_; }

size_t ByteStream::bytes_read() const { return bytes_read_; }

size_t ByteStream::remaining_capacity() const { return capacity_ - data_.size(); }

bool ByteStream::canWrite() const { return !(end_ || _error); }
