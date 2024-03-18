#include "GLFW/glfw3.h"
#include "app/options.hpp"
#include "app/window.hpp"
#include "app/input.hpp"
#include "editor/grid.hpp"
#include "editor/state.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/nvim.hpp"
#include "utils/clock.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"

#include <iostream>
#include <format>
#include <atomic>
#include <chrono>
#include <vector>

using namespace wgpu;
using namespace std::chrono_literals;
using namespace std::chrono;

const WGPUContext& ctx = Window::_ctx;
AppOptions options;

int main() {
  // AppWindow window({1400, 800}, "Neovim GUI", PresentMode::Immediate);
  Window window({1600, 1000}, "Neovim GUI", PresentMode::Immediate);
  Renderer renderer(window.size, window.fbSize);
  Font font("/Library/Fonts/SF-Mono-Medium.otf", 13, window.dpiScale);
  // Font font("/Users/williamhou/Library/Fonts/Hack Regular Nerd Font Complete
  // Mono.ttf", 15, 2); Font font(ROOT_DIR
  // "/res/NerdFontsSymbolsOnly/SymbolsNerdFontMono-Regular.ttf", 15, 2);

  int uiRows = window.size.x / font.charSize.x;
  int uiCols = window.size.y / font.charSize.y;

  Nvim nvim(false);
  nvim.UiAttach(
    uiRows, uiCols,
    {
      {"rgb", true},
      {"ext_multigrid", true},
      {"ext_linegrid", true},
    }
  );

  EditorState editorState{
    .gridManager{
      .gridSize = font.charSize,
      .dpiScale = window.dpiScale,
    },
    .winManager{
      .gridSize = font.charSize,
    },
    .cursor{.fullSize = font.charSize},
  };
  editorState.winManager.gridManager = &editorState.gridManager;

  // lock whenever ctx.device is used
  std::mutex wgpuDeviceMutex;

  // input -----------------------------------------------------------
  InputHandler input(nvim, window.GetCursorPos(), font.charSize);

  window.keyCallback = [&](int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_F2) {
      for (auto& [id, grid] : editorState.gridManager.grids) {
        LOG("grid {}: dirty={}", id, grid.dirty);
      }
      for (auto& [id, grid] : editorState.winManager.windows) {
        LOG("win {}: hidden={}", id, grid.hidden);
      }
    }
    input.HandleKey(key, scancode, action, mods);
  };

  window.charCallback = [&](unsigned int codepoint) {
    input.HandleChar(codepoint);
  };

  window.mouseButtonCallback = [&](int button, int action, int mods) {
    input.HandleMouseButton(button, action, mods);
  };

  window.cursorPosCallback = [&](double xpos, double ypos) {
    input.HandleCursorPos(xpos, ypos);
  };

  window.scrollCallback = [&](double xoffset, double yoffset) {
    input.HandleScroll(xoffset, yoffset);
  };

  // resizing and dpi changed -------------------------------------
  window.framebufferSizeCallback = [&](int width, int height) {
    std::scoped_lock lock(wgpuDeviceMutex);
    window._ctx.Resize(window.fbSize);
    renderer.FbResize(window.fbSize);

    glm::vec2 size(window.fbSize / (unsigned int)window.dpiScale);
    int rows = size.x / font.charSize.x;
    int cols = size.y / font.charSize.y;
    nvim.UiTryResize(rows, cols);
    renderer.Resize(size);
  };

  window.windowContentScaleCallback = [&](float xscale, float yscale) {
    std::scoped_lock lock(wgpuDeviceMutex);
    font = Font("/Library/Fonts/SF-Mono-Medium.otf", 15, window.dpiScale);
  };

  // main thread -----------------------------------
  std::atomic_bool windowShouldClose = false;

  window.windowCloseCallback = [&] {
    windowShouldClose = true;
  };

  std::thread renderThread([&] {
    Clock clock;

    while (!windowShouldClose) {
      auto dt = clock.Tick();
      // LOG("dt: {}", dt);

      auto fps = clock.GetFps();
      auto fpsStr = std::format("fps: {:.2f}", fps);
      std::cout << '\r' << fpsStr << std::string(10, ' ') << std::flush;

      // nvim events -------------------------------------------
      if (!nvim.client.IsConnected()) {
        windowShouldClose = true;
        glfwPostEmptyEvent();
      };

      nvim.ParseEvents();
      // if (nvim.redrawState.numFlushes == 0) continue;

      // process events ---------------------------------------
      {
        std::scoped_lock lock(wgpuDeviceMutex);
        ProcessRedrawEvents(nvim.redrawState, editorState);
      }

      // update ----------------------------------------------
      // for (auto& [id, grid] : editorState.gridManager.grids) {
      //   editorState.cursor.SetDestPos(
      //     glm::vec2(grid.cursorCol, grid.cursorRow) * font.charSize
      //   );
      // }
      // editorState.cursor.Update(dt);

      // render ----------------------------------------------
      if (auto hlIter = editorState.hlTable.find(0);
          hlIter != editorState.hlTable.end()) {
        auto color = hlIter->second.background.value();
        renderer.clearColor = {color.r, color.g, color.b, color.a};
      }

      {
        std::scoped_lock lock(wgpuDeviceMutex);
        renderer.Begin();

        for (auto& [id, grid] : editorState.gridManager.grids) {
          if (id == 4) continue;
          if (grid.dirty) {
            renderer.RenderGrid(grid, font, editorState.hlTable);
            grid.dirty = false;
          }
        }

        std::vector<const Win*> windows;
        auto it = editorState.winManager.windows.find(1);
        if (it != editorState.winManager.windows.end()) {
          windows.push_back(&it->second);
        }
        for (auto& [id, win] : editorState.winManager.windows) {
          if (id == 4 || id == 1) continue;
          if (!win.hidden) windows.push_back(&win);
        }
        renderer.RenderWindows(windows);

        // if (editorState.cursor.blinkState != BlinkState::Off) {
        //   renderer.RenderCursor(editorState.cursor, editorState.hlTable);
        // }
        renderer.End();
        renderer.Present();

        ctx.device.Tick();
      }
    }
  });

  while (!windowShouldClose) {
    window.WaitEvents();
    std::this_thread::sleep_for(1ms);
  }

  renderThread.join();
  nvim.client.Disconnect();
}
