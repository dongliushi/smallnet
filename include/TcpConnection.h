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
  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                size_t highWaterMark) {
    highWaterMarkCallback_ = cb;
    // highWaterMark_ = highWaterMark;
  }
  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }
  EventLoop *getLoop() const { return loop_; }
  void connectEstablished();
  void connectDestroyed();

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
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
};