#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "gfx/context.hpp"

struct SDL_Window;

struct SDL_WindowDeleter {
  void operator()(SDL_Window* window);
};

namespace sdl {

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
