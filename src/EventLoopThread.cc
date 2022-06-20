#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread() : started_(false), loop_(nullptr) {}

EventLoopThread::~EventLoopThread() {
  if (started_) {
    if (loop_ != nullptr) {
      loop_->quit();
      thread_.join();
    }
  }
}

EventLoop *EventLoopThread::startLoop() {
  started_ = true;
  thread_ = std::thread([this] { threadFunc(); });
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return loop_ != nullptr; });
  }
  return loop_;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }
  loop.loop();
  loop_ = nullptr;
}