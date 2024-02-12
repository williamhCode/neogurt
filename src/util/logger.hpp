#pragma once

#include "msgpack/object.hpp"
#include <format>
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

#ifdef XCODE  // apple clang doesn't support std::format yet
  #define LOG(...) {}
#else
  #define LOG(...) logger.Log(std::format(__VA_ARGS__))
#endif

#define LOG_ENABLE() logger.enabled = true
#define LOG_DISABLE() logger.enabled = false
