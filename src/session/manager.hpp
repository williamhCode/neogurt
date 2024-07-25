#pragma once

#include <map>
#include <string>

// Forward decl
struct Options;
namespace sdl { struct Window; }
struct SizeHandler;
struct Renderer;
struct SessionState;

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
  SessionState* prevSession = nullptr;
  SessionState* currSession = nullptr;

  SessionManager(
    SpawnMode mode,
    Options& options,
    sdl::Window& window,
    SizeHandler& sizes,
    Renderer& renderer
  );
  ~SessionManager();

  // return string if error
  std::string NewSession(const NewSessionOpts& opts = {});
  std::string SwitchSession(int id);

  // returns true if all sessions are closed
  bool Update();

  // void LoadSessions(std::string_view filename);
  // void SaveSessions(std::string_view filename);

  // void SpawnNvimProcess(uint16_t port);
};
