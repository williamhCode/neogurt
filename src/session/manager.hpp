#pragma once

#include "session/state.hpp"
#include "app/options.hpp"
#include "app/sdl_window.hpp"
#include "gfx/renderer.hpp"
#include <deque>

enum class SpawnMode {
  Child,
  Detached,
};

struct NewSessionOpts {
  std::string name;
  std::string dir;
  bool switchTo = true;
};

struct SessionManager {
  SpawnMode mode;

  Options& options;
  sdl::Window& window;
  SizeHandler& sizes;
  Renderer& renderer;

  int currId = 0;
  std::map<int, SessionState> sessions;
  // SessionState* prevSession = nullptr;
  // SessionState* currSession = nullptr;

  // front to back = recency
  std::deque<SessionState*> sessionsOrder;
  inline SessionState* Curr() {
    auto it = sessionsOrder.begin();
    return it == sessionsOrder.end() ? nullptr : *it;
  }

  SessionManager(
    SpawnMode mode,
    Options& options,
    sdl::Window& window,
    SizeHandler& sizes,
    Renderer& renderer
  );

  // return string if error
  std::string NewSession(const NewSessionOpts& opts = {});
  std::string SwitchSession(int id);

  // returns true if all sessions are closed
  bool Update();

  // void LoadSessions(std::string_view filename);
  // void SaveSessions(std::string_view filename);

  // void SpawnNvimProcess(uint16_t port);
};
