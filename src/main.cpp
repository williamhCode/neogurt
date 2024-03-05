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
#include "utils/variant.hpp"

using namespace wgpu;

const WGPUContext& ctx = Window::_ctx;

int main() {
  // std::vector<std::future<void>> threads;

  Window window({1400, 800}, "Neovim GUI", PresentMode::Fifo);
  Renderer renderer(window.size);
  Font font("/Library/Fonts/SF-Mono-Medium.otf", 15, 2);

  int width = window.size.x / font.charWidth;
  int height = window.size.y / font.charHeight;

  Nvim nvim(true);
  nvim.UiAttach(width, height);

  EditorState editorState{};

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
                for (auto& [id, grid] : editorState.gridManager.grids) {
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
          [&](Window::MouseButtonData&) {

          },
          [&](Window::CursorPosData&) {
          },
        },
        event
      );
    }

    // seperate logic for window size event, else it builds up when resizing
    if (window.windowSizeEvent.has_value()) {
      auto [width, height] = window.windowSizeEvent.value();
      int widthChars = width / font.charWidth;
      int heightChars = height / font.charHeight;
      nvim.UiTryResize(widthChars, heightChars);
      renderer.Resize({width, height});
      LOG("resize: {} {}", width, height);
      window.windowSizeEvent.reset();
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

    // parse redraw and rendering ------------------------------------------------
    // if using continue, have to lock main loop to 60 fps, or will keep looping
    // and waste cpu cycles
    // else just let webgpu vsync handle timing, but will spend extra power
    // on rendering when there's no need
    // best prob make own timer to lock main loop to 60 fps when using continue
    // if (nvim.redrawState.numFlushes == 0) continue;

    // LOG_DISABLE();
    // size of queue
    // timer.Start();
    ProcessRedrawEvents(nvim.redrawState, editorState);
    // LOG_ENABLE();
    // timer.End();
    // auto duration = duration_cast<nanoseconds>(timer.GetAverageDuration());
    // LOG("rendering: {}", duration);

    auto color = editorState.hlTable.at(0).background.value();
    renderer.clearColor = {color.r, color.g, color.b, color.a};
    renderer.Begin();
    for (auto& [id, grid] : editorState.gridManager.grids) {
      // if (grid.empty) continue;
      // LOG("render grid: {}", id);
      renderer.RenderGrid(grid, font, editorState.hlTable);
    }
    renderer.End();
    renderer.Present();
  }
}
