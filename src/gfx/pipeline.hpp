#pragma once

#include <webgpu/webgpu_cpp.h>
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"

struct WGPUContext;

struct TextQuadVertex {
  glm::vec2 position;
  glm::vec2 uv;
  glm::vec4 foreground;
};

struct RectQuadVertex {
  glm::vec2 position;
  glm::vec4 color;
};

struct Pipeline {
  wgpu::BindGroupLayout viewProjBGL;

  wgpu::RenderPipeline rectRPL;

  wgpu::BindGroupLayout fontTextureBGL;
  wgpu::RenderPipeline textRPL;

  wgpu::BindGroupLayout maskBGL;
  wgpu::RenderPipeline cursorRPL;

  Pipeline() = default;
  Pipeline(const WGPUContext& ctx);
};
