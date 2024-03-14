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

#include <chrono>
#include <format>
#include <iostream>
#include <atomic>

using namespace wgpu;
using namespace std::chrono_literals;
using namespace std::chrono;

const WGPUContext& ctx = Window::_ctx;
AppOptions options;

int main() {
  // Window window({1400, 800}, "Neovim GUI", PresentMode::Immediate);
  Window window({1600, 1000}, "Neovim GUI", PresentMode::Immediate);
  Renderer renderer(window.size, window.fbSize);
  Font font("/Library/Fonts/SF-Mono-Medium.otf", 15, window.dpiScale);
  // Font font("/Users/williamhou/Library/Fonts/Hack Regular Nerd Font Complete
  // Mono.ttf", 15, 2); Font font(ROOT_DIR
  // "/res/NerdFontsSymbolsOnly/SymbolsNerdFontMono-Regular.ttf", 15, 2);

  int gridWidth = window.size.x / font.charWidth;
  int gridHeight = window.size.y / font.charHeight;

  Nvim nvim(false);
  nvim.UiAttach(gridWidth, gridHeight);

  EditorState editorState{};
  editorState.cursor.fullSize = {font.charWidth, font.charHeight};

  // lock whenever ctx.device is used
  std::mutex wgpuDeviceMutex;

  // input -----------------------------------------------------------
  InputHandler input(nvim, window.GetCursorPos(), {font.charWidth, font.charHeight});

  window.keyCallback = [&](int key, int scancode, int action, int mods) {
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

  // resizing -------------------------------------
  window.framebufferSizeCallback = [&](int width, int height) {
    std::scoped_lock lock(wgpuDeviceMutex);
    window._ctx.Resize(window.fbSize);
    renderer.FbResize(window.fbSize);

    glm::vec2 size(window.fbSize / (unsigned int)window.dpiScale);
    int widthChars = size.x / font.charWidth;
    int heightChars = size.y / font.charHeight;
    nvim.UiTryResize(widthChars, heightChars);
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
      auto dt = clock.Tick(60);
      // LOG("dt: {}", dt);

      auto fps = clock.GetFps();
      auto fpsStr = std::format("fps: {:.2f}", fps);
      // std::cout << '\r' << fpsStr << std::string(10, ' ') << std::flush;

      {
        std::scoped_lock lock(wgpuDeviceMutex);
        ctx.device.Tick();
      }

      // nvim events -------------------------------------------
      if (!nvim.client.IsConnected()) {
        windowShouldClose = true;
        glfwPostEmptyEvent();
      };

      nvim.ParseEvents();

      // process events ---------------------------------------
      // if (nvim.redrawState.numFlushes == 0) continue;
      ProcessRedrawEvents(nvim.redrawState, editorState);

      // update ----------------------------------------------
      for (auto& [id, grid] : editorState.gridManager.grids) {
        editorState.cursor.SetDestPos(
          glm::vec2(grid.cursorCol * font.charWidth, grid.cursorRow * font.charHeight)
        );
      }
      editorState.cursor.Update(dt);

      // render ----------------------------------------------
      if (auto hlIter = editorState.hlTable.find(0);
          hlIter != editorState.hlTable.end()) {
        auto color = hlIter->second.background.value();
        renderer.clearColor = {color.r, color.g, color.b, color.a};
      }

      {
        std::scoped_lock lock(wgpuDeviceMutex);
        renderer.Begin();
        // LOG("----------------------");
        for (auto& [id, grid] : editorState.gridManager.grids) {
          // LOG("render grid: {}", id);
          renderer.RenderGrid(grid, font, editorState.hlTable);
          if (editorState.cursor.blinkState != BlinkState::Off) {
            renderer.RenderCursor(editorState.cursor, editorState.hlTable);
          }
        }
        renderer.End();
        renderer.Present();
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
