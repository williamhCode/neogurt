#include "app/window.hpp"
#include "editor/grid.hpp"
#include "editor/state.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "nvim/input.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/nvim.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/unicode.hpp"
#include "utils/variant.hpp"
#include <atomic>

using namespace wgpu;

const WGPUContext& ctx = Window::_ctx;

int main() {
  // Window window({1400, 800}, "Neovim GUI", PresentMode::Fifo);
  Window window({1600, 1000}, "Neovim GUI", PresentMode::Immediate);
  Renderer renderer(window.size, window.fbSize);
  Font font("/Library/Fonts/SF-Mono-Medium.otf", 13, 2);
  // Font font("/Users/williamhou/Library/Fonts/Hack Regular Nerd Font Complete
  // Mono.ttf", 15, 2); Font font(ROOT_DIR
  // "/res/NerdFontsSymbolsOnly/SymbolsNerdFontMono-Regular.ttf", 15, 2);

  int width = window.size.x / font.charWidth;
  int height = window.size.y / font.charHeight;

  Nvim nvim(false);
  nvim.UiAttach(width, height);

  EditorState editorState{};

  std::atomic_bool rendered = true;
  std::mutex renderMutex;

  window.keyCallback = [&](int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      auto string = ConvertKeyInput(key, mods);
      if (string != "") nvim.Input(string);
    }
  };

  window.charCallback = [&](unsigned int codepoint) {
    auto string = ConvertCharInput(codepoint);
    if (string != "") nvim.Input(string);
  };

  window.windowSizeCallback = [&](int width, int height) {
    glm::uvec2 size(width, height);
    int widthChars = size.x / font.charWidth;
    int heightChars = size.y / font.charHeight;
    nvim.UiTryResize(widthChars, heightChars);
    {
      std::scoped_lock lock(renderMutex);
      renderer.Resize(size);
    }

    using namespace std::chrono_literals;
    rendered = false;
    while (!rendered) {
      std::this_thread::sleep_for(1ms);
    }
  };

  window.framebufferSizeCallback = [&](int width, int height) {
    glm::uvec2 fbSize(width, height);
    {
      std::scoped_lock lock(renderMutex);
      window._ctx.Resize(fbSize);
      renderer.FbResize(fbSize);
    }
  };

  using namespace std::chrono_literals;
  using namespace std::chrono;

  std::atomic_bool running = true;

  std::thread nvimThread([&] {
    while (!window.ShouldClose()) {
      // static Timer timer{60};
      // timer.Start();

      {
        std::scoped_lock lock(renderMutex);
        ctx.device.Tick();
      }

      // events ------------------------------------------------
      if (window.ShouldClose()) nvim.client.Disconnect();
      if (!nvim.client.IsConnected()) window.SetShouldClose(true);

      // LOG("\n -----------------------------------------------");
      nvim.ParseEvents();

      // parse redraw and rendering ------------------------------------------------
      // clang-format off
      // if using continue, have to lock main loop to 60 fps, or will keep looping and waste cpu cycles
      // else just let webgpu vsync handle timing, but will spend extra power on rendering when there's no need
      // best prob make own timer to lock main loop to 60 fps when using continue
      // clang-format on
      // if (nvim.redrawState.numFlushes == 0) continue;

      // LOG_DISABLE();
      // size of queue
      // timer.Start();
      ProcessRedrawEvents(nvim.redrawState, editorState);
      // LOG_ENABLE();
      // timer.End();
      // auto duration = duration_cast<nanoseconds>(timer.GetAverageDuration());
      // LOG("rendering: {}", duration);

      // timer.End();
      // auto duration = duration_cast<milliseconds>(timer.GetAverageDuration());
      // if (duration > 1us) {
      //   LOG("frame time: {}", duration);
      // }

      if (auto hlIter = editorState.hlTable.find(0);
          hlIter != editorState.hlTable.end()) {
        auto color = hlIter->second.background.value();
        renderer.clearColor = {color.r, color.g, color.b, color.a};
      }

      {
        std::scoped_lock lock(renderMutex);
        renderer.Begin();
        // LOG("----------------------");
        for (auto& [id, grid] : editorState.gridManager.grids) {
          // LOG("render grid: {}", id);
          renderer.RenderGrid(grid, font, editorState.hlTable);
          glm::vec2 cursorPos(
            grid.cursorCol * font.charWidth, grid.cursorRow * font.charHeight
          );
          renderer.RenderCursor(cursorPos, {font.charWidth, font.charHeight});
        }
        renderer.End();
        renderer.Present();
        rendered = true;
      }
    }

    running = false;
    glfwPostEmptyEvent();
  });

  while (running) {
    window.WaitEvents();
    std::this_thread::sleep_for(1ms);
  }

  nvimThread.join();
}
