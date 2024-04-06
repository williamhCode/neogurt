#pragma once

#include "utils/region.hpp"
#include "webgpu/webgpu_cpp.h"
#include "gfx/camera.hpp"
#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"
#include <optional>

struct RenderTexture {
  Ortho2D camera;

  glm::vec2 size;
  wgpu::Texture texture;
  wgpu::TextureView textureView;
  wgpu::BindGroup textureBG;

  QuadRenderData<TextureQuadVertex> renderData;

  RenderTexture() = default;
  RenderTexture(glm::vec2 size, float dpiScale, wgpu::TextureFormat format);

  // region is the subregion of the texture to draw
  void UpdatePos(glm::vec2 pos, std::optional<RegionHandle> region = std::nullopt);
};
