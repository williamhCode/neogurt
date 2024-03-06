#pragma once

#include "freetype/freetype.h"
#include "glm/ext/vector_float2.hpp"
#include "gfx/texture.hpp"

struct Font {
  FT_Face face;

  int size;    // font size
  float ratio; // high dpi

  // texture related
  struct Color {
    uint8_t r, g, b, a;
  };
  std::vector<Color> textureData; // cpu side texture data
  TextureHandle texture;
  wgpu::BindGroup fontTextureBG;
  bool dirty = true;

  // dimension
  static constexpr int atlasWidth = 16;
  int atlasHeight;
  glm::vec2 textureSize;
  glm::vec2 bufferSize;

  float charWidth;
  float charHeight;

  uint32_t defaultGlyphIndex;

  struct GlyphInfo {
    // floats because of high dpi
    glm::vec2 size;
    glm::vec2 bearing;
    float advance;
    glm::vec2 pos;
    static inline std::array<glm::vec2, 4> positions;
    // std::array<glm::vec2, 4> texCoords;
  };
  using GlyphInfoMap = std::unordered_map<uint32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;
  GlyphInfoMap nerdGlyphInfoMap;
    
  Font(const std::string& path, int size, float ratio);
  GlyphInfoMap::iterator AddOrGetGlyphInfo(FT_ULong charcode);
  const GlyphInfo& GetGlyphInfo(FT_ULong charcode);
  void UpdateTexture();
};
