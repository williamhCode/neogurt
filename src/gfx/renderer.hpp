#pragma once

#include "editor/grid.hpp"
#include "gfx/font.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"
#include "webgpu_utils/webgpu.hpp"

struct Renderer {
  wgpu::Color clearColor;
  wgpu::CommandEncoder commandEncoder;
  wgpu::TextureView nextTexture;

  // shared
  wgpu::Buffer viewProjBuffer;
  wgpu::BindGroup viewProjBG;

  // text
  wgpu::Buffer textVertexBuffer;
  wgpu::Buffer textIndexBuffer;
  wgpu::utils::RenderPassDescriptor textRenderPassDesc;

  Renderer(glm::uvec2 size);

  void Begin();
  void RenderGrid(const Grid& grid, const Font& font);
  void End();
  void Present();
};
