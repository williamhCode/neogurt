#pragma once

#include "freetype/freetype.h"
#include "glm/ext/vector_int2.hpp"
#include "gfx/texture.hpp"

struct Font {
  static inline FT_Library library;
  static inline bool ftInitialized = false;

  int size;  // font size

  // texture related
  TextureHandle texture;
  static constexpr int atlasWidth = 16;
  int atlasHeight;

  struct GlyphInfo {
    glm::ivec2 size;
    glm::ivec2 bearing;
    int advance;
    glm::ivec2 position;
  };
  std::unordered_map<uint32_t, GlyphInfo> glyphInfoMap;

  Font(const std::string& path, int size, int ratio);
};
