#include "./logger.hpp"
#include <iostream>

void Logger::RedirToPath(const fs::path& path) {
  logFile.open(path, std::ios::out | std::ios::app);
  if (logFile.is_open()) {
    std::cout.rdbuf(logFile.rdbuf()); 
    std::cerr.rdbuf(logFile.rdbuf());
  } else {
    LOG_ERR("Failed to open log file: {}", path.string());
  }
}

void Logger::Log(const std::string& message) {
  std::unique_lock lock(mutex);
  if (!enabled) return;
  std::cout << message << '\n';
}

void Logger::LogInfo(const std::string& message) {
  std::unique_lock lock(mutex);
  std::cout << "INFO: " << message << '\n';
}

void Logger::LogWarn(const std::string& message) {
  std::unique_lock lock(mutex);
  std::cout << "WARNING: " << message << '\n';
}

void Logger::LogErr(const std::string& message) {
  std::unique_lock lock(mutex);
  std::cerr << "ERROR: " << message << '\n';
}

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

void Logger::LogTrace() {
  std::unique_lock lock(mutex);
  std::cout << "TRACE: " << boost::stacktrace::stacktrace() << '\n';
}
