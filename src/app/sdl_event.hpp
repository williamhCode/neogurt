#pragma once

#include "SDL3/SDL_events.h"
#include <functional>

namespace sdl {

using EventFilter = std::function<bool(SDL_Event&)>;
void AddEventWatch(EventFilter&& callback);

} // namespace sdl
