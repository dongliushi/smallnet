#pragma once

#include <cassert>
#include <chrono>
#include <functional>

typedef std::chrono::time_point<std::chrono::steady_clock,
                                std::chrono::nanoseconds>
    TimeStamp;

inline TimeStamp now() noexcept { return std::chrono::steady_clock::now(); }

inline TimeStamp nowAfter(std::chrono::nanoseconds interval) {
  return now() + interval;
}

inline TimeStamp nowBefore(std::chrono::nanoseconds interval) {
  return now() - interval;
}

class Timer {
public:
  typedef std::function<void()> TimerCallback;
  typedef std::chrono::nanoseconds nanoseconds;
  typedef std::chrono::microseconds microseconds;
  typedef std::chrono::milliseconds milliseconds;
  typedef std::chrono::seconds seconds;
  typedef std::chrono::minutes minutes;
  typedef std::chrono::hours hours;
  Timer(TimeStamp when, nanoseconds interval, const TimerCallback &callback)
      : when_(when), interval_(interval), callback_(callback),
        repeat_(interval_ > nanoseconds::zero()), canceled_(false) {}

public:
  void run() {
    if (callback_)
      callback_();
  }

  bool repeat() const { return repeat_; }
  bool canceled() const { return canceled_; }
  bool expired(TimeStamp now) const { return now >= when_; }
  TimeStamp when() const { return when_; }

  void restart() {
    assert(repeat_);
    when_ += interval_;
  }
  void cancel() {
    assert(!canceled_);
    canceled_ = true;
  }

private:
  TimeStamp when_;
  const nanoseconds interval_;
  TimerCallback callback_;
  bool repeat_;
  bool canceled_;
};