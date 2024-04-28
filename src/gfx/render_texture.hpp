#pragma once

#include "utils/region.hpp"
#include "webgpu/webgpu_cpp.h"
#include "gfx/camera.hpp"
#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"

struct RenderTexture {
  Ortho2D camera;

  glm::vec2 size;
  wgpu::Texture texture;
  wgpu::TextureView textureView;
  wgpu::BindGroup textureBG;

  QuadRenderData<TextureQuadVertex> renderData;

  RenderTexture() = default;
  RenderTexture(
    glm::vec2 size,
    float dpiScale,
    wgpu::TextureFormat format,
    const void* data = nullptr
  );

  // pos is the position (top left) of the texture in the screen
  // region is the subregion of the texture to draw
  void UpdatePos(glm::vec2 pos, RegionHandle* region = nullptr);
};
