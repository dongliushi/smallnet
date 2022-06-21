#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <cassert>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int threadNums)
    : baseLoop_(baseLoop), started_(false), threadNums_(threadNums),
      nextLoopIdx_(0) {
  assert(threadNums_ > 0);
}

EventLoopThreadPool::~EventLoopThreadPool() { assert(started_); }

void EventLoopThreadPool::start() {
  baseLoop_->assertInLoopThread();
  started_ = true;
  for (int i = 0; i < threadNums_; i++) {
    std::shared_ptr<EventLoopThread> t(new EventLoopThread());
    threads_.emplace_back(t);
    loops_.emplace_back(t->startLoop());
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop *loop = baseLoop_;
  if (!loops_.empty()) {
    loop = loops_[nextLoopIdx_];
    nextLoopIdx_ = (nextLoopIdx_ + 1) % threadNums_;
  }
  return loop;
}