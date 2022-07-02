#include "Buffer.h"

const char Buffer::CRLF[] = "\r\n";

ssize_t Buffer::readFd(int fd, int *savedErrno) {
  char extrabuf[65536];
  struct iovec vec[2];
  const std::size_t writeable = writeableBytes();
  vec[0].iov_base = begin() + write_index_;
  vec[0].iov_len = writeable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = readv(fd, vec, iovcnt);
  if (n < 0)
    *savedErrno = errno;
  else if (static_cast<size_t>(n) <= writeable)
    write_index_ += n;
  else {
    write_index_ = buffer_.size();
    append(extrabuf, n - writeable);
  }
  return n;
}