#include "./manager.hpp"
#include "SDL3/SDL_hints.h"
#include "app/input.hpp"
#include "app/task_helper.hpp"
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
  const AppOptions& _appOptions,
  sdl::Window& _window,
  SizeHandler& _sizes,
  Renderer& _renderer
)
    : mode(_mode), appOptions(_appOptions), window(_window), sizes(_sizes),
      renderer(_renderer) {
}

void SessionManager::OptionSet(const std::map<std::string_view, msgpack::object>& optionTable) {
  SessionHandle& session = CurrSession();
  SessionOptions& options = session->options;

  bool updateSizes = false;
  bool setOpacity = false;

  for (const auto& [key, value] : optionTable) {
    auto convertOption = [&]<typename T>(T& option) {
      try {
        option = value.as<T>();
      } catch (const msgpack::type_error& e) {
        throw std::runtime_error(
          std::format("failed to convert option '{}', bad type", key)
        );
      }
    };

    if (key == "margin_top") {
      convertOption(options.marginTop);
      updateSizes = true;

    } else if (key == "margin_bottom") {
      convertOption(options.marginBottom);
      updateSizes = true;

    } else if (key == "margin_left") {
      convertOption(options.marginLeft);
      updateSizes = true;

    } else if (key == "margin_right") {
      convertOption(options.marginRight);
      updateSizes = true;

    } else if (key == "macos_option_is_meta") {
      convertOption(options.macosOptionIsMeta);
      SDL_SetHint(SDL_HINT_MAC_OPTION_AS_ALT, options.macosOptionIsMeta.c_str());
      session->input.macosOptionIsMeta = ParseMacosOptionIsMeta(options.macosOptionIsMeta);

    } else if (key == "cursor_idle_time") {
      convertOption(options.cursorIdleTime);

    } else if (key == "scroll_speed") {
      convertOption(options.scrollSpeed);
      session->input.scrollSpeed = options.scrollSpeed;

    } else if (key == "bg_color") {
      convertOption(options.bgColor);
      setOpacity = true;

    } else if (key == "opacity") {
      convertOption(options.opacity);
      setOpacity = true;

    } else if (key == "fps") {
      convertOption(options.fps);

    } else {
      LOG_WARN("Unknown option: {}", key);
    }
  }

  if (updateSizes) {
    UpdateSessionSizes(session);
  }

  if (setOpacity) {
    session->editorState.hlManager.SetOpacity(options.opacity, options.bgColor);
    UpdateSessionSizes(session);
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
  auto& options = session->options;
  auto& nvim = session->nvim;
  auto& editorState = session->editorState;
  auto& input = session->input;
  auto& ime = session->ime;

  // Nvim ------------------------------------------------------
  if (!nvim.ConnectStdio(appOptions.interactive, opts.dir)) {
    throw std::runtime_error("Failed to connect to nvim");
  }
  nvim.GuiSetup(appOptions.multigrid);

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

  // if (appOptions.borderless) {
  //   options.marginTop += window.titlebarHeight;
  // }

  // EditorState ---------------------------------------------------
  editorState.winManager.gridManager = &editorState.gridManager;

  editorState.hlManager.SetOpacity(options.opacity, options.bgColor);

  editorState.fontFamily = FontFamily::Default(0, window.dpiScale).value();

  // InputHandler ---------------------------------------------------
  input.winManager = &editorState.winManager;
  input.nvim = &nvim;

  input.multigrid = appOptions.multigrid;
  input.marginTop = options.marginTop;
  SDL_SetHint(SDL_HINT_MAC_OPTION_AS_ALT, options.macosOptionIsMeta.c_str());
  input.macosOptionIsMeta = ParseMacosOptionIsMeta(options.macosOptionIsMeta);
  input.scrollSpeed = options.scrollSpeed;

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
    session->options
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
  DeferToMainThread([winPtr = window.Get(),
                     title = std::format("Neogurt - {}", session->name)] {
    SDL_SetWindowTitle(winPtr, title.c_str());
  });
  PushSessionToMainThread(session);

  session->reattached = true;
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

