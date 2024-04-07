#pragma once

#include "SDL3/SDL.h"
#include "glm/ext/vector_uint2.hpp"
#include "gfx/context.hpp"

namespace sdl {

struct Window {
  static inline WGPUContext _ctx;
  SDL_Window* window;

  glm::uvec2 size;
  glm::uvec2 fbSize;
  float dpiScale;
  // float contentScale;

  Window() = default;
  Window(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode);
  ~Window();
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;


  using EventFilter = std::function<void(SDL_Event&)>;
  std::vector<EventFilter> eventFilters;
  void AddEventWatch(EventFilter&& callback);
};

} // namespace sdl
