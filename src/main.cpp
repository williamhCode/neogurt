#include "gfx/context.hpp"
#include "gfx/window.hpp"
#include "nvim/msgpack_rpc/client.hpp"

#include <iostream>

using namespace wgpu;

int main() {
  Client client("localhost", 6666);
  client.Send("nvim_ui_attach", 100, 100, std::tuple());

  Window window({1200, 800}, "Neovim GUI", PresentMode::Fifo);

  while (!window.ShouldClose()) {
    // if nvim disconnects, close the window
    if (!client.IsConnected()) window.SetShouldClose(true);

    ctx.device.Tick();

    window.PollEvents();
    for (auto& [key, scancode, action, modes] : window.keyCallbacks) {
      if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
          window.SetShouldClose(true);
          client.Disconnect();
        }
        if (key == GLFW_KEY_J) {
          client.Send("nvim_input", std::string("j"));
        }
        if (key == GLFW_KEY_K) {
          client.Send("nvim_input", std::string("k"));
        }
        if (key == GLFW_KEY_T) {
          client.AsyncCall("nvim_get_current_line");
        }
        if (key == GLFW_KEY_C) {
          std::cout << client.IsConnected() << std::endl;
        }
      }
    }

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
