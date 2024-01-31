#include "gfx/context.hpp"
#include "editor/window.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "editor/input.hpp"

#include <iostream>
#include "nvim/nvim.hpp"

using namespace wgpu;

int main() {
  Window window({600, 400}, "Neovim GUI", PresentMode::Fifo);

  Nvim nvim(true);
  nvim.StartUi(120, 80);

  std::vector<std::future<void>> threads;

  while (!window.ShouldClose()) {
    ctx.device.Tick();

    // events ------------------------------------------------
    window.PollEvents();
    for (const auto& event : window.events) {
      switch (event.type) {
        case Window::EventType::Key: {
          auto& [key, scancode, action, mods] = std::get<Window::KeyData>(event.data);
          if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            auto string = KeyInput(key, mods);
            // std::cout << "key: " << string << std::endl;
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
        case Window::EventType::Char: {
          auto& [codepoint] = std::get<Window::CharData>(event.data);
          auto string = CharInput(codepoint);
          if (string != "") nvim.Input(string);
          break;
        }
        case Window::EventType::MouseButton: {
          break;
        }
        case Window::EventType::CursorPos: {
          break;
        }
      }
    }

    if (window.ShouldClose()) nvim.client.Disconnect();
    if (!nvim.client.IsConnected()) window.SetShouldClose(true);

    // get info and stuff ---------------------------------------
    while (nvim.client.HasNotification()) {
      auto notification = nvim.client.PopNotification();
      std::cout << "\n\n---------------------------------" << std::endl;
      std::cout << "method: " << notification.method << std::endl;
      std::cout << "params: " << notification.params.get() << std::endl;
    }

    std::erase_if(threads, [](const std::future<void>& f) {
      return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    });

    // rendering ------------------------------------------------
    // TODO: figure out why sometimes WebGPU segfaults when beginning to render
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
