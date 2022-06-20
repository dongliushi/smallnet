#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;

class Channel {
  typedef std::function<void()> ReadCallback;
  typedef std::function<void()> WriteCallback;
  typedef std::function<void()> CloseCallback;
  typedef std::function<void()> ErrorCallback;

public:
  Channel(EventLoop *loop, int fd);
  ~Channel();

  void setReadCallback(const ReadCallback &cb) { readCallback_ = cb; }
  void setWriteCallback(const WriteCallback &cb) { writeCallback_ = cb; }
  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }
  void setErrorCallback(const ErrorCallback &cb) { errorCallback_ = cb; }

  void handleEvents();
  int fd() const { return fd_; }
  int events() const { return events_; }
  void setRevents(int revents) { revents_ = revents; }
  int inEpoll() { return inEpoll_; }
  void setInEpoll(bool on) { inEpoll_ = on; }

  void tie(const std::shared_ptr<void> &obj);

  void enableRead() {
    events_ |= (EPOLLIN | EPOLLPRI);
    update();
  }
  void enableWrite() {
    events_ |= EPOLLOUT;
    update();
  }
  void disableRead() {
    events_ &= ~(EPOLLIN | EPOLLPRI);
    update();
  }
  void disableWrite() {
    events_ &= ~EPOLLOUT;
    update();
  }
  void disableAll() {
    events_ = 0;
    update();
  }

  bool isReading() const { return events_ & EPOLLIN; }
  bool isWriting() const { return events_ & EPOLLOUT; }
  void remove();

private:
  void update();
  void handleEventsWithGuard();

  EventLoop *loop_;
  bool tied_;
  std::weak_ptr<void> tie_;
  const int fd_;
  bool handlingEvents_;
  bool inEpoll_;
  int events_;
  int revents_;
  ReadCallback readCallback_;
  WriteCallback writeCallback_;
  CloseCallback closeCallback_;
  ErrorCallback errorCallback_;
};