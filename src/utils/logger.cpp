#include "logger.hpp"
#include <iostream>
#include <sstream>
#include "msgpack/object.hpp"

void Logger::Log(const std::string& message) {
  if (!enabled) return;
  std::cout << message << std::endl;
}

void Logger::LogInfo(const std::string& message) {
  std::cout << "INFO: " << message << std::endl;
}

void Logger::LogWarn(const std::string& message) {
  std::cout << "WARNING: " << message << std::endl;
}

void Logger::LogErr(const std::string& message) {
  std::cout << "ERROR: " << message << std::endl;
}

std::string ToString(const msgpack::object& obj) {
  return (std::ostringstream() << obj).str();
}
