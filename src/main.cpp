#include "app/window.hpp"
#include "editor/grid.hpp"
#include "editor/state.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/renderer.hpp"
#include "nvim/input.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/nvim.hpp"
#include "nvim/parse.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/variant.hpp"

using namespace wgpu;

const WGPUContext& ctx = Window::_ctx;

int main() {
  Nvim nvim(true);
  nvim.StartUi(120, 40);
  // std::vector<std::future<void>> threads;

  Window window({1400, 800}, "Neovim GUI", PresentMode::Fifo);

  Font font("/Library/Fonts/SF-Mono-Medium.otf", 15, 2);
  Renderer renderer(window.size);
  renderer.clearColor = {0.9, 0.5, 0.6, 1.0};
  GridManager gridManager;

  while (!window.ShouldClose()) {
    ctx.device.Tick();

    // events ------------------------------------------------
    if (nvim.client.HasNotification())
      window.PollEvents();
    else
      window.WaitEvents();

    for (auto& event : window.events) {
      std::visit(
        overloaded{
          [&](Window::KeyData& e) {
            auto& [key, scancode, action, mods] = e;
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
              auto string = ConvertKeyInput(key, mods);
              if (string != "") nvim.Input(string);
            }

            if (action == GLFW_PRESS) {
              if (key == GLFW_KEY_F2) {
                for (auto& [id, grid] : gridManager.grids) {
                  LOG("grid: {} ----------------------------", id);
                  for (size_t i = 0; i < grid.lines.Size(); i++) {
                    auto& line = grid.lines[i];
                    std::string lineStr;
                    for (auto& cell : line) {
                      lineStr += cell;
                    }
                    LOG("{}", lineStr);
                  }
                }
              }
            }

            // if (key == GLFW_KEY_ESCAPE) {
            //   threads.emplace_back(std::async(std::launch::async, [&]() mutable {
            //     auto result = nvim.client.Call("nvim_get_current_line");
            //     std::cout << "future result: " << result.get() << std::endl;
            //   }));
            // }
          },
          [&](Window::CharData& e) {
            auto& [codepoint] = e;
            auto string = ConvertCharInput(codepoint);
            if (string != "") nvim.Input(string);
          },
          [&](Window::MouseButtonData& e) {

          },
          [&](Window::CursorPosData& e) {},
          [&](Window::WindowSizeData& e) {

          },
        },
        event
      );
    }

    if (window.ShouldClose()) nvim.client.Disconnect();
    if (!nvim.client.IsConnected()) window.SetShouldClose(true);

    // LOG("\n -----------------------------------------------");
    // using namespace std::chrono_literals;
    // using namespace std::chrono;
    // static Timer timer{60};
    // timer.Start();
    nvim.ParseEvents();
    // timer.End();
    // auto duration = timer.GetAverageDuration();
    // if (duration > 1us) {
    // LOG("\nnumFlushes: {}", nvim.redrawState.numFlushes);
    // LOG("parse_notifications: {}", duration);
    // }

    // std::erase_if(threads, [](const std::future<void>& f) {
    //   return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    // });

    // rendering ------------------------------------------------
    // if using continue, have to lock main loop to 60 fps, or will keep looping
    // and waste cpu cycles
    // else just let webgpu vsync handle timing, but will spend extra power
    // on rendering when there's no need
    // best prob make own timer to lock main loop to 60 fps when using continue
    // if (nvim.redrawState.numFlushes == 0) continue;

    // LOG_DISABLE();
    // size of queue
    // timer.Start();
    for (int i = 0; i < nvim.redrawState.numFlushes; i++) {
      auto& redrawEvents = nvim.redrawState.eventsQueue.front();
      for (auto& event : redrawEvents) {
        std::visit(
          overloaded{
            [&](SetTitle& e) {
              // LOG("set_title");
            },
            [&](SetIcon& e) {
              // LOG("set_icon");
            },
            [&](ModeInfoSet& e) {
              // LOG("mode_info_set");
              for (auto& elem : e.modeInfo) {
                for (auto& [key, value] : elem) {
                  if (value.is_string()) {
                    // LOG("mode_info: {} {}", key, value.as_string());
                  } else if (value.is_uint64_t()) {
                    // LOG("mode_info: {} {}", key, value.as_uint64_t());
                  } else {
                    assert(false);
                  }
                }
              }
            },
            [&](OptionSet& e) {
              // LOG("option_set");
            },
            [&](ModeChange& e) {
              // LOG("mode_change");
            },
            [&](MouseOn&) {
              // LOG("mouse_on");
            },
            [&](MouseOff&) {
              // LOG("mouse_off");
            },
            [&](BusyStart&) {
              // LOG("busy_start");
            },
            [&](BusyStop&) {
              // LOG("busy_stop");
            },
            [&](UpdateMenu&) {
              // LOG("update_menu");
            },
            [&](DefaultColorsSet& e) {
              // LOG("default_colors_set");
            },
            [&](Flush&) {
              // LOG("flush");
            },
            [&](GridResize& e) {
              // LOG("grid_resize");
              gridManager.Resize(e);
            },
            [&](GridClear& e) {
              // LOG("grid_clear");
              gridManager.Clear(e);
            },
            [&](GridCursorGoto& e) {
              // LOG("grid_cursor_goto");
              gridManager.CursorGoto(e);
            },
            [&](GridLine& e) {
              // LOG("grid_line");
              gridManager.Line(e);
            },
            [&](GridScroll& e) {
              // LOG("grid_scroll");
              gridManager.Scroll(e);
            },
            [&](GridDestroy& e) {
              // LOG("grid_destroy");
              gridManager.Destroy(e);
            },
            [&](auto&) { LOG("unknown event"); },
          },
          event
        );
      }
      nvim.redrawState.eventsQueue.pop_front();
    }
    // LOG_ENABLE();
    // timer.End();
    // auto duration = duration_cast<nanoseconds>(timer.GetAverageDuration());
    // LOG("rendering: {}", duration);

    renderer.Begin();
    for (auto& [id, grid] : gridManager.grids) {
      // if (grid.empty) continue;
      // LOG("render grid: {}", id);
      renderer.RenderGrid(grid, font);
    }
    renderer.End();
    renderer.Present();
  }
}
