#pragma once

#include "session/forward.hpp"

struct EventManager {
  SessionManager& sessionManager;
  void ProcessSessionEvents(SessionHandle& session);
};
