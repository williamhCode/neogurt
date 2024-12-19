#pragma once

#include "SDL3/SDL_events.h"
#include <functional>

inline const Uint32 EVENT_DEFERRED_TASK = SDL_RegisterEvents(1);

void DeferToMainThread(std::function<void()>&& task);
void ProcessNextMainThreadTask();
