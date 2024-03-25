#pragma once

#include "webgpu/webgpu_cpp.h"
#include "gfx/camera.hpp"
#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"

struct RenderTexture {
  Ortho2D camera;

  wgpu::Texture texture;
  wgpu::TextureView textureView;
  wgpu::BindGroup textureBG;

  QuadRenderData<TextureQuadVertex> renderData;

  RenderTexture() = default;
  RenderTexture(
    glm::vec2 pos, glm::vec2 size, float dpiScale, wgpu::TextureFormat format
  );
  void Resize(glm::vec2 pos, glm::vec2 size, float dpiScale);
};
