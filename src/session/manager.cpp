#include "manager.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <stdexcept>
#include <ranges>

SessionManager::SessionManager(
  SpawnMode _mode,
  Options& _options,
  sdl::Window& _window,
  SizeHandler& _sizes,
  Renderer& _renderer,
  InputHandler& _inputHandler
)
  : mode(_mode), options(_options), window(_window), sizes(_sizes),
    renderer(_renderer), inputHandler(_inputHandler) {
}

void SessionManager::New(const SessionNewOpts& opts) {
  using namespace std::chrono_literals;
  bool first = sessions.empty();

  int id = currId++;
  auto name = opts.name.empty() ? std::to_string(id) : opts.name;

  auto [it, success] = sessions.try_emplace(id, id, name);
  if (!success) {
    throw std::runtime_error("Session with id " + std::to_string(id) + " already exists");
  }

  auto& session = it->second;
  if (!session.nvim.ConnectStdio(opts.dir).get()) {
    throw std::runtime_error("Failed to connect to nvim");
  }

  session.nvim.UiAttach(
    100, 50,
    {
      {"rgb", true},
      {"ext_multigrid", true},
      {"ext_linegrid", true},
    }
  );

  auto optionsFut = LoadOptions(session.nvim);
  if (optionsFut.wait_for(500ms) == std::future_status::ready) {
    options = optionsFut.get();
  } else {
    LOG_WARN("Failed to load options (timeout)");
  }

  if (first) {
    window = sdl::Window({1200, 800}, "Neovim GUI", options.window);
  }

  auto guifontFut = session.nvim.GetOptionValue("guifont", {});
  std::string guifont;
  if (guifontFut.wait_for(500ms) == std::future_status::ready) {
    guifont = guifontFut.get()->as<std::string>();
  } else {
    LOG_WARN("Failed to load guifont option (timeout), using default");
    guifont = "SF Mono";
  }
  auto fontFamilyResult = FontFamily::FromGuifont(guifont, window.dpiScale);
  if (!fontFamilyResult) {
    throw std::runtime_error("Invalid guifont: " + fontFamilyResult.error());
  }
  session.editorState.fontFamily = std::move(*fontFamilyResult);

  sizes.UpdateSizes(
    window.size, window.dpiScale,
    session.editorState.fontFamily.DefaultFont().charSize, options.margins
  );

  if (first) {
    renderer = Renderer(sizes);
  }

  session.editorState.winManager.sizes = &sizes;
  session.editorState.winManager.gridManager = &session.editorState.gridManager;
  session.editorState.cursor.Init(sizes.charSize, sizes.dpiScale);

  if (options.opacity < 1) {
    auto& hl = session.editorState.hlTable[0];
    hl.background = IntToColor(options.bgColor);
    hl.background->a = options.opacity;
  }

  inputHandler = InputHandler(
    &session.nvim, &session.editorState.winManager, options.macOptIsMeta,
    options.multigrid, options.scrollSpeed
  );

  session.nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);

  sessionsOrder.push_front(&session);

  if (!opts.switchTo) {
    Prev();
  }
}

bool SessionManager::Prev() {
  if (sessionsOrder.size() < 2) {
    return false;
  }
  Switch(sessionsOrder[1]->id);
  return true;
}

void SessionManager::Switch(int id) {
  auto it = sessions.find(id);
  if (it == sessions.end()) {
    throw std::runtime_error(
      "Session with id " + std::to_string(id) + " does not exist"
    );
  }

  auto& session = it->second;

  // options.Load(session.nvim);

  // sizes.UpdateSizes(
  //   window.size, window.dpiScale,
  //   session.editorState.fontFamily.DefaultFont().charSize, options.margins
  // );

  // renderer.Resize(sizes);

  inputHandler = InputHandler(
    &session.nvim, &session.editorState.winManager, options.macOptIsMeta,
    options.multigrid, options.scrollSpeed
  );

  session.nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);
  session.reattached = true;

  auto currIt = std::ranges::find(sessionsOrder, &session);
  // move to front
  if (currIt != sessionsOrder.end()) {
    sessionsOrder.erase(currIt);
    sessionsOrder.push_front(&session);
  }
}

std::vector<SessionListEntry> SessionManager::List(const SessionListOpts& opts) {
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
  auto* curr = CurrSession();
  if (!curr->nvim.IsConnected()) {
    sessions.erase(curr->id);
    sessionsOrder.pop_front();

    curr = CurrSession();
    if (curr == nullptr) {
      return true;
    }

    auto& session = *curr;

    // options.Load(session.nvim);

    // sizes.UpdateSizes(
    //   window.size, window.dpiScale,
    //   session.editorState.fontFamily.DefaultFont().charSize, options.margins
    // );

    // renderer.Resize(sizes);

    inputHandler = InputHandler(
      &session.nvim, &session.editorState.winManager, options.macOptIsMeta,
      options.multigrid, options.scrollSpeed
    );

    session.nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);
    session.reattached = true;
  }

  return false;
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

