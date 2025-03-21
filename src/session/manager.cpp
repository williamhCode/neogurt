#include "./manager.hpp"
#include "SDL3/SDL_hints.h"
#include "app/input.hpp"
#include "app/task_helper.hpp"
#include "app/window_funcs.h"
#include "editor/highlight.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <format>
#include <stdexcept>
#include <ranges>
#include <utility>
#include <vector>
#include "glm/gtx/string_cast.hpp"

SessionManager::SessionManager(
  SpawnMode _mode,
  const StartupOptions& _startupOpts,
  GlobalOptions& _globalOpts,
  sdl::Window& _window,
  SizeHandler& _sizes,
  Renderer& _renderer
)
    : mode(_mode), startupOpts(_startupOpts), globalOpts(_globalOpts), window(_window),
      sizes(_sizes), renderer(_renderer) {
}

void SessionManager::OptionSet(const std::map<std::string_view, msgpack::object>& optionTable) {
  SessionHandle& session = CurrSession();
  SessionOptions& sessionOpts = session->sessionOpts;

  bool setOpacity = false;
  bool updateSizes = false;
  bool resizeCtx = false;

  for (const auto& [key, value] : optionTable) {
    // returns true if option changed
    auto convertOption = [&]<typename T>(T& option) -> bool {
      try {
        T optionOld = option;
        option = value.as<T>();
        return option != optionOld;
      } catch (const msgpack::type_error& e) {
        throw std::runtime_error(
          std::format("failed to convert option '{}', bad type", key)
        );
      }
    };

    if (key == "borderless") {
      if (convertOption(globalOpts.borderless)) {
        ExecuteOnMainThread([win = window.Get(), borderless = globalOpts.borderless] {
          SetTitlebarStyle(win, borderless);
        });
      }

    } else if (key == "blur") {
      if (convertOption(globalOpts.blur)) {
        int blur = std::max(globalOpts.blur, 0);
        SetSDLWindowBlur(window.Get(), blur);
      }

    } else if (key == "gamma") {
      convertOption(globalOpts.gamma);

    } else if (key == "vsync") {
      if (convertOption(globalOpts.vsync)) {
        resizeCtx = true;
      }

    } else if (key == "fps") {
      convertOption(globalOpts.fps);

    } else if (key == "margin_top") {
      if (convertOption(sessionOpts.marginTop)) {
        updateSizes = true;
      }

    } else if (key == "margin_bottom") {
      if (convertOption(sessionOpts.marginBottom)) {
        updateSizes = true;
      }

    } else if (key == "margin_left") {
      if (convertOption(sessionOpts.marginLeft)) {
        updateSizes = true;
      }

    } else if (key == "margin_right") {
      if (convertOption(sessionOpts.marginRight)) {
        updateSizes = true;
      }

    } else if (key == "macos_option_is_meta") {
      if (convertOption(sessionOpts.macosOptionIsMeta)) {
        SDL_SetHint(SDL_HINT_MAC_OPTION_AS_ALT, sessionOpts.macosOptionIsMeta.c_str());
        session->input.macosOptionIsMeta = ParseMacosOptionIsMeta(sessionOpts.macosOptionIsMeta);
      }

    } else if (key == "cursor_idle_time") {
      convertOption(sessionOpts.cursorIdleTime);

    } else if (key == "scroll_speed") {
      if (convertOption(sessionOpts.scrollSpeed)) {
        session->input.scrollSpeed = sessionOpts.scrollSpeed;
      }

    } else if (key == "bg_color") {
      if (convertOption(sessionOpts.bgColor)) {
        setOpacity = true;
        updateSizes = true;
      }

    } else if (key == "opacity") {
      if (convertOption(sessionOpts.opacity)) {
        setOpacity = true;
        updateSizes = true;
      }

    } else {
      LOG_WARN("Unknown option: {}", key);
    }
  }

  if (setOpacity) {
    session->editorState.hlManager.SetOpacity(sessionOpts.opacity, sessionOpts.bgColor);
  }

  if (updateSizes) {
    UpdateSessionSizes(session);
  }

  // resize ctx after sizes are updated
  if (resizeCtx) {
    ctx.Resize(sizes.fbSize, globalOpts.vsync);
  }
}

