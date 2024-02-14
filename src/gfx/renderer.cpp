#include "renderer.hpp"
#include "gfx/instance.hpp"

using namespace wgpu;

Renderer::Renderer() {
}

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  nextTexture = ctx.swapChain.GetCurrentTextureView();
}

void Renderer::Clear(const wgpu::Color& color) {
  RenderPassColorAttachment colorAttachment{
    .view = nextTexture,
    .loadOp = LoadOp::Clear,
    .storeOp = StoreOp::Store,
    .clearValue = color,
  };
  RenderPassDescriptor renderPassDesc{
    .colorAttachmentCount = 1,
    .colorAttachments = &colorAttachment,
  };
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&renderPassDesc);
  passEncoder.End();
}

void Renderer::End() {


  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
}

void Renderer::Present() {
  ctx.swapChain.Present();
}
