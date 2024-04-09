#include "sdl_window.hpp"
#include "app/options.hpp"
#include <stdexcept>
#include <format>

namespace sdl {

Window::Window(
  glm::uvec2 _size, const std::string& title, wgpu::PresentMode presentMode
)
    : size(_size) {
  // window ---------------------------------
  int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT;
  if (appOpts.highDpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (appOpts.borderless) flags |= SDL_WINDOW_BORDERLESS;

  window = SDL_WindowPtr(SDL_CreateWindow(title.c_str(), size.x, size.y, flags));
  if (window == nullptr) {
    throw std::runtime_error(
      std::format("Unable to Create SDL_Window: {}", SDL_GetError())
    );
  }

  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

  // sizing ---------------------------------
  dpiScale = SDL_GetWindowPixelDensity(Get());
  fbSize = size * (unsigned int)dpiScale;

  // auto displayId = SDL_GetPrimaryDisplay();
  // contentScale = SDL_GetDisplayContentScale(displayId);
  // LOG_INFO("contentScale: {}", contentScale);

  // webgpu ------------------------------------
  _ctx = WGPUContext(Get(), fbSize, presentMode);
  // LOG_INFO("WGPUContext created with size: {}, {}", fbSize.x, fbSize.y);
}

Window::~Window() {
  for (auto& [eventFilter, callback] : eventFilters) {
    SDL_DelEventWatch(eventFilter, &callback);
  }
}

SDL_Window* Window::Get() {
  return window.get();
}

void Window::AddEventWatch(EventFilter&& _callback) {
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

} // namespace sdl
