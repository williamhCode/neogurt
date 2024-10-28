#pragma once
#include "gfx/glyph_info.hpp"
#include "gfx/pen.hpp"
#include "gfx/texture_atlas.hpp"
#include "glm/ext/vector_float2.hpp"
#include <mdspan>
#include <unordered_map>

void PopulateBoxChars();

struct BoxDrawing {
  box::Pen pen;

  using GlyphInfoMap = std::unordered_map<char32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;

  BoxDrawing() = default;
  BoxDrawing(glm::vec2 charSize, float dpiScale);

  // returns nullptr if not implemented
  const GlyphInfo* GetGlyphInfo(char32_t charcode, TextureAtlas& textureAtlas);

  // std::chrono::nanoseconds totalTime = std::chrono::nanoseconds(0);
};
