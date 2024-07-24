#include "manager.hpp"
#include "utils/logger.hpp"

// #include "boost/asio/io_service.hpp"
// #include "boost/asio/ip/tcp.hpp"
// #include "utils/logger.hpp"
// #include <fstream>
// #include <sstream>
// #include <filesystem>

namespace bp = boost::process;
namespace fs = std::filesystem;

SessionManager::SessionManager(SpawnMode _mode) : mode(_mode) {
  if (mode == SpawnMode::Child) {

  } else if (mode == SpawnMode::Detached) {
    // std::string filename = ROOT_DIR "nvim-sessions.txt";
    // LoadSessions(filename);
  }
}

SessionManager::~SessionManager() {
  if (mode == SpawnMode::Child) {

  } else if (mode == SpawnMode::Detached) {
    // std::string filename = ROOT_DIR "nvim-sessions.txt";
    // SaveSessions(filename);
  }
}

SessionState& SessionManager::NewSession(std::string name) {
  if (mode == SpawnMode::Child) {
    int id = currId++;
    return sessions.emplace_back(SessionState{
      .id = id,
      .name = std::move(name),
      .nvim{},
      .editorState{},
      .inputHandler{},
    });
  }
  assert(false);
}

SessionState* SessionManager::GetSession(int id) {
  for (auto& session : sessions) {
    if (session.id == id) {
      return &session;
    }
  }
  return nullptr;
}

SessionState* SessionManager::GetSession(std::string_view name) {
  for (auto& session : sessions) {
    if (session.name == name) {
      return &session;
    }
  }
  return nullptr;
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

