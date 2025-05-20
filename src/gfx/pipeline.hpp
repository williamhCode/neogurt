#pragma once

#include "slang_utils/context.hpp"
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
  wgpu::BindGroupLayout textureBGL;

  wgpu::RenderPipeline rectRPL;

  wgpu::BindGroupLayout textureSizeBGL;
  wgpu::RenderPipeline textRPL;
  wgpu::RenderPipeline emojiRPL;
  wgpu::RenderPipeline textMaskRPL;
  wgpu::RenderPipeline emojiMaskRPL;

  wgpu::BindGroupLayout defaultBgLinearBGL;
  wgpu::RenderPipeline textureNoBlendRPL;
  wgpu::RenderPipeline textureRPL;

  wgpu::RenderPipeline textureFinalRPL;

  wgpu::BindGroupLayout cursorMaskPosBGL;
  wgpu::RenderPipeline cursorRPL;
  wgpu::RenderPipeline cursorEmojiRPL;

  Pipeline() = default;
  Pipeline(const WGPUContext& ctx, SlangContext& slang, float gamma);
};
