#include "Socket.h"
#include <cassert>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

Socket::~Socket() { close(sockfd_); }

void Socket::bindAddr(const NetAddr &addr) {
  int ret = bind(sockfd_, addr.getSockAddr(),
                 static_cast<socklen_t>(sizeof(sockaddr_in)));
  assert(ret != -1);
}

void Socket::setTcpNoDelay(bool on) {
  int opt_val = on ? 1 : 0;
  int ret = setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &opt_val,
                       static_cast<socklen_t>(sizeof opt_val));
  assert(ret != -1);
};

void Socket::setReuseAddr(bool on) {
  int opt_val = on ? 1 : 0;
  int ret = setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt_val,
                       static_cast<socklen_t>(sizeof opt_val));
  assert(ret != -1);
};

void Socket::setReusePort(bool on) {
  int opt_val = on ? 1 : 0;
  int ret = setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &opt_val,
                       static_cast<socklen_t>(sizeof opt_val));
  assert(ret != -1);
};

void Socket::setKeepAlive(bool on) {
  int opt_val = on ? 1 : 0;
  int ret = setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &opt_val,
                       static_cast<socklen_t>(sizeof opt_val));
  assert(ret != -1);
}

int Socket::acceptAddr(NetAddr &peerAddr) {
  sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
  int connfd = accept4(sockfd_, reinterpret_cast<sockaddr *>(&addr), &addrlen,
                       SOCK_NONBLOCK | SOCK_CLOEXEC);
  assert(connfd != -1);
  peerAddr.setAddress(addr);
  return connfd;
}

int Socket::getFd() { return sockfd_; }

void Socket::shutdownWrite() {
  int ret = shutdown(sockfd_, SHUT_WR);
  assert(ret != -1);
}

void Socket::listen() {
  int ret = ::listen(sockfd_, SOMAXCONN);
  assert(ret != -1);
}