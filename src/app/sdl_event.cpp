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

} // namespace sdl
