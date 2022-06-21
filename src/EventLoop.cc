#include "EventLoop.h"
#include "Channel.h"
#include "Epoller.h"
#include "Logger.h"
#include <cassert>
#include <sys/eventfd.h>
#include <syscall.h>
#include <unistd.h>

pid_t gettid() { return static_cast<pid_t>(syscall(SYS_gettid)); }

EventLoop::EventLoop()
    : threadId_(gettid()), epoller_(new Epoller(this)), quit_(false),
      doingPendingTasks_(false),
      wakeupFd_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
  assert(wakeupFd_ != -1);
  wakeupChannel_->setReadCallback(
      std::bind(&EventLoop::handleWakeUpRead, this));
  wakeupChannel_->enableRead();
}

EventLoop::~EventLoop() {
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  close(wakeupFd_);
}

void EventLoop::loop() {
  while (!quit_) {
    activeChannels_.clear();
    epoller_->poll(activeChannels_);
    for (auto channel : activeChannels_)
      channel->handleEvents();
    doPendingTasks();
  }
}

void EventLoop::quit() {
  assert(!quit_);
  quit_ = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Task &&task) {
  if (isInLoopThread())
    task();
  else
    queueInLoop(std::move(task));
}

void EventLoop::queueInLoop(Task &&task) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingTasks_.emplace_back(task);
  }
  if (!isInLoopThread() || doingPendingTasks_)
    wakeup();
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_DEBUG << "EventLoop::wakeup() should ::write() " << sizeof(one)
              << "bytes";
  }
}

void EventLoop::assertInLoopThread() { assert(isInLoopThread()); }

bool EventLoop::isInLoopThread() { return threadId_ == gettid(); }

void EventLoop::updateChannel(Channel *channel) {
  assertInLoopThread();
  epoller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  assertInLoopThread();
  epoller_->removeChannel(channel);
}

void EventLoop::handleWakeUpRead() {
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
}

void EventLoop::doPendingTasks() {
  assertInLoopThread();
  std::vector<Task> tasks;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    tasks.swap(pendingTasks_);
  }
  doingPendingTasks_ = true;
  for (Task &task : tasks)
    task();
  doingPendingTasks_ = false;
}