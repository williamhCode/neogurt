#pragma once

#include "msgpack.hpp"

namespace event {

using OptionTable = std::map<std::string_view, msgpack::object>;

struct NeogurtCmd {
  std::string_view cmd;
  OptionTable opts;
  MSGPACK_DEFINE(cmd, opts);
};

}

struct SessionManager;

void ProcessUserEvents(SessionManager& SessionManager);
