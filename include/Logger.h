#pragma once

#include <string>
#include <thread>
#include <vector>

class Logger {
public:
  enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  Logger(LogLevel logLevel, const char *fileName, int line);
  Logger(LogLevel logLevel, const char *fileName, int line, const char *func);
  ~Logger();
  Logger &stream() { return *this; }
  static LogLevel logLevel();
  static void setLogLevel(LogLevel logLevel);
  Logger &operator<<(const std::string &log);

private:
  LogLevel logLevel_;
  std::string fileName_;
  int line_;
  std::vector<char> buffer_;
};

extern Logger::LogLevel globalLogLevel;

inline Logger::LogLevel Logger::logLevel() { return globalLogLevel; }

inline void Logger::setLogLevel(Logger::LogLevel level) {
  globalLogLevel = level;
}

#define LOG_INFO Logger(Logger::LogLevel::INFO, __FILE__, __LINE__).stream()

#define LOG_DEBUG                                                              \
  Logger(Logger::LogLevel::INFO, __FILE__, __LINE__, __func__).stream()

class AsyncLogger {};
