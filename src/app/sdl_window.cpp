#include "sdl_window.hpp"

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_hints.h"
#include "app/options.hpp"
#include "app/window_funcs.h"
#include "utils/types.hpp"
#include <stdexcept>
#include <format>

void SDL_WindowDeleter::operator()(SDL_Window* window) {
  SDL_DestroyWindow(window);
}

namespace sdl {

using namespace wgpu;

Window::Window(glm::uvec2 _size, const std::string& title, Options::Window winOpts)
    : size(_size) {
  // window ---------------------------------
  int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT;
  if (winOpts.highDpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (winOpts.borderless) flags |= SDL_WINDOW_BORDERLESS;

  window = SDL_WindowPtr(SDL_CreateWindow(title.c_str(), size.x, size.y, flags));
  if (window == nullptr) {
    throw std::runtime_error(
      std::format("Unable to Create SDL_Window: {}", SDL_GetError())
    );
  }

  // SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
  SDL_SetHint(SDL_HINT_KEYCODE_OPTIONS, "unmodified,french_numbers");

  if (winOpts.blur > 0) {
    SetSDLWindowBlur(Get(), winOpts.blur);
  }
  EnableScrollMomentum();

  // sizing ---------------------------------
  dpiScale = SDL_GetWindowPixelDensity(Get());
  fbSize = size * (uint)dpiScale;

  // auto displayId = SDL_GetPrimaryDisplay();
  // contentScale = SDL_GetDisplayContentScale(displayId);
  // LOG_INFO("contentScale: {}", contentScale);

  // webgpu ------------------------------------
  auto presentMode = winOpts.vsync ? PresentMode::Mailbox : PresentMode::Immediate;
  _ctx = WGPUContext(Get(), fbSize, presentMode);
  // LOG_INFO("WGPUContext created with size: {}, {}", fbSize.x, fbSize.y);
}

SDL_Window* Window::Get() {
  return window.get();
}

} // namespace sdl
