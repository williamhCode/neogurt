#include "manager.hpp"
#include "utils/color.hpp"
#include <algorithm>
#include <stdexcept>

SessionManager::SessionManager(
  SpawnMode _mode,
  Options& _options,
  sdl::Window& _window,
  SizeHandler& _sizes,
  Renderer& _renderer
)
  : mode(_mode), options(_options), window(_window), sizes(_sizes),
    renderer(_renderer) {
}

void SessionManager::NewSession(const NewSessionOpts& opts) {
  bool first = sessions.empty();
  int id = currId++;
  auto [it, success] = sessions.try_emplace(id, SessionState{
    .id = id,
    .name = opts.name,
    .nvim{},
    .editorState{},
    .inputHandler{},
  });
  if (!success) {
    throw std::runtime_error("Session with id " + std::to_string(id) + " already exists");
  }

  auto& session = it->second;
  if (!session.nvim.ConnectStdio(opts.dir)) {
    throw std::runtime_error("Failed to connect to nvim");
  }

  options.Load(session.nvim);

  if (first) {
    window = sdl::Window({1200, 800}, "Neovim GUI", options.window);
  }

  std::string guifont = session.nvim.GetOptionValue("guifont", {}).get()->convert();
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

  session.editorState.Init(sizes);

  if (options.transparency < 1) {
    auto& hl = session.editorState.hlTable[0];
    hl.background = IntToColor(options.bgColor);
    hl.background->a = options.transparency;
  }

  session.inputHandler = InputHandler(
    &session.nvim, &session.editorState.winManager, options.macOptIsMeta,
    options.multigrid, options.scrollSpeed
  );

  if (opts.switchTo) {
    session.nvim.UiAttach(
      sizes.uiWidth, sizes.uiHeight,
      {
        {"rgb", true},
        {"ext_multigrid", options.multigrid},
        {"ext_linegrid", true},
      }
    ).wait();

    if (Curr() != nullptr) {
      Curr()->nvim.UiDetach().wait();
    }
    sessionsOrder.push_front(&session);
  } else {
    sessionsOrder.push_back(&session);
  }
}

bool SessionManager::PrevSession() {
  if (sessionsOrder.size() < 2) {
    return false;
  }
  SwitchSession(sessionsOrder[1]->id);
  return true;
}

void SessionManager::SwitchSession(int id) {
  auto it = sessions.find(id);
  if (it == sessions.end()) {
    throw std::runtime_error(
      "Session with id " + std::to_string(id) + " does not exist"
    );
  }
  Curr()->nvim.UiDetach().wait();

  auto& session = it->second;

  options.Load(session.nvim);

  sizes.UpdateSizes(
    window.size, window.dpiScale,
    session.editorState.fontFamily.DefaultFont().charSize, options.margins
  );

  renderer.Resize(sizes);

  session.nvim.UiAttach(
    sizes.uiWidth, sizes.uiHeight,
    {
      {"rgb", true},
      {"ext_multigrid", options.multigrid},
      {"ext_linegrid", true},
    }
  ).wait();

  auto currIt = std::ranges::find(sessionsOrder, &session);
  // move to front
  if (currIt != sessionsOrder.end()) {
    sessionsOrder.erase(currIt);
    sessionsOrder.push_front(&session);
  }
}

bool SessionManager::Update() {
  auto* curr = Curr();
  if (!curr->nvim.IsConnected()) {
    sessions.erase(curr->id);
    sessionsOrder.pop_front();

    curr = Curr();
    if (curr == nullptr) {
      return true;
    }

    auto& session = *curr;

    options.Load(session.nvim);

    sizes.UpdateSizes(
      window.size, window.dpiScale,
      session.editorState.fontFamily.DefaultFont().charSize, options.margins
    );

    renderer.Resize(sizes);

    session.nvim.UiAttach(
      sizes.uiWidth, sizes.uiHeight,
      {
        {"rgb", true},
        {"ext_multigrid", options.multigrid},
        {"ext_linegrid", true},
      }
    ).wait();
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

