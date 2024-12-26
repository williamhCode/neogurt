#include "./manager.hpp"
#include "app/input.hpp"
#include "app/task_helper.hpp"
#include "app/window_funcs.h"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <format>
#include <stdexcept>
#include <ranges>
#include <utility>
#include <vector>
#include "glm/gtx/string_cast.hpp"

SessionManager::SessionManager(
  SpawnMode _mode, sdl::Window& _window, SizeHandler& _sizes, Renderer& _renderer
)
    : mode(_mode), window(_window), sizes(_sizes), renderer(_renderer) {
}

int SessionManager::SessionNew(const SessionNewOpts& opts) {
  using namespace std::chrono_literals;
  bool first = sessions.empty();

  int id = currId++;
  auto name = opts.name.empty() ? std::format("session {}", id) : opts.name;

  auto [it, success] = sessions.try_emplace(id, std::make_shared<Session>(id, name, opts.dir));
  if (!success) {
    throw std::runtime_error("Session with id " + std::to_string(id) + " failed to be created");
  }

  auto& session = it->second;
  auto& options = session->options;
  auto& nvim = session->nvim;
  auto& editorState = session->editorState;

  if (!nvim.ConnectStdio(Options::interactiveShell, opts.dir)) {
    throw std::runtime_error("Failed to connect to nvim");
  }
  nvim.Setup();

  auto optionsFut = Options::Load(nvim, first);
  auto guifontFut = nvim.GetOptionValue("guifont", {});
  auto linespaceFut = nvim.GetOptionValue("linespace", {});

  bool timeout = false;
  // TODO: make this non-blocking
  if (optionsFut.wait_for(1s) == std::future_status::ready) {
    options = optionsFut.get();
  } else {
    LOG_WARN("Failed to load options (timeout)");
    timeout = true;
  }

  std::string guifont;
  if (!timeout) {
    try {
      guifont = guifontFut.get()->as<std::string>();
    } catch (const msgpack::type_error& e) {
      // NOTE: neovim should cover this but just in case
      LOG_WARN("Failed to load guifont option: {}", e.what());
    }
  } else {
    LOG_WARN("Failed to load guifont option (timeout)");
  }

  int linespace = 0;
  if (!timeout) {
    try {
      linespace = linespaceFut.get()->convert();
    } catch (const msgpack::type_error& e) {
      // NOTE: neovim should cover this but just in case
      LOG_WARN("Failed to load linespace option: {}", e.what());
      linespace = 0;
    }
  } else {
    LOG_WARN("Failed to load linespace option (timeout)");
    linespace = 0;
  }

  static float titlebarHeight = 0;
  if (first) {
    window = sdl::Window({1200, 800}, "Neogurt");
    // run in main thread only, so first
    titlebarHeight = GetTitlebarHeight(window.Get());
  }
  if (Options::borderless) {
    options.marginTop += titlebarHeight;
  }

  editorState.fontFamily =
    FontFamily::FromGuifont(guifont, linespace, window.dpiScale)
      .or_else([&](const std::string& error) -> std::expected<FontFamily, std::string> {
        LOG_WARN("Failed to load font family: {}", error);
        LOG_WARN("Using default font family");
        return FontFamily::Default(linespace, window.dpiScale);
      })
      .value();

  editorState.winManager.gridManager = &editorState.gridManager;

  session->input = InputHandler(&editorState.winManager, &nvim, options);

  if (options.opacity < 1) {
    auto& hl = editorState.hlTable[0];
    hl.background = IntToColor(options.bgColor);
    hl.background->a = options.opacity;
  }

  if (first) {
    sizes.UpdateSizes(
      window.size, window.dpiScale, session->editorState.fontFamily.DefaultFont().charSize,
      session->options
    );
    renderer = Renderer(sizes);
  }

  sessionsOrder.push_back(&session);

  if (opts.switchTo) {
    SessionSwitch(session->id);
  }

  return id;
}

bool SessionManager::SessionKill(int id) {
  if (id == 0) id = CurrSession()->id;

  auto it = sessions.find(id);
  if (it == sessions.end()) {
    return false;
  }
  auto& session = *it->second;

  session.nvim.client->Disconnect();
  return true;
}

int SessionManager::SessionRestart(int id, bool currDir) {
  if (id == 0) id = CurrSession()->id;

  auto it = sessions.find(id);
  if (it == sessions.end()) {
    return false;
  }
  auto& session = it->second;

  session->nvim.client->Disconnect();

  auto dir = currDir ? session->editorState.currDir : session->dir;
  int newId = SessionNew({session->name, dir});

  return newId;
}

bool SessionManager::SessionSwitch(int id) {
  auto it = sessions.find(id);
  if (it == sessions.end()) {
    return false;
  }
  auto& session = it->second;

  // move order to front
  auto currIt = std::ranges::find(sessionsOrder, &session);
  assert(currIt != sessionsOrder.end());
  sessionsOrder.erase(currIt);
  sessionsOrder.push_front(&session);

  SessionSwitchInternal(session);

  return true;
}

bool SessionManager::SessionPrev() {
  if (sessionsOrder.size() < 2) {
    return false;
  }
  SessionSwitch((*sessionsOrder[1])->id);
  return true;
}

