#pragma once

#include "msgpack.hpp"
#include "nvim/msgpack_rpc/message.hpp"
#include "session/forward.hpp"

namespace event {

using OptionTable = std::map<std::string_view, msgpack::object>;

struct NeogurtCmd {
  std::string_view cmd;
  OptionTable opts;
  MSGPACK_DEFINE(cmd, opts);
};

}

void ProcessNeogurtCmd(
  rpc::Request& request, SessionHandle& session, SessionManager& sessionManager
);
