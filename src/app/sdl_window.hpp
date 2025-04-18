#pragma once

#include "session/options.hpp"
#include "glm/ext/vector_uint2.hpp"
#include <memory>

struct SDL_Window;

struct SDL_WindowDeleter {
  void operator()(SDL_Window* window);
};

namespace sdl {

using SDL_WindowPtr = std::unique_ptr<SDL_Window, SDL_WindowDeleter>;

struct Window {
  SDL_WindowPtr window;

  glm::uvec2 size;
  glm::uvec2 fbSize;
  float dpiScale;

  int realTitlebarHeight;
  int titlebarHeight;

  Window(glm::uvec2 size, const std::string& title, const GlobalOptions& globalOpts);

  SDL_Window* Get();
};

} // namespace sdl
