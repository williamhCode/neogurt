#include "sdl_window.hpp"
#include "app/options.hpp"
#include "utils/logger.hpp"

namespace sdl {

Window::Window(
  glm::uvec2 _size, const std::string& title, wgpu::PresentMode presentMode
)
    : size(_size) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    LOG_ERR("Unable to initialize SDL: {}", SDL_GetError());
    std::exit(1);
  }

  // window ---------------------------------
  int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT;
  if (appOpts.highDpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (appOpts.borderless) flags |= SDL_WINDOW_BORDERLESS;

  window = SDL_CreateWindow(title.c_str(), size.x, size.y, flags);
  if (window == nullptr) {
    LOG_ERR("Unable to create window: {}", SDL_GetError());
    std::exit(1);
  }

  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

  // sizing ---------------------------------
  dpiScale = SDL_GetWindowPixelDensity(window);
  fbSize = size * (unsigned int)dpiScale;

  // auto displayId = SDL_GetPrimaryDisplay();
  // contentScale = SDL_GetDisplayContentScale(displayId);
  // LOG_INFO("contentScale: {}", contentScale);

  // webgpu ------------------------------------
  _ctx = WGPUContext(window, fbSize, presentMode);
  // LOG_INFO("WGPUContext created with size: {}, {}", fbSize.x, fbSize.y);
}

Window::~Window() {
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Window::AddEventWatch(EventFilter&& _callback) {
  auto& callback = eventFilters.emplace_back(std::move(_callback));
  SDL_AddEventWatch([](void* userdata, SDL_Event* event) {
    auto& callback = *static_cast<EventFilter*>(userdata);
    callback(*event);
    return 0;
  }, &callback);
}

} // namespace sdl
