#pragma once

#include <cstdint>
#include <unordered_map>
#include <string_view>
#include <string>
#include "boost/process/child.hpp"

enum class SpawnMode {
  Child,
  Detached,
};

struct SessionManager {
  SpawnMode mode;
  std::unordered_map<std::string, uint16_t> sessions;
  // used if SpawnMode is Child
  std::vector<boost::process::child> processes;

  SessionManager() = default;
  SessionManager(SpawnMode mode);
  ~SessionManager();

  void LoadSessions(std::string_view filename);
  void SaveSessions(std::string_view filename);

  void SpawnNvimProcess(uint16_t port);
  uint16_t GetOrCreateSession(const std::string& session_name);
  void RemoveSession(const std::string& session_name);
};
