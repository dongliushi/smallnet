#pragma once

#include "Callbacks.h"
#include "NetAddr.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_set>

class Acceptor;
class EventLoopThreadPool;
class EventLoop;
class TcpConnection;
class Buffer;

class TcpServer {

public:
  TcpServer(EventLoop *loop, const NetAddr &local, int threadNums);
  ~TcpServer();

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }
  void start();

private:
  void newConnection(int sockfd, const NetAddr &peerAddr);
  void closeConnection(const TcpConnectionPtr &conn);
  void closeConnectionInLoop(const TcpConnectionPtr &conn);
  EventLoop *loop_;
  std::atomic<bool> started_;
  NetAddr localAddr_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> threadPool_;
  std::unordered_set<TcpConnectionPtr> connectionSet_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
};