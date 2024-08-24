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
  glm::vec2 regionCoord; // region in the font texture
  glm::vec4 foreground;
};

// underline, strikethrough, undercurl, etc.
struct LineQuadVertex {
  glm::vec2 position;
  glm::vec2 coord;
  glm::vec4 color;
  uint32_t lineType;
};

struct TextMaskQuadVertex {
  glm::vec2 position;
  glm::vec2 regionCoord; // region in the font texture
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
  wgpu::RenderPipeline textMaskRPL;

  wgpu::RenderPipeline lineRPL;

  wgpu::BindGroupLayout textureBGL;
  wgpu::BindGroupLayout defaultColorBGL;
  wgpu::RenderPipeline textureNoBlendRPL;
  wgpu::RenderPipeline textureRPL;

  wgpu::RenderPipeline finalTextureRPL;

  wgpu::BindGroupLayout cursorMaskPosBGL;
  wgpu::RenderPipeline cursorRPL;

  Pipeline() = default;
  Pipeline(const WGPUContext& ctx);
};
