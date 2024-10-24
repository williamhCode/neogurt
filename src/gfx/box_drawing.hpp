#pragma once
#include "gfx/glyph_info.hpp"
#include "gfx/pen.hpp"
#include "gfx/texture_atlas.hpp"
#include "glm/ext/vector_float2.hpp"
#include <mdspan>
#include <unordered_map>
#include <vector>

void PopulateBoxChars();

struct BoxDrawing {
  glm::vec2 size;
  float dpiScale;

  using GlyphInfoMap = std::unordered_map<char32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;

  int width;
  int height;

  box::Pen pen;

  BoxDrawing() = default;
  BoxDrawing(glm::vec2 size, float dpiScale);

  // returns nullptr if not implemented
  const GlyphInfo* GetGlyphInfo(char32_t charcode, TextureAtlas& textureAtlas);
};
