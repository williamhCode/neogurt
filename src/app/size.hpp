#pragma once

#include "glm/ext/vector_float2.hpp"
#include "app/options.hpp"

struct SizeHandler {
  glm::vec2 size;
  glm::vec2 fbSize;
  glm::vec2 uiSize;
  glm::vec2 uiFbSize;
  glm::vec2 charSize;
  glm::vec2 charFbSize;
  int uiWidth;
  int uiHeight;
  float dpiScale;

  glm::vec2 offset;
  glm::vec2 fbOffset;

  void UpdateSizes(glm::vec2 size, float dpiScale, glm::vec2 charSize, const Options& options);
};
