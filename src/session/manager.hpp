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
  std::string dir;
  MSGPACK_DEFINE_MAP(id, name, dir);
};

struct SessionManager {
private:
  SpawnMode mode;

  sdl::Window& window;
  SizeHandler& sizes;
  Renderer& renderer;
  InputHandler& inputHandler;

  int currId = 1;
  std::map<int, SessionState> sessions;

  // front to back = recency
  std::deque<SessionState*> sessionsOrder;

public:
  SessionManager(
    SpawnMode mode,
    sdl::Window& window,
    SizeHandler& sizes,
    Renderer& renderer,
    InputHandler& inputHandler
  );

   SessionState* CurrSession() {
    auto it = sessionsOrder.begin();
    return it == sessionsOrder.end() ? nullptr : *it;
  }

  int SessionNew(const SessionNewOpts& opts = {});      // returns session id (0 if failed)
  bool SessionKill(int id = 0);                         // returns success
  int SessionRestart(int id = 0, bool currDir = false); // returns session id (0 if failed)
  bool SessionSwitch(int id);                           // returns success
  bool SessionPrev();                                   // returns success
  SessionListEntry SessionInfo(int id = 0);             // returns {} if failed
  std::vector<SessionListEntry> SessionList(const SessionListOpts& opts = {});
  // returns true if all sessions are closed
  bool ShouldQuit();

  void FontSizeChange(float delta, bool all = false);
  void FontSizeReset(bool all = false);

private:
  // all session switching leads to this function
  void SessionSwitchInternal(SessionState& session);
  void UpdateSessionSizes(SessionState& session);

  // void LoadSessions(std::string_view filename);
  // void SaveSessions(std::string_view filename);

  // void SpawnNvimProcess(uint16_t port);
};
