#include "gfx/context.hpp"
#include "gfx/window.hpp"
#include "nvim/msgpack_rpc/client.hpp"

#include <iostream>
#include "utf8/unchecked.h"

using namespace wgpu;

std::string unicodeToUTF8(unsigned int unicode) {
  std::string utf8String;
  utf8::unchecked::append(unicode, std::back_inserter(utf8String));
  return utf8String;
}

int main() {
  rpc::Client client("localhost", 6666);
  client.Send("nvim_ui_attach", 100, 100, std::tuple());

  Window window({1200, 800}, "Neovim GUI", PresentMode::Fifo);

  std::vector<std::future<void>> threads;

  while (!window.ShouldClose()) {
    ctx.device.Tick();

    // events ------------------------------------------------
    window.PollEvents();

    for (auto& [key, scancode, action, modes] : window.keyCallbacks) {
      if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_UNKNOWN) {
          // window.SetShouldClose(true);
        } else if (key == GLFW_KEY_A) {
          client.AsyncCall("nvim_ui_attach", 100, 100, std::tuple());
        } else if (key == GLFW_KEY_ESCAPE) {
          auto future = client.AsyncCall("nvim_get_current_line");
          threads.emplace_back(std::async(
            std::launch::async,
            [future = std::move(future)]() mutable {
              auto result = future.get();
              std::cout << "future result: " << result.get() << std::endl;
            }
          ));
        }
      }
    }

    for (auto& [codepoint] : window.charCallbacks) {
      auto string = unicodeToUTF8(codepoint);
      client.Send("nvim_input", string);
    }

    if (window.ShouldClose()) client.Disconnect();
    if (!client.IsConnected()) window.SetShouldClose(true);

    // get info and stuff ---------------------------------------
    while (client.HasNotification()) {
      auto notification = client.PopNotification();
      // std::cout << "\n\n---------------------------------" << std::endl;
      // std::cout << "method: " << notification.method << std::endl;
      // std::cout << "params: " << notification.params.get() << std::endl;
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
