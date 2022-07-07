#pragma once

#include "Callbacks.h"
#include "EventLoop.h"
#include "NetAddr.h"
#include "TcpConnection.h"
#include <mutex>
#include <string>

class Connector;

class TcpClient {
public:
  typedef std::shared_ptr<Connector> ConnectorPtr;
  TcpClient(EventLoop *loop, const NetAddr &serverAddr);
  ~TcpClient();
  void connect();
  void disconnect();
  void stop();
  bool isConnected();
  bool isConnecting();
  bool isDisconnected();
  TcpConnectionPtr connection() const { return connection_; };
  EventLoop *getLoop() const { return loop_; }
  bool retry() const { return retry_; }
  void enableRetry() { retry_ = true; }
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

private:
  void newConnection(int sockfd);
  void removeConnection(const TcpConnectionPtr &conn);
  EventLoop *loop_;
  ConnectorPtr connector_;
  TcpConnectionPtr connection_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  bool retry_;
  bool connect_;
  std::mutex mutex_;
};