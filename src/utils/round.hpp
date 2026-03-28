#pragma once
#include "glm/ext/vector_float2.hpp"
#include "glm/common.hpp"
#include <cmath>

inline float RoundToPixel(float val, float scale) {
  return std::round(val * scale) / scale;
}

inline glm::vec2 RoundToPixel(glm::vec2 val, float scale) {
  return glm::round(val * scale) / scale;
}
