#pragma once

#include "editor/highlight.hpp"
#include "gfx/render_texture.hpp"

struct ShapesManager {
  // stores textures of prerendered shapes (underline, braille, box drawing)
  wgpu::Buffer textureSizeBuffer;
  wgpu::BindGroup textureSizeBG;

  // atlas
  RenderTexture renderTexture;

  struct ShapeInfo {
    Region localPoss;
    Region atlasRegion;
  };
  std::array<ShapeInfo, 5> infoArray;

  bool dirty = true;

  ShapesManager() = default;
  ShapesManager(glm::vec2 charSize, float dpiScale);

  inline const ShapeInfo& GetUnderlineInfo(UnderlineType underlineType) {
    return infoArray[static_cast<int>(underlineType)];
  }
};
