#pragma once

#include <array>
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

struct RegionHandle {
  glm::vec2 pos;
  glm::vec2 size;

  [[nodiscard]] Region Get() const {
    return MakeRegion(pos, size);
  }
};
