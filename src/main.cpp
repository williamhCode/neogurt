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

using namespace wgpu;

const WGPUContext& ctx = Window::_ctx;

Font* fontPtr;
Nvim* nvimPtr;
Renderer* rendererPtr;
EditorState* editorStatePtr;

int main() {
  Window window({1400, 800}, "Neovim GUI", PresentMode::Fifo);
  Renderer renderer(window.size);
  Font font("/Library/Fonts/SF-Mono-Medium.otf", 13, 2);
  // Font font("/Users/williamhou/Library/Fonts/Hack Regular Nerd Font Complete
  // Mono.ttf", 15, 2); Font font(ROOT_DIR
  // "/res/NerdFontsSymbolsOnly/SymbolsNerdFontMono-Regular.ttf", 15, 2);

  int width = window.size.x / font.charWidth;
  int height = window.size.y / font.charHeight;

  Nvim nvim(false);
  nvim.UiAttach(width, height);

  EditorState editorState{};

  fontPtr = &font;
  nvimPtr = &nvim;
  rendererPtr = &renderer;
  editorStatePtr = &editorState;

  window.keyCallback = [](int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      auto string = ConvertKeyInput(key, mods);
      if (string != "") nvimPtr->Input(string);
    }

    if (action == GLFW_PRESS) {
      if (key == GLFW_KEY_F2) {
        for (auto& [id, grid] : editorStatePtr->gridManager.grids) {
          LOG("grid: {} ----------------------------", id);
          for (size_t i = 0; i < grid.lines.Size(); i++) {
            auto& line = grid.lines[i];
            std::string lineStr;
            for (auto& cell : line) {
              lineStr += cell.text;
            }
            LOG("{}", lineStr);
          }
        }
      }
    }
  };

  window.charCallback = [](unsigned int codepoint) {
    auto string = ConvertCharInput(codepoint);
    if (string != "") nvimPtr->Input(string);
  };

  window.windowSizeCallback = [](int width, int height) {
    int widthChars = width / fontPtr->charWidth;
    int heightChars = height / fontPtr->charHeight;
    nvimPtr->UiTryResize(widthChars, heightChars);
    rendererPtr->Resize({width, height});
    LOG("resize: {} {}", width, height);
  };

  using namespace std::chrono_literals;
  using namespace std::chrono;

  bool running = true;

  std::thread nvimThread([&] {
    while (!window.ShouldClose()) {
      static Timer timer{60};
      timer.Start();

      ctx.device.Tick();

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

      timer.End();
      auto duration = duration_cast<milliseconds>(timer.GetAverageDuration());
      if (duration > 1us) {
        LOG("frame time: {}", duration);
      }

      if (auto hlIter = editorState.hlTable.find(0);
          hlIter != editorState.hlTable.end()) {
        auto color = hlIter->second.background.value();
        renderer.clearColor = {color.r, color.g, color.b, color.a};
      }

      {
        std::scoped_lock lock(window.renderMutex);
        renderer.Begin();
        // LOG("----------------------");
        for (auto& [id, grid] : editorState.gridManager.grids) {
          // LOG("render grid: {}", id);
          renderer.RenderGrid(grid, font, editorState.hlTable);
        }
        renderer.End();
        renderer.Present();
      }
    }

    running = false;
  });

  while (running) {
    // if (nvim.client.HasNotification())
    // window.PollEvents();
    // else
    window.WaitEvents();
    std::this_thread::sleep_for(1ms);
  }

  nvimThread.join();
}
