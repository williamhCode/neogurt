#include "app/options.hpp"
#include "app/window.hpp"
#include "editor/grid.hpp"
#include "editor/state.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "app/input.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/nvim.hpp"
#include "utils/clock.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"

#include "glm/gtx/string_cast.hpp"

#include <iostream>
#include <atomic>
#include <algorithm>

using namespace wgpu;
using namespace std::chrono_literals;
using namespace std::chrono;

const WGPUContext& ctx = Window::_ctx;
AppOptions options;

int main() {
  Window window({1400, 800}, "Neovim GUI", PresentMode::Immediate);
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
  editorState.cursor.size = {font.charWidth, font.charHeight};

  std::mutex resizeMutex;

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
    std::scoped_lock lock(resizeMutex);

    glm::uvec2 fbSize(width, height);
    window._ctx.Resize(fbSize);
    renderer.FbResize(fbSize);

    glm::uvec2 size(fbSize / 2u);
    int widthChars = size.x / font.charWidth;
    int heightChars = size.y / font.charHeight;
    nvim.UiTryResize(widthChars, heightChars);
    renderer.Resize(size);
  };

  // main loop -------------------------------------
  std::atomic_bool running = true;

  std::thread renderThread([&] {
    Clock clock;

    while (!window.ShouldClose()) {
      auto dt = clock.Tick(60);
      // LOG("dt: {}", dt);

      auto fps = clock.GetFps();
      // std::cout << std::fixed << std::setprecision(2);
      // std::cout << '\r' << "fps: " << fps << std::flush;

      {
        std::scoped_lock lock(resizeMutex);
        ctx.device.Tick();
      }

      // nvim events -------------------------------------------
      if (!nvim.client.IsConnected()) window.SetShouldClose(true);

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
        std::scoped_lock lock(resizeMutex);
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

    running = false;
    glfwPostEmptyEvent();
  });

  while (running) {
    window.WaitEvents();
    std::this_thread::sleep_for(1ms);
  }
  renderThread.join();

  nvim.client.Disconnect();
}
