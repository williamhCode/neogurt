#pragma once

#include "app/input.hpp"
#include "session/state.hpp"
#include "app/options.hpp"
#include "app/sdl_window.hpp"
#include "gfx/renderer.hpp"
#include <deque>

enum class SpawnMode {
  Child,
  Detached,
};

struct SessionNewOpts {
  std::string name;
  std::string dir;
  bool switchTo = true;
};

struct SessionListOpts {
  // id, name, time (recency)
  std::string sort = "id";
  bool reverse = false;
};

struct SessionListEntry {
  int id;
  std::string name;
  MSGPACK_DEFINE(id, name);
};

struct SessionManager {
private:
  SpawnMode mode;

  Options& options;
  sdl::Window& window;
  SizeHandler& sizes;
  Renderer& renderer;
  InputHandler& inputHandler;

  int currId = 0;
  std::map<int, SessionState> sessions;

  // front to back = recency
  std::deque<SessionState*> sessionsOrder;

public:
  SessionManager(
    SpawnMode mode,
    Options& options,
    sdl::Window& window,
    SizeHandler& sizes,
    Renderer& renderer,
    InputHandler& inputHandler
  );

  inline SessionState* CurrSession() {
    auto it = sessionsOrder.begin();
    return it == sessionsOrder.end() ? nullptr : *it;
  }

  void New(const SessionNewOpts& opts = {});
  bool Prev(); // return true if successful
  void Switch(int id);
  std::vector<SessionListEntry> List(const SessionListOpts& opts = {});

  // returns true if all sessions are closed
  bool ShouldQuit();

  // void LoadSessions(std::string_view filename);
  // void SaveSessions(std::string_view filename);

  // void SpawnNvimProcess(uint16_t port);
};
