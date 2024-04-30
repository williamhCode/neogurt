#pragma once

#include "nvim/msgpack_rpc/client.hpp"
#include "ui.hpp"

void ParseEvents(rpc::Client& client, UiEvents& uiEvents);
