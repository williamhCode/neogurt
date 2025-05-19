#pragma once

#include "./glyph_info.hpp"
#include "./shape_pen.hpp"
#include "./texture_atlas.hpp"
#include "editor/highlight.hpp"
#include "glm/ext/vector_float2.hpp"
#include <string>
#include <mdspan>
#include <unordered_map>

void PopulateBoxChars();

struct ShapeDrawing {
  shape::Pen pen;

  using GlyphInfoMap = std::unordered_map<char32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;

  using UnderlineGlyphInfoMap = std::unordered_map<UnderlineType, GlyphInfo>;
  UnderlineGlyphInfoMap underlineGlyphInfoMap;

  ShapeDrawing() = default;
  ShapeDrawing(glm::vec2 charSize, float dpiScale);

  // returns nullptr if not implemented
  const GlyphInfo*
  GetGlyphInfo(const std::string& text, TextureAtlas<false>& textureAtlas);

  const GlyphInfo*
  GetGlyphInfo(UnderlineType underlineType, TextureAtlas<false>& textureAtlas);
};
