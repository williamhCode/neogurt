#pragma once

#include "nvim/msgpack_rpc/client.hpp"
#include "ui.hpp"
// #include "session.hpp"

void ParseEvents(rpc::Client& client, UiEvents& uiEvents);
