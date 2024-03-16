#pragma once

#include <array>
#include "glm/ext/vector_float2.hpp"

using Region = std::array<glm::vec2, 4>;

inline Region MakeRegion(float x, float y, float width, float height) {
  return {
    glm::vec2(x, y),
    glm::vec2(x + width, y),
    glm::vec2(x + width, y + height),
    glm::vec2(x, y + height),
  };
}

inline Region MakeRegion(glm::vec2 pos, glm::vec2 size) {
  return {
    pos,
    pos + glm::vec2(size.x, 0),
    pos + size,
    pos + glm::vec2(0, size.y),
  };
}
