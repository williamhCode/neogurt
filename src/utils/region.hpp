#pragma once

#include <array>
#include "utils/round.hpp"
#include "glm/common.hpp"
#include "glm/ext/vector_float2.hpp"

using Region = std::array<glm::vec2, 4>;

inline Region MakeRegion(glm::vec2 pos, glm::vec2 size) {
  return {
    pos,
    pos + glm::vec2(size.x, 0),
    pos + size,
    pos + glm::vec2(0, size.y),
  };
}

struct Rect {
  glm::vec2 pos;
  glm::vec2 size;

  Region Region() const {
    return MakeRegion(pos, size);
  }

  float Top() const {
    return pos.y;
  }

  float Bottom() const {
    return pos.y + size.y;
  }

  float Left() const {
    return pos.x;
  }

  float Right() const {
    return pos.x + size.x;
  }

  // rounds rect to nearest pixel
  void RoundToPixel(float dpiScale) {
    pos.x = ::RoundToPixel(pos.x, dpiScale);
    pos.y = ::RoundToPixel(pos.y, dpiScale);
    size.x = ::RoundToPixel(size.x, dpiScale);
    size.y = ::RoundToPixel(size.y, dpiScale);
  }
};

// inline bool RectIntersect(const Rect& a, const Rect& b) {
//   return a.Left() < b.Right() && a.Right() > b.Left() &&
//          a.Top() < b.Bottom() && a.Bottom() > b.Top();
// }

