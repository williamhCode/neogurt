#pragma once

#include "SDL3/SDL_events.h"
#include "nvim/msgpack_rpc/tsqueue.hpp"
#include <vector>
#include <functional>
#include <utility>

namespace sdl {

using EventFilter = std::function<void(SDL_Event&)>;
void AddEventWatch(EventFilter&& callback);

} // namespace sdl
