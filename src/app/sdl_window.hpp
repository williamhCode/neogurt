#pragma once

#include "SDL3/SDL.h"
#include "glm/ext/vector_uint2.hpp"
#include "gfx/context.hpp"
#include "nvim/msgpack_rpc/tsqueue.hpp"
#include <vector>
#include <functional>
#include <utility>

namespace sdl {

using EventFilter = std::function<void(SDL_Event&)>;
static std::vector<std::pair<SDL_EventFilter, EventFilter>> eventFilters;

inline void AddEventWatch(EventFilter&& _callback) {
  auto& [eventFilter, callback] = eventFilters.emplace_back(
    [](void* userdata, SDL_Event* event) {
      auto& callback = *static_cast<EventFilter*>(userdata);
      callback(*event);
      return 0;
    },
    std::move(_callback)
  );
  SDL_AddEventWatch(eventFilter, &callback);
}

inline TSQueue<SDL_Event> events;

// window related
struct SDL_WindowDeleter {
  void operator()(SDL_Window* window) {
    SDL_DestroyWindow(window);
  }
};

using SDL_WindowPtr = std::unique_ptr<SDL_Window, SDL_WindowDeleter>;

struct Window {
  static inline WGPUContext _ctx;
  SDL_WindowPtr window;

  glm::uvec2 size;
  glm::uvec2 fbSize;
  float dpiScale;
  float contentScale;

  Window() = default;
  Window(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode);

  SDL_Window* Get();
};

} // namespace sdl
