#pragma once

#include "Buffer.h"
#include "Callbacks.h"
#include "NetAddr.h"
#include <functional>
#include <memory>

class EventLoop;
class Socket;
class Channel;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {

public:
  TcpConnection(EventLoop *loop, int sockfd, const NetAddr &localAddr,
                const NetAddr &peerAddr);
  ~TcpConnection();

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }
  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

  NetAddr &localAddr() { return localAddr_; }
  NetAddr &peerAddr() { return peerAddr_; }
  EventLoop *getLoop() const { return loop_; }

  void connectEstablished();
  void connectDestroyed();

  void send(const std::string& data);
private:
  enum class TcpConnectionState {
    Connecting,
    Connected,
    Disconnecting,
    Disconnected
  };

  void handleRead();
  void handleWrite();
  void handleClose();
  void handleError();
  void sendInLoop(const std::string& data);
  void setState(TcpConnectionState state) { state_ = state; }
  void shutdownInLoop();

  EventLoop *loop_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  TcpConnectionState state_;
  NetAddr localAddr_;
  NetAddr peerAddr_;
  Buffer inputBuffer_;
  Buffer outputBuffer_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  CloseCallback closeCallback_;
};