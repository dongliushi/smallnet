#include "TcpServer.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>

void defaultConnectionCallback(const TcpConnectionPtr &conn) {}

void defaultMessageCallback(const TcpConnectionPtr &, Buffer &buf) {
  // buf.retrieveAll();
}

TcpServer::TcpServer(EventLoop *loop, const NetAddr &localAddr, int threadNums)
    : loop_(loop), localAddr_(localAddr), started_(false),
      acceptor_(new Acceptor(loop, localAddr)),
      threadPool_(new EventLoopThreadPool(loop, threadNums)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback) {
  using namespace std::placeholders;
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {
  loop_->assertInLoopThread();
  for (auto tcp_connection_ptr : connectionSet_) {
    tcp_connection_ptr.reset();
    tcp_connection_ptr->getLoop()->runInLoop(
        std::bind(&TcpConnection::connectDestroyed, tcp_connection_ptr));
  }
}

void TcpServer::start() {
  if (!started_) {
    started_ = true;
    threadPool_->start();
    assert(!acceptor_->isListening());
    loop_->runInLoop(std::bind(&Acceptor::startListen, acceptor_.get()));
  }
}

void TcpServer::newConnection(int connfd, const NetAddr &peerAddr) {
  using namespace std::placeholders;
  loop_->assertInLoopThread();
  EventLoop *ioLoop = threadPool_->getNextLoop();
  TcpConnectionPtr conn(
      new TcpConnection(ioLoop, connfd, localAddr_, peerAddr));
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(std::bind(&TcpServer::closeConnection, this, _1));
  connectionSet_.emplace(conn);
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn.get()));
}

void TcpServer::closeConnection(const TcpConnectionPtr &conn) {
  loop_->runInLoop(std::bind(&TcpServer::closeConnectionInLoop, this, conn));
}

void TcpServer::closeConnectionInLoop(const TcpConnectionPtr &conn) {
  loop_->assertInLoopThread();
  size_t n = connectionSet_.erase(conn);
  assert(n == 1);
  EventLoop *ioLoop = conn->getLoop();
  ioLoop->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}