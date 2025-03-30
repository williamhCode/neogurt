#pragma once

#include "app/options.hpp"
#include "events/user.hpp"
#include "session/options.hpp"
#include "session/state.hpp"
#include "app/sdl_window.hpp"
#include "gfx/renderer.hpp"
#include <deque>
#include <memory>

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

using SessionHandle = std::shared_ptr<Session>;

struct SessionManager {
private:
  SpawnMode mode;

  const StartupOptions& startupOpts;
  GlobalOptions& globalOpts;
  sdl::Window& window;
  SizeHandler& sizes;
  Renderer& renderer;

  int currId = 1;
  SessionHandle nullSession{nullptr};
  // sessions are only owned by sessions map and main thread
  // others should hold pointer/reference to sessions in the sessions map
  std::map<int, SessionHandle> sessions;

  // front to back = recency
  std::deque<SessionHandle*> sessionsOrder;

public:
  SessionManager(
    SpawnMode mode,
    const StartupOptions& startupOpts,
    GlobalOptions& globalOpts,
    sdl::Window& window,
    SizeHandler& sizes,
    Renderer& renderer
  );

  auto& Sessions() { return sessions; }
  SessionHandle& CurrSession() {
    auto it = sessionsOrder.begin();
    return it == sessionsOrder.end() ? nullSession : **it;
  }

  void OptionSet(SessionHandle& session, const event::OptionTable& optionTable);

  int SessionNew(const SessionNewOpts& opts = {});      // returns session id (0 if failed)
  bool SessionKill(int id = 0);                         // returns success
  int SessionRestart(int id = 0, bool currDir = false); // returns session id (0 if failed)
  bool SessionSwitch(int id);                           // returns success
  bool SessionPrev();                                   // returns success
  SessionListEntry SessionInfo(int id = 0);             // returns {} if failed
  std::vector<SessionListEntry> SessionList(const SessionListOpts& opts = {});
  std::optional<SessionHandle> GetCurrentSession(); // returns nullopt if should quit

  void FontSizeChange(float delta, bool all = false);
  void FontSizeReset(bool all = false);

  void UpdateSessionSizes(SessionHandle& session);

private:
  // all session switching leads to this function
  void SessionSwitchInternal(SessionHandle& session);

  // void LoadSessions(std::string_view filename);
  // void SaveSessions(std::string_view filename);

  // void SpawnNvimProcess(uint16_t port);
};
