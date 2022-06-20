#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

class Channel;
class Epoller;

class EventLoop {
  typedef std::function<void()> Task;

public:
  EventLoop();
  ~EventLoop();
  void loop();
  void quit();
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  void assertInLoopThread();
  bool isInLoopThread();
  void runInLoop(const Task &task);
  void runInLoop(Task &&task);
  void queueInLoop(const Task &task);
  void queueInLoop(Task &&task);
  void wakeup();
  void handleWakeUpRead();
  void doPendingTasks();

private:
  const pid_t threadId_;
  std::atomic<bool> quit_;
  std::unique_ptr<Epoller> epoller_;
  std::mutex mutex_;
  std::vector<Channel *> activeChannels_;
  bool doingPendingTasks_;
  std::vector<Task> pendingTasks_;
  const int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;
};
