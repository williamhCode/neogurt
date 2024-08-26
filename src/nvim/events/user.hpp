#pragma once

#include "msgpack.hpp"

namespace event {

struct NeoguiCmd {
  std::string_view cmd;
  std::map<std::string_view, msgpack::object> opts;
  MSGPACK_DEFINE(cmd, opts);
};

}

namespace rpc { struct Client; }
struct SessionManager;

void ProcessUserEvents(rpc::Client& client, SessionManager& SessionManager);
