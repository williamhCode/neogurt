#include "logger.hpp"
#include <iostream>
#include <sstream>
#include "msgpack/object.hpp"

void Logger::Log(const std::string& message) {
  if (!enabled) return;
  std::cout << message << std::endl;
}

std::string ToString(const msgpack::object& obj) {
  return (std::ostringstream() << obj).str();
}
