#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

struct SessionManager {
  std::unordered_map<std::string, unsigned short> sessions;

  void LoadSessions(const std::string& filename);
  void SaveSessions(const std::string& filename);
  uint16_t GetOrCreateSession(const std::string& session_name);
  void RemoveSession(const std::string& session_name);
};
