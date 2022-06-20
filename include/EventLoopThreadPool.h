#pragma once

#include <memory>
#include <vector>

class EventLoop;
class EventLoopThread;
class EventLoopThreadPool {
public:
  EventLoopThreadPool(EventLoop *baseLoop, int threadNums);
  ~EventLoopThreadPool();
  void start();
  EventLoop *getNextLoop();

private:
  EventLoop *baseLoop_;
  bool started_;
  int threadNums_;
  int nextLoopIdx_;
  std::vector<std::shared_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};