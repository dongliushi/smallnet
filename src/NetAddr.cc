#include "NetAddr.h"
#include <arpa/inet.h>
#include <cstring>

NetAddr::NetAddr(size_t port, bool loopback) {
  memset(&addr_, 0, sizeof addr_);
  addr_.sin_family = AF_INET;
  in_addr_t ip = loopback ? INADDR_LOOPBACK : INADDR_ANY;
  addr_.sin_addr.s_addr = htonl(ip);
  addr_.sin_port = htons(port);
}

NetAddr::NetAddr(const std::string &ip, size_t port) {
  memset(&addr_, 0, sizeof addr_);
  addr_.sin_family = AF_INET;
  inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
  addr_.sin_port = htons(port);
}