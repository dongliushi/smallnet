#pragma once

#include "NetAddr.h"

class Socket {
public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  void bindAddr(const NetAddr &addr);
  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void setKeepAlive(bool on);
  void shutdownWrite();
  void listen();
  int acceptAddr(NetAddr &peerAddr);
  int getFd();

private:
  const int sockfd_;
};