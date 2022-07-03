#include "TimerQueue.h"
#include "EventLoop.h"
#include <cstring>
#include <functional>
#include <sys/timerfd.h>
#include <unistd.h>

int timerFdCreate() {
  int timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  assert(timerFd != -1);
  return timerFd;
}

void readTimerFd(int fd) {
  uint64_t val;
  ssize_t n = read(fd, &val, sizeof(val));
}

struct timespec durationFromNow(TimeStamp when) {
  using namespace std::literals::chrono_literals;
  struct timespec ret;
  Timer::nanoseconds ns = when - ::now();
  if (ns < 1ms)
    ns = 1ms;

  ret.tv_sec = static_cast<time_t>(ns.count() / std::nano::den);
  ret.tv_nsec = ns.count() % std::nano::den;
  return ret;
}

void timerfdSet(int fd, TimeStamp when) {
  struct itimerspec oldtime, newtime;
  memset(&oldtime, 0, sizeof(itimerspec));
  memset(&newtime, 0, sizeof(itimerspec));
  newtime.it_value = durationFromNow(when);
  int ret = timerfd_settime(fd, 0, &newtime, &oldtime);
  assert(ret != -1);
}

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop), timerFd_(timerFdCreate()), timerChannel_(loop, timerFd_) {
  loop_->assertInLoopThread();
  timerChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
  timerChannel_.enableRead();
}

TimerQueue::~TimerQueue() {
  for (auto &p : timers_)
    delete p.second;
  ::close(timerFd_);
}

Timer *TimerQueue::addTimer(TimeStamp when, Timer::nanoseconds interval,
                            const Timer::TimerCallback &cb) {
  Timer *timer = new Timer(when, interval, cb);
  loop_->runInLoop([=]() {
    auto ret = timers_.insert({when, timer});
    assert(ret.second);
    if (timers_.begin() == ret.first)
      timerfdSet(timerFd_, when);
  });
  return timer;
}

void TimerQueue::cancelTimer(Timer *timer) {
  loop_->runInLoop([timer, this]() {
    timer->cancel();
    timers_.erase({timer->when(), timer});
    delete timer;
  });
}

void TimerQueue::handleRead() {
  loop_->assertInLoopThread();
  TimeStamp now(::now());
  readTimerFd(timerFd_);
  std::vector<Entry> expired = getExpired(now);
  for (auto &e : expired) {
    Timer *timer = e.second;
    assert(timer->expired(now));
    if (!timer->canceled())
      timer->run();
    if (!timer->canceled() && timer->repeat()) {
      timer->restart();
      e.first = timer->when();
      timers_.insert(e);
    } else
      delete timer;
  }
  if (!timers_.empty())
    timerfdSet(timerFd_, timers_.begin()->first);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(TimeStamp now) {
  Entry en(now, nullptr);
  std::vector<Entry> entries;
  TimerList::iterator end = timers_.lower_bound(en);
  entries.assign(timers_.begin(), end);
  timers_.erase(timers_.begin(), end);
  return entries;
}
