#include "./sdl_event.hpp"

namespace sdl {

static std::pair<SDL_EventFilter, EventFilter> eventFilter;

void SetEventFilter(EventFilter&& callback) {
  eventFilter = {
    [](void* userdata, SDL_Event* event) {
      auto& callback = *static_cast<EventFilter*>(userdata);
      return callback(*event);
    },
    std::move(callback)
  };
  SDL_SetEventFilter(eventFilter.first, &eventFilter.second);
}

bool PushAppExit() {
  SDL_Event event{};
  event.type = EVENT_APP_EXIT;
  return SDL_PushEvent(&event);
}

} // namespace sdl
