#include "Epoller.h"
#include "Channel.h"
#include "EventLoop.h"
#include <cassert>
#include <sys/epoll.h>
#include <unistd.h>

Epoller::Epoller(EventLoop *loop)
    : loop_(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(EventsInitSize_) {}

Epoller::~Epoller() { close(epollfd_); }

void Epoller::poll(std::vector<Channel *> &activeChannels, int timeoutMs) {
  int maxEvents = static_cast<int>(events_.size());
  int numEvents = epoll_wait(epollfd_, events_.data(), maxEvents, timeoutMs);
  if (numEvents == -1) {
    if (errno != EINTR) {
      // SYSERR("EPoller::epoll_wait()");
    }
  } else if (numEvents > 0) {
    for (int i = 0; i < numEvents; i++) {
      auto channel = static_cast<Channel *>(events_[i].data.ptr);
      channel->setRevents(events_[i].events);
      activeChannels.emplace_back(channel);
    }
    if (numEvents == maxEvents)
      events_.resize(2 * events_.size());
  }
}

void Epoller::updateChannel(Channel *channel) {
  loop_->assertInLoopThread();
  int op = 0;
  if (!channel->inEpoll()) {
    op = EPOLL_CTL_ADD;
  } else {
    op = EPOLL_CTL_MOD;
  }
  channel->setInEpoll(true);
  update(op, channel);
}

void Epoller::removeChannel(Channel *channel) {
  loop_->assertInLoopThread();
  int op = 0;
  op = EPOLL_CTL_DEL;
  if (channel->inEpoll()) {
    update(op, channel);
  }
  channel->setInEpoll(false);
}

void Epoller::update(int op, Channel *channel) {
  epoll_event event;
  event.events = channel->events();
  event.data.ptr = channel;
  int ret = epoll_ctl(epollfd_, op, channel->fd(), &event);
  assert(ret != -1);
}