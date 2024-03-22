#pragma once

#include "msgpack/v3/object_fwd_decl.hpp"
#include <format>
#include <string>

struct Logger {
  bool enabled = true;

  void Log(const std::string& message);
  void LogInfo(const std::string& message);
  void LogWarn(const std::string& message);
  void LogErr(const std::string& message);
};

std::string ToString(const msgpack::object& obj);

inline Logger logger;

#define LOG(...) logger.Log(std::format(__VA_ARGS__))
#define LOG_ENABLE() logger.enabled = true
#define LOG_DISABLE() logger.enabled = false
#define LOG_INFO(...) logger.LogInfo(std::format(__VA_ARGS__))
#define LOG_WARN(...) logger.LogWarn(std::format(__VA_ARGS__))
#define LOG_ERR(...) logger.LogErr(std::format(__VA_ARGS__))
