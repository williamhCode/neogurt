#include "GLFW/glfw3.h"
#include "app/options.hpp"
#include "app/size.hpp"
#include "app/window.hpp"
#include "app/input.hpp"
#include "editor/grid.hpp"
#include "editor/state.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "glm/ext/vector_float2.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/nvim.hpp"
#include "utils/clock.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/unicode.hpp"

#include <algorithm>
#include <deque>
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
  // ─	━	│	┃	┄	┅	┆	┇	┈	┉	┊ ┋
  // ┌	┍	┎	┏  LOG_INFO("{}", UTF8ToUnicode(""));

  auto presentMode = options.vsync ? PresentMode::Mailbox : PresentMode::Immediate;
  Window window({1600, 1000}, "Neovim GUI", presentMode);
  Font font("/Library/Fonts/SF-Mono-Medium.otf", 15, window.dpiScale);
  // font = Font("/Library/Fonts/SF-Mono-Medium.otf", 18, window.dpiScale);
  // Font font(ROOT_DIR "/res/Hack/HackNerdFont-Regular.ttf", 18, window.dpiScale);
  // Font font(ROOT_DIR "/res/JetBrainsMono/JetBrainsMonoNerdFont-Regular.ttf", 18,
  // window.dpiScale);

  SizeHandler sizes;
  sizes.UpdateSizes(window.size, window.dpiScale, font.charSize);

  Renderer renderer(sizes);

  Nvim nvim(false);
  nvim.UiAttach(
    sizes.uiWidth, sizes.uiHeight,
    {
      {"rgb", true},
      {"ext_multigrid", options.multigrid},
      {"ext_linegrid", true},
    }
  );

  EditorState editorState{
    .winManager{.sizes = sizes},
    .cursor{.fullSize = sizes.charSize},
  };
  editorState.winManager.gridManager = &editorState.gridManager;

  // lock whenever ctx.device is used
  std::mutex wgpuDeviceMutex;

  // input -----------------------------------------------------------
  InputHandler input(nvim, editorState.winManager, window.GetCursorPos());

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

  // resizing and dpi changed -------------------------------------
  window.framebufferSizeCallback = [&](int /* width */, int /* height */) {
    std::scoped_lock lock(wgpuDeviceMutex);
    glm::vec2 size(window.fbSize / (unsigned int)window.dpiScale);
    sizes.UpdateSizes(size, window.dpiScale, font.charSize);

    Window::_ctx.Resize(sizes.fbSize);
    renderer.Resize(sizes);

    nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);
  };

  window.windowContentScaleCallback = [&](float /* xscale */, float /* yscale */) {
    std::scoped_lock lock(wgpuDeviceMutex);
    font = Font("/Library/Fonts/SF-Mono-Medium.otf", 15, window.dpiScale);
    editorState.cursor.fullSize = font.charSize;
  };

  // main thread -----------------------------------
  std::atomic_bool windowShouldClose = false;

  window.windowCloseCallback = [&] {
    windowShouldClose = true;
  };

  std::thread renderThread([&] {
    Clock clock;
    Timer timer(30);

    while (!windowShouldClose) {
      auto dt = clock.Tick(60);
      // LOG("dt: {}", dt);

      // auto fps = clock.GetFps();
      // auto fpsStr = std::format("fps: {:.2f}", fps);
      // std::cout << '\r' << fpsStr << std::string(10, ' ') << std::flush;
      // std::cout << fpsStr << std::string(10, ' ') << '\n';

      // timer.Start();

      // nvim events -------------------------------------------
      if (!nvim.client.IsConnected()) {
        windowShouldClose = true;
        glfwPostEmptyEvent();
      };

      nvim.ParseRedrawEvents();

      // process events ---------------------------------------
      {
        std::scoped_lock lock(wgpuDeviceMutex);
        ProcessRedrawEvents(nvim.redrawState, editorState);
      }

      // update ----------------------------------------------
      wgpu::BindGroup currMaskBG;
      if (auto* win = editorState.winManager.GetActiveWin()) {
        auto cursorPos =
          glm::vec2{
            win->startCol + win->grid.cursorCol,
            win->startRow + win->grid.cursorRow,
          } *
            sizes.charSize +
          sizes.offset;
        editorState.cursor.SetDestPos(cursorPos);

        editorState.cursor.offset =
          win->scrolling
          ? glm::vec2(0, win->scrollDist - win->scrollCurr)
          : glm::vec2(0);

        currMaskBG = win->maskBG;
      }
      editorState.cursor.currMaskBG = std::move(currMaskBG);
      editorState.cursor.Update(dt);

      editorState.winManager.UpdateScrolling(dt);

      // render ----------------------------------------------
      if (auto hlIter = editorState.hlTable.find(0);
          hlIter != editorState.hlTable.end()) {
        renderer.SetClearColor(hlIter->second.background.value());
      }

      {
        std::scoped_lock lock(wgpuDeviceMutex);
        renderer.Begin();

        bool renderWindows = false;
        for (auto& [id, win] : editorState.winManager.windows) {
          if (win.grid.dirty) {
            renderer.RenderWindow(win, font, editorState.hlTable);
            win.grid.dirty = false;
            renderWindows = true;
          }
        }

        if (renderWindows || editorState.winManager.dirty) {
          editorState.winManager.dirty = false;

          std::deque<const Win*> windows;
          std::vector<const Win*> floatingWindows;
          for (auto& [id, win] : editorState.winManager.windows) {
            if (id == editorState.winManager.msgWinId) {
              windows.push_front(&win);
            } else if (!win.hidden) {
              if (win.floatData.has_value()) {
                floatingWindows.push_back(&win);
              } else {
                windows.push_back(&win);
              }
            }
          }
          if (auto winIt = editorState.winManager.windows.find(1);
              winIt != editorState.winManager.windows.end()) {
            windows.push_front(&winIt->second);
          }

          // see editor/window.hpp comment for WinManager::windows
          std::ranges::reverse(floatingWindows);

          // sort floating windows by zindex
          // std::ranges::sort(floatingWindows, [](const Win* win, const Win* other) {
          //   return win->floatData->zindex < other->floatData->zindex;
          // });

          windows.insert(windows.end(), floatingWindows.begin(), floatingWindows.end());
          renderer.RenderWindows(windows);
        }

        renderer.RenderFinalTexture();

        if (editorState.cursor.ShouldRender()) {
          renderer.RenderCursor(editorState.cursor, editorState.hlTable);
        }

        renderer.End();
        renderer.Present();

        ctx.device.Tick();
      }

      // timer.End();
      // auto avgDuration = duration_cast<microseconds>(timer.GetAverageDuration());
      // std::cout << '\r' << avgDuration << std::string(10, ' ') << std::flush;
    }
  });

  while (!windowShouldClose) {
    window.WaitEvents();
    std::this_thread::sleep_for(1ms);
  }

  renderThread.join();
  nvim.client.Disconnect();
}
