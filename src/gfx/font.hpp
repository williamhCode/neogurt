#pragma once

#include "freetype/freetype.h"
#include "glm/ext/vector_float2.hpp"
#include "utils/region.hpp"
#include "webgpu/webgpu_cpp.h"
#include <vector>

struct Font {
  FT_Face face;

  int size; // font size
  int trueSize; // font size in pixels
  float dpiScale;

  // texture related
  struct Color {
    uint8_t r, g, b, a;
  };
  std::vector<Color> textureData;
  wgpu::TextureView textureView;
  wgpu::BindGroup fontTextureBG;
  bool dirty = true;

  // dimensions
  static constexpr int atlasWidth = 16;
  int atlasHeight;
  wgpu::Buffer textureSizeBuffer;
  glm::vec2 textureSize;
  glm::vec2 bufferSize;
  glm::vec2 charSize;
  glm::vec2 texCharSize;  // size in texture;

  Region positions; // positions of untranslated character quad

  struct GlyphInfo {
    // floats because of high dpi
    // glm::vec2 size;
    glm::vec2 bearing;
    // float advance;
    Region region;
  };
  using GlyphInfoMap = std::unordered_map<uint32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;
  GlyphInfoMap nerdGlyphInfoMap;

  Font() = default;
  Font(const std::string& path, int size, float dpiScale);
  const GlyphInfo& GetGlyphInfoOrAdd(FT_ULong charcode);
  void UpdateTexture();
  void DpiChanged(float ratio);
};
