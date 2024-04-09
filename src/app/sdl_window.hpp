#pragma once

#include "SDL3/SDL.h"
#include "glm/ext/vector_uint2.hpp"
#include "gfx/context.hpp"
#include <vector>
#include <functional>

namespace sdl {

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

  using EventFilter = std::function<void(SDL_Event&)>;
  std::vector<EventFilter> eventFilters;

  Window() = default;
  Window(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode);

  SDL_Window* Get();

  void AddEventWatch(EventFilter&& callback);
};

} // namespace sdl
