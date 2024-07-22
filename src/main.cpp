#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "app/size.hpp"
#include "app/input.hpp"
#include "app/sdl_window.hpp"
#include "app/sdl_event.hpp"
#include "app/options.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/state.hpp"
#include "editor/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/render_texture.hpp"
#include "gfx/renderer.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtx/string_cast.hpp"
#include "nvim/nvim.hpp"
#include "session/manager.hpp"
#include "utils/clock.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/unicode.hpp"
#include "webgpu_tools/utils/webgpu.hpp"

#include <boost/core/demangle.hpp>
#include <typeinfo>
#include <algorithm>
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

int main() {
  if (SDL_Init(SDL_INIT_VIDEO)) {
    LOG_ERR("Unable to initialize SDL: {}", SDL_GetError());
    return 1;
  }

  if (FtInit()) {
    LOG_ERR("Unable to initialize freetype");
    return 1;
  }

  try {
    SessionManager sessionManager(SpawnMode::Child);
    // SessionManager sessionManager(SpawnMode::Detached);

    uint16_t port = 2040;
    port = sessionManager.GetOrCreateSession("default");
    Nvim nvim("localhost", port);

    Options options;
    options.Load(nvim);

    // sdl::Window window({1600, 1000}, "Neovim GUI", options.window);
    sdl::Window window({1200, 800}, "Neovim GUI", options.window);

    // create font
    std::string guifont = nvim.GetOptionValue("guifont", {})->convert();
    auto fontFamilyResult = FontFamily::FromGuifont(guifont, window.dpiScale);
    if (!fontFamilyResult) {
      LOG_ERR("Failed to create font family: {}", fontFamilyResult.error());
      return 1;
    }
    FontFamily fontFamily = std::move(*fontFamilyResult);

    SizeHandler sizes{};
    sizes.UpdateSizes(
      window.size, window.dpiScale, fontFamily.DefaultFont().charSize, options.margins
    );

    Renderer renderer(sizes);

    EditorState editorState{
      .winManager{.sizes = sizes},
    };
    editorState.winManager.gridManager = &editorState.gridManager;
    editorState.cursor.Init(sizes.charSize, sizes.dpiScale);

    if (options.transparency < 1) {
      auto& hl = editorState.hlTable[0];
      hl.background = IntToColor(options.bgColor);
      hl.background->a = options.transparency;
    }

    nvim.UiAttach(
      sizes.uiWidth, sizes.uiHeight,
      {
        {"rgb", true},
        {"ext_multigrid", options.multigrid},
        {"ext_linegrid", true},
      }
    );

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
      Timer timer(10);

      while (!exitWindow) {
        auto dt = clock.Tick(
          options.maxFps == 0 ? std::nullopt : std::optional(options.maxFps)
        );
        // LOG("dt: {}", dt);

        // auto fps = clock.GetFps();
        // auto fpsStr = std::format("fps: {:.2f}", fps);
        // std::cout << '\r' << fpsStr << std::string(10, ' ') << std::flush;

        // timer.Start();

        // nvim events -------------------------------------------
        if (!nvim.IsConnected()) {
          exitWindow = true;
          sessionManager.RemoveSession("default");

          SDL_Event quitEvent{.type = SDL_EVENT_QUIT};
          SDL_PushEvent(&quitEvent);
        };

        LOG_DISABLE();
        ParseEvents(nvim.client, nvim.uiEvents);
        LOG_ENABLE();

        // process events ---------------------------------------
        {
          std::scoped_lock lock(wgpuDeviceMutex);
          LOG_DISABLE();
          if (ParseEditorState(nvim.uiEvents, editorState)) {
            idle = false;
            idleElasped = 0;
          }
          LOG_ENABLE();
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
                // LOG(
                //   "pixel size changed: {} {}", event.window.data1, event.window.data2
                // );
                window.fbSize = {event.window.data1, event.window.data2};

                std::scoped_lock lock(wgpuDeviceMutex);
                sizes.UpdateSizes(
                  window.size, window.dpiScale, fontFamily.DefaultFont().charSize,
                  options.margins
                );

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

        auto* activeWin = editorState.winManager.GetWin(editorState.cursor.grid);
        if (activeWin) {
          auto cursorPos =
            glm::vec2{
              activeWin->startCol + editorState.cursor.col,
              activeWin->startRow + editorState.cursor.row,
            } *
              sizes.charSize +
            sizes.offset;

          auto& winTex = activeWin->sRenderTexture;
          auto scrollOffset = //
            winTex.scrolling  //
              ? glm::vec2(0, (winTex.scrollDist - winTex.scrollCurr))
              : glm::vec2(0);

          editorState.cursor.SetDestPos(cursorPos + scrollOffset);
        }
        editorState.cursor.Update(dt);

        // render ----------------------------------------------
        if (auto hlIter = editorState.hlTable.find(0);
            hlIter != editorState.hlTable.end()) {
          auto color = hlIter->second.background.value();
          renderer.SetClearColor(color);
        }

        if (idle) continue;
        idleElasped += dt;
        if (idleElasped >= options.cursorIdleTime) {
          idle = true;
          editorState.cursor.blinkState = BlinkState::On;
        }

        if (!windowFocused) {
          editorState.cursor.blinkState = BlinkState::On;
        }

        {
          std::scoped_lock lock(wgpuDeviceMutex);
          renderer.Begin();

          bool renderWindows = false;
          for (auto& [id, win] : editorState.winManager.windows) {
            if (win.grid.dirty) {
              renderer.RenderToWindow(win, fontFamily, editorState.hlTable);
              win.grid.dirty = false;
              renderWindows = true;
            }
          }

          if (editorState.cursor.dirty && activeWin != nullptr) {
            renderer.RenderCursorMask(
              editorState.cursor, *activeWin, fontFamily, editorState.hlTable
            );
            editorState.cursor.dirty = false;
          }

          if (renderWindows || editorState.winManager.dirty) {
            editorState.winManager.dirty = false;

            std::vector<const Win*> windows;
            if (auto* msgWin = editorState.winManager.GetMsgWin()) {
              windows.push_back(msgWin);
            }
            std::vector<const Win*> floatWindows;
            for (auto& [id, win] : editorState.winManager.windows) {
              if (id == 1 || id == editorState.winManager.msgWinId || win.hidden) {
                continue;
              }
              if (win.IsFloating()) {
                floatWindows.push_back(&win);
              } else {
                windows.push_back(&win);
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
    InputHandler input(
      nvim, editorState.winManager, options.macOptIsMeta, options.multigrid,
      options.scrollSpeed
    );

    SDL_StartTextInput(window.Get());
    SDL_Rect rect{0, 0, 100, 100};
    SDL_SetTextInputArea(window.Get(), &rect, 0);

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
          float prevDpiScale = window.dpiScale;
          window.dpiScale = SDL_GetWindowPixelDensity(window.Get());
          if (prevDpiScale == window.dpiScale) break;
          // LOG("display scale changed: {}", window.dpiScale);
          fontFamily.ChangeDpiScale(window.dpiScale);
          editorState.cursor.Init(sizes.charSize, sizes.dpiScale);
          break;
        }

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
          sdlEvents.Push(event);
          break;
      }
    }

    renderThread.join();
    if (nvim.IsConnected()) {
      // send escape so nvim doesn't get stuck when reattaching
      // prevents cmd + q exiting window getting stuck
      nvim.Input("<Esc>");
      nvim.UiDetach();
      // LOG_INFO("Detached UI");
    }

  } catch (const std::exception& e) {
    LOG_ERR(
      "Exception of type {} caught: {}", boost::core::demangled_name(typeid(e)),
      e.what()
    );
    LOG_ERR("Exiting...");
  }
  // destructors cleans up window and font before quitting sdl and freetype

  FtDone();
  SDL_Quit();
}
