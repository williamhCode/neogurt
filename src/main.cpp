#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_stdinc.h"
#include "app/path.hpp"
#include "app/size.hpp"
#include "app/input.hpp"
#include "app/ime.hpp"
#include "app/sdl_window.hpp"
#include "app/sdl_event.hpp"
#include "app/options.hpp"
#include "app/task_helper.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/state.hpp"
#include "event/manager.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtx/string_cast.hpp"
#include "event/ui_parse.hpp"
#include "event/ui_process.hpp"
#include "session/manager.hpp"
#include "session/options.hpp"
#include "utils/clock.hpp"
#include "utils/logger.hpp"
#include "utils/thread.hpp"
#include "utils/timer.hpp"

#include <boost/core/typeinfo.hpp>
#include <algorithm>
#include <memory>
#include <print>
#include <span>
#include <thread>
#include <vector>
#include <atomic>
#include <ranges>
#include <queue>
#include <chrono>

using namespace wgpu;
using namespace std::chrono;

WGPUContext ctx;

int main(int argc, char** argv) {
  // std::locale::global(std::locale("en_US.UTF-8"));

  auto optionsResult = StartupOptions::LoadFromCommandLine(argc, argv);
  if (!optionsResult) return optionsResult.error();
  StartupOptions startupOpts = *optionsResult;

  SetupPaths();

  // SDL ------------------------------------------------
  SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
  SDL_SetHint(SDL_HINT_MAC_SCROLL_MOMENTUM, "1");
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOG_ERR("Unable to initialize SDL: {}", SDL_GetError());
    return 1;
  }

  // Freetype -------------------------------------------
  if (int error = FtInit()) {
    LOG_ERR("Unable to initialize freetype: {}", FT_Error_String(error));
    return 1;
  }

  try {
    // init variables ---------------------
    GlobalOptions globalOpts;
    sdl::Window window({1200, 800}, "Neogurt", globalOpts);
    Renderer renderer;
    SizeHandler sizes{};
    SessionManager sessionManager(
      SpawnMode::Child, startupOpts, globalOpts, window, sizes, renderer
    );
    EventManager eventManager(sessionManager);

    sessionManager.SessionNew();
    SessionHandle session = sessionManager.CurrSession();
    SessionOptions* options = &session->sessionOpts;
    Nvim* nvim = &session->nvim;
    EditorState* editorState = &session->editorState;

    // main loop -----------------------------------
    std::atomic_bool exitWindow = false;
    struct ResizeEvents {
      std::optional<SDL_Event> windowResized;
      SDL_Event windowPixelSizeChanged;
    };
    Sync<std::queue<ResizeEvents>> resizeEvents;
    Sync<std::queue<SDL_Event>> sdlEvents;

    int frameCount = 0;

    std::jthread renderThread([&](std::stop_token stopToken) {
      bool windowFocused = true;
      bool windowOccluded = false;

      bool idle = false;
      float idleElasped = 0;
      auto IdleReset = [&] {
        idle = false;
        idleElasped = 0;
      };

      Clock clock;
      StableClock stableClock;
      // Timer timer1(20);

      // main loop
      while (!exitWindow && !stopToken.stop_requested()) {

        // session handling ------------------------------------------
        if (auto* sessionPtr = sessionManager.GetCurrentSession()) {
          if (session != *sessionPtr) {
            session = *sessionPtr;
            options = &session->sessionOpts;
            nvim = &session->nvim;
            editorState = &session->editorState;
          }
        } else {
          // quit if no sessions left
          SDL_Event quitEvent;
          SDL_zero(quitEvent);
          quitEvent.type = SDL_EVENT_QUIT;
          SDL_PushEvent(&quitEvent);
          break;
        }

        // resize handling -------------------------------------------
        while (!resizeEvents.lock()->empty()) {
          if (resizeEvents.lock()->size() == 1) { // only process last event
            const auto& resizeEvent = resizeEvents.lock()->front();

            if (const auto& event = resizeEvent.windowResized) {
              window.size = {event->window.data1, event->window.data2};
            }

            const auto& event = resizeEvent.windowPixelSizeChanged;
            window.fbSize = {event.window.data1, event.window.data2};
            window.dpiScale = (float)window.fbSize.x / window.size.x;

            sessionManager.UpdateSessionSizes(session);
            ctx.Resize(sizes.fbSize, globalOpts.vsync);
          }
          resizeEvents.lock()->pop();
        }

        // sdl events -----------------------------------------
        while (!sdlEvents.lock()->empty()) {
          auto& event = sdlEvents.lock()->front();

          switch (event.type) {
            case SDL_EVENT_TEXT_EDITING:
              IdleReset();
              session->ime.HandleTextEditing(event.edit);
              break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
              windowFocused = true;
              IdleReset();
              editorState->cursor.SetBlinkState(BlinkState::Wait);
              break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
              windowFocused = false;
              session->ime.Clear();
              break;

            case SDL_EVENT_WINDOW_EXPOSED:
              // LOG_INFO("window exposed");
              windowOccluded = false;
              IdleReset();
              break;
            case SDL_EVENT_WINDOW_OCCLUDED:
              // LOG_INFO("window occluded");
              windowOccluded = true;
              break;
          }
          sdlEvents.lock()->pop();
        }

        // user events -------------------------------------------
        // process nvim events for all sessions except current
        // do this before getting next texture to minimize cpu time
        LOG_DISABLE();
        for (auto& [id, _session] : sessionManager.Sessions()) {
          if (_session != session) {
            eventManager.ProcessSessionEvents(_session);
            ProcessUiEvents(_session);
          }
        }
        LOG_ENABLE();

        // timing (run before nvim ui-events to minimize latency) --------
        renderer.GetNextTexture(); // blocks until next frame if vsync is on

        // SDL_DisplayID displayID = SDL_GetDisplayForWindow(window.Get());
        // const SDL_DisplayMode* currentMode = SDL_GetCurrentDisplayMode(displayID);
        // float displayFps = currentMode->refresh_rate == 0 ? 60 : currentMode->refresh_rate;

        float targetFps = globalOpts.vsync ? 0 : globalOpts.fps;
        if (idle) {
          targetFps = windowOccluded ? 10 : 60;
        }
        float dt = clock.Tick(targetFps);

        // frameCount++;
        // if (frameCount % 60 == 0) {
        //   frameCount = 0;
        //   auto fps = clock.GetFps();
        //   auto fpsStr = std::format("fps: {:.2f}", fps);
        //   std::print("\rfps: {:.2f}", fps);
        // }

        // timer1.Start();

        // nvim events  -------------------------------------------
        LOG_DISABLE();
        eventManager.ProcessSessionEvents(session);
        if (session->uiEvents.numFlushes > 0) {
          IdleReset();
        }

        ProcessUiEvents(session);
        LOG_ENABLE();

        eventManager.ExecuteTasks();

        // update --------------------------------------------
        sessionManager.UpdateUiOptions(session);

        // window resources
        editorState->winManager.UpdateRenderData();
        // stable clock for animations
        auto steps = stableClock.Tick();
        editorState->winManager.UpdateScrolling(steps);

        // mouse
        const auto* currWin = editorState->winManager.GetWin(editorState->cursor.grid);
        if (editorState->cursor.SetDestPos(currWin, sizes)) {
          const auto& cursorPos = editorState->cursor.destPos;
          SDL_Rect rect(cursorPos.x, cursorPos.y, sizes.charSize.x, sizes.charSize.y);
          SDL_SetTextInputArea(window.Get(), &rect, 0);
          // LOG_INFO("cursorPos: {}", glm::to_string(cursorPos));
        }
        editorState->cursor.Update(dt);

        // check idle -----------------------------------
        if (idle) continue;
        idleElasped += dt;
        if (idleElasped >= globalOpts.cursorIdleTime || windowOccluded) {
          idle = true;
          editorState->cursor.SetBlinkState(BlinkState::On);
        }

        if (!windowFocused) {
          editorState->cursor.SetBlinkState(BlinkState::On);
        }

        // render ----------------------------------------------
        renderer.Begin();

        auto color = editorState->hlManager.GetDefaultBackground();
        renderer.SetColors(color, globalOpts.gamma);

        bool renderWindows = true;
        for (auto& [id, win] : editorState->winManager.windows) {
          if (win.grid.dirty && !win.hidden) {
            renderer.RenderToWindow(win, editorState->fontFamily, editorState->hlManager);
            win.grid.dirty = false;
            renderWindows = true;
          }
        }

        if (currWin != nullptr && editorState->cursor.dirty) {
          renderer.RenderCursorMask(
            *currWin, editorState->cursor, editorState->fontFamily, editorState->hlManager
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
          renderer.RenderCursor(editorState->cursor, editorState->hlManager);
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
    SDL_StartTextInput(window.Get());

    // resize handling
    ResizeEvents currResizeEvents{};

    sdl::SetEventFilter([&](SDL_Event& event) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
          currResizeEvents.windowResized = event;
          break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
          currResizeEvents.windowPixelSizeChanged = event;
          resizeEvents.lock()->push(currResizeEvents);
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
    SessionHandle currSession;

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
        case SDL_EVENT_KEY_UP: {
          // we dont want ime moving around, so disable keyboard
          if (currSession && !currSession->ime.active.load()) {
            currSession->input.HandleKeyboard(event.key);
          }
          break;
        }

        case SDL_EVENT_TEXT_EDITING:
          if (currSession) {
            currSession->input.HandleTextEditing(event.edit);
            // copy event.edit.text
            event.edit.text = SDL_strdup(event.edit.text);
            sdlEvents.lock()->push(event);
          }
          break;
        case SDL_EVENT_TEXT_INPUT:
          if (currSession) currSession->input.HandleTextInput(event.text);
          break;

        // mouse handling ------------------------
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
          if (currSession) currSession->input.HandleMouseButton(event.button);
          break;
        case SDL_EVENT_MOUSE_MOTION:
          SDL_ShowCursor();
          if (currSession) currSession->input.HandleMouseMotion(event.motion);
          break;
        case SDL_EVENT_MOUSE_WHEEL:
          if (currSession) currSession->input.HandleMouseWheel(event.wheel);
          break;

        // window handling -----------------------
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_OCCLUDED:
          sdlEvents.lock()->push(event);
          break;
      }

      if (event.type == EVENT_DEFERRED_TASK) {
        ProcessNextMainThreadTask();
      } else if (event.type == EVENT_SWITCH_SESSION) {
        currSession = PopSessionFromMainThread();
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
