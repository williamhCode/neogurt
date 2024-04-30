#include "manager.hpp"

#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/process/spawn.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace bp = boost::process;
namespace fs = std::filesystem;

void SessionManager::LoadSessions(std::string_view filename) {
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string session_name;
    uint16_t port;
    if (iss >> session_name >> port) {
      sessions[session_name] = port;
    }
  }
}

void SessionManager::SaveSessions(std::string_view filename) {
  std::ofstream file(filename);
  for (auto& [key, value] : sessions) {
    file << key << " " << value << "\n";
  }
}

static uint16_t FindFreePort() {
  using namespace boost::asio;
  io_service io_service;
  ip::tcp::acceptor acceptor(io_service);
  ip::tcp::endpoint endpoint(ip::tcp::v4(), 0);
  boost::system::error_code ec;
  acceptor.open(endpoint.protocol(), ec);
  acceptor.bind(endpoint, ec);
  if (ec) {
    throw std::runtime_error("Failed to bind to open port: " + ec.message());
  }
  return acceptor.local_endpoint().port();
}

static void SpawnNvimProcess(uint16_t port) {
  std::string luaInitPath = ROOT_DIR "/lua/init.lua";
  std::string cmd = "nvim --listen localhost:" + std::to_string(port) +
                    " --headless " +
                    "--cmd \"luafile " + luaInitPath + "\"";
  bp::spawn(cmd);
}

uint16_t SessionManager::GetOrCreateSession(const std::string& session_name) {
  auto [it, inserted] = sessions.try_emplace(session_name);
  if (inserted) {
    it->second = FindFreePort();
    SpawnNvimProcess(it->second);
    LOG_INFO("Created session {} on port {}", session_name, it->second);
  }
  return it->second;
}

void SessionManager::RemoveSession(const std::string& session_name) {
  sessions.erase(session_name);
}
