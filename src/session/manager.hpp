#pragma once

#include <cstdint>
#include <unordered_map>
#include <string_view>
#include <string>

struct SessionManager {
  std::unordered_map<std::string, uint16_t> sessions;

  void LoadSessions(std::string_view filename);
  void SaveSessions(std::string_view filename);
  uint16_t GetOrCreateSession(const std::string& session_name);
  void RemoveSession(const std::string& session_name);
};
