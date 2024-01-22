#include "gfx/context.hpp"
#include "gfx/window.hpp"

#include "asio.hpp"
#include <iostream>
#include <thread>

using namespace wgpu;

static void AsioTest() {
  asio::error_code ec;
  asio::io_context context;
  asio::io_context::work idleWork(context);
  std::thread thrContext = std::thread([&]() { context.run(); });

  asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1", ec), 80);
  asio::ip::tcp::socket socket(context);
  socket.connect(endpoint, ec);

  if (!ec) {
    std::cout << "Connected!" << std::endl;
  } else {
    std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
  }
}

int main() {
  AsioTest();
  return 0;

  // Window window({1200, 800}, "Neovim GUI", PresentMode::Fifo);

  // while (!window.ShouldClose()) {
  //   ctx.device.Tick();

  //   window.PollEvents();
  //   for (auto &[key, scancode, action, modes] : window.keyCallbacks) {
  //     if (action == GLFW_PRESS) {
  //       if (key == GLFW_KEY_ESCAPE) {
  //         window.SetShouldClose(true);
  //       }
  //     }
  //   }

  //   TextureView nextTexture = ctx.swapChain.GetCurrentTextureView();
  //   RenderPassColorAttachment colorAttachment{
  //     .view = nextTexture,
  //     .loadOp = LoadOp::Clear,
  //     .storeOp = StoreOp::Store,
  //     .clearValue = {0.9, 0.5, 0.6, 1.0},
  //   };
  //   RenderPassDescriptor renderPassDesc{
  //     .colorAttachmentCount = 1,
  //     .colorAttachments = &colorAttachment,
  //   };

  //   CommandEncoder commandEncoder = ctx.device.CreateCommandEncoder();
  //   {
  //     RenderPassEncoder passEncoder =
  //     commandEncoder.BeginRenderPass(&renderPassDesc); passEncoder.End();
  //   }
  //   CommandBuffer commandBuffer = commandEncoder.Finish();

  //   ctx.queue.Submit(1, &commandBuffer);
  //   ctx.swapChain.Present();
  // }
}