int SessionManager::SessionNew(const SessionNewOpts& opts) {
  using namespace std::chrono_literals;

  int id = currId++;
  auto name = opts.name.empty() ? std::format("session {}", id) : opts.name;

  auto [it, success] = sessions.try_emplace(id, std::make_shared<Session>(id, name, opts.dir));
  if (!success) {
    throw std::runtime_error("Session with id " + std::to_string(id) + " failed to be created");
  }

  auto& session = it->second;
  auto& sessionOpts = session->sessionOpts;
  auto& nvim = session->nvim;
  auto& editorState = session->editorState;
  auto& input = session->input;
  auto& ime = session->ime;

  // Nvim ------------------------------------------------------
  if (!nvim.ConnectStdio(startupOpts.interactive, opts.dir)) {
    throw std::runtime_error("Failed to connect to nvim");
  }
  nvim.GuiSetup(startupOpts.multigrid);

  // Options ---------------------------------------------------
  // auto optionsFut = GetAll(
  //   SessionOptions::Load(nvim),
  //   nvim.GetHl(0, {{"name", "NeogurtImeNormal"}, {"link", false}}),
  //   nvim.GetHl(0, {{"name", "NeogurtImeSelected"}, {"link", false}})
  // );

  // if (optionsFut.wait_for(1s) == std::future_status::ready) {
  //   msgpack::object_handle imeNormalHlObj, imeSelectedHlObj;
  //   std::tie(options, imeNormalHlObj, imeSelectedHlObj) = optionsFut.get();

  //   ImeHandler::imeNormalHlId = -1;
  //   ImeHandler::imeSelectedHlId = -2;
  //   editorState.hlTable[ImeHandler::imeNormalHlId] = Highlight::FromDesc(imeNormalHlObj->convert());
  //   editorState.hlTable[ImeHandler::imeSelectedHlId] = Highlight::FromDesc(imeSelectedHlObj->convert());
  // } else {
  //   LOG_WARN("Failed to load options (timeout)");
  // }

  // if (startupOpts.borderless) {
  //   options.marginTop += window.titlebarHeight;
  // }

  // EditorState ---------------------------------------------------
  editorState.winManager.gridManager = &editorState.gridManager;

  editorState.hlManager.SetOpacity(sessionOpts.opacity, sessionOpts.bgColor);

  editorState.fontFamily = FontFamily::Default(0, window.dpiScale).value();

  // InputHandler ---------------------------------------------------
  input.winManager = &editorState.winManager;
  input.nvim = &nvim;

  input.multigrid = startupOpts.multigrid;
  input.marginTop = sessionOpts.marginTop;
  SDL_SetHint(SDL_HINT_MAC_OPTION_AS_ALT, sessionOpts.macosOptionIsMeta.c_str());
  input.macosOptionIsMeta = ParseMacosOptionIsMeta(sessionOpts.macosOptionIsMeta);
  input.scrollSpeed = sessionOpts.scrollSpeed;

  // ImeHandler ---------------------------------------------------
  ime.editorState = &editorState;

  // others -----------------------------------------------------
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

std::optional<SessionHandle> SessionManager::GetCurrentSession() {
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
    return {};
  }

  // switch to most recent session if current session was removed
  if (prevId != currSession->id) {
    SessionSwitchInternal(currSession);
  }

  return currSession;
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
    if (auto& session = CurrSession()) {
      session->editorState.fontFamily.ResetSize();
      UpdateSessionSizes(session);
    }
  }
}

void SessionManager::UpdateSessionSizes(SessionHandle& session) {
  session->editorState.fontFamily.TryChangeDpiScale(window.dpiScale);
  sizes.UpdateSizes(
    window.size, window.dpiScale, session->editorState.fontFamily.GetCharSize(),
    session->sessionOpts
  );
  renderer.Resize(sizes);
  session->editorState.winManager.sizes = sizes;
  session->editorState.cursor.Resize(sizes.charSize, sizes.dpiScale);
  // force to make nvim update all windows even if uiSize is the same
  session->nvim.UiTryResize(sizes.uiWidth + 1, sizes.uiHeight);
  session->nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);
}

void SessionManager::SessionSwitchInternal(SessionHandle& session) {
  UpdateSessionSizes(session);
  ExecuteOnMainThread([win = window.Get(),
                       title = std::format("Neogurt - {}", session->name)] {
    SDL_SetWindowTitle(win, title.c_str());
  });
  SDL_SetHint(SDL_HINT_MAC_OPTION_AS_ALT, session->sessionOpts.macosOptionIsMeta.c_str());
  session->reattached = true;

  PushSessionToMainThread(session);
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

