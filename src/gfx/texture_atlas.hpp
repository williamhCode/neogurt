#pragma once
#include "gfx/render_texture.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "utils/region.hpp"
#include "utils/types.hpp"
#include "webgpu/webgpu_cpp.h"
#include <cstdint>
#include <vector>
#include <mdspan>

// texture atlas for storing glyphs
// width is constant, size expands vertically
struct TextureAtlas {
  // bufferSize.x = glyphsPerRow * glyphSize
  static constexpr uint glyphsPerRow = 16;

  float dpiScale;
  uint trueGlyphSize; // only used for approximate scale, no precision needed
  glm::vec2 textureSize; // virtual size of texture (used in shader)
  glm::uvec2 bufferSize; // size of texture in texels

  // texture data
  struct Color {
    uint8_t r, g, b, a;
  };
  std::vector<Color> dataRaw;
  std::mdspan<Color, std::dextents<uint, 2>> data;
  bool dirty = false;

  glm::uvec2 currentPos = {0, 0};
  // max height of glyph in current row
  // advance pos.y by this value when moving to next row
  uint currMaxHeight = 0;

  wgpu::Buffer textureSizeBuffer;
  wgpu::BindGroup textureSizeBG;
  RenderTexture renderTexture;
  bool resized = false;

  TextureAtlas() = default;
  TextureAtlas(float glyphSize, float dpiScale);

  // Adds data to texture atlas, and returns the region where the data was added.
  // Region coordinates is relative to textureSize.
  Region AddGlyph(std::mdspan<uint8_t, std::dextents<uint, 2>> glyphData);
  // Resize cpu side data and sizes
  void Resize();
  // Resize gpu side data and update bind group
  void Update();
};
