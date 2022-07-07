#pragma once

#include "Callbacks.h"
#include "Channel.h"
#include "EventLoop.h"
#include "NetAddr.h"
#include "Timer.h"

class Connector : public std::enable_shared_from_this<Connector> {
public:
  typedef std::function<void(int sockfd)> NewConnectionCallback;
  enum class States { Disconnected, Connecting, Connected };
  Connector(EventLoop *loop, const NetAddr &serverAddr);
  ~Connector();
  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }
  void start();
  void restart();
  void stop();
  States state() const { return state_; }
  const NetAddr &serverAddress() const { return serverAddr_; }
  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

private:
  constexpr static const Timer::milliseconds MaxRetryDelayMs =
      Timer::milliseconds(30 * 1000);
  constexpr static const Timer::milliseconds InitRetryDelayMs =
      Timer::milliseconds(500);
  EventLoop *loop_;
  const NetAddr serverAddr_;
  States state_;
  bool connected_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  Timer::milliseconds retryDelayMs_;
};