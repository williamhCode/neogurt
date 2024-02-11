#pragma once

#include "msgpack.hpp"
#include <sstream>
#include <string>
#include <iostream>

struct Logger {
  bool enabled = true;

  void Log(const std::string& message) {
    if (!enabled) return;
    std::cout << message << std::endl;
  }
};

inline std::string ToString(const msgpack::object& obj) {
  return (std::ostringstream() << obj).str();
}

inline Logger logger;

#define LOG(...) logger.Log(std::format(__VA_ARGS__))
#define LOG_ENABLE() logger.enabled = true
#define LOG_DISABLE() logger.enabled = false
