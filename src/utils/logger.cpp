#include "logger.hpp"
#include <iostream>
#include <sstream>
#include "msgpack/object.hpp"

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

std::string ToString(const msgpack::object& obj) {
  return (std::ostringstream() << obj).str();
}
