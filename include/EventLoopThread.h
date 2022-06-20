#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

class EventLoop;

class EventLoopThread {
public:
  EventLoopThread();
  ~EventLoopThread();
  EventLoop *startLoop();

private:
  void threadFunc();
  bool started_;
  std::thread thread_;
  EventLoop *loop_;
  std::mutex mutex_;
  std::condition_variable cond_;
};