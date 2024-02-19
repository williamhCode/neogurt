#pragma once

#include <webgpu/webgpu_cpp.h>
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"

struct WGPUContext;

struct TextQuadVertex {
  glm::vec2 position;
  glm::vec2 uv;
  glm::vec4 foreground;
  glm::vec4 background;
};

struct Pipeline {
  wgpu::BindGroupLayout viewProjBGL;
  wgpu::BindGroupLayout fontTextureBGL;
  wgpu::RenderPipeline textRPL;

  Pipeline() = default;
  Pipeline(const WGPUContext& ctx);
};
