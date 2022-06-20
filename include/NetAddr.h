#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

class NetAddr {
public:
  NetAddr(size_t port = 0, bool loopback = false);
  NetAddr(const std::string &ip, size_t port);
  void setAddress(const sockaddr_in &addr) { addr_ = addr; }
  std::string getIp() {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, ip, sizeof ip);
    return ip;
  }
  std::string getPort() {
    int port = ntohs(addr_.sin_port);
    return std::to_string(port);
  }
  const sockaddr *getSockAddr() const {
    return reinterpret_cast<const sockaddr *>(&addr_);
  }

private:
  sockaddr_in addr_;
};