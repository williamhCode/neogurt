#pragma once

#include "glm/ext/vector_float2.hpp"
#include <tuple>

struct SizeHandler {
  glm::vec2 size;
  glm::vec2 fbSize;
  glm::vec2 uiSize;
  glm::vec2 uiFbSize;
  glm::vec2 charSize;
  glm::vec2 uiWidthHeight;
  float dpiScale;

  glm::vec2 offset;

  void UpdateSizes(glm::vec2 size, float dpiScale, glm::vec2 charSize);
  std::tuple<int, int> GetUiWidthHeight();
};
