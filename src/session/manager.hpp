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

struct NewSessionOpts {
  std::string name;
  std::string dir;
  bool switchTo = true;
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

  void NewSession(const NewSessionOpts& opts = {});
  bool PrevSession(); // return true if successful
  void SwitchSession(int id);

  // returns true if all sessions are closed
  bool ShouldQuit();

  // void LoadSessions(std::string_view filename);
  // void SaveSessions(std::string_view filename);

  // void SpawnNvimProcess(uint16_t port);
};
