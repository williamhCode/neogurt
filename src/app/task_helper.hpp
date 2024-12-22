#pragma once

#include "SDL3/SDL_events.h"
#include <functional>
#include "session/manager.hpp"

inline const Uint32 EVENT_DEFERRED_TASK = SDL_RegisterEvents(1);
void DeferToMainThread(std::function<void()>&& task);
void ProcessNextMainThreadTask();

inline const Uint32 EVENT_SWITCH_SESSION = SDL_RegisterEvents(1);
void PushSessionToMainThread(SessionHandle session);
SessionHandle PopSessionFromMainThread();
