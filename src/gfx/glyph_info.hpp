#pragma once
#include "utils/region.hpp"

struct GlyphInfo {
  Region localPoss;   // relative position to ascender (aside from box drawing)
  Region atlasRegion; // position in texture atlas
  bool useAscender = true;
};

