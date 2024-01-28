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
  Client client("localhost", 6666);
  client.Send("nvim_ui_attach", 100, 100, std::tuple());

  Window window({1200, 800}, "Neovim GUI", PresentMode::Fifo);

  // std::future<msgpack::object_handle> future;

  while (!window.ShouldClose()) {
    ctx.device.Tick();

    // events ------------------------------------------------
    window.PollEvents();

    for (auto& [key, scancode, action, modes] : window.keyCallbacks) {
      if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_ESCAPE) {
          window.SetShouldClose(true);
        } else if (key == GLFW_KEY_A) {
          client.AsyncCall("nvim_ui_attach", 100, 100, std::tuple());
        } else if (key == GLFW_KEY_T) {
          auto future = client.AsyncCall("nvim_get_current_line");
          std::thread thread([&]() {
            auto result = future.get();
            std::cout << "future result: " << result.get() << std::endl;
          });
          thread.join();
        }
      }
    }

    while (client.HasNotification()) {
      auto notification = client.PopNotification();
      std::cout << "---------------------------------\n";
      std::cout << "method: " << notification.method << std::endl;
      std::cout << "params: " << notification.params.get() << std::endl;
    }

    for (auto& [codepoint] : window.charCallbacks) {
      auto string = unicodeToUTF8(codepoint);
      client.Send("nvim_input", string);
    }

    if (window.ShouldClose()) client.Disconnect();
    if (!client.IsConnected()) window.SetShouldClose(true);

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
