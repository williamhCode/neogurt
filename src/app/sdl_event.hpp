#pragma once

#include "SDL3/SDL_events.h"
#include <functional>

namespace sdl {

using EventFilter = std::function<bool(SDL_Event&)>;
void SetEventFilter(EventFilter&& callback);

inline const Uint32 EVENT_APP_EXIT = SDL_RegisterEvents(1);
bool PushAppExit();

} // namespace sdl
