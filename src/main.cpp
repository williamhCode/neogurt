#include "gfx/context.hpp"
#include "gfx/window.hpp"

int main() {
  Window window({1200, 800}, "Neovim GUI", wgpu::PresentMode::Fifo);

  while (!window.ShouldClose()) {
    ctx.device.Tick();

    window.PollEvents();
    for (auto &[key, scancode, action, modes] : window.keyCallbacks) {
      if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
          window.SetShouldClose(true);
        }
      }
    }
  }
}
