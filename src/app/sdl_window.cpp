#include "./sdl_window.hpp"

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_hints.h"
#include "app/window_funcs.h"
#include "gfx/instance.hpp"
#include "utils/logger.hpp"
#include <stdexcept>
#include <format>

void SDL_WindowDeleter::operator()(SDL_Window* window) {
  SDL_DestroyWindow(window);
}

namespace sdl {

using namespace wgpu;

Window::Window(glm::uvec2 _size, const std::string& title, const AppOptions& options)
    : size(_size) {
  // window ---------------------------------
  int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT;
  if (options.highDpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  // if (winOpts.borderless) flags |= SDL_WINDOW_BORDERLESS;

  window = SDL_WindowPtr(SDL_CreateWindow(title.c_str(), size.x, size.y, flags));
  if (window == nullptr) {
    throw std::runtime_error(
      std::format("Unable to Create SDL_Window: {}", SDL_GetError())
    );
  }

  SDL_SetWindowMinimumSize(Get(), 200, 100);

  if (options.blur > 0) {
    SetSDLWindowBlur(Get(), options.blur);
  }

  if (options.borderless) {
    SetTransparentTitlebar(Get());
  }
  titlebarHeight = GetTitlebarHeight(Get());

  EnableScrollMomentum();

  // sizing ---------------------------------
  dpiScale = SDL_GetWindowPixelDensity(Get());
  fbSize = size * (uint)dpiScale;

  // webgpu ------------------------------------
  vsync = options.vsync;
  auto presentMode = vsync ? PresentMode::Mailbox : PresentMode::Immediate;
  ctx = WGPUContext(Get(), fbSize, presentMode);
  // LOG_INFO("WGPUContext created with size: {}, {}", fbSize.x, fbSize.y);
}

SDL_Window* Window::Get() {
  return window.get();
}

} // namespace sdl
