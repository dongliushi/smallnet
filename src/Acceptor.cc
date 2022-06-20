#include "Acceptor.h"
#include "Channel.h"
#include "EventLoop.h"
#include <cassert>
#include <iostream>
#include <unistd.h>

int creatSocket() {
  int sockfd =
      socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  assert(sockfd != -1);
  return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const NetAddr &local)
    : acceptSocket_(creatSocket()), loop_(loop), listening_(false),
      acceptChannel_(loop, acceptSocket_.getFd()) {
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.bindAddr(local);
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleAccept, this));
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback &cb) {
  newConnectionCallback_ = cb;
}

void Acceptor::startListen() {
  loop_->assertInLoopThread();
  listening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableRead();
}

void Acceptor::handleAccept() {
  loop_->assertInLoopThread();
  NetAddr peerAddr;
  int connfd = acceptSocket_.acceptAddr(peerAddr);
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      close(connfd);
    }
  }
}