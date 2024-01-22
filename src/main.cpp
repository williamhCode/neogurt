#include "gfx/context.hpp"
#include "gfx/window.hpp"

using namespace wgpu;

int main() {
  Window window({1200, 800}, "Neovim GUI", PresentMode::Fifo);

  while (!window.ShouldClose()) {
    ctx.device.Tick();

    window.PollEvents();
    for (auto &[key, scancode, action, modes] : window.keyCallbacks) {
      if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
          window.SetShouldClose(true);
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
