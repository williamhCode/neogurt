#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"

struct WGPUContext;

struct RectQuadVertex {
  glm::vec2 position;
  glm::vec4 color;
};

struct TextQuadVertex {
  glm::vec2 position;
  glm::vec2 regionCoords; // region in the font texture
  glm::vec4 foreground;
};

struct TextureQuadVertex {
  glm::vec2 position;
  glm::vec2 uv;
};

struct CursorQuadVertex {
  glm::vec2 position;
  glm::vec4 foreground;
  glm::vec4 background;
};

struct Pipeline {
  wgpu::BindGroupLayout viewProjBGL;

  wgpu::RenderPipeline rectRPL;

  wgpu::BindGroupLayout fontTextureBGL;
  wgpu::RenderPipeline textRPL;

  wgpu::BindGroupLayout textureBGL;
  wgpu::RenderPipeline textureNoBlendRPL;
  wgpu::RenderPipeline textureRPL;

  wgpu::RenderPipeline finalTextureRPL;

  wgpu::BindGroupLayout maskBGL;
  wgpu::RenderPipeline cursorRPL;

  Pipeline() = default;
  Pipeline(const WGPUContext& ctx);
};
