#pragma once

#include <memory>
#include <set>

#include "Channel.h"
#include "Timer.h"

class TimerQueue {
public:
  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue();

  Timer *addTimer(TimeStamp when, Timer::nanoseconds interval,
                  const Timer::TimerCallback &cb);
  void cancelTimer(Timer *timer);

private:
  typedef std::pair<TimeStamp, Timer *> Entry;
  typedef std::set<Entry> TimerList;

  void handleRead();
  std::vector<Entry> getExpired(TimeStamp now);

  EventLoop *loop_;
  const int timerFd_;
  Channel timerChannel_;
  TimerList timers_;
};