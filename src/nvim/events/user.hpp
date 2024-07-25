#pragma once

#include "msgpack.hpp"

namespace event {

struct Session {
  std::string cmd;
  std::map<std::string, std::string> opts;
  MSGPACK_DEFINE(cmd, opts);
};

}

namespace rpc { struct Client; }
struct SessionManager;

void ProcessUserEvents(rpc::Client& client, SessionManager& SessionManager);
