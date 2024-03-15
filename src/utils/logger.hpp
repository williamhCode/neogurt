#pragma once

#include "msgpack/v3/object_fwd_decl.hpp"
#include <format>
#include <string>

struct Logger {
  bool enabled = true;

  void Log(const std::string& message);
};

std::string ToString(const msgpack::object& obj);

inline Logger logger;

#ifdef XCODE  // apple clang doesn't support std::format yet
  #define LOG(...) {}
#else
  #define LOG(...) logger.Log(std::format(__VA_ARGS__))
#endif

#define LOG_ENABLE() logger.enabled = true
#define LOG_DISABLE() logger.enabled = false
