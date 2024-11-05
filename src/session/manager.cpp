#include "manager.hpp"
#include "app/window_funcs.h"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <stdexcept>
#include <ranges>
#include <vector>
#include "glm/gtx/string_cast.hpp"

SessionManager::SessionManager(
  SpawnMode _mode,
  sdl::Window& _window,
  SizeHandler& _sizes,
  Renderer& _renderer,
  InputHandler& _inputHandler
)
  : mode(_mode), window(_window), sizes(_sizes),
    renderer(_renderer), inputHandler(_inputHandler) {
}

int SessionManager::SessionNew(const SessionNewOpts& opts) {
  using namespace std::chrono_literals;
  bool first = sessions.empty();

  int id = currId++;
  auto name = opts.name.empty() ? std::to_string(id) : opts.name;

  auto [it, success] = sessions.try_emplace(id, id, name);
  if (!success) {
    throw std::runtime_error("Session with id " + std::to_string(id) + " already exists");
  }

  auto& session = it->second;
  auto& options = session.options;
  auto& nvim = session.nvim;
  auto& editorState = session.editorState;

  if (!nvim.ConnectStdio(opts.dir)) {
    throw std::runtime_error("Failed to connect to nvim");
  }
  nvim.Setup();

  auto optionsFut = LoadOptions(nvim);
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

  if (first) {
    window = sdl::Window({1200, 800}, "Neogurt", options);
    // needs to be in main thread
    Options::titlebarHeight = GetTitlebarHeight(window.Get());
  }

  if (options.window.borderless) {
    options.margins.top += Options::titlebarHeight;
  }

  editorState.fontFamily =
    FontFamily::FromGuifont(guifont, linespace, window.dpiScale)
      .or_else([&](const std::string& error) -> std::expected<FontFamily, std::string> {
        LOG_WARN("Failed to load font family: {}", error);
        LOG_WARN("Using default font family");
        return FontFamily::Default(linespace, window.dpiScale);
      })
      .value();

  sizes.UpdateSizes(
    window.size, window.dpiScale, editorState.fontFamily.DefaultFont().charSize,
    options.margins
  );

  if (first) {
    renderer = Renderer(sizes);
  } else {
    renderer.Resize(sizes);
  }

  editorState.winManager.sizes = sizes;
  editorState.winManager.gridManager = &editorState.gridManager;
  editorState.cursor.Resize(sizes.charSize, sizes.dpiScale);

  if (options.opacity < 1) {
    auto& hl = editorState.hlTable[0];
    hl.background = IntToColor(options.bgColor);
    hl.background->a = options.opacity;
  }

  nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);

  inputHandler = InputHandler(&nvim, &editorState.winManager, options);

  sessionsOrder.push_front(&session);

  if (!opts.switchTo) {
    SessionPrev();
  }

  return id;
}

bool SessionManager::SessionKill(int id) {
  if (id == 0) id = CurrSession()->id;

  auto it = sessions.find(id);
  if (it == sessions.end()) {
    return false;
  }
  auto& session = it->second;

  session.nvim.client->Disconnect();
  return true;
}

bool SessionManager::SessionSwitch(int id) {
  auto it = sessions.find(id);
  if (it == sessions.end()) {
    return false;
  }
  auto& session = it->second;

  UpdateSessionSizes(session);
  inputHandler =
    InputHandler(&session.nvim, &session.editorState.winManager, session.options);
  session.reattached = true;

  auto currIt = std::ranges::find(sessionsOrder, &session);
  // move to front
  if (currIt != sessionsOrder.end()) {
    sessionsOrder.erase(currIt);
    sessionsOrder.push_front(&session);
  }

  return true;
}

bool SessionManager::SessionPrev() {
  if (sessionsOrder.size() < 2) {
    return false;
  }
  SessionSwitch(sessionsOrder[1]->id);
  return true;
}

SessionListEntry SessionManager::SessionInfo(int id) {
  if (id == 0) id = CurrSession()->id;

  auto it = sessions.find(id);
  if (it == sessions.end()) {
    return {0, ""};
  }
  auto& session = it->second;

  return {session.id, session.name};
}

std::vector<SessionListEntry> SessionManager::SessionList(const SessionListOpts& opts) {
  auto entries =
    sessionsOrder | std::views::transform([](auto* session) {
      return SessionListEntry{session->id, session->name};
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
  std::erase_if(sessionsOrder, [this](auto* session) {
    bool toErase = !session->nvim.client->IsConnected();
    if (toErase) {
      sessions.erase(session->id);
    }
    return toErase;
  });

  // no sessions left
  auto* curr = CurrSession();
  if (curr == nullptr) {
    return true;
  }

  // switch to most recent session if current session was removed
  if (prevId != curr->id) {
    auto& session = *curr;

    UpdateSessionSizes(session);
    inputHandler =
      InputHandler(&session.nvim, &session.editorState.winManager, session.options);
    session.reattached = true;
  }

  return false;
}

void SessionManager::FontSizeChange(float delta, bool all) {
  if (all) {
    auto* curr = CurrSession();
    for (auto& [_, session] : sessions) {
      session.editorState.fontFamily.ChangeSize(delta);
      if (curr == &session) {
        UpdateSessionSizes(session);
      }
    }

  } else {
    if (auto* session = CurrSession()) {
      session->editorState.fontFamily.ChangeSize(delta);
      UpdateSessionSizes(*session);
    }
  }
}

void SessionManager::FontSizeReset(bool all) {
  if (all) {
    auto* curr = CurrSession();
    for (auto& [_, session] : sessions) {
      session.editorState.fontFamily.ResetSize();
      if (curr == &session) {
        UpdateSessionSizes(session);
      }
    }

  } else {
    if (auto* session = CurrSession()) {
      session->editorState.fontFamily.ResetSize();
      UpdateSessionSizes(*session);
    }
  }
}

void SessionManager::UpdateSessionSizes(SessionState& session) {
  sizes.UpdateSizes(
    window.size, window.dpiScale, session.editorState.fontFamily.DefaultFont().charSize,
    session.options.margins
  );
  renderer.Resize(sizes);
  session.editorState.winManager.sizes = sizes;
  session.editorState.cursor.Resize(sizes.charSize, sizes.dpiScale);
  session.nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);
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

