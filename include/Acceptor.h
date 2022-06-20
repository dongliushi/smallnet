#pragma once

#include "Channel.h"
#include "Socket.h"
#include <functional>

class EventLoop;
class NetAddr;

class Acceptor {
  typedef std::function<void(int sockfd, const NetAddr &peerAddr)>
      NewConnectionCallback;

public:
  Acceptor(EventLoop *loop, const NetAddr &local);
  ~Acceptor();
  bool isListening() { return listening_; }
  void startListen();
  void setNewConnectionCallback(const NewConnectionCallback &cb);

private:
  void handleAccept();
  EventLoop *loop_;
  bool listening_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
};