SessionListEntry SessionManager::SessionInfo(int id) {
  if (id == 0) id = CurrSession()->id;

  auto it = sessions.find(id);
  if (it == sessions.end()) {
    return {};
  }
  auto& session = it->second;

  return {session->id, session->name, session->dir};
}

std::vector<SessionListEntry> SessionManager::SessionList(const SessionListOpts& opts) {
  auto entries =
    sessionsOrder | std::views::transform([](auto* sessionPtr) {
      auto& session = *sessionPtr;
      return SessionListEntry{session->id, session->name, session->dir};
    }) |
    std::ranges::to<std::vector>();

  if (opts.sort == "id") {
    std::ranges::sort(entries, [&](auto& a, auto& b) {
      return opts.reverse ? a.id > b.id : a.id < b.id;
    });

  } else if (opts.sort == "name") {
    std::ranges::sort(entries, [&](auto& a, auto& b) {
      return opts.reverse ? a.name > b.name : a.name < b.name;
    });

  } else if (opts.sort == "time") {
    if (opts.reverse) std::ranges::reverse(entries);

  } else {
    throw std::runtime_error("Unknown sort option: " + opts.sort);
  }

  return entries;
}

bool SessionManager::ShouldQuit() {
  int prevId = CurrSession()->id;

  // remove disconnected sessions
  std::erase_if(sessionsOrder, [this](auto* sessionPtr) {
    auto& session = *sessionPtr;
    bool toErase = !session->nvim.client->IsConnected();
    if (toErase) sessions.erase(session->id);
    return toErase;
  });

  // check if no sessions left
  auto& currSession = CurrSession();
  if (currSession == nullptr) {
    return true;
  }

  // switch to most recent session if current session was removed
  if (prevId != currSession->id) {
    SessionSwitchInternal(currSession);
  }

  return false;
}

void SessionManager::FontSizeChange(float delta, bool all) {
  if (all) {
    auto& curr = CurrSession();
    for (auto& [_, session] : sessions) {
      session->editorState.fontFamily.ChangeSize(delta);
      if (curr == session) {
        UpdateSessionSizes(session);
      }
    }

  } else {
    if (auto& session = CurrSession()) {
      session->editorState.fontFamily.ChangeSize(delta);
      UpdateSessionSizes(session);
    }
  }
}

void SessionManager::FontSizeReset(bool all) {
  if (all) {
    auto& curr = CurrSession();
    for (auto& [_, session] : sessions) {
      session->editorState.fontFamily.ResetSize();
      if (curr == session) {
        UpdateSessionSizes(session);
      }
    }

  } else {
    if (auto session = CurrSession()) {
      session->editorState.fontFamily.ResetSize();
      UpdateSessionSizes(session);
    }
  }
}

void SessionManager::SessionSwitchInternal(SessionHandle& session) {
  UpdateSessionSizes(session);
  DeferToMainThread([winPtr = window.Get(),
                     title = std::format("Neogurt - {}", session->name)] {
    SDL_SetWindowTitle(winPtr, title.c_str());
  });
  PushSessionToMainThread(session);

  session->reattached = true;
}

void SessionManager::UpdateSessionSizes(SessionHandle& session) {
  session->editorState.fontFamily.TryChangeDpiScale(window.dpiScale);
  sizes.UpdateSizes(
    window.size, window.dpiScale,
    session->editorState.fontFamily.DefaultFont().charSize, session->options
  );
  renderer.Resize(sizes);
  session->editorState.winManager.sizes = sizes;
  session->editorState.cursor.Resize(sizes.charSize, sizes.dpiScale);
  session->nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);
}

// void SessionManager::LoadSessions(std::string_view filename) {
//   std::ifstream file(filename);
//   std::string line;
//   while (std::getline(file, line)) {
//     std::istringstream iss(line);
//     std::string session_name;
//     uint16_t port;
//     if (iss >> session_name >> port) {
//       sessions[session_name] = port;
//     }
//   }
// }

// void SessionManager::SaveSessions(std::string_view filename) {
//   std::ofstream file(filename);
//   for (auto& [key, value] : sessions) {
//     file << key << " " << value << "\n";
//   }
// }

// static uint16_t FindFreePort() {
//   using namespace boost::asio;
//   io_service io_service;
//   ip::tcp::acceptor acceptor(io_service);
//   ip::tcp::endpoint endpoint(ip::tcp::v4(), 0);
//   boost::system::error_code ec;
//   acceptor.open(endpoint.protocol(), ec);
//   acceptor.bind(endpoint, ec);
//   if (ec) {
//     throw std::runtime_error("Failed to bind to open port: " + ec.message());
//   }
//   return acceptor.local_endpoint().port();
// }

// void SessionManager::SpawnNvimProcess(uint16_t port) {
//   std::string luaInitPath = ROOT_DIR "/lua/init.lua";
//   std::string cmd = "nvim --listen localhost:" + std::to_string(port) +
//                     " --headless ";
//   LOG_INFO("Spawning nvim process: {}", cmd);
//   bp::child child(cmd);
//   child.detach();
// }

