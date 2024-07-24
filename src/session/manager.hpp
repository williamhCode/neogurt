#pragma once

#include "session/state.hpp"
#include <map>
#include <string>

enum class SpawnMode {
  Child,
  Detached,
};

struct SessionManager {
  SpawnMode mode;

  int currId = 0;
  std::vector<SessionState> sessions;

  SessionManager() = default;
  SessionManager(SpawnMode mode);
  ~SessionManager();

  SessionState& NewSession(std::string name = {});
  SessionState* GetSession(int id);
  SessionState* GetSession(std::string_view name);

  // void LoadSessions(std::string_view filename);
  // void SaveSessions(std::string_view filename);

  // void SpawnNvimProcess(uint16_t port);
};
