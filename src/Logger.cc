#include "Logger.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <syscall.h>
#include <unistd.h>

Logger::Logger(LogLevel logLevel, const char *fileName, int line)
    : logLevel_(logLevel), line_(line), fileName_(fileName) {
  *this << "[Date: " << __DATE__ << "]:[Time: " << __TIME__ << "] ";
}

Logger::Logger(LogLevel logLevel, const char *fileName, int line,
               const char *func)
    : logLevel_(logLevel), line_(line), fileName_(fileName) {
  *this << "[Func: " << func
        << "]:[Tid: " << static_cast<pid_t>(syscall(SYS_gettid)) << "] ";
}

Logger::~Logger() {
  *this << " —— " << fileName_ << ":" << std::to_string(line_);
  std::string str(buffer_.begin(), buffer_.end());
  std::cout << str << std::endl;
}

Logger::LogLevel g_logLevel = Logger::LogLevel::INFO;

Logger &Logger::operator<<(const std::string &log) {
  for (auto &ch : log) {
    buffer_.emplace_back(ch);
  }
  return *this;
}

Logger &Logger::operator<<(int log) {
  for (auto &ch : std::to_string(log)) {
    buffer_.emplace_back(ch);
  }
  return *this;
}