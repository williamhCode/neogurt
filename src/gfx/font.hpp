#pragma once

#include "freetype/freetype.h"
#include "glm/ext/vector_float2.hpp"
#include "gfx/texture.hpp"

struct Font {
  FT_Face face;

  int size; // font size

  // texture related
  wgpu::BindGroup fontTextureBG;
  TextureHandle texture;
  static constexpr int atlasWidth = 16;
  int atlasHeight;

  float charWidth;
  float charHeight;

  struct GlyphInfo {
    // floats because of high dpi
    glm::vec2 size;
    glm::vec2 bearing;
    float advance;
    static inline std::array<glm::vec2, 4> positions;
    std::array<glm::vec2, 4> texCoords;
  };
  std::unordered_map<uint32_t, GlyphInfo> glyphInfoMap;

  Font(const std::string& path, int size, float ratio);
};
