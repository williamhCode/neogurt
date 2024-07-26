#pragma once

#include "msgpack.hpp"

namespace event {

struct Session {
  std::string cmd;
  std::map<std::string, msgpack::type::variant> opts;
  MSGPACK_DEFINE(cmd, opts);
};

}

namespace rpc { struct Client; }
struct SessionManager;

void ProcessUserEvents(rpc::Client& client, SessionManager& SessionManager);
