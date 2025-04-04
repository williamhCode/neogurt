#include "./sdl_window.hpp"

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_hints.h"
#include "app/window_funcs.h"
#include "gfx/instance.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <stdexcept>
#include <format>

void SDL_WindowDeleter::operator()(SDL_Window* window) {
  SDL_DestroyWindow(window);
}

namespace sdl {

using namespace wgpu;

Window::Window(glm::uvec2 _size, const std::string& title, const GlobalOptions& globalOpts)
    : size(_size) {
  // window ---------------------------------
  int flags =
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_HIGH_PIXEL_DENSITY;

  window = SDL_WindowPtr(SDL_CreateWindow(title.c_str(), size.x, size.y, flags));
  if (window == nullptr) {
    throw std::runtime_error(
      std::format("Unable to Create SDL_Window: {}", SDL_GetError())
    );
  }

  SDL_SetWindowMinimumSize(Get(), 200, 100);

  int blur = std::max(globalOpts.blur, 0);
  SetSDLWindowBlur(Get(), blur);

  realTitlebarHeight = GetTitlebarHeight(Get());
  titlebarHeight = 0;

  // sizing ---------------------------------
  dpiScale = SDL_GetWindowPixelDensity(Get());
  fbSize = size * (uint)dpiScale;

  // webgpu ------------------------------------
  ctx = WGPUContext(Get(), fbSize, globalOpts.vsync, globalOpts.gamma);
  // LOG_INFO("WGPUContext created with size: {}, {}", fbSize.x, fbSize.y);
}

SDL_Window* Window::Get() {
  return window.get();
}

} // namespace sdl
