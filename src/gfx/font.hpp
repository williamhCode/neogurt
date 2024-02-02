#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_int2.hpp"

struct Font {
  // number of glyphs in a row
  static constexpr int atlasWidth = 16;
  int atlasHeight;

  int size;
  wgpu::Texture texture;

  struct GlyphInfo {
    glm::ivec2 size;
    glm::ivec2 bearing;
    int advance;
    glm::ivec2 position;
  };
  std::unordered_map<uint32_t, GlyphInfo> glyphInfoMap;

  Font(const std::string& path, int size, int ratio);
};
