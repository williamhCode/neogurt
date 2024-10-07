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

  // temporary storage for drawing
  std::vector<uint8_t> dataRaw;
  std::mdspan<uint8_t, std::dextents<size_t, 2>> data;

  box::Pen pen;

  BoxDrawing() = default;
  BoxDrawing(glm::vec2 size, float dpiScale);

  // returns nullptr if not implemented
  const GlyphInfo* GetGlyphInfo(char32_t charcode, TextureAtlas& textureAtlas);
};
