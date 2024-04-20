#include "sdl_event.hpp"

namespace sdl {

static std::vector<std::pair<SDL_EventFilter, EventFilter>> eventFilters;

void AddEventWatch(EventFilter&& callback) {
  auto& [filter, userData] = eventFilters.emplace_back(
    [](void* userdata, SDL_Event* event) {
      auto& callback = *static_cast<EventFilter*>(userdata);
      callback(*event);
      return 0;
    },
    std::move(callback)
  );
  SDL_AddEventWatch(filter, &userData);
}

} // namespace sdl
