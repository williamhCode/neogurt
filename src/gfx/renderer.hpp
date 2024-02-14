#pragma once

#include "webgpu/webgpu_cpp.h"

struct Renderer {
  wgpu::CommandEncoder commandEncoder;
  wgpu::TextureView nextTexture;

  Renderer();

  void Begin();

  void Clear(const wgpu::Color& color);

  void End();
  void Present();
};
