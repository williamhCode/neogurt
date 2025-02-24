#pragma once

#include "./glyph_info.hpp"
#include "./shape_pen.hpp"
#include "./texture_atlas.hpp"
#include "glm/ext/vector_float2.hpp"
#include <mdspan>
#include <unordered_map>

void PopulateBoxChars();

struct ShapeDrawing {
  shape::Pen pen;

  using GlyphInfoMap = std::unordered_map<char32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;

  ShapeDrawing() = default;
  ShapeDrawing(glm::vec2 charSize, float dpiScale);

  // returns nullptr if not implemented
  const GlyphInfo* GetGlyphInfo(char32_t charcode, TextureAtlas<false>& textureAtlas);

  // std::chrono::nanoseconds totalTime = std::chrono::nanoseconds(0);
};
