#pragma once

#include <sys/epoll.h>
#include <vector>

class Channel;
class EventLoop;

class Epoller {
public:
  Epoller(EventLoop *loop);
  ~Epoller();
  void poll(std::vector<Channel *> &activeChannels, int timeoutMs = -1);
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);

private:
  static const int EventsInitSize_ = 16;
  void update(int op, Channel *channel);
  int epollfd_;
  EventLoop *loop_;
  std::vector<epoll_event> events_;
};