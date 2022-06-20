#pragma once
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

class NetAddr {
public:
  NetAddr(size_t port = 0, bool loopback = false);
  NetAddr(const std::string &ip, size_t port);
  void setAddress(const sockaddr_in &addr) { addr_ = addr; }
  const sockaddr *getSockAddr() const {
    return reinterpret_cast<const sockaddr *>(&addr_);
  }

private:
  sockaddr_in addr_;
};