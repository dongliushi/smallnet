#pragma once

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <string>
#include <sys/uio.h>
#include <vector>

/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

class Buffer {
  static const size_t Prepend_Initsize = 8;
  static const size_t Buffer_Initsize = 1024;

public:
  explicit Buffer(size_t buffer_initsize = Buffer_Initsize)
      : buffer_(Prepend_Initsize + buffer_initsize),
        read_index_(Prepend_Initsize), write_index_(Prepend_Initsize) {
    assert(readableBytes() == 0);
    assert(writeableBytes() == buffer_initsize);
    assert(prependableBytes() == Prepend_Initsize);
  }

  size_t readableBytes() { return write_index_ - read_index_; }

  size_t writeableBytes() { return buffer_.size() - write_index_; }

  size_t prependableBytes() { return read_index_; }

  const char *peek() const { return begin() + read_index_; }

  char *beginWrite() { return begin() + write_index_; }

  const char *beginWrite() const { return begin() + write_index_; }

  const char *findCRLF() const {
    const char *crlf = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  const char *findCRLF(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char *crlf = std::search(start, beginWrite(), CRLF, CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  void ensureWriteableBytes(size_t len) {
    if (writeableBytes() < len)
      makeSpace(len);
    assert(writeableBytes() >= len);
  }

  void append(const char *data, size_t len) {
    ensureWriteableBytes(len);
    std::copy(data, data + len, begin() + write_index_);
  }

  void retrieveAll() {
    read_index_ = Prepend_Initsize;
    write_index_ = Prepend_Initsize;
  }

  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      read_index_ += len;
    } else {
      retrieveAll();
    }
  }

  std::string retrieveString(size_t len) {
    assert(len <= readableBytes());
    std::string result(begin() + read_index_, len);
    retrieve(len);
    return result;
  }

  std::string retrieveAllString() { return retrieveString(readableBytes()); }

  void prepend(const void *data, size_t len) {
    assert(len <= prependableBytes());
    read_index_ -= len;
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, begin() + read_index_);
  }

  ssize_t readFd(int fd, int *savedErrno);

private:
  char *begin() { return &*buffer_.begin(); }
  const char *begin() const { return &*buffer_.begin(); }

  void makeSpace(size_t len) {
    if (writeableBytes() + prependableBytes() < len + Prepend_Initsize) {
      buffer_.resize(write_index_ + len);
    } else {
      assert(Prepend_Initsize < read_index_);
      size_t readable = readableBytes();
      std::copy(begin() + read_index_, begin() + write_index_,
                begin() + Prepend_Initsize);
      read_index_ = Prepend_Initsize;
      write_index_ = read_index_ + readable;
      assert(readable == readableBytes());
    }
  }

private:
  std::vector<char> buffer_;
  size_t read_index_;
  size_t write_index_;
  static const char CRLF[];
};