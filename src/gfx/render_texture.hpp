#pragma once

#include "webgpu/webgpu_cpp.h"

#include "gfx/pipeline.hpp"
#include "gfx/quad.hpp"
#include "glm/ext/vector_uint2.hpp"

struct RenderTexture {
  wgpu::Buffer viewProjBuffer;
  wgpu::BindGroup viewProjBG;

  wgpu::Texture texture;
  wgpu::TextureView textureView;
  wgpu::BindGroup textureBG;

  QuadRenderData<TextureQuadVertex> textureData;

  RenderTexture(glm::uvec2 size, wgpu::TextureFormat format);
  void Resize(glm::uvec2 size);
};
