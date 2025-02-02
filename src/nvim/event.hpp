#pragma once

#include "nvim/events/ui.hpp"
#include "nvim/msgpack_rpc/client.hpp"

void ParseNotifications(rpc::Client& client, UiEvents& uiEvents);
