#include "SDL3/SDL_init.h"
#include "app/path.hpp"
#include "app/size.hpp"
#include "app/input.hpp"
#include "app/sdl_window.hpp"
#include "app/sdl_event.hpp"
#include "app/options.hpp"
#include "app/task_helper.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/state.hpp"
#include "editor/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtx/string_cast.hpp"
#include "nvim/events/ui.hpp"
#include "nvim/events/user.hpp"
#include "session/manager.hpp"
#include "utils/clock.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "session/state.hpp"

#include <boost/core/demangle.hpp>
#include <algorithm>
#include <future>
#include <print>
#include <span>
#include <vector>
#include <atomic>
#include <ranges>
#include <chrono>

using namespace wgpu;
using namespace std::chrono;

WGPUContext ctx;

int main(int argc, char** argv) {
  if (auto exit = Options::LoadFromCommandLine(argc, argv)) {
    return *exit;
  }

  SetupPaths();

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOG_ERR("Unable to initialize SDL: {}", SDL_GetError());
    return 1;
  }

  if (int error = FtInit()) {
    LOG_ERR("Unable to initialize freetype: {}", FT_Error_String(error));
    return 1;
  }

  try {
    // init variables ---------------------
    sdl::Window window;
    SizeHandler sizes;
    Renderer renderer;
    InputHandler input;

    SessionManager sessionManager(SpawnMode::Child, window, sizes, renderer, input);
    sessionManager.SessionNew();
    SessionState* session = sessionManager.CurrSession();
    Options* options = &session->options;
    Nvim* nvim = &session->nvim;
    EditorState* editorState = &session->editorState;

    nvim->ExecLua("vim.g.neogurt_startup()", {});

    // main loop -----------------------------------
    std::atomic_bool exitWindow = false;
    struct ResizeEvents {
      std::optional<SDL_Event> windowResized;
      SDL_Event windowPixelSizeChanged;
    };
    TSQueue<ResizeEvents> resizeEvents;
    TSQueue<SDL_Event> sdlEvents;

    int frameCount = 0;

    std::jthread renderThread([&](std::stop_token stopToken) {
      bool windowFocused = true;
      bool windowOccluded = false;
      bool idle = false;
      float idleElasped = 0;

      Clock clock;
      // Timer timer1(1);

      // main loop
      while (!exitWindow && !stopToken.stop_requested()) {

        // resize events, put before getting next texture
        while (!resizeEvents.Empty()) {

          if (resizeEvents.Size() == 1) { // only process last event
            const auto& resizeEvent = resizeEvents.Front();

            if (const auto& event = resizeEvent.windowResized) {
              window.size = {event->window.data1, event->window.data2};
            }

            {
              const auto& event = resizeEvent.windowPixelSizeChanged;
              window.fbSize = {event.window.data1, event.window.data2};
              window.dpiScale = window.fbSize.x / window.size.x;

              bool dpiChanged = editorState->fontFamily.TryChangeDpiScale(window.dpiScale);

              auto uiFbSize = sizes.uiFbSize;
              sizes.UpdateSizes(
                window.size, window.dpiScale,
                editorState->fontFamily.DefaultFont().charSize, *options
              );
              editorState->winManager.sizes = sizes;

              if (dpiChanged) {
                editorState->cursor.Resize(sizes.charSize, sizes.dpiScale);
              }

              ctx.Resize(sizes.fbSize);

              if (uiFbSize == sizes.uiFbSize) {
                renderer.camera.Resize(sizes.size);

              } else {
                renderer.Resize(sizes);
                nvim->UiTryResize(sizes.uiWidth, sizes.uiHeight);
              }

              break;
            }
          }
          resizeEvents.Pop();
        }

        // this blocks if vsync is enabled 
        renderer.GetNextTexture();

        // timing -------------------------------------------
        // if in vsync, disable clock infinite fps (120 for now cuz occlusion events are
        // not sent immediately when switchint desktop)
        // TODO: change when bug is fixed

        // however set to maxFps option if any of these happen
        // 1. idle is true (which bypasses surface.Present() calls)
        // 2. occluded is true (in which macOS stops presenting at all)
        // these all prevent vsync from working

        float targetFps =
          window.vsync && !idle && !windowOccluded ? 120 : options->maxFps;
        float dt = clock.Tick(targetFps);

        // frameCount++;
        // if (frameCount % 60 == 0) {
        //   frameCount = 0;
        //   auto fps = clock.GetFps();
        //   auto fpsStr = std::format("fps: {:.2f}", fps);
        //   std::print("\rfps: {:.2f}", fps);
        // }

        // timer1.Start();

        // sdl events -----------------------------------------
        while (!sdlEvents.Empty()) {
          auto& event = sdlEvents.Front();
          switch (event.type) {
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
              windowFocused = true;
              idle = false;
              idleElasped = 0;
              editorState->cursor.blinkState = BlinkState::Wait;
              editorState->cursor.blinkElasped = 0;
              break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
              windowFocused = false;
              break;

            case SDL_EVENT_WINDOW_EXPOSED:
              // LOG_INFO("window exposed");
              windowOccluded = false;
              break;
            case SDL_EVENT_WINDOW_OCCLUDED:
              // LOG_INFO("window occluded");
              windowOccluded = true;
              break;
          }
          sdlEvents.Pop();
        }

        // nvim events  -------------------------------------------
        if (sessionManager.ShouldQuit()) {
          SDL_Event quitEvent;
          SDL_zero(quitEvent);
          quitEvent.type = SDL_EVENT_QUIT;
          SDL_PushEvent(&quitEvent);
          break;
        };
        session = sessionManager.CurrSession();
        options = &session->options;
        nvim = &session->nvim;
        editorState = &session->editorState;

        LOG_DISABLE();
        ProcessUserEvents(*nvim->client, sessionManager);
        LOG_ENABLE();

        LOG_DISABLE();
        // process_events:
        int numFlushes = ParseUiEvents(*nvim->client, nvim->uiEvents);
        if (numFlushes) {
          idle = false;
          idleElasped = 0;
        }
        LOG_ENABLE();

        LOG_DISABLE();
        ParseEditorState(nvim->uiEvents, session->editorState);
        LOG_ENABLE();


        // timer1.End();

        // update --------------------------------------------
        editorState->winManager.UpdateRenderData();
        editorState->winManager.UpdateScrolling(dt);

        const auto* currWin = editorState->winManager.GetWin(editorState->cursor.grid);
        if (editorState->cursor.SetDestPos(currWin, sizes)) {
          const auto& cursorPos = editorState->cursor.destPos;
          SDL_Rect rect(cursorPos.x, cursorPos.y, sizes.charSize.x, sizes.charSize.y);
          SDL_SetTextInputArea(window.Get(), &rect, 0);
        }
        editorState->cursor.Update(dt);

        // check idle -----------------------------------
        if (idle) continue;
        idleElasped += dt;
        if (idleElasped >= options->cursorIdleTime) {
          idle = true;
          editorState->cursor.blinkState = BlinkState::On;
        }

        if (!windowFocused) {
          editorState->cursor.blinkState = BlinkState::On;
        }

        // render ----------------------------------------------
        renderer.Begin();

        auto color = GetDefaultBackground(editorState->hlTable);
        renderer.SetColors(color, options->gamma);

        bool renderWindows = true;
        for (auto& [id, win] : editorState->winManager.windows) {
          if (win.grid.dirty) {
            renderer.RenderToWindow(win, editorState->fontFamily, editorState->hlTable);
            win.grid.dirty = false;
            renderWindows = true;
          }
        }

        if (currWin != nullptr && editorState->cursor.dirty) {
          renderer.RenderCursorMask(
            *currWin, editorState->cursor, editorState->fontFamily, editorState->hlTable
          );
          editorState->cursor.dirty = false;
        }

        if (renderWindows || editorState->winManager.dirty || session->reattached) {
          editorState->winManager.dirty = false;

          std::vector<const Win*> windows;
          std::vector<const Win*> floatWindows;
          for (const auto* win : editorState->winManager.windowsOrder) {
            if (win->id == 1 || win->id == editorState->winManager.msgWinId || win->hidden) {
              continue;
            }
            if (win->IsFloating()) {
              floatWindows.push_back(win);
            } else {
              windows.push_back(win);
            }
          }
          if (auto winIt = editorState->winManager.windows.find(1);
              winIt != editorState->winManager.windows.end()) {
            windows.push_back(&winIt->second);
          }

          // sort floating windows by zindex
          std::ranges::stable_sort(floatWindows, [](const Win* win, const Win* other) {
            return win->floatData->zindex > other->floatData->zindex;
          });

          renderer.RenderWindows(
            editorState->winManager.GetMsgWin(), windows, floatWindows
          );
          // reset reattached flag after rendering
          session->reattached = false;
        }

        renderer.RenderFinalTexture();

        if (editorState->cursor.ShouldRender()) {
          renderer.RenderCursor(editorState->cursor, editorState->hlTable);
        }

        renderer.End();

        ctx.surface.Present();
        ctx.device.Tick();

        // auto t1 = TimeToMs(timer1.GetAverageDuration());
        // if (t1 > 1ms) {
        //   LOG_INFO("render: {}", t1);
        // }
      }
    });

    // event loop --------------------------------
    // SDL_StartTextInput(window.Get());

    // resize handling
    ResizeEvents currResizeEvents{};

    sdl::SetEventFilter([&](SDL_Event& event) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
          currResizeEvents.windowResized = event;
          break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
          currResizeEvents.windowPixelSizeChanged = event;
          resizeEvents.Push(currResizeEvents);
          currResizeEvents = {};
          break;
        }

        case SDL_EVENT_QUIT: {
          const bool* state = SDL_GetKeyboardState(nullptr);
          SDL_Keymod mods = SDL_GetModState();

          auto scancode = SDL_GetScancodeFromKey(SDLK_W, nullptr);
          if (state[scancode] && (mods & SDL_KMOD_GUI)) {
            return false;
          }

          break;
        }
      }
      return true;
    });

    SDL_Event event;
    while (!exitWindow) {
      auto success = SDL_WaitEvent(&event);
      if (!success) {
        LOG_ERR("SDL_WaitEvent error: {}", SDL_GetError());
      }

      switch (event.type) {
        case SDL_EVENT_QUIT:
          LOG_INFO("quitting...");
          exitWindow = true;
          break;

        // keyboard handling ----------------------
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
          input.HandleKeyboard(event.key);
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
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_OCCLUDED:
          sdlEvents.Push(event);
          break;
      }

      if (event.type == EVENT_DEFERRED_TASK) {
        ProcessNextMainThreadTask();
      }
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
