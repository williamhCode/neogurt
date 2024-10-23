#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "gfx/context.hpp"
#include "app/options.hpp"

struct SDL_Window;

struct SDL_WindowDeleter {
  void operator()(SDL_Window* window);
};

namespace sdl {

using SDL_WindowPtr = std::unique_ptr<SDL_Window, SDL_WindowDeleter>;

struct Window {
  SDL_WindowPtr window;

  bool vsync;
  glm::uvec2 size;
  glm::uvec2 fbSize;
  float dpiScale;

  Window() = default;
  Window(glm::uvec2 size, const std::string& title, Options& opts);

  SDL_Window* Get();
};

} // namespace sdl
