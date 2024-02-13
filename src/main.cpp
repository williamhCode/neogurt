#include "app/window.hpp"
#include "editor/state.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "nvim/input.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/nvim.hpp"
#include "nvim/parse.hpp"
#include "util/logger.hpp"
#include "util/timer.hpp"
#include "util/variant.hpp"

using namespace wgpu;

const WGPUContext& ctx = Window::_ctx;

int main() {
  Nvim nvim(true);
  nvim.StartUi(120, 80);

  Window window({600, 400}, "Neovim GUI", PresentMode::Fifo);
  Font font("/System/Library/Fonts/Supplemental/Arial.ttf", 30, 1);

  // std::vector<std::future<void>> threads;

  while (!window.ShouldClose()) {
    ctx.device.Tick();

    // events ------------------------------------------------
    window.PollEvents();
    for (const auto& event : window.events) {
      switch (event.index()) {
        case vIndex<Window::Event, Window::KeyData>(): {
          auto& [key, scancode, action, mods] = std::get<Window::KeyData>(event);
          if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            auto string = ConvertKeyInput(key, mods);
            if (string != "") nvim.Input(string);

            // if (key == GLFW_KEY_ESCAPE) {
            //   threads.emplace_back(std::async(std::launch::async, [&]() mutable {
            //     auto result = nvim.client.Call("nvim_get_current_line");
            //     std::cout << "future result: " << result.get() << std::endl;
            //   }));
            // }
          }
          break;
        }
        case vIndex<Window::Event, Window::CharData>(): {
          auto& [codepoint] = std::get<Window::CharData>(event);
          auto string = ConvertCharInput(codepoint);
          if (string != "") nvim.Input(string);
          break;
        }
        case vIndex<Window::Event, Window::MouseButtonData>(): {
          break;
        }
        case vIndex<Window::Event, Window::CursorPosData>(): {
          break;
        }
      }
    }

    if (window.ShouldClose()) nvim.client.Disconnect();
    if (!nvim.client.IsConnected()) window.SetShouldClose(true);

    // LOG("\n -----------------------------------------------");
    using namespace std::chrono_literals;
    using namespace std::chrono;
    static Timer timer{60};
    // timer.Start();
    ParseNotifications(nvim.client);
    // timer.End();
    // auto duration = timer.GetAverageDuration();
    // if (duration > 5us) {
      // LOG("\nnumFlushes: {}", editorState.numFlushes);
      // LOG("parse_notifications: {}", duration);
    // }

    // std::erase_if(threads, [](const std::future<void>& f) {
    //   return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    // });

    // rendering ------------------------------------------------
    // LOG_DISABLE();
    // size of queue
    timer.Start();
    for (int i = 0; i < editorState.numFlushes; i++) {
      auto& redrawEvents = editorState.redrawEventsQueue.front();
      for (auto& event : redrawEvents) {
        std::visit(overloaded{
          [&](Flush&) {
            // LOG("flush");
          },
          [&](GridResize& e) {
            auto& [grid, width, height] = e;
            // LOG("grid_resize");
          },
          [&](GridClear& e) {
            auto& [grid] = e;
            // LOG("grid_clear");
          },
          [&](GridCursorGoto& e) {
            auto& [grid, row, col] = e;
            // LOG("grid_cursor_goto");
          },
          [&](GridLine& e) {
            auto& [grid, row, colStart, cells] = e;
            // LOG("grid_line");
          },
          [&](GridScroll& e) {
            auto& [grid, top, bot, left, right, rows, cols] = e;
            // LOG("grid_scroll");
          },
          [&](GridDestroy& e) {
            auto& [grid] = e;
            // LOG("grid_destroy");
          },
        }, event);
      }
      editorState.redrawEventsQueue.pop_front();
    }
    LOG_ENABLE();
    // timer.End();
    // auto duration = duration_cast<nanoseconds>(timer.GetAverageDuration());
    // LOG("rendering: {}", duration);

    TextureView nextTexture = ctx.swapChain.GetCurrentTextureView();
    RenderPassColorAttachment colorAttachment{
      .view = nextTexture,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = {0.9, 0.5, 0.6, 1.0},
    };
    RenderPassDescriptor renderPassDesc{
      .colorAttachmentCount = 1,
      .colorAttachments = &colorAttachment,
    };

    CommandEncoder commandEncoder = ctx.device.CreateCommandEncoder();
    {
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&renderPassDesc);
      passEncoder.End();
    }
    CommandBuffer commandBuffer = commandEncoder.Finish();

    ctx.queue.Submit(1, &commandBuffer);
    ctx.swapChain.Present();
  }
}
