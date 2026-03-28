#pragma once
#include "utils/region.hpp"

struct GlyphInfo {
  Region localPoss;   // relative position to ascender (aside from box drawing)
  Region atlasRegion; // position in texture atlas
  bool useAscender = true;
  bool isEmoji = false;
};

struct ShapedGlyph {
  const GlyphInfo* glyphInfo; // null = blank/skip
  int numCells;               // >1 for true liga, always 1 for calt
  float xOffset;              // HarfBuzz x_offset in pixels — needed for calt shifting
};
