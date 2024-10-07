#pragma once
#include "utils/region.hpp"

struct GlyphInfo {
  bool boxDrawing = false;
  Region localPoss;   // relative position to ascender (aside from box drawing)
  Region atlasRegion; // position in texture atlas
};

