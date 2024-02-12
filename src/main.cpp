#include "app/window.hpp"
#include "editor/state.hpp"
#include "gfx/instance.hpp"
#include "gfx/font.hpp"
#include "nvim/nvim.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/parse.hpp"
#include "nvim/input.hpp"
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
        case Index<Window::Event, Window::KeyData>(): {
          auto& [key, scancode, action, mods] = std::get<Window::KeyData>(event);
          if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            auto string = KeyInputToString(key, mods);
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
        case Index<Window::Event, Window::CharData>(): {
          auto& [codepoint] = std::get<Window::CharData>(event);
          auto string = CharInputToString(codepoint);
          if (string != "") nvim.Input(string);
          break;
        }
        case Index<Window::Event, Window::MouseButtonData>(): {
          break;
        }
        case Index<Window::Event, Window::CursorPosData>(): {
          break;
        }
      }
    }

    if (window.ShouldClose()) nvim.client.Disconnect();
    if (!nvim.client.IsConnected()) window.SetShouldClose(true);

    // LOG("\n -----------------------------------------------");
    using namespace std::chrono_literals;
    static Timer timer(1);
    timer.Start();
    ParseNotifications(nvim.client);
    timer.End();
    auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(timer.GetAverageDuration());
    if (duration > 5us) {
      LOG("\nnumFlushes: {}", editorState.numFlushes);
      LOG("parse_notifications: {}", duration);
    }

    // std::erase_if(threads, [](const std::future<void>& f) {
    //   return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    // });

    // rendering ------------------------------------------------
    LOG_DISABLE();
    // size of queue
    for (int i = 0; i < editorState.numFlushes; i++) {
      const auto& redrawEvents = editorState.redrawEventsQueue.front();
      for (const auto& event : redrawEvents) {
        switch (event.index()) {
          case Index<RedrawEvent, Flush>(): {
            LOG("flush");
            break;
          }
          case Index<RedrawEvent, GridResize>(): {
            auto& [grid, width, height] = std::get<GridResize>(event);
            LOG("grid_resize");
            break;
          }
          case Index<RedrawEvent, GridClear>(): {
            auto& [grid] = std::get<GridClear>(event);
            LOG("grid_clear");
            break;
          }
          case Index<RedrawEvent, GridCursorGoto>(): {
            auto& [grid, row, col] = std::get<GridCursorGoto>(event);
            LOG("grid_cursor_goto");
            break;
          }
          case Index<RedrawEvent, GridLine>(): {
            auto& [grid, row, colStart, cells] = std::get<GridLine>(event);
            LOG("grid_line");
            break;
          }
        }
      }
      editorState.redrawEventsQueue.pop_front();
    }
    LOG_ENABLE();

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
