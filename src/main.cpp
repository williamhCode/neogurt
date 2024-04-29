#include "SDL3/SDL_init.h"
#include "SDL_keycode.h"
#include "app/options.hpp"
#include "app/size.hpp"
#include "app/input.hpp"
#include "app/sdl_window.hpp"
#include "app/sdl_event.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/state.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtx/string_cast.hpp"
#include "nvim/nvim.hpp"
#include "utils/clock.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"

#include <algorithm>
#include <span>
#include <vector>
#include <deque>
#include <atomic>
#include <iostream>
#include <format>
#include <chrono>

using namespace wgpu;
using namespace std::chrono_literals;
using namespace std::chrono;

const WGPUContext& ctx = sdl::Window::_ctx;
AppOptions appOpts;

int main() {
  if (SDL_Init(SDL_INIT_VIDEO)) {
    LOG_ERR("Unable to initialize SDL: {}", SDL_GetError());
    return 1;
  }

  try {
    FtInit();
  } catch (const std::exception& e) {
    LOG_ERR("{}", e.what());
    return 1;
  }

  try {
    appOpts = {
      .multigrid = true,
      .vsync = true,
      .windowMargins{5, 5, 5, 5},
      .borderless = false,
      .bgColor = 0x282c34,
      .transparency = 0.92,
      .windowBlur = 20,
    };
    appOpts.transparency = int(appOpts.transparency * 255) / 255.0f;

    auto presentMode = appOpts.vsync ? PresentMode::Mailbox : PresentMode::Immediate;
    sdl::Window window({1600, 1000}, "Neovim GUI", presentMode);

    Font font("/Library/Fonts/SF-Mono-Medium.otf", 15, window.dpiScale);

    SizeHandler sizes;
    sizes.UpdateSizes(window.size, window.dpiScale, font.charSize);

    Renderer renderer(sizes);

    Nvim nvim(false);
    nvim.UiAttach(
      sizes.uiWidth, sizes.uiHeight,
      {
        {"rgb", true},
        {"ext_multigrid", appOpts.multigrid},
        {"ext_linegrid", true},
      }
    );

    EditorState editorState{
      .winManager{.sizes = sizes},
      .cursor{.fullSize = sizes.charSize},
    };
    editorState.winManager.gridManager = &editorState.gridManager;

    // main loop -----------------------------------
    // lock whenever ctx.device is used
    std::mutex wgpuDeviceMutex;

    std::atomic_bool exitWindow = false;
    TSQueue<SDL_Event> resizeEvents;
    TSQueue<SDL_Event> sdlEvents;

    std::thread renderThread([&] {
      bool windowFocused = true;
      bool idle = false;
      float idleElasped = 0;

      Clock clock;
      // Timer timer(30);

      while (!exitWindow) {
        auto dt = clock.Tick(60);
        // LOG("dt: {}", dt);

        // auto fps = clock.GetFps();
        // auto fpsStr = std::format("fps: {:.2f}", fps);
        // std::cout << '\r' << fpsStr << std::string(10, ' ') << std::flush;

        // timer.Start();

        // nvim events -------------------------------------------
        if (!nvim.client.IsConnected()) {
          exitWindow = true;
          nvim.sessionManager.RemoveSession("default");
        };

        nvim.ParseRedrawEvents();

        // process events ---------------------------------------
        {
          std::scoped_lock lock(wgpuDeviceMutex);
          if (ProcessRedrawEvents(nvim.redrawState, editorState)) {
            idle = false;
            idleElasped = 0;
          }
        }

        // update ----------------------------------------------
        while (!resizeEvents.Empty()) {
          // only process the last 2 resize events
          if (resizeEvents.Size() <= 2) {
            auto& event = resizeEvents.Front();
            switch (event.type) {
              case SDL_EVENT_WINDOW_RESIZED:
                window.size = {event.window.data1, event.window.data2};
                break;
              case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                window.fbSize = {event.window.data1, event.window.data2};

                std::scoped_lock lock(wgpuDeviceMutex);
                sizes.UpdateSizes(window.size, window.dpiScale, font.charSize);

                sdl::Window::_ctx.Resize(sizes.fbSize);
                renderer.Resize(sizes);
                nvim.UiTryResize(sizes.uiWidth, sizes.uiHeight);
                break;
              }
            }
          }
          resizeEvents.Pop();
        }

        while (!sdlEvents.Empty()) {
          auto& event = sdlEvents.Front();
          switch (event.type) {
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
              windowFocused = true;
              idle = false;
              idleElasped = 0;
              editorState.cursor.blinkState = BlinkState::Wait;
              editorState.cursor.blinkElasped = 0;
              break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
              windowFocused = false;
              break;
            // case SDL_EVENT_KEY_DOWN:
            //   if (event.key.keysym.sym == SDLK_1) {
            //     LOG("key down: {}", event.key.keysym.sym);
            //     nvim.NvimListUis();
            //   }
            //   if (event.key.keysym.sym == SDLK_2) {
            //     LOG("key down: {}", event.key.keysym.sym);
            //     nvim.UiDetach();
            //   }
            //   break;
          }
          sdlEvents.Pop();
        }

        editorState.winManager.UpdateScrolling(dt);

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

          editorState.cursor.winScrollOffset =
            win->scrolling ? glm::vec2(0, win->scrollDist - win->scrollCurr)
                           : glm::vec2(0);

          currMaskBG = win->maskBG;
        }
        editorState.cursor.currMaskBG = std::move(currMaskBG);
        editorState.cursor.Update(dt);

        // render ----------------------------------------------
        if (auto hlIter = editorState.hlTable.find(0);
            hlIter != editorState.hlTable.end()) {
          auto color = hlIter->second.background.value();
          renderer.SetClearColor(color);
        }

        if (idle) continue;
        idleElasped += dt;
        if (idleElasped >= appOpts.cursorIdleTime || !windowFocused) {
          idle = true;
          editorState.cursor.blinkState = BlinkState::On;
        }
        {
          std::scoped_lock lock(wgpuDeviceMutex);
          renderer.Begin();

          bool renderWindows = false;
          // bool renderWindows = true;
          for (auto& [id, win] : editorState.winManager.windows) {
            if (win.grid.dirty) {
            // if (true) {
              renderer.RenderWindow(win, font, editorState.hlTable);
              win.grid.dirty = false;
              renderWindows = true;
            }
          }

          if (renderWindows || editorState.winManager.dirty) {
            editorState.winManager.dirty = false;

            std::deque<const Win*> windows;
            std::vector<const Win*> floatWindows;
            for (auto& [id, win] : editorState.winManager.windows) {
              if (id == 1) continue;
              if (id == editorState.winManager.msgWinId) {
                windows.push_front(&win);
              } else if (!win.hidden) {
                if (win.floatData.has_value()) {
                  floatWindows.push_back(&win);
                } else {
                  windows.push_back(&win);
                }
              }
            }
            if (auto winIt = editorState.winManager.windows.find(1);
                winIt != editorState.winManager.windows.end()) {
              windows.push_back(&winIt->second);
            }

            // see editor/window.hpp comment for WinManager::windows
            std::ranges::reverse(floatWindows);

            // sort floating windows by zindex
            std::ranges::sort(floatWindows, [](const Win* win, const Win* other) {
              return win->floatData->zindex < other->floatData->zindex;
            });

            renderer.RenderWindows(windows, floatWindows);
          }

          renderer.RenderFinalTexture();

          if (editorState.cursor.ShouldRender()) {
            renderer.RenderCursor(editorState.cursor, editorState.hlTable);
          }

          renderer.End();

          ctx.surface.Present();
          ctx.device.Tick();
        }

        // timer.End();
        // auto avgDuration = duration_cast<microseconds>(timer.GetAverageDuration());
        // std::cout << '\r' << avgDuration << std::string(10, ' ') << std::flush;
      }
    });

    // event loop --------------------------------
    InputHandler input(nvim, editorState.winManager);

    SDL_StartTextInput();
    // SDL_Rect rect{0, 0, 100, 100};
    // SDL_SetTextInputRect(&rect);

    // resize handling
    sdl::AddEventWatch([&](SDL_Event& event) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
          resizeEvents.Push(event);
          break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
          resizeEvents.Push(event);
          break;
        }
      }
    });

    SDL_Event event;
    while (!exitWindow) {
      auto success = SDL_WaitEvent(&event);
      if (!success) {
        LOG_ERR("SDL_WaitEvent error: {}", SDL_GetError());
      }

      switch (event.type) {
        case SDL_EVENT_QUIT:
          LOG("exit window");
          exitWindow = true;
          break;

        // keyboard handling ----------------------
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
          input.HandleKeyboard(event.key);
          sdlEvents.Push(event);
          break;

        case SDL_EVENT_TEXT_EDITING:
          break;
        case SDL_EVENT_TEXT_INPUT:
          input.HandleTextInput(event.text);
          break;

        // mouse handling ------------------------
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
          input.HandleMouseButton(event.button);
          break;
        case SDL_EVENT_MOUSE_MOTION:
          input.HandleMouseMotion(event.motion);
          break;
        case SDL_EVENT_MOUSE_WHEEL:
          input.HandleMouseWheel(event.wheel);
          break;

        // window handling -----------------------
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
          std::scoped_lock lock(wgpuDeviceMutex);
          window.dpiScale = SDL_GetWindowPixelDensity(window.Get());
          LOG("display scale changed: {}", window.dpiScale);
          font = Font("/Library/Fonts/SF-Mono-Medium.otf", 15, window.dpiScale);
          editorState.cursor.fullSize = font.charSize;
          break;
        }

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
          sdlEvents.Push(event);
          break;
      }
    }

    renderThread.join();
    if (nvim.client.IsConnected()) {
      nvim.UiDetach();
      LOG_INFO("Detached UI");
    }

  } catch (const std::exception& e) {
    LOG_ERR("{}", e.what());
  }
  // destructors cleans up window and font before quitting sdl and freetype

  FtDone();
  SDL_Quit();
}
